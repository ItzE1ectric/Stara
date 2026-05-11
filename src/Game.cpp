#include "Game.hpp"
#include <windows.h>

namespace Stara::Game {

// Direct RVA function typedefs (IL2CPP x86: ret func(this, params..., MethodInfo*))
typedef void (__cdecl *RpcCompleteTask_fn)(void*, uint32_t, void*);
typedef void (__cdecl *RpcStartMeeting_fn)(void*, void*, void*);
typedef void (__cdecl *RpcSetColor_fn)(void*, uint8_t, void*);
typedef void (__cdecl *RpcSetName_fn)(void*, void*, void*);
typedef void (__cdecl *RpcSetHat_fn)(void*, void*, void*);
typedef void (__cdecl *RpcSetPet_fn)(void*, void*, void*);
typedef void (__cdecl *RpcSetSkin_fn)(void*, void*, void*);
typedef void (__cdecl *RpcSendChat_fn)(void*, void*, void*);
typedef void (__cdecl *RpcPlayAnimation_fn)(void*, uint8_t, void*);
typedef void (__cdecl *RpcSnapTo_fn)(void*, float, float, void*);
typedef void (__cdecl *StartGame_fn)(void*, void*);
typedef void (__cdecl *RpcSetRole_fn)(void*, uint16_t, bool, void*);
typedef void (__cdecl *CmdCheckMurder_fn)(void*, void*, void*);
typedef void (__cdecl *CmdReportDeadBody_fn)(void*, void*, void*);
typedef void (__cdecl *RpcSetVisor_fn)(void*, void*, void*);
typedef void (__cdecl *RpcSetNamePlate_fn)(void*, void*, void*);
typedef void (__cdecl *RpcSetLevel_fn)(void*, uint32_t, void*);
typedef void (__cdecl *RpcShapeshift_fn)(void*, void*, bool, void*);
typedef void (__cdecl *RpcVanish_fn)(void*, void*);
typedef void (__cdecl *RpcAppear_fn)(void*, bool, void*);
typedef void (__cdecl *RpcVent_fn)(void*, int, void*);
typedef void (__cdecl *RpcCloseDoors_fn)(void*, int, void*);
typedef void (__cdecl *RpcUpdateSystem_fn)(void*, int, uint8_t, void*);
typedef void (__cdecl *RpcProtectPlayer_fn)(void*, void*, int, void*);

static void *gameDomain = nullptr;
static bool threadAttached = false;
static bool antiCheatPatched = false;

// Anti-cheat: rate limiter for RPCs to avoid server-side detection
static float lastRpcTime = 0;
static const float RPC_MIN_INTERVAL = 0.05f; // 50ms between RPCs

static bool CanSendRpc() {
  float now = (float)GetTickCount64() / 1000.f;
  if (now - lastRpcTime < RPC_MIN_INTERVAL) return false;
  lastRpcTime = now;
  return true;
}

template <typename T> static bool IsValid(T *ptr) {
  if (!ptr || (uintptr_t)ptr < 0x10000 || (uintptr_t)ptr > 0x7FFFFFFF)
    return false;
  return true;
}

// Anti-kick: patch KickPlayer to NOP (ret immediately)
static void PatchAntiKick() {
  if (antiCheatPatched || !gameAssembly) return;
  
  // KickPlayer on AmongUsClient — RVA: 0x6FB460
  // KickPlayer on InnerNetServer — RVA: 0x7011A0
  uintptr_t kickAddrs[] = { gameAssembly + 0x6FB460, gameAssembly + 0x7011A0 };
  
  for (auto addr : kickAddrs) {
    DWORD oldProt;
    if (VirtualProtect((void*)addr, 8, PAGE_EXECUTE_READWRITE, &oldProt)) {
      // x86 ret = 0xC3, pad with NOPs
      *(uint8_t*)(addr) = 0xC3;     // ret
      *(uint8_t*)(addr + 1) = 0x90; // nop
      *(uint8_t*)(addr + 2) = 0x90; // nop
      VirtualProtect((void*)addr, 8, oldProt, &oldProt);
    }
  }
  
  // Also patch CanKick() to always return false — RVA: 0x6F7230
  uintptr_t canKickAddr = gameAssembly + 0x6F7230;
  DWORD oldProt;
  if (VirtualProtect((void*)canKickAddr, 8, PAGE_EXECUTE_READWRITE, &oldProt)) {
    *(uint8_t*)(canKickAddr) = 0x31;      // xor eax, eax
    *(uint8_t*)(canKickAddr + 1) = 0xC0;
    *(uint8_t*)(canKickAddr + 2) = 0xC3;  // ret
    VirtualProtect((void*)canKickAddr, 8, oldProt, &oldProt);
  }
  
  antiCheatPatched = true;
  printf("[+] Anti-kick patches applied\n");
}

// Speed clamp: prevent server-side speed detection
static float ClampSpeed(float speed) {
  // Server allows up to ~3x normal speed before flagging
  return (speed > 3.0f) ? 3.0f : speed;
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
  if (!gameAssembly)
    return false;

  auto resolve = [](const char *name) -> void * {
    return (void *)GetProcAddress((HMODULE)gameAssembly, name);
  };

  il2cpp_domain_get = (il2cpp_domain_get_t)resolve("il2cpp_domain_get");
  il2cpp_domain_get_assemblies =
      (il2cpp_domain_get_assemblies_t)resolve("il2cpp_domain_get_assemblies");
  il2cpp_class_from_name =
      (il2cpp_class_from_name_t)resolve("il2cpp_class_from_name");
  il2cpp_string_new = (il2cpp_string_new_t)resolve("il2cpp_string_new");
  il2cpp_assembly_get_image =
      (il2cpp_assembly_get_image_t)resolve("il2cpp_assembly_get_image");
  il2cpp_class_get_field_from_name =
      (il2cpp_class_get_field_from_name_t)resolve(
          "il2cpp_class_get_field_from_name");
  il2cpp_field_static_get_value =
      (il2cpp_field_static_get_value_t)resolve("il2cpp_field_static_get_value");
  il2cpp_class_get_method_from_name =
      (il2cpp_class_get_method_from_name_t)resolve(
          "il2cpp_class_get_method_from_name");
  il2cpp_runtime_invoke =
      (il2cpp_runtime_invoke_t)resolve("il2cpp_runtime_invoke");
  il2cpp_thread_attach =
      (il2cpp_thread_attach_t)resolve("il2cpp_thread_attach");

  if (!il2cpp_domain_get || !il2cpp_class_from_name || !il2cpp_thread_attach)
    return false;

  gameDomain = il2cpp_domain_get();
  if (!gameDomain)
    return false;

  Attach();

  size_t asmCount = 0;
  void **assemblies = il2cpp_domain_get_assemblies(gameDomain, &asmCount);
  if (!assemblies)
    return false;

  for (size_t i = 0; i < asmCount; i++) {
    void *img = il2cpp_assembly_get_image(assemblies[i]);
    if (!img)
      continue;
    if (!PlayerControl::klass)
      PlayerControl::klass = il2cpp_class_from_name(img, "", "PlayerControl");
    if (!PlayerPhysics::klass)
      PlayerPhysics::klass = il2cpp_class_from_name(img, "", "PlayerPhysics");
    if (!GameData::klass)
      GameData::klass = il2cpp_class_from_name(img, "", "GameData");
    if (!AmongUsClient::klass)
      AmongUsClient::klass = il2cpp_class_from_name(img, "", "AmongUsClient");
    if (!ShipStatus::klass)
      ShipStatus::klass = il2cpp_class_from_name(img, "", "ShipStatus");
    if (!GameOptionsManager::klass)
      GameOptionsManager::klass = il2cpp_class_from_name(
          img, "AmongUs.GameOptions", "GameOptionsManager");
    if (!Transform::klass)
      Transform::klass =
          il2cpp_class_from_name(img, "UnityEngine", "Transform");
    if (!Behaviour::klass)
      Behaviour::klass =
          il2cpp_class_from_name(img, "UnityEngine", "Behaviour");
    if (!NetworkedPlayerInfo::klass)
      NetworkedPlayerInfo::klass =
          il2cpp_class_from_name(img, "", "NetworkedPlayerInfo");
  }

  // Apply anti-cheat patches
  PatchAntiKick();

  return (PlayerControl::klass && GameData::klass && AmongUsClient::klass &&
          GameOptionsManager::klass && Transform::klass && Behaviour::klass);
}

void *GetLocalPlayer() {
  if (!PlayerControl::klass)
    return nullptr;
  void *field =
      il2cpp_class_get_field_from_name(PlayerControl::klass, "LocalPlayer");
  if (!field)
    return nullptr;
  void *lp = nullptr;
  il2cpp_field_static_get_value(field, &lp);
  return IsValid(lp) ? lp : nullptr;
}

static void *get_playerName_method = nullptr;

static std::string ReadIl2CppString(void *str) {
  if (!str)
    return "Unknown";
  struct Il2CppString {
    void *klass;
    void *monitor;
    int32_t length;
    wchar_t chars[1];
  };
  Il2CppString *is = (Il2CppString *)str;
  std::wstring ws(is->chars, is->length);
  return std::string(ws.begin(), ws.end());
}

static std::string GetRoleName(uint16_t roleType) {
  switch (roleType) {
  case 0:
    return "Crewmate";
  case 1:
    return "Impostor";
  case 2:
    return "Scientist";
  case 3:
    return "Engineer";
  case 4:
    return "Guardian Angel";
  case 5:
    return "Shapeshifter";
  case 6:
    return "Crewmate (Ghost)";
  case 7:
    return "Impostor (Ghost)";
  case 8:
    return "Noisemaker";
  case 9:
    return "Phantom";
  case 10:
    return "Tracker";
  case 12:
    return "Detective";
  case 18:
    return "Viper";
  default:
    return "Unknown (" + std::to_string(roleType) + ")";
  }
}

static void UpdateInternal() {
  static float lastUpdateTime = 0;
  float currentTime = (float)ImGui::GetTime();

  if (currentTime - lastUpdateTime < 0.1f)
    return;
  lastUpdateTime = currentTime;

  if (!gameAssembly || !PlayerControl::klass)
    return;
  Attach();

  if (AmongUsClient::klass) {
    void *field =
        il2cpp_class_get_field_from_name(AmongUsClient::klass, "Instance");
    void *inst = nullptr;
    if (field)
      il2cpp_field_static_get_value(field, &inst);
    isInGame = IsValid(inst);
  }

  if (!isInGame) {
    players.clear();
    return;
  }

  if (!get_playerName_method && GameData::klass) {
    get_playerName_method =
        il2cpp_class_get_method_from_name(NetworkedPlayerInfo::klass, "get_PlayerName", 0);
  }

  void *lp = GetLocalPlayer();
  if (lp) {
    void *nt = *(void **)((uintptr_t)lp + 0x98); // NetTransform
    if (IsValid(nt)) {
      localX = *(float *)((uintptr_t)nt + 0x44); // lastPosition.x
      localY = *(float *)((uintptr_t)nt + 0x48); // lastPosition.y
    }

    void *coll = *(void **)((uintptr_t)lp + 0x90); // Collider
    if (IsValid(coll)) {
      static void *set_enabled_method = nullptr;
      if (!set_enabled_method && Behaviour::klass) {
        set_enabled_method =
            il2cpp_class_get_method_from_name(Behaviour::klass, "set_enabled", 1);
      }
      if (set_enabled_method) {
        bool enabled = !g_noclip;
        void *p[1] = {&enabled};
        il2cpp_runtime_invoke(set_enabled_method, coll, p, nullptr);
      }
    }

    // Set inVent as backup for NoClip
    *(bool *)((uintptr_t)lp + 0x48) = g_noclip;

    void *phys = *(void **)((uintptr_t)lp + 0x94); // MyPhysics
    if (IsValid(phys)) {
      *(float *)((uintptr_t)phys + 0x34) = g_speed; // Speed field
    }

    if (g_fullbright) {
      SetFullbright(true);
    }

    if (g_rainbow) {
      static float h = 0;
      h += 0.05f;
      if (h > 1.0f)
        h = 0;
      SetPlayerColor((int)(h * 10));
    }

    if (g_spin && gameAssembly) {
      static float spinTimer = 0;
      spinTimer += 0.1f;
      if (spinTimer > 0.3f) {
        spinTimer = 0;
        auto fn = (RpcPlayAnimation_fn)(gameAssembly + 0x5C8D80);
        fn(lp, 2, nullptr);
      }
    }

    // ── New continuous toggle features ──
    // 1. No Kill Cooldown — constantly reset kill timer to 0
    if (g_noKillCd)
      *(float *)((uintptr_t)lp + 0x80) = 0.f; // killTimer = 0

    // 2. Infinite Emergencies — set RemainingEmergencies to 999
    if (g_infiniteEmergencies)
      *(int *)((uintptr_t)lp + 0x84) = 999; // RemainingEmergencies

    // 3. Always Moveable — force moveable flag
    if (g_alwaysMoveable)
      *(bool *)((uintptr_t)lp + 0x38) = true; // moveable

    // 4. Impostor Vision — force high light mod every frame
    if (g_impostorVision)
      SetFullbright(true);

    // 5. Max Report Distance — see/report bodies from anywhere
    if (g_maxReportDist)
      *(float *)((uintptr_t)lp + 0x34) = 9999.f; // MaxReportDistance

    // 6. God Mode — prevent death by resetting IsDead
    if (g_godmode) {
      void *data = *(void **)((uintptr_t)lp + 0x58);
      if (IsValid(data))
        *(bool *)((uintptr_t)data + 0x54) = false; // IsDead = false
    }

    // 7. Color Cycle — rapidly cycle through all 18 colors
    if (g_colorCycle) {
      static float cc = 0;
      cc += 0.03f;
      if (cc > 18.f) cc = 0;
      if (gameAssembly) {
        auto fn = (RpcSetColor_fn)(gameAssembly + 0x5C9430);
        fn(lp, (uint8_t)(int)cc, nullptr);
      }
    }

    // 8. Spam Animation — rapidly play random animations
    if (g_spamAnim && gameAssembly) {
      static float saTimer = 0;
      saTimer += 0.1f;
      if (saTimer > 0.2f) {
        saTimer = 0;
        auto fn = (RpcPlayAnimation_fn)(gameAssembly + 0x5C8D80);
        fn(lp, (uint8_t)(rand() % 3), nullptr);
      }
    }

    // 9. Auto Tasks — complete tasks every few seconds
    if (g_autoTasks) {
      static float atTimer = 0;
      atTimer += 0.1f;
      if (atTimer > 3.0f) {
        atTimer = 0;
        CompleteAllTasks();
      }
    }

    // 10. Force Protect — constantly apply guardian angel shield
    if (g_forceProtect && gameAssembly) {
      static float fpTimer = 0;
      fpTimer += 0.1f;
      if (fpTimer > 2.0f) {
        fpTimer = 0;
        auto fn = (RpcProtectPlayer_fn)(gameAssembly + 0x5C8E70);
        fn(lp, lp, 0, nullptr); // protect self
      }
    }
  }

  if (PlayerControl::klass) {
    void *field = il2cpp_class_get_field_from_name(PlayerControl::klass,
                                                   "AllPlayerControls");
    void *allPlayersList = nullptr;
    if (field)
      il2cpp_field_static_get_value(field, &allPlayersList);

    if (IsValid(allPlayersList)) {
      struct SystemList {
        void *k;
        void *m;
        void *items;
        int size;
      };
      struct SystemArray {
        void *k;
        void *m;
        void *b;
        int len;
        void *m_Items[1];
      };

      SystemList *list = (SystemList *)allPlayersList;
      if (IsValid(list->items) && list->size > 0 && list->size <= 15) {
        SystemArray *arr = (SystemArray *)list->items;
        std::vector<PlayerInfo> temp;

        for (int i = 0; i < list->size; i++) {
          void *pcObj = arr->m_Items[i];
          if (!IsValid(pcObj))
            continue;

          void *data = *(void **)((uintptr_t)pcObj + 0x58); // CachedPlayerData
          if (!IsValid(data))
            continue;

          // 11. Freeze All — set all other players speed to 0
          if (g_freezeAll && pcObj != lp) {
            void *phys2 = *(void **)((uintptr_t)pcObj + 0x94);
            if (IsValid(phys2))
              *(float *)((uintptr_t)phys2 + 0x34) = 0.f;
          }

          PlayerInfo p;
          p.isDead = *(bool *)((uintptr_t)data + 0x54);
          uint16_t role = *(uint16_t *)((uintptr_t)data + 0x38);
          p.isImpostor = (role == 1 || role == 5 || role == 7 || role == 9);
          p.roleName = GetRoleName(role);

          if (get_playerName_method) {
            void *nameStr = il2cpp_runtime_invoke(get_playerName_method, data,
                                                  nullptr, nullptr);
            p.name = ReadIl2CppString(nameStr);
          } else {
            p.name = "Player " +
                     std::to_string(*(uint8_t *)((uintptr_t)pcObj + 0x28));
          }

          void *pcNt = *(void **)((uintptr_t)pcObj + 0x98);
          if (IsValid(pcNt)) {
            p.x = *(float *)((uintptr_t)pcNt + 0x44);
            p.y = *(float *)((uintptr_t)pcNt + 0x48);
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
  } __except (EXCEPTION_EXECUTE_HANDLER) {
  }
}

void SetSpeed(float speed) {
  Attach();
  void *lp = GetLocalPlayer();
  if (!lp)
    return;
  void *phys = *(void **)((uintptr_t)lp + 0x94);
  if (IsValid(phys)) {
    *(float *)((uintptr_t)phys + 0x34) = speed;
    *(float *)((uintptr_t)phys + 0x38) = speed;
  }
}

void SetFullbright(bool enabled) {
  Attach();
  if (ShipStatus::klass) {
    void *field = il2cpp_class_get_field_from_name(ShipStatus::klass, "Instance");
    void *inst = nullptr;
    if (field)
      il2cpp_field_static_get_value(field, &inst);
    if (IsValid(inst)) {
      *(float *)((uintptr_t)inst + 0x38) = enabled ? 100.f : 1.f; // MaxLightRadius
      *(float *)((uintptr_t)inst + 0x3C) = enabled ? 100.f : 1.f; // MinLightRadius
    }
  }

  // Also set vision in GameOptions for better effect
  if (GameOptionsManager::klass) {
    void *field = il2cpp_class_get_field_from_name(GameOptionsManager::klass,
                                                   "<Instance>k__BackingField");
    void *inst = nullptr;
    if (field)
      il2cpp_field_static_get_value(field, &inst);
    if (IsValid(inst)) {
      void *opt = *(void **)((uintptr_t)inst + 0x18); // currentNormalGameOptions
      if (IsValid(opt)) {
        *(float *)((uintptr_t)opt + 0x1C) = enabled ? 5.0f : 1.0f; // CrewLightMod
        *(float *)((uintptr_t)opt + 0x20) = enabled ? 5.0f : 1.0f; // ImpostorLightMod
      }
    }
  }
}

// Direct RVA function typedefs for IL2CPP compiled methods (x86 cdecl)


void CompleteAllTasks() {
  Attach();
  void *lp = GetLocalPlayer();
  if (!lp || !gameAssembly)
    return;

  // Direct RVA call — RpcCompleteTask RVA: 0x5C8C20
  auto RpcCompleteTask =
      (RpcCompleteTask_fn)(gameAssembly + 0x5C8C20);

  void *tasks = *(void **)((uintptr_t)lp + 0xAC); // myTasks (List<PlayerTask>)
  if (!IsValid(tasks))
    return;

  struct Il2CppList {
    void *klass;
    void *monitor;
    void *items; // Il2CppArray*
    int size;
  };
  struct Il2CppArray {
    void *klass;
    void *monitor;
    void *bounds;
    int max_length;
    void *m_Items[1];
  };

  __try {
    Il2CppList *list = (Il2CppList *)tasks;
    if (!IsValid(list->items) || list->size <= 0)
      return;
    Il2CppArray *arr = (Il2CppArray *)list->items;
    for (int i = 0; i < list->size && i < arr->max_length; i++) {
      void *task = arr->m_Items[i];
      if (!IsValid(task))
        continue;
      // PlayerTask.Index at 0x10, PlayerTask.Id at 0x14
      uint32_t idx = *(uint32_t *)((uintptr_t)task + 0x10); // Index
      RpcCompleteTask(lp, idx, nullptr);
    }
  } __except (EXCEPTION_EXECUTE_HANDLER) {
    printf("[!] Stara: CompleteAllTasks caught exception\n");
  }
}

void ForceEmergencyMeeting() {
  Attach();
  void *lp = GetLocalPlayer();
  if (!IsValid(lp) || !gameAssembly)
    return;

  // Direct RVA call — RpcStartMeeting RVA: 0x5C9F90
  auto RpcStartMeeting =
      (RpcStartMeeting_fn)(gameAssembly + 0x5C9F90);

  __try {
    // Pass nullptr = emergency meeting (no body found)
    RpcStartMeeting(lp, nullptr, nullptr);
  } __except (EXCEPTION_EXECUTE_HANDLER) {
    printf("[!] Stara: ForceEmergencyMeeting caught exception\n");
  }
}

// (typedefs moved to top of file)

void SetPlayerColor(int colorId) {
  Attach();
  void *lp = GetLocalPlayer();
  if (!IsValid(lp) || !gameAssembly) return;
  __try {
    auto fn = (RpcSetColor_fn)(gameAssembly + 0x5C9430);
    fn(lp, (uint8_t)colorId, nullptr);
  } __except(EXCEPTION_EXECUTE_HANDLER) {}
}

void TeleportTo(float x, float y) {
  Attach();
  void *lp = GetLocalPlayer();
  if (!IsValid(lp) || !gameAssembly) return;
  void *nt = *(void **)((uintptr_t)lp + 0x98);
  if (!IsValid(nt)) return;
  __try {
    // RpcSnapTo syncs position to all clients (RVA 0x535E60)
    auto fn = (RpcSnapTo_fn)(gameAssembly + 0x535E60);
    fn(nt, x, y, nullptr);
  } __except(EXCEPTION_EXECUTE_HANDLER) {}
}

void SetName(const char *name) {
  Attach();
  void *lp = GetLocalPlayer();
  if (!IsValid(lp) || !gameAssembly || !il2cpp_string_new) return;
  __try {
    auto fn = (RpcSetName_fn)(gameAssembly + 0x5C9790);
    void *s = il2cpp_string_new(name);
    fn(lp, s, nullptr);
  } __except(EXCEPTION_EXECUTE_HANDLER) {}
}

void SetKillCooldown(float time) {
  void *lp = GetLocalPlayer();
  if (IsValid(lp))
    *(float *)((uintptr_t)lp + 0x80) = time;
  if (!GameOptionsManager::klass) return;
  void *field = il2cpp_class_get_field_from_name(GameOptionsManager::klass, "<Instance>k__BackingField");
  void *inst = nullptr;
  if (field) il2cpp_field_static_get_value(field, &inst);
  if (IsValid(inst)) {
    void *opt = *(void **)((uintptr_t)inst + 0x18);
    if (IsValid(opt)) *(float *)((uintptr_t)opt + 0x24) = time;
  }
}

void SetKillDistance(float dist) {
  if (!GameOptionsManager::klass) return;
  void *field = il2cpp_class_get_field_from_name(GameOptionsManager::klass, "<Instance>k__BackingField");
  void *inst = nullptr;
  if (field) il2cpp_field_static_get_value(field, &inst);
  if (IsValid(inst)) {
    void *opt = *(void **)((uintptr_t)inst + 0x18);
    if (IsValid(opt)) *(int *)((uintptr_t)opt + 0x44) = (int)dist;
  }
}

void SetWallhack(bool enabled) { SetFullbright(enabled); }

void SetHat(int hatId) {
  Attach();
  void *lp = GetLocalPlayer();
  if (!IsValid(lp) || !gameAssembly || !il2cpp_string_new) return;
  const char* hatIds[] = {"hat_NoHat","hat_crown","hat_tophat","hat_beanie","hat_horns"};
  if (hatId < 0 || hatId > 4) return;
  __try {
    auto fn = (RpcSetHat_fn)(gameAssembly + 0x5C94F0);
    fn(lp, il2cpp_string_new(hatIds[hatId]), nullptr);
  } __except(EXCEPTION_EXECUTE_HANDLER) {}
}

void SetPet(int petId) {
  Attach();
  void *lp = GetLocalPlayer();
  if (!IsValid(lp) || !gameAssembly || !il2cpp_string_new) return;
  const char* petIds[] = {"pet_EmptyPet","pet_Crewmate","pet_Dog","pet_Cat","pet_Robot"};
  if (petId < 0 || petId > 4) return;
  __try {
    auto fn = (RpcSetPet_fn)(gameAssembly + 0x5C9850);
    fn(lp, il2cpp_string_new(petIds[petId]), nullptr);
  } __except(EXCEPTION_EXECUTE_HANDLER) {}
}

void SetSkin(int skinId) {
  Attach();
  void *lp = GetLocalPlayer();
  if (!IsValid(lp) || !gameAssembly || !il2cpp_string_new) return;
  const char* skinIds[] = {"skin_None","skin_Suit","skin_Astronaut","skin_Military","skin_Mech"};
  if (skinId < 0 || skinId > 4) return;
  __try {
    auto fn = (RpcSetSkin_fn)(gameAssembly + 0x5C9BC0);
    fn(lp, il2cpp_string_new(skinIds[skinId]), nullptr);
  } __except(EXCEPTION_EXECUTE_HANDLER) {}
}

void SetCharacterScale(float scale) {
  Attach();
  void *lp = GetLocalPlayer();
  if (!IsValid(lp)) return;
  // Write scale directly to the defaultCosmeticsScale field
  *(float *)((uintptr_t)lp + 0xA0) = scale;       // x
  *(float *)((uintptr_t)lp + 0xA4) = scale;       // y
  *(float *)((uintptr_t)lp + 0xA8) = 1.0f;        // z
}

void PlayAnimation(uint8_t animId) {
  Attach();
  void *lp = GetLocalPlayer();
  if (!IsValid(lp) || !gameAssembly) return;
  __try {
    auto fn = (RpcPlayAnimation_fn)(gameAssembly + 0x5C8D80);
    fn(lp, animId, nullptr);
  } __except(EXCEPTION_EXECUTE_HANDLER) {}
}

void DrawESP(ImDrawList *drawList) {
  if (!isInGame || players.empty()) return;
  for (const auto &p : players) {
    ImVec2 sc = {ImGui::GetIO().DisplaySize.x/2, ImGui::GetIO().DisplaySize.y/2};
    float sx = sc.x + (p.x - localX) * 35.f;
    float sy = sc.y - (p.y - localY) * 35.f;
    if (sx < -100 || sx > ImGui::GetIO().DisplaySize.x+100 ||
        sy < -100 || sy > ImGui::GetIO().DisplaySize.y+100) continue;
    ImU32 col = p.isImpostor ? IM_COL32(255,30,30,255) : IM_COL32(0,220,255,255);
    if (p.isDead) col = IM_COL32(150,150,150,200);
    if (g_espBox) {
      drawList->AddRect({sx-19,sy-29},{sx+19,sy+3}, IM_COL32(0,0,0,150), 4.f, 0, 1.f);
      drawList->AddRect({sx-18,sy-28},{sx+18,sy+2}, col, 4.f, 0, 1.8f);
    }
    float textY = sy - 44;
    if (g_espName) {
      std::string dn = p.name; if (p.isDead) dn += " [DEAD]";
      ImVec2 sz = ImGui::CalcTextSize(dn.c_str());
      drawList->AddText({sx-sz.x/2+1,textY+1}, IM_COL32(0,0,0,180), dn.c_str());
      drawList->AddText({sx-sz.x/2,textY}, col, dn.c_str());
      textY -= 14;
    }
    if (g_espDist) {
      char dbuf[16]; snprintf(dbuf, sizeof(dbuf), "%.1fm", p.distance);
      ImVec2 sz = ImGui::CalcTextSize(dbuf);
      drawList->AddText({sx-sz.x/2, sy+8}, IM_COL32(200,200,200,200), dbuf);
    }
    if (g_espRole) {
      ImVec2 sz = ImGui::CalcTextSize(p.roleName.c_str());
      ImU32 rc = p.isImpostor ? IM_COL32(255,80,80,255) : IM_COL32(100,255,100,255);
      drawList->AddText({sx-sz.x/2, sy+20}, rc, p.roleName.c_str());
    }
    if (g_espTracer) {
      ImVec2 bot = {ImGui::GetIO().DisplaySize.x/2.f, ImGui::GetIO().DisplaySize.y};
      drawList->AddLine(bot, {sx, sy+2}, (col & 0x00FFFFFF)|0x80000000, 1.2f);
    }
  }
}

void SpamChat(const char *text) {
  Attach();
  void *lp = GetLocalPlayer();
  if (!IsValid(lp) || !gameAssembly || !il2cpp_string_new) return;
  __try {
    // RpcSendChat RVA: 0x5C90C0
    auto fn = (RpcSendChat_fn)(gameAssembly + 0x5C90C0);
    fn(lp, il2cpp_string_new(text), nullptr);
  } __except(EXCEPTION_EXECUTE_HANDLER) {}
}

void EndGame() {
  Attach();
  if (!gameAssembly || !AmongUsClient::klass) return;
  void *field = il2cpp_class_get_field_from_name(AmongUsClient::klass, "Instance");
  void *inst = nullptr;
  if (field) il2cpp_field_static_get_value(field, &inst);
  if (!IsValid(inst)) return;
  __try {
    // AmongUsClient.StartGame RVA: 0x5487F0 — triggers end sequence
    // Actually use ExitGame: RVA 0x546850
    typedef void (__cdecl *ExitGame_fn)(void*, int, void*);
    auto fn = (ExitGame_fn)(gameAssembly + 0x546850);
    fn(inst, 0, nullptr); // reason = 0
  } __except(EXCEPTION_EXECUTE_HANDLER) {}
}

void StartGame() {
  Attach();
  if (!gameAssembly || !AmongUsClient::klass) return;
  void *field = il2cpp_class_get_field_from_name(AmongUsClient::klass, "Instance");
  void *inst = nullptr;
  if (field) il2cpp_field_static_get_value(field, &inst);
  if (!IsValid(inst)) return;
  __try {
    // AmongUsClient.StartGame() RVA: 0x5487F0
    auto fn = (StartGame_fn)(gameAssembly + 0x5487F0);
    fn(inst, nullptr);
  } __except(EXCEPTION_EXECUTE_HANDLER) {}
}

void TeleportToRoom(int roomId) {
  // Skeld room coords
  const float coords[][2] = {
    {-1.f, -1.f},     // 0: Cafeteria
    {-17.f, -5.f},    // 1: Reactor
    {6.5f, -3.5f},    // 2: Navigation
    {-8.8f, -3.f},    // 3: Medbay
    {-2.f, -16.f},    // 4: Electrical
    {3.5f, -12.f},    // 5: Storage
    {9.4f, 2.8f},     // 6: Weapons
    {-20.5f, -5.5f},  // 7: Upper Engine
    {-20.5f, -12.f},  // 8: Lower Engine
  };
  if (roomId >= 0 && roomId < 9)
    TeleportTo(coords[roomId][0], coords[roomId][1]);
}

// ── Helper: get a PlayerControl* by index from AllPlayerControls ──
static void *GetPlayerByIndex(int idx) {
  if (!PlayerControl::klass) return nullptr;
  void *field = il2cpp_class_get_field_from_name(PlayerControl::klass, "AllPlayerControls");
  void *list = nullptr;
  if (field) il2cpp_field_static_get_value(field, &list);
  if (!IsValid(list)) return nullptr;
  struct L { void *k; void *m; void *items; int size; };
  struct A { void *k; void *m; void *b; int len; void *m_Items[1]; };
  L *l = (L *)list;
  if (!IsValid(l->items) || idx < 0 || idx >= l->size) return nullptr;
  A *a = (A *)l->items;
  return (idx < a->len && IsValid(a->m_Items[idx])) ? a->m_Items[idx] : nullptr;
}

// RpcSetRole RVA: 0x5C99C0

void SetRole(int roleType) {
  Attach();
  void *lp = GetLocalPlayer();
  if (!IsValid(lp) || !gameAssembly) return;
  __try {
    auto fn = (RpcSetRole_fn)(gameAssembly + 0x5C99C0);
    fn(lp, (uint16_t)roleType, true, nullptr);
  } __except(EXCEPTION_EXECUTE_HANDLER) {}
}

// CmdCheckMurder RVA: 0x5C1A50

void KillPlayer(int playerIndex) {
  Attach();
  void *lp = GetLocalPlayer();
  void *target = GetPlayerByIndex(playerIndex);
  if (!IsValid(lp) || !IsValid(target) || !gameAssembly) return;
  __try {
    auto fn = (CmdCheckMurder_fn)(gameAssembly + 0x5C1A50);
    fn(lp, target, nullptr);
  } __except(EXCEPTION_EXECUTE_HANDLER) {}
}

void KillAllPlayers() {
  Attach();
  void *lp = GetLocalPlayer();
  if (!IsValid(lp) || !gameAssembly) return;
  auto fn = (CmdCheckMurder_fn)(gameAssembly + 0x5C1A50);
  // Iterate all players
  void *field = il2cpp_class_get_field_from_name(PlayerControl::klass, "AllPlayerControls");
  void *list = nullptr;
  if (field) il2cpp_field_static_get_value(field, &list);
  if (!IsValid(list)) return;
  struct L { void *k; void *m; void *items; int size; };
  struct A { void *k; void *m; void *b; int len; void *m_Items[1]; };
  L *l = (L *)list;
  if (!IsValid(l->items)) return;
  A *a = (A *)l->items;
  __try {
    for (int i = 0; i < l->size && i < a->len; i++) {
      void *p = a->m_Items[i];
      if (IsValid(p) && p != lp)
        fn(lp, p, nullptr);
    }
  } __except(EXCEPTION_EXECUTE_HANDLER) {}
}

// CmdReportDeadBody RVA: 0x5C2150

void ReportBody(int playerIndex) {
  Attach();
  void *lp = GetLocalPlayer();
  void *target = GetPlayerByIndex(playerIndex);
  if (!IsValid(lp) || !gameAssembly) return;
  void *data = nullptr;
  if (IsValid(target))
    data = *(void **)((uintptr_t)target + 0x58); // CachedPlayerData
  __try {
    auto fn = (CmdReportDeadBody_fn)(gameAssembly + 0x5C2150);
    fn(lp, data, nullptr); // null = self report / emergency
  } __except(EXCEPTION_EXECUTE_HANDLER) {}
}

// RpcSetVisor RVA: 0x5C9D90

void SetVisor(int visorId) {
  Attach();
  void *lp = GetLocalPlayer();
  if (!IsValid(lp) || !gameAssembly || !il2cpp_string_new) return;
  const char* ids[] = {"visor_EmptyVisor","visor_lollipopCrew","visor_lollipopImp","visor_starCrew","visor_pk01_AngeryVisor"};
  if (visorId < 0 || visorId > 4) return;
  __try {
    auto fn = (RpcSetVisor_fn)(gameAssembly + 0x5C9D90);
    fn(lp, il2cpp_string_new(ids[visorId]), nullptr);
  } __except(EXCEPTION_EXECUTE_HANDLER) {}
}

// RpcSetNamePlate RVA: 0x5C96C0

void SetNamePlate(int npId) {
  Attach();
  void *lp = GetLocalPlayer();
  if (!IsValid(lp) || !gameAssembly || !il2cpp_string_new) return;
  const char* ids[] = {"nameplate_NoPlate","nameplate_airship_Toppat","nameplate_airship_CCC","nameplate_airship_Government","nameplate_is_yard"};
  if (npId < 0 || npId > 4) return;
  __try {
    auto fn = (RpcSetNamePlate_fn)(gameAssembly + 0x5C96C0);
    fn(lp, il2cpp_string_new(ids[npId]), nullptr);
  } __except(EXCEPTION_EXECUTE_HANDLER) {}
}

// RpcSetLevel RVA: 0x5C9620

void SetLevel(int level) {
  Attach();
  void *lp = GetLocalPlayer();
  if (!IsValid(lp) || !gameAssembly) return;
  __try {
    auto fn = (RpcSetLevel_fn)(gameAssembly + 0x5C9620);
    fn(lp, (uint32_t)level, nullptr);
  } __except(EXCEPTION_EXECUTE_HANDLER) {}
}

void RevivePlayer() {
  Attach();
  void *lp = GetLocalPlayer();
  if (!IsValid(lp)) return;
  // Set IsDead = false on CachedPlayerData
  void *data = *(void **)((uintptr_t)lp + 0x58);
  if (IsValid(data))
    *(bool *)((uintptr_t)data + 0x54) = false; // IsDead = false
}

// RpcShapeshift RVA: 0x5C9ED0

void ShapeshiftTo(int playerIndex) {
  Attach();
  void *lp = GetLocalPlayer();
  void *target = GetPlayerByIndex(playerIndex);
  if (!IsValid(lp) || !IsValid(target) || !gameAssembly) return;
  __try {
    auto fn = (RpcShapeshift_fn)(gameAssembly + 0x5C9ED0);
    fn(lp, target, true, nullptr);
  } __except(EXCEPTION_EXECUTE_HANDLER) {}
}

// RpcVanish RVA: 0x5CA3E0

void Vanish() {
  Attach();
  void *lp = GetLocalPlayer();
  if (!IsValid(lp) || !gameAssembly) return;
  __try {
    auto fn = (RpcVanish_fn)(gameAssembly + 0x5CA3E0);
    fn(lp, nullptr);
  } __except(EXCEPTION_EXECUTE_HANDLER) {}
}

// RpcAppear RVA: 0x5C8BA0

void Appear() {
  Attach();
  void *lp = GetLocalPlayer();
  if (!IsValid(lp) || !gameAssembly) return;
  __try {
    auto fn = (RpcAppear_fn)(gameAssembly + 0x5C8BA0);
    fn(lp, true, nullptr);
  } __except(EXCEPTION_EXECUTE_HANDLER) {}
}

// Vent RPCs: Enter 0x5E3250, Exit 0x5E3340

void EnterVent(int ventId) {
  Attach();
  void *lp = GetLocalPlayer();
  if (!IsValid(lp) || !gameAssembly) return;
  void *phys = *(void **)((uintptr_t)lp + 0x94); // MyPhysics (PlayerPhysics)
  if (!IsValid(phys)) return;
  __try {
    auto fn = (RpcVent_fn)(gameAssembly + 0x5E3250);
    fn(phys, ventId, nullptr);
  } __except(EXCEPTION_EXECUTE_HANDLER) {}
}

void ExitVent(int ventId) {
  Attach();
  void *lp = GetLocalPlayer();
  if (!IsValid(lp) || !gameAssembly) return;
  void *phys = *(void **)((uintptr_t)lp + 0x94);
  if (!IsValid(phys)) return;
  __try {
    auto fn = (RpcVent_fn)(gameAssembly + 0x5E3340);
    fn(phys, ventId, nullptr);
  } __except(EXCEPTION_EXECUTE_HANDLER) {}
}

// RpcCloseDoorsOfType RVA: 0x637E00

void CloseDoors(int roomType) {
  Attach();
  if (!ShipStatus::klass || !gameAssembly) return;
  void *field = il2cpp_class_get_field_from_name(ShipStatus::klass, "Instance");
  void *inst = nullptr;
  if (field) il2cpp_field_static_get_value(field, &inst);
  if (!IsValid(inst)) return;
  __try {
    auto fn = (RpcCloseDoors_fn)(gameAssembly + 0x637E00);
    fn(inst, roomType, nullptr);
  } __except(EXCEPTION_EXECUTE_HANDLER) {}
}

// RpcUpdateSystem RVA: 0x637EB0

void RepairSabotage(int systemType) {
  Attach();
  if (!ShipStatus::klass || !gameAssembly) return;
  void *field = il2cpp_class_get_field_from_name(ShipStatus::klass, "Instance");
  void *inst = nullptr;
  if (field) il2cpp_field_static_get_value(field, &inst);
  if (!IsValid(inst)) return;
  __try {
    // amount = 128 fixes most sabotages
    auto fn = (RpcUpdateSystem_fn)(gameAssembly + 0x637EB0);
    fn(inst, systemType, 128, nullptr);
  } __except(EXCEPTION_EXECUTE_HANDLER) {}
}

void TeleportToPlayer(int playerIndex) {
  Attach();
  void *target = GetPlayerByIndex(playerIndex);
  if (!IsValid(target)) return;
  void *nt = *(void **)((uintptr_t)target + 0x98);
  if (!IsValid(nt)) return;
  float x = *(float *)((uintptr_t)nt + 0x44);
  float y = *(float *)((uintptr_t)nt + 0x48);
  TeleportTo(x, y);
}

// RpcProtectPlayer RVA: 0x5C8E70

void ProtectPlayer(int playerIndex) {
  Attach();
  void *lp = GetLocalPlayer();
  void *target = GetPlayerByIndex(playerIndex);
  if (!IsValid(lp) || !IsValid(target) || !gameAssembly) return;
  __try {
    auto fn = (RpcProtectPlayer_fn)(gameAssembly + 0x5C8E70);
    fn(lp, target, 0, nullptr);
  } __except(EXCEPTION_EXECUTE_HANDLER) {}
}

void CloseAllDoors() {
  // SystemTypes 0-14 covers all rooms on Skeld
  for (int i = 0; i <= 14; i++)
    CloseDoors(i);
}

void FixAllSabotage() {
  // Fix all system types
  for (int i = 0; i <= 14; i++)
    RepairSabotage(i);
}

void SendChat(const char* msg) {
  Attach();
  void *lp = GetLocalPlayer();
  if (!IsValid(lp) || !gameAssembly || !il2cpp_string_new) return;
  __try {
    auto fn = (RpcSendChat_fn)(gameAssembly + 0x5C90C0);
    fn(lp, il2cpp_string_new(msg), nullptr);
  } __except(EXCEPTION_EXECUTE_HANDLER) {}
}

void TeleportAllToSelf() {
  Attach();
  void *lp = GetLocalPlayer();
  if (!IsValid(lp) || !gameAssembly) return;
  void *myNt = *(void **)((uintptr_t)lp + 0x98);
  if (!IsValid(myNt)) return;
  float mx = *(float *)((uintptr_t)myNt + 0x44);
  float my = *(float *)((uintptr_t)myNt + 0x48);

  if (!PlayerControl::klass) return;
  void *field = il2cpp_class_get_field_from_name(PlayerControl::klass, "AllPlayerControls");
  void *list = nullptr;
  if (field) il2cpp_field_static_get_value(field, &list);
  if (!IsValid(list)) return;
  struct L { void *k; void *m; void *items; int size; };
  struct A { void *k; void *m; void *b; int len; void *m_Items[1]; };
  L *l = (L *)list;
  if (!IsValid(l->items)) return;
  A *a = (A *)l->items;
  auto fn = (RpcSnapTo_fn)(gameAssembly + 0x535E60);
  __try {
    for (int i = 0; i < l->size && i < a->len; i++) {
      void *p = a->m_Items[i];
      if (IsValid(p) && p != lp) {
        void *nt = *(void **)((uintptr_t)p + 0x98);
        if (IsValid(nt))
          fn(nt, mx, my, nullptr);
      }
    }
  } __except(EXCEPTION_EXECUTE_HANDLER) {}
}

void SetDiscussionTime(float time) {
  if (!GameOptionsManager::klass) return;
  void *field = il2cpp_class_get_field_from_name(GameOptionsManager::klass, "<Instance>k__BackingField");
  void *inst = nullptr;
  if (field) il2cpp_field_static_get_value(field, &inst);
  if (!IsValid(inst)) return;
  void *opt = *(void **)((uintptr_t)inst + 0x18);
  if (IsValid(opt))
    *(int *)((uintptr_t)opt + 0x2C) = (int)time; // DiscussionTime
}

void SetVotingTime(float time) {
  if (!GameOptionsManager::klass) return;
  void *field = il2cpp_class_get_field_from_name(GameOptionsManager::klass, "<Instance>k__BackingField");
  void *inst = nullptr;
  if (field) il2cpp_field_static_get_value(field, &inst);
  if (!IsValid(inst)) return;
  void *opt = *(void **)((uintptr_t)inst + 0x18);
  if (IsValid(opt))
    *(int *)((uintptr_t)opt + 0x30) = (int)time; // VotingTime
}

void SetEmergencyCount(int count) {
  void *lp = GetLocalPlayer();
  if (IsValid(lp))
    *(int *)((uintptr_t)lp + 0x84) = count; // RemainingEmergencies
}

} // namespace Stara::Game
