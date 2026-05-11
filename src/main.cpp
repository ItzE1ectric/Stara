#include "Common.hpp"
#include "Hooks.hpp"
#include "Game.hpp"

namespace Stara {
    bool g_menuVisible = true;
    static HMODULE g_hModule = nullptr;
    static std::atomic<bool> g_running = true;
}

static DWORD WINAPI MainThread(LPVOID param) {
    AllocConsole();
    FILE* f;
    freopen_s(&f, "CONOUT$", "w", stdout);
    printf("[+] Injecting Stara Among Us Client...\n");

    // Wait for game to fully load
    Sleep(3000);
    printf("[+] Calling Game::Init()...\n");

    // Init game data / IL2CPP
    Stara::Game::Init();

    // Hook DX11
    if (!Stara::Hooks::Init()) {
        MessageBoxA(nullptr, "Failed to hook DX11", "Stara", MB_OK);
        FreeLibraryAndExitThread(Stara::g_hModule, 1);
        return 1;
    }

    // Main loop — wait for unload key (END)
    while (Stara::g_running) {
        if (GetAsyncKeyState(VK_END) & 1) {
            Stara::g_running = false;
        }
        Sleep(50);
    }

    // Cleanup
    Stara::Hooks::Shutdown();
    Sleep(200);
    FreeLibraryAndExitThread(Stara::g_hModule, 0);
    return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID reserved) {
    if (reason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(hModule);
        Beep(500, 300); // Quick beep to confirm injection
        
        Stara::g_hModule = hModule;
        HANDLE thread = CreateThread(nullptr, 0, MainThread, nullptr, 0, nullptr);
        if (thread) CloseHandle(thread);
    }
    return TRUE;
}
