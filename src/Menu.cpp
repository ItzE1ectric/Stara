#include "Common.hpp"
#include "DumpDatabase.hpp"
#include "Game.hpp"
#include "Hooks.hpp"
#include <cctype>
#include <filesystem>
#include <shellapi.h>
#include <wincodec.h>

namespace Stara {

extern bool g_menuVisible;

// ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ State
// ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬
static Tab g_tab = Tab::Dashboard;
static float g_menuAlpha = 0.f;
static float g_hoverAnim[9] = {};
static std::unordered_map<ImGuiID, float> g_toggleAnim;
static std::unordered_map<ImGuiID, float> g_buttonAnim;
static float g_tabContentFade = 1.f;
static Tab g_lastRenderedTab = Tab::Dashboard;

struct IconTexture {
  ID3D11ShaderResourceView *srv = nullptr;
  int width = 0;
  int height = 0;
  bool attempted = false;
};
static IconTexture g_boltIcon;
static IconTexture g_discordPfp;

static char g_dumpSearch[96] = "";
static bool g_dumpShowMethods = true;
static bool g_dumpShowFields = true;
static int g_dumpRowsTarget = 180;
static bool g_dumpPinClass = false;
static std::vector<DumpDatabase::MethodEntry> g_dumpMethodCache;
static std::vector<DumpDatabase::FieldEntry> g_dumpFieldCache;
static std::string g_dumpSelectedKey;
static std::string g_dumpPinnedClass;
static int g_lastEspPreset = -1;
static int g_lastVisualPreset = -1;
static char g_tabSearch[48] = "";

struct UiToast {
  std::string text;
  ImU32 color = IM_COL32(90, 220, 165, 245);
  float bornAt = 0.f;
  float duration = 2.6f;
};
static std::vector<UiToast> g_uiToasts;

// Config values
float g_speed = 2.5f;
float g_fov = 90.f, g_zoom = 1.f, g_bloom = 0.5f;
float g_walkSpeed = 2.5f, g_animSpeed = 1.f, g_uiScale = 1.f;
float g_ping = 50.f, g_taskProg = 0.f, g_themeInt = 1.f, g_blurInt = 0.8f;
float g_killCd = 0.f, g_killDist = 1.0f;
bool g_fullbright = false, g_wireframe = false, g_smoothMove = true,
     g_wallhack = false;
bool g_espBox = true, g_espName = true, g_espDist = true, g_espRole = true;
bool g_espTracer = false, g_espOutline = true, g_espTask = false,
     g_espVent = true;
bool g_rainbow = false, g_spin = false, g_tiny = false, g_giant = false;
bool g_dance = false, g_particle = false, g_autoPath = false;
bool g_noclip = false, g_chatSpam = false;
bool g_devMode = false, g_fpsDisp = true, g_rgbAccent = false;
float g_accentCol[4] = {0, 0.86f, 1, 1};
char g_nameBuf[64] = "Stara";
float g_playerCol[4] = {0, 0.86f, 1, 1};
int g_hat = 0, g_pet = 0, g_skin = 0, g_trail = 0, g_emote = 0, g_rarity = 0;
// New feature globals
bool g_noKillCd = false, g_infiniteEmergencies = false,
     g_alwaysMoveable = false, g_impostorVision = false;
bool g_maxReportDist = false, g_autoTasks = false, g_freezeAll = false,
     g_colorCycle = false;
bool g_antiKick = false, g_forceProtect = false, g_spamAnim = false,
     g_godmode = false;
bool g_walkInVents = false, g_useVents = false, g_seeGhosts = false,
     g_alwaysChat = false;
bool g_killReach = false, g_killAnyone = false;
// Role-specific cheats
bool g_endlessVentTime = false, g_noVentCooldown = false;
bool g_endlessBattery = false, g_noVitalsCooldown = false;
bool g_endlessTracking = false, g_noTrackDelay = false,
     g_noTrackCooldown = false;
bool g_noSsAnimation = false, g_endlessSsDuration = false;
bool g_killWhileVanished = false;
bool g_unfixableLights = false;
// Tracers
bool g_tracerCrew = false, g_tracerImp = false, g_tracerGhost = false,
     g_tracerBodies = false, g_tracerColorBased = false;
// Camera
bool g_freecam = false, g_spectate = false, g_teleportToCursor = false;
int g_spectateTarget = -1;
float g_customDiscussTime = 15.f, g_customVoteTime = 120.f;
char g_chatBuf[128] = "Stara Client";
static float g_hue = 0.f;

// Ã¢â€¢ÂÃ¢â€¢ÂÃ¢â€¢ÂÃ¢â€¢ÂÃ¢â€¢ÂÃ¢â€¢ÂÃ¢â€¢Â Phase 1-9: MalumMenu Ported Feature Toggles Ã¢â€¢ÂÃ¢â€¢ÂÃ¢â€¢ÂÃ¢â€¢ÂÃ¢â€¢ÂÃ¢â€¢ÂÃ¢â€¢Â
// Phase 1: Vote Manipulation
bool g_revealVotes = false;
// Phase 2: Overload
static int g_overloadStrength = 10;
// Phase 3: Map-Aware Sabotage
bool g_spamDoors = false;
// Phase 4: Animations
bool g_moonwalk = false, g_medScan = false, g_camsInUse = false;
// Phase 5: Chat
bool g_alwaysChatEnabled = false; // re-uses g_alwaysChat internally
static float g_chatRateOverride = 1.5f;
// Phase 6: Event Logger
bool g_eventLogVisible = false;
// Phase 7: Panic / Stealth
bool g_panicMode = false;
bool g_stealthMode = false; // hides ESP/tracers temporarily
// Phase 8: Minimap ESP
bool g_minimapEsp = false;
static float g_minimapScale = 0.12f;
static float g_minimapX = 10.f, g_minimapY = 10.f;
// Phase 9: QoL
bool g_invertControls = false;
bool g_noGameEnd = false;
bool g_distanceTracers = false;
bool g_freeCosmetics = false;


static ImVec4 Accent() {
  if (g_rgbAccent)
    return ImGui::ColorConvertU32ToFloat4(IM_COL32(
        (int)(sin(g_hue) * 127 + 128), (int)(sin(g_hue + 2.1f) * 127 + 128),
        (int)(sin(g_hue + 4.2f) * 127 + 128), 255));
  return {g_accentCol[0], g_accentCol[1], g_accentCol[2], g_accentCol[3]};
}

static float Damp(float cur, float target, float speed) {
  float dt = ImGui::GetIO().DeltaTime;
  return cur + (target - cur) * std::min(1.f, dt * speed);
}

static ImU32 ScaledAccent(const ImVec4 &ac, float scale, int alpha) {
  int r = std::clamp((int)(ac.x * 255.f * scale), 0, 255);
  int g = std::clamp((int)(ac.y * 255.f * scale), 0, 255);
  int b = std::clamp((int)(ac.z * 255.f * scale), 0, 255);
  return IM_COL32(r, g, b, alpha);
}

static bool ContainsNoCase(const std::string &text, const std::string &needle) {
  if (needle.empty())
    return true;
  auto it = std::search(text.begin(), text.end(), needle.begin(), needle.end(),
                        [](char a, char b) {
                          return std::tolower((unsigned char)a) ==
                                 std::tolower((unsigned char)b);
                        });
  return it != text.end();
}

static bool EnsureComReadyForWic() {
  static bool tried = false;
  static bool ready = false;
  if (tried)
    return ready;
  tried = true;
  HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
  if (hr == RPC_E_CHANGED_MODE || SUCCEEDED(hr))
    ready = true;
  return ready;
}

static bool LoadDx11TextureFromPng(const wchar_t *path, IconTexture &outTex) {
  if (!path || !Hooks::GetDevice())
    return false;
  if (!EnsureComReadyForWic())
    return false;
  if (!std::filesystem::exists(path))
    return false;

  IWICImagingFactory *factory = nullptr;
  IWICBitmapDecoder *decoder = nullptr;
  IWICBitmapFrameDecode *frame = nullptr;
  IWICFormatConverter *converter = nullptr;
  ID3D11Texture2D *texture = nullptr;
  bool ok = false;

  do {
    HRESULT hr = CoCreateInstance(CLSID_WICImagingFactory, nullptr,
                                  CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&factory));
    if (FAILED(hr) || !factory)
      break;

    hr = factory->CreateDecoderFromFilename(
        path, nullptr, GENERIC_READ, WICDecodeMetadataCacheOnLoad, &decoder);
    if (FAILED(hr) || !decoder)
      break;

    hr = decoder->GetFrame(0, &frame);
    if (FAILED(hr) || !frame)
      break;

    UINT width = 0, height = 0;
    frame->GetSize(&width, &height);
    if (width == 0 || height == 0)
      break;

    hr = factory->CreateFormatConverter(&converter);
    if (FAILED(hr) || !converter)
      break;

    hr = converter->Initialize(frame, GUID_WICPixelFormat32bppRGBA,
                               WICBitmapDitherTypeNone, nullptr, 0.0,
                               WICBitmapPaletteTypeCustom);
    if (FAILED(hr))
      break;

    std::vector<unsigned char> pixels((size_t)width * (size_t)height * 4u);
    hr = converter->CopyPixels(nullptr, width * 4u, (UINT)pixels.size(),
                               pixels.data());
    if (FAILED(hr))
      break;

    D3D11_TEXTURE2D_DESC desc{};
    desc.Width = width;
    desc.Height = height;
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

    D3D11_SUBRESOURCE_DATA init{};
    init.pSysMem = pixels.data();
    init.SysMemPitch = width * 4u;

    hr = Hooks::GetDevice()->CreateTexture2D(&desc, &init, &texture);
    if (FAILED(hr) || !texture)
      break;

    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc{};
    srvDesc.Format = desc.Format;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = 1;

    hr = Hooks::GetDevice()->CreateShaderResourceView(texture, &srvDesc,
                                                      &outTex.srv);
    if (FAILED(hr) || !outTex.srv)
      break;

    outTex.width = (int)width;
    outTex.height = (int)height;
    ok = true;
  } while (false);

  if (texture)
    texture->Release();
  if (converter)
    converter->Release();
  if (frame)
    frame->Release();
  if (decoder)
    decoder->Release();
  if (factory)
    factory->Release();
  return ok;
}

static void EnsureBoltIconLoaded() {
  if (g_boltIcon.attempted)
    return;
  g_boltIcon.attempted = true;

  const wchar_t *paths[] = {
      L"assets\\icons\\bolt.png",
      L"assets\\bolt.png",
      L"bolt.png",
  };
  for (const wchar_t *p : paths) {
    if (LoadDx11TextureFromPng(p, g_boltIcon))
      return;
  }
}

static void EnsureDiscordPfpLoaded() {
  if (g_discordPfp.attempted)
    return;
  g_discordPfp.attempted = true;

  const wchar_t *paths[] = {
      L"assets\\icons\\discord_pfp.png",
      L"assets\\discord_pfp.png",
      L"discord_pfp.png",
  };
  for (const wchar_t *p : paths) {
    if (LoadDx11TextureFromPng(p, g_discordPfp))
      return;
  }
}

enum class MiniIcon { Discord, Game, Link, Database };

static void DrawMiniIcon(ImDrawList *dl, MiniIcon kind, ImVec2 center,
                         float size, ImU32 col, float thickness = 1.6f) {
  const float s = size * 0.5f;
  switch (kind) {
  case MiniIcon::Discord:
    dl->AddRect({center.x - s * 0.85f, center.y - s * 0.45f},
                {center.x + s * 0.85f, center.y + s * 0.45f}, col, 4.f, 0,
                thickness);
    dl->AddCircleFilled({center.x - s * 0.35f, center.y - s * 0.04f}, s * 0.08f,
                        col, 8);
    dl->AddCircleFilled({center.x + s * 0.35f, center.y - s * 0.04f}, s * 0.08f,
                        col, 8);
    dl->AddBezierCubic({center.x - s * 0.45f, center.y + s * 0.18f},
                       {center.x - s * 0.2f, center.y + s * 0.34f},
                       {center.x + s * 0.2f, center.y + s * 0.34f},
                       {center.x + s * 0.45f, center.y + s * 0.18f}, col,
                       thickness, 14);
    break;
  case MiniIcon::Game:
    dl->AddRect({center.x - s * 0.9f, center.y - s * 0.42f},
                {center.x + s * 0.9f, center.y + s * 0.42f}, col, 4.f, 0,
                thickness);
    dl->AddLine({center.x - s * 0.45f, center.y},
                {center.x - s * 0.2f, center.y}, col, thickness);
    dl->AddLine({center.x - s * 0.325f, center.y - s * 0.13f},
                {center.x - s * 0.325f, center.y + s * 0.13f}, col, thickness);
    dl->AddCircleFilled({center.x + s * 0.28f, center.y - s * 0.06f}, s * 0.08f,
                        col, 8);
    dl->AddCircleFilled({center.x + s * 0.48f, center.y + s * 0.1f}, s * 0.08f,
                        col, 8);
    break;
  case MiniIcon::Link:
    dl->AddCircle({center.x - s * 0.28f, center.y}, s * 0.28f, col, 16,
                  thickness);
    dl->AddCircle({center.x + s * 0.28f, center.y}, s * 0.28f, col, 16,
                  thickness);
    dl->AddLine({center.x - s * 0.02f, center.y - s * 0.16f},
                {center.x + s * 0.02f, center.y + s * 0.16f}, col, thickness);
    break;
  case MiniIcon::Database:
    dl->AddEllipse({center.x, center.y - s * 0.26f}, {s * 0.82f, s * 0.34f},
                   col, 0.f, 18, thickness);
    dl->AddLine({center.x - s * 0.82f, center.y - s * 0.26f},
                {center.x - s * 0.82f, center.y + s * 0.42f}, col, thickness);
    dl->AddLine({center.x + s * 0.82f, center.y - s * 0.26f},
                {center.x + s * 0.82f, center.y + s * 0.42f}, col, thickness);
    dl->AddEllipse({center.x, center.y + s * 0.42f}, {s * 0.82f, s * 0.34f},
                   col, 0.f, 18, thickness);
    break;
  }
}

static void DrawBoltFallback(ImDrawList *dl, ImVec2 center, float size,
                             ImU32 col) {
  float s = size * 0.5f;
  ImVec2 pts[] = {
      {center.x - s * 0.15f, center.y - s * 0.95f},
      {center.x + s * 0.08f, center.y - s * 0.95f},
      {center.x - s * 0.05f, center.y - s * 0.1f},
      {center.x + s * 0.26f, center.y - s * 0.1f},
      {center.x - s * 0.12f, center.y + s * 0.95f},
      {center.x - s * 0.02f, center.y + s * 0.22f},
      {center.x - s * 0.33f, center.y + s * 0.22f},
  };
  dl->AddConvexPolyFilled(pts, (int)std::size(pts), col);
}

static void EnsureDumpCache() {
  static std::string lastPath;
  static size_t lastMethods = 0;
  static size_t lastFields = 0;
  if (!DumpDatabase::IsLoaded()) {
    g_dumpMethodCache.clear();
    g_dumpFieldCache.clear();
    lastPath.clear();
    lastMethods = 0;
    lastFields = 0;
    return;
  }

  const std::string &path = DumpDatabase::LoadedPath();
  size_t mCount = DumpDatabase::MethodCount();
  size_t fCount = DumpDatabase::FieldCount();
  if (path == lastPath && mCount == lastMethods && fCount == lastFields &&
      !g_dumpMethodCache.empty()) {
    return;
  }

  g_dumpMethodCache = DumpDatabase::GetMethodEntries();
  g_dumpFieldCache = DumpDatabase::GetFieldEntries();
  lastPath = path;
  lastMethods = mCount;
  lastFields = fCount;
}

