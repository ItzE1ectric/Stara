#include "Game.hpp"
#include <windows.h>

namespace Stara {
    extern bool g_rainbow;
    extern bool g_espBox;
    extern bool g_espName;
    extern bool g_noclip;
    extern bool g_chatSpam;
    extern bool g_spin;
    extern float g_speed;
}

namespace Stara::Game {

static void* gameDomain = nullptr;
static bool threadAttached = false;

template<typename T>
static bool IsValid(T* ptr) {
    if (!ptr || (uintptr_t)ptr < 0x10000 || (uintptr_t)ptr > 0x7FFFFFFF) return false;
    return true;
}

void Attach() {
    if (gameDomain && il2cpp_thread_attach && !threadAttached) {
        il2cpp_thread_attach(gameDomain);
        threadAttached = true;
    }
}

bool Init() {
    printf("[*] Stara: Starting Initialization...\n");
    gameAssembly = (uintptr_t)GetModuleHandleA("GameAssembly.dll");
    if (!gameAssembly) return false;

    auto resolve = [](const char* name) -> void* {
        return (void*)GetProcAddress((HMODULE)gameAssembly, name);
    };

    il2cpp_domain_get          = (il2cpp_domain_get_t)resolve("il2cpp_domain_get");
    il2cpp_domain_get_assemblies = (il2cpp_domain_get_assemblies_t)resolve("il2cpp_domain_get_assemblies");
    il2cpp_class_from_name     = (il2cpp_class_from_name_t)resolve("il2cpp_class_from_name");
    il2cpp_string_new          = (il2cpp_string_new_t)resolve("il2cpp_string_new");
    il2cpp_assembly_get_image  = (il2cpp_assembly_get_image_t)resolve("il2cpp_assembly_get_image");
    il2cpp_class_get_field_from_name = (il2cpp_class_get_field_from_name_t)resolve("il2cpp_class_get_field_from_name");
    il2cpp_field_static_get_value = (il2cpp_field_static_get_value_t)resolve("il2cpp_field_static_get_value");
    il2cpp_class_get_method_from_name = (il2cpp_class_get_method_from_name_t)resolve("il2cpp_class_get_method_from_name");
    il2cpp_runtime_invoke = (il2cpp_runtime_invoke_t)resolve("il2cpp_runtime_invoke");
    il2cpp_thread_attach = (il2cpp_thread_attach_t)resolve("il2cpp_thread_attach");

    if (!il2cpp_domain_get || !il2cpp_class_from_name || !il2cpp_thread_attach) return false;

    gameDomain = il2cpp_domain_get();
    if (!gameDomain) return false;

    Attach();

    size_t asmCount = 0;
    void** assemblies = il2cpp_domain_get_assemblies(gameDomain, &asmCount);
    if (!assemblies) return false;

    for (size_t i = 0; i < asmCount; i++) {
        void* img = il2cpp_assembly_get_image(assemblies[i]);
        if (!img) continue;
        if (!PlayerControl::klass) PlayerControl::klass = il2cpp_class_from_name(img, "", "PlayerControl");
        if (!PlayerPhysics::klass) PlayerPhysics::klass = il2cpp_class_from_name(img, "", "PlayerPhysics");
        if (!GameData::klass)      GameData::klass      = il2cpp_class_from_name(img, "", "GameData");
        if (!AmongUsClient::klass) AmongUsClient::klass = il2cpp_class_from_name(img, "", "AmongUsClient");
        if (!ShipStatus::klass)    ShipStatus::klass    = il2cpp_class_from_name(img, "", "ShipStatus");
        if (!GameOptionsManager::klass) GameOptionsManager::klass = il2cpp_class_from_name(img, "AmongUs.GameOptions", "GameOptionsManager");
        if (!Transform::klass) Transform::klass = il2cpp_class_from_name(img, "UnityEngine", "Transform");
    }

    return (PlayerControl::klass && GameData::klass && AmongUsClient::klass && GameOptionsManager::klass && Transform::klass);
}

void* GetLocalPlayer() {
    if (!PlayerControl::klass) return nullptr;
    void* field = il2cpp_class_get_field_from_name(PlayerControl::klass, "LocalPlayer");
    if (!field) return nullptr;
    void* lp = nullptr;
    il2cpp_field_static_get_value(field, &lp);
    return IsValid(lp) ? lp : nullptr;
}

static void UpdateInternal() {
    static float lastUpdateTime = 0;
    float currentTime = (float)ImGui::GetTime();
    
    if (currentTime - lastUpdateTime < 0.1f) return;
    lastUpdateTime = currentTime;

    if (!gameAssembly || !PlayerControl::klass) return;
    Attach();

    if (AmongUsClient::klass) {
        void* field = il2cpp_class_get_field_from_name(AmongUsClient::klass, "Instance");
        void* inst = nullptr;
        if (field) il2cpp_field_static_get_value(field, &inst);
        isInGame = IsValid(inst);
    }

    if (!isInGame) {
        players.clear();
        return;
    }

    void* lp = GetLocalPlayer();
    if (lp) {
        void* nt = *(void**)((uintptr_t)lp + 0x98); // NetTransform
        if (IsValid(nt)) {
            localX = *(float*)((uintptr_t)nt + 0x44); // lastPosition.x
            localY = *(float*)((uintptr_t)nt + 0x48); // lastPosition.y
        }
        
        void* coll = *(void**)((uintptr_t)lp + 0x90); // Collider
        if (IsValid(coll)) {
            // Layer 8 is Ignore Collisions
            *(int*)((uintptr_t)coll + 0x1C) = g_noclip ? 8 : 0; 
        }

        void* phys = *(void**)((uintptr_t)lp + 0x94); // MyPhysics
        if (IsValid(phys)) {
            *(float*)((uintptr_t)phys + 0x34) = g_speed; // Speed field
        }

        if (g_rainbow) {
            static float h = 0; h += 0.05f; if(h>1.0f) h=0;
            SetPlayerColor((int)(h * 10));
        }

        if (g_spin) {
            void* nt = *(void**)((uintptr_t)lp + 0x98);
            if (IsValid(nt)) {
                static float rot = 0; rot += 5.0f;
                // Rotating by writing to Z rotation if we can find it, otherwise skipping for stability
            }
        }
    }

    if (PlayerControl::klass) {
        void* field = il2cpp_class_get_field_from_name(PlayerControl::klass, "AllPlayerControls");
        void* allPlayersList = nullptr;
        if (field) il2cpp_field_static_get_value(field, &allPlayersList);

        if (IsValid(allPlayersList)) {
            struct SystemList { void* k; void* m; void* items; int size; };
            struct SystemArray { void* k; void* m; void* b; int len; void* m_Items[1]; };

            SystemList* list = (SystemList*)allPlayersList;
            if (IsValid(list->items) && list->size > 0 && list->size <= 15) {
                SystemArray* arr = (SystemArray*)list->items;
                std::vector<PlayerInfo> temp;

                for (int i = 0; i < list->size; i++) {
                    void* pcObj = arr->m_Items[i];
                    if (!IsValid(pcObj)) continue;

                    void* data = *(void**)((uintptr_t)pcObj + 0x58); // CachedPlayerData
                    if (!IsValid(data)) continue;

                    PlayerInfo p;
                    p.isDead = *(bool*)((uintptr_t)data + 0x54);
                    int role = *(int*)((uintptr_t)data + 0x38); // RoleType
                    p.isImpostor = (role == 1); // 1 = Impostor
                    
                    p.name = "Player " + std::to_string(*(uint8_t*)((uintptr_t)pcObj + 0x28)); // PlayerId

                    void* pcNt = *(void**)((uintptr_t)pcObj + 0x98);
                    if (IsValid(pcNt)) {
                        p.x = *(float*)((uintptr_t)pcNt + 0x44);
                        p.y = *(float*)((uintptr_t)pcNt + 0x48);
                    }

                    if (pcObj == lp) {
                        isImpostor = p.isImpostor;
                    } else {
                        p.distance = sqrtf(powf(p.x - localX, 2) + powf(p.y - localY, 2));
                        temp.push_back(p);
                    }
                }
                players = temp;
            }
        }
    }

    if (g_chatSpam) {
        static float lastSpam = 0;
        if (currentTime - lastSpam > 1.0f) {
            lastSpam = currentTime;
            SpamChat("Stara Client on TOP");
        }
    }
}

void Update() {
    __try {
        UpdateInternal();
    } __except(EXCEPTION_EXECUTE_HANDLER) {}
}

void SetSpeed(float speed) {
    Attach();
    void* lp = GetLocalPlayer();
    if (!lp) return;
    void* phys = *(void**)((uintptr_t)lp + 0x94);
    if (IsValid(phys)) { *(float*)((uintptr_t)phys + 0x34) = speed; *(float*)((uintptr_t)phys + 0x38) = speed; }
}

void SetFullbright(bool enabled) {
    Attach();
    if (!ShipStatus::klass) return;
    void* field = il2cpp_class_get_field_from_name(ShipStatus::klass, "Instance");
    void* inst = nullptr;
    if (field) il2cpp_field_static_get_value(field, &inst);
    if (IsValid(inst)) {
        *(float*)((uintptr_t)inst + 0x38) = enabled ? 100.f : 1.f; // MaxLightRadius
        *(float*)((uintptr_t)inst + 0x3C) = enabled ? 100.f : 1.f; // MinLightRadius
    }
}

void CompleteAllTasks() {
    Attach();
    void* lp = GetLocalPlayer();
    if (!lp || !il2cpp_runtime_invoke) return;
    
    void* tasks = *(void**)((uintptr_t)lp + 0xAC); // myTasks
    if (!IsValid(tasks)) return;

    void* method = il2cpp_class_get_method_from_name(PlayerControl::klass, "RpcCompleteTask", 1);
    if (method) {
        struct SystemList { void* k; void* m; void* items; int size; };
        struct SystemArray { void* k; void* m; void* b; int len; void* m_Items[1]; };
        SystemList* list = (SystemList*)tasks;
        if (IsValid(list->items)) {
            SystemArray* arr = (SystemArray*)list->items;
            for (int i = 0; i < list->size; i++) {
                void* t = arr->m_Items[i];
                if (IsValid(t)) { 
                    uint32_t taskIdx = *(uint32_t*)((uintptr_t)t + 0x10); // Task Index
                    void* p[1] = { &taskIdx }; 
                    il2cpp_runtime_invoke(method, lp, p, nullptr); 
                }
            }
        }
    }
}

void ForceEmergencyMeeting() {
    Attach();
    void* lp = GetLocalPlayer();
    if (IsValid(lp) && il2cpp_runtime_invoke) {
        void* method = il2cpp_class_get_method_from_name(PlayerControl::klass, "RpcStartMeeting", 1);
        if (method) { void* p[1] = { lp }; il2cpp_runtime_invoke(method, lp, p, nullptr); }
    }
}

void SetPlayerColor(int colorId) {
    Attach();
    void* lp = GetLocalPlayer();
    if (IsValid(lp) && il2cpp_runtime_invoke) {
        void* method = il2cpp_class_get_method_from_name(PlayerControl::klass, "RpcSetColor", 1);
        if (method) { uint8_t cid = (uint8_t)colorId; void* p[1] = { &cid }; il2cpp_runtime_invoke(method, lp, p, nullptr); }
    }
}

void TeleportTo(float x, float y) {
    void* lp = GetLocalPlayer();
    if (IsValid(lp)) {
        void* nt = *(void**)((uintptr_t)lp + 0x98);
        if (IsValid(nt)) { 
            *(float*)((uintptr_t)nt + 0x44) = x; // lastPosition.x
            *(float*)((uintptr_t)nt + 0x48) = y; // lastPosition.y
        }
    }
}

void SetName(const char* name) {
    Attach();
    void* lp = GetLocalPlayer();
    if (IsValid(lp) && il2cpp_string_new && il2cpp_runtime_invoke) {
        void* method = il2cpp_class_get_method_from_name(PlayerControl::klass, "RpcSetName", 1);
        if (method) { void* s = il2cpp_string_new(name); void* p[1] = { s }; il2cpp_runtime_invoke(method, lp, p, nullptr); }
    }
}

void SetKillCooldown(float time) {
    void* lp = GetLocalPlayer();
    if (IsValid(lp)) {
        *(float*)((uintptr_t)lp + 0x80) = time; // killTimer
    }
    
    if (!GameOptionsManager::klass) return;
    void* field = il2cpp_class_get_field_from_name(GameOptionsManager::klass, "<Instance>k__BackingField");
    void* inst = nullptr;
    if (field) il2cpp_field_static_get_value(field, &inst);
    if (IsValid(inst)) {
        void* opt = *(void**)((uintptr_t)inst + 0x18); // currentNormalGameOptions
        if (IsValid(opt)) *(float*)((uintptr_t)opt + 0x24) = time; // KillCooldown
    }
}

void SetKillDistance(float dist) {
    if (!GameOptionsManager::klass) return;
    void* field = il2cpp_class_get_field_from_name(GameOptionsManager::klass, "<Instance>k__BackingField");
    void* inst = nullptr;
    if (field) il2cpp_field_static_get_value(field, &inst);
    if (IsValid(inst)) {
        void* opt = *(void**)((uintptr_t)inst + 0x18);
        if (IsValid(opt)) *(int*)((uintptr_t)opt + 0x44) = (int)dist; // KillDistance
    }
}

void SetWallhack(bool enabled) { SetFullbright(enabled); }

void SetHat(int hatId) {
    Attach();
    void* lp = GetLocalPlayer();
    if (IsValid(lp) && il2cpp_runtime_invoke) {
        void* method = il2cpp_class_get_method_from_name(PlayerControl::klass, "RpcSetHat", 1);
        if (method) { uint32_t hid = (uint32_t)hatId; void* p[1] = { &hid }; il2cpp_runtime_invoke(method, lp, p, nullptr); }
    }
}

void SetPet(int petId) {
    Attach();
    void* lp = GetLocalPlayer();
    if (IsValid(lp) && il2cpp_runtime_invoke) {
        void* method = il2cpp_class_get_method_from_name(PlayerControl::klass, "RpcSetPet", 1);
        if (method) { uint32_t pid = (uint32_t)petId; void* p[1] = { &pid }; il2cpp_runtime_invoke(method, lp, p, nullptr); }
    }
}

void SetSkin(int skinId) {
    Attach();
    void* lp = GetLocalPlayer();
    if (IsValid(lp) && il2cpp_runtime_invoke) {
        void* method = il2cpp_class_get_method_from_name(PlayerControl::klass, "RpcSetSkin", 1);
        if (method) { uint32_t sid = (uint32_t)skinId; void* p[1] = { &sid }; il2cpp_runtime_invoke(method, lp, p, nullptr); }
    }
}

void SetCharacterScale(float scale) {
    Attach();
    void* lp = GetLocalPlayer();
    if (IsValid(lp) && il2cpp_runtime_invoke) {
        void* get_trans = il2cpp_class_get_method_from_name(PlayerControl::klass, "get_transform", 0);
        if (get_trans) {
            void* trans = il2cpp_runtime_invoke(get_trans, lp, nullptr, nullptr);
            if (IsValid(trans)) {
                void* set_scale = il2cpp_class_get_method_from_name(Transform::klass, "set_localScale", 1);
                if (set_scale) {
                    float s[3] = { scale, scale, 1.0f };
                    void* p[1] = { s };
                    il2cpp_runtime_invoke(set_scale, trans, p, nullptr);
                }
            }
        }
    }
}

void PlayAnimation(uint8_t animId) {
    Attach();
    void* lp = GetLocalPlayer();
    if (IsValid(lp) && il2cpp_runtime_invoke) {
        void* method = il2cpp_class_get_method_from_name(PlayerControl::klass, "RpcPlayAnimation", 1);
        if (method) { void* p[1] = { &animId }; il2cpp_runtime_invoke(method, lp, p, nullptr); }
    }
}

void DrawESP(ImDrawList* drawList) {
    if (!isInGame || players.empty()) return;
    for (const auto& p : players) {
        ImVec2 sc = { ImGui::GetIO().DisplaySize.x / 2, ImGui::GetIO().DisplaySize.y / 2 };
        float sx = sc.x + (p.x - localX) * 35.f;
        float sy = sc.y - (p.y - localY) * 35.f;
        
        if (sx < 0 || sx > ImGui::GetIO().DisplaySize.x || sy < 0 || sy > ImGui::GetIO().DisplaySize.y) continue;

        ImU32 col = p.isImpostor ? IM_COL32(255, 30, 30, 255) : IM_COL32(0, 220, 255, 255);
        if (g_espBox) drawList->AddRect({sx - 15, sy - 30}, {sx + 15, sy + 5}, col, 0, 0, 1.5f);
        if (g_espName) {
            char b[64]; sprintf(b, "%s%s", p.name.c_str(), p.isImpostor ? " [IMP]" : "");
            drawList->AddText({sx - 15, sy - 45}, col, b);
        }
    }
}

void SpamChat(const char* text) {
    Attach();
    void* lp = GetLocalPlayer();
    if (IsValid(lp) && il2cpp_string_new && il2cpp_runtime_invoke) {
        void* method = il2cpp_class_get_method_from_name(PlayerControl::klass, "CmdChat", 1);
        if (method) { void* s = il2cpp_string_new(text); void* p[1] = { s }; il2cpp_runtime_invoke(method, lp, p, nullptr); }
    }
}

void EndGame() {
    Attach();
    if (!AmongUsClient::klass || !il2cpp_runtime_invoke) return;
    void* field = il2cpp_class_get_field_from_name(AmongUsClient::klass, "Instance");
    void* inst = nullptr;
    if (field) il2cpp_field_static_get_value(field, &inst);
    if (IsValid(inst)) {
        void* method = il2cpp_class_get_method_from_name(AmongUsClient::klass, "RpcEndGame", 1);
        if (method) { int reason = 0; void* p[1] = { &reason }; il2cpp_runtime_invoke(method, inst, p, nullptr); }
    }
}

void StartGame() {
    Attach();
    if (!AmongUsClient::klass || !il2cpp_runtime_invoke) return;
    void* field = il2cpp_class_get_field_from_name(AmongUsClient::klass, "Instance");
    void* inst = nullptr;
    if (field) il2cpp_field_static_get_value(field, &inst);
    if (IsValid(inst)) {
        void* method = il2cpp_class_get_method_from_name(AmongUsClient::klass, "CmdStartGame", 0);
        if (method) il2cpp_runtime_invoke(method, inst, nullptr, nullptr);
    }
}

void TeleportToRoom(int roomId) {
    if (roomId == 1) TeleportTo(-10, 5);
    if (roomId == 2) TeleportTo(5, 5);
}

} // namespace Stara::Game
