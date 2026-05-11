#include "Game.hpp"

// External state from Menu.cpp (Stara namespace)
namespace Stara {
    extern bool g_rainbow;
    extern bool g_spin;
}

namespace Stara::Game {

bool Init() {
    printf("[*] Waiting for GameAssembly.dll...\n");
    // Wait for GameAssembly.dll to load
    while (!(gameAssembly = (uintptr_t)GetModuleHandleA("GameAssembly.dll"))) {
        Sleep(100);
    }
    printf("[+] Found GameAssembly.dll at 0x%IX\n", gameAssembly);

    // Resolve IL2CPP exports
    auto resolve = [](const char* name) -> void* {
        return (void*)GetProcAddress((HMODULE)gameAssembly, name);
    };

    il2cpp_domain_get          = (il2cpp_domain_get_t)resolve("il2cpp_domain_get");
    il2cpp_domain_get_assemblies = (il2cpp_domain_get_assemblies_t)resolve("il2cpp_domain_get_assemblies");
    il2cpp_class_from_name     = (il2cpp_class_from_name_t)resolve("il2cpp_class_from_name");
    il2cpp_class_get_methods   = (il2cpp_class_get_methods_t)resolve("il2cpp_class_get_methods");
    il2cpp_method_get_name     = (il2cpp_method_get_name_t)resolve("il2cpp_method_get_name");
    il2cpp_string_new          = (il2cpp_string_new_t)resolve("il2cpp_string_new");
    il2cpp_assembly_get_image  = (il2cpp_assembly_get_image_t)resolve("il2cpp_assembly_get_image");
    il2cpp_field_get_offset    = (il2cpp_field_get_offset_t)resolve("il2cpp_field_get_offset");
    il2cpp_class_get_field_from_name = (il2cpp_class_get_field_from_name_t)resolve("il2cpp_class_get_field_from_name");
    il2cpp_field_static_get_value = (il2cpp_field_static_get_value_t)resolve("il2cpp_field_static_get_value");
    il2cpp_field_set_value = (il2cpp_field_set_value_t)resolve("il2cpp_field_set_value");
    il2cpp_field_get_value = (il2cpp_field_get_value_t)resolve("il2cpp_field_get_value");
    il2cpp_class_get_method_from_name = (il2cpp_class_get_method_from_name_t)resolve("il2cpp_class_get_method_from_name");
    il2cpp_runtime_invoke = (il2cpp_runtime_invoke_t)resolve("il2cpp_runtime_invoke");

    if (!il2cpp_domain_get || !il2cpp_class_from_name) return false;

    // Get Assembly-CSharp image
    void* domain = il2cpp_domain_get();
    if (!domain) return false;

    size_t asmCount = 0;
    void** assemblies = il2cpp_domain_get_assemblies(domain, &asmCount);

    void* csharpImage = nullptr;
    for (size_t i = 0; i < asmCount; i++) {
        void* img = il2cpp_assembly_get_image(assemblies[i]);
        if (img) {
            void* pc = il2cpp_class_from_name(img, "", "PlayerControl");
            if (pc) {
                PlayerControl::klass = pc;
                printf("[+] Resolved PlayerControl::klass\n");
                csharpImage = img;
                break;
            }
        }
    }

    if (!csharpImage) {
        printf("[-] Could not find Assembly-CSharp image\n");
        return false;
    }

    // Resolve game classes
    PlayerPhysics::klass = il2cpp_class_from_name(csharpImage, "", "PlayerPhysics");
    if (PlayerPhysics::klass) printf("[+] Resolved PlayerPhysics::klass\n");

    GameData::klass      = il2cpp_class_from_name(csharpImage, "", "GameData");
    if (GameData::klass) printf("[+] Resolved GameData::klass\n");

    ShipStatus::klass    = il2cpp_class_from_name(csharpImage, "", "ShipStatus");
    if (ShipStatus::klass) printf("[+] Resolved ShipStatus::klass\n");

    AmongUsClient::klass = il2cpp_class_from_name(csharpImage, "", "AmongUsClient");
    if (AmongUsClient::klass) printf("[+] Resolved AmongUsClient::klass\n");

    return true;
}

// Helper to get LocalPlayer safely
void* GetLocalPlayer() {
    if (!PlayerControl::klass || !il2cpp_class_get_field_from_name || !il2cpp_field_static_get_value) return nullptr;
    
    void* field = il2cpp_class_get_field_from_name(PlayerControl::klass, "LocalPlayer");
    if (!field) {
        return nullptr;
    }
    
    void* localPlayer = nullptr;
    il2cpp_field_static_get_value(field, &localPlayer);
    return localPlayer;
}

void Update() {
    if (!gameAssembly || !PlayerControl::klass) return;

    // Check if in game via AmongUsClient.Instance
    if (AmongUsClient::klass && il2cpp_class_get_field_from_name) {
        void* field = il2cpp_class_get_field_from_name(AmongUsClient::klass, "Instance");
        void* instance = nullptr;
        if (field) il2cpp_field_static_get_value(field, &instance);
        isInGame = (instance != nullptr);
    }

    if (!isInGame) {
        players.clear();
        return;
    }

    // Refresh player list for ESP
    void* allPlayersField = il2cpp_class_get_field_from_name(PlayerControl::klass, "AllPlayerControls");
    void* playerList = nullptr;
    if (allPlayersField) il2cpp_field_static_get_value(allPlayersField, &playerList);

    if (playerList) {
        struct SystemList {
            void* klass;
            void* monitor;
            void* items; // Il2CppArray
            int size;
        };
        struct SystemArray {
            void* klass;
            void* monitor;
            void* bounds;
            int max_length;
            void* m_Items[1];
        };

        SystemList* list = (SystemList*)playerList;
        SystemArray* arr = (SystemArray*)list->items;
        
        std::vector<PlayerInfo> tempPlayers;
        void* localPlayer = GetLocalPlayer();

        for (int i = 0; i < list->size; i++) {
            void* pc = arr->m_Items[i];
            if (!pc) continue;

            PlayerInfo info;
            
            // Get NetworkedPlayerInfo
            void* data = *(void**)((uintptr_t)pc + 0x58); // CachedPlayerData
            if (data) {
                info.isDead = *(bool*)((uintptr_t)data + 0x54);
                int role = *(int*)((uintptr_t)data + 0x38);
                info.isImpostor = (role == 1); // RoleTypes.Impostor
                info.name = "Player"; 
                int colorId = 0; // Default
                info.color = GetAmongUsColor(colorId);
            }

            // Get Position
            void* netTransform = *(void**)((uintptr_t)pc + 0x98);
            if (netTransform) {
                info.x = *(float*)((uintptr_t)netTransform + 0x2C);
                info.y = *(float*)((uintptr_t)netTransform + 0x30);
            }

            if (pc == localPlayer) {
                localX = info.x;
                localY = info.y;
                isImpostor = info.isImpostor;

                // Rainbow logic
                static float lastColorUpdate = 0;
                if (g_rainbow && (ImGui::GetTime() - lastColorUpdate > 0.1f)) {
                    lastColorUpdate = (float)ImGui::GetTime();
                    static int rainbowId = 0;
                    SetPlayerColor(rainbowId++ % 12);
                }
            } else {
                info.distance = sqrtf(powf(info.x - localX, 2) + powf(info.y - localY, 2));
                tempPlayers.push_back(info);
            }
        }
        players = tempPlayers;
    }
}

void SetSpeed(float speed) {
    if (!gameAssembly || !PlayerPhysics::klass || !PlayerControl::klass) return;
    
    void* localPlayer = GetLocalPlayer();
    if (!localPlayer) return;
    
    void* myPhysics = *(void**)((uintptr_t)localPlayer + PlayerControl::offset_MyPhysics);
    if (!myPhysics) return;
    
    *(float*)((uintptr_t)myPhysics + PlayerPhysics::offset_Speed) = speed;
    *(float*)((uintptr_t)myPhysics + PlayerPhysics::offset_GhostSpeed) = speed;
}

void SetFullbright(bool enabled) {
    if (!gameAssembly || !PlayerControl::klass) return;
    void* localPlayer = GetLocalPlayer();
    if (!localPlayer) return;

    void* lightSource = *(void**)((uintptr_t)localPlayer + 0x8C); // lightSource
    if (lightSource) {
        *(float*)((uintptr_t)lightSource + 0x10) = enabled ? 100.0f : 1.0f; // viewDistance
    }
}

void CompleteAllTasks() {
    if (!gameAssembly || !PlayerControl::klass || !il2cpp_class_get_method_from_name || !il2cpp_runtime_invoke) return;
    void* localPlayer = GetLocalPlayer();
    if (!localPlayer) return;

    void* myTasks = *(void**)((uintptr_t)localPlayer + 0xAC);
    if (!myTasks) return;

    void* method = il2cpp_class_get_method_from_name(PlayerControl::klass, "CompleteTask", 1);
    if (!method) return;

    struct SystemList {
        void* klass;
        void* monitor;
        void* items; 
        int size;
    };
    struct SystemArray {
        void* klass;
        void* monitor;
        void* bounds;
        int max_length;
        void* m_Items[1];
    };

    SystemList* list = (SystemList*)myTasks;
    if (list->items) {
        SystemArray* arr = (SystemArray*)list->items;
        for (int i = 0; i < list->size; i++) {
            void* task = arr->m_Items[i];
            if (task) {
                void* params[1] = { task };
                il2cpp_runtime_invoke(method, localPlayer, params, nullptr);
            }
        }
    }
}

void ForceEmergencyMeeting() {
    if (!gameAssembly || !PlayerControl::klass || !il2cpp_class_get_method_from_name || !il2cpp_runtime_invoke) return;
    void* localPlayer = GetLocalPlayer();
    if (!localPlayer) return;

    void* method = il2cpp_class_get_method_from_name(PlayerControl::klass, "CmdReportDeadBody", 1);
    if (method) {
        void* params[1] = { nullptr };
        il2cpp_runtime_invoke(method, localPlayer, params, nullptr);
    }
}

void SetPlayerColor(int colorId) {
    if (!gameAssembly || !PlayerControl::klass || !il2cpp_class_get_method_from_name || !il2cpp_runtime_invoke) return;
    void* localPlayer = GetLocalPlayer();
    if (!localPlayer) return;

    void* method = il2cpp_class_get_method_from_name(PlayerControl::klass, "CmdCheckColor", 1);
    if (method) {
        uint8_t cid = (uint8_t)colorId;
        void* params[1] = { &cid };
        il2cpp_runtime_invoke(method, localPlayer, params, nullptr);
    }
}

void TeleportTo(float x, float y) {
    if (!gameAssembly || !PlayerControl::klass) return;
    void* localPlayer = GetLocalPlayer();
    if (!localPlayer) return;

    void* netTransform = *(void**)((uintptr_t)localPlayer + PlayerControl::offset_NetTransform);
    if (netTransform) {
        *(float*)((uintptr_t)netTransform + 0x2C) = x;
        *(float*)((uintptr_t)netTransform + 0x30) = y;
    }
}

void SetName(const char* name) {
    if (!gameAssembly || !PlayerControl::klass || !il2cpp_string_new || !il2cpp_class_get_method_from_name || !il2cpp_runtime_invoke) return;
    void* localPlayer = GetLocalPlayer();
    if (!localPlayer) return;

    void* method = il2cpp_class_get_method_from_name(PlayerControl::klass, "CmdCheckName", 1);
    if (method) {
        void* il2str = il2cpp_string_new(name);
        void* params[1] = { il2str };
        il2cpp_runtime_invoke(method, localPlayer, params, nullptr);
    }
}

void SetKillCooldown(float time) {
    void* localPlayer = GetLocalPlayer();
    if (localPlayer) {
        *(float*)((uintptr_t)localPlayer + 0x80) = time; // killTimer
    }
}

void SetKillDistance(float dist) {
    void* localPlayer = GetLocalPlayer();
    if (localPlayer) {
        *(float*)((uintptr_t)localPlayer + 0x34) = dist; // MaxReportDistance
    }
}

void SetWallhack(bool enabled) {
    SetFullbright(enabled);
}

void SetHat(int hatId) {
    if (!gameAssembly || !PlayerControl::klass || !il2cpp_class_get_method_from_name || !il2cpp_runtime_invoke) return;
    void* localPlayer = GetLocalPlayer();
    if (!localPlayer) return;

    void* method = il2cpp_class_get_method_from_name(PlayerControl::klass, "SetHat", 1);
    if (method) {
        void* params[1] = { &hatId };
        il2cpp_runtime_invoke(method, localPlayer, params, nullptr);
    }
}

void SetPet(int petId) {
    if (!gameAssembly || !PlayerControl::klass || !il2cpp_class_get_method_from_name || !il2cpp_runtime_invoke) return;
    void* localPlayer = GetLocalPlayer();
    if (!localPlayer) return;

    void* method = il2cpp_class_get_method_from_name(PlayerControl::klass, "SetPet", 1);
    if (method) {
        void* params[1] = { &petId };
        il2cpp_runtime_invoke(method, localPlayer, params, nullptr);
    }
}

void SetCharacterScale(float scale) {
    void* localPlayer = GetLocalPlayer();
    if (localPlayer) {
        void* transform = *(void**)((uintptr_t)localPlayer + 0x18); // GameObject -> Transform
        if (transform) {
            // Setting scale (Vector3 at 0x18 or similar)
            // For now, let's just use the direct offset if known. 
            // Most Unity versions use 0x18 for localScale in Transform.
        }
    }
}

} // namespace Stara::Game
