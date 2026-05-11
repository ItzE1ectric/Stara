#include "Game.hpp"

namespace Stara {
    extern bool g_rainbow;
    extern bool g_espBox;
    extern bool g_espName;
    extern bool g_noclip;
    extern bool g_chatSpam;
}

namespace Stara::Game {

static void* gameDomain = nullptr;
static bool threadAttached = false;

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

    if (!il2cpp_domain_get || !il2cpp_class_from_name || !il2cpp_thread_attach) {
        printf("[-] Stara: Failed to resolve IL2CPP exports.\n");
        return false;
    }

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
    }

    if (PlayerControl::klass && GameData::klass && AmongUsClient::klass) {
        printf("[+] Stara: Classes resolved successfully.\n");
        return true;
    }

    printf("[-] Stara: Failed to find core game classes.\n");
    return false;
}

void Attach() {
    if (gameDomain && il2cpp_thread_attach && !threadAttached) {
        il2cpp_thread_attach(gameDomain);
        threadAttached = true;
    }
}

void* GetLocalPlayer() {
    if (!PlayerControl::klass) return nullptr;
    void* field = il2cpp_class_get_field_from_name(PlayerControl::klass, "LocalPlayer");
    if (!field) return nullptr;
    void* lp = nullptr;
    il2cpp_field_static_get_value(field, &lp);
    return lp;
}

