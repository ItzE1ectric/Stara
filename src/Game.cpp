#include "Game.hpp"
#include "DumpDatabase.hpp"
#include <windows.h>

namespace Stara::Game {

// Direct RVA function typedefs (IL2CPP x86: ret func(this, params...,
// MethodInfo*))
typedef void(__cdecl *RpcCompleteTask_fn)(void *, uint32_t, void *);
typedef void(__cdecl *RpcStartMeeting_fn)(void *, void *, void *);
typedef void(__cdecl *RpcSetColor_fn)(void *, uint8_t, void *);
typedef void(__cdecl *RpcSetName_fn)(void *, void *, void *);
typedef void(__cdecl *RpcSetHat_fn)(void *, void *, void *);
typedef void(__cdecl *RpcSetPet_fn)(void *, void *, void *);
typedef void(__cdecl *RpcSetSkin_fn)(void *, void *, void *);
typedef bool(__cdecl *RpcSendChat_fn)(void *, void *, void *); // returns bool
typedef void(__cdecl *RpcPlayAnimation_fn)(void *, uint8_t, void *);
typedef void(__cdecl *RpcSnapTo_fn)(void *, float, float, void *);
typedef void(__cdecl *StartGame_fn)(void *, void *);
typedef void(__cdecl *RpcSetRole_fn)(void *, uint16_t, bool, void *);
typedef void(__cdecl *CmdCheckMurder_fn)(void *, void *, void *);
typedef void(__cdecl *CmdReportDeadBody_fn)(void *, void *, void *);
typedef void(__cdecl *RpcSetVisor_fn)(void *, void *, void *);
typedef void(__cdecl *RpcSetNamePlate_fn)(void *, void *, void *);
typedef void(__cdecl *RpcSetLevel_fn)(void *, uint32_t, void *);
typedef void(__cdecl *RpcShapeshift_fn)(void *, void *, bool, void *);
typedef void(__cdecl *RpcVanish_fn)(void *, void *);
typedef void(__cdecl *RpcAppear_fn)(void *, bool, void *);
typedef void(__cdecl *RpcVent_fn)(void *, int, void *);
typedef void(__cdecl *RpcCloseDoors_fn)(void *, int, void *);
typedef void(__cdecl *RpcUpdateSystem_fn)(void *, int, uint8_t, void *);
typedef void(__cdecl *RpcProtectPlayer_fn)(void *, void *, int, void *);

static void *gameDomain = nullptr;
static bool antiCheatPatched = false;
static bool antiCheatPatchStateCaptured = false;
static uint8_t g_origKickPlayerClient[8] = {};
static uint8_t g_origKickPlayerServer[8] = {};
static uint8_t g_origCanKick[8] = {};

// Anti-cheat: rate limiter for RPCs to avoid server-side detection
static DWORD lastRpcTick = 0;
static const DWORD RPC_MIN_INTERVAL_MS = 100; // 100ms min between RPCs

static bool CanSendRpc() {
  DWORD now = GetTickCount();
  if (now - lastRpcTick < RPC_MIN_INTERVAL_MS)
    return false;
  lastRpcTick = now;
  return true;
}

// Separate rate limiter for cosmetic RPCs (less strict)
static DWORD lastCosmeticTick = 0;
static bool CanSendCosmeticRpc() {
  DWORD now = GetTickCount();
  if (now - lastCosmeticTick < 500)
    return false; // 500ms for cosmetics
  lastCosmeticTick = now;
  return true;
}

template <typename T> static bool IsValid(T *ptr) {
  if (!ptr)
    return false;

  constexpr uintptr_t kMinUserAddr = 0x10000;
#if INTPTR_MAX == INT64_MAX
  constexpr uintptr_t kMaxUserAddr = 0x00007FFFFFFFFFFFULL;
#else
  constexpr uintptr_t kMaxUserAddr = 0x7FFFFFFF;
#endif

  uintptr_t p = (uintptr_t)ptr;
  if (p < kMinUserAddr || p > kMaxUserAddr)
    return false;
  return true;
}

// Dump.cs-backed RVA table (fallbacks keep compatibility if dump is missing).
static uintptr_t g_rvaHandleDisconnect = 0x6F9400;
static uintptr_t g_rvaEnqueueDisconnect = 0x6F8020;
static uintptr_t g_rvaDisconnectInternal = 0x6F7A00;
static uintptr_t g_rvaOnDisconnect = 0x6FB700;
static uintptr_t g_rvaKickPlayerClient = 0x6FB460;
static uintptr_t g_rvaKickPlayerServer = 0x7011A0;
static uintptr_t g_rvaCanKick = 0x6F7230;

static uintptr_t g_rvaRpcSetColor = 0x5C9430;
static uintptr_t g_rvaRpcPlayAnimation = 0x5C8D80;
static uintptr_t g_rvaRpcProtectPlayer = 0x5C8E70;
static uintptr_t g_rvaRpcCompleteTask = 0x5C8C20;
static uintptr_t g_rvaRpcStartMeeting = 0x5C9F90;
static uintptr_t g_rvaRpcSnapTo = 0x535E60;
static uintptr_t g_rvaRpcSetName = 0x5C9790;
static uintptr_t g_rvaRpcSetHat = 0x5C94F0;
static uintptr_t g_rvaRpcSetPet = 0x5C9850;
static uintptr_t g_rvaRpcSetSkin = 0x5C9BC0;
static uintptr_t g_rvaRpcSendChat = 0x5C90C0;

static uintptr_t g_rvaCameraGetMain = 0x1F18980;
static uintptr_t g_rvaCameraGetOrthoSize = 0x1F18A70;

static uintptr_t g_rvaAmongUsClientExitGame = 0x546850;
static uintptr_t g_rvaAmongUsClientStartGame = 0x5487F0;
static uintptr_t g_rvaAmongUsClientSendStartGame = 0x6FD880;

static uintptr_t g_rvaRoleManagerSetRole = 0x60FA50;
static uintptr_t g_rvaRpcSetRole = 0x5C99C0;
static uintptr_t g_rvaMurderPlayer = 0x5C5D70;
static uintptr_t g_rvaRpcMurderPlayer = 0x5C8CC0;
static uintptr_t g_rvaCmdReportDeadBody = 0x5C2150;
static uintptr_t g_rvaRpcSetVisor = 0x5C9D90;
static uintptr_t g_rvaRpcSetNamePlate = 0x5C96C0;
static uintptr_t g_rvaRpcSetLevel = 0x5C9620;
static uintptr_t g_rvaRpcShapeshift = 0x5C9ED0;
static uintptr_t g_rvaRpcVanish = 0x5CA3E0;
static uintptr_t g_rvaRpcAppear = 0x5C8BA0;
static uintptr_t g_rvaRpcEnterVent = 0x5E3250;
static uintptr_t g_rvaRpcExitVent = 0x5E3340;
static uintptr_t g_rvaRpcCloseDoorsOfType = 0x637E00;
static uintptr_t g_rvaRpcUpdateSystem = 0x637EB0;

static void ResolveRvasFromDump() {
  using namespace DumpDatabase;
  g_rvaHandleDisconnect =
      GetMethodRva("InnerNetClient", "HandleDisconnect", g_rvaHandleDisconnect);
  g_rvaEnqueueDisconnect = GetMethodRva("InnerNetClient", "EnqueueDisconnect",
                                        g_rvaEnqueueDisconnect);
  g_rvaDisconnectInternal = GetMethodRva("InnerNetClient", "DisconnectInternal",
                                         g_rvaDisconnectInternal);
  g_rvaOnDisconnect =
      GetMethodRva("InnerNetClient", "OnDisconnect", g_rvaOnDisconnect);
  g_rvaKickPlayerClient =
      GetMethodRva("InnerNetClient", "KickPlayer", g_rvaKickPlayerClient);
  g_rvaKickPlayerServer =
      GetMethodRva("InnerNetServer", "KickPlayer", g_rvaKickPlayerServer);
  g_rvaCanKick = GetMethodRva("InnerNetClient", "CanKick", g_rvaCanKick);

  g_rvaRpcSetColor =
      GetMethodRva("PlayerControl", "RpcSetColor", g_rvaRpcSetColor);
  g_rvaRpcPlayAnimation =
      GetMethodRva("PlayerControl", "RpcPlayAnimation", g_rvaRpcPlayAnimation);
  g_rvaRpcProtectPlayer =
      GetMethodRva("PlayerControl", "RpcProtectPlayer", g_rvaRpcProtectPlayer);
  g_rvaRpcCompleteTask =
      GetMethodRva("PlayerControl", "RpcCompleteTask", g_rvaRpcCompleteTask);
  g_rvaRpcStartMeeting =
      GetMethodRva("PlayerControl", "RpcStartMeeting", g_rvaRpcStartMeeting);
  g_rvaRpcSnapTo =
      GetMethodRva("CustomNetworkTransform", "RpcSnapTo", g_rvaRpcSnapTo);
  g_rvaRpcSetName =
      GetMethodRva("PlayerControl", "RpcSetName", g_rvaRpcSetName);
  g_rvaRpcSetHat = GetMethodRva("PlayerControl", "RpcSetHat", g_rvaRpcSetHat);
  g_rvaRpcSetPet = GetMethodRva("PlayerControl", "RpcSetPet", g_rvaRpcSetPet);
  g_rvaRpcSetSkin =
      GetMethodRva("PlayerControl", "RpcSetSkin", g_rvaRpcSetSkin);
  g_rvaRpcSendChat =
      GetMethodRva("PlayerControl", "RpcSendChat", g_rvaRpcSendChat);

  g_rvaCameraGetMain = GetMethodRva("Camera", "get_main", g_rvaCameraGetMain);
  g_rvaCameraGetOrthoSize =
      GetMethodRva("Camera", "get_orthographicSize", g_rvaCameraGetOrthoSize);

  g_rvaAmongUsClientExitGame =
      GetMethodRva("AmongUsClient", "ExitGame", g_rvaAmongUsClientExitGame);
  g_rvaAmongUsClientStartGame =
      GetMethodRva("AmongUsClient", "StartGame", g_rvaAmongUsClientStartGame);
  // SendStartGame is on InnerNetClient (base class of AmongUsClient)
  g_rvaAmongUsClientSendStartGame = GetMethodRva(
      "InnerNetClient", "SendStartGame", g_rvaAmongUsClientSendStartGame);

  g_rvaRoleManagerSetRole =
      GetMethodRva("RoleManager", "SetRole", g_rvaRoleManagerSetRole);
  g_rvaRpcSetRole =
      GetMethodRva("PlayerControl", "RpcSetRole", g_rvaRpcSetRole);
  g_rvaMurderPlayer =
      GetMethodRva("PlayerControl", "MurderPlayer", g_rvaMurderPlayer);
  g_rvaRpcMurderPlayer =
      GetMethodRva("PlayerControl", "RpcMurderPlayer", g_rvaRpcMurderPlayer);
  g_rvaCmdReportDeadBody = GetMethodRva("PlayerControl", "CmdReportDeadBody",
                                        g_rvaCmdReportDeadBody);
  g_rvaRpcSetVisor =
      GetMethodRva("PlayerControl", "RpcSetVisor", g_rvaRpcSetVisor);
  g_rvaRpcSetNamePlate =
      GetMethodRva("PlayerControl", "RpcSetNamePlate", g_rvaRpcSetNamePlate);
  g_rvaRpcSetLevel =
      GetMethodRva("PlayerControl", "RpcSetLevel", g_rvaRpcSetLevel);
  g_rvaRpcShapeshift =
      GetMethodRva("PlayerControl", "RpcShapeshift", g_rvaRpcShapeshift);
  g_rvaRpcVanish = GetMethodRva("PlayerControl", "RpcVanish", g_rvaRpcVanish);
  g_rvaRpcAppear = GetMethodRva("PlayerControl", "RpcAppear", g_rvaRpcAppear);
  g_rvaRpcEnterVent =
      GetMethodRva("PlayerPhysics", "RpcEnterVent", g_rvaRpcEnterVent);
  g_rvaRpcExitVent =
      GetMethodRva("PlayerPhysics", "RpcExitVent", g_rvaRpcExitVent);
  g_rvaRpcCloseDoorsOfType = GetMethodRva("ShipStatus", "RpcCloseDoorsOfType",
                                          g_rvaRpcCloseDoorsOfType);
  g_rvaRpcUpdateSystem =
      GetMethodRva("ShipStatus", "RpcUpdateSystem", g_rvaRpcUpdateSystem);
}

// NOP helper: patches a function to immediately return
static void PatchToRet(uintptr_t addr) {
  DWORD oldProt;
  if (VirtualProtect((void *)addr, 4, PAGE_EXECUTE_READWRITE, &oldProt)) {
    *(uint8_t *)(addr) = 0xC3;     // ret
    *(uint8_t *)(addr + 1) = 0x90; // nop
    *(uint8_t *)(addr + 2) = 0x90; // nop
    VirtualProtect((void *)addr, 4, oldProt, &oldProt);
  }
}

// NOP helper: patches a bool-returning function to always return false
static void PatchToRetFalse(uintptr_t addr) {
  DWORD oldProt;
  if (VirtualProtect((void *)addr, 4, PAGE_EXECUTE_READWRITE, &oldProt)) {
    *(uint8_t *)(addr) = 0x31; // xor eax, eax
    *(uint8_t *)(addr + 1) = 0xC0;
    *(uint8_t *)(addr + 2) = 0xC3; // ret
    VirtualProtect((void *)addr, 4, oldProt, &oldProt);
  }
}

static void PatchAntiKick() {
  if (antiCheatPatched || !gameAssembly)
    return;

  // Safe anti-kick mode:
  // Patch only kick checks/handlers, DO NOT patch core disconnect handlers.
  // Patching disconnect flow causes "frozen lobby" ghost states after server
  // drops.
  uintptr_t kickClient = gameAssembly + g_rvaKickPlayerClient;
  uintptr_t kickServer = gameAssembly + g_rvaKickPlayerServer;
  uintptr_t canKick = gameAssembly + g_rvaCanKick;

  if (!antiCheatPatchStateCaptured) {
    memcpy(g_origKickPlayerClient, (void *)kickClient, 4);
    memcpy(g_origKickPlayerServer, (void *)kickServer, 4);
    memcpy(g_origCanKick, (void *)canKick, 4);
    antiCheatPatchStateCaptured = true;
  }

  PatchToRet(kickClient);
  PatchToRet(kickServer);
  PatchToRetFalse(canKick);

  antiCheatPatched = true;
  printf("[+] Anti-kick enabled (safe mode)\n");
}

static void UnpatchAntiKick() {
  if (!antiCheatPatched || !gameAssembly || !antiCheatPatchStateCaptured)
    return;

  uintptr_t kickClient = gameAssembly + g_rvaKickPlayerClient;
  uintptr_t kickServer = gameAssembly + g_rvaKickPlayerServer;
  uintptr_t canKick = gameAssembly + g_rvaCanKick;

  DWORD oldProt = 0;
  if (VirtualProtect((void *)kickClient, 4, PAGE_EXECUTE_READWRITE, &oldProt)) {
    memcpy((void *)kickClient, g_origKickPlayerClient, 4);
    VirtualProtect((void *)kickClient, 4, oldProt, &oldProt);
  }
  if (VirtualProtect((void *)kickServer, 4, PAGE_EXECUTE_READWRITE, &oldProt)) {
    memcpy((void *)kickServer, g_origKickPlayerServer, 4);
    VirtualProtect((void *)kickServer, 4, oldProt, &oldProt);
  }
  if (VirtualProtect((void *)canKick, 4, PAGE_EXECUTE_READWRITE, &oldProt)) {
    memcpy((void *)canKick, g_origCanKick, 4);
    VirtualProtect((void *)canKick, 4, oldProt, &oldProt);
  }

  antiCheatPatched = false;
  printf("[+] Anti-kick disabled\n");
}

// Speed clamp: prevent server-side speed detection
static float ClampSpeed(float speed) {
  // Server allows up to ~3x normal speed before flagging
  return (speed > 3.0f) ? 3.0f : speed;
}

void Attach() {
  // Each thread that calls IL2CPP must be registered with the GC.
  // Using thread_local ensures the render thread (DX11 Present hook) and
  // the init thread (DllMain) each get attached independently.
  // Without this: "Fatal error in GC: Collecting from unknown thread"
  static thread_local bool thisThreadAttached = false;
  if (gameDomain && il2cpp_thread_attach && !thisThreadAttached) {
    il2cpp_thread_attach(gameDomain);
    thisThreadAttached = true;
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
      GameOptionsManager::klass =
          il2cpp_class_from_name(img, "", "GameOptionsManager");
    if (!Transform::klass)
      Transform::klass =
          il2cpp_class_from_name(img, "UnityEngine", "Transform");
    if (!Behaviour::klass)
      Behaviour::klass =
          il2cpp_class_from_name(img, "UnityEngine", "Behaviour");
    if (!NetworkedPlayerInfo::klass)
      NetworkedPlayerInfo::klass =
          il2cpp_class_from_name(img, "", "NetworkedPlayerInfo");
    if (!RoleManager::klass)
      RoleManager::klass = il2cpp_class_from_name(img, "", "RoleManager");
    if (!RoleBehaviour::klass)
      RoleBehaviour::klass = il2cpp_class_from_name(img, "", "RoleBehaviour");
    if (!MeetingHud::klass)
      MeetingHud::klass = il2cpp_class_from_name(img, "", "MeetingHud");
  }

  DumpDatabase::AutoLoad();
  if (DumpDatabase::IsLoaded()) {
    printf("[+] Dump.cs linked (%zu methods, %zu fields, %zu types)\n",
           DumpDatabase::MethodCount(), DumpDatabase::FieldCount(),
           DumpDatabase::ClassCount());
  } else {
    printf("[!] Dump.cs not found. Using built-in RVA fallbacks.\n");
  }
  ResolveRvasFromDump();

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

// Check if we are the host (MalumMenu: AmongUsClient.Instance.AmHost)
bool IsHost() {
  if (!AmongUsClient::klass)
    return false;
  void *field =
      il2cpp_class_get_field_from_name(AmongUsClient::klass, "Instance");
  void *inst = nullptr;
  if (field)
    il2cpp_field_static_get_value(field, &inst);
  if (!IsValid(inst))
    return false;
  __try {
    static void *amHost_method = nullptr;
    static bool methodResolved = false;
    if (!methodResolved) {
      // Try AmongUsClient first, then its parent InnerNetClient
      amHost_method = il2cpp_class_get_method_from_name(
          AmongUsClient::klass, "get_AmHost", 0);
      methodResolved = true;
    }
    if (!amHost_method)
      return false;
    void *exc = nullptr;
    void *result = il2cpp_runtime_invoke(amHost_method, inst, nullptr, &exc);
    if (exc || !IsValid(result))
      return false;
    return *(bool *)((uintptr_t)result + sizeof(void *) * 2); // unbox bool
  } __except (EXCEPTION_EXECUTE_HANDLER) {
    return false;
  }
}

static void *get_playerName_method = nullptr;
static void *pc_setName_method = nullptr;
static void *pc_setLevel_method = nullptr;
static void *pc_setKillTimer_method = nullptr;
static void *pc_revive_method = nullptr;
static void *pc_completeTask_method = nullptr;
static void *pc_getTruePosition_method = nullptr;
static void *rb_getIsImpostor_method = nullptr;
static std::unordered_map<int, float> g_frozenPlayerSpeeds;
static bool g_freezeWasActive = false;
static void ApplyRoleToPlayer(void *targetPc, int roleType);
static void RevivePlayerControl(void *playerControl, bool syncRole);

static void EnsurePlayerControlMethods() {
  if (!PlayerControl::klass || !il2cpp_class_get_method_from_name)
    return;
  if (!pc_setName_method)
    pc_setName_method =
        il2cpp_class_get_method_from_name(PlayerControl::klass, "SetName", 1);
  if (!pc_setLevel_method)
    pc_setLevel_method =
        il2cpp_class_get_method_from_name(PlayerControl::klass, "SetLevel", 1);
  if (!pc_setKillTimer_method)
    pc_setKillTimer_method = il2cpp_class_get_method_from_name(
        PlayerControl::klass, "SetKillTimer", 1);
  if (!pc_revive_method)
    pc_revive_method =
        il2cpp_class_get_method_from_name(PlayerControl::klass, "Revive", 0);
  if (!pc_completeTask_method)
    pc_completeTask_method = il2cpp_class_get_method_from_name(
        PlayerControl::klass, "CompleteTask", 1);
  if (!pc_getTruePosition_method)
    pc_getTruePosition_method = il2cpp_class_get_method_from_name(
        PlayerControl::klass, "GetTruePosition", 0);
  if (!rb_getIsImpostor_method && RoleBehaviour::klass)
    rb_getIsImpostor_method = il2cpp_class_get_method_from_name(
        RoleBehaviour::klass, "get_IsImpostor", 0);
}

// SEH helper: validate and read Il2CppString length safely
static bool SafeReadStringData(void *str, int32_t *outLen, wchar_t **outChars) {
  __try {
    struct Il2CppString {
      void *klass;
      void *monitor;
      int32_t length;
      wchar_t chars[1];
    };
    Il2CppString *is = (Il2CppString *)str;
    if (is->length <= 0 || is->length > 256)
      return false;
    *outLen = is->length;
    *outChars = is->chars;
    return true;
  } __except (EXCEPTION_EXECUTE_HANDLER) {
    return false;
  }
}

static std::string ReadIl2CppString(void *str) {
  if (!str)
    return "Unknown";
  int32_t len = 0;
  wchar_t *chars = nullptr;
  if (!SafeReadStringData(str, &len, &chars))
    return "Unknown";
  int needed =
      WideCharToMultiByte(CP_UTF8, 0, chars, len, nullptr, 0, nullptr, nullptr);
  if (needed <= 0)
    return "Unknown";
  std::string out((size_t)needed, '\0');
  WideCharToMultiByte(CP_UTF8, 0, chars, len, out.data(), needed, nullptr,
                      nullptr);
  return out;
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

static bool TryReadBoxedBool(void *boxed, bool &out) {
  if (!IsValid(boxed))
    return false;
  __try {
    out = *(bool *)((uintptr_t)boxed + sizeof(void *) * 2);
    return true;
  } __except (EXCEPTION_EXECUTE_HANDLER) {
    return false;
  }
}

static bool TryGetPlayerWorldPos(void *pcObj, float &outX, float &outY) {
  if (!IsValid(pcObj))
    return false;

  EnsurePlayerControlMethods();
  if (pc_getTruePosition_method) {
    void *posBox = il2cpp_runtime_invoke(pc_getTruePosition_method, pcObj,
                                         nullptr, nullptr);
    if (IsValid(posBox)) {
      // Boxed Vector2 layout: klass, monitor, then x/y payload.
      float *xy = (float *)((uintptr_t)posBox + sizeof(void *) * 2);
      if (std::isfinite(xy[0]) && std::isfinite(xy[1]) &&
          fabsf(xy[0]) < 5000.f && fabsf(xy[1]) < 5000.f) {
        outX = xy[0];
        outY = xy[1];
        return true;
      }
    }
  }

  // Fallback to CustomNetworkTransform.lastPosition.
  void *pcNt = *(void **)((uintptr_t)pcObj + 0x98);
  if (!IsValid(pcNt))
    return false;
  float x = *(float *)((uintptr_t)pcNt + 0x44);
  float y = *(float *)((uintptr_t)pcNt + 0x48);
  if (!std::isfinite(x) || !std::isfinite(y) || fabsf(x) > 5000.f ||
      fabsf(y) > 5000.f)
    return false;
  outX = x;
  outY = y;
  return true;
}

static void UpdateCameraState(); // forward decl

// Read color from PlayerControl.cosmetics.get_ColorId (SEH-safe helper)
static int ReadColorId(void *pcObj) {
  if (!IsValid(pcObj))
    return -1;
  static void *getColorId_method = nullptr;
  static bool colorMethodResolved = false;
  void *cosmetics = *(void **)((uintptr_t)pcObj + 0x3C); // PlayerControl.cosmetics
  if (!IsValid(cosmetics))
    return -1;
  if (!colorMethodResolved) {
    void *cosKlass = *(void **)cosmetics;
    if (IsValid(cosKlass))
      getColorId_method =
          il2cpp_class_get_method_from_name(cosKlass, "get_ColorId", 0);
    colorMethodResolved = true;
  }
  if (!getColorId_method)
    return -1;
  __try {
    void *exc = nullptr;
    void *boxed =
        il2cpp_runtime_invoke(getColorId_method, cosmetics, nullptr, &exc);
    if (!exc && IsValid(boxed))
      return *(int *)((uintptr_t)boxed + sizeof(void *) * 2);
  } __except (EXCEPTION_EXECUTE_HANDLER) {
  }
  return -1;
}

static void UpdateInternal() {
  static float lastUpdateTime = 0;
  float currentTime = (float)ImGui::GetTime();

  if (currentTime - lastUpdateTime < 0.033f)
    return;
  lastUpdateTime = currentTime;

  if (!gameAssembly || !PlayerControl::klass)
    return;
  Attach();

  if (g_antiKick) {
    PatchAntiKick();
  } else {
    UnpatchAntiKick();
  }

  // Determine isInGame/isInLobby: require AmongUsClient.Instance + GameState + LocalPlayer.
  // InnerNetClient.GameState at offset 0x64: 0=NotJoined, 1=Joined(lobby), 2=Started, 3=Ended
  {
    bool hasClient = false;
    int gameState = 0;
    if (AmongUsClient::klass) {
      void *field =
          il2cpp_class_get_field_from_name(AmongUsClient::klass, "Instance");
      void *inst = nullptr;
      if (field)
        il2cpp_field_static_get_value(field, &inst);
      if (IsValid(inst)) {
        hasClient = true;
        gameState = *(int *)((uintptr_t)inst + 0x64); // GameState
      }
    }
    void *lp = GetLocalPlayer();
    isInGame = hasClient && (gameState == 2) && IsValid(lp);  // Started
    isInLobby = hasClient && (gameState == 1) && IsValid(lp); // Joined (lobby)
  }

  isInMeeting = false;
  if (MeetingHud::klass) {
    void *field =
        il2cpp_class_get_field_from_name(MeetingHud::klass, "Instance");
    void *meeting = nullptr;
    if (field)
      il2cpp_field_static_get_value(field, &meeting);
    isInMeeting = IsValid(meeting);
  }

  if (!isInGame && !isInLobby) {
    players.clear();
    isInMeeting = false;
    return;
  }

  // Update camera for ESP (in-game only)
  if (isInGame)
    UpdateCameraState();

  if (!get_playerName_method && GameData::klass) {
    get_playerName_method = il2cpp_class_get_method_from_name(
        NetworkedPlayerInfo::klass, "get_PlayerName", 0);
  }

  void *lp = GetLocalPlayer();
  if (lp) {
    void *nt = *(void **)((uintptr_t)lp + 0x98); // NetTransform
    if (IsValid(nt)) {
      localX = *(float *)((uintptr_t)nt + 0x44); // lastPosition.x
      localY = *(float *)((uintptr_t)nt + 0x48); // lastPosition.y
    } else {
      float tx = 0.f, ty = 0.f;
      if (TryGetPlayerWorldPos(lp, tx, ty)) {
        localX = tx;
        localY = ty;
      }
    }

    // NoClip: properly disable collision without freezing player
    {
      static void *set_enabled_method = nullptr;
      if (!set_enabled_method && Behaviour::klass)
        set_enabled_method = il2cpp_class_get_method_from_name(
            Behaviour::klass, "set_enabled", 1);
      void *coll = *(void **)((uintptr_t)lp + 0x90); // Collider2D
      if (IsValid(coll) && set_enabled_method) {
        bool val = !g_noclip; // true = collider on, false = collider off
        void *p[1] = {&val};
        il2cpp_runtime_invoke(set_enabled_method, coll, p, nullptr);
      }
      if (g_noclip)
        *(bool *)((uintptr_t)lp + 0x38) = true; // keep moveable=true
    }

    void *phys = *(void **)((uintptr_t)lp + 0x94); // MyPhysics
    if (IsValid(phys)) {
      static float smoothedSpeed = 2.5f;
      float desiredSpeed = (g_walkSpeed > 0.01f) ? g_walkSpeed : g_speed;
      desiredSpeed = std::clamp(desiredSpeed, 0.5f, 15.f);
      if (g_smoothMove) {
        smoothedSpeed += (desiredSpeed - smoothedSpeed) * 0.18f;
      } else {
        smoothedSpeed = desiredSpeed;
      }
      *(float *)((uintptr_t)phys + 0x34) = smoothedSpeed; // Speed field
    }

    {
      static bool wasFbOn = false;
      if (g_fullbright) {
        SetFullbright(true);
        wasFbOn = true;
      } else if (wasFbOn) {
        SetFullbright(false);
        wasFbOn = false;
      }
    }

    if (g_rainbow && CanSendCosmeticRpc()) {
      static int h = 0;
      h = (h + 1) % 18;
      if (gameAssembly) {
        auto fn = (RpcSetColor_fn)(gameAssembly + g_rvaRpcSetColor);
        fn(lp, (uint8_t)h, nullptr);
      }
    }

    if (g_spin && gameAssembly && CanSendRpc()) {
      auto fn = (RpcPlayAnimation_fn)(gameAssembly + g_rvaRpcPlayAnimation);
      fn(lp, 2, nullptr);
    }

    if (g_dance && gameAssembly && CanSendRpc()) {
      static uint8_t danceAnim = 0;
      auto fn = (RpcPlayAnimation_fn)(gameAssembly + g_rvaRpcPlayAnimation);
      fn(lp, danceAnim, nullptr);
      danceAnim = (uint8_t)((danceAnim + 1) % 3);
    }

    if (g_particle && gameAssembly && CanSendCosmeticRpc()) {
      // Cycle color while particle mode is active so the visual effect is
      // obvious.
      static int particleHue = 0;
      particleHue = (particleHue + 1) % 18;
      auto fn = (RpcSetColor_fn)(gameAssembly + g_rvaRpcSetColor);
      fn(lp, (uint8_t)particleHue, nullptr);
    }

    {
      static bool pathInit = false;
      static float anchorX = 0.f, anchorY = 0.f;
      static float theta = 0.f;
      static float pathTimer = 0.f;
      // Reset state when toggled off so anchor re-centers next enable
      if (!g_autoPath) {
        pathInit = false;
      } else if (gameAssembly) {
        if (!pathInit) {
          pathInit = true;
          anchorX = localX;
          anchorY = localY;
          theta = 0.f;
          pathTimer = 0.f;
        }
        pathTimer += 0.1f;
        if (pathTimer >= 0.2f) {
          pathTimer = 0.f;
          theta += 0.28f;
          float radius = 0.85f;
          TeleportTo(anchorX + cosf(theta) * radius,
                     anchorY + sinf(theta) * radius);
        }
      }
    }

    // ── Continuous toggle features (in-game only) ──
    if (isInGame) {
      // 1. No Kill Cooldown — constantly reset kill timer to 0
      if (g_noKillCd) {
        EnsurePlayerControlMethods();
        if (pc_setKillTimer_method) {
          float v = 0.f;
          void *p[1] = {&v};
          il2cpp_runtime_invoke(pc_setKillTimer_method, lp, p, nullptr);
        }
        // Also write directly to the killTimer field as backup
        *(float *)((uintptr_t)lp + 0x80) = 0.f;
      }

      // 2. Infinite Emergencies — set RemainingEmergencies to 999
      if (g_infiniteEmergencies)
        *(int *)((uintptr_t)lp + 0x84) = 999; // RemainingEmergencies

      // 3. Always Moveable — force moveable flag
      if (g_alwaysMoveable)
        *(bool *)((uintptr_t)lp + 0x38) = true; // moveable

      // 4. Impostor Vision — set crew vision to impostor level via GameOptions
      if (g_impostorVision && GameOptionsManager::klass) {
        void *gomField = il2cpp_class_get_field_from_name(
            GameOptionsManager::klass, "<Instance>k__BackingField");
        void *gomInst = nullptr;
        if (gomField)
          il2cpp_field_static_get_value(gomField, &gomInst);
        if (IsValid(gomInst)) {
          void *opt = *(void **)((uintptr_t)gomInst + 0x18);
          if (IsValid(opt)) {
            float impVision =
                *(float *)((uintptr_t)opt + 0x20); // ImpostorLightMod
            *(float *)((uintptr_t)opt + 0x1C) =
                impVision; // CrewLightMod = ImpostorLightMod
          }
        }
      }

      // 5. Max Report Distance — see/report bodies from anywhere
      if (g_maxReportDist)
        *(float *)((uintptr_t)lp + 0x34) = 9999.f; // MaxReportDistance

      // 6. God Mode — prevent death by resetting IsDead
      if (g_godmode) {
        void *data = *(void **)((uintptr_t)lp + 0x58);
        if (IsValid(data)) {
          bool dead = *(bool *)((uintptr_t)data + 0x54);
          *(bool *)((uintptr_t)data + 0x54) = false; // IsDead = false
          *(bool *)((uintptr_t)data + 0x55) = false; // WasEjected = false
          if (dead)
            RevivePlayerControl(lp, false);
        }
      }

      // 7. Color Cycle — rate-limited to avoid spam
      if (g_colorCycle && CanSendCosmeticRpc() && gameAssembly) {
        static int cc = 0;
        cc = (cc + 1) % 18;
        auto fn = (RpcSetColor_fn)(gameAssembly + g_rvaRpcSetColor);
        fn(lp, (uint8_t)cc, nullptr);
      }

      // 8. Spam Animation — rate-limited
      if (g_spamAnim && gameAssembly && CanSendRpc()) {
        auto fn =
            (RpcPlayAnimation_fn)(gameAssembly + g_rvaRpcPlayAnimation);
        fn(lp, (uint8_t)(rand() % 3), nullptr);
      }

      // 9. Auto Tasks — complete tasks every 5 seconds
      if (g_autoTasks) {
        static float atTimer = 0;
        atTimer += 0.1f;
        if (atTimer > 5.0f) {
          atTimer = 0;
          CompleteAllTasks();
        }
      }

      // 10. Force Protect — rate-limited to every 5s
      if (g_forceProtect && gameAssembly) {
        static float fpTimer = 0;
        fpTimer += 0.1f;
        if (fpTimer > 5.0f) {
          fpTimer = 0;
          auto fn =
              (RpcProtectPlayer_fn)(gameAssembly + g_rvaRpcProtectPlayer);
          fn(lp, lp, 0, nullptr);
        }
      }
    } // end isInGame toggles
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
      if (IsValid(list->items) && list->size > 0 && list->size <= 32) {
        SystemArray *arr = (SystemArray *)list->items;
        std::vector<PlayerInfo> temp;

        for (int i = 0; i < list->size; i++) {
          void *pcObj = arr->m_Items[i];
          if (!IsValid(pcObj))
            continue;

          void *data = *(void **)((uintptr_t)pcObj + 0x58); // CachedPlayerData
          if (!IsValid(data))
            continue;

          int pid = (int)*(uint8_t *)((uintptr_t)pcObj + 0x28);

          // 11. Freeze All — local visual only (sets speed locally)
          if (g_freezeAll && pcObj != lp) {
            void *phys2 = *(void **)((uintptr_t)pcObj + 0x94);
            if (IsValid(phys2)) {
              if (!g_frozenPlayerSpeeds.count(pid))
                g_frozenPlayerSpeeds[pid] = *(float *)((uintptr_t)phys2 + 0x34);
              *(float *)((uintptr_t)phys2 + 0x34) = 0.f;
            }
          }

          PlayerInfo p{};
          p.playerId = pid;
          p.isDead = *(bool *)((uintptr_t)data + 0x54);
          p.hasWorldPos = false;
          p.distance = -1.f;
          uint16_t role = *(uint16_t *)((uintptr_t)data + 0x38);
          bool hasRoleWhenAlive = *(bool *)((uintptr_t)data + 0x3C);
          if (hasRoleWhenAlive) {
            uint16_t roleWhenAlive = *(uint16_t *)((uintptr_t)data + 0x3A);
            if (role == 0 || role == 6)
              role = roleWhenAlive;
          }
          p.isImpostor = (role == 1 || role == 5 || role == 7 || role == 9 || role == 18);

          // Pull stronger role/impostor hints from RoleBehaviour when
          // available.
          void *roleObj = *(void **)((uintptr_t)data + 0x4C);
          if (IsValid(roleObj)) {
            uint16_t rbRole =
                *(uint16_t *)((uintptr_t)roleObj + 0x10); // RoleBehaviour.Role
            if (rbRole != 0 || role == 0 || role == 6)
              role = rbRole;

            int teamType =
                *(int *)((uintptr_t)roleObj + 0x4C); // RoleBehaviour.TeamType
            if (teamType == 1)
              p.isImpostor = true;

            if (rb_getIsImpostor_method) {
              bool rbImp = false;
              void *boxed = il2cpp_runtime_invoke(rb_getIsImpostor_method,
                                                  roleObj, nullptr, nullptr);
              if (TryReadBoxedBool(boxed, rbImp) && rbImp)
                p.isImpostor = true;
            }
          }
          p.roleName = GetRoleName(role);
          if (isInMeeting && p.isImpostor)
            p.roleName = "Impostor";

          if (get_playerName_method) {
            void *nameStr = il2cpp_runtime_invoke(get_playerName_method, data,
                                                  nullptr, nullptr);
            p.name = ReadIl2CppString(nameStr);
          } else {
            p.name = "Player " + std::to_string(p.playerId);
          }

          p.hasWorldPos = TryGetPlayerWorldPos(pcObj, p.x, p.y);
          if (!p.hasWorldPos) {
            p.x = localX;
            p.y = localY;
          }

          if (pcObj == lp) {
            isImpostor = p.isImpostor;
            localRole = role;
            localRoleName = p.roleName;
            // Read level from NetworkedPlayerInfo (offset 0x44)
            // Game stores level as (displayLevel - 1), so add 1 for display
            if (IsValid(data))
              localLevel = (int)*(uint32_t *)((uintptr_t)data + 0x44) + 1;
            // Read color from CosmeticsLayer.get_ColorId
            localColorId = ReadColorId(pcObj);
          } else {
            if (p.hasWorldPos)
              p.distance = sqrtf(powf(p.x - localX, 2) + powf(p.y - localY, 2));
            temp.push_back(p);
          }
        }
        players = temp;

        if (!g_freezeAll && g_freezeWasActive) {
          for (int i = 0; i < list->size; i++) {
            void *pcObj = arr->m_Items[i];
            if (!IsValid(pcObj) || pcObj == lp)
              continue;
            int pid = (int)*(uint8_t *)((uintptr_t)pcObj + 0x28);
            auto it = g_frozenPlayerSpeeds.find(pid);
            if (it == g_frozenPlayerSpeeds.end())
              continue;
            void *phys2 = *(void **)((uintptr_t)pcObj + 0x94);
            if (IsValid(phys2))
              *(float *)((uintptr_t)phys2 + 0x34) = it->second;
          }
          g_frozenPlayerSpeeds.clear();
        }
        g_freezeWasActive = g_freezeAll;
      }
    }
  }

  if (g_chatSpam) {
    static float lastSpam = 0;
    static int spamSeq = 1;
    if (currentTime - lastSpam > 2.0f) { // 2s interval to avoid spam detection
      lastSpam = currentTime;
      const char *base = g_chatBuf[0] ? g_chatBuf : "Stara Client";
      char msg[196];
      snprintf(msg, sizeof(msg), "%s [%d]", base, spamSeq++);
      SpamChat(msg);
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
  g_speed = speed;
  g_walkSpeed = speed;
  void *lp = GetLocalPlayer();
  if (!lp)
    return;
  void *phys = *(void **)((uintptr_t)lp + 0x94);
  if (IsValid(phys)) {
    *(float *)((uintptr_t)phys + 0x34) = speed;
  }
}

void SetFullbright(bool enabled) {
  Attach();
  if (ShipStatus::klass) {
    void *field =
        il2cpp_class_get_field_from_name(ShipStatus::klass, "Instance");
    void *inst = nullptr;
    if (field)
      il2cpp_field_static_get_value(field, &inst);
    if (IsValid(inst)) {
      *(float *)((uintptr_t)inst + 0x38) =
          enabled ? 100.f : 1.f; // MaxLightRadius
      *(float *)((uintptr_t)inst + 0x3C) =
          enabled ? 100.f : 1.f; // MinLightRadius
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
      void *opt =
          *(void **)((uintptr_t)inst + 0x18); // currentNormalGameOptions
      if (IsValid(opt)) {
        *(float *)((uintptr_t)opt + 0x1C) =
            enabled ? 5.0f : 1.0f; // CrewLightMod
        *(float *)((uintptr_t)opt + 0x20) =
            enabled ? 5.0f : 1.0f; // ImpostorLightMod
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

  EnsurePlayerControlMethods();
  auto rpcCompleteTask =
      (RpcCompleteTask_fn)(gameAssembly + g_rvaRpcCompleteTask);

  // Try two known offsets for myTasks pointer
  void *tasks = nullptr;
  for (uintptr_t off : {0xACu, 0xA8u, 0xB0u}) {
    void *t = *(void **)((uintptr_t)lp + off);
    if (IsValid(t)) {
      tasks = t;
      break;
    }
  }
  bool completedAny = false;
  if (!IsValid(tasks)) {
    // Fallback: brute task ids when myTasks pointer layout changes.
    for (uint32_t taskId = 0; taskId < 64; taskId++) {
      __try {
        if (pc_completeTask_method) {
          void *p[1] = {&taskId};
          il2cpp_runtime_invoke(pc_completeTask_method, lp, p, nullptr);
        }
        rpcCompleteTask(lp, taskId, nullptr);
      } __except (EXCEPTION_EXECUTE_HANDLER) {
      }
    }
    return;
  }

  struct Il2CppList {
    void *klass;
    void *monitor;
    void *items;
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
    if (!IsValid(list->items) || list->size <= 0 || list->size > 64)
      return;
    Il2CppArray *arr = (Il2CppArray *)list->items;
    for (int i = 0; i < list->size && i < arr->max_length; i++) {
      void *task = arr->m_Items[i];
      if (!IsValid(task))
        continue;
      uint32_t taskId = *(uint32_t *)((uintptr_t)task + 0x14);
      // Complete locally first
      if (pc_completeTask_method) {
        void *p[1] = {&taskId};
        il2cpp_runtime_invoke(pc_completeTask_method, lp, p, nullptr);
      }
      // Then sync to network
      rpcCompleteTask(lp, taskId, nullptr);
      completedAny = true;
      Sleep(20); // small delay between tasks to avoid server spam rejection
    }
  } __except (EXCEPTION_EXECUTE_HANDLER) {
  }

  if (!completedAny) {
    for (uint32_t taskId = 0; taskId < 64; taskId++) {
      __try {
        if (pc_completeTask_method) {
          void *p[1] = {&taskId};
          il2cpp_runtime_invoke(pc_completeTask_method, lp, p, nullptr);
        }
        rpcCompleteTask(lp, taskId, nullptr);
        Sleep(20);
      } __except (EXCEPTION_EXECUTE_HANDLER) {
      }
    }
  }
}

void ForceEmergencyMeeting() {
  Attach();
  void *lp = GetLocalPlayer();
  if (!IsValid(lp) || !gameAssembly)
    return;

  __try {
    if (IsHost()) {
      // Host: use RpcStartMeeting(null) — bypasses meeting checks
      auto fn = (RpcStartMeeting_fn)(gameAssembly + g_rvaRpcStartMeeting);
      fn(lp, nullptr, nullptr);
    } else {
      // Non-host: use CmdReportDeadBody(null) — normal report flow
      auto fn = (CmdReportDeadBody_fn)(gameAssembly + g_rvaCmdReportDeadBody);
      fn(lp, nullptr, nullptr);
    }
  } __except (EXCEPTION_EXECUTE_HANDLER) {
    printf("[!] Stara: ForceEmergencyMeeting caught exception\n");
  }
}

// (typedefs moved to top of file)

void SetPlayerColor(int colorId) {
  Attach();
  void *lp = GetLocalPlayer();
  if (!IsValid(lp) || !gameAssembly)
    return;
  __try {
    auto fn = (RpcSetColor_fn)(gameAssembly + g_rvaRpcSetColor);
    fn(lp, (uint8_t)colorId, nullptr);
  } __except (EXCEPTION_EXECUTE_HANDLER) {
  }
}

void TeleportTo(float x, float y) {
  Attach();
  void *lp = GetLocalPlayer();
  if (!IsValid(lp) || !gameAssembly)
    return;
  void *nt = *(void **)((uintptr_t)lp + 0x98);
  if (!IsValid(nt))
    return;
  __try {
    // RpcSnapTo syncs position to all clients (RVA 0x535E60)
    auto fn = (RpcSnapTo_fn)(gameAssembly + g_rvaRpcSnapTo);
    fn(nt, x, y, nullptr);
  } __except (EXCEPTION_EXECUTE_HANDLER) {
  }
}

void SetName(const char *name) {
  Attach();
  void *lp = GetLocalPlayer();
  if (!IsValid(lp) || !gameAssembly || !il2cpp_string_new || !name)
    return;
  __try {
    void *nameStr = il2cpp_string_new(name);
    EnsurePlayerControlMethods();
    auto rpcSetName = (RpcSetName_fn)(gameAssembly + g_rvaRpcSetName);
    if (pc_setName_method) {
      void *p[1] = {nameStr};
      il2cpp_runtime_invoke(pc_setName_method, lp, p, nullptr);
    }
    rpcSetName(lp, nameStr, nullptr);
  } __except (EXCEPTION_EXECUTE_HANDLER) {
  }
}

void SetKillCooldown(float time) {
  Attach();
  void *lp = GetLocalPlayer();
  if (IsValid(lp)) {
    EnsurePlayerControlMethods();
    if (pc_setKillTimer_method) {
      void *p[1] = {&time};
      il2cpp_runtime_invoke(pc_setKillTimer_method, lp, p, nullptr);
    } else {
      *(float *)((uintptr_t)lp + 0x80) = time;
    }
  }
  if (!GameOptionsManager::klass)
    return;
  void *field = il2cpp_class_get_field_from_name(GameOptionsManager::klass,
                                                 "<Instance>k__BackingField");
  void *inst = nullptr;
  if (field)
    il2cpp_field_static_get_value(field, &inst);
  if (IsValid(inst)) {
    void *opt = *(void **)((uintptr_t)inst + 0x18);
    if (IsValid(opt))
      *(float *)((uintptr_t)opt + 0x24) = time;
  }
}

void SetKillDistance(float dist) {
  Attach();
  if (!GameOptionsManager::klass)
    return;
  int killDistance = 1; // 0=Short, 1=Medium, 2=Long
  if (dist <= 2.0f)
    killDistance = 0;
  else if (dist >= 4.0f)
    killDistance = 2;
  else
    killDistance = 1;

  void *field = il2cpp_class_get_field_from_name(GameOptionsManager::klass,
                                                 "<Instance>k__BackingField");
  void *inst = nullptr;
  if (field)
    il2cpp_field_static_get_value(field, &inst);
  if (IsValid(inst)) {
    void *opt = *(void **)((uintptr_t)inst + 0x18);
    if (IsValid(opt))
      *(int *)((uintptr_t)opt + 0x44) = killDistance;
  }
}

void SetWallhack(bool enabled) {
  // Wallhack gives impostor-level vision (wider sight in dark areas) rather
  // than the extreme fullbright (which removes all shadows). This makes
  // wallhack more subtle and less detectable than fullbright.
  if (!GameOptionsManager::klass)
    return;
  void *field = il2cpp_class_get_field_from_name(GameOptionsManager::klass,
                                                 "<Instance>k__BackingField");
  void *inst = nullptr;
  if (field)
    il2cpp_field_static_get_value(field, &inst);
  if (IsValid(inst)) {
    void *opt = *(void **)((uintptr_t)inst + 0x18);
    if (IsValid(opt)) {
      *(float *)((uintptr_t)opt + 0x1C) =
          enabled ? 1.5f : 1.0f; // CrewLightMod (impostor-like)
      *(float *)((uintptr_t)opt + 0x20) =
          enabled ? 1.5f : 1.0f; // ImpostorLightMod
    }
  }
}

void SetHat(int hatId) {
  Attach();
  void *lp = GetLocalPlayer();
  if (!IsValid(lp) || !gameAssembly || !il2cpp_string_new)
    return;
  const char *hatIds[] = {"hat_NoHat", "hat_crown", "hat_tophat", "hat_beanie",
                          "hat_horns"};
  if (hatId < 0 || hatId > 4)
    return;
  __try {
    auto fn = (RpcSetHat_fn)(gameAssembly + g_rvaRpcSetHat);
    fn(lp, il2cpp_string_new(hatIds[hatId]), nullptr);
  } __except (EXCEPTION_EXECUTE_HANDLER) {
  }
}

void SetPet(int petId) {
  Attach();
  void *lp = GetLocalPlayer();
  if (!IsValid(lp) || !gameAssembly || !il2cpp_string_new)
    return;
  const char *petIds[] = {"pet_EmptyPet", "pet_Crewmate", "pet_Dog", "pet_Cat",
                          "pet_Robot"};
  if (petId < 0 || petId > 4)
    return;
  __try {
    auto fn = (RpcSetPet_fn)(gameAssembly + g_rvaRpcSetPet);
    fn(lp, il2cpp_string_new(petIds[petId]), nullptr);
  } __except (EXCEPTION_EXECUTE_HANDLER) {
  }
}

void SetSkin(int skinId) {
  Attach();
  void *lp = GetLocalPlayer();
  if (!IsValid(lp) || !gameAssembly || !il2cpp_string_new)
    return;
  const char *skinIds[] = {"skin_None", "skin_Suit", "skin_Astronaut",
                           "skin_Military", "skin_Mech"};
  if (skinId < 0 || skinId > 4)
    return;
  __try {
    auto fn = (RpcSetSkin_fn)(gameAssembly + g_rvaRpcSetSkin);
    fn(lp, il2cpp_string_new(skinIds[skinId]), nullptr);
  } __except (EXCEPTION_EXECUTE_HANDLER) {
  }
}

void SetCharacterScale(float scale) {
  Attach();
  void *lp = GetLocalPlayer();
  if (!IsValid(lp))
    return;

  // Get the player's Transform via Component.get_transform, then set localScale.
  // Writing to arbitrary PlayerControl offsets (0xA0-0xA8) doesn't set scale.
  static void *get_transform_method = nullptr;
  static void *set_localScale_method = nullptr;
  if (!get_transform_method) {
    // Component.get_transform is inherited by PlayerControl
    void *klass = *(void **)lp; // klass pointer
    if (IsValid(klass))
      get_transform_method =
          il2cpp_class_get_method_from_name(klass, "get_transform", 0);
  }
  if (!get_transform_method)
    return;

  __try {
    void *transform =
        il2cpp_runtime_invoke(get_transform_method, lp, nullptr, nullptr);
    if (!IsValid(transform))
      return;

    if (!set_localScale_method && Transform::klass)
      set_localScale_method = il2cpp_class_get_method_from_name(
          Transform::klass, "set_localScale", 1);
    if (!set_localScale_method)
      return;

    // Vector3 struct passed by pointer for il2cpp_runtime_invoke
    struct Vec3 {
      float x, y, z;
    };
    Vec3 newScale = {scale, scale, 1.0f};
    void *p[1] = {&newScale};
    il2cpp_runtime_invoke(set_localScale_method, transform, p, nullptr);
  } __except (EXCEPTION_EXECUTE_HANDLER) {
  }
}

void PlayAnimation(uint8_t animId) {
  Attach();
  void *lp = GetLocalPlayer();
  if (!IsValid(lp) || !gameAssembly)
    return;
  __try {
    auto fn = (RpcPlayAnimation_fn)(gameAssembly + g_rvaRpcPlayAnimation);
    fn(lp, animId, nullptr);
  } __except (EXCEPTION_EXECUTE_HANDLER) {
  }
}

// Camera state for ESP (updated in UpdateInternal)
static float camX = 0, camY = 0, orthoSize = 3.0f;

static void UpdateCameraState() {
  if (!gameAssembly)
    return;
  // Camera.get_main() — resolved from dump.cs when available.
  typedef void *(__cdecl * GetMainCamera_fn)(void *);
  auto getMain = (GetMainCamera_fn)(gameAssembly + g_rvaCameraGetMain);
  void *cam = getMain(nullptr);
  if (!IsValid(cam))
    return;

  // Camera.get_orthographicSize() — resolved from dump.cs when available.
  typedef float(__cdecl * GetOrthoSize_fn)(void *, void *);
  auto getOrtho = (GetOrthoSize_fn)(gameAssembly + g_rvaCameraGetOrthoSize);
  float os = getOrtho(cam, nullptr);
  if (os > 0.1f && os < 100.f)
    orthoSize = os;

  // Get camera transform via il2cpp_runtime_invoke
  if (Transform::klass) {
    static void *get_transform_method = nullptr;
    if (!get_transform_method) {
      // Component.get_transform is inherited — resolve from Camera's klass
      void *camKlass = *(void **)cam; // first field is klass pointer
      if (IsValid(camKlass))
        get_transform_method =
            il2cpp_class_get_method_from_name(camKlass, "get_transform", 0);
    }
    if (get_transform_method) {
      void *camTransform =
          il2cpp_runtime_invoke(get_transform_method, cam, nullptr, nullptr);
      if (IsValid(camTransform)) {
        static void *get_position_method = nullptr;
        if (!get_position_method)
          get_position_method = il2cpp_class_get_method_from_name(
              Transform::klass, "get_position", 0);
        if (get_position_method) {
          // get_position returns a boxed Vector3 — we need the unboxed values
          void *posBox = il2cpp_runtime_invoke(get_position_method,
                                               camTransform, nullptr, nullptr);
          if (IsValid(posBox)) {
            // Boxed value type: klass, monitor, then the value data (Vector3).
            // Skip two pointer-sized fields so this works on both x86 and x64.
            float *xyz = (float *)((uintptr_t)posBox + sizeof(void *) * 2);
            camX = xyz[0];
            camY = xyz[1];
          }
        }
      }
    }
  }
}

// World-to-screen for orthographic 2D camera
static ImVec2 WorldToScreen(float wx, float wy) {
  float screenW = ImGui::GetIO().DisplaySize.x;
  float screenH = ImGui::GetIO().DisplaySize.y;
  float ppu = screenH / (2.f * orthoSize); // pixels per world unit
  float sx = (wx - camX) * ppu + screenW / 2.f;
  float sy =
      screenH / 2.f - (wy - camY) * ppu; // Y is flipped (screen Y goes down)
  return {sx, sy};
}

void DrawESP(ImDrawList *drawList) {
  if (!isInGame || players.empty())
    return;

  float screenW = ImGui::GetIO().DisplaySize.x;
  float screenH = ImGui::GetIO().DisplaySize.y;
  float ppu = screenH / (2.f * orthoSize);
  // Box size scaled to camera zoom
  float boxW = 0.5f * ppu; // ~0.5 world units wide
  float boxH = 0.8f * ppu; // ~0.8 world units tall

  for (const auto &p : players) {
    if (!p.hasWorldPos)
      continue;
    ImVec2 sp = WorldToScreen(p.x, p.y);
    // Cull offscreen
    if (sp.x < -200 || sp.x > screenW + 200 || sp.y < -200 ||
        sp.y > screenH + 200)
      continue;

    ImU32 col =
        p.isImpostor ? IM_COL32(255, 30, 30, 255) : IM_COL32(0, 220, 255, 255);
    if (p.isDead)
      col = IM_COL32(150, 150, 150, 200);

    if (g_espBox) {
      // Shadow
      drawList->AddRect({sp.x - boxW - 1, sp.y - boxH - 1},
                        {sp.x + boxW + 1, sp.y + boxH * 0.1f + 1},
                        IM_COL32(0, 0, 0, 150), 3.f, 0, 1.5f);
      // Main box
      drawList->AddRect({sp.x - boxW, sp.y - boxH},
                        {sp.x + boxW, sp.y + boxH * 0.1f}, col, 3.f, 0, 2.0f);
    }

    float textY = sp.y - boxH - 16;
    if (g_espName) {
      std::string dn = p.name;
      if (p.isDead)
        dn += " [DEAD]";
      ImVec2 sz = ImGui::CalcTextSize(dn.c_str());
      drawList->AddText({sp.x - sz.x / 2 + 1, textY + 1},
                        IM_COL32(0, 0, 0, 180), dn.c_str());
      drawList->AddText({sp.x - sz.x / 2, textY}, col, dn.c_str());
      textY -= 14;
    }
    if (g_espDist && p.distance >= 0.f) {
      char dbuf[16];
      snprintf(dbuf, sizeof(dbuf), "%.1fm", p.distance);
      ImVec2 sz = ImGui::CalcTextSize(dbuf);
      drawList->AddText({sp.x - sz.x / 2, sp.y + boxH * 0.1f + 4},
                        IM_COL32(200, 200, 200, 200), dbuf);
    }
    if (g_espRole) {
      ImVec2 sz = ImGui::CalcTextSize(p.roleName.c_str());
      ImU32 rc = p.isImpostor ? IM_COL32(255, 80, 80, 255)
                              : IM_COL32(100, 255, 100, 255);
      drawList->AddText({sp.x - sz.x / 2, sp.y + boxH * 0.1f + 18}, rc,
                        p.roleName.c_str());
    }
    if (g_espTask) {
      const char *taskTag = p.isDead ? "X" : "T";
      ImU32 tc =
          p.isDead ? IM_COL32(140, 140, 140, 220) : IM_COL32(255, 220, 90, 230);
      drawList->AddCircleFilled({sp.x + boxW + 9.f, sp.y - boxH + 7.f}, 6.f, tc,
                                14);
      ImVec2 ts = ImGui::CalcTextSize(taskTag);
      drawList->AddText({sp.x + boxW + 9.f - ts.x * 0.5f, sp.y - boxH + 2.f},
                        IM_COL32(15, 15, 15, 230), taskTag);
    }
    if (g_particle) {
      float t = (float)ImGui::GetTime();
      for (int k = 0; k < 5; k++) {
        float a = t * 2.2f + k * (6.283185f / 5.f);
        float r = 9.f + 3.f * sinf(t * 3.7f + k);
        ImVec2 pp = {sp.x + cosf(a) * r, sp.y - boxH * 0.45f + sinf(a) * r};
        drawList->AddCircleFilled(pp, 1.6f, IM_COL32(135, 235, 255, 170), 8);
      }
    }
    if (g_espTracer) {
      ImVec2 bot = {screenW / 2.f, screenH};
      drawList->AddLine(bot, {sp.x, sp.y}, (col & 0x00FFFFFF) | 0x80000000,
                        1.2f);
    }
  }
}

void SpamChat(const char *text) {
  Attach();
  void *lp = GetLocalPlayer();
  if (!IsValid(lp) || !gameAssembly || !il2cpp_string_new || !text || !text[0])
    return;
  __try {
    auto fn = (RpcSendChat_fn)(gameAssembly + g_rvaRpcSendChat);
    void *str = il2cpp_string_new(text);
    fn(lp, str, nullptr); // returns bool but we ignore it
  } __except (EXCEPTION_EXECUTE_HANDLER) {
  }
}

void EndGame() {
  Attach();
  if (!gameAssembly || !AmongUsClient::klass)
    return;
  void *field =
      il2cpp_class_get_field_from_name(AmongUsClient::klass, "Instance");
  void *inst = nullptr;
  if (field)
    il2cpp_field_static_get_value(field, &inst);
  if (!IsValid(inst))
    return;
  __try {
    // AmongUsClient.StartGame RVA: 0x5487F0 — triggers end sequence
    // Actually use ExitGame: RVA 0x546850
    typedef void(__cdecl * ExitGame_fn)(void *, int, void *);
    auto fn = (ExitGame_fn)(gameAssembly + g_rvaAmongUsClientExitGame);
    fn(inst, 0, nullptr); // reason = 0
  } __except (EXCEPTION_EXECUTE_HANDLER) {
  }
}

void StartGame() {
  Attach();
  if (!gameAssembly || !AmongUsClient::klass)
    return;
  // MalumMenu: requires isHost && isLobby to force-start
  if (!IsHost() || !isInLobby)
    return;
  void *field =
      il2cpp_class_get_field_from_name(AmongUsClient::klass, "Instance");
  void *inst = nullptr;
  if (field)
    il2cpp_field_static_get_value(field, &inst);
  if (!IsValid(inst))
    return;
  __try {
    // MalumMenu uses SendStartGame() not StartGame()
    auto fn = (StartGame_fn)(gameAssembly + g_rvaAmongUsClientSendStartGame);
    fn(inst, nullptr);
  } __except (EXCEPTION_EXECUTE_HANDLER) {
  }
}

void TeleportToRoom(int roomId) {
  // Skeld room coords
  const float coords[][2] = {
      {-1.f, -1.f},    // 0: Cafeteria
      {-17.f, -5.f},   // 1: Reactor
      {6.5f, -3.5f},   // 2: Navigation
      {-8.8f, -3.f},   // 3: Medbay
      {-2.f, -16.f},   // 4: Electrical
      {3.5f, -12.f},   // 5: Storage
      {9.4f, 2.8f},    // 6: Weapons
      {-20.5f, -5.5f}, // 7: Upper Engine
      {-20.5f, -12.f}, // 8: Lower Engine
  };
  if (roomId >= 0 && roomId < 9)
    TeleportTo(coords[roomId][0], coords[roomId][1]);
}

// ── Helper: get a PlayerControl* by index from AllPlayerControls ──
static void *GetPlayerByIndex(int idx) {
  if (!PlayerControl::klass)
    return nullptr;
  void *field = il2cpp_class_get_field_from_name(PlayerControl::klass,
                                                 "AllPlayerControls");
  void *list = nullptr;
  if (field)
    il2cpp_field_static_get_value(field, &list);
  if (!IsValid(list))
    return nullptr;
  struct L {
    void *k;
    void *m;
    void *items;
    int size;
  };
  struct A {
    void *k;
    void *m;
    void *b;
    int len;
    void *m_Items[1];
  };
  L *l = (L *)list;
  if (!IsValid(l->items) || idx < 0 || idx >= l->size)
    return nullptr;
  A *a = (A *)l->items;
  return (idx < a->len && IsValid(a->m_Items[idx])) ? a->m_Items[idx] : nullptr;
}

static void *GetPlayerByPlayerId(int playerId) {
  if (!PlayerControl::klass || playerId < 0 || playerId > 255)
    return nullptr;
  void *field = il2cpp_class_get_field_from_name(PlayerControl::klass,
                                                 "AllPlayerControls");
  void *list = nullptr;
  if (field)
    il2cpp_field_static_get_value(field, &list);
  if (!IsValid(list))
    return nullptr;
  struct L {
    void *k;
    void *m;
    void *items;
    int size;
  };
  struct A {
    void *k;
    void *m;
    void *b;
    int len;
    void *m_Items[1];
  };
  L *l = (L *)list;
  if (!IsValid(l->items) || l->size <= 0)
    return nullptr;
  A *a = (A *)l->items;
  int maxN = (l->size < a->len) ? l->size : a->len;
  for (int i = 0; i < maxN; i++) {
    void *p = a->m_Items[i];
    if (!IsValid(p))
      continue;
    if (*(uint8_t *)((uintptr_t)p + 0x28) == (uint8_t)playerId)
      return p;
  }
  return nullptr;
}

static void *ResolveTargetPlayer(int playerSelector) {
  void *target = GetPlayerByPlayerId(playerSelector);
  if (IsValid(target))
    return target;
  target = GetPlayerByIndex(playerSelector);
  if (IsValid(target))
    return target;
  return GetPlayerByIndex(playerSelector - 1);
}

// RoleManager.SetRole(PlayerControl, RoleTypes) — RVA: 0x60FA50
// Direct call: works regardless of host status, bypasses server authority.
typedef void(__cdecl *RoleManager_SetRole_fn)(void *, void *, uint16_t, void *);

// Get RoleManager.Instance via get_Instance() method or _instance static field
static void *GetRoleManager() {
  if (!RoleManager::klass)
    return nullptr;
  // Try get_Instance method first (works reliably on DestroyableSingleton<T>)
  static void *getInstance_method = nullptr;
  static bool methodResolved = false;
  if (!methodResolved) {
    getInstance_method =
        il2cpp_class_get_method_from_name(RoleManager::klass, "get_Instance", 0);
    methodResolved = true;
  }
  if (getInstance_method) {
    __try {
      void *inst =
          il2cpp_runtime_invoke(getInstance_method, nullptr, nullptr, nullptr);
      if (IsValid(inst))
        return inst;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
    }
  }
  // Fallback: try _instance static field directly
  void *field =
      il2cpp_class_get_field_from_name(RoleManager::klass, "_instance");
  if (!field)
    return nullptr;
  void *inst = nullptr;
  il2cpp_field_static_get_value(field, &inst);
  return IsValid(inst) ? inst : nullptr;
}

static bool IsGhostRoleType(uint16_t roleType) {
  return roleType == 6 || roleType == 7;
}

static uint16_t ResolveAliveRoleType(void *cachedPlayerData) {
  if (!IsValid(cachedPlayerData))
    return 0;

  uint16_t currentRole = *(uint16_t *)((uintptr_t)cachedPlayerData + 0x38);
  bool hasRoleWhenAlive = *(bool *)((uintptr_t)cachedPlayerData + 0x3C);
  uint16_t roleWhenAlive =
      hasRoleWhenAlive ? *(uint16_t *)((uintptr_t)cachedPlayerData + 0x3A) : 0;

  if (hasRoleWhenAlive && !IsGhostRoleType(roleWhenAlive))
    return roleWhenAlive;
  if (IsGhostRoleType(currentRole))
    return (currentRole == 7) ? 1 : 0;
  return currentRole;
}

// Apply role both client-side (memory) and server-side (RoleManager + Rpc)
static void ApplyRoleToPlayer(void *targetPc, int roleType) {
  if (!IsValid(targetPc) || !gameAssembly)
    return;

  // 1. Client-side: write roleType directly into CachedPlayerData.Role (offset
  // 0x38)
  void *data = *(void **)((uintptr_t)targetPc + 0x58); // CachedPlayerData
  if (IsValid(data))
    *(uint16_t *)((uintptr_t)data + 0x38) = (uint16_t)roleType;

  // 2. Server-side: RoleManager.SetRole(playerControl, roleType)
  //    This is the INTERNAL game method — no host check, applies the actual
  //    RoleBehaviour component and syncs state locally.
  void *rm = GetRoleManager();
  if (IsValid(rm)) {
    __try {
      auto fn =
          (RoleManager_SetRole_fn)(gameAssembly + g_rvaRoleManagerSetRole);
      fn(rm, targetPc, (uint16_t)roleType, nullptr);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
    }
  }

  // 3. Network: RpcSetRole broadcasts to all clients
  //    RVA: 0x5C99C0  signature: void(PlayerControl*, RoleTypes, bool,
  //    MethodInfo*)
  __try {
    auto fn = (RpcSetRole_fn)(gameAssembly + g_rvaRpcSetRole);
    fn(targetPc, (uint16_t)roleType, true, nullptr);
  } __except (EXCEPTION_EXECUTE_HANDLER) {
  }
}

static void RevivePlayerControl(void *playerControl, bool syncRole) {
  if (!IsValid(playerControl))
    return;

  void *data = *(void **)((uintptr_t)playerControl + 0x58); // CachedPlayerData
  if (!IsValid(data))
    return;

  __try {
    uint16_t currentRole = *(uint16_t *)((uintptr_t)data + 0x38);
    uint16_t aliveRole = ResolveAliveRoleType(data);

    // Clear death/ejection flags first so the revive call is not ignored.
    *(bool *)((uintptr_t)data + 0x54) = false; // IsDead
    *(bool *)((uintptr_t)data + 0x55) = false; // WasEjected

    // Recover locomotion flags that can stay stale when reviving mid-round.
    *(bool *)((uintptr_t)playerControl + 0x38) = true;  // moveable
    *(bool *)((uintptr_t)playerControl + 0x48) = false; // inVent

    if (aliveRole != currentRole) {
      if (syncRole)
        ApplyRoleToPlayer(playerControl, (int)aliveRole);
      else
        *(uint16_t *)((uintptr_t)data + 0x38) = aliveRole;
    }

    EnsurePlayerControlMethods();
    if (pc_revive_method)
      il2cpp_runtime_invoke(pc_revive_method, playerControl, nullptr, nullptr);

    // Re-apply once after Revive in case game logic writes dead state back.
    *(bool *)((uintptr_t)data + 0x54) = false;
    *(bool *)((uintptr_t)data + 0x55) = false;
    *(bool *)((uintptr_t)playerControl + 0x38) = true;
    *(bool *)((uintptr_t)playerControl + 0x48) = false;
  } __except (EXCEPTION_EXECUTE_HANDLER) {
  }
}

void SetRole(int roleType) {
  Attach();
  void *lp = GetLocalPlayer();
  if (!IsValid(lp))
    return;
  ApplyRoleToPlayer(lp, roleType);
  isImpostor =
      (roleType == 1 || roleType == 5 || roleType == 7 || roleType == 9 || roleType == 18);
}

void SetPlayerRole(int playerIndex, int roleType) {
  Attach();
  void *target = ResolveTargetPlayer(playerIndex);
  if (!IsValid(target))
    return;
  ApplyRoleToPlayer(target, roleType);
}

// MurderPlayer RVA: 0x5C5D70  (direct kill, no role check)
// RpcMurderPlayer RVA: 0x5C8CC0 (network sync)
typedef void(__cdecl *MurderPlayer_fn)(void *, void *, uint32_t, void *);
typedef void(__cdecl *RpcMurderPlayer_fn)(void *, void *, bool, void *);

void KillPlayer(int playerIndex) {
  Attach();
  void *lp = GetLocalPlayer();
  void *target = ResolveTargetPlayer(playerIndex);
  if (!IsValid(lp) || !IsValid(target) || !gameAssembly)
    return;
  __try {
    auto fn = (MurderPlayer_fn)(gameAssembly + g_rvaMurderPlayer);
    fn(lp, target, 1, nullptr); // MurderResultFlags::Succeeded
    auto rpc = (RpcMurderPlayer_fn)(gameAssembly + g_rvaRpcMurderPlayer);
    rpc(lp, target, true, nullptr);
  } __except (EXCEPTION_EXECUTE_HANDLER) {
  }
}

void KillAllPlayers() {
  Attach();
  void *lp = GetLocalPlayer();
  if (!IsValid(lp) || !gameAssembly)
    return;
  auto murder = (MurderPlayer_fn)(gameAssembly + g_rvaMurderPlayer);
  auto rpcMurder = (RpcMurderPlayer_fn)(gameAssembly + g_rvaRpcMurderPlayer);
  void *field = il2cpp_class_get_field_from_name(PlayerControl::klass,
                                                 "AllPlayerControls");
  void *list = nullptr;
  if (field)
    il2cpp_field_static_get_value(field, &list);
  if (!IsValid(list))
    return;
  struct L {
    void *k;
    void *m;
    void *items;
    int size;
  };
  struct A {
    void *k;
    void *m;
    void *b;
    int len;
    void *m_Items[1];
  };
  L *l = (L *)list;
  if (!IsValid(l->items))
    return;
  A *a = (A *)l->items;
  __try {
    for (int i = 0; i < l->size && i < a->len; i++) {
      void *p = a->m_Items[i];
      if (IsValid(p) && p != lp) {
        murder(lp, p, 1, nullptr);
        rpcMurder(lp, p, true, nullptr);
        Sleep(50);
      }
    }
  } __except (EXCEPTION_EXECUTE_HANDLER) {
  }
}

// CmdReportDeadBody RVA: 0x5C2150

void ReportBody(int playerIndex) {
  Attach();
  void *lp = GetLocalPlayer();
  void *target = ResolveTargetPlayer(playerIndex);
  if (!IsValid(lp) || !gameAssembly)
    return;
  void *data = nullptr;
  if (IsValid(target))
    data = *(void **)((uintptr_t)target + 0x58); // CachedPlayerData
  __try {
    auto fn = (CmdReportDeadBody_fn)(gameAssembly + g_rvaCmdReportDeadBody);
    fn(lp, data, nullptr); // null = self report / emergency
  } __except (EXCEPTION_EXECUTE_HANDLER) {
  }
}

// RpcSetVisor RVA: 0x5C9D90

void SetVisor(int visorId) {
  Attach();
  void *lp = GetLocalPlayer();
  if (!IsValid(lp) || !gameAssembly || !il2cpp_string_new)
    return;
  const char *ids[] = {"visor_EmptyVisor", "visor_lollipopCrew",
                       "visor_lollipopImp", "visor_starCrew",
                       "visor_pk01_AngeryVisor"};
  if (visorId < 0 || visorId > 4)
    return;
  __try {
    auto fn = (RpcSetVisor_fn)(gameAssembly + g_rvaRpcSetVisor);
    fn(lp, il2cpp_string_new(ids[visorId]), nullptr);
  } __except (EXCEPTION_EXECUTE_HANDLER) {
  }
}

// RpcSetNamePlate RVA: 0x5C96C0

void SetNamePlate(int npId) {
  Attach();
  void *lp = GetLocalPlayer();
  if (!IsValid(lp) || !gameAssembly || !il2cpp_string_new)
    return;
  const char *ids[] = {"nameplate_NoPlate", "nameplate_airship_Toppat",
                       "nameplate_airship_CCC", "nameplate_airship_Government",
                       "nameplate_is_yard"};
  if (npId < 0 || npId > 4)
    return;
  __try {
    auto fn = (RpcSetNamePlate_fn)(gameAssembly + g_rvaRpcSetNamePlate);
    fn(lp, il2cpp_string_new(ids[npId]), nullptr);
  } __except (EXCEPTION_EXECUTE_HANDLER) {
  }
}

// RpcSetLevel RVA: 0x5C9620

void SetLevel(int level) {
  Attach();
  void *lp = GetLocalPlayer();
  if (!IsValid(lp) || !gameAssembly)
    return;
  __try {
    // Among Us stores level internally as (displayLevel - 1).
    // The game displays stats.level + 1. To show level N, write N-1.
    uint32_t storedLevel = (level > 0) ? (uint32_t)(level - 1) : 0;

    // Write to NetworkedPlayerInfo.PlayerLevel field (offset 0x44)
    void *data = *(void **)((uintptr_t)lp + 0x58);
    if (IsValid(data))
      *(uint32_t *)((uintptr_t)data + 0x44) = storedLevel;
    EnsurePlayerControlMethods();
    auto rpcSetLevel = (RpcSetLevel_fn)(gameAssembly + g_rvaRpcSetLevel);
    if (pc_setLevel_method) {
      void *p[1] = {&storedLevel};
      il2cpp_runtime_invoke(pc_setLevel_method, lp, p, nullptr);
    }
    rpcSetLevel(lp, storedLevel, nullptr);
  } __except (EXCEPTION_EXECUTE_HANDLER) {
  }
}

void RevivePlayer() {
  Attach();
  void *lp = GetLocalPlayer();
  if (!IsValid(lp))
    return;

  RevivePlayerControl(lp, true);

  void *data = *(void **)((uintptr_t)lp + 0x58);
  if (IsValid(data)) {
    uint16_t role = *(uint16_t *)((uintptr_t)data + 0x38);
    isImpostor = (role == 1 || role == 5 || role == 7 || role == 9 || role == 18);
  }
}

// RpcShapeshift RVA: 0x5C9ED0

void ShapeshiftTo(int playerIndex) {
  Attach();
  void *lp = GetLocalPlayer();
  void *target = ResolveTargetPlayer(playerIndex);
  if (!IsValid(lp) || !IsValid(target) || !gameAssembly)
    return;
  __try {
    auto fn = (RpcShapeshift_fn)(gameAssembly + g_rvaRpcShapeshift);
    fn(lp, target, true, nullptr);
  } __except (EXCEPTION_EXECUTE_HANDLER) {
  }
}

// RpcVanish RVA: 0x5CA3E0

void Vanish() {
  Attach();
  void *lp = GetLocalPlayer();
  if (!IsValid(lp) || !gameAssembly)
    return;
  __try {
    auto fn = (RpcVanish_fn)(gameAssembly + g_rvaRpcVanish);
    fn(lp, nullptr);
  } __except (EXCEPTION_EXECUTE_HANDLER) {
  }
}

// RpcAppear RVA: 0x5C8BA0

void Appear() {
  Attach();
  void *lp = GetLocalPlayer();
  if (!IsValid(lp) || !gameAssembly)
    return;
  __try {
    auto fn = (RpcAppear_fn)(gameAssembly + g_rvaRpcAppear);
    fn(lp, true, nullptr);
  } __except (EXCEPTION_EXECUTE_HANDLER) {
  }
}

// Vent RPCs: Enter 0x5E3250, Exit 0x5E3340

void EnterVent(int ventId) {
  Attach();
  void *lp = GetLocalPlayer();
  if (!IsValid(lp) || !gameAssembly)
    return;
  void *phys = *(void **)((uintptr_t)lp + 0x94); // MyPhysics (PlayerPhysics)
  if (!IsValid(phys))
    return;
  __try {
    auto fn = (RpcVent_fn)(gameAssembly + g_rvaRpcEnterVent);
    fn(phys, ventId, nullptr);
  } __except (EXCEPTION_EXECUTE_HANDLER) {
  }
}

void ExitVent(int ventId) {
  Attach();
  void *lp = GetLocalPlayer();
  if (!IsValid(lp) || !gameAssembly)
    return;
  void *phys = *(void **)((uintptr_t)lp + 0x94);
  if (!IsValid(phys))
    return;
  __try {
    auto fn = (RpcVent_fn)(gameAssembly + g_rvaRpcExitVent);
    fn(phys, ventId, nullptr);
  } __except (EXCEPTION_EXECUTE_HANDLER) {
  }
}

// RpcCloseDoorsOfType RVA: 0x637E00

void CloseDoors(int roomType) {
  Attach();
  if (!ShipStatus::klass || !gameAssembly)
    return;
  void *field = il2cpp_class_get_field_from_name(ShipStatus::klass, "Instance");
  void *inst = nullptr;
  if (field)
    il2cpp_field_static_get_value(field, &inst);
  if (!IsValid(inst))
    return;
  __try {
    auto fn = (RpcCloseDoors_fn)(gameAssembly + g_rvaRpcCloseDoorsOfType);
    fn(inst, roomType, nullptr);
  } __except (EXCEPTION_EXECUTE_HANDLER) {
  }
}

// RpcUpdateSystem RVA: 0x637EB0

void RepairSabotage(int systemType) {
  Attach();
  if (!ShipStatus::klass || !gameAssembly)
    return;
  void *field = il2cpp_class_get_field_from_name(ShipStatus::klass, "Instance");
  void *inst = nullptr;
  if (field)
    il2cpp_field_static_get_value(field, &inst);
  if (!IsValid(inst))
    return;
  __try {
    // amount = 128 fixes most sabotages
    auto fn = (RpcUpdateSystem_fn)(gameAssembly + g_rvaRpcUpdateSystem);
    fn(inst, systemType, 128, nullptr);
  } __except (EXCEPTION_EXECUTE_HANDLER) {
  }
}

void TeleportToPlayer(int playerIndex) {
  Attach();
  void *target = ResolveTargetPlayer(playerIndex);
  if (!IsValid(target))
    return;
  void *nt = *(void **)((uintptr_t)target + 0x98);
  if (!IsValid(nt))
    return;
  float x = *(float *)((uintptr_t)nt + 0x44);
  float y = *(float *)((uintptr_t)nt + 0x48);
  TeleportTo(x, y);
}

// RpcProtectPlayer RVA: 0x5C8E70

void ProtectPlayer(int playerIndex) {
  Attach();
  void *lp = GetLocalPlayer();
  void *target = ResolveTargetPlayer(playerIndex);
  if (!IsValid(lp) || !IsValid(target) || !gameAssembly)
    return;
  __try {
    auto fn = (RpcProtectPlayer_fn)(gameAssembly + g_rvaRpcProtectPlayer);
    fn(lp, target, 0, nullptr);
  } __except (EXCEPTION_EXECUTE_HANDLER) {
  }
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

void SendChat(const char *msg) {
  Attach();
  if (!msg || !msg[0])
    return; // reject empty messages
  void *lp = GetLocalPlayer();
  if (!IsValid(lp) || !gameAssembly || !il2cpp_string_new)
    return;
  __try {
    auto fn = (RpcSendChat_fn)(gameAssembly + g_rvaRpcSendChat);
    fn(lp, il2cpp_string_new(msg), nullptr);
  } __except (EXCEPTION_EXECUTE_HANDLER) {
  }
}

void TeleportAllToSelf() {
  Attach();
  if (!IsHost())
    return; // only host can teleport other players
  void *lp = GetLocalPlayer();
  if (!IsValid(lp) || !gameAssembly)
    return;
  void *myNt = *(void **)((uintptr_t)lp + 0x98);
  if (!IsValid(myNt))
    return;
  float mx = *(float *)((uintptr_t)myNt + 0x44);
  float my = *(float *)((uintptr_t)myNt + 0x48);

  if (!PlayerControl::klass)
    return;
  void *field = il2cpp_class_get_field_from_name(PlayerControl::klass,
                                                 "AllPlayerControls");
  void *list = nullptr;
  if (field)
    il2cpp_field_static_get_value(field, &list);
  if (!IsValid(list))
    return;
  struct L {
    void *k;
    void *m;
    void *items;
    int size;
  };
  struct A {
    void *k;
    void *m;
    void *b;
    int len;
    void *m_Items[1];
  };
  L *l = (L *)list;
  if (!IsValid(l->items))
    return;
  A *a = (A *)l->items;
  auto fn = (RpcSnapTo_fn)(gameAssembly + g_rvaRpcSnapTo);
  __try {
    for (int i = 0; i < l->size && i < a->len; i++) {
      void *p = a->m_Items[i];
      if (IsValid(p) && p != lp) {
        void *nt = *(void **)((uintptr_t)p + 0x98);
        if (IsValid(nt))
          fn(nt, mx, my, nullptr);
      }
    }
  } __except (EXCEPTION_EXECUTE_HANDLER) {
  }
}

void SetDiscussionTime(float time) {
  Attach();
  if (!GameOptionsManager::klass)
    return;
  void *field = il2cpp_class_get_field_from_name(GameOptionsManager::klass,
                                                 "<Instance>k__BackingField");
  void *inst = nullptr;
  if (field)
    il2cpp_field_static_get_value(field, &inst);
  if (!IsValid(inst))
    return;
  void *opt = *(void **)((uintptr_t)inst + 0x18);
  if (IsValid(opt))
    *(int *)((uintptr_t)opt + 0x48) = (int)time; // DiscussionTime
}

void SetVotingTime(float time) {
  Attach();
  if (!GameOptionsManager::klass)
    return;
  void *field = il2cpp_class_get_field_from_name(GameOptionsManager::klass,
                                                 "<Instance>k__BackingField");
  void *inst = nullptr;
  if (field)
    il2cpp_field_static_get_value(field, &inst);
  if (!IsValid(inst))
    return;
  void *opt = *(void **)((uintptr_t)inst + 0x18);
  if (IsValid(opt))
    *(int *)((uintptr_t)opt + 0x4C) = (int)time; // VotingTime
}

void SetEmergencyCount(int count) {
  Attach();
  if (GameOptionsManager::klass) {
    void *field = il2cpp_class_get_field_from_name(GameOptionsManager::klass,
                                                   "<Instance>k__BackingField");
    void *inst = nullptr;
    if (field)
      il2cpp_field_static_get_value(field, &inst);
    if (IsValid(inst)) {
      void *opt = *(void **)((uintptr_t)inst + 0x18);
      if (IsValid(opt))
        *(int *)((uintptr_t)opt + 0x34) = count; // NumEmergencyMeetings
    }
  }
  void *lp = GetLocalPlayer();
  if (IsValid(lp))
    *(int *)((uintptr_t)lp + 0x84) = count; // RemainingEmergencies
}

} // namespace Stara::Game
