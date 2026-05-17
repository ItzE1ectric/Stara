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

// â”€â”€ State
// â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
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
bool g_espTracer = false, g_espOutline = true, g_espTask = false;
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
float g_customDiscussTime = 15.f, g_customVoteTime = 120.f;
char g_chatBuf[128] = "Stara Client";
static float g_hue = 0.f;

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
      L"C:\\Users\\xklyo\\Downloads\\bolt.png",
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
      L"C:\\Users\\xklyo\\Downloads\\discord_pfp.png",
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

struct StarParticle {
  float x, y;
  float driftX, driftY;
  float size;
  float twinkleSpeed;
  float twinklePhase;
  float depth;
};
struct ShootingStar {
  bool active = false;
  float x = 0.f, y = 0.f;
  float vx = 0.f, vy = 0.f;
  float life = 0.f, maxLife = 0.f;
  float thickness = 1.f;
};
static std::array<StarParticle, 180> g_stars = {};
static std::array<ShootingStar, 5> g_shootingStars = {};
static bool g_starfieldInit = false;
static ImVec2 g_starfieldSize = {0.f, 0.f};
static float g_nextShoot = 2.4f;
static float Randf(float minV, float maxV) {
  return minV + (maxV - minV) * ((float)rand() / (float)RAND_MAX);
}
static void InitStarfield(const ImVec2 &screen) {
  g_starfieldInit = true;
  g_starfieldSize = screen;
  for (auto &s : g_stars) {
    s.x = Randf(0.f, screen.x);
    s.y = Randf(0.f, screen.y);
    s.driftX = Randf(-2.6f, 2.6f);
    s.driftY = Randf(-5.2f, -0.3f);
    s.size = Randf(0.8f, 2.3f);
    s.twinkleSpeed = Randf(0.8f, 3.4f);
    s.twinklePhase = Randf(0.f, 6.283185f);
    s.depth = Randf(0.45f, 1.9f);
  }
  for (auto &st : g_shootingStars) {
    st = ShootingStar{};
  }
}
static void SpawnShootingStar(const ImVec2 &screen) {
  for (auto &st : g_shootingStars) {
    if (st.active)
      continue;
    st.active = true;
    st.x = Randf(-220.f, screen.x * 0.55f);
    st.y = Randf(10.f, screen.y * 0.32f);
    st.vx = Randf(420.f, 850.f);
    st.vy = Randf(90.f, 210.f);
    st.maxLife = Randf(0.7f, 1.35f);
    st.life = st.maxLife;
    st.thickness = Randf(1.2f, 2.4f);
    return;
  }
}
static void DrawSpaceBackground(ImDrawList *dl, const ImVec2 &screen, float dt,
                                float menuAlpha, const ImVec4 &accent) {
  if (!g_starfieldInit || fabsf(g_starfieldSize.x - screen.x) > 2.f ||
      fabsf(g_starfieldSize.y - screen.y) > 2.f) {
    InitStarfield(screen);
  }
  float t = (float)ImGui::GetTime();
  ImU32 topA = IM_COL32(5, 8, 20, (int)(220 * menuAlpha));
  ImU32 topB = IM_COL32(9, 16, 34, (int)(220 * menuAlpha));
  ImU32 botA = IM_COL32(2, 4, 12, (int)(255 * menuAlpha));
  ImU32 botB = IM_COL32(5, 10, 22, (int)(255 * menuAlpha));
  dl->AddRectFilledMultiColor({0, 0}, screen, topA, topB, botB, botA);
  float nebulaPulse = 0.5f + 0.5f * sinf(t * 0.27f);
  int nebulaA = (int)(46 * nebulaPulse * menuAlpha);
  dl->AddCircleFilled({screen.x * 0.18f, screen.y * 0.22f}, screen.y * 0.23f,
                      IM_COL32((int)(accent.x * 100), (int)(accent.y * 130),
                               (int)(accent.z * 190), nebulaA),
                      64);
  dl->AddCircleFilled({screen.x * 0.83f, screen.y * 0.70f}, screen.y * 0.28f,
                      IM_COL32(18, 48, 90, (int)(38 * menuAlpha)), 64);
  for (size_t i = 0; i < g_stars.size(); i++) {
    auto &s = g_stars[i];
    float driftScale = dt * (28.f / s.depth);
    s.x += s.driftX * driftScale;
    s.y += s.driftY * driftScale;
    if (s.x < -4.f)
      s.x = screen.x + 4.f;
    if (s.x > screen.x + 4.f)
      s.x = -4.f;
    if (s.y < -4.f)
      s.y = screen.y + 4.f;
    if (s.y > screen.y + 4.f)
      s.y = -4.f;
    float tw =
        0.35f +
        0.65f * (0.5f + 0.5f * sinf(t * s.twinkleSpeed + s.twinklePhase));
    float bright = tw * menuAlpha;
    int alpha = (int)(65 + bright * 155);
    ImU32 col = IM_COL32((int)(180 + accent.x * 65), (int)(190 + accent.y * 55),
                         (int)(215 + accent.z * 40), alpha);
    float radius = s.size * (1.75f - s.depth * 0.42f);
    dl->AddCircleFilled({s.x, s.y}, radius, col, 10);
    if ((i % 13) == 0 && bright > 0.7f) {
      float arm = radius * 4.2f;
      ImU32 sparkle = IM_COL32(220, 245, 255, (int)(95 * bright));
      dl->AddLine({s.x - arm, s.y}, {s.x + arm, s.y}, sparkle, 1.f);
      dl->AddLine({s.x, s.y - arm}, {s.x, s.y + arm}, sparkle, 1.f);
    }
  }
  for (int i = 0; i < (int)g_stars.size(); i += 18) {
    const auto &a = g_stars[i];
    const auto &b = g_stars[(i + 6) % g_stars.size()];
    const auto &c = g_stars[(i + 11) % g_stars.size()];
    float abdx = a.x - b.x, abdy = a.y - b.y;
    float acdx = a.x - c.x, acdy = a.y - c.y;
    if (abdx * abdx + abdy * abdy < 44000.f) {
      dl->AddLine({a.x, a.y}, {b.x, b.y},
                  IM_COL32(120, 170, 230, (int)(25 * menuAlpha)), 1.f);
    }
    if (acdx * acdx + acdy * acdy < 44000.f) {
      dl->AddLine({a.x, a.y}, {c.x, c.y},
                  IM_COL32(120, 170, 230, (int)(18 * menuAlpha)), 1.f);
    }
  }
  g_nextShoot -= dt;
  if (g_nextShoot <= 0.f) {
    SpawnShootingStar(screen);
    g_nextShoot = Randf(2.0f, 4.6f);
  }
  for (auto &st : g_shootingStars) {
    if (!st.active)
      continue;
    st.life -= dt;
    st.x += st.vx * dt;
    st.y += st.vy * dt;
    if (st.life <= 0.f || st.x > screen.x + 280.f || st.y > screen.y + 120.f) {
      st.active = false;
      continue;
    }
    float lifeNorm = st.life / st.maxLife;
    ImVec2 tip = {st.x, st.y};
    ImVec2 tail = {st.x - st.vx * 0.11f, st.y - st.vy * 0.11f};
    for (int seg = 0; seg < 4; seg++) {
      float a0 = (float)seg / 4.f;
      float a1 = (float)(seg + 1) / 4.f;
      ImVec2 p0 = {ImLerp(tip.x, tail.x, a0), ImLerp(tip.y, tail.y, a0)};
      ImVec2 p1 = {ImLerp(tip.x, tail.x, a1), ImLerp(tip.y, tail.y, a1)};
      int a = (int)((1.f - a0) * lifeNorm * 185.f * menuAlpha);
      dl->AddLine(p0, p1, IM_COL32(180, 230, 255, a),
                  st.thickness + (1.f - a0));
    }
    dl->AddCircleFilled(
        tip, 2.6f, IM_COL32(230, 250, 255, (int)(220 * lifeNorm * menuAlpha)),
        14);
  }
}
// â”€â”€ Custom Widgets
// â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
static bool Toggle(const char *label, bool *v) {
  ImGuiWindow *w = ImGui::GetCurrentWindow();
  if (w->SkipItems)
    return false;
  ImGuiID id = w->GetID(label);
  float h = 22.f, wd = 44.f, r = h * .5f;
  ImVec2 p = w->DC.CursorPos;
  ImVec2 ls = ImGui::CalcTextSize(label);
  ImRect bb(p, ImVec2(p.x + wd + 8 + ls.x, p.y + std::max(h, ls.y)));
  ImGui::ItemSize(bb);
  if (!ImGui::ItemAdd(bb, id))
    return false;
  bool hov, held;
  bool press = ImGui::ButtonBehavior(bb, id, &hov, &held);
  if (press)
    *v = !*v;
  float &ta = g_toggleAnim[id];
  float tgt = *v ? 1.f : 0.f;
  ta = Damp(ta, tgt, 12.f);
  ImDrawList *dl = w->DrawList;
  ImVec4 ac = Accent();
  ImU32 offBg = IM_COL32(34, 40, 56, 220);
  ImU32 onBg = ScaledAccent(ac, 0.58f, 220);
  ImU32 bg = ImGui::ColorConvertFloat4ToU32(
      ImLerp(ImGui::ColorConvertU32ToFloat4(offBg),
             ImGui::ColorConvertU32ToFloat4(onBg), ta));
  dl->AddRectFilled(p, {p.x + wd, p.y + h}, bg, r);
  dl->AddRect(p, {p.x + wd, p.y + h},
              hov ? ScaledAccent(ac, 0.85f, 170) : IM_COL32(60, 70, 95, 140), r,
              0, 1.f);
  float kx = p.x + r + (wd - h) * ta;
  dl->AddCircleFilled({kx, p.y + r}, r - 2.5f,
                      IM_COL32(240, 246, 255, hov ? 255 : 238), 24);
  if (ta > 0.01f) {
    dl->AddCircle({kx, p.y + r}, r + 1.2f,
                  ScaledAccent(ac, 1.1f, (int)(110 * ta)), 24, 1.2f);
  }
  dl->AddText({p.x + wd + 8, p.y + (h - ls.y) * .5f},
              hov ? IM_COL32(250, 250, 255, 255) : Colors::TextPrimary, label);
  return press;
}