static const char *TabTag(Tab t) {
  switch (t) {
  case Tab::Dashboard:
    return "DB";
  case Tab::Player:
    return "PL";
  case Tab::Visuals:
    return "VS";
  case Tab::ESP:
    return "ES";
  case Tab::Movement:
    return "MV";
  case Tab::Fun:
    return "FN";
  case Tab::Troll:
    return "TR";
  case Tab::Cosmetics:
    return "CS";
  case Tab::Settings:
    return "ST";
  default:
    return "--";
  }
}

static const char *TabSubtitle(Tab t) {
  switch (t) {
  case Tab::Dashboard:
    return "Session overview, quick actions, and dump explorer";
  case Tab::Player:
    return "Identity, role setup, combat and lobby controls";
  case Tab::Visuals:
    return "Rendering style, camera tuning, visual presets";
  case Tab::ESP:
    return "Player overlays, role markers, and diagnostics";
  case Tab::Movement:
    return "Speed, pathing, and teleport utilities";
  case Tab::Fun:
    return "Character effects, emotes, and flavor controls";
  case Tab::Troll:
    return "Advanced lobby actions and role distribution";
  case Tab::Cosmetics:
    return "Hat, pet, skin, visor, and color customization";
  case Tab::Settings:
    return "Theme, config, and client behavior preferences";
  default:
    return "";
  }
}

static const char *TabSidebarHint(Tab t) {
  switch (t) {
  case Tab::Dashboard:
    return "overview";
  case Tab::Player:
    return "identity";
  case Tab::Visuals:
    return "rendering";
  case Tab::ESP:
    return "overlay";
  case Tab::Movement:
    return "mobility";
  case Tab::Fun:
    return "effects";
  case Tab::Troll:
    return "advanced";
  case Tab::Cosmetics:
    return "style";
  case Tab::Settings:
    return "config";
  default:
    return "";
  }
}

static ImU32 TabTone(Tab t, int alpha = 255) {
  switch (t) {
  case Tab::Dashboard:
    return IM_COL32(86, 176, 255, alpha);
  case Tab::Player:
    return IM_COL32(90, 220, 188, alpha);
  case Tab::Visuals:
    return IM_COL32(120, 196, 255, alpha);
  case Tab::ESP:
    return IM_COL32(72, 232, 255, alpha);
  case Tab::Movement:
    return IM_COL32(120, 220, 140, alpha);
  case Tab::Fun:
    return IM_COL32(236, 170, 110, alpha);
  case Tab::Troll:
    return IM_COL32(255, 128, 146, alpha);
  case Tab::Cosmetics:
    return IM_COL32(202, 162, 255, alpha);
  case Tab::Settings:
    return IM_COL32(255, 220, 120, alpha);
  default:
    return IM_COL32(160, 190, 225, alpha);
  }
}

static void DrawTabGlyph(ImDrawList *dl, Tab t, ImVec2 center, float size,
                         ImU32 col, float thickness = 1.35f) {
  const float s = size * 0.5f;
  switch (t) {
  case Tab::Dashboard:
    for (int y = 0; y < 2; y++) {
      for (int x = 0; x < 2; x++) {
        float ox = (x ? 0.22f : -0.22f) * size;
        float oy = (y ? 0.22f : -0.22f) * size;
        dl->AddRect({center.x + ox - s * 0.25f, center.y + oy - s * 0.25f},
                    {center.x + ox + s * 0.25f, center.y + oy + s * 0.25f}, col,
                    2.f, 0, thickness);
      }
    }
    break;
  case Tab::Player:
    dl->AddCircle(center, s * 0.34f, col, 18, thickness);
    dl->AddLine({center.x, center.y + s * 0.34f},
                {center.x, center.y + s * 0.9f}, col, thickness);
    dl->AddLine({center.x - s * 0.5f, center.y + s * 0.62f},
                {center.x + s * 0.5f, center.y + s * 0.62f}, col, thickness);
    break;
  case Tab::Visuals:
    dl->AddEllipse(center, {s * 0.88f, s * 0.5f}, col, 0.f, 24, thickness);
    dl->AddCircleFilled(center, s * 0.2f, col, 12);
    break;
  case Tab::ESP:
    dl->AddCircle(center, s * 0.58f, col, 22, thickness);
    dl->AddLine({center.x - s * 0.8f, center.y},
                {center.x - s * 0.32f, center.y}, col, thickness);
    dl->AddLine({center.x + s * 0.32f, center.y},
                {center.x + s * 0.8f, center.y}, col, thickness);
    dl->AddLine({center.x, center.y - s * 0.8f},
                {center.x, center.y - s * 0.32f}, col, thickness);
    dl->AddLine({center.x, center.y + s * 0.32f},
                {center.x, center.y + s * 0.8f}, col, thickness);
    break;
  case Tab::Movement: {
    ImVec2 pts[4] = {{center.x - s * 0.85f, center.y + s * 0.65f},
                     {center.x - s * 0.24f, center.y + s * 0.08f},
                     {center.x + s * 0.28f, center.y + s * 0.28f},
                     {center.x + s * 0.86f, center.y - s * 0.46f}};
    dl->AddPolyline(pts, 4, col, 0, thickness);
    dl->AddTriangleFilled({center.x + s * 0.86f, center.y - s * 0.46f},
                          {center.x + s * 0.56f, center.y - s * 0.42f},
                          {center.x + s * 0.75f, center.y - s * 0.16f}, col);
    break;
  }
  case Tab::Fun:
    dl->AddLine({center.x - s * 0.7f, center.y},
                {center.x + s * 0.7f, center.y}, col, thickness);
    dl->AddLine({center.x, center.y - s * 0.7f},
                {center.x, center.y + s * 0.7f}, col, thickness);
    dl->AddLine({center.x - s * 0.48f, center.y - s * 0.48f},
                {center.x + s * 0.48f, center.y + s * 0.48f}, col, thickness);
    dl->AddLine({center.x - s * 0.48f, center.y + s * 0.48f},
                {center.x + s * 0.48f, center.y - s * 0.48f}, col, thickness);
    break;
  case Tab::Troll:
    dl->AddRect({center.x - s * 0.75f, center.y - s * 0.62f},
                {center.x + s * 0.75f, center.y + s * 0.62f}, col, 4.f, 0,
                thickness);
    dl->AddCircleFilled({center.x - s * 0.28f, center.y - s * 0.08f}, s * 0.09f,
                        col, 8);
    dl->AddCircleFilled({center.x + s * 0.28f, center.y - s * 0.08f}, s * 0.09f,
                        col, 8);
    dl->AddBezierCubic({center.x - s * 0.46f, center.y + s * 0.24f},
                       {center.x - s * 0.2f, center.y + s * 0.42f},
                       {center.x + s * 0.2f, center.y + s * 0.42f},
                       {center.x + s * 0.46f, center.y + s * 0.24f}, col,
                       thickness, 12);
    break;
  case Tab::Cosmetics:
    dl->AddRect({center.x - s * 0.66f, center.y - s * 0.4f},
                {center.x + s * 0.24f, center.y + s * 0.35f}, col, 2.f, 0,
                thickness);
    dl->AddTriangleFilled({center.x + s * 0.24f, center.y - s * 0.28f},
                          {center.x + s * 0.92f, center.y + s * 0.02f},
                          {center.x + s * 0.24f, center.y + s * 0.3f}, col);
    break;
  case Tab::Settings:
    dl->AddCircle(center, s * 0.54f, col, 20, thickness);
    for (int k = 0; k < 6; k++) {
      float a = k * (6.283185f / 6.f);
      float cx = center.x + cosf(a) * s * 0.8f;
      float cy = center.y + sinf(a) * s * 0.8f;
      dl->AddCircleFilled({cx, cy}, s * 0.1f, col, 8);
    }
    dl->AddCircleFilled(center, s * 0.18f, col, 10);
    break;
  default:
    dl->AddCircle(center, s * 0.5f, col, 16, thickness);
    break;
  }
}

static void PushToast(const std::string &text,
                      ImU32 color = IM_COL32(90, 220, 165, 245),
                      float duration = 2.6f) {
  UiToast t;
  t.text = text;
  t.color = color;
  t.bornAt = (float)ImGui::GetTime();
  t.duration = std::max(1.0f, duration);
  g_uiToasts.push_back(std::move(t));
  if (g_uiToasts.size() > 8)
    g_uiToasts.erase(g_uiToasts.begin(),
                     g_uiToasts.begin() + (g_uiToasts.size() - 8));
}

static void DrawToasts(float menuAlpha) {
  if (g_uiToasts.empty())
    return;
  ImDrawList *dl = ImGui::GetForegroundDrawList();
  ImVec2 screen = ImGui::GetIO().DisplaySize;
  float now = (float)ImGui::GetTime();
  float y = 18.f;

  for (size_t i = 0; i < g_uiToasts.size();) {
    UiToast &t = g_uiToasts[i];
    float age = now - t.bornAt;
    if (age >= t.duration) {
      g_uiToasts.erase(g_uiToasts.begin() + (long long)i);
      continue;
    }

    float fadeIn = std::clamp(age / 0.22f, 0.f, 1.f);
    float fadeOut = std::clamp((t.duration - age) / 0.35f, 0.f, 1.f);
    float alpha = std::clamp(fadeIn * fadeOut * menuAlpha, 0.f, 1.f);
    ImVec2 ts = ImGui::CalcTextSize(t.text.c_str());
    float w = ts.x + 26.f;
    float h = std::max(28.f, ts.y + 12.f);
    float x = screen.x - w - 20.f + (1.f - fadeIn) * 14.f;

    ImU32 bg = IM_COL32(9, 14, 24, (int)(220 * alpha));
    ImU32 bd = IM_COL32((int)(((t.color >> IM_COL32_R_SHIFT) & 0xFF)),
                        (int)(((t.color >> IM_COL32_G_SHIFT) & 0xFF)),
                        (int)(((t.color >> IM_COL32_B_SHIFT) & 0xFF)),
                        (int)(190 * alpha));
    dl->AddRectFilled({x, y}, {x + w, y + h}, bg, 8.f);
    dl->AddRect({x, y}, {x + w, y + h}, bd, 8.f, 0, 1.2f);
    dl->AddRectFilled({x, y}, {x + 3.5f, y + h}, bd, 8.f,
                      ImDrawFlags_RoundCornersLeft);
    dl->AddText({x + 12.f, y + (h - ts.y) * 0.5f},
                IM_COL32(238, 246, 255, (int)(255 * alpha)), t.text.c_str());

    y += h + 8.f;
    ++i;
  }
}

static float DrawStatusChip(ImDrawList *dl, ImVec2 pos, const char *text,
                            ImU32 textCol, ImU32 bgCol, ImU32 borderCol) {
  ImVec2 pad = {9.f, 4.f};
  ImVec2 sz = ImGui::CalcTextSize(text);
  ImVec2 min = pos;
  ImVec2 max = {pos.x + sz.x + pad.x * 2.f, pos.y + sz.y + pad.y * 2.f};
  dl->AddRectFilled(min, max, bgCol, 6.f);
  dl->AddRect(min, max, borderCol, 6.f, 0, 1.f);
  dl->AddText({min.x + pad.x, min.y + pad.y}, textCol, text);
  return max.x - min.x;
}

// Minimal background Ã¢â‚¬â€ clean gradient + subtle accent wash
static void DrawCleanBackground(ImDrawList *dl, const ImVec2 &screen,
                                float menuAlpha, const ImVec4 &accent) {
  ImU32 topL = IM_COL32(8, 12, 22, (int)(240 * menuAlpha));
  ImU32 topR = IM_COL32(10, 15, 28, (int)(240 * menuAlpha));
  ImU32 botR = IM_COL32(4, 6, 14, (int)(255 * menuAlpha));
  ImU32 botL = IM_COL32(6, 8, 18, (int)(255 * menuAlpha));
  dl->AddRectFilledMultiColor({0, 0}, screen, topL, topR, botR, botL);
  // Subtle accent wash in corner
  dl->AddCircleFilled(
      {screen.x * 0.15f, screen.y * 0.2f}, screen.y * 0.18f,
      IM_COL32((int)(accent.x * 40), (int)(accent.y * 50),
               (int)(accent.z * 70), (int)(18 * menuAlpha)),
      48);
  dl->AddCircleFilled(
      {screen.x * 0.85f, screen.y * 0.75f}, screen.y * 0.22f,
      IM_COL32(8, 18, 35, (int)(22 * menuAlpha)), 48);
}

static float Randf(float minV, float maxV) {
  return minV + (maxV - minV) * ((float)rand() / (float)RAND_MAX);
}


// ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ Custom Widgets
// ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬
static bool Toggle(const char *label, bool *v) {
  ImGuiWindow *w = ImGui::GetCurrentWindow();
  if (w->SkipItems)
    return false;
  ImGuiID id = w->GetID(label);
  const float h = 20.f, wd = 38.f, r = h * .5f;
  const float knobR = r - 2.f;
  ImVec2 p = w->DC.CursorPos;
  ImVec2 ls = ImGui::CalcTextSize(label);
  ImRect bb(p, ImVec2(p.x + wd + 10 + ls.x, p.y + std::max(h, ls.y)));
  ImGui::ItemSize(bb);
  if (!ImGui::ItemAdd(bb, id))
    return false;
  bool hov, held;
  bool press = ImGui::ButtonBehavior(bb, id, &hov, &held);
  if (press)
    *v = !*v;
  float &ta = g_toggleAnim[id];
  float tgt = *v ? 1.f : 0.f;
  float speed = ImGui::GetIO().DeltaTime * 10.f;
  float diff = tgt - ta;
  ta += diff * std::min(1.f, speed) * (1.f + (1.f - powf(1.f - fabsf(diff), 3.f)) * 2.f);
  if (fabsf(diff) < 0.005f) ta = tgt;
  ImDrawList *dl = w->DrawList;
  ImVec4 ac = Accent();
  ImU32 offBg = IM_COL32(30, 35, 50, 200);
  ImU32 onBg = IM_COL32((int)(ac.x*200), (int)(ac.y*200), (int)(ac.z*200), 210);
  ImU32 bg = ImGui::ColorConvertFloat4ToU32(
      ImLerp(ImGui::ColorConvertU32ToFloat4(offBg),
             ImGui::ColorConvertU32ToFloat4(onBg), ta));
  dl->AddRectFilled(p, {p.x + wd, p.y + h}, bg, r);
  dl->AddRectFilled(p, {p.x + wd, p.y + 3.f},
                    IM_COL32(0, 0, 0, (int)(40*(1.f-ta))), r,
                    ImDrawFlags_RoundCornersTop);
  float kx = p.x + r + (wd - h) * ta;
  float ky = p.y + r;
  if (ta > 0.02f) {
    for (int ring = 3; ring > 0; ring--) {
      float grow = ring * 1.8f;
      int a = (int)(28 * ta * (4 - ring));
      dl->AddCircleFilled({kx, ky}, knobR + grow,
                          IM_COL32((int)(ac.x*255),(int)(ac.y*255),
                                   (int)(ac.z*255), a), 24);
    }
  }
  dl->AddCircleFilled({kx, ky + 0.5f}, knobR, IM_COL32(0,0,0,35), 24);
  dl->AddCircleFilled({kx, ky}, knobR,
                      hov ? IM_COL32(255,255,255,255) : IM_COL32(240,243,248,245), 24);
  ImU32 textCol = hov ? IM_COL32(235,240,250,255) : IM_COL32(180,190,210,230);
  if (*v) textCol = IM_COL32(226,232,240,255);
  dl->AddText({p.x + wd + 10, p.y + (h - ls.y) * .5f}, textCol, label);
  return press;
}

