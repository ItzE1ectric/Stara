#include "Hooks.hpp"
#include "Game.hpp"

extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND, UINT, WPARAM, LPARAM);

// Global state
namespace Stara {
    extern bool g_menuVisible;
    extern void RenderMenu();
}

namespace Stara {

// Get swapchain vtable via dummy device
static void* GetSwapChainVTable() {
    WNDCLASSEXA wc = { sizeof(WNDCLASSEX), CS_CLASSDC, DefWindowProc, 0L, 0L, GetModuleHandle(nullptr), nullptr, nullptr, nullptr, nullptr, "DummyDX", nullptr };
    RegisterClassExA(&wc);
    HWND hwnd = CreateWindowA("DummyDX", "", WS_OVERLAPPEDWINDOW, 0, 0, 100, 100, nullptr, nullptr, wc.hInstance, nullptr);

    DXGI_SWAP_CHAIN_DESC sd{};
    sd.BufferCount = 1;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.Width = 1;
    sd.BufferDesc.Height = 1;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hwnd;
    sd.SampleDesc.Count = 1;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    ID3D11Device* dev = nullptr;
    IDXGISwapChain* sc = nullptr;
    ID3D11DeviceContext* ctx = nullptr;
    D3D_FEATURE_LEVEL fl;

    HRESULT hr = D3D11CreateDeviceAndSwapChain(
        nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0,
        nullptr, 0, D3D11_SDK_VERSION,
        &sd, &sc, &dev, &fl, &ctx
    );

    DestroyWindow(hwnd);
    UnregisterClassA("DummyDX", wc.hInstance);

    if (FAILED(hr)) {
        printf("[-] D3D11CreateDeviceAndSwapChain failed: 0x%lX\n", hr);
        return nullptr;
    }

    static void* vtable[256];
    memcpy(vtable, *(void***)sc, sizeof(vtable));

    sc->Release();
    dev->Release();
    ctx->Release();

    return vtable;
}

LRESULT __stdcall Hooks::hkWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    if (msg == WM_KEYDOWN && wp == VK_INSERT) {
        g_menuVisible = !g_menuVisible;
        return 0;
    }

    if (g_menuVisible) {
        ImGui_ImplWin32_WndProcHandler(hwnd, msg, wp, lp);
        // Block game input when menu is open
        if (msg >= WM_MOUSEFIRST && msg <= WM_MOUSELAST) return 0;
        if (msg >= WM_KEYFIRST && msg <= WM_KEYLAST) return 0;
    }

    return CallWindowProcA(s_origWndProc, hwnd, msg, wp, lp);
}

HRESULT __stdcall Hooks::hkPresent(IDXGISwapChain* sc, UINT sync, UINT flags) {
    if (!s_imguiInit) {
        sc->GetDevice(__uuidof(ID3D11Device), (void**)&s_device);
        s_device->GetImmediateContext(&s_context);
        s_swapChain = sc;

        DXGI_SWAP_CHAIN_DESC desc;
        sc->GetDesc(&desc);
        s_hwnd = desc.OutputWindow;

        // Create RTV
        ID3D11Texture2D* backBuf = nullptr;
        sc->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&backBuf);
        s_device->CreateRenderTargetView(backBuf, nullptr, &s_rtv);
        backBuf->Release();

        // Init ImGui
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        io.IniFilename = nullptr;

        ImGui_ImplWin32_Init(s_hwnd);
        ImGui_ImplDX11_Init(s_device, s_context);

        // Subclass WndProc
        s_origWndProc = (WNDPROC)SetWindowLongPtrA(s_hwnd, GWLP_WNDPROC, (LONG_PTR)hkWndProc);

        s_imguiInit = true;
    }

    // Update game state
    Game::Update();

    // Begin ImGui frame
    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    // Render ESP
    Game::DrawESP(ImGui::GetForegroundDrawList());

    // Render our menu
    RenderMenu();

    // Finish
    ImGui::Render();
    s_context->OMSetRenderTargets(1, &s_rtv, nullptr);
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

    return s_origPresent(sc, sync, flags);
}

HRESULT __stdcall Hooks::hkResizeBuffers(IDXGISwapChain* sc, UINT count, UINT w, UINT h, DXGI_FORMAT fmt, UINT flags) {
    if (s_rtv) { s_rtv->Release(); s_rtv = nullptr; }

    HRESULT hr = s_origResize(sc, count, w, h, fmt, flags);

    ID3D11Texture2D* backBuf = nullptr;
    sc->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&backBuf);
    if (backBuf) {
        s_device->CreateRenderTargetView(backBuf, nullptr, &s_rtv);
        backBuf->Release();
    }
    return hr;
}

bool Hooks::Init() {
    if (MH_Initialize() != MH_OK) return false;

    void* vtable = GetSwapChainVTable();
    if (!vtable) return false;

    void** vt = (void**)vtable;

    // Present = vtable[8], ResizeBuffers = vtable[13]
    if (MH_CreateHook(vt[8], (void*)hkPresent, (void**)&s_origPresent) != MH_OK) return false;
    if (MH_CreateHook(vt[13], (void*)hkResizeBuffers, (void**)&s_origResize) != MH_OK) return false;

    MH_EnableHook(MH_ALL_HOOKS);
    s_init = true;
    return true;
}

void Hooks::Shutdown() {
    if (!s_init) return;
    MH_DisableHook(MH_ALL_HOOKS);
    MH_RemoveHook(MH_ALL_HOOKS);
    MH_Uninitialize();

    if (s_origWndProc && s_hwnd) {
        SetWindowLongPtrA(s_hwnd, GWLP_WNDPROC, (LONG_PTR)s_origWndProc);
    }

    if (s_imguiInit) {
        ImGui_ImplDX11_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();
    }

    if (s_rtv) s_rtv->Release();
    s_init = false;
    s_imguiInit = false;
}

} // namespace Stara