static bool GlowBtn(const char *label, ImVec2 sz = {0, 0}) {
  ImGuiID id = ImGui::GetCurrentWindow()->GetID(label);
  float &ha = g_buttonAnim[id];
  ImVec4 ac = Accent();
  ImVec4 base = {ac.x * .16f, ac.y * .16f, ac.z * .18f, .88f};
  ImVec4 hovc = {ac.x * .30f, ac.y * .28f, ac.z * .32f, .96f};
  ImVec4 actv = {ac.x * .44f, ac.y * .40f, ac.z * .46f, 1.f};
  ImGui::PushStyleColor(ImGuiCol_Button,
                        ImLerp(base, hovc, std::clamp(ha * 0.7f, 0.f, 1.f)));
  ImGui::PushStyleColor(ImGuiCol_ButtonHovered, hovc);
  ImGui::PushStyleColor(ImGuiCol_ButtonActive, actv);
  bool r = ImGui::Button(label, sz);
  bool hovered = ImGui::IsItemHovered();
  bool active = ImGui::IsItemActive();
  ha = Damp(ha, active ? 1.2f : (hovered ? 1.f : 0.f), 14.f);
  ImVec2 mn = ImGui::GetItemRectMin();
  ImVec2 mx = ImGui::GetItemRectMax();
  ImDrawList *dl = ImGui::GetWindowDrawList();
  if (ha > 0.01f) {
    int a = (int)(75.f * std::clamp(ha, 0.f, 1.2f));
    dl->AddRect({mn.x - 1.f, mn.y - 1.f}, {mx.x + 1.f, mx.y + 1.f},
                ScaledAccent(ac, 1.05f, a), 8.f, 0, 1.15f);
  }
  ImGui::PopStyleColor(3);
  return r;
}