static bool GlowBtn(const char *label, ImVec2 sz = {0, 0}) {
  ImGuiWindow *w = ImGui::GetCurrentWindow();
  if (w->SkipItems)
    return false;
  ImGuiID id = w->GetID(label);
  float &ha = g_buttonAnim[id];
  ImVec2 ls = ImGui::CalcTextSize(label);
  if (sz.x <= 0) sz.x = ls.x + 24.f;
  if (sz.y <= 0) sz.y = 28.f;
  ImVec2 p = w->DC.CursorPos;
  ImRect bb(p, {p.x + sz.x, p.y + sz.y});
  ImGui::ItemSize(bb);
  if (!ImGui::ItemAdd(bb, id))
    return false;
  bool hov, held;
  bool press = ImGui::ButtonBehavior(bb, id, &hov, &held);
  ha = Damp(ha, held ? 1.3f : (hov ? 1.f : 0.f), 16.f);
  ImDrawList *dl = w->DrawList;
  ImVec4 ac = Accent();
  float rnd = sz.y * 0.38f;
  if (ha > 0.01f) {
    for (int i = 2; i > 0; i--) {
      float grow = i * 2.2f;
      int a = (int)(22 * ha * (3 - i));
      dl->AddRect({p.x-grow, p.y-grow}, {p.x+sz.x+grow, p.y+sz.y+grow},
                  IM_COL32((int)(ac.x*255),(int)(ac.y*255),(int)(ac.z*255), a),
                  rnd + grow*0.3f, 0, 1.f);
    }
  }
  ImU32 topCol = IM_COL32((int)(ac.x*48+14),(int)(ac.y*48+14),
                          (int)(ac.z*54+16), 210+(int)(45*ha));
  ImU32 botCol = IM_COL32((int)(ac.x*28+8),(int)(ac.y*28+8),
                          (int)(ac.z*32+10), 200+(int)(55*ha));
  dl->AddRectFilledMultiColor(p, {p.x+sz.x,p.y+sz.y}, topCol, topCol, botCol, botCol);
  dl->AddRectFilledMultiColor(
      {p.x+4.f,p.y}, {p.x+sz.x-4.f,p.y+1.f},
      IM_COL32(255,255,255,(int)(18+22*ha)), IM_COL32(255,255,255,(int)(8+12*ha)),
      IM_COL32(255,255,255,0), IM_COL32(255,255,255,0));
  dl->AddRect(p, {p.x+sz.x,p.y+sz.y},
              IM_COL32((int)(ac.x*120+30),(int)(ac.y*120+30),
                       (int)(ac.z*130+35), 80+(int)(60*ha)),
              rnd, 0, 0.8f);
  ImVec2 tp = {p.x+(sz.x-ls.x)*0.5f, p.y+(sz.y-ls.y)*0.5f};
  dl->AddText(tp, IM_COL32(226,232,240,230+(int)(25*ha)), label);
  return press;
}

static bool Slider(const char *l, float *v, float mn, float mx,
                   const char *fmt = "%.1f") {
  ImVec4 ac = Accent();
  ImGui::PushStyleColor(ImGuiCol_SliderGrab, {ac.x*.9f, ac.y*.9f, ac.z*.9f, 1.f});
  ImGui::PushStyleColor(ImGuiCol_SliderGrabActive, {ac.x*1.15f, ac.y*1.15f, ac.z*1.15f, 1.f});
  ImGui::PushStyleColor(ImGuiCol_FrameBg, {.06f,.07f,.11f,.75f});
  ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, {.08f,.09f,.14f,.85f});
  ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6.f);
  ImGui::PushStyleVar(ImGuiStyleVar_GrabRounding, 10.f);
  ImGui::PushStyleVar(ImGuiStyleVar_GrabMinSize, 14.f);
  bool c = ImGui::SliderFloat(l, v, mn, mx, fmt);
  ImGui::PopStyleVar(3);
  ImGui::PopStyleColor(4);
  return c;
}

static void Card(const char *title) {
  ImGui::PushStyleColor(ImGuiCol_ChildBg, {.042f,.055f,.09f,.78f});
  ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 10.f);
  ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 0.8f);
  ImGui::BeginChild(title, {0, 0},
                    ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_Borders |
                        ImGuiChildFlags_AlwaysUseWindowPadding);
  ImDrawList *dl = ImGui::GetWindowDrawList();
  ImVec2 p = ImGui::GetWindowPos(), s = ImGui::GetWindowSize();
  ImVec4 ac = Accent();
  dl->AddRectFilled(p, {p.x+s.x,p.y+s.y}, IM_COL32(6,10,18,60), 10.f);
  dl->AddRect(p, {p.x+s.x,p.y+s.y}, IM_COL32(40,52,75,70), 10.f, 0, 0.7f);
  dl->AddRectFilledMultiColor(
      p, {p.x+s.x, p.y+2.f},
      IM_COL32((int)(ac.x*200),(int)(ac.y*200),(int)(ac.z*200),120),
      IM_COL32((int)(ac.x*100),(int)(ac.y*100),(int)(ac.z*120),40),
      IM_COL32(0,0,0,0), IM_COL32(0,0,0,0));
  ImGui::TextColored({.88f,.91f,.96f,1.f}, "%s", title);
  ImGui::Dummy({0, 1});
  float sepY = ImGui::GetCursorScreenPos().y - 1.f;
  dl->AddRectFilledMultiColor(
      {p.x+10.f,sepY}, {p.x+s.x-10.f,sepY+1.f},
      IM_COL32(60,75,100,80), IM_COL32(40,52,70,30),
      IM_COL32(40,52,70,30), IM_COL32(60,75,100,80));
  ImGui::Dummy({0, 4});
}
static void EndCard() {
  ImGui::EndChild();
  ImGui::PopStyleVar(2);
  ImGui::PopStyleColor();
  ImGui::Dummy({0, 4});
}



static void Sep(const char *t) {
  ImGui::Dummy({0, 2});
  ImGui::TextColored({.52f, .58f, .72f, 1}, "-- %s --", t);
  ImGui::Dummy({0, 3});
}

static void DrawDiscordProfileCard() {
  Card("Discord Corner");
  ImDrawList *dl = ImGui::GetWindowDrawList();
  ImVec2 p = ImGui::GetCursorScreenPos();
  float w = ImGui::GetContentRegionAvail().x;
  float h = 126.f;
  ImVec4 ac = Accent();

  dl->AddRectFilled(p, {p.x + w, p.y + h}, IM_COL32(10, 16, 28, 205), 10.f);
  dl->AddRect(p, {p.x + w, p.y + h}, IM_COL32(64, 94, 138, 140), 10.f, 0, 1.f);

  ImVec2 avatar = {p.x + 34.f, p.y + 34.f};
  dl->AddCircleFilled(avatar, 22.f, IM_COL32(20, 36, 58, 240), 24);
  dl->AddCircle(avatar, 22.f, ScaledAccent(ac, 1.05f, 185), 24, 2.f);
  EnsureDiscordPfpLoaded();
  if (g_discordPfp.srv) {
    dl->AddImageRounded((ImTextureID)g_discordPfp.srv,
                        {avatar.x - 17.f, avatar.y - 17.f},
                        {avatar.x + 17.f, avatar.y + 17.f}, {0, 0}, {1, 1},
                        IM_COL32(255, 255, 255, 255), 17.f);
  } else {
    DrawMiniIcon(dl, MiniIcon::Discord, avatar, 18.f,
                 IM_COL32(208, 232, 255, 245), 1.35f);
  }

  ImGui::SetCursorScreenPos({p.x + 68.f, p.y + 10.f});
  ImGui::TextColored({0.82f, 0.9f, 1.f, 1.f}, "@stara.client");
  ImGui::TextColored({0.52f, 0.72f, 1.f, 1.f}, "Discord Profile");

  ImGui::SetCursorScreenPos({p.x + 68.f, p.y + 50.f});
  DrawMiniIcon(dl, MiniIcon::Game, {p.x + 76.f, p.y + 58.f}, 14.f,
               IM_COL32(170, 220, 255, 220), 1.2f);
  ImGui::TextColored({0.92f, 0.95f, 1.f, 1.f}, "Game Now: %s",
                     Game::isInGame ? "In Match" : (Game::isInLobby ? "In Lobby" : "Idle"));

  ImGui::SetCursorScreenPos({p.x + 68.f, p.y + 74.f});
  DrawMiniIcon(dl, MiniIcon::Database, {p.x + 76.f, p.y + 82.f}, 14.f,
               IM_COL32(170, 220, 255, 220), 1.2f);
  ImGui::TextColored({0.88f, 0.92f, 1.f, 1.f}, "Dump Index: %zu",
                     DumpDatabase::MethodCount() + DumpDatabase::FieldCount());

  ImGui::SetCursorScreenPos({p.x + 12.f, p.y + h - 32.f});
  if (GlowBtn("Open Discord Link", {144, 24})) {
    ShellExecuteA(nullptr, "open", "https://discord.com/", nullptr, nullptr,
                  SW_SHOWNORMAL);
  }

  ImGui::SetCursorScreenPos({p.x + w - 38.f, p.y + h - 32.f});
  ImGui::PushID("discord_bolt_settings");
  bool boltPressed = ImGui::InvisibleButton("##boltSettings", {26, 24});
  bool boltHover = ImGui::IsItemHovered();
  ImGui::PopID();
  ImRect bb(ImGui::GetItemRectMin(), ImGui::GetItemRectMax());
  dl->AddRectFilled(
      bb.Min, bb.Max,
      boltHover ? IM_COL32(20, 34, 52, 250) : IM_COL32(14, 24, 38, 220), 6.f);
  dl->AddRect(bb.Min, bb.Max, IM_COL32(80, 120, 170, boltHover ? 220 : 150),
              6.f, 0, 1.f);
  EnsureBoltIconLoaded();
  if (g_boltIcon.srv) {
    dl->AddImage((ImTextureID)g_boltIcon.srv, {bb.Min.x + 5.f, bb.Min.y + 3.f},
                 {bb.Max.x - 5.f, bb.Max.y - 3.f});
  } else {
    DrawBoltFallback(
        dl, {(bb.Min.x + bb.Max.x) * 0.5f, (bb.Min.y + bb.Max.y) * 0.5f}, 14.f,
        IM_COL32(225, 242, 255, 240));
  }
  if (boltPressed)
    g_tab = Tab::Settings;

  ImGui::Dummy({w, h});
  EndCard();
}

static void DrawDumpExplorerCard() {
  Card("Dump.cs Explorer (125+ Entries)");
  EnsureDumpCache();
  bool dumpLoaded = DumpDatabase::IsLoaded();
  if (!dumpLoaded) {
    ImGui::TextColored({1.f, 0.72f, 0.35f, 1.f},
                       "dump.cs is not loaded yet. Set STARA_DUMP_PATH or "
                       "place dump.cs nearby.");
    EndCard();
    return;
  }

  ImGui::TextWrapped("Source: %s", DumpDatabase::LoadedPath().c_str());
  if (GlowBtn("Refresh Cache", {130, 24})) {
    g_dumpMethodCache = DumpDatabase::GetMethodEntries();
    g_dumpFieldCache = DumpDatabase::GetFieldEntries();
  }
  ImGui::SameLine();
  ImGui::SetNextItemWidth(260);
  ImGui::InputTextWithHint("##dumpSearchBox", "Search class/member",
                           g_dumpSearch, sizeof(g_dumpSearch));

  Toggle("Methods##dumpToggle", &g_dumpShowMethods);
  ImGui::SameLine();
  Toggle("Fields##dumpToggle", &g_dumpShowFields);
  ImGui::SameLine();
  Toggle("Pin Class##dumpPinClass", &g_dumpPinClass);
  ImGui::SetNextItemWidth(240);
  ImGui::SliderInt("Rows Target", &g_dumpRowsTarget, 125, 1500, "%d");
  if (g_dumpPinClass && !g_dumpPinnedClass.empty()) {
    ImGui::TextColored({0.62f, 0.86f, 1.f, 1.f}, "Pinned Class: %s",
                       g_dumpPinnedClass.c_str());
    ImGui::SameLine();
    if (GlowBtn("Clear Pin", {90, 22}))
      g_dumpPinnedClass.clear();
  }

  std::string query = g_dumpSearch;
  int shown = 0;
  int matched = 0;
  int typeCount = 0;
  int actionId = 1;

  ImGui::BeginChild("##dumpExplorerRows", {0, 280}, true,
                    ImGuiWindowFlags_HorizontalScrollbar);
  if (ImGui::BeginTable("DumpExplorerTable", 5,
                        ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
                            ImGuiTableFlags_ScrollY |
                            ImGuiTableFlags_SizingStretchProp)) {
    ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_WidthFixed, 64.f);
    ImGui::TableSetupColumn("Class");
    ImGui::TableSetupColumn("Member");
    ImGui::TableSetupColumn("RVA / Offset", ImGuiTableColumnFlags_WidthFixed,
                            110.f);
    ImGui::TableSetupColumn("Use", ImGuiTableColumnFlags_WidthFixed, 62.f);
    ImGui::TableHeadersRow();

    if (g_dumpShowMethods) {
      for (const auto &m : g_dumpMethodCache) {
        if (g_dumpPinClass && !g_dumpPinnedClass.empty() &&
            m.className != g_dumpPinnedClass)
          continue;
        bool hit = ContainsNoCase(m.className, query) ||
                   ContainsNoCase(m.methodName, query);
        if (!hit)
          continue;
        matched++;
        if (shown >= g_dumpRowsTarget)
          continue;

        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::TextColored({0.36f, 0.9f, 1.f, 1.f}, "Method");
        ImGui::TableNextColumn();
        ImGui::TextUnformatted(m.className.c_str());
        ImGui::TableNextColumn();
        ImGui::TextUnformatted(m.methodName.c_str());
        ImGui::TableNextColumn();
        char buf[32];
        snprintf(buf, sizeof(buf), "0x%llX", (unsigned long long)m.rva);
        ImGui::TextUnformatted(buf);
        ImGui::TableNextColumn();
        ImGui::PushID(actionId++);
        if (ImGui::SmallButton("Use")) {
          g_dumpSelectedKey = m.className + "::" + m.methodName;
          g_dumpPinnedClass = m.className;
        }
        ImGui::PopID();
        shown++;
      }
    }

    if (g_dumpShowFields) {
      for (const auto &f : g_dumpFieldCache) {
        if (g_dumpPinClass && !g_dumpPinnedClass.empty() &&
            f.className != g_dumpPinnedClass)
          continue;
        bool hit = ContainsNoCase(f.className, query) ||
                   ContainsNoCase(f.fieldName, query);
        if (!hit)
          continue;
        matched++;
        if (shown >= g_dumpRowsTarget)
          continue;

        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::TextColored({1.f, 0.82f, 0.42f, 1.f}, "Field");
        ImGui::TableNextColumn();
        ImGui::TextUnformatted(f.className.c_str());
        ImGui::TableNextColumn();
        ImGui::TextUnformatted(f.fieldName.c_str());
        ImGui::TableNextColumn();
        char buf[32];
        snprintf(buf, sizeof(buf), "+0x%X", f.offset);
        ImGui::TextUnformatted(buf);
        ImGui::TableNextColumn();
        ImGui::PushID(actionId++);
        if (ImGui::SmallButton("Use")) {
          g_dumpSelectedKey = f.className + "::" + f.fieldName;
          g_dumpPinnedClass = f.className;
        }
        ImGui::PopID();
        shown++;
      }
    }

    ImGui::EndTable();
  }
  ImGui::EndChild();

  typeCount = (int)DumpDatabase::ClassCount();
  ImGui::Text("Showing %d / %d matched entries | %d types indexed", shown,
              matched, typeCount);
  if (matched >= 125) {
    ImGui::TextColored({0.32f, 1.f, 0.56f, 1.f},
                       "Requirement hit: 125+ dump-linked entries available.");
  } else {
    ImGui::TextColored({1.f, 0.62f, 0.35f, 1.f},
                       "Fewer than 125 matched right now. Clear search or load "
                       "a fuller dump.");
  }
  if (!g_dumpSelectedKey.empty()) {
    ImGui::TextWrapped("Selected Entry: %s", g_dumpSelectedKey.c_str());
    if (GlowBtn("Copy Selected Key", {150, 24}))
      ImGui::SetClipboardText(g_dumpSelectedKey.c_str());
    ImGui::SameLine();
    if (GlowBtn("Filter to Selection", {150, 24}))
      snprintf(g_dumpSearch, sizeof(g_dumpSearch), "%s",
               g_dumpSelectedKey.c_str());
    ImGui::SameLine();
    if (GlowBtn("Pin Selected Class", {150, 24})) {
      size_t sep = g_dumpSelectedKey.find("::");
      if (sep != std::string::npos) {
        g_dumpPinnedClass = g_dumpSelectedKey.substr(0, sep);
        g_dumpPinClass = true;
      }
    }
  }
  EndCard();
}

// ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ Tab Renderers
// ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬
static void ApplyEspPreset(int presetId) {
  uint32_t x = 0x9E3779B9u * (uint32_t)(presetId + 1);
  x ^= (x << 13);
  x ^= (x >> 17);
  x ^= (x << 5);

  g_espBox = (x & (1u << 0)) != 0;
  g_espName = (x & (1u << 1)) != 0;
  g_espDist = (x & (1u << 2)) != 0;
  g_espRole = (x & (1u << 3)) != 0;
  g_espTracer = (x & (1u << 4)) != 0;
  g_espOutline = (x & (1u << 5)) != 0;
  g_espTask = (x & (1u << 6)) != 0;

  if (!g_espBox && !g_espName && !g_espDist && !g_espRole)
    g_espBox = true;

  g_killDist = 1.f + (float)(x % 50u);
  g_speed = 0.75f + (float)((x >> 9) % 800u) / 100.f;
  g_walkSpeed = 0.75f + (float)((x >> 17) % 800u) / 100.f;
  g_lastEspPreset = presetId;
}

static void ApplyVisualPreset(int presetId) {
  uint32_t x = 0x85EBCA6Bu * (uint32_t)(presetId + 3);
  x ^= (x << 11);
  x ^= (x >> 7);
  x ^= (x << 3);

  g_fullbright = (x & (1u << 0)) != 0;
  g_wallhack = (x & (1u << 1)) != 0;
  g_rgbAccent = (x & (1u << 2)) != 0;
  g_wireframe = (x & (1u << 3)) != 0;
  g_smoothMove = (x & (1u << 4)) != 0;

  g_fov = 55.f + (float)(x % 95u);
  g_zoom = 0.6f + (float)((x >> 8) % 190u) / 100.f;
  g_bloom = (float)((x >> 12) % 200u) / 100.f;
  g_themeInt = 0.3f + (float)((x >> 16) % 180u) / 100.f;
  g_blurInt = (float)((x >> 21) % 101u) / 100.f;
  g_uiScale = 0.8f + (float)((x >> 24) % 80u) / 100.f;

  g_accentCol[0] = 0.2f + (float)((x >> 3) & 0x1F) / 40.f;
  g_accentCol[1] = 0.25f + (float)((x >> 9) & 0x1F) / 40.f;
  g_accentCol[2] = 0.3f + (float)((x >> 14) & 0x1F) / 40.f;
  g_accentCol[3] = 1.f;
  for (int i = 0; i < 3; i++)
    g_accentCol[i] = std::clamp(g_accentCol[i], 0.f, 1.f);

  Game::SetFullbright(g_fullbright);
  Game::SetWallhack(g_wallhack);
  g_lastVisualPreset = presetId;
}

static void DrawPresetBankCard(const char *title, int startId, int count,
                               bool espBank) {
  Card(title);
  ImGui::TextColored({0.75f, 0.84f, 1.f, 1.f},
                     espBank ? "One-click ESP packs (usable)"
                             : "One-click Visual packs (usable)");
  ImGui::TextColored({0.56f, 0.64f, 0.78f, 1.f},
                     "Each button applies a different live configuration.");
  ImGui::Separator();

  const int cols = 8;
  const ImVec2 bs = {66.f, 22.f};
  for (int i = 0; i < count; i++) {
    int id = startId + i;
    char label[16];
    snprintf(label, sizeof(label), "%s%03d", espBank ? "E" : "V", id + 1);
    ImGui::PushID((espBank ? 7000 : 9000) + id);
    if (GlowBtn(label, bs)) {
      if (espBank)
        ApplyEspPreset(id);
      else
        ApplyVisualPreset(id);
    }
    ImGui::PopID();
    if ((i % cols) != (cols - 1))
      ImGui::SameLine();
  }

  int last = espBank ? g_lastEspPreset : g_lastVisualPreset;
  if (last >= 0) {
    ImGui::TextColored({0.4f, 1.f, 0.72f, 1.f}, "Last Applied: %s%03d",
                       espBank ? "E" : "V", last + 1);
  }
  EndCard();
}
static void TabDashboard() {
  Card("Profile");
  ImGui::TextColored({.75f, .82f, .95f, 1}, "Player:");
  ImGui::SameLine();
  ImGui::TextColored({1, 1, 1, 1}, "%s", g_nameBuf);
  ImGui::SameLine(220);
  ImGui::TextColored({0, 1, .5f, 1}, "Online");
  ImGui::TextColored({.75f, .82f, .95f, 1}, "Version:");
  ImGui::SameLine();
  ImGui::TextColored({1, 1, 1, 1}, "%s", APP_VERSION);
  bool dumpLoaded = DumpDatabase::IsLoaded();
  ImGui::TextColored({.75f, .82f, .95f, 1}, "Dump.cs:");
  ImGui::SameLine();
  ImGui::TextColored(dumpLoaded ? ImVec4{.3f, 1, .5f, 1}
                                : ImVec4{1, .5f, .3f, 1},
                     "%s", dumpLoaded ? "Loaded" : "Not Found");
  if (dumpLoaded) {
    ImGui::TextColored({.65f, .72f, .85f, 1},
                       "%zu methods | %zu fields | %zu types",
                       DumpDatabase::MethodCount(), DumpDatabase::FieldCount(),
                       DumpDatabase::ClassCount());
  } else {
    ImGui::TextColored({1.f, 0.72f, 0.35f, 1.f},
                       "Tip: set STARA_DUMP_PATH to your dump.cs file.");
  }
  EndCard();

  Card("Live Player Info");
  if (!Game::isInGame && !Game::isInLobby) {
    ImGui::TextColored({1.f, 0.6f, 0.3f, 1.f}, "Not connected to a game.");
  } else {
    // State badge
    if (Game::isInGame)
      ImGui::TextColored({.3f, 1, .5f, 1}, "IN GAME");
    else
      ImGui::TextColored({.4f, .85f, 1, 1}, "IN LOBBY");
    ImGui::SameLine(120);
    if (Game::IsHost())
      ImGui::TextColored({1.f, 0.85f, 0.2f, 1.f}, "[HOST]");
    else
      ImGui::TextColored({0.65f, 0.65f, 0.75f, 1.f}, "[CLIENT]");

    ImGui::Spacing();

    // Role (in-game only)
    if (Game::isInGame) {
      ImVec4 roleCol = Game::isImpostor ? ImVec4{1, .2f, .2f, 1}
                                        : ImVec4{.3f, 1, .5f, 1};
      ImGui::TextColored({.75f, .82f, .95f, 1}, "Role:");
      ImGui::SameLine();
      ImGui::TextColored(roleCol, "%s %s", Game::localRoleName.c_str(),
                         Game::isImpostor ? "[IMP]" : "[CREW]");
    }

    // Level
    ImGui::TextColored({.75f, .82f, .95f, 1}, "Level:");
    ImGui::SameLine();
    ImGui::TextColored({1, 1, 1, 1}, "%d", Game::localLevel);

    // Color
    {
      const char *colorNames[] = {"Red",    "Blue",   "Green",  "Pink",
                                  "Orange", "Yellow", "Black",  "White",
                                  "Purple", "Brown",  "Cyan",   "Lime",
                                  "Maroon", "Rose",   "Banana", "Gray",
                                  "Tan",    "Coral"};
      int cid = Game::localColorId;
      ImGui::TextColored({.75f, .82f, .95f, 1}, "Color:");
      ImGui::SameLine();
      if (cid >= 0 && cid < 18)
        ImGui::TextColored({1, 1, 1, 1}, "%s (%d)", colorNames[cid], cid);
      else
        ImGui::TextColored({.7f, .7f, .7f, 1}, "Unknown (%d)", cid);
    }

    // Position
    ImGui::TextColored({.75f, .82f, .95f, 1}, "Position:");
    ImGui::SameLine();
    ImGui::TextColored({1, 1, 1, 1}, "(%.1f, %.1f)", Game::localX,
                       Game::localY);

    // Speed
    ImGui::TextColored({.75f, .82f, .95f, 1}, "Speed:");
    ImGui::SameLine();
    ImGui::TextColored({1, 1, 1, 1}, "%.1f", g_walkSpeed);

    // Meeting status
    if (Game::isInMeeting)
      ImGui::TextColored({1, .4f, .4f, 1}, ">> IN MEETING <<");

    // Player count
    int alive = 0, imps = 0;
    for (const auto &p : Game::players) {
      if (!p.isDead)
        alive++;
      if (p.isImpostor && !p.isDead)
        imps++;
    }
    ImGui::TextColored({.75f, .82f, .95f, 1}, "Players:");
    ImGui::SameLine();
    ImGui::TextColored({1, 1, 1, 1}, "%d alive (%d impostors)", alive + 1,
                       imps + (Game::isImpostor ? 1 : 0));
  }
  EndCard();

  Card("Active Cheats");
  {
    int activeCount = 0;
    struct CheatEntry { const char* name; bool active; ImVec4 col; };
    CheatEntry cheats[] = {
      {"NoClip",            g_noclip,              {.3f, .9f, 1, 1}},
      {"Fullbright",        g_fullbright,           {1, 1, .4f, 1}},
      {"Wallhack",          g_wallhack,             {1, .7f, .3f, 1}},
      {"No Kill CD",        g_noKillCd,             {1, .3f, .3f, 1}},
      {"God Mode",          g_godmode,              {.4f, 1, .4f, 1}},
      {"Inf. Emergencies",  g_infiniteEmergencies,  {1, .85f, .2f, 1}},
      {"Always Moveable",   g_alwaysMoveable,       {.5f, .9f, 1, 1}},
      {"Impostor Vision",   g_impostorVision,       {.8f, .3f, 1, 1}},
      {"Max Report Dist",   g_maxReportDist,        {1, .6f, .2f, 1}},
      {"Auto Tasks",        g_autoTasks,            {.3f, 1, .6f, 1}},
      {"Anti-Kick",         g_antiKick,             {.3f, .8f, 1, 1}},
      {"Force Shield",      g_forceProtect,         {.6f, .9f, .3f, 1}},
      {"Freeze All",        g_freezeAll,            {.4f, .7f, 1, 1}},
      {"Rainbow",           g_rainbow,              {1, .5f, .8f, 1}},
      {"Spin",              g_spin,                 {.8f, .6f, 1, 1}},
      {"Dance",             g_dance,                {1, .8f, .3f, 1}},
      {"Chat Spam",         g_chatSpam,             {1, .4f, .4f, 1}},
      {"Color Cycle",       g_colorCycle,           {.6f, 1, .8f, 1}},
      {"Spam Anim",         g_spamAnim,             {1, .5f, .5f, 1}},
      {"Auto Path",         g_autoPath,             {.5f, 1, .7f, 1}},
      {"RGB Accent",        g_rgbAccent,            {.9f, .4f, 1, 1}},
    };
    for (const auto &ch : cheats)
      if (ch.active) activeCount++;

    if (activeCount == 0) {
      ImGui::TextColored({.6f, .6f, .7f, 1}, "No cheats active.");
    } else {
      ImGui::TextColored({.4f, 1, .6f, 1}, "%d cheat(s) active:", activeCount);
      for (const auto &ch : cheats) {
        if (ch.active)
          ImGui::TextColored(ch.col, "  [ON] %s", ch.name);
      }
    }
  }
  EndCard();
  Card("Quick Actions");
  if (GlowBtn("Complete Tasks", {140, 30}))
    Game::CompleteAllTasks();
  ImGui::SameLine();
  if (GlowBtn("Emergency Meeting", {160, 30}))
    Game::ForceEmergencyMeeting();
  EndCard();

  DrawDiscordProfileCard();
}