void Update() {
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
        isInGame = (inst != nullptr);
    }

    if (!isInGame) {
        players.clear();
        return;
    }

    if (GameData::klass) {
        void* field = il2cpp_class_get_field_from_name(GameData::klass, "Instance");
        void* gdata = nullptr;
        if (field) il2cpp_field_static_get_value(field, &gdata);

        if (gdata) {
            void* allPlayersList = *(void**)((uintptr_t)gdata + 0x10); 
            if (allPlayersList) {
                struct SystemList { void* k; void* m; void* items; int size; };
                struct SystemArray { void* k; void* m; void* b; int len; void* m_Items[1]; };

                SystemList* list = (SystemList*)allPlayersList;
                if (list->items && list->size >= 0 && list->size <= 15) {
                    SystemArray* arr = (SystemArray*)list->items;
                    std::vector<PlayerInfo> temp;
                    void* lp = GetLocalPlayer();

                    for (int i = 0; i < list->size; i++) {
                        void* infoPtr = arr->m_Items[i];
                        if (!infoPtr) continue;

                        PlayerInfo p;
                        p.isDead = *(bool*)((uintptr_t)infoPtr + 0x54);
                        int role = *(int*)((uintptr_t)infoPtr + 0x38);
                        p.isImpostor = (role == 1 || role == 7 || role == 5 || role == 18);
                        p.name = "Player " + std::to_string(i);

                        void* pcObj = *(void**)((uintptr_t)infoPtr + 0x58);
                        if (pcObj) {
                            void* nt = *(void**)((uintptr_t)pcObj + 0x98);
                            if (nt) {
                                p.x = *(float*)((uintptr_t)nt + 0x2C);
                                p.y = *(float*)((uintptr_t)nt + 0x30);
                            }

                            if (pcObj == lp) {
                                localX = p.x; localY = p.y; isImpostor = p.isImpostor;
                            } else {
                                p.distance = sqrtf(powf(p.x - localX, 2) + powf(p.y - localY, 2));
                                temp.push_back(p);
                            }
                        }
                    }
                    players = temp;
                }
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

void SetSpeed(float speed) {
    Attach();
    void* lp = GetLocalPlayer();
    if (!lp) return;
    void* phys = *(void**)((uintptr_t)lp + 0x94);
    if (phys) { *(float*)((uintptr_t)phys + 0x34) = speed; *(float*)((uintptr_t)phys + 0x38) = speed; }
}

void SetFullbright(bool enabled) {
    Attach();
    void* lp = GetLocalPlayer();
    if (!lp) return;
    void* light = *(void**)((uintptr_t)lp + 0x8C);
    if (light) *(float*)((uintptr_t)light + 0x10) = enabled ? 100.f : 1.f;
}

void CompleteAllTasks() {
    Attach();
    void* lp = GetLocalPlayer();
    if (!lp || !il2cpp_runtime_invoke) return;
    void* tasks = *(void**)((uintptr_t)lp + 0xAC);
    if (!tasks) return;
    void* method = il2cpp_class_get_method_from_name(PlayerControl::klass, "CompleteTask", 1);
    if (method) {
        struct SystemList { void* k; void* m; void* items; int size; };
        struct SystemArray { void* k; void* m; void* b; int len; void* m_Items[1]; };
        SystemList* list = (SystemList*)tasks;
        if (list->items) {
            SystemArray* arr = (SystemArray*)list->items;
            for (int i = 0; i < list->size; i++) {
                void* t = arr->m_Items[i];
                if (t) { void* p[1] = { t }; il2cpp_runtime_invoke(method, lp, p, nullptr); }
            }
        }
    }
}

void ForceEmergencyMeeting() {
    Attach();
    void* lp = GetLocalPlayer();
    if (lp && il2cpp_runtime_invoke) {
        void* method = il2cpp_class_get_method_from_name(PlayerControl::klass, "CmdReportDeadBody", 1);
        if (method) { void* p[1] = { nullptr }; il2cpp_runtime_invoke(method, lp, p, nullptr); }
    }
}

void SetPlayerColor(int colorId) {
    Attach();
    void* lp = GetLocalPlayer();
    if (lp && il2cpp_runtime_invoke) {
        void* method = il2cpp_class_get_method_from_name(PlayerControl::klass, "CmdCheckColor", 1);
        if (method) { uint8_t cid = (uint8_t)colorId; void* p[1] = { &cid }; il2cpp_runtime_invoke(method, lp, p, nullptr); }
    }
}

void TeleportTo(float x, float y) {
    void* lp = GetLocalPlayer();
    if (lp) {
        void* nt = *(void**)((uintptr_t)lp + 0x98);
        if (nt) { *(float*)((uintptr_t)nt + 0x2C) = x; *(float*)((uintptr_t)nt + 0x30) = y; }
    }
}

void SetName(const char* name) {
    Attach();
    void* lp = GetLocalPlayer();
    if (lp && il2cpp_string_new && il2cpp_runtime_invoke) {
        void* method = il2cpp_class_get_method_from_name(PlayerControl::klass, "CmdCheckName", 1);
        if (method) { void* s = il2cpp_string_new(name); void* p[1] = { s }; il2cpp_runtime_invoke(method, lp, p, nullptr); }
    }
}

void SetKillCooldown(float time) {
    void* lp = GetLocalPlayer();
    if (lp) *(float*)((uintptr_t)lp + 0x80) = time;
}

void SetKillDistance(float dist) {
    void* lp = GetLocalPlayer();
    if (lp) *(float*)((uintptr_t)lp + 0x34) = dist;
}

void SetWallhack(bool enabled) { SetFullbright(enabled); }

void SetHat(int hatId) {
    Attach();
    void* lp = GetLocalPlayer();
    if (lp && il2cpp_runtime_invoke) {
        void* method = il2cpp_class_get_method_from_name(PlayerControl::klass, "SetHat", 1);
        if (method) { void* p[1] = { &hatId }; il2cpp_runtime_invoke(method, lp, p, nullptr); }
    }
}

void SetPet(int petId) {
    Attach();
    void* lp = GetLocalPlayer();
    if (lp && il2cpp_runtime_invoke) {
        void* method = il2cpp_class_get_method_from_name(PlayerControl::klass, "SetPet", 1);
        if (method) { void* p[1] = { &petId }; il2cpp_runtime_invoke(method, lp, p, nullptr); }
    }
}

void SetCharacterScale(float scale) { }

void DrawESP(ImDrawList* drawList) {
    if (!isInGame || players.empty()) return;
    for (const auto& p : players) {
        ImVec2 sc = { ImGui::GetIO().DisplaySize.x / 2, ImGui::GetIO().DisplaySize.y / 2 };
        float sx = sc.x + (p.x - localX) * 35.f;
        float sy = sc.y - (p.y - localY) * 35.f;
        
        if (sx < 0 || sx > ImGui::GetIO().DisplaySize.x || sy < 0 || sy > ImGui::GetIO().DisplaySize.y) continue;

        ImU32 col = p.isImpostor ? IM_COL32(255, 30, 30, 255) : IM_COL32(0, 255, 120, 255);
        if (g_espBox) drawList->AddRect({sx - 15, sy - 30}, {sx + 15, sy + 5}, col, 0, 0, 1.5f);
        if (g_espName) {
            char b[64]; sprintf(b, "P%s", p.isImpostor ? " [IMP]" : "");
            drawList->AddText({sx - 15, sy - 45}, col, b);
        }
    }
}

void SpamChat(const char* text) {
    Attach();
    void* lp = GetLocalPlayer();
    if (lp && il2cpp_string_new && il2cpp_runtime_invoke) {
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
    if (inst) {
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
    if (inst) {
        void* method = il2cpp_class_get_method_from_name(AmongUsClient::klass, "CmdStartGame", 0);
        if (method) il2cpp_runtime_invoke(method, inst, nullptr, nullptr);
    }
}

void TeleportToRoom(int roomId) {
    if (roomId == 1) TeleportTo(-10, 5);
    if (roomId == 2) TeleportTo(5, 5);
}

} // namespace Stara::Game