static bool Slider(const char *l, float *v, float mn, float mx,
                   const char *fmt = "%.1f") {
  ImVec4 ac = Accent();
  ImGui::PushStyleColor(ImGuiCol_SliderGrab, ac);
  ImGui::PushStyleColor(ImGuiCol_SliderGrabActive,
                        {ac.x * 1.3f, ac.y * 1.3f, ac.z * 1.3f, 1});
  ImGui::PushStyleColor(ImGuiCol_FrameBg, {.08f, .08f, .14f, .8f});
  bool c = ImGui::SliderFloat(l, v, mn, mx, fmt);
  ImGui::PopStyleColor(3);
  return c;
}

static void Card(const char *title) {
  ImGui::PushStyleColor(ImGuiCol_ChildBg, {.05f, .07f, .12f, .82f});
  ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 12.f);
  ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 1.f);
  ImGui::BeginChild(title, {0, 0},
                    ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_Borders |
                        ImGuiChildFlags_AlwaysUseWindowPadding);
  ImDrawList *dl = ImGui::GetWindowDrawList();
  ImVec2 p = ImGui::GetWindowPos(), s = ImGui::GetWindowSize();
  ImVec4 ac = Accent();
  float t = (float)ImGui::GetTime();
  float pulse = 0.55f + 0.45f * sinf(t * 2.6f);
  dl->AddRectFilled(p, {p.x + s.x, p.y + s.y}, IM_COL32(8, 12, 20, 90), 12.f);
  dl->AddRect(p, {p.x + s.x, p.y + s.y}, IM_COL32(65, 88, 124, 105), 12.f, 0,
              1.f);
  dl->AddRectFilledMultiColor(p, {p.x + s.x, p.y + 3},
                              ScaledAccent(ac, 1.0f, (int)(140 * pulse)),
                              IM_COL32(255, 105, 148, (int)(65 * pulse)),
                              IM_COL32(255, 105, 148, (int)(65 * pulse)),
                              ScaledAccent(ac, 1.0f, (int)(140 * pulse)));
  dl->AddCircleFilled({p.x + s.x - 14.f, p.y + 12.f}, 2.1f,
                      ScaledAccent(ac, 1.15f, 180), 10);
  ImGui::TextColored({.94f, .94f, 1, 1}, "%s", title);
  ImGui::Dummy({0, 3});
  dl->AddLine({p.x + 11.f, p.y + 28.f}, {p.x + s.x - 11.f, p.y + 28.f},
              IM_COL32(56, 72, 104, 120), 1.f);
  ImGui::Dummy({0, 6});
}
static void EndCard() {
  ImGui::EndChild();
  ImGui::PopStyleVar(2);
  ImGui::PopStyleColor();
  ImGui::Dummy({0, 6});
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

// â”€â”€ Tab Renderers
// â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
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
  ImGui::Text("Player: %s", g_nameBuf);
  ImGui::SameLine(200);
  ImGui::TextColored({0, 1, .5f, 1}, "Online");
  ImGui::Text("Version: %s", APP_VERSION);
  bool dumpLoaded = DumpDatabase::IsLoaded();
  ImGui::Text("Dump.cs: %s", dumpLoaded ? "Loaded" : "Not Found");
  if (dumpLoaded) {
    ImGui::Text("Dump Index: %zu methods | %zu fields | %zu types",
                DumpDatabase::MethodCount(), DumpDatabase::FieldCount(),
                DumpDatabase::ClassCount());
  } else {
    ImGui::TextColored({1.f, 0.72f, 0.35f, 1.f},
                       "Tip: set STARA_DUMP_PATH to your dump.cs file.");
  }
  EndCard();

  Card("Live Player Info");
  if (!Game::isInGame) {
    ImGui::TextColored({1.f, 0.6f, 0.3f, 1.f}, "Not in a game session.");
  } else {
    // Role
    ImVec4 roleCol = Game::isImpostor ? ImVec4{1, .2f, .2f, 1}
                                      : ImVec4{.3f, 1, .5f, 1};
    ImGui::TextColored(roleCol, "Role: %s", Game::localRoleName.c_str());
    ImGui::SameLine(200);
    ImGui::TextColored(Game::isImpostor ? ImVec4{1, .3f, .3f, 1}
                                        : ImVec4{.5f, 1, .5f, 1},
                       Game::isImpostor ? "[IMPOSTOR]" : "[CREW]");

    // Host status
    ImGui::SameLine(300);
    if (Game::IsHost())
      ImGui::TextColored({1.f, 0.85f, 0.2f, 1.f}, "[HOST]");
    else
      ImGui::TextColored({0.5f, 0.5f, 0.6f, 1.f}, "[CLIENT]");

    // Level
    ImGui::Text("Level: %d", Game::localLevel);

    // Color
    {
      const char *colorNames[] = {"Red",    "Blue",  "Green",  "Pink",
                                  "Orange", "Yellow", "Black", "White",
                                  "Purple", "Brown",  "Cyan",  "Lime",
                                  "Maroon", "Rose",   "Banana", "Gray",
                                  "Tan",    "Coral"};
      int cid = Game::localColorId;
      if (cid >= 0 && cid < 18)
        ImGui::Text("Color: %s (%d)", colorNames[cid], cid);
      else
        ImGui::Text("Color: %d", cid);
    }

    // Position
    ImGui::Text("Position: (%.1f, %.1f)", Game::localX, Game::localY);

    // Speed
    ImGui::Text("Speed: %.1f", g_walkSpeed);

    // Meeting status
    if (Game::isInMeeting)
      ImGui::TextColored({1, .4f, .4f, 1}, "Currently in MEETING");

    // Player count
    int alive = 0, imps = 0;
    for (const auto &p : Game::players) {
      if (!p.isDead) alive++;
      if (p.isImpostor && !p.isDead) imps++;
    }
    ImGui::Text("Players: %d alive (%d impostors)",
                alive + 1, imps + (Game::isImpostor ? 1 : 0));
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
  if (Game::isInGame) {
    ImGui::TextColored({.5f, .85f, 1, 1}, "Current Level: %d | Role: %s | Pos: (%.1f, %.1f)",
                       Game::localLevel, Game::localRoleName.c_str(),
                       Game::localX, Game::localY);
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
  ImGui::TextColored({.6f, .6f, .75f, 1}, "(Current: %d)", Game::localLevel);
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
  ImGui::Combo("Set Role##r", &selRole, roles, 13);
  if (GlowBtn("Apply Role", {140, 28}))
    Game::SetRole(roleIds[selRole]);
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

  Card("Lobby Actions");
  {
    bool host = Game::IsHost();
    bool canStart = host && Game::isInLobby;
    if (!canStart)
      ImGui::BeginDisabled();
    if (GlowBtn("Force Start Game (Host)", {200, 28}))
      Game::StartGame();
    if (!canStart)
      ImGui::EndDisabled();
    if (!host)
      ImGui::TextColored({0.7f, 0.55f, 0.4f, 1}, "Start requires host");
    else if (!Game::isInLobby)
      ImGui::TextColored({0.7f, 0.55f, 0.4f, 1}, "Only works in lobby");
  }
  if (GlowBtn("Force End Game", {180, 28}))
    Game::EndGame();
  EndCard();

  Card("Tasks");
  if (GlowBtn("Complete All Tasks", {180, 28}))
    Game::CompleteAllTasks();
  EndCard();

  Card("Combat");
  Toggle("No Kill Cooldown", &g_noKillCd);
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
  Toggle("Force Shield (GA)", &g_forceProtect);
  Toggle("Freeze All Players", &g_freezeAll);
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
  Slider("FOV##v", &g_fov, 30, 160, "%.0f");
  Slider("Camera Zoom##v", &g_zoom, 0.5f, 5, "%.2f");
  Slider("Bloom##v", &g_bloom, 0, 2);
  Slider("Theme Intensity##v", &g_themeInt, 0, 2);
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
  Toggle("Tracer Lines", &g_espTracer);
  Toggle("Outline", &g_espOutline);
  Toggle("Task Markers", &g_espTask);
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
  if (Toggle("Rainbow Character", &g_rainbow)) {
  }
  if (Toggle("Spin Preview", &g_spin)) {
  }
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
  Toggle("Dance Animation", &g_dance);
  Toggle("Particle Effects", &g_particle);
  EndCard();
  Card("Emotes");
  const char *emotes[] = {"Wave", "Dance", "Clap", "Dab", "Flex"};
  for (int i = 0; i < 5; i++) {
    if (i)
      ImGui::SameLine();
    if (GlowBtn(emotes[i], {60, 28})) {
      Game::PlayAnimation((uint8_t)i);
    }
  }
  EndCard();
}

static void TabTroll() {
  Card("Server Trolls");
  Toggle("Chat Spam", &g_chatSpam);
  if (GlowBtn("Force Emergency Meeting", {200, 28}))
    Game::ForceEmergencyMeeting();
  if (GlowBtn("Force Start Game (Host)", {200, 28}))
    Game::StartGame();
  if (GlowBtn("Force End / Leave", {200, 28}))
    Game::EndGame();
  if (GlowBtn("Complete All Tasks", {200, 28}))
    Game::CompleteAllTasks();
  EndCard();

  Card("Impostor Abilities");
  if (GlowBtn("Vanish (Phantom)", {160, 28}))
    Game::Vanish();
  ImGui::SameLine();
  if (GlowBtn("Appear", {100, 28}))
    Game::Appear();
  EndCard();

  Card("Shapeshift");
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

  Card("Set Player Roles");
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
    if (GlowBtn("Set ALL to Role", {180, 28})) {
      for (int i = 0; i < (int)Game::players.size(); i++)
        Game::SetPlayerRole(Game::players[i].playerId, roleIds[massRole]);
      // Also set self
      Game::SetRole(roleIds[massRole]);
    }
    ImGui::Separator();

    // Self role
    ImGui::TextColored({0, 0.86f, 1, 1}, "You:");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(140);
    ImGui::Combo("##selfRole", &selfRole, roleNames, NUM_ROLES);
    ImGui::SameLine();
    if (GlowBtn("Set##self", {50, 22}))
      Game::SetRole(roleIds[selfRole]);

    ImGui::Separator();
    ImGui::TextColored({0.6f, 0.6f, 0.75f, 1}, "Other Players:");

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
  static int ventId = 0;
  ImGui::InputInt("Vent ID##v", &ventId);
  if (GlowBtn("Enter Vent", {120, 28}))
    Game::EnterVent(ventId);
  ImGui::SameLine();
  if (GlowBtn("Exit Vent", {120, 28}))
    Game::ExitVent(ventId);
  EndCard();

  Card("Sabotage / Doors");
  // SystemTypes:
  // 0=Hallway,1=Storage,2=Cafeteria,3=Reactor,4=UpperEngine,5=Navigation,
  //              6=Admin,7=Electrical,8=O2,9=Shields,10=MedBay,11=Security,12=Weapons,
  //              13=LowerEngine,14=ShortComms,15=LobbyComm
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
  if (GlowBtn("Teleport ALL to Self", {200, 28}))
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
  const char *hats[] = {"None", "Crown", "Top Hat", "Beanie", "Horns"};
  const char *pets[] = {"None", "Mini Crewmate", "Dog", "Cat", "Robot"};
  const char *skins[] = {"None", "Suit", "Astronaut", "Military", "Mech"};
  const char *visors[] = {"None", "Lollipop (Crew)", "Lollipop (Imp)",
                          "Star (Crew)", "Angery"};
  const char *nameplates[] = {"None", "Toppat", "CCC", "Government", "Yard"};
  if (ImGui::Combo("Hat##c", &g_hat, hats, 5))
    Game::SetHat(g_hat);
  if (ImGui::Combo("Pet##c", &g_pet, pets, 5))
    Game::SetPet(g_pet);
  if (ImGui::Combo("Skin##c", &g_skin, skins, 5))
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
  if (Game::isInGame && Game::localColorId >= 0 && Game::localColorId < 18) {
    ImGui::TextColored({.5f, .85f, 1, 1}, "Current Color: %s",
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
  Toggle("Developer Mode", &g_devMode);
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
  EndCard();
}

// â”€â”€ Apply Theme
// â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
static void ApplyTheme() {
  ImGuiStyle &s = ImGui::GetStyle();
  s.WindowPadding = {12, 10};
  s.FramePadding = {9, 5};
  s.ItemSpacing = {8, 6};
  s.ItemInnerSpacing = {7, 5};
  s.ScrollbarSize = 11;
  s.IndentSpacing = 18.f;
  s.WindowBorderSize = 1.f;
  s.ChildBorderSize = 1.f;
  s.FrameBorderSize = 1.f;
  s.WindowRounding = 11.f;
  s.ChildRounding = 10.f;
  s.FrameRounding = 7.f;
  s.PopupRounding = 9.f;
  s.ScrollbarRounding = 12.f;
  s.GrabRounding = 6.f;
  s.TabRounding = 7.f;
  ImVec4 *c = s.Colors;
  c[ImGuiCol_WindowBg] = {.028f, .036f, .062f, .96f};
  c[ImGuiCol_ChildBg] = {0.f, 0.f, 0.f, 0.f};
  c[ImGuiCol_PopupBg] = {.046f, .055f, .084f, .97f};
  c[ImGuiCol_Border] = {.28f, .34f, .48f, .58f};
  c[ImGuiCol_FrameBg] = {.075f, .098f, .152f, .86f};
  c[ImGuiCol_FrameBgHovered] = {.105f, .132f, .195f, .94f};
  c[ImGuiCol_FrameBgActive] = {.12f, .152f, .22f, 1.f};
  c[ImGuiCol_TitleBg] = {.03f, .042f, .07f, .98f};
  c[ImGuiCol_TitleBgActive] = {.042f, .056f, .09f, .98f};
  c[ImGuiCol_ScrollbarBg] = {.032f, .04f, .062f, .6f};
  c[ImGuiCol_ScrollbarGrab] = {.27f, .34f, .47f, .7f};
  c[ImGuiCol_ScrollbarGrabHovered] = {.34f, .43f, .58f, .82f};
  c[ImGuiCol_ScrollbarGrabActive] = {.42f, .52f, .7f, .9f};
  c[ImGuiCol_Text] = {.95f, .97f, 1.f, 1.f};
  c[ImGuiCol_TextDisabled] = {.58f, .64f, .75f, .95f};
  ImVec4 ac = Accent();
  c[ImGuiCol_CheckMark] = ac;
  c[ImGuiCol_SliderGrab] = ac;
  c[ImGuiCol_SliderGrabActive] = {ac.x * 1.18f, ac.y * 1.18f, ac.z * 1.18f,
                                  1.f};
  c[ImGuiCol_Button] = {ac.x * .17f, ac.y * .17f, ac.z * .19f, .9f};
  c[ImGuiCol_ButtonHovered] = {ac.x * .27f, ac.y * .27f, ac.z * .3f, .98f};
  c[ImGuiCol_ButtonActive] = {ac.x * .4f, ac.y * .38f, ac.z * .43f, 1.f};
  c[ImGuiCol_Header] = {ac.x * .16f, ac.y * .16f, ac.z * .18f, .7f};
  c[ImGuiCol_HeaderHovered] = {ac.x * .23f, ac.y * .23f, ac.z * .27f, .86f};
  c[ImGuiCol_HeaderActive] = {ac.x * .3f, ac.y * .3f, ac.z * .34f, .98f};
  c[ImGuiCol_Separator] = {.26f, .32f, .45f, .5f};
  c[ImGuiCol_TableHeaderBg] = {.074f, .098f, .15f, .92f};
  c[ImGuiCol_TableBorderStrong] = {.26f, .31f, .45f, .7f};
  c[ImGuiCol_TableBorderLight] = {.2f, .25f, .37f, .5f};
  c[ImGuiCol_TableRowBg] = {.04f, .05f, .08f, .14f};
  c[ImGuiCol_TableRowBgAlt] = {.06f, .08f, .12f, .23f};
  c[ImGuiCol_ResizeGrip] = {ac.x * .2f, ac.y * .2f, ac.z * .24f, .5f};
  c[ImGuiCol_ResizeGripHovered] = {ac.x * .34f, ac.y * .34f, ac.z * .4f, .82f};
  c[ImGuiCol_ResizeGripActive] = {ac.x * .44f, ac.y * .44f, ac.z * .52f, 1.f};
}

static bool TabMatchesSearch(Tab t) {
  if (!g_tabSearch[0])
    return true;
  std::string q = g_tabSearch;
  return ContainsNoCase(TabName(t), q) || ContainsNoCase(TabTag(t), q) ||
         ContainsNoCase(TabSubtitle(t), q);
}

static void DrawTabHero(ImDrawList *dl, const ImVec2 &cursor, float width) {
  ImVec4 ac = Accent();
  const char *title = TabName(g_tab);
  const char *subtitle = TabSubtitle(g_tab);
  ImU32 tone = TabTone(g_tab, 230);
  float heroH = 56.f;
  ImVec2 min = cursor;
  ImVec2 max = {cursor.x + width, cursor.y + heroH};
  dl->AddRectFilled(min, max, IM_COL32(8, 14, 24, 215), 10.f);
  dl->AddRect(min, max,
              IM_COL32((int)(ac.x * 255.f * 0.8f), (int)(ac.y * 255.f * 0.8f),
                       (int)(ac.z * 255.f * 0.8f), 120),
              10.f, 0, 1.0f);
  dl->AddRectFilledMultiColor(
      min, {max.x, min.y + 2.5f},
      IM_COL32((int)(ac.x * 255), (int)(ac.y * 255), (int)(ac.z * 255), 165),
      IM_COL32(255, 110, 140, 135), IM_COL32(255, 110, 140, 135),
      IM_COL32((int)(ac.x * 255), (int)(ac.y * 255), (int)(ac.z * 255), 165));
  dl->AddText({min.x + 12.f, min.y + 10.f}, Colors::TextPrimary, title);
  dl->AddText({min.x + 12.f, min.y + 30.f}, IM_COL32(170, 190, 220, 230),
              subtitle);
  ImVec2 tagSize = ImGui::CalcTextSize(TabTag(g_tab));
  ImVec2 tagMin = {max.x - tagSize.x - 14.f, min.y + 8.f};
  ImVec2 tagMax = {max.x - 8.f, min.y + 24.f};
  dl->AddRectFilled(tagMin, tagMax, IM_COL32(16, 28, 44, 210), 8.f);
  dl->AddRect(tagMin, tagMax, IM_COL32(80, 124, 176, 170), 8.f, 0, 1.f);
  dl->AddText({tagMin.x + 5.f, tagMin.y + 2.f}, IM_COL32(218, 238, 255, 240),
              TabTag(g_tab));
  ImVec2 iconMin = {max.x - 34.f, min.y + 30.f};
  ImVec2 iconMax = {max.x - 10.f, min.y + 52.f};
  dl->AddRectFilled(iconMin, iconMax,
                    IM_COL32((int)((tone >> IM_COL32_R_SHIFT) & 0xFF),
                             (int)((tone >> IM_COL32_G_SHIFT) & 0xFF),
                             (int)((tone >> IM_COL32_B_SHIFT) & 0xFF), 55),
                    6.f);
  dl->AddRect(iconMin, iconMax, IM_COL32(95, 136, 184, 170), 6.f, 0, 1.f);
  DrawTabGlyph(dl, g_tab,
               {(iconMin.x + iconMax.x) * 0.5f, (iconMin.y + iconMax.y) * 0.5f},
               11.8f, IM_COL32(232, 245, 255, 245), 1.2f);
  dl->AddCircleFilled({max.x - 18.f, min.y + heroH - 15.f}, 2.4f,
                      IM_COL32(220, 245, 255, 170), 12);
  dl->AddCircleFilled({max.x - 28.f, min.y + heroH - 22.f}, 1.8f,
                      IM_COL32(180, 230, 255, 130), 12);
}

// â”€â”€ Main Render
// â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
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

  // Background stars + particles
  {
    ImDrawList *bg = ImGui::GetBackgroundDrawList();
    ImVec2 screen = displaySize;
    ImVec4 ac = Accent();
    DrawSpaceBackground(bg, screen, dt, g_menuAlpha, ac);

    static struct {
      float x, y, vx, vy, s, a;
    } dust[64];
    static bool dustInit = false;
    if (!dustInit) {
      dustInit = true;
      for (auto &p : dust) {
        p.x = Randf(0.f, screen.x);
        p.y = Randf(0.f, screen.y);
        p.vx = Randf(-8.f, 8.f);
        p.vy = Randf(-12.f, -2.f);
        p.s = Randf(0.7f, 2.2f);
        p.a = Randf(0.15f, 0.55f);
      }
    }
    for (auto &p : dust) {
      p.x += p.vx * dt;
      p.y += p.vy * dt;
      if (p.x < -2.f)
        p.x = screen.x + 2.f;
      if (p.x > screen.x + 2.f)
        p.x = -2.f;
      if (p.y < -2.f)
        p.y = screen.y + 2.f;
      if (p.y > screen.y + 2.f)
        p.y = -2.f;
      ImU32 c = IM_COL32(120, 200, 255, (int)(p.a * 70.f * g_menuAlpha));
      bg->AddCircleFilled({p.x, p.y}, p.s, c, 8);
    }
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

    // Animated outer glow
    float pulse = 0.5f + 0.5f * sinf(t * 2.15f);
    for (int i = 0; i < 3; i++) {
      float grow = 1.2f + i * 2.5f;
      int a = (int)((46 - i * 11) * pulse * g_menuAlpha);
      dl->AddRect(
          {wp.x - grow, wp.y - grow}, {wp.x + ws.x + grow, wp.y + ws.y + grow},
          IM_COL32((int)(ac.x * 255), (int)(ac.y * 255), (int)(ac.z * 255), a),
          12.f + grow, 0, 1.1f);
    }

    // Title bar
    dl->AddRectFilled(wp, {wp.x + ws.x, wp.y + 44}, IM_COL32(10, 14, 24, 240),
                      12, ImDrawFlags_RoundCornersTop);
    dl->AddRectFilledMultiColor(
        wp, {wp.x + ws.x, wp.y + 3},
        IM_COL32((int)(ac.x * 255), (int)(ac.y * 255), (int)(ac.z * 255), 200),
        IM_COL32(255, 80, 120, 120), IM_COL32(255, 80, 120, 120),
        IM_COL32((int)(ac.x * 255), (int)(ac.y * 255), (int)(ac.z * 255), 200));
    dl->AddText({wp.x + 14, wp.y + 13}, Colors::TextPrimary, APP_NAME);
    dl->AddText({wp.x + ws.x - 114, wp.y + 13}, Colors::TextSecondary,
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

    // Tiny header stars for extra motion
    float hs = 2.0f + 0.7f * sinf(t * 4.1f);
    dl->AddCircleFilled({wp.x + ws.x - 104, wp.y + 22}, hs,
                        IM_COL32(220, 245, 255, (int)(190 * g_menuAlpha)), 10);
    dl->AddCircleFilled({wp.x + ws.x - 130, wp.y + 17}, hs * 0.7f,
                        IM_COL32(180, 230, 255, (int)(150 * g_menuAlpha)), 10);

    // Footer
    dl->AddRectFilled({wp.x, wp.y + ws.y - 24}, {wp.x + ws.x, wp.y + ws.y},
                      IM_COL32(8, 8, 14, 220), 10,
                      ImDrawFlags_RoundCornersBottom);
    dl->AddText({wp.x + 12, wp.y + ws.y - 18}, Colors::TextDim,
                "Among Stara Client | github.com/stara");

    ImGui::SetCursorPosY(50);

    // Sidebar + Content
    float sidebarWidth = std::clamp(178.f * g_uiScale, 170.f, 240.f);
    ImGui::BeginChild("##sidebar", {sidebarWidth, ws.y - 78}, false);
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
      float itemH = 42.f;
      float itemW = sidebarWidth - 6.f;
      ImVec2 cp = ImGui::GetCursorScreenPos();
      ImDrawList *sdl = ImGui::GetWindowDrawList();
      bool preHover =
          ImGui::IsMouseHoveringRect(cp, {cp.x + itemW, cp.y + itemH});
      float &ha = g_hoverAnim[i];
      ha = Damp(ha, (sel || preHover) ? 1.f : 0.f, 12.f);
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
        int a = sel ? (int)(95 + 45 * ha) : (int)(58 * ha);
        sdl->AddRectFilled(cp, {cp.x + itemW, cp.y + itemH},
                           IM_COL32((int)(ac.x * 255 * .11f),
                                    (int)(ac.y * 255 * .11f),
                                    (int)(ac.z * 255 * .13f), a),
                           8.f);
      }
      if (sel) {
        sdl->AddRectFilled({cp.x, cp.y + 5.f}, {cp.x + 3.f, cp.y + itemH - 5.f},
                           tone, 2.f);
      }

      ImVec2 iconMin = {cp.x + 7.f, cp.y + 8.f};
      ImVec2 iconMax = {cp.x + 30.f, cp.y + itemH - 8.f};
      sdl->AddRectFilled(iconMin, iconMax,
                         IM_COL32((int)((tone >> IM_COL32_R_SHIFT) & 0xFF),
                                  (int)((tone >> IM_COL32_G_SHIFT) & 0xFF),
                                  (int)((tone >> IM_COL32_B_SHIFT) & 0xFF),
                                  sel ? 54 : 30),
                         6.f);
      sdl->AddRect(iconMin, iconMax, IM_COL32(88, 122, 170, sel ? 170 : 105),
                   6.f, 0, 1.0f);
      DrawTabGlyph(
          sdl, t,
          {(iconMin.x + iconMax.x) * 0.5f, (iconMin.y + iconMax.y) * 0.5f},
          12.f, sel ? IM_COL32(238, 248, 255, 250) : tone, 1.2f);

      ImU32 titleCol = sel ? IM_COL32(238, 246, 255, 255)
                           : (hovered ? IM_COL32(212, 230, 250, 245)
                                      : IM_COL32(176, 200, 226, 232));
      ImU32 hintCol =
          sel ? IM_COL32(150, 193, 232, 245) : IM_COL32(124, 156, 190, 205);
      sdl->AddText({cp.x + 36.f, cp.y + 6.f}, titleCol, TabName(t));
      sdl->AddText({cp.x + 36.f, cp.y + 22.f}, hintCol, TabSidebarHint(t));

      const char *tag = TabTag(t);
      ImVec2 tg = ImGui::CalcTextSize(tag);
      float pillW = tg.x + 10.f;
      ImVec2 pillMin = {cp.x + itemW - pillW - 8.f, cp.y + 11.f};
      ImVec2 pillMax = {pillMin.x + pillW, pillMin.y + 18.f};
      sdl->AddRectFilled(pillMin, pillMax,
                         IM_COL32(16, 28, 44, sel ? 210 : 170), 8.f);
      sdl->AddRect(pillMin, pillMax, IM_COL32(72, 116, 165, sel ? 188 : 130),
                   8.f, 0, 1.0f);
      sdl->AddText({pillMin.x + 5.f, pillMin.y + 2.f},
                   sel ? IM_COL32(232, 244, 255, 250)
                       : IM_COL32(170, 200, 232, 225),
                   tag);
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
    ImGui::BeginChild("##content", {0, ws.y - 78}, false);
    ImGui::Spacing();
    {
      ImVec2 heroPos = ImGui::GetCursorScreenPos();
      float heroW = std::max(260.f, ImGui::GetContentRegionAvail().x);
      DrawTabHero(dl, heroPos, heroW);
      ImGui::Dummy({heroW, 60.f});
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