static void TabPlayer() {
  Card("Player Settings");
  if (Game::isInGame || Game::isInLobby) {
    ImGui::TextColored({.75f, .82f, .95f, 1}, "Level:");
    ImGui::SameLine();
    ImGui::TextColored({1, 1, 1, 1}, "%d", Game::localLevel);
    ImGui::SameLine(0, 20);
    if (Game::isInGame) {
      ImVec4 rc = Game::isImpostor ? ImVec4{1, .3f, .3f, 1}
                                   : ImVec4{.3f, 1, .5f, 1};
      ImGui::TextColored({.75f, .82f, .95f, 1}, "Role:");
      ImGui::SameLine();
      ImGui::TextColored(rc, "%s", Game::localRoleName.c_str());
      ImGui::SameLine(0, 20);
      ImGui::TextColored({.75f, .82f, .95f, 1}, "Pos:");
      ImGui::SameLine();
      ImGui::TextColored({1, 1, 1, 1}, "(%.1f, %.1f)", Game::localX,
                         Game::localY);
    }
    ImGui::Separator();
  }
  Toggle("NoClip", &g_noclip);
  Slider("Speed##p", &g_speed, 0.5f, 10.f);
  if (ImGui::IsItemDeactivatedAfterEdit()) {
    g_walkSpeed = g_speed;
    Game::SetSpeed(g_speed);
  }
  ImGui::InputText("Name##p", g_nameBuf, 64);
  if (ImGui::IsItemDeactivatedAfterEdit())
    Game::SetName(g_nameBuf);
  static int g_level = 100;
  ImGui::InputInt("Level##p", &g_level);
  ImGui::SameLine();
  ImGui::TextColored({.8f, .85f, 1, 1}, "(Current: %d)", Game::localLevel);
  if (ImGui::IsItemDeactivatedAfterEdit())
    Game::SetLevel(g_level);
  EndCard();

  Card("Role");
  if (Game::isInGame) {
    ImVec4 rc = Game::isImpostor ? ImVec4{1, .2f, .2f, 1} : ImVec4{.3f, 1, .5f, 1};
    ImGui::TextColored(rc, "Current: %s %s", Game::localRoleName.c_str(),
                       Game::isImpostor ? "[IMP]" : "[CREW]");
  }
  static int selRole = 0;
  const char *roles[] = {"Crewmate",       "Impostor",       "Scientist",
                         "Engineer",       "Guardian Angel", "Shapeshifter",
                         "Crewmate Ghost", "Impostor Ghost", "Noisemaker",
                         "Phantom",        "Tracker",        "Detective",
                         "Viper"};
  const int roleIds[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 12, 18};
  ImGui::Combo("Set Fake Role##r", &selRole, roles, 13);
  if (GlowBtn("Apply Fake Role", {140, 28}))
    Game::SetRole(roleIds[selRole]);
  ImGui::TextColored({0.5f, 0.8f, 1, 1}, "Local only - gives you abilities (no kick)");
  ImGui::SameLine();
  bool canRevive = Game::isInGame && !Game::isInMeeting;
  if (!canRevive)
    ImGui::BeginDisabled();
  if (GlowBtn("Revive Self", {120, 28})) {
    Game::RevivePlayer();
    PushToast("Revive sent", IM_COL32(120, 205, 255, 245), 2.0f);
  }
  if (!canRevive)
    ImGui::EndDisabled();
  if (!canRevive)
    ImGui::TextColored({0.8f, 0.64f, 0.5f, 1.f},
                       "Revive disabled during meeting/lobby for stability");
  EndCard();

  Card("Lobby Actions (Host)");
  if (GlowBtn("Force Start Game", {200, 28}))
    Game::StartGame();
  if (GlowBtn("Force End Game", {180, 28}))
    Game::EndGame();
  EndCard();

  Card("Tasks");
  if (GlowBtn("Complete All Tasks", {180, 28}))
    Game::CompleteAllTasks();
  EndCard();

  Card("Impostor");
  Toggle("No Kill Cooldown", &g_noKillCd);
  Toggle("Kill Reach (Infinite)", &g_killReach);
  Toggle("Kill Anyone (Bypass Protection)", &g_killAnyone);
  Toggle("God Mode (Anti-Death)", &g_godmode);
  Slider("Kill Cooldown##op", &g_killCd, 0.f, 60.f);
  if (ImGui::IsItemDeactivatedAfterEdit())
    Game::SetKillCooldown(g_killCd);
  Slider("Kill Distance##op", &g_killDist, 1.f, 50.f);
  if (ImGui::IsItemDeactivatedAfterEdit())
    Game::SetKillDistance(g_killDist);
  if (Toggle("Wallhack / No Shadows##op", &g_wallhack))
    Game::SetWallhack(g_wallhack);
  EndCard();

  Card("Cheats");
  Toggle("Infinite Emergencies", &g_infiniteEmergencies);
  Toggle("Always Moveable", &g_alwaysMoveable);
  Toggle("Impostor Vision", &g_impostorVision);
  Toggle("Max Report Distance", &g_maxReportDist);
  Toggle("Auto Complete Tasks", &g_autoTasks);
  Toggle("Anti-Kick (Safe Mode)", &g_antiKick);
  Toggle("Force Shield (GA) (Host)", &g_forceProtect);
  Toggle("Freeze All Players", &g_freezeAll);
  Toggle("See Ghosts", &g_seeGhosts);
  Toggle("Always Chat", &g_alwaysChat);
  EndCard();

  Card("Lobby Settings");
  Slider("Discussion Time##ls", &g_customDiscussTime, 0.f, 120.f);
  if (ImGui::IsItemDeactivatedAfterEdit())
    Game::SetDiscussionTime(g_customDiscussTime);
  Slider("Voting Time##ls", &g_customVoteTime, 0.f, 300.f);
  if (ImGui::IsItemDeactivatedAfterEdit())
    Game::SetVotingTime(g_customVoteTime);
  static int g_emergCount = 9;
  ImGui::InputInt("Emergencies##ls", &g_emergCount);
  if (ImGui::IsItemDeactivatedAfterEdit())
    Game::SetEmergencyCount(g_emergCount);
  EndCard();

  Card("Chat");
  ImGui::InputText("Message##chat", g_chatBuf, 128);
  if (GlowBtn("Send Chat", {120, 28}))
    Game::SendChat(g_chatBuf);
  Toggle("Auto Spam Chat", &g_chatSpam);
  EndCard();

  Card("Kill Players");
  if (!Game::isInGame) {
    ImGui::TextColored({1, 0.3f, 0.3f, 1}, "Not in game");
  } else {
    for (int i = 0; i < (int)Game::players.size(); i++) {
      const auto &p = Game::players[i];
      if (p.isDead)
        continue;
      ImGui::PushID(i);
      if (GlowBtn(p.name.c_str(), {140, 24}))
        Game::KillPlayer(p.playerId);
      ImGui::SameLine();
      ImGui::TextColored(p.isImpostor ? ImVec4{1, .2f, .2f, 1}
                                      : ImVec4{.5f, 1, .5f, 1},
                         "%s", p.roleName.c_str());
      ImGui::PopID();
    }
    ImGui::Spacing();
    if (GlowBtn("Kill ALL Players", {180, 28}))
      Game::KillAllPlayers();
    if (GlowBtn("Kill All Crewmates", {180, 28}))
      Game::KillAllCrewmates();
    ImGui::SameLine();
    if (GlowBtn("Kill All Impostors", {180, 28}))
      Game::KillAllImpostors();
  }
  EndCard();

  Card("Vote Manipulation (Host)");
  ImGui::TextColored({1.f, 0.85f, 0.2f, 1.f}, "Requires Host or Host Spoof");
  if (!Game::isInMeeting) {
    ImGui::TextColored({0.6f, 0.6f, 0.7f, 1.f}, "Not in a meeting.");
  } else {
    if (GlowBtn("Skip Meeting (No Eject)", {220, 28})) {
      Game::SkipMeeting();
      PushToast("Meeting skipped", IM_COL32(255, 220, 90, 245), 2.0f);
    }
    ImGui::TextColored({0.5f, 0.8f, 1, 1}, "Ends meeting with no ejection (tie result)");
    ImGui::Spacing();
    ImGui::TextColored({1.f, 0.4f, 0.4f, 1.f}, "Eject Player:");
    for (int i = 0; i < (int)Game::players.size(); i++) {
      const auto &p = Game::players[i];
      if (p.isDead) continue;
      ImGui::PushID(5000 + i);
      if (GlowBtn(("Eject " + p.name).c_str(), {200, 24})) {
        Game::EjectPlayer(p.playerId);
        PushToast("Ejected " + p.name, IM_COL32(255, 100, 100, 245), 2.5f);
      }
      ImGui::PopID();
    }
  }
  EndCard();
}

static void TabVisuals() {
  Card("Rendering");
  if (Toggle("Fullbright", &g_fullbright))
    Game::SetFullbright(g_fullbright);
  if (Toggle("Wireframe View", &g_wireframe)) {
    // Wireframe flag is mapped to wallhack-style visibility so it is always
    // functional.
    Game::SetWallhack(g_wireframe || g_wallhack);
  }
  EndCard();
  Card("Camera");
  if (Slider("Camera Zoom##v", &g_zoom, 1.0f, 12.0f, "%.1f"))
    Game::SetCameraZoom(g_zoom);
  ImGui::TextColored({.65f, .72f, .85f, 1},
                     "Zoom out to see more of the map.");
  EndCard();
  Card("Effects");
  Toggle("RGB Accent##v", &g_rgbAccent);
  EndCard();
}

static void TabESP() {
  Card("ESP Configuration");
  Toggle("Player Box", &g_espBox);
  Toggle("Player Name", &g_espName);
  Toggle("Show Distance", &g_espDist);
  Toggle("Show Roles", &g_espRole);
  EndCard();

  Card("Session Player List (Impostor Detector)");
  if (!Game::isInGame) {
    ImGui::TextColored({1, 0.3f, 0.3f, 1}, "Not in a session.");
  } else {
    if (ImGui::BeginTable("PlayerListTable", 3,
                          ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
      ImGui::TableSetupColumn("Name");
      ImGui::TableSetupColumn("Role");
      ImGui::TableSetupColumn("Dist");
      ImGui::TableHeadersRow();

      for (const auto &p : Game::players) {
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        if (p.isImpostor) {
          if (p.isDead)
            ImGui::TextColored({1, 0.1f, 0.1f, 1}, "%s [IMP] [DEAD]",
                               p.name.c_str());
          else
            ImGui::TextColored({1, 0.1f, 0.1f, 1}, "%s [IMP]", p.name.c_str());
        } else if (p.isDead) {
          ImGui::TextColored({0.5f, 0.5f, 0.5f, 1}, "%s [DEAD]",
                             p.name.c_str());
        } else {
          ImGui::Text("%s", p.name.c_str());
        }

        ImGui::TableNextColumn();
        if (p.isDead) {
          ImGui::TextColored({0.4f, 0.4f, 0.4f, 1}, "DEAD (%s)",
                             p.roleName.c_str());
        } else {
          ImU32 roleCol = p.isImpostor ? IM_COL32(255, 100, 100, 255)
                                       : IM_COL32(100, 255, 100, 255);
          ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(roleCol), "%s",
                             p.roleName.c_str());
        }

        ImGui::TableNextColumn();
        if (p.distance >= 0.f)
          ImGui::Text("%.1fm", p.distance);
        else
          ImGui::TextUnformatted("N/A");
      }
      ImGui::EndTable();
    }
  }
  EndCard();

  Card("Visuals");
  Toggle("Tracer Lines (All)", &g_espTracer);
  Toggle("Outline", &g_espOutline);
  Toggle("Task Markers", &g_espTask);
  EndCard();

  Card("Tracers");
  Toggle("Crewmates (Cyan)", &g_tracerCrew);
  Toggle("Impostors (Red)", &g_tracerImp);
  Toggle("Ghosts (White)", &g_tracerGhost);
  Toggle("Dead Bodies (Yellow)", &g_tracerBodies);
  Toggle("Color-based", &g_tracerColorBased);
  ImGui::TextColored({0.5f, 0.8f, 1, 1}, "Changes tracer color to player color");
  EndCard();

  Card("Camera");
  Toggle("Teleport to Cursor", &g_teleportToCursor);
  ImGui::TextColored({0.5f, 0.8f, 1, 1}, "Right-click to teleport. Best with Zoom Out");
  Toggle("Freecam", &g_freecam);
  ImGui::TextColored({0.5f, 0.8f, 1, 1}, "WASD/Arrows to move camera freely");
  if (g_freecam && g_spectate) g_spectate = false;
  if (Toggle("Spectate", &g_spectate) && g_spectate) g_freecam = false;
  if (g_spectate && Game::isInGame) {
    for (int i = 0; i < (int)Game::players.size(); i++) {
      const auto &sp = Game::players[i];
      if (sp.isDead) continue;
      ImGui::PushID(700 + i);
      bool active = (g_spectateTarget == sp.playerId);
      if (active) ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{0, 0.6f, 1, 0.7f});
      if (GlowBtn(sp.name.c_str(), {160, 22}))
        g_spectateTarget = sp.playerId;
      if (active) ImGui::PopStyleColor();
      ImGui::PopID();
    }
  }
  EndCard();

  Card("Vent ESP");
  Toggle("Show Vents", &g_espVent);
  if (g_espVent)
    ImGui::TextColored({1.f, .75f, .2f, 1},
                       "Orange diamonds with vent IDs shown on map.");
  EndCard();

  Card("Minimap ESP");
  Toggle("Show Minimap", &g_minimapEsp);
  if (g_minimapEsp) {
    Slider("Minimap Scale##mm", &g_minimapScale, 0.05f, 0.3f, "%.2f");
    ImGui::TextColored({0.5f, 0.8f, 1, 1}, "Shows player dots on a corner minimap");
  }
  Toggle("Distance Tracers", &g_distanceTracers);
  if (g_distanceTracers)
    ImGui::TextColored({0.5f, 0.8f, 1, 1}, "Lines from you to all players with distance");
  Toggle("Stealth Mode (Hide ESP)", &g_stealthMode);
  if (g_stealthMode)
    ImGui::TextColored({1.f, 0.6f, 0.3f, 1.f}, "ESP/Tracers hidden while active");
  EndCard();

  // ESP preview panel
  Card("##espPreview");
  ImDrawList *dl = ImGui::GetWindowDrawList();
  ImVec2 p = ImGui::GetCursorScreenPos();
  float pw = 280, ph = 200;
  dl->AddRectFilled(p, {p.x + pw, p.y + ph}, IM_COL32(8, 8, 14, 220), 6);
  // Draw a sample "player" in the preview
  float cx = p.x + pw / 2, cy = p.y + ph / 2;
  float bw = 30, bh = 50;
  if (g_espBox)
    dl->AddRect({cx - bw, cy - bh}, {cx + bw, cy + bh}, Colors::Cyan, 2, 0,
                1.5f);
  if (g_espName)
    dl->AddText({cx - 20, cy - bh - 16}, Colors::TextPrimary, "Player");
  if (g_espDist)
    dl->AddText({cx - 10, cy + bh + 4}, Colors::TextSecondary, "15m");
  if (g_espTracer)
    dl->AddLine({p.x + pw / 2, p.y + ph}, {cx, cy + bh}, Colors::Cyan, 1);
  if (g_espOutline) {
    for (float i = 3; i > 0; i -= 1.f)
      dl->AddRect({cx - bw - i, cy - bh - i}, {cx + bw + i, cy + bh + i},
                  IM_COL32(0, 220, 255, (int)(20 * i)), 2);
  }
  ImGui::Dummy({pw, ph});
  EndCard();
}

static void TabMovement() {
  Card("Movement");
  if (Game::isInGame) {
    ImGui::TextColored({.5f, .85f, 1, 1}, "Position: (%.1f, %.1f) | Speed: %.1f",
                       Game::localX, Game::localY, g_walkSpeed);
    ImGui::Separator();
  }
  Slider("Walk Speed##m", &g_walkSpeed, 0.5f, 10);
  if (ImGui::IsItemDeactivatedAfterEdit()) {
    g_speed = g_walkSpeed;
    Game::SetSpeed(g_walkSpeed);
  }
  Toggle("Smooth Movement##m", &g_smoothMove);
  Slider("Animation Speed##m", &g_animSpeed, 0.1f, 5);
  Toggle("Auto Path##m", &g_autoPath);
  if (g_autoPath)
    ImGui::TextColored({.65f, .72f, .85f, 1},
                       "Walking in a circle around your position.");
  EndCard();
  Card("Teleport");
  static float tpX = 0, tpY = 0;
  ImGui::InputFloat("X##tp", &tpX);
  ImGui::InputFloat("Y##tp", &tpY);
  if (GlowBtn("Teleport", {100, 28}))
    Game::TeleportTo(tpX, tpY);
  EndCard();
}

