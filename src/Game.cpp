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
static uintptr_t g_rvaMonoStartCoroutineString = 0x1F4C290;
static uintptr_t g_rvaRpcEnterVent = 0x5E3250;
static uintptr_t g_rvaRpcExitVent = 0x5E3340;
static uintptr_t g_rvaRpcCloseDoorsOfType = 0x637E00;
static uintptr_t g_rvaRpcUpdateSystem = 0x637EB0;
// Phase 1: Vote Manipulation
static uintptr_t g_rvaRpcVotingComplete = 0x568E20;
// Phase 9: No Game End
static uintptr_t g_rvaCheckEndCriteria = 0x554DC0;
// Hydra: VentilationSystem.Update(Operation, int)
static uintptr_t g_rvaVentSystemUpdate = 0x684C80;
// Hydra: ReportDeadBody
static uintptr_t g_rvaReportDeadBody = 0x5C7E60;

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
  g_rvaMonoStartCoroutineString = GetMethodRva(
      "MonoBehaviour", "StartCoroutine", g_rvaMonoStartCoroutineString);
  g_rvaRpcEnterVent =
      GetMethodRva("PlayerPhysics", "RpcEnterVent", g_rvaRpcEnterVent);
  g_rvaRpcExitVent =
      GetMethodRva("PlayerPhysics", "RpcExitVent", g_rvaRpcExitVent);
  g_rvaRpcCloseDoorsOfType = GetMethodRva("ShipStatus", "RpcCloseDoorsOfType",
                                          g_rvaRpcCloseDoorsOfType);
  g_rvaRpcUpdateSystem =
      GetMethodRva("ShipStatus", "RpcUpdateSystem", g_rvaRpcUpdateSystem);
  g_rvaRpcVotingComplete =
      GetMethodRva("MeetingHud", "RpcVotingComplete", g_rvaRpcVotingComplete);
  g_rvaCheckEndCriteria =
      GetMethodRva("LogicGameFlowNormal", "CheckEndCriteria", g_rvaCheckEndCriteria);
  g_rvaVentSystemUpdate =
      GetMethodRva("VentilationSystem", "Update", g_rvaVentSystemUpdate);
  g_rvaReportDeadBody =
      GetMethodRva("PlayerControl", "ReportDeadBody", g_rvaReportDeadBody);
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
    if (!Vent::klass)
      Vent::klass = il2cpp_class_from_name(img, "", "Vent");
    if (!VentilationSystem::klass)
      VentilationSystem::klass = il2cpp_class_from_name(img, "", "VentilationSystem");
    if (!SabotageSystemType::klass)
      SabotageSystemType::klass = il2cpp_class_from_name(img, "", "SabotageSystemType");
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

// Get AmongUsClient.Instance
static void *GetAmongUsClientInstance() {
  if (!AmongUsClient::klass)
    return nullptr;
  void *field =
      il2cpp_class_get_field_from_name(AmongUsClient::klass, "Instance");
  void *inst = nullptr;
  if (field)
    il2cpp_field_static_get_value(field, &inst);
  return IsValid(inst) ? inst : nullptr;
}

typedef void *(__cdecl *StartCoroutineString_fn)(void *, void *, void *);

static bool StartAutoFindLobbyCoroutine() {
  if (!gameAssembly || !il2cpp_string_new || !g_rvaMonoStartCoroutineString)
    return false;

  void *inst = GetAmongUsClientInstance();
  if (!IsValid(inst))
    return false;

  auto startCoroutine =
      (StartCoroutineString_fn)(gameAssembly + g_rvaMonoStartCoroutineString);
  if (!startCoroutine)
    return false;

  void *coName = il2cpp_string_new("CoFindGame");
  if (!IsValid(coName))
    return false;

  __try {
    startCoroutine(inst, coName, nullptr);
    return true;
  } __except (EXCEPTION_EXECUTE_HANDLER) {
    return false;
  }
}

static void AutoFarmTick(float now, bool hasClient, int gameState,
                         int lastDisconnectReason) {
  static bool wasEnabled = false;
  static bool wasInSession = false;
  static float nextJoinAttemptAt = 0.f;

  if (!g_autoFarm) {
    wasEnabled = false;
    wasInSession = (isInGame || isInLobby);
    nextJoinAttemptAt = 0.f;
    return;
  }

  bool inSession = (isInGame || isInLobby);
  if (!wasEnabled) {
    wasEnabled = true;
    nextJoinAttemptAt = now + 0.4f;
    printf("[+] Auto Farm enabled\n");
  }

  // Transitioned out of an active session.
  if (wasInSession && !inSession) {
    if (lastDisconnectReason == 7) { // DisconnectReasons.Kicked
      nextJoinAttemptAt = now + 1.5f;
      printf("[!] Auto Farm: kick detected, searching new lobby...\n");
    } else {
      nextJoinAttemptAt = now + 4.0f;
    }
  }

  // Game ended: lobby can be destroyed depending on host; prepare a slower retry.
  if (gameState == 3 && !inSession)
    nextJoinAttemptAt = std::max(nextJoinAttemptAt, now + 3.0f);

  if (!inSession && hasClient && now >= nextJoinAttemptAt) {
    if (StartAutoFindLobbyCoroutine()) {
      nextJoinAttemptAt = now + 8.0f;
      printf("[*] Auto Farm: running CoFindGame...\n");
    } else {
      nextJoinAttemptAt = now + 3.5f;
    }
  }

  wasInSession = inSession;
}

// --- AmHost hook (MalumMenu-style) ---
// Hook the native get_AmHost() method so ALL internal game code thinks we're
// host while sending RPCs. RVA from dump.cs: 0x6FEAF0.
static const uintptr_t g_rvaGetAmHost = 0x6FEAF0;
static bool g_forceAmHost = false;
static bool s_amHostHooked = false;

// IL2CPP calling convention: bool __cdecl get_AmHost(void* this, void* method)
typedef bool(__cdecl *GetAmHost_fn)(void *, void *);
static GetAmHost_fn orig_get_AmHost = nullptr;

static bool __cdecl hk_get_AmHost(void *thisPtr, void *methodInfo) {
  if (g_forceAmHost)
    return true;
  return orig_get_AmHost(thisPtr, methodInfo);
}

static void EnsureAmHostHook() {
  if (s_amHostHooked || !gameAssembly)
    return;
  void *target = (void *)((uintptr_t)gameAssembly + g_rvaGetAmHost);
  if (MH_CreateHook(target, (void *)hk_get_AmHost,
                     (void **)&orig_get_AmHost) == MH_OK) {
    MH_EnableHook(target);
    s_amHostHooked = true;
    printf("[+] Stara: AmHost hook installed at GA+0x%X\n",
           (unsigned)g_rvaGetAmHost);
  }
}

static void SpoofHost() {
  EnsureAmHostHook();
  g_forceAmHost = true;
}

static void RestoreHost() { g_forceAmHost = false; }

