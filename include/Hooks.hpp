#pragma once
#include "Common.hpp"

namespace Stara {

// DX11 swapchain Present hook + ImGui rendering
class Hooks {
public:
    static bool Init();
    static void Shutdown();
    static bool IsInitialized() { return s_init; }

    static ID3D11Device*           GetDevice()  { return s_device; }
    static ID3D11DeviceContext*    GetContext() { return s_context; }
    static IDXGISwapChain*         GetSwapChain() { return s_swapChain; }
    static ID3D11RenderTargetView* GetRTV()    { return s_rtv; }

private:
    static HRESULT __stdcall hkPresent(IDXGISwapChain* sc, UINT sync, UINT flags);
    static HRESULT __stdcall hkResizeBuffers(IDXGISwapChain* sc, UINT count, UINT w, UINT h, DXGI_FORMAT fmt, UINT flags);
    static LRESULT __stdcall hkWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);

    static inline bool s_init = false;
    static inline bool s_imguiInit = false;
    static inline ID3D11Device* s_device = nullptr;
    static inline ID3D11DeviceContext* s_context = nullptr;
    static inline IDXGISwapChain* s_swapChain = nullptr;
    static inline ID3D11RenderTargetView* s_rtv = nullptr;
    static inline HWND s_hwnd = nullptr;
    static inline WNDPROC s_origWndProc = nullptr;

    using PresentFn = HRESULT(__stdcall*)(IDXGISwapChain*, UINT, UINT);
    using ResizeFn  = HRESULT(__stdcall*)(IDXGISwapChain*, UINT, UINT, UINT, DXGI_FORMAT, UINT);
    static inline PresentFn s_origPresent = nullptr;
    static inline ResizeFn  s_origResize  = nullptr;
};

} // namespace Stara