static void TabFun() {
  Card("Character Effects");
  Toggle("Rainbow Character", &g_rainbow);
  Toggle("Spin Preview", &g_spin);
  Toggle("Dance Animation", &g_dance);
  Toggle("Particle Effects", &g_particle);
  if (Toggle("Tiny Character", &g_tiny)) {
    if (g_tiny) {
      g_giant = false;
      Game::SetCharacterScale(0.4f);
    } else
      Game::SetCharacterScale(1.0f);
  }
  if (Toggle("Giant Character", &g_giant)) {
    if (g_giant) {
      g_tiny = false;
      Game::SetCharacterScale(2.5f);
    } else
      Game::SetCharacterScale(1.0f);
  }
  EndCard();
  Card("Emotes");
  const char *emotes[] = {"Wave", "Dance", "Clap", "Dab", "Flex"};
  for (int i = 0; i < 5; i++) {
    if (i)
      ImGui::SameLine();
    if (GlowBtn(emotes[i], {60, 28}))
      Game::PlayAnimation((uint8_t)i);
  }
  EndCard();
}

static void TabTroll() {
  Card("Server Trolls");
  Toggle("Chat Spam", &g_chatSpam);
  if (GlowBtn("Force Emergency Meeting (Host)", {230, 28}))
    Game::ForceEmergencyMeeting();
  if (GlowBtn("Force Start Game (Host)", {220, 28}))
    Game::StartGame();
  if (GlowBtn("Force End / Leave", {200, 28}))
    Game::EndGame();
  if (GlowBtn("Complete All Tasks", {200, 28}))
    Game::CompleteAllTasks();
  if (GlowBtn("Close Meeting (Local)", {200, 28}))
    Game::CloseMeeting();
  ImGui::TextColored({0.5f, 0.8f, 1, 1}, "Closes meeting UI Ã¢â‚¬â€ lets you move during meetings");
  EndCard();

  Card("Impostor Abilities (Host)");
  if (GlowBtn("Vanish (Phantom)", {160, 28}))
    Game::Vanish();
  ImGui::SameLine();
  if (GlowBtn("Appear", {100, 28}))
    Game::Appear();
  Toggle("Kill While Vanished", &g_killWhileVanished);
  EndCard();

  Card("Shapeshifter");
  Toggle("No Shapeshift Animation", &g_noSsAnimation);
  Toggle("Endless Shapeshift Duration", &g_endlessSsDuration);
  EndCard();

  Card("Engineer");
  Toggle("Endless Vent Time", &g_endlessVentTime);
  Toggle("No Vent Cooldown", &g_noVentCooldown);
  EndCard();

  Card("Scientist");
  Toggle("Endless Battery", &g_endlessBattery);
  Toggle("No Vitals Cooldown", &g_noVitalsCooldown);
  EndCard();

  Card("Tracker");
  Toggle("Endless Tracking", &g_endlessTracking);
  Toggle("No Track Delay", &g_noTrackDelay);
  Toggle("No Track Cooldown", &g_noTrackCooldown);
  EndCard();

  Card("Ship");
  Toggle("Unfixable Lights", &g_unfixableLights);
  ImGui::TextColored({0.5f, 0.8f, 1, 1}, "Lights cannot be fixed while enabled");
  EndCard();

  Card("Shapeshift (Host)");
  if (!Game::isInGame) {
    ImGui::TextColored({1, 0.3f, 0.3f, 1}, "Not in game");
  } else {
    for (int i = 0; i < (int)Game::players.size(); i++) {
      const auto &p = Game::players[i];
      if (p.isDead)
        continue;
      ImGui::PushID(100 + i);
      if (GlowBtn(("Shift -> " + p.name).c_str(), {200, 24}))
        Game::ShapeshiftTo(p.playerId);
      ImGui::PopID();
    }
  }
  EndCard();

  Card("Set Player Roles (Host = Network, Self = Local)");
  if (!Game::isInGame) {
    ImGui::TextColored({1, 0.3f, 0.3f, 1}, "Not in game");
  } else {
    const char *roleNames[] = {"Crewmate",   "Impostor",       "Scientist",
                               "Engineer",   "Guardian Angel", "Shapeshifter",
                               "Crew Ghost", "Imp Ghost",      "Noisemaker",
                               "Phantom",    "Tracker",        "Detective",
                               "Viper"};
    const int roleIds[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 12, 18};
    constexpr int NUM_ROLES = 13;
    static std::unordered_map<int, int> selectedRoleByPid;
    static int selfRole = 0;
    static int selfOdds[NUM_ROLES] = {};
    static int otherOdds[NUM_ROLES] = {};
    static bool oddsInit = false;
    static bool oddsSeeded = false;
    if (!oddsInit) {
      oddsInit = true;
      selfOdds[0] = 100;
      otherOdds[0] = 100;
    }
    if (!oddsSeeded) {
      oddsSeeded = true;
      srand((unsigned)GetTickCount());
    }

    auto ClampOdds = [](int *arr) {
      for (int i = 0; i < NUM_ROLES; i++)
        arr[i] = std::clamp(arr[i], 0, 100);
    };
    auto RollRoleByOdds = [](const int *arr) -> int {
      int total = 0;
      for (int i = 0; i < NUM_ROLES; i++)
        total += (arr[i] > 0 ? arr[i] : 0);
      if (total <= 0)
        return -1;
      int r = rand() % total;
      int acc = 0;
      for (int i = 0; i < NUM_ROLES; i++) {
        int w = (arr[i] > 0 ? arr[i] : 0);
        acc += w;
        if (r < acc)
          return i;
      }
      return 0;
    };

    // Set ALL players to a role at once
    static int massRole = 1;
    ImGui::Combo("Mass Role##mr", &massRole, roleNames, NUM_ROLES);
    if (GlowBtn("Set ALL to Role (Host)", {200, 28})) {
      for (int i = 0; i < (int)Game::players.size(); i++)
        Game::SetPlayerRole(Game::players[i].playerId, roleIds[massRole]);
      // Also set self
      Game::SetRole(roleIds[massRole]);
    }
    ImGui::Separator();

    // Self role (local only Ã¢â‚¬â€ safe for non-host)
    ImGui::TextColored({0, 0.86f, 1, 1}, "You (Fake Role):");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(140);
    ImGui::Combo("##selfRole", &selfRole, roleNames, NUM_ROLES);
    ImGui::SameLine();
    if (GlowBtn("Set##self", {50, 22}))
      Game::SetRole(roleIds[selfRole]);

    ImGui::Separator();
    ImGui::TextColored({0.6f, 0.6f, 0.75f, 1}, "Other Players (Host Only):");

    for (int i = 0; i < (int)Game::players.size() && i < 15; i++) {
      const auto &p = Game::players[i];
      ImGui::PushID(400 + i);
      int &pick = selectedRoleByPid[p.playerId];
      if (pick < 0 || pick >= NUM_ROLES)
        pick = 0;
      ImGui::TextColored(p.isImpostor ? ImVec4{1, .2f, .2f, 1}
                                      : ImVec4{.5f, 1, .5f, 1},
                         "%s", p.name.c_str());
      ImGui::SameLine();
      ImGui::SetNextItemWidth(130);
      ImGui::Combo("##role", &pick, roleNames, NUM_ROLES);
      ImGui::SameLine();
      if (GlowBtn("Set", {45, 22}))
        Game::SetPlayerRole(p.playerId, roleIds[pick]);
      ImGui::PopID();
    }

    ImGui::Separator();
    ImGui::TextColored({1.f, 0.85f, 0.2f, 1.f}, "Role Odds (Weighted Roll)");
    ImGui::TextColored({0.65f, 0.65f, 0.78f, 1.f},
                       "Set weights 0-100, then roll roles from your odds.");
    ClampOdds(selfOdds);
    ClampOdds(otherOdds);

    if (ImGui::BeginTable("RoleOddsTbl", 3,
                          ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
                              ImGuiTableFlags_SizingStretchProp)) {
      ImGui::TableSetupColumn("Role");
      ImGui::TableSetupColumn("Self %");
      ImGui::TableSetupColumn("Others %");
      ImGui::TableHeadersRow();
      for (int i = 0; i < NUM_ROLES; i++) {
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::TextUnformatted(roleNames[i]);
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-1);
        ImGui::InputInt((std::string("##selfOdds") + std::to_string(i)).c_str(),
                        &selfOdds[i], 1, 5);
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-1);
        ImGui::InputInt(
            (std::string("##otherOdds") + std::to_string(i)).c_str(),
            &otherOdds[i], 1, 5);
      }
      ImGui::EndTable();
    }
    ClampOdds(selfOdds);
    ClampOdds(otherOdds);

    if (GlowBtn("Roll Self by Odds", {150, 26})) {
      int pick = RollRoleByOdds(selfOdds);
      if (pick >= 0) {
        selfRole = pick;
        Game::SetRole(roleIds[pick]);
      }
    }
    ImGui::SameLine();
    if (GlowBtn("Roll Others by Odds", {170, 26})) {
      for (int i = 0; i < (int)Game::players.size(); i++) {
        int pick = RollRoleByOdds(otherOdds);
        if (pick >= 0) {
          int pid = Game::players[i].playerId;
          selectedRoleByPid[pid] = pick;
          Game::SetPlayerRole(pid, roleIds[pick]);
        }
      }
    }
    if (GlowBtn("Roll Everyone by Odds", {170, 26})) {
      int selfPick = RollRoleByOdds(selfOdds);
      if (selfPick >= 0) {
        selfRole = selfPick;
        Game::SetRole(roleIds[selfPick]);
      }
      for (int i = 0; i < (int)Game::players.size(); i++) {
        int pick = RollRoleByOdds(otherOdds);
        if (pick >= 0) {
          int pid = Game::players[i].playerId;
          selectedRoleByPid[pid] = pick;
          Game::SetPlayerRole(pid, roleIds[pick]);
        }
      }
    }
  }
  EndCard();

  Card("Vents");
  Toggle("Use Vents (Any Role)", &g_useVents);
  Toggle("Walk In Vents (Invisibility)", &g_walkInVents);
  ImGui::TextColored({0.5f, 0.8f, 1, 1}, "Move freely while invisible in vent");
  static int ventId = 0;
  ImGui::InputInt("Vent ID##v", &ventId);
  if (GlowBtn("Enter Vent", {120, 28}))
    Game::EnterVent(ventId);
  ImGui::SameLine();
  if (GlowBtn("Exit Vent", {120, 28}))
    Game::ExitVent(ventId);
  if (GlowBtn("Kick All From Vents", {180, 28}))
    Game::KickAllFromVents();
  EndCard();

  Card("Sabotage");
  ImGui::TextColored({0.5f, 0.8f, 1, 1}, "Works as any role, no cooldown");
  if (GlowBtn("Reactor", {100, 28}))
    Game::TriggerSabotage(3);
  ImGui::SameLine();
  if (GlowBtn("Oxygen", {100, 28}))
    Game::TriggerSabotage(8);
  ImGui::SameLine();
  if (GlowBtn("Lights", {100, 28}))
    Game::TriggerSabotage(7);
  if (GlowBtn("Comms", {100, 28}))
    Game::TriggerSabotage(14);
  ImGui::SameLine();
  if (GlowBtn("Mushroom Mixup", {140, 28}))
    Game::MushroomMixup();
  EndCard();

  Card("Doors");
  const char *rooms[] = {
      "Hallway",    "Storage",  "Cafeteria",  "Reactor",      "Upper Engine",
      "Navigation", "Admin",    "Electrical", "O2",           "Shields",
      "MedBay",     "Security", "Weapons",    "Lower Engine", "Comms"};
  static int doorRoom = 3;
  ImGui::Combo("Room##doors", &doorRoom, rooms, 15);
  if (GlowBtn("Close Doors", {140, 28}))
    Game::CloseDoors(doorRoom);
  ImGui::SameLine();
  if (GlowBtn("Fix Sabotage", {140, 28}))
    Game::RepairSabotage(doorRoom);
  if (GlowBtn("Close ALL Doors", {200, 28}))
    Game::CloseAllDoors();
  ImGui::SameLine();
  if (GlowBtn("Fix ALL Sabotage", {200, 28}))
    Game::FixAllSabotage();
  EndCard();

  Card("Mass Actions");
  if (GlowBtn("Teleport ALL to Self (Host)", {240, 28}))
    Game::TeleportAllToSelf();
  Toggle("Color Cycle (Strobe)", &g_colorCycle);
  Toggle("Spam Animations", &g_spamAnim);
  EndCard();

  Card("Teleport To Player");
  if (!Game::isInGame) {
    ImGui::TextColored({1, 0.3f, 0.3f, 1}, "Not in game");
  } else {
    for (int i = 0; i < (int)Game::players.size(); i++) {
      const auto &p = Game::players[i];
      if (p.isDead)
        continue;
      ImGui::PushID(200 + i);
      if (GlowBtn(("TP -> " + p.name).c_str(), {180, 24}))
        Game::TeleportToPlayer(p.playerId);
      ImGui::PopID();
    }
  }
  EndCard();

  Card("Teleport To Room (Skeld)");
  const char *skeldRooms[] = {"Cafeteria", "Reactor",      "Navigation",
                              "MedBay",    "Electrical",   "Storage",
                              "Weapons",   "Upper Engine", "Lower Engine"};
  for (int i = 0; i < 9; i++) {
    ImGui::PushID(300 + i);
    if (GlowBtn(skeldRooms[i], {140, 24}))
      Game::TeleportToRoom(i);
    if ((i % 3) != 2)
      ImGui::SameLine();
    ImGui::PopID();
  }
  EndCard();

  Card("Character Trolls");
  if (GlowBtn("Play Kill Anim", {160, 28}))
    Game::PlayAnimation(2);
  if (GlowBtn("Play Scan Anim", {160, 28}))
    Game::PlayAnimation(1);
  EndCard();

  Card("Overload System");
  ImGui::TextColored({1.f, 0.3f, 0.3f, 1.f}, "WARNING: High kick risk! Use at own risk.");
  ImGui::TextColored({0.5f, 0.8f, 1, 1}, "Floods target with garbage RPCs to lag/crash them.");
  ImGui::SliderInt("Strength##overload", &g_overloadStrength, 1, 100, "%d RPCs");
  if (Game::isInGame) {
    for (int i = 0; i < (int)Game::players.size(); i++) {
      const auto &p = Game::players[i];
      if (p.isDead) continue;
      ImGui::PushID(6000 + i);
      if (GlowBtn(("Overload " + p.name).c_str(), {200, 24})) {
        Game::OverloadPlayer(p.playerId, g_overloadStrength);
        PushToast("Overloading " + p.name, IM_COL32(255, 80, 80, 245), 2.0f);
        Game::LogEvent("Overload -> " + p.name, IM_COL32(255, 80, 80, 255));
      }
      ImGui::PopID();
    }
  } else {
    ImGui::TextColored({1, 0.3f, 0.3f, 1}, "Not in game");
  }
  EndCard();

  Card("Map-Aware Sabotage");
  {
    int mapId = Game::GetCurrentMapId();
    const char *mapNames[] = {"Skeld", "MiraHQ", "Polus", "dlekS", "Airship", "Fungle"};
    ImGui::TextColored({0.4f, 0.85f, 1, 1}, "Current Map: %s",
                       (mapId >= 0 && mapId <= 5) ? mapNames[mapId] : "Unknown");
    ImGui::Spacing();
    if (GlowBtn("Reactor (Auto)", {140, 28}))
      Game::TriggerSabotageMapAware(0);
    ImGui::SameLine();
    if (GlowBtn("Oxygen (Auto)", {140, 28}))
      Game::TriggerSabotageMapAware(1);
    if (GlowBtn("Lights (Auto)", {140, 28}))
      Game::TriggerSabotageMapAware(2);
    ImGui::SameLine();
    if (GlowBtn("Comms (Auto)", {140, 28}))
      Game::TriggerSabotageMapAware(3);
    if (GlowBtn("Spores (Fungle)", {160, 28}))
      Game::TriggerSpores();
    ImGui::SameLine();
    if (GlowBtn("Open All Doors", {160, 28}))
      Game::OpenAllDoors();
    Toggle("Spam Doors (Auto Close)", &g_spamDoors);
  }
  EndCard();

  Card("Animation Framework");
  if (Toggle("Moonwalk", &g_moonwalk))
    Game::SetMoonwalk(g_moonwalk);
  if (Toggle("MedScan Animation", &g_medScan))
    Game::ToggleMedScan(g_medScan);
  if (Toggle("Show Cams In Use", &g_camsInUse))
    Game::ToggleCamsInUse(g_camsInUse);
  ImGui::Spacing();
  ImGui::TextColored({0.5f, 0.8f, 1, 1}, "Task Animations:");
  if (GlowBtn("Shields", {90, 24}))
    Game::PlayTaskAnimation(4);
  ImGui::SameLine();
  if (GlowBtn("Asteroids", {90, 24}))
    Game::PlayTaskAnimation(5);
  ImGui::SameLine();
  if (GlowBtn("Garbage", {90, 24}))
    Game::PlayTaskAnimation(6);
  EndCard();
}