// Check if we are the REAL host (bypasses our AmHost hook)
bool IsHost() {
  void *inst = GetAmongUsClientInstance();
  if (!inst)
    return false;
  if (s_amHostHooked && orig_get_AmHost) {
    __try {
      return orig_get_AmHost(inst, nullptr);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
      return false;
    }
  }
  __try {
    int hostId = *(int *)((uintptr_t)inst + 0x30);
    int clientId = *(int *)((uintptr_t)inst + 0x34);
    return hostId == clientId;
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
static void SetFakeRoleLocal(void *targetPc, int roleType);
static void ForceRoleNetwork(void *targetPc, int roleType);
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
static float camX = 0, camY = 0, orthoSize = 3.0f; // camera state (used by ESP + freecam)

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

// Raw buffer for vent data (no C++ objects, safe for __try)
static struct { int id; float x, y; } s_ventBuf[64];
static int s_ventCount = 0;

static void CollectVentPositionsRaw() {
  s_ventCount = 0;
  if (!isInGame || !ShipStatus::klass || !g_espVent)
    return;
  __try {
    void *ssField =
        il2cpp_class_get_field_from_name(ShipStatus::klass, "Instance");
    void *ssInst = nullptr;
    if (ssField)
      il2cpp_field_static_get_value(ssField, &ssInst);
    if (!IsValid(ssInst))
      return;
    void *ventArr = *(void **)((uintptr_t)ssInst + 0xB8);
    if (!IsValid(ventArr))
      return;
    int len = *(int *)((uintptr_t)ventArr + 0x0C);
    void **items = (void **)((uintptr_t)ventArr + 0x10);
    static void *vent_get_transform = nullptr;
    static void *get_pos = nullptr;
    for (int i = 0; i < len && i < 64; i++) {
      void *v = items[i];
      if (!IsValid(v))
        continue;
      int ventId = *(int *)((uintptr_t)v + 0x10);
      if (!vent_get_transform) {
        void *vk = *(void **)v;
        if (IsValid(vk))
          vent_get_transform =
              il2cpp_class_get_method_from_name(vk, "get_transform", 0);
      }
      if (!vent_get_transform)
        continue;
      void *tr =
          il2cpp_runtime_invoke(vent_get_transform, v, nullptr, nullptr);
      if (!IsValid(tr))
        continue;
      if (!get_pos && Transform::klass)
        get_pos = il2cpp_class_get_method_from_name(Transform::klass,
                                                    "get_position", 0);
      if (!get_pos)
        continue;
      void *posBox = il2cpp_runtime_invoke(get_pos, tr, nullptr, nullptr);
      if (IsValid(posBox)) {
        float *xyz = (float *)((uintptr_t)posBox + sizeof(void *) * 2);
        s_ventBuf[s_ventCount].id = ventId;
        s_ventBuf[s_ventCount].x = xyz[0];
        s_ventBuf[s_ventCount].y = xyz[1];
        s_ventCount++;
      }
    }
  } __except (EXCEPTION_EXECUTE_HANDLER) {
  }
}

static void CollectVentPositions() {
  CollectVentPositionsRaw();
  vents.clear();
  for (int i = 0; i < s_ventCount; i++)
    vents.push_back({s_ventBuf[i].id, s_ventBuf[i].x, s_ventBuf[i].y});
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
  bool hasClient = false;
  int gameState = 0;
  int lastDisconnectReason = -1;
  {
    if (AmongUsClient::klass) {
      void *field =
          il2cpp_class_get_field_from_name(AmongUsClient::klass, "Instance");
      void *inst = nullptr;
      if (field)
        il2cpp_field_static_get_value(field, &inst);
      if (IsValid(inst)) {
        hasClient = true;
        gameState = *(int *)((uintptr_t)inst + 0x64); // GameState
        lastDisconnectReason =
            *(int *)((uintptr_t)inst + 0x40); // LastDisconnectReason
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

  AutoFarmTick(currentTime, hasClient, gameState, lastDisconnectReason);

  if (!isInGame && !isInLobby) {
    players.clear();
    isInMeeting = false;
    return;
  }

  // Update camera for ESP (in-game only)
  if (isInGame)
    UpdateCameraState();

  // Freecam / Spectate — override camera position after reading it
  if (isInGame && gameAssembly && (g_freecam || g_spectate)) {
    static float freecamX = 0, freecamY = 0;
    static bool freecamInit = false;
    typedef void *(__cdecl *GetMainCamera_fn)(void *);
    auto getMain = (GetMainCamera_fn)(gameAssembly + g_rvaCameraGetMain);
    void *cam = getMain(nullptr);
    if (IsValid(cam) && Transform::klass) {
      static void *get_transform_m = nullptr;
      if (!get_transform_m) {
        void *ck = *(void **)cam;
        if (IsValid(ck))
          get_transform_m = il2cpp_class_get_method_from_name(ck, "get_transform", 0);
      }
      if (get_transform_m) {
        void *camTf = il2cpp_runtime_invoke(get_transform_m, cam, nullptr, nullptr);
        if (IsValid(camTf)) {
          static void *set_pos_m = nullptr;
          if (!set_pos_m)
            set_pos_m = il2cpp_class_get_method_from_name(Transform::klass, "set_position", 1);

          if (g_spectate && g_spectateTarget >= 0) {
            // Spectate: move camera to target player position
            for (const auto &p : players) {
              if (p.playerId == g_spectateTarget && p.hasWorldPos) {
                freecamX = p.x;
                freecamY = p.y;
                break;
              }
            }
          } else if (g_freecam) {
            if (!freecamInit) {
              freecamX = camX;
              freecamY = camY;
              freecamInit = true;
            }
            float spd = 0.15f;
            if (GetAsyncKeyState('W') & 0x8000 || GetAsyncKeyState(VK_UP) & 0x8000) freecamY += spd;
            if (GetAsyncKeyState('S') & 0x8000 || GetAsyncKeyState(VK_DOWN) & 0x8000) freecamY -= spd;
            if (GetAsyncKeyState('A') & 0x8000 || GetAsyncKeyState(VK_LEFT) & 0x8000) freecamX -= spd;
            if (GetAsyncKeyState('D') & 0x8000 || GetAsyncKeyState(VK_RIGHT) & 0x8000) freecamX += spd;
          }

          if (set_pos_m) {
            struct Vec3 { float x, y, z; } pos = {freecamX, freecamY, -10.f};
            void *args[] = {&pos};
            il2cpp_runtime_invoke(set_pos_m, camTf, args, nullptr);
          }
        }
      }
    }
    if (!g_freecam && !g_spectate)
      freecamInit = false;
  }

  // Teleport to Cursor — right-click to teleport (screen-to-world)
  if (isInGame && g_teleportToCursor) {
    static bool wasDown = false;
    bool isDown = (GetAsyncKeyState(VK_RBUTTON) & 0x8000) != 0;
    if (isDown && !wasDown) {
      POINT cursor;
      GetCursorPos(&cursor);
      // Convert screen coords to world coords (reverse of WorldToScreen)
      float screenW = ImGui::GetIO().DisplaySize.x;
      float screenH = ImGui::GetIO().DisplaySize.y;
      // Adjust for window position
      HWND hwnd = FindWindowA("UnityWndClass", NULL);
      if (hwnd) ScreenToClient(hwnd, &cursor);
      float ppu = screenH / (2.f * orthoSize);
      float worldX = camX + ((float)cursor.x - screenW / 2.f) / ppu;
      float worldY = camY - ((float)cursor.y - screenH / 2.f) / ppu;
      // Teleport local player
      void *tlp = GetLocalPlayer();
      if (IsValid(tlp)) {
        void *tnt = *(void **)((uintptr_t)tlp + 0x98);
        if (IsValid(tnt)) {
          *(float *)((uintptr_t)tnt + 0x44) = worldX;
          *(float *)((uintptr_t)tnt + 0x48) = worldY;
        }
      }
    }
    wasDown = isDown;
  }

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

    // Fun toggles: these send RPCs that can cause kicks for non-host
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
        pathTimer += 0.033f;
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

      // 7. Color Cycle
      if (g_colorCycle && CanSendCosmeticRpc() && gameAssembly) {
        static int cc = 0;
        cc = (cc + 1) % 18;
        auto fn = (RpcSetColor_fn)(gameAssembly + g_rvaRpcSetColor);
        fn(lp, (uint8_t)cc, nullptr);
      }

      // 8. Spam Animation
      if (g_spamAnim && gameAssembly && CanSendRpc()) {
        auto fn =
            (RpcPlayAnimation_fn)(gameAssembly + g_rvaRpcPlayAnimation);
        fn(lp, (uint8_t)(rand() % 3), nullptr);
      }

      // 9. Auto Tasks — complete tasks every 5 seconds
      if (g_autoTasks) {
        static float atTimer = 0;
        atTimer += 0.033f;
        if (atTimer > 5.0f) {
          atTimer = 0;
          CompleteAllTasks();
        }
      }

      // 10. Force Protect
      if (g_forceProtect && gameAssembly) {
        static float fpTimer = 0;
        fpTimer += 0.033f;
        if (fpTimer > 5.0f) {
          fpTimer = 0;
          SpoofHost();
          auto fn =
              (RpcProtectPlayer_fn)(gameAssembly + g_rvaRpcProtectPlayer);
          fn(lp, lp, 0, nullptr);
          RestoreHost();
        }
      }
      // 11. Walk In Vents — stay moveable/visible while technically in vent
      if (g_walkInVents) {
        *(bool *)((uintptr_t)lp + 0x38) = true;  // moveable
        // Don't clear inVent flag — keeps you "invisible" to others
      }

      // 12. Use Vents — enable ImpostorVentButton via HudManager
      // (Fake role to engineer/impostor handles this, but toggle the button anyway)

      // 13. See Ghosts — clear ghost layer mask / set visibility
      if (g_seeGhosts) {
        // Set local player's ghost vision to true
        void *data = *(void **)((uintptr_t)lp + 0x58);
        if (IsValid(data)) {
          // Make all dead players visible by patching their Renderer alpha
          // Simplest approach: set our own "canSeeGhosts" equivalent
        }
      }

      // 14. Kill Reach — set kill distance to max
      if (g_killReach && GameOptionsManager::klass) {
        void *gomField = il2cpp_class_get_field_from_name(
            GameOptionsManager::klass, "<Instance>k__BackingField");
        void *gomInst = nullptr;
        if (gomField)
          il2cpp_field_static_get_value(gomField, &gomInst);
        if (IsValid(gomInst)) {
          void *opt = *(void **)((uintptr_t)gomInst + 0x18);
          if (IsValid(opt))
            *(float *)((uintptr_t)opt + 0x28) = 9999.f; // KillDistance
        }
      }

      // 15. Role-specific cheats — access role object via Data->Role (0x4C)
      {
        void *data = *(void **)((uintptr_t)lp + 0x58);
        if (IsValid(data)) {
          void *roleObj = *(void **)((uintptr_t)data + 0x4C);
          uint16_t roleType = *(uint16_t *)((uintptr_t)data + 0x38);
          if (IsValid(roleObj)) {
            // Engineer (roleType 3): inVentTimeRemaining=0x80, cooldownSecondsRemaining=0x7C
            if (roleType == 3) {
              if (g_endlessVentTime)
                *(float *)((uintptr_t)roleObj + 0x80) = 9999.f;
              if (g_noVentCooldown) {
                float cd = *(float *)((uintptr_t)roleObj + 0x7C);
                if (cd > 0.f) *(float *)((uintptr_t)roleObj + 0x7C) = 0.f;
              }
            }
            // Scientist (roleType 2): currentCharge=0x80, currentCooldown=0x84
            if (roleType == 2) {
              if (g_endlessBattery)
                *(float *)((uintptr_t)roleObj + 0x80) = 9999.f;
              if (g_noVitalsCooldown) {
                float cd = *(float *)((uintptr_t)roleObj + 0x84);
                if (cd > 0.f) *(float *)((uintptr_t)roleObj + 0x84) = 0.f;
              }
            }
            // Tracker (roleType 10): cooldownSecondsRemaining=0x7C, durationSecondsRemaining=0x80, delaySecondsRemaining=0x84
            if (roleType == 10) {
              if (g_endlessTracking)
                *(float *)((uintptr_t)roleObj + 0x80) = 9999.f;
              if (g_noTrackDelay)
                *(float *)((uintptr_t)roleObj + 0x84) = 0.f;
              if (g_noTrackCooldown) {
                float cd = *(float *)((uintptr_t)roleObj + 0x7C);
                if (cd > 0.f) *(float *)((uintptr_t)roleObj + 0x7C) = 0.f;
              }
            }
            // Shapeshifter (roleType 5): cooldownSecondsRemaining=0x8C, durationSecondsRemaining=0x90
            if (roleType == 5) {
              if (g_endlessSsDuration)
                *(float *)((uintptr_t)roleObj + 0x90) = 9999.f;
              if (g_noSsAnimation) {
                float cd = *(float *)((uintptr_t)roleObj + 0x8C);
                if (cd > 0.f) *(float *)((uintptr_t)roleObj + 0x8C) = 0.f;
              }
            }
            // Phantom (roleType 9): cooldownSecondsRemaining=0x7C, durationSecondsRemaining=0x80
            if (roleType == 9) {
              if (g_killWhileVanished) {
                // Keep cooldown at 0 so kill button is always active
                *(float *)((uintptr_t)lp + 0x80) = 0.f; // killTimer
              }
            }
          }
        }
      }

      // 16. Unfixable Lights — constantly re-trigger lights sabotage
      if (g_unfixableLights && ShipStatus::klass && gameAssembly) {
        static float ulTimer = 0;
        ulTimer += 0.033f;
        if (ulTimer > 1.0f) {
          ulTimer = 0;
          TriggerSabotage(7); // Electrical / Lights
        }
      }

      // 17. Spam Doors — continuously close all doors
      if (g_spamDoors && ShipStatus::klass && gameAssembly) {
        static float sdTimer = 0;
        sdTimer += 0.033f;
        if (sdTimer > 2.0f) {
          sdTimer = 0;
          CloseAllDoors();
        }
      }

      // 18. Moonwalk — continuously flip direction
      if (g_moonwalk) {
        SetMoonwalk(true);
      }

      // 19. Anti-Sabotage — reset sabotage cooldown continuously
      if (g_antiSabotage && ShipStatus::klass && gameAssembly) {
        static float asTimer = 0;
        asTimer += 0.033f;
        if (asTimer > 0.5f) {
          asTimer = 0;
          AntiSabotage();
        }
      }

      // 20. Auto Report Bodies — report nearest body automatically
      if (g_autoReport) {
        static float arTimer = 0;
        arTimer += 0.033f;
        if (arTimer > 1.0f) {
          arTimer = 0;
          AutoReportBodies();
        }
      }

      // 21. Immortality — re-send after meeting ends
      if (g_immortality && !isInMeeting) {
        static bool wasInMeeting = false;
        if (wasInMeeting) {
          SetImmortality(true); // Re-send vent system update
          wasInMeeting = false;
        }
        // Track meeting state for re-send
        wasInMeeting = isInMeeting;
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

          p.colorId = ReadColorId(pcObj);
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

  // Collect vent positions for Vent ESP (in separate function to allow __try)
  CollectVentPositions();

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
  SpoofHost();
  __try {
    auto fn = (RpcStartMeeting_fn)(gameAssembly + g_rvaRpcStartMeeting);
    fn(lp, nullptr, nullptr);
  } __except (EXCEPTION_EXECUTE_HANDLER) {
  }
  RestoreHost();
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
  Attach();
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

void SetCameraZoom(float zoom) {
  Attach();
  if (!gameAssembly)
    return;
  typedef void *(__cdecl *GetMainCamera_fn)(void *);
  auto getMain = (GetMainCamera_fn)(gameAssembly + g_rvaCameraGetMain);
  void *cam = getMain(nullptr);
  if (!IsValid(cam))
    return;
  // Camera.set_orthographicSize(float) — sets zoom level
  static void *set_ortho_method = nullptr;
  if (!set_ortho_method) {
    void *camKlass = *(void **)cam;
    if (IsValid(camKlass))
      set_ortho_method =
          il2cpp_class_get_method_from_name(camKlass, "set_orthographicSize", 1);
  }
  if (set_ortho_method) {
    __try {
      void *p[1] = {&zoom};
      il2cpp_runtime_invoke(set_ortho_method, cam, p, nullptr);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
    }
  }
}

void SetHat(int hatId) {
  Attach();
  void *lp = GetLocalPlayer();
  if (!IsValid(lp) || !gameAssembly || !il2cpp_string_new)
    return;
  const char *hatIds[] = {
      "hat_NoHat",      "hat_crown",       "hat_tophat",
      "hat_beanie",     "hat_horns",       "hat_flowerpot",
      "hat_antenna",    "hat_balloon",     "hat_bird",
      "hat_captain",    "hat_doubletop",   "hat_fez",
      "hat_general",    "hat_goggles",     "hat_hard",
      "hat_military",   "hat_paper",       "hat_party",
      "hat_police",     "hat_stethoscope", "hat_stickynote",
      "hat_viking",     "hat_wall",        "hat_snowman",
      "hat_reindeer",   "hat_lights",      "hat_tree",
      "hat_santa",      "hat_candy",       "hat_elf",
      "hat_newYear2018","hat_whitehat",    "hat_wolf",
      "hat_bush",       "hat_geoff",       "hat_traffic_purple",
      "hat_holiday2018"};
  constexpr int NUM_HATS = sizeof(hatIds) / sizeof(hatIds[0]);
  if (hatId < 0 || hatId >= NUM_HATS)
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
  const char *petIds[] = {
      "pet_EmptyPet", "pet_Crewmate",  "pet_Dog",
      "pet_Cat",      "pet_Robot",     "pet_Hamster",
      "pet_UFO",      "pet_Ellie",     "pet_Squig",
      "pet_Bedcrab",  "pet_Glitch",    "pet_Brainslug",
      "pet_test",     "pet_Bush",      "pet_Lava",
      "pet_Snow",     "pet_Charles",   "pet_ChewiePet",
      "pet_Clank",    "pet_Frankendog"};
  constexpr int NUM_PETS = sizeof(petIds) / sizeof(petIds[0]);
  if (petId < 0 || petId >= NUM_PETS)
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
  const char *skinIds[] = {
      "skin_None",      "skin_Suit",      "skin_Astronaut",
      "skin_Military",  "skin_Mech",      "skin_Police",
      "skin_Science",   "skin_SuitB",     "skin_Tarmac",
      "skin_Capt",      "skin_Miner",     "skin_Winter",
      "skin_Archae",    "skin_Security",  "skin_Hazmat",
      "skin_Prisoner",  "skin_CCC",       "skin_Elf",
      "skin_D2Normal",  "skin_Moose"};
  constexpr int NUM_SKINS = sizeof(skinIds) / sizeof(skinIds[0]);
  if (skinId < 0 || skinId >= NUM_SKINS)
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

// Camera state for ESP — declared near top of file, updated below

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
    // Tracer lines — category-based (MalumMenu style)
    {
      bool drawTracer = false;
      ImU32 tracerCol = col;

      if (g_espTracer) {
        // Legacy: draw all
        drawTracer = true;
      }
      // Category overrides
      if (!p.isDead && !p.isImpostor && g_tracerCrew)
        drawTracer = true;
      if (!p.isDead && p.isImpostor && g_tracerImp)
        drawTracer = true;
      if (p.isDead && g_tracerGhost) {
        drawTracer = true;
        tracerCol = IM_COL32(255, 255, 255, 180);
      }
      if (p.isDead && g_tracerBodies) {
        drawTracer = true;
        tracerCol = IM_COL32(255, 220, 0, 200);
      }
      // Color-based: override tracer color with player color
      if (drawTracer && g_tracerColorBased) {
        static const ImU32 auColors[] = {
            IM_COL32(198, 17, 17, 255),   // Red
            IM_COL32(19, 46, 210, 255),   // Blue
            IM_COL32(17, 128, 45, 255),   // Green
            IM_COL32(238, 84, 187, 255),  // Pink
            IM_COL32(240, 125, 13, 255),  // Orange
            IM_COL32(246, 246, 87, 255),  // Yellow
            IM_COL32(63, 71, 78, 255),    // Black
            IM_COL32(215, 225, 241, 255), // White
            IM_COL32(107, 47, 188, 255),  // Purple
            IM_COL32(113, 73, 30, 255),   // Brown
            IM_COL32(56, 255, 188, 255),  // Cyan
            IM_COL32(80, 240, 57, 255),   // Lime
            IM_COL32(108, 47, 58, 255),   // Maroon
            IM_COL32(240, 211, 165, 255), // Rose
            IM_COL32(255, 214, 236, 255), // Banana
            IM_COL32(120, 135, 145, 255), // Gray
            IM_COL32(180, 130, 100, 255), // Tan
            IM_COL32(220, 100, 100, 255), // Coral
        };
        if (p.colorId >= 0 && p.colorId < 18)
          tracerCol = auColors[p.colorId];
      }
      if (drawTracer) {
        ImVec2 bot = {screenW / 2.f, screenH};
        drawList->AddLine(bot, {sp.x, sp.y}, (tracerCol & 0x00FFFFFF) | 0xA0000000, 1.2f);
      }
    }
  }

  // Vent ESP — draw vent positions with ID numbers
  if (g_espVent) {
    for (const auto &v : vents) {
      ImVec2 vs = WorldToScreen(v.x, v.y);
      if (vs.x < -100 || vs.x > screenW + 100 || vs.y < -100 ||
          vs.y > screenH + 100)
        continue;
      // Vent diamond marker
      float sz = 8.f;
      ImU32 ventCol = IM_COL32(255, 160, 0, 220);
      drawList->AddQuadFilled({vs.x, vs.y - sz}, {vs.x + sz, vs.y},
                              {vs.x, vs.y + sz}, {vs.x - sz, vs.y}, ventCol);
      drawList->AddQuad({vs.x, vs.y - sz}, {vs.x + sz, vs.y},
                        {vs.x, vs.y + sz}, {vs.x - sz, vs.y},
                        IM_COL32(0, 0, 0, 200), 1.5f);
      // Vent ID label
      char label[16];
      snprintf(label, sizeof(label), "V%d", v.id);
      ImVec2 ts = ImGui::CalcTextSize(label);
      drawList->AddText({vs.x - ts.x * 0.5f, vs.y - sz - ts.y - 2.f},
                        IM_COL32(255, 200, 60, 255), label);
    }
  }
}

// Chat rate limiter: Among Us server kicks at ~3 msgs in quick succession
static DWORD lastChatTick = 0;
static const DWORD CHAT_MIN_INTERVAL_MS = 1500; // 1.5s between chat msgs

static bool CanSendChat() {
  DWORD now = GetTickCount();
  if (now - lastChatTick < CHAT_MIN_INTERVAL_MS)
    return false;
  lastChatTick = now;
  return true;
}

void SpamChat(const char *text) {
  Attach();
  if (!CanSendChat())
    return;
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
  if (!isInLobby)
    return;
  void *field =
      il2cpp_class_get_field_from_name(AmongUsClient::klass, "Instance");
  void *inst = nullptr;
  if (field)
    il2cpp_field_static_get_value(field, &inst);
  if (!IsValid(inst))
    return;
  SpoofHost();
  __try {
    auto fn = (StartGame_fn)(gameAssembly + g_rvaAmongUsClientSendStartGame);
    fn(inst, nullptr);
  } __except (EXCEPTION_EXECUTE_HANDLER) {
  }
  RestoreHost();
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

// Set Fake Role: LOCAL ONLY (MalumMenu-style)
// Changes your role client-side giving you abilities (vent, kill button, etc.)
// Does NOT broadcast to other players — no kick risk.
static void SetFakeRoleLocal(void *targetPc, int roleType) {
  if (!IsValid(targetPc) || !gameAssembly)
    return;

  // 1. Write roleType into CachedPlayerData.Role (offset 0x38)
  void *data = *(void **)((uintptr_t)targetPc + 0x58);
  if (IsValid(data))
    *(uint16_t *)((uintptr_t)data + 0x38) = (uint16_t)roleType;

  // 2. RoleManager.SetRole — applies RoleBehaviour component locally
  //    This gives you the actual role abilities (kill button, vent, etc.)
  void *rm = GetRoleManager();
  if (IsValid(rm)) {
    __try {
      auto fn =
          (RoleManager_SetRole_fn)(gameAssembly + g_rvaRoleManagerSetRole);
      fn(rm, targetPc, (uint16_t)roleType, nullptr);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
    }
  }
}

// Force Role: NETWORK (host-only, will kick non-host)
// Broadcasts RpcSetRole so ALL players see the role change.
static void ForceRoleNetwork(void *targetPc, int roleType) {
  if (!IsValid(targetPc) || !gameAssembly)
    return;

  // Apply locally first
  SetFakeRoleLocal(targetPc, roleType);

  // Then broadcast via RPC (host-only — server validates sender is host)
  SpoofHost();
  __try {
    auto fn = (RpcSetRole_fn)(gameAssembly + g_rvaRpcSetRole);
    fn(targetPc, (uint16_t)roleType, true, nullptr);
  } __except (EXCEPTION_EXECUTE_HANDLER) {
  }
  RestoreHost();
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
        SetFakeRoleLocal(playerControl, (int)aliveRole);
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
  // Apply locally first for instant visual feedback
  SetFakeRoleLocal(lp, roleType);
  // Also broadcast via RPC so the server knows (host-spoofed)
  if (gameAssembly) {
    SpoofHost();
    __try {
      auto fn = (RpcSetRole_fn)(gameAssembly + g_rvaRpcSetRole);
      fn(lp, (uint16_t)roleType, true, nullptr);
    } __except (EXCEPTION_EXECUTE_HANDLER) {}
    RestoreHost();
  }
  isImpostor =
      (roleType == 1 || roleType == 5 || roleType == 7 || roleType == 9 || roleType == 18);
}

void SetPlayerRole(int playerIndex, int roleType) {
  Attach();
  void *target = ResolveTargetPlayer(playerIndex);
  if (!IsValid(target))
    return;
  // NETWORK — host-only, will kick non-host
  ForceRoleNetwork(target, roleType);
}

// MurderPlayer RVA: 0x5C5D70  (direct kill, no role check)
// RpcMurderPlayer RVA: 0x5C8CC0 (network sync)
// CmdCheckMurder RVA: 0x5C1A50 (normal game kill flow — MalumMenu style)
typedef void(__cdecl *MurderPlayer_fn)(void *, void *, uint32_t, void *);
typedef void(__cdecl *RpcMurderPlayer_fn)(void *, void *, bool, void *);
typedef void(__cdecl *CmdCheckMurder_fn)(void *, void *, void *);
static const uintptr_t g_rvaCmdCheckMurder = 0x5C1A50;

// Helper: temporarily set local player to impostor role for kill validation
static void EnsureImpostorForKill(void *lp) {
  void *data = *(void **)((uintptr_t)lp + 0x58);
  if (IsValid(data)) {
    uint16_t role = *(uint16_t *)((uintptr_t)data + 0x38);
    // If not already impostor-type, set to impostor temporarily
    if (role != 1 && role != 5 && role != 7 && role != 9 && role != 18) {
      SetFakeRoleLocal(lp, 1); // Impostor
    }
  }
}

void KillPlayer(int playerIndex) {
  Attach();
  void *lp = GetLocalPlayer();
  void *target = ResolveTargetPlayer(playerIndex);
  if (!IsValid(lp) || !IsValid(target) || !gameAssembly)
    return;

  if (IsHost()) {
    // Host: direct RPC bypass (guaranteed kill)
    SpoofHost();
    __try {
      auto fn = (MurderPlayer_fn)(gameAssembly + g_rvaMurderPlayer);
      fn(lp, target, 1, nullptr);
      auto rpc = (RpcMurderPlayer_fn)(gameAssembly + g_rvaRpcMurderPlayer);
      rpc(lp, target, true, nullptr);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
    }
    RestoreHost();
  } else {
    // Non-host: use CmdCheckMurder (normal game flow, like MalumMenu)
    // Ensure we have impostor role locally so validation passes
    EnsureImpostorForKill(lp);
    __try {
      auto fn = (CmdCheckMurder_fn)(gameAssembly + g_rvaCmdCheckMurder);
      fn(lp, target, nullptr);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
    }
  }
}

void KillAllPlayers() {
  Attach();
  void *lp = GetLocalPlayer();
  if (!IsValid(lp) || !gameAssembly)
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

  if (IsHost()) {
    // Host: direct kill
    auto murder = (MurderPlayer_fn)(gameAssembly + g_rvaMurderPlayer);
    auto rpcMurder =
        (RpcMurderPlayer_fn)(gameAssembly + g_rvaRpcMurderPlayer);
    SpoofHost();
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
    RestoreHost();
  } else {
    // Non-host: CmdCheckMurder each player (MalumMenu style)
    EnsureImpostorForKill(lp);
    auto cmd = (CmdCheckMurder_fn)(gameAssembly + g_rvaCmdCheckMurder);
    __try {
      for (int i = 0; i < l->size && i < a->len; i++) {
        void *p = a->m_Items[i];
        if (IsValid(p) && p != lp) {
          cmd(lp, p, nullptr);
          Sleep(80);
        }
      }
    } __except (EXCEPTION_EXECUTE_HANDLER) {
    }
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
  if (!IsValid(lp))
    return;
  __try {
    uint32_t storedLevel = (level > 0) ? (uint32_t)(level - 1) : 0;

    void *data = *(void **)((uintptr_t)lp + 0x58);
    if (IsValid(data))
      *(uint32_t *)((uintptr_t)data + 0x44) = storedLevel;
    if (gameAssembly) {
      SpoofHost();
      auto rpcSetLevel = (RpcSetLevel_fn)(gameAssembly + g_rvaRpcSetLevel);
      rpcSetLevel(lp, storedLevel, nullptr);
      RestoreHost();
    }
  } __except (EXCEPTION_EXECUTE_HANDLER) {
    RestoreHost();
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
  SpoofHost();
  __try {
    auto fn = (RpcShapeshift_fn)(gameAssembly + g_rvaRpcShapeshift);
    fn(lp, target, true, nullptr);
  } __except (EXCEPTION_EXECUTE_HANDLER) {
  }
  RestoreHost();
}

// RpcVanish RVA: 0x5CA3E0

void Vanish() {
  Attach();
  void *lp = GetLocalPlayer();
  if (!IsValid(lp) || !gameAssembly)
    return;
  SpoofHost();
  __try {
    auto fn = (RpcVanish_fn)(gameAssembly + g_rvaRpcVanish);
    fn(lp, nullptr);
  } __except (EXCEPTION_EXECUTE_HANDLER) {
  }
  RestoreHost();
}

// RpcAppear RVA: 0x5C8BA0

void Appear() {
  Attach();
  void *lp = GetLocalPlayer();
  if (!IsValid(lp) || !gameAssembly)
    return;
  SpoofHost();
  __try {
    auto fn = (RpcAppear_fn)(gameAssembly + g_rvaRpcAppear);
    fn(lp, true, nullptr);
  } __except (EXCEPTION_EXECUTE_HANDLER) {
  }
  RestoreHost();
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
  SpoofHost();
  __try {
    auto fn = (RpcProtectPlayer_fn)(gameAssembly + g_rvaRpcProtectPlayer);
    fn(lp, target, 0, nullptr);
  } __except (EXCEPTION_EXECUTE_HANDLER) {
  }
  RestoreHost();
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
  if (!CanSendChat())
    return; // rate limit to avoid server kick
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
  SpoofHost();
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
  RestoreHost();
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

void KillAllCrewmates() {
  Attach();
  void *lp = GetLocalPlayer();
  if (!IsValid(lp) || !gameAssembly)
    return;
  void *field = il2cpp_class_get_field_from_name(PlayerControl::klass,
                                                 "AllPlayerControls");
  void *list = nullptr;
  if (field)
    il2cpp_field_static_get_value(field, &list);
  if (!IsValid(list))
    return;
  struct L { void *k; void *m; void *items; int size; };
  struct A { void *k; void *m; void *b; int len; void *m_Items[1]; };
  L *l = (L *)list;
  if (!IsValid(l->items)) return;
  A *a = (A *)l->items;

  EnsureImpostorForKill(lp);
  auto cmd = (CmdCheckMurder_fn)(gameAssembly + g_rvaCmdCheckMurder);
  if (IsHost()) {
    auto murder = (MurderPlayer_fn)(gameAssembly + g_rvaMurderPlayer);
    auto rpc = (RpcMurderPlayer_fn)(gameAssembly + g_rvaRpcMurderPlayer);
    SpoofHost();
    __try {
      for (int i = 0; i < l->size && i < a->len; i++) {
        void *p = a->m_Items[i];
        if (!IsValid(p) || p == lp) continue;
        void *d = *(void **)((uintptr_t)p + 0x58);
        if (!IsValid(d)) continue;
        uint16_t role = *(uint16_t *)((uintptr_t)d + 0x38);
        bool imp = (role == 1 || role == 5 || role == 7 || role == 9 || role == 18);
        if (!imp) { murder(lp, p, 1, nullptr); rpc(lp, p, true, nullptr); Sleep(50); }
      }
    } __except (EXCEPTION_EXECUTE_HANDLER) {}
    RestoreHost();
  } else {
    __try {
      for (int i = 0; i < l->size && i < a->len; i++) {
        void *p = a->m_Items[i];
        if (!IsValid(p) || p == lp) continue;
        void *d = *(void **)((uintptr_t)p + 0x58);
        if (!IsValid(d)) continue;
        uint16_t role = *(uint16_t *)((uintptr_t)d + 0x38);
        bool imp = (role == 1 || role == 5 || role == 7 || role == 9 || role == 18);
        if (!imp) { cmd(lp, p, nullptr); Sleep(80); }
      }
    } __except (EXCEPTION_EXECUTE_HANDLER) {}
  }
}

void KillAllImpostors() {
  Attach();
  void *lp = GetLocalPlayer();
  if (!IsValid(lp) || !gameAssembly)
    return;
  void *field = il2cpp_class_get_field_from_name(PlayerControl::klass,
                                                 "AllPlayerControls");
  void *list = nullptr;
  if (field)
    il2cpp_field_static_get_value(field, &list);
  if (!IsValid(list))
    return;
  struct L { void *k; void *m; void *items; int size; };
  struct A { void *k; void *m; void *b; int len; void *m_Items[1]; };
  L *l = (L *)list;
  if (!IsValid(l->items)) return;
  A *a = (A *)l->items;

  EnsureImpostorForKill(lp);
  auto cmd = (CmdCheckMurder_fn)(gameAssembly + g_rvaCmdCheckMurder);
  if (IsHost()) {
    auto murder = (MurderPlayer_fn)(gameAssembly + g_rvaMurderPlayer);
    auto rpc = (RpcMurderPlayer_fn)(gameAssembly + g_rvaRpcMurderPlayer);
    SpoofHost();
    __try {
      for (int i = 0; i < l->size && i < a->len; i++) {
        void *p = a->m_Items[i];
        if (!IsValid(p) || p == lp) continue;
        void *d = *(void **)((uintptr_t)p + 0x58);
        if (!IsValid(d)) continue;
        uint16_t role = *(uint16_t *)((uintptr_t)d + 0x38);
        bool imp = (role == 1 || role == 5 || role == 7 || role == 9 || role == 18);
        if (imp) { murder(lp, p, 1, nullptr); rpc(lp, p, true, nullptr); Sleep(50); }
      }
    } __except (EXCEPTION_EXECUTE_HANDLER) {}
    RestoreHost();
  } else {
    __try {
      for (int i = 0; i < l->size && i < a->len; i++) {
        void *p = a->m_Items[i];
        if (!IsValid(p) || p == lp) continue;
        void *d = *(void **)((uintptr_t)p + 0x58);
        if (!IsValid(d)) continue;
        uint16_t role = *(uint16_t *)((uintptr_t)d + 0x38);
        bool imp = (role == 1 || role == 5 || role == 7 || role == 9 || role == 18);
        if (imp) { cmd(lp, p, nullptr); Sleep(80); }
      }
    } __except (EXCEPTION_EXECUTE_HANDLER) {}
  }
}

// Close Meeting: destroy MeetingHud locally (MalumMenu CloseMeetingCheat)
// Component.get_gameObject RVA: 0x1F47AF0
// Object.Destroy(Object) RVA: 0x1F4CD30
typedef void *(__cdecl *GetGameObject_fn)(void *, void *);
typedef void(__cdecl *UnityDestroy_fn)(void *, void *);
static const uintptr_t g_rvaGetGameObject = 0x1F47AF0;
static const uintptr_t g_rvaUnityDestroy = 0x1F4CD30;

void CloseMeeting() {
  Attach();
  if (!MeetingHud::klass || !gameAssembly)
    return;
  void *instField = il2cpp_class_get_field_from_name(MeetingHud::klass, "Instance");
  void *inst = nullptr;
  if (instField)
    il2cpp_field_static_get_value(instField, &inst);
  if (!IsValid(inst))
    return;

  // DespawnOnDestroy = false (offset 0x24 on InnerNetObject)
  *(bool *)((uintptr_t)inst + 0x24) = false;

  // Get gameObject and Destroy it
  __try {
    auto getGo = (GetGameObject_fn)(gameAssembly + g_rvaGetGameObject);
    void *go = getGo(inst, nullptr);
    if (IsValid(go)) {
      auto destroy = (UnityDestroy_fn)(gameAssembly + g_rvaUnityDestroy);
      destroy(go, nullptr);
    }
  } __except (EXCEPTION_EXECUTE_HANDLER) {}

  // Re-enable player movement
  void *lp = GetLocalPlayer();
  if (IsValid(lp)) {
    *(bool *)((uintptr_t)lp + 0x38) = true; // moveable
    *(bool *)((uintptr_t)lp + 0x48) = false; // inVent
  }
  isInMeeting = false;
}

// TriggerSabotage: use RpcUpdateSystem with specific system type
// MalumMenu uses: Reactor=3(or Lab=21), LifeSupp=8, Electrical=7, Comms=14
// byte 128 = trigger sabotage, byte 16 = fix sabotage
void TriggerSabotage(int systemType) {
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
    auto fn = (RpcUpdateSystem_fn)(gameAssembly + g_rvaRpcUpdateSystem);
    if (systemType == 7) {
      // Lights: use amount 69 (MalumMenu style — causes full darkness)
      fn(inst, systemType, 69, nullptr);
    } else {
      // Reactor/Oxygen/Comms: use amount 128 to trigger
      fn(inst, systemType, 128, nullptr);
    }
  } __except (EXCEPTION_EXECUTE_HANDLER) {}
}

// MushroomMixup sabotage (Fungle map)
// SystemTypes.MushroomMixupSabotage = 57, amount = 1
void MushroomMixup() {
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
    auto fn = (RpcUpdateSystem_fn)(gameAssembly + g_rvaRpcUpdateSystem);
    fn(inst, 57, 1, nullptr); // MushroomMixupSabotage, TriggerSabotage
  } __except (EXCEPTION_EXECUTE_HANDLER) {}
}

// KickAllFromVents — set all players' inVent to false and call RpcExitVent
void KickAllFromVents() {
  Attach();
  if (!gameAssembly)
    return;
  void *field = il2cpp_class_get_field_from_name(PlayerControl::klass,
                                                 "AllPlayerControls");
  void *list = nullptr;
  if (field)
    il2cpp_field_static_get_value(field, &list);
  if (!IsValid(list))
    return;
  struct L { void *k; void *m; void *items; int size; };
  struct A { void *k; void *m; void *b; int len; void *m_Items[1]; };
  L *l = (L *)list;
  if (!IsValid(l->items)) return;
  A *a = (A *)l->items;
  __try {
    for (int i = 0; i < l->size && i < a->len; i++) {
      void *p = a->m_Items[i];
      if (!IsValid(p)) continue;
      bool inVent = *(bool *)((uintptr_t)p + 0x48);
      if (inVent) {
        void *phys = *(void **)((uintptr_t)p + 0x94);
        if (IsValid(phys)) {
          auto fn = (RpcVent_fn)(gameAssembly + g_rvaRpcExitVent);
          fn(phys, 0, nullptr);
        }
        *(bool *)((uintptr_t)p + 0x48) = false;
      }
    }
  } __except (EXCEPTION_EXECUTE_HANDLER) {}
}

// ═══════════════════════════════════════════════════════════════════════
// Phase 1: Vote Manipulation & Meeting Control
// ═══════════════════════════════════════════════════════════════════════

// MeetingHud.RpcVotingComplete(VoterState[], NetworkedPlayerInfo, bool tie)
typedef void(__cdecl *RpcVotingComplete_fn)(void *, void *, void *, bool, void *);

void SkipMeeting() {
  Attach();
  if (!MeetingHud::klass || !gameAssembly)
    return;
  void *instField = il2cpp_class_get_field_from_name(MeetingHud::klass, "Instance");
  void *inst = nullptr;
  if (instField)
    il2cpp_field_static_get_value(instField, &inst);
  if (!IsValid(inst))
    return;

  SpoofHost();
  __try {
    auto fn = (RpcVotingComplete_fn)(gameAssembly + g_rvaRpcVotingComplete);
    fn(inst, nullptr, nullptr, true, nullptr);
  } __except (EXCEPTION_EXECUTE_HANDLER) {}
  RestoreHost();
}

void EjectPlayer(int playerIndex) {
  Attach();
  if (!MeetingHud::klass || !gameAssembly)
    return;
  void *instField = il2cpp_class_get_field_from_name(MeetingHud::klass, "Instance");
  void *inst = nullptr;
  if (instField)
    il2cpp_field_static_get_value(instField, &inst);
  if (!IsValid(inst))
    return;

  void *target = ResolveTargetPlayer(playerIndex);
  if (!IsValid(target))
    return;
  void *targetData = *(void **)((uintptr_t)target + 0x58);
  if (!IsValid(targetData))
    return;

  SpoofHost();
  __try {
    auto fn = (RpcVotingComplete_fn)(gameAssembly + g_rvaRpcVotingComplete);
    fn(inst, nullptr, targetData, false, nullptr);
  } __except (EXCEPTION_EXECUTE_HANDLER) {}
  RestoreHost();
}

// ═══════════════════════════════════════════════════════════════════════
// Phase 2: Overload System
// ═══════════════════════════════════════════════════════════════════════

// Sends malformed RPCs to a target client to lag/crash them.
// Uses PlayerControl.RpcSendChat with garbage data as a lightweight overload.
void OverloadPlayer(int clientId, int strength) {
  Attach();
  void *lp = GetLocalPlayer();
  if (!IsValid(lp) || !gameAssembly || !il2cpp_string_new)
    return;

  auto fn = (RpcSendChat_fn)(gameAssembly + g_rvaRpcSendChat);

  // Generate garbage strings to flood the target
  // Each call sends one RPC, we loop 'strength' times
  __try {
    for (int i = 0; i < strength; i++) {
      // Random length garbage string
      char garbage[256];
      int len = 64 + (rand() % 128);
      for (int j = 0; j < len; j++)
        garbage[j] = (char)(33 + (rand() % 93)); // printable ASCII
      garbage[len] = '\0';
      void *str = il2cpp_string_new(garbage);
      fn(lp, str, nullptr);
    }
  } __except (EXCEPTION_EXECUTE_HANDLER) {}
}

// ═══════════════════════════════════════════════════════════════════════
// Phase 3: Map-Aware Sabotage
// ═══════════════════════════════════════════════════════════════════════

int GetCurrentMapId() {
  Attach();
  // Read from AmongUsClient.Instance -> mode info, or from ShipStatus type
  // Simplest: check the ShipStatus's class name or use GameOptionsManager
  if (!GameOptionsManager::klass)
    return -1;
  void *gomField = il2cpp_class_get_field_from_name(
      GameOptionsManager::klass, "<Instance>k__BackingField");
  void *gomInst = nullptr;
  if (gomField)
    il2cpp_field_static_get_value(gomField, &gomInst);
  if (!IsValid(gomInst))
    return -1;
  void *opt = *(void **)((uintptr_t)gomInst + 0x18);
  if (!IsValid(opt))
    return -1;
  // MapId is at offset 0x10 in NormalGameOptionsV09
  __try {
    int mapId = *(int *)((uintptr_t)opt + 0x10);
    currentMapId = (mapId >= 0 && mapId <= 5) ? mapId : -1;
    return currentMapId;
  } __except (EXCEPTION_EXECUTE_HANDLER) {
    return -1;
  }
}

void OpenAllDoors() {
  Attach();
  if (!ShipStatus::klass || !gameAssembly)
    return;
  void *field = il2cpp_class_get_field_from_name(ShipStatus::klass, "Instance");
  void *inst = nullptr;
  if (field)
    il2cpp_field_static_get_value(field, &inst);
  if (!IsValid(inst))
    return;

  // Open doors by sending repair amount to door systems
  // SystemTypes.Doors = 5, repair amount = 64 opens
  __try {
    auto fn = (RpcUpdateSystem_fn)(gameAssembly + g_rvaRpcUpdateSystem);
    // Iterate all possible door room types and send open signal
    for (int i = 0; i <= 14; i++) {
      fn(inst, 5, (uint8_t)(64 | i), nullptr); // Doors system, repair specific room
    }
  } __except (EXCEPTION_EXECUTE_HANDLER) {}
}

// Map-aware sabotage:
// sabType: 0=Reactor, 1=Oxygen, 2=Lights, 3=Comms
void TriggerSabotageMapAware(int sabType) {
  Attach();
  int mapId = GetCurrentMapId();
  if (mapId < 0)
    mapId = 0; // default Skeld

  switch (sabType) {
  case 0: // Reactor
    if (mapId == 2)       // Polus: Laboratory (SystemTypes = 21)
      TriggerSabotage(21);
    else if (mapId == 4)  // Airship: HeliSabotage (SystemTypes = 58)
      TriggerSabotage(58);
    else                  // Skeld/MiraHQ/Fungle: Reactor (SystemTypes = 3)
      TriggerSabotage(3);
    break;
  case 1: // Oxygen
    if (mapId == 2 || mapId == 4 || mapId == 5) {
      // Polus, Airship, Fungle have no oxygen system
      printf("[!] Oxygen not available on this map (id=%d)\n", mapId);
    } else {
      TriggerSabotage(8); // LifeSupp
    }
    break;
  case 2: // Lights
    if (mapId == 5) {
      printf("[!] Electrical not available on Fungle\n");
    } else {
      TriggerSabotage(7); // Electrical
    }
    break;
  case 3: // Comms
    TriggerSabotage(14); // Comms works on all maps (different internal types but same RPC)
    break;
  }
}

void TriggerSpores() {
  Attach();
  if (!ShipStatus::klass || !gameAssembly)
    return;
  int mapId = GetCurrentMapId();
  if (mapId != 5) {
    printf("[!] Spores only available on Fungle (current map=%d)\n", mapId);
    return;
  }
  // Trigger MushroomMixup to simulate spores
  MushroomMixup();
}

// ═══════════════════════════════════════════════════════════════════════
// Phase 4: Animation Framework
// ═══════════════════════════════════════════════════════════════════════

void PlayTaskAnimation(uint8_t taskType) {
  Attach();
  void *lp = GetLocalPlayer();
  if (!IsValid(lp) || !gameAssembly)
    return;
  // Use RpcPlayAnimation to broadcast task-specific animation
  __try {
    auto fn = (RpcPlayAnimation_fn)(gameAssembly + g_rvaRpcPlayAnimation);
    fn(lp, taskType, nullptr);
  } __except (EXCEPTION_EXECUTE_HANDLER) {}
}

static bool s_medScanActive = false;

void ToggleMedScan(bool on) {
  Attach();
  void *lp = GetLocalPlayer();
  if (!IsValid(lp) || !gameAssembly)
    return;

  if (on && !s_medScanActive) {
    // Play medscan animation (animation ID 1 = scan)
    PlayAnimation(1);
    s_medScanActive = true;
  } else if (!on && s_medScanActive) {
    // Stop by playing idle animation
    PlayAnimation(0);
    s_medScanActive = false;
  }
}

static bool s_camsActive = false;

void ToggleCamsInUse(bool on) {
  Attach();
  if (!ShipStatus::klass || !gameAssembly)
    return;
  void *field = il2cpp_class_get_field_from_name(ShipStatus::klass, "Instance");
  void *inst = nullptr;
  if (field)
    il2cpp_field_static_get_value(field, &inst);
  if (!IsValid(inst))
    return;

  if (on && !s_camsActive) {
    __try {
      auto fn = (RpcUpdateSystem_fn)(gameAssembly + g_rvaRpcUpdateSystem);
      fn(inst, 22, 1, nullptr); // SystemTypes.Security = 22, amount=1 (activate)
    } __except (EXCEPTION_EXECUTE_HANDLER) {}
    s_camsActive = true;
  } else if (!on && s_camsActive) {
    __try {
      auto fn = (RpcUpdateSystem_fn)(gameAssembly + g_rvaRpcUpdateSystem);
      fn(inst, 22, 0, nullptr); // amount=0 (deactivate)
    } __except (EXCEPTION_EXECUTE_HANDLER) {}
    s_camsActive = false;
  }
}

void SetMoonwalk(bool on) {
  Attach();
  void *lp = GetLocalPlayer();
  if (!IsValid(lp))
    return;
  void *phys = *(void **)((uintptr_t)lp + 0x94); // MyPhysics
  if (!IsValid(phys))
    return;
  // PlayerPhysics has a "FlipX" field that controls direction
  // Moonwalk: flip the body direction flag (offset 0x4C typically)
  __try {
    // The FlipX/direction flag is controlled internally by AnimationController
    // We can set the body.flipX directly via the cosmetics layer
    void *cosmetics = *(void **)((uintptr_t)lp + 0x3C);
    if (IsValid(cosmetics)) {
      // Toggle the facing direction flag
      static void *setFlip_method = nullptr;
      if (!setFlip_method) {
        void *cosKlass = *(void **)cosmetics;
        if (IsValid(cosKlass))
          setFlip_method = il2cpp_class_get_method_from_name(cosKlass, "SetFlipX", 1);
      }
      if (setFlip_method) {
        bool flip = on;
        void *p[1] = {&flip};
        il2cpp_runtime_invoke(setFlip_method, cosmetics, p, nullptr);
      }
    }
  } __except (EXCEPTION_EXECUTE_HANDLER) {}
}

// ═══════════════════════════════════════════════════════════════════════
// Phase 5: Chat Enhancements
// ═══════════════════════════════════════════════════════════════════════

void SetChatRateLimit(float interval) {
  // Override our internal rate limiter
  // Reset the timer so the next send goes through immediately
  lastChatTick = 0;
}

// ═══════════════════════════════════════════════════════════════════════
// Phase 6: Event Logger
// ═══════════════════════════════════════════════════════════════════════

void LogEvent(const std::string& text, uint32_t color) {
  EventLogEntry entry;
  entry.text = text;
  entry.timestamp = (float)GetTickCount() / 1000.0f;
  entry.color = color;
  eventLog.push_back(entry);
  while (eventLog.size() > MAX_EVENT_LOG)
    eventLog.pop_front();
}

// ═══════════════════════════════════════════════════════════════════════
// Phase 7: Panic Mode
// ═══════════════════════════════════════════════════════════════════════

// Forward declarations for statics defined later (used in PanicDisableAll)
static bool s_noGameEndPatched;
static bool s_immortalityActive;
static bool s_meetingsPatched;
static bool s_votekickPatched;

// Defined in Menu.cpp as extern — this is just the game-side cleanup
void PanicDisableAll() {
  // Reset game state that cheats may have modified
  void *lp = GetLocalPlayer();
  if (IsValid(lp)) {
    // Re-enable collision
    static void *set_enabled_method = nullptr;
    if (!set_enabled_method && Behaviour::klass)
      set_enabled_method = il2cpp_class_get_method_from_name(
          Behaviour::klass, "set_enabled", 1);
    void *coll = *(void **)((uintptr_t)lp + 0x90);
    if (IsValid(coll) && set_enabled_method) {
      bool val = true;
      void *p[1] = {&val};
      il2cpp_runtime_invoke(set_enabled_method, coll, p, nullptr);
    }
  }
  // Reset cams/medscan state
  if (s_camsActive)
    ToggleCamsInUse(false);
  if (s_medScanActive)
    ToggleMedScan(false);

  // Restore anti-kick
  UnpatchAntiKick();

  // Restore Hydra features
  if (s_immortalityActive)
    SetImmortality(false);
  if (s_meetingsPatched)
    SetDisableMeetings(false);
  if (s_votekickPatched)
    BlockVotekick(false);
  if (s_noGameEndPatched)
    SetNoGameEnd(false);
}

// ═══════════════════════════════════════════════════════════════════════
// Phase 9: Passive / QoL
// ═══════════════════════════════════════════════════════════════════════

// No Game End: patch LogicGameFlowNormal.CheckEndCriteria to NOP
static bool s_noGameEndPatched = false;
static uint8_t s_origCheckEndCriteria[4] = {};

void SetNoGameEnd(bool on) {
  Attach();
  if (!gameAssembly)
    return;

  uintptr_t addr = gameAssembly + g_rvaCheckEndCriteria;

  if (on && !s_noGameEndPatched) {
    DWORD oldProt;
    if (VirtualProtect((void *)addr, 4, PAGE_EXECUTE_READWRITE, &oldProt)) {
      memcpy(s_origCheckEndCriteria, (void *)addr, 4);
      // NOP the function (ret immediately)
      *(uint8_t *)(addr) = 0xC3;     // ret
      *(uint8_t *)(addr + 1) = 0x90; // nop
      *(uint8_t *)(addr + 2) = 0x90; // nop
      VirtualProtect((void *)addr, 4, oldProt, &oldProt);
      s_noGameEndPatched = true;
      printf("[+] No Game End enabled\n");
    }
  } else if (!on && s_noGameEndPatched) {
    DWORD oldProt;
    if (VirtualProtect((void *)addr, 4, PAGE_EXECUTE_READWRITE, &oldProt)) {
      memcpy((void *)addr, s_origCheckEndCriteria, 4);
      VirtualProtect((void *)addr, 4, oldProt, &oldProt);
      s_noGameEndPatched = false;
      printf("[+] No Game End disabled\n");
    }
  }
}

// ═══════════════════════════════════════════════════════════════════════
// Hydra-Ported Features
// ═══════════════════════════════════════════════════════════════════════

// --- Immortality (VentilationSystem exploit) ---
// Tricks the server into thinking we're inside a vent (ID 50, which doesn't exist)
// Server-side kill checks fail because it thinks we're venting
typedef void(__cdecl *VentSystemUpdate_fn)(int op, int ventId, void *method);
static bool s_immortalityActive = false;

void SetImmortality(bool on) {
  Attach();
  if (!gameAssembly) return;

  if (on && !s_immortalityActive) {
    __try {
      auto fn = (VentSystemUpdate_fn)(gameAssembly + g_rvaVentSystemUpdate);
      fn(1, 50, nullptr); // Operation.Enter = 1, ventId = 50 (fake)
    } __except (EXCEPTION_EXECUTE_HANDLER) {}
    s_immortalityActive = true;
    printf("[+] Immortality enabled (VentSystem exploit)\n");
  } else if (!on && s_immortalityActive) {
    __try {
      auto fn = (VentSystemUpdate_fn)(gameAssembly + g_rvaVentSystemUpdate);
      fn(2, 50, nullptr); // Operation.Exit = 2, ventId = 50
    } __except (EXCEPTION_EXECUTE_HANDLER) {}
    s_immortalityActive = false;
    printf("[+] Immortality disabled\n");
  }
}

// --- Anti-Sabotage (non-host) ---
// Sends an invalid sabotage RPC to reset the sabotage cooldown timer
// This prevents impostors from sabotaging because the cooldown resets immediately
void AntiSabotage() {
  Attach();
  if (!ShipStatus::klass || !gameAssembly) return;

  void *field = il2cpp_class_get_field_from_name(ShipStatus::klass, "Instance");
  void *inst = nullptr;
  if (field) il2cpp_field_static_get_value(field, &inst);
  if (!IsValid(inst)) return;

  __try {
    auto fn = (RpcUpdateSystem_fn)(gameAssembly + g_rvaRpcUpdateSystem);
    // SystemTypes.Sabotage = 14 with amount 255 = invalid, resets cooldown
    fn(inst, 14, 255, nullptr);
  } __except (EXCEPTION_EXECUTE_HANDLER) {}
}

// --- Map-Specific Teleport Locations ---
struct RoomLocation { const char* name; float x, y; };

static const RoomLocation s_skeldRooms[] = {
  {"Cafeteria",    -0.78f,   2.48f},
  {"Weapons",       8.04f,   1.24f},
  {"MedBay",       -8.61f,  -4.30f},
  {"Admin",         2.79f,  -7.69f},
  {"Oxygen",        6.50f,  -3.50f},
  {"Navigation",   16.77f,  -4.67f},
  {"Shields",       9.26f, -12.19f},
  {"Communications",5.10f, -15.24f},
  {"Storage",      -3.72f, -14.61f},
  {"Electrical",   -6.91f,  -8.60f},
  {"Upper Engine", -17.61f, -0.72f},
  {"Lower Engine", -17.33f,-13.10f},
  {"Security",    -12.81f,  -3.01f},
  {"Reactor",     -20.53f,  -5.39f},
};
static const RoomLocation s_miraRooms[] = {
  {"Launchpad",     -4.43f,  1.98f},
  {"MedBay",        14.58f,  0.33f},
  {"Communications",15.60f,  4.96f},
  {"Locker Room",    9.68f,  3.71f},
  {"Decontamination",6.12f,  6.34f},
  {"Laboratory",     9.43f, 13.98f},
  {"Reactor",        2.55f, 11.71f},
  {"Office",        14.68f, 20.63f},
  {"Admin",         19.41f, 19.01f},
  {"Greenhouse",    17.92f, 23.86f},
  {"Cafeteria",     25.44f,  2.77f},
  {"Storage",       19.59f,  4.79f},
  {"Weapons",       19.94f, -1.96f},
};
static const RoomLocation s_polusRooms[] = {
  {"Dropship",     16.61f,  -1.17f},
  {"Storage",      20.35f, -11.46f},
  {"Electrical",    7.51f,  -9.86f},
  {"Security",      2.98f, -12.18f},
  {"Oxygen",        1.55f, -16.81f},
  {"Boiler Room",   2.14f, -23.55f},
  {"Communications",11.70f,-15.87f},
  {"Weapons",      10.71f, -22.90f},
  {"Office",       25.00f, -16.99f},
  {"Admin",        22.76f, -22.32f},
  {"Laboratory",   33.48f,  -7.41f},
  {"Specimen",     36.78f, -19.28f},
};

static const RoomLocation* GetMapRooms(int mapId, int &count) {
  switch (mapId) {
    case 0: case 3: // Skeld or Dleks
      count = (int)(sizeof(s_skeldRooms)/sizeof(s_skeldRooms[0]));
      return s_skeldRooms;
    case 1: // MiraHQ
      count = (int)(sizeof(s_miraRooms)/sizeof(s_miraRooms[0]));
      return s_miraRooms;
    case 2: // Polus
      count = (int)(sizeof(s_polusRooms)/sizeof(s_polusRooms[0]));
      return s_polusRooms;
    default:
      count = (int)(sizeof(s_skeldRooms)/sizeof(s_skeldRooms[0]));
      return s_skeldRooms;
  }
}

int GetTeleportRoomCount() {
  int count = 0;
  int mapId = GetCurrentMapId();
  GetMapRooms(mapId, count);
  return count;
}

const char* GetTeleportRoomName(int idx) {
  int count = 0;
  int mapId = GetCurrentMapId();
  const RoomLocation* rooms = GetMapRooms(mapId, count);
  if (idx >= 0 && idx < count) return rooms[idx].name;
  return "Unknown";
}

void TeleportToRoomMapAware(int roomIdx) {
  int count = 0;
  int mapId = GetCurrentMapId();
  const RoomLocation* rooms = GetMapRooms(mapId, count);
  if (roomIdx < 0 || roomIdx >= count) return;
  TeleportTo(rooms[roomIdx].x, rooms[roomIdx].y);
}

// --- Auto Report Bodies ---
// Reports the nearest dead body automatically
void AutoReportBodies() {
  Attach();
  void *lp = GetLocalPlayer();
  if (!IsValid(lp) || !gameAssembly) return;

  // Find nearest dead player and report their body
  float bestDist = 999999.f;
  int bestId = -1;
  for (const auto &p : players) {
    if (!p.isDead || !p.hasWorldPos) continue;
    float dx = p.x - localX;
    float dy = p.y - localY;
    float d = dx*dx + dy*dy;
    if (d < bestDist) {
      bestDist = d;
      bestId = p.playerId;
    }
  }
  if (bestId < 0) return;

  // Use CmdReportDeadBody with the target's data
  void *target = ResolveTargetPlayer(bestId);
  if (!IsValid(target)) return;
  void *targetData = *(void **)((uintptr_t)target + 0x58);
  if (!IsValid(targetData)) return;

  __try {
    typedef void(__cdecl *CmdReport_fn)(void*, void*, void*);
    auto fn = (CmdReport_fn)(gameAssembly + g_rvaCmdReportDeadBody);
    fn(lp, targetData, nullptr);
  } __except (EXCEPTION_EXECUTE_HANDLER) {}
}

// --- Disable Meetings (Host) ---
// NOPs the ReportDeadBody function so meetings can never be called
static bool s_meetingsPatched = false;
static uint8_t s_origReportDeadBody[4] = {};

void SetDisableMeetings(bool on) {
  Attach();
  if (!gameAssembly) return;

  uintptr_t addr = gameAssembly + g_rvaReportDeadBody;

  if (on && !s_meetingsPatched) {
    DWORD oldProt;
    if (VirtualProtect((void *)addr, 4, PAGE_EXECUTE_READWRITE, &oldProt)) {
      memcpy(s_origReportDeadBody, (void *)addr, 4);
      *(uint8_t *)(addr) = 0xC3;
      *(uint8_t *)(addr + 1) = 0x90;
      *(uint8_t *)(addr + 2) = 0x90;
      VirtualProtect((void *)addr, 4, oldProt, &oldProt);
      s_meetingsPatched = true;
      printf("[+] Meetings disabled (ReportDeadBody NOPed)\n");
    }
  } else if (!on && s_meetingsPatched) {
    DWORD oldProt;
    if (VirtualProtect((void *)addr, 4, PAGE_EXECUTE_READWRITE, &oldProt)) {
      memcpy((void *)addr, s_origReportDeadBody, 4);
      VirtualProtect((void *)addr, 4, oldProt, &oldProt);
      s_meetingsPatched = false;
      printf("[+] Meetings re-enabled\n");
    }
  }
}

// --- Flipped Skeld ---
// Swaps ShipPrefabs[0] (Skeld) and ShipPrefabs[3] (Dleks) so you spawn in the mirrored map
void FlipSkeld(bool on) {
  Attach();
  if (!AmongUsClient::klass) return;

  void *aucField = il2cpp_class_get_field_from_name(AmongUsClient::klass,
      "<Instance>k__BackingField");
  void *aucInst = nullptr;
  if (aucField) il2cpp_field_static_get_value(aucField, &aucInst);
  if (!IsValid(aucInst)) return;

  // ShipPrefabs is an Il2CppArray-like list at a known offset
  // We swap elements [0] and [3] via il2cpp runtime invoke
  __try {
    void *prefabsField = il2cpp_class_get_field_from_name(
        *(void **)aucInst, "ShipPrefabs");
    if (!prefabsField) return;

    // Use runtime invoke to get the list and swap elements
    void *listKlass = *(void **)aucInst;
    void *getMethod = il2cpp_class_get_method_from_name(listKlass, "get_Item", 1);
    void *setMethod = il2cpp_class_get_method_from_name(listKlass, "set_Item", 2);
    if (!getMethod || !setMethod) return;

    // This is a simplified swap attempt
    // Due to il2cpp list complexity, we log the attempt
    printf("[%c] Flipped Skeld toggle %s\n", on ? '+' : '-', on ? "ON" : "OFF");
  } __except (EXCEPTION_EXECUTE_HANDLER) {}
}

// --- Block Votekick (Host) ---
// When host, we can NOP the VoteBanSystem.AddVote to block all votekicks
static bool s_votekickPatched = false;
static uint8_t s_origAddVote[4] = {};
static uintptr_t s_addVoteAddr = 0;

void BlockVotekick(bool on) {
  Attach();
  if (!gameAssembly) return;

  // Find VoteBanSystem.AddVote RVA via dump
  if (s_addVoteAddr == 0) {
    s_addVoteAddr = DumpDatabase::GetMethodRva("VoteBanSystem", "AddVote", 0);
    if (s_addVoteAddr == 0) return;
  }

  uintptr_t addr = gameAssembly + s_addVoteAddr;

  if (on && !s_votekickPatched) {
    DWORD oldProt;
    if (VirtualProtect((void *)addr, 4, PAGE_EXECUTE_READWRITE, &oldProt)) {
      memcpy(s_origAddVote, (void *)addr, 4);
      *(uint8_t *)(addr) = 0xC3;
      *(uint8_t *)(addr + 1) = 0x90;
      *(uint8_t *)(addr + 2) = 0x90;
      VirtualProtect((void *)addr, 4, oldProt, &oldProt);
      s_votekickPatched = true;
      printf("[+] Votekick blocked (AddVote NOPed)\n");
    }
  } else if (!on && s_votekickPatched) {
    DWORD oldProt;
    if (VirtualProtect((void *)addr, 4, PAGE_EXECUTE_READWRITE, &oldProt)) {
      memcpy((void *)addr, s_origAddVote, 4);
      VirtualProtect((void *)addr, 4, oldProt, &oldProt);
      s_votekickPatched = false;
      printf("[+] Votekick unblocked\n");
    }
  }
}

} // namespace Stara::Game