static void TabCosmetics() {
  Card("Character Preview");
  ImDrawList *dl = ImGui::GetWindowDrawList();
  ImVec2 p = ImGui::GetCursorScreenPos();
  // Draw Among Us character silhouette
  float cx = p.x + 60, cy = p.y + 70;
  float t = (float)ImGui::GetTime();
  if (g_spin)
    cx += sin(t * 2) * 5;
  ImU32 bodyCol =
      IM_COL32((int)(g_playerCol[0] * 255), (int)(g_playerCol[1] * 255),
               (int)(g_playerCol[2] * 255), 255);
  dl->AddRectFilled({cx - 20, cy - 30}, {cx + 20, cy + 25}, bodyCol,
                    12); // body
  dl->AddRectFilled({cx - 25, cy - 10}, {cx - 18, cy + 15}, bodyCol,
                    4); // backpack
  dl->AddRectFilled({cx - 15, cy - 25}, {cx + 15, cy - 10},
                    IM_COL32(180, 220, 255, 200), 8); // visor
  dl->AddRectFilled({cx - 12, cy + 25}, {cx - 2, cy + 40}, bodyCol,
                    3); // left leg
  dl->AddRectFilled({cx + 2, cy + 25}, {cx + 12, cy + 40}, bodyCol,
                    3); // right leg
  ImGui::Dummy({120, 100});
  EndCard();
  Card("Cosmetics");
  const char *hats[] = {
      "None",        "Crown",       "Top Hat",      "Beanie",
      "Horns",       "Flowerpot",   "Antenna",      "Balloon",
      "Bird",        "Captain",     "Double Top",   "Fez",
      "General",     "Goggles",     "Hard Hat",     "Military",
      "Paper",       "Party Hat",   "Police",       "Stethoscope",
      "Sticky Note", "Viking",      "Wall",         "Snowman",
      "Reindeer",    "Lights",      "Tree",         "Santa",
      "Candy",       "Elf Hat",     "New Year 2018","White Hat",
      "Wolf",        "Bush",        "Geoff",        "Purple Traffic",
      "Holiday 2018"};
  constexpr int NUM_HATS = sizeof(hats) / sizeof(hats[0]);
  const char *pets[] = {
      "None",        "Mini Crewmate","Dog",          "Cat",
      "Robot",       "Hamster",     "UFO",          "Ellie",
      "Squig",       "Bedcrab",     "Glitch",       "Brainslug",
      "Test",        "Bush",        "Lava",         "Snow",
      "Charles",     "Chewie",      "Clank",        "Frankendog"};
  constexpr int NUM_PETS = sizeof(pets) / sizeof(pets[0]);
  const char *skins[] = {
      "None",        "Suit",        "Astronaut",    "Military",
      "Mech",        "Police",      "Science",      "Suit B",
      "Tarmac",      "Captain",     "Miner",        "Winter",
      "Archaeologist","Security",   "Hazmat",       "Prisoner",
      "CCC",         "Elf",         "D2 Normal",    "Moose"};
  constexpr int NUM_SKINS = sizeof(skins) / sizeof(skins[0]);
  const char *visors[] = {"None", "Lollipop (Crew)", "Lollipop (Imp)",
                          "Star (Crew)", "Angery"};
  const char *nameplates[] = {"None", "Toppat", "CCC", "Government", "Yard"};
  if (ImGui::Combo("Hat##c", &g_hat, hats, NUM_HATS))
    Game::SetHat(g_hat);
  if (ImGui::Combo("Pet##c", &g_pet, pets, NUM_PETS))
    Game::SetPet(g_pet);
  if (ImGui::Combo("Skin##c", &g_skin, skins, NUM_SKINS))
    Game::SetSkin(g_skin);
  static int g_visor = 0;
  if (ImGui::Combo("Visor##c", &g_visor, visors, 5))
    Game::SetVisor(g_visor);
  static int g_nameplate = 0;
  if (ImGui::Combo("Name Plate##c", &g_nameplate, nameplates, 5))
    Game::SetNamePlate(g_nameplate);
  EndCard();

  Card("Color");
  static int g_colorId = 0;
  const char *colors[] = {"Red",    "Blue",  "Green",  "Pink",   "Orange",
                          "Yellow", "Black", "White",  "Purple", "Brown",
                          "Cyan",   "Lime",  "Maroon", "Rose",   "Banana",
                          "Gray",   "Tan",   "Coral"};
  if ((Game::isInGame || Game::isInLobby) && Game::localColorId >= 0 &&
      Game::localColorId < 18) {
    ImGui::TextColored({.75f, .85f, 1, 1}, "Current Color: %s",
                       colors[Game::localColorId]);
  }
  if (ImGui::Combo("Body Color##c", &g_colorId, colors, 18))
    Game::SetPlayerColor(g_colorId);
  EndCard();
}

static void TabSettings() {
  Card("Theme");
  ImGui::ColorEdit4("Accent Color", g_accentCol, ImGuiColorEditFlags_NoInputs);
  Toggle("RGB Accent##s", &g_rgbAccent);
  Slider("UI Scale##s", &g_uiScale, 0.5f, 2);
  Slider("Blur Intensity##s", &g_blurInt, 0, 1);
  Slider("Anim Speed##s", &g_animSpeed, 0.1f, 3);
  EndCard();
  Card("Display");
  Toggle("FPS Counter", &g_fpsDisp);
  EndCard();
  Card("Config");
  if (GlowBtn("Save Config", {120, 28})) {
    // Implementation: Serialize all static globals to a simple .cfg file
    FILE *f = fopen("stara.cfg", "wb");
    if (f) {
      fwrite(&g_speed, sizeof(float), 1, f);
      fwrite(&g_fov, sizeof(float), 1, f);
      // ... (keeping it simple for now)
      fclose(f);
      PushToast("Config saved", IM_COL32(86, 220, 160, 245), 2.2f);
    } else {
      PushToast("Config save failed", IM_COL32(240, 110, 110, 245), 3.0f);
    }
  }
  ImGui::SameLine();
  if (GlowBtn("Load Config", {120, 28})) {
    FILE *f = fopen("stara.cfg", "rb");
    if (f) {
      fread(&g_speed, sizeof(float), 1, f);
      fread(&g_fov, sizeof(float), 1, f);
      fclose(f);
      Game::SetSpeed(g_speed);
      PushToast("Config loaded", IM_COL32(110, 205, 255, 245), 2.2f);
    } else {
      PushToast("No config file found", IM_COL32(255, 178, 95, 245), 3.0f);
    }
  }
  ImGui::SameLine();
  if (GlowBtn("Reset", {80, 28})) {
    g_speed = 2.5f;
    g_walkSpeed = 2.5f;
    g_fov = 90.0f;
    Game::SetSpeed(2.5f);
    PushToast("Defaults restored", IM_COL32(110, 205, 255, 245), 2.2f);
  }
  EndCard();
  Card("Keybinds");
  ImGui::Text("Toggle Menu: INSERT");
  ImGui::Text("Unload DLL: END");
  ImGui::Text("Quick Tabs: 1-9");
  ImGui::Text("Panic Mode: F12");
  EndCard();

  Card("Panic Mode");
  ImGui::TextColored({1.f, 0.3f, 0.3f, 1.f}, "Instantly disables ALL cheats");
  if (GlowBtn("PANIC - Disable All", {220, 34})) {
    g_panicMode = true;
    // Reset all toggle globals
    g_noclip = false; g_fullbright = false; g_wallhack = false;
    g_noKillCd = false; g_godmode = false; g_infiniteEmergencies = false;
    g_alwaysMoveable = false; g_impostorVision = false; g_maxReportDist = false;
    g_autoTasks = false; g_antiKick = false; g_forceProtect = false;
    g_freezeAll = false; g_rainbow = false; g_spin = false; g_dance = false;
    g_chatSpam = false; g_colorCycle = false; g_spamAnim = false;
    g_autoPath = false; g_walkInVents = false; g_useVents = false;
    g_seeGhosts = false; g_alwaysChat = false; g_killReach = false;
    g_unfixableLights = false; g_moonwalk = false; g_medScan = false;
    g_camsInUse = false; g_espBox = false; g_espName = false;
    g_espDist = false; g_espRole = false; g_espTracer = false;
    g_tracerCrew = false; g_tracerImp = false; g_tracerGhost = false;
    g_tracerBodies = false; g_minimapEsp = false; g_stealthMode = false;
    g_noGameEnd = false; g_spamDoors = false;
    // Call game-side cleanup
    Game::PanicDisableAll();
    Game::SetFullbright(false);
    Game::SetWallhack(false);
    Game::SetMoonwalk(false);
    if (g_noGameEnd) Game::SetNoGameEnd(false);
    g_walkSpeed = 2.5f; g_speed = 2.5f;
    Game::SetSpeed(2.5f);
    PushToast("PANIC: All cheats disabled!", IM_COL32(255, 60, 60, 255), 3.5f);
    Game::LogEvent("PANIC MODE ACTIVATED", IM_COL32(255, 60, 60, 255));
    g_panicMode = false;
  }
  EndCard();

  Card("QoL Features");
  if (Toggle("No Game End (Host)", &g_noGameEnd))
    Game::SetNoGameEnd(g_noGameEnd);
  ImGui::TextColored({0.5f, 0.8f, 1, 1}, "Prevents game from ending (host only)");
  Toggle("Invert Controls", &g_invertControls);
  Toggle("Free Cosmetics (Visual Only)", &g_freeCosmetics);
  ImGui::TextColored({0.5f, 0.8f, 1, 1}, "Cosmetics appear free in selector (local only)");
  EndCard();

  Card("Event Log");
  Toggle("Show Event Log", &g_eventLogVisible);
  if (g_eventLogVisible && !Game::eventLog.empty()) {
    ImGui::BeginChild("##eventLogScroll", {0, 180}, true);
    for (const auto &e : Game::eventLog) {
      ImVec4 col = ImGui::ColorConvertU32ToFloat4(e.color);
      ImGui::TextColored(col, "[%.1fs] %s", e.timestamp, e.text.c_str());
    }
    if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
      ImGui::SetScrollHereY(1.0f);
    ImGui::EndChild();
    if (GlowBtn("Clear Log", {100, 24}))
      Game::eventLog.clear();
  } else if (g_eventLogVisible) {
    ImGui::TextColored({0.6f, 0.6f, 0.7f, 1.f}, "No events logged yet.");
  }
  EndCard();
}

// ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ Apply Theme
// ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬
static void ApplyTheme() {
  ImGuiStyle &s = ImGui::GetStyle();
  s.WindowPadding = {14, 10};
  s.FramePadding = {10, 5};
  s.ItemSpacing = {8, 5};
  s.ItemInnerSpacing = {7, 4};
  s.ScrollbarSize = 8;
  s.IndentSpacing = 18.f;
  s.WindowBorderSize = 0.8f;
  s.ChildBorderSize = 0.6f;
  s.FrameBorderSize = 0.f;
  s.WindowRounding = 12.f;
  s.ChildRounding = 10.f;
  s.FrameRounding = 8.f;
  s.PopupRounding = 10.f;
  s.ScrollbarRounding = 12.f;
  s.GrabRounding = 8.f;
  s.TabRounding = 8.f;
  ImVec4 *c = s.Colors;
  c[ImGuiCol_WindowBg] = {.039f, .055f, .098f, .95f};
  c[ImGuiCol_ChildBg] = {0.f, 0.f, 0.f, 0.f};
  c[ImGuiCol_PopupBg] = {.035f, .048f, .082f, .97f};
  c[ImGuiCol_Border] = {.118f, .161f, .231f, .45f};
  c[ImGuiCol_FrameBg] = {.055f, .07f, .12f, .82f};
  c[ImGuiCol_FrameBgHovered] = {.07f, .09f, .15f, .90f};
  c[ImGuiCol_FrameBgActive] = {.085f, .11f, .18f, 1.f};
  c[ImGuiCol_TitleBg] = {.025f, .035f, .065f, .98f};
  c[ImGuiCol_TitleBgActive] = {.035f, .048f, .08f, .98f};
  c[ImGuiCol_ScrollbarBg] = {.025f, .035f, .06f, .4f};
  c[ImGuiCol_ScrollbarGrab] = {.2f, .26f, .38f, .55f};
  c[ImGuiCol_ScrollbarGrabHovered] = {.28f, .36f, .5f, .7f};
  c[ImGuiCol_ScrollbarGrabActive] = {.35f, .44f, .6f, .85f};
  c[ImGuiCol_Text] = {.886f, .91f, .94f, 1.f};
  c[ImGuiCol_TextDisabled] = {.58f, .64f, .72f, .8f};
  ImVec4 ac = Accent();
  c[ImGuiCol_CheckMark] = ac;
  c[ImGuiCol_SliderGrab] = ac;
  c[ImGuiCol_SliderGrabActive] = {ac.x * 1.15f, ac.y * 1.15f, ac.z * 1.15f, 1.f};
  c[ImGuiCol_Button] = {ac.x * .12f, ac.y * .12f, ac.z * .14f, .85f};
  c[ImGuiCol_ButtonHovered] = {ac.x * .22f, ac.y * .22f, ac.z * .26f, .95f};
  c[ImGuiCol_ButtonActive] = {ac.x * .34f, ac.y * .32f, ac.z * .38f, 1.f};
  c[ImGuiCol_Header] = {ac.x * .12f, ac.y * .12f, ac.z * .14f, .6f};
  c[ImGuiCol_HeaderHovered] = {ac.x * .18f, ac.y * .18f, ac.z * .22f, .8f};
  c[ImGuiCol_HeaderActive] = {ac.x * .25f, ac.y * .25f, ac.z * .3f, .95f};
  c[ImGuiCol_Separator] = {.12f, .16f, .24f, .4f};
  c[ImGuiCol_TableHeaderBg] = {.05f, .065f, .1f, .9f};
  c[ImGuiCol_TableBorderStrong] = {.15f, .2f, .3f, .5f};
  c[ImGuiCol_TableBorderLight] = {.1f, .14f, .22f, .4f};
  c[ImGuiCol_TableRowBg] = {.03f, .04f, .065f, .12f};
  c[ImGuiCol_TableRowBgAlt] = {.04f, .055f, .085f, .18f};
  c[ImGuiCol_ResizeGrip] = {ac.x * .15f, ac.y * .15f, ac.z * .18f, .4f};
  c[ImGuiCol_ResizeGripHovered] = {ac.x * .28f, ac.y * .28f, ac.z * .34f, .7f};
  c[ImGuiCol_ResizeGripActive] = {ac.x * .38f, ac.y * .38f, ac.z * .45f, .95f};
}

static bool TabMatchesSearch(Tab t) {
  if (!g_tabSearch[0])
    return true;
  std::string q = g_tabSearch;
  return ContainsNoCase(TabName(t), q) || ContainsNoCase(TabTag(t), q) ||
         ContainsNoCase(TabSubtitle(t), q);
}

static void DrawTabHero(ImDrawList *dl, const ImVec2 &cursor, float width) {
  const char *title = TabName(g_tab);
  const char *subtitle = TabSubtitle(g_tab);
  ImU32 tone = TabTone(g_tab, 200);
  float heroH = 48.f;
  ImVec2 min = cursor;
  ImVec2 max = {cursor.x + width, cursor.y + heroH};

  // Clean dark panel
  dl->AddRectFilled(min, max, IM_COL32(12, 16, 28, 160), 8.f);

  // Subtle left accent bar
  dl->AddRectFilled({min.x, min.y + 4.f}, {min.x + 2.5f, max.y - 4.f},
                    tone, 2.f);

  // Title + subtitle
  dl->AddText({min.x + 14.f, min.y + 8.f}, IM_COL32(226, 232, 240, 255), title);
  dl->AddText({min.x + 14.f, min.y + 26.f}, IM_COL32(140, 160, 190, 200),
              subtitle);

  // Tab icon glyph on far right
  ImVec2 iconCenter = {max.x - 22.f, (min.y + max.y) * 0.5f};
  DrawTabGlyph(dl, g_tab, iconCenter, 11.f, tone, 1.1f);
}

// ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ Main Render
// ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬
void RenderMenu() {
  float dt = ImGui::GetIO().DeltaTime;
  ImVec2 displaySize = ImGui::GetIO().DisplaySize;
  if (displaySize.x < 10.f || displaySize.y < 10.f)
    displaySize = {1920.f, 1080.f};

  // Quick tab hotkeys (1-9) when menu is open.
  for (int i = 0; i < (int)Tab::Count && i < 9; i++) {
    if (GetAsyncKeyState('1' + i) & 1)
      g_tab = (Tab)i;
  }

  // RGB hue
  if (g_rgbAccent) {
    g_hue += dt * 2;
    if (g_hue > 6.28f)
      g_hue -= 6.28f;
  }

  // Menu fade
  float tgt = g_menuVisible ? 1.f : 0.f;
  g_menuAlpha += (tgt - g_menuAlpha) * std::min(1.f, 8.f * dt);
  if (g_menuAlpha < 0.01f)
    return;

  if (g_tab != g_lastRenderedTab) {
    g_lastRenderedTab = g_tab;
    g_tabContentFade = 0.f;
  }
  g_tabContentFade = std::min(1.f, g_tabContentFade + dt * 8.f);

  ApplyTheme();
  ImGui::PushStyleVar(ImGuiStyleVar_Alpha, g_menuAlpha);

  // FPS
  if (g_fpsDisp) {
    float safeDt = (dt > 0.0001f) ? dt : 0.0001f;
    ImGui::GetForegroundDrawList()->AddText(
        {10, 10}, IM_COL32(0, 220, 255, (int)(200 * g_menuAlpha)),
        (std::string("FPS: ") + std::to_string((int)(1.f / safeDt))).c_str());
  }

  // Clean background
  {
    ImDrawList *bg = ImGui::GetBackgroundDrawList();
    ImVec4 ac = Accent();
    DrawCleanBackground(bg, displaySize, g_menuAlpha, ac);
  }

  // Main window
  float scaledMinW = 760.f * g_uiScale;
  float scaledMinH = 500.f * g_uiScale;
  ImVec2 minSize = {std::clamp(scaledMinW, 680.f, displaySize.x * 0.9f),
                    std::clamp(scaledMinH, 460.f, displaySize.y * 0.9f)};
  ImVec2 maxSize = {std::max(minSize.x, displaySize.x * 0.95f),
                    std::max(minSize.y, displaySize.y * 0.95f)};
  ImGui::SetNextWindowSizeConstraints(minSize, maxSize);
  ImGui::SetNextWindowSize({980.f * g_uiScale, 600.f * g_uiScale},
                           ImGuiCond_FirstUseEver);
  ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar |
                           ImGuiWindowFlags_NoScrollbar |
                           ImGuiWindowFlags_NoCollapse;

  if (ImGui::Begin("##StaraMain", nullptr, flags)) {
    ImGui::SetWindowFontScale(std::clamp(g_uiScale, 0.75f, 1.35f));
    ImDrawList *dl = ImGui::GetWindowDrawList();
    ImVec2 wp = ImGui::GetWindowPos(), ws = ImGui::GetWindowSize();
    ImVec4 ac = Accent();
    float t = (float)ImGui::GetTime();

    // Subtle static border (no pulsing)
    dl->AddRect(wp, {wp.x + ws.x, wp.y + ws.y},
                IM_COL32((int)(ac.x * 100 + 20), (int)(ac.y * 100 + 20),
                         (int)(ac.z * 110 + 25), (int)(40 * g_menuAlpha)),
                12.f, 0, 0.8f);

    // Title bar — clean, dark
    dl->AddRectFilled(wp, {wp.x + ws.x, wp.y + 40}, IM_COL32(10, 14, 22, 235),
                      12, ImDrawFlags_RoundCornersTop);

    // Top accent line — single color gradient fade
    dl->AddRectFilledMultiColor(
        wp, {wp.x + ws.x, wp.y + 1.5f},
        IM_COL32((int)(ac.x * 255), (int)(ac.y * 255), (int)(ac.z * 255), 140),
        IM_COL32((int)(ac.x * 150), (int)(ac.y * 150), (int)(ac.z * 160), 60),
        IM_COL32(0, 0, 0, 0), IM_COL32(0, 0, 0, 0));

    // Title text
    dl->AddText({wp.x + 14, wp.y + 11}, IM_COL32(226, 232, 240, 255), "STARA");
    dl->AddText({wp.x + ws.x - 100, wp.y + 11}, IM_COL32(100, 120, 150, 180),
                APP_VERSION);

    EnsureBoltIconLoaded();
    ImVec2 settingsPos = {wp.x + ws.x - 40.f, wp.y + 9.f};
    ImGui::SetCursorScreenPos(settingsPos);
    ImGui::PushID("header_settings_bolt");
    bool headerBoltPressed = ImGui::InvisibleButton("##settingsBolt", {26, 26});
    bool headerBoltHover = ImGui::IsItemHovered();
    ImGui::PopID();
    ImRect hdrBtn(ImGui::GetItemRectMin(), ImGui::GetItemRectMax());
    dl->AddRectFilled(hdrBtn.Min, hdrBtn.Max,
                      headerBoltHover ? IM_COL32(22, 34, 52, 245)
                                      : IM_COL32(14, 24, 38, 220),
                      7.f);
    dl->AddRect(hdrBtn.Min, hdrBtn.Max, IM_COL32(84, 132, 188, 170), 7.f, 0,
                1.2f);
    if (g_boltIcon.srv) {
      dl->AddImage((ImTextureID)g_boltIcon.srv,
                   {hdrBtn.Min.x + 5.f, hdrBtn.Min.y + 4.f},
                   {hdrBtn.Max.x - 5.f, hdrBtn.Max.y - 4.f});
    } else {
      DrawBoltFallback(dl,
                       {(hdrBtn.Min.x + hdrBtn.Max.x) * 0.5f,
                        (hdrBtn.Min.y + hdrBtn.Max.y) * 0.5f},
                       15.f, IM_COL32(230, 246, 255, 240));
    }
    if (headerBoltPressed)
      g_tab = Tab::Settings;

    // Live status chips
    char chipGame[32];
    char chipPlayers[32];
    snprintf(chipGame, sizeof(chipGame), "Game: %s",
             Game::isInGame ? "In Match" : (Game::isInLobby ? "Lobby" : "Idle"));
    snprintf(chipPlayers, sizeof(chipPlayers), "Players: %d",
             (int)Game::players.size());
    float chipX = wp.x + 250.f;
    float chipY = wp.y + 11.f;
    chipX += DrawStatusChip(dl, {chipX, chipY}, chipGame, Colors::TextPrimary,
                            (Game::isInGame || Game::isInLobby) ? IM_COL32(18, 72, 46, 200)
                                           : IM_COL32(60, 66, 92, 200),
                            (Game::isInGame || Game::isInLobby) ? IM_COL32(70, 170, 110, 170)
                                           : IM_COL32(96, 110, 156, 150));
    chipX += 8.f;
    chipX +=
        DrawStatusChip(dl, {chipX, chipY}, chipPlayers, Colors::TextPrimary,
                       IM_COL32(20, 36, 58, 200), IM_COL32(82, 128, 170, 150));
    if (Game::isInMeeting) {
      chipX += 8.f;
      DrawStatusChip(dl, {chipX, chipY}, "Meeting",
                     IM_COL32(255, 220, 220, 255), IM_COL32(102, 28, 34, 215),
                     IM_COL32(194, 84, 94, 170));
    }

    // Footer — minimal
    dl->AddRectFilled({wp.x, wp.y + ws.y - 20}, {wp.x + ws.x, wp.y + ws.y},
                      IM_COL32(8, 10, 16, 200), 10,
                      ImDrawFlags_RoundCornersBottom);
    dl->AddText({wp.x + 12, wp.y + ws.y - 15}, IM_COL32(80, 95, 120, 160),
                "STARA | github.com/ItzE1ectric/Stara");

    ImGui::SetCursorPosY(46);

    // Sidebar + Content
    float sidebarWidth = std::clamp(178.f * g_uiScale, 170.f, 240.f);
    ImGui::BeginChild("##sidebar", {sidebarWidth, ws.y - 70}, false);
    ImGui::Spacing();
    ImGui::SetNextItemWidth(-6.f);
    ImGui::InputTextWithHint("##tabSearch", "Search tabs...", g_tabSearch,
                             (int)std::size(g_tabSearch));
    ImGui::Spacing();
    int visibleTabs = 0;
    for (int i = 0; i < (int)Tab::Count; i++)
      if (TabMatchesSearch((Tab)i))
        visibleTabs++;
    ImGui::TextColored({0.53f, 0.67f, 0.83f, 0.9f}, "%d tabs", visibleTabs);
    ImGui::Separator();
    for (int i = 0; i < (int)Tab::Count; i++) {
      Tab t = (Tab)i;
      if (!TabMatchesSearch(t))
        continue;
      bool sel = (g_tab == t);
      float itemH = 36.f;
      float itemW = sidebarWidth - 6.f;
      ImVec2 cp = ImGui::GetCursorScreenPos();
      ImDrawList *sdl = ImGui::GetWindowDrawList();
      bool preHover =
          ImGui::IsMouseHoveringRect(cp, {cp.x + itemW, cp.y + itemH});
      float &ha = g_hoverAnim[i];
      ha = Damp(ha, (sel || preHover) ? 1.f : 0.f, 14.f);
      ha = std::clamp(ha, 0.f, 1.f);

      ImGui::PushID(4000 + i);
      bool pressed = ImGui::InvisibleButton("##tabItem", {itemW, itemH});
      bool hovered = ImGui::IsItemHovered();
      ImGui::PopID();
      if (pressed) {
        g_tab = t;
        PushToast(std::string("Opened ") + TabName(t), TabTone(t, 245), 1.1f);
      }

      ImU32 tone = TabTone(t, sel ? 240 : (hovered ? 210 : 172));
      if (ha > 0.01f || sel) {
        int a = sel ? (int)(55 + 35 * ha) : (int)(35 * ha);
        sdl->AddRectFilled(cp, {cp.x + itemW, cp.y + itemH},
                           IM_COL32((int)(ac.x * 255 * .08f),
                                    (int)(ac.y * 255 * .08f),
                                    (int)(ac.z * 255 * .10f), a),
                           8.f);
      }
      if (sel) {
        sdl->AddRectFilled({cp.x, cp.y + 6.f}, {cp.x + 2.5f, cp.y + itemH - 6.f},
                           tone, 2.f);
      }

      ImVec2 iconMin = {cp.x + 8.f, cp.y + 6.f};
      ImVec2 iconMax = {cp.x + 28.f, cp.y + itemH - 6.f};
      sdl->AddRectFilled(iconMin, iconMax,
                         IM_COL32((int)((tone >> IM_COL32_R_SHIFT) & 0xFF),
                                  (int)((tone >> IM_COL32_G_SHIFT) & 0xFF),
                                  (int)((tone >> IM_COL32_B_SHIFT) & 0xFF),
                                  sel ? 40 : 20),
                         5.f);
      DrawTabGlyph(
          sdl, t,
          {(iconMin.x + iconMax.x) * 0.5f, (iconMin.y + iconMax.y) * 0.5f},
          11.f, sel ? IM_COL32(235, 245, 255, 250) : tone, 1.1f);

      ImU32 titleCol = sel ? IM_COL32(226, 232, 240, 255)
                           : (hovered ? IM_COL32(200, 215, 235, 240)
                                      : IM_COL32(160, 180, 210, 220));
      ImU32 hintCol =
          sel ? IM_COL32(140, 175, 215, 220) : IM_COL32(110, 140, 175, 180);
      sdl->AddText({cp.x + 34.f, cp.y + 4.f}, titleCol, TabName(t));
      sdl->AddText({cp.x + 34.f, cp.y + 19.f}, hintCol, TabSidebarHint(t));
    }
    ImGui::EndChild();

    ImGui::SameLine();

    // Separator line
    {
      ImVec2 sp = ImGui::GetCursorScreenPos();
      dl->AddLine({sp.x, wp.y + 50}, {sp.x, wp.y + ws.y - 24},
                  IM_COL32(50, 50, 80, 100));
    }

    ImGui::SameLine();

    // Content
    ImGui::BeginChild("##content", {0, ws.y - 70}, false);
    ImGui::Spacing();
    {
      ImVec2 heroPos = ImGui::GetCursorScreenPos();
      float heroW = std::max(260.f, ImGui::GetContentRegionAvail().x);
      DrawTabHero(dl, heroPos, heroW);
      ImGui::Dummy({heroW, 52.f});
      ImGui::Spacing();
    }
    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, g_tabContentFade);

    switch (g_tab) {
    case Tab::Dashboard:
      TabDashboard();
      break;
    case Tab::Player:
      TabPlayer();
      break;
    case Tab::Visuals:
      TabVisuals();
      break;
    case Tab::ESP:
      TabESP();
      break;
    case Tab::Movement:
      TabMovement();
      break;
    case Tab::Fun:
      TabFun();
      break;
    case Tab::Troll:
      TabTroll();
      break;
    case Tab::Cosmetics:
      TabCosmetics();
      break;
    case Tab::Settings:
      TabSettings();
      break;
    default:
      break;
    }

    ImGui::PopStyleVar();
    ImGui::EndChild();
  }
  ImGui::End();
  DrawToasts(g_menuAlpha);

  ImGui::PopStyleVar();
}

} // namespace Stara
