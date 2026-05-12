#pragma once
#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cmath>
#include <fstream>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <numbers>
#include <random>
#include <sstream>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <d3d11.h>
#include <dwmapi.h>
#include <dxgi.h>

#include "MinHook.h"
#include "imgui.h"
#include "imgui_impl_dx11.h"
#include "imgui_impl_win32.h"
#include "imgui_internal.h"
#include <nlohmann/json.hpp>

namespace Stara {

constexpr const char *APP_NAME = "Among Stara Client";
constexpr const char *APP_VERSION = "v1.2.1";

namespace Colors {
constexpr ImU32 Cyan = IM_COL32(0, 220, 255, 255);
constexpr ImU32 CyanDim = IM_COL32(0, 220, 255, 120);
constexpr ImU32 Red = IM_COL32(255, 60, 80, 255);
constexpr ImU32 RedDim = IM_COL32(255, 60, 80, 120);
constexpr ImU32 BgDark = IM_COL32(12, 12, 18, 240);
constexpr ImU32 BgPanel = IM_COL32(18, 18, 28, 220);
constexpr ImU32 BgCard = IM_COL32(24, 24, 36, 200);
constexpr ImU32 BgSidebar = IM_COL32(14, 14, 22, 245);
constexpr ImU32 TextPrimary = IM_COL32(240, 240, 255, 255);
constexpr ImU32 TextSecondary = IM_COL32(160, 160, 190, 255);
constexpr ImU32 TextDim = IM_COL32(100, 100, 130, 200);
constexpr ImU32 Border = IM_COL32(50, 50, 80, 100);
constexpr ImU32 Success = IM_COL32(50, 255, 120, 255);
constexpr ImU32 Warning = IM_COL32(255, 200, 50, 255);
constexpr ImU32 Epic = IM_COL32(180, 60, 255, 255);
} // namespace Colors

namespace Ease {
inline float OutCubic(float t) { return 1.f - std::pow(1.f - t, 3.f); }
inline float Lerp(float a, float b, float t) { return a + (b - a) * t; }
inline ImVec4 LerpColor(const ImVec4 &a, const ImVec4 &b, float t) {
  return {Lerp(a.x, b.x, t), Lerp(a.y, b.y, t), Lerp(a.z, b.z, t),
          Lerp(a.w, b.w, t)};
}
} // namespace Ease

enum class Tab {
  Dashboard = 0,
  Player,
  Visuals,
  ESP,
  Movement,
  Fun,
  Troll,
  Cosmetics,
  Settings,
  Count
};
inline const char *TabName(Tab t) {
  constexpr const char *n[] = {"Dashboard", "Player",    "Visuals",
                               "ESP",       "Movement",  "Fun",
                               "Troll",     "Cosmetics", "Settings"};
  return n[(int)t];
}

inline ImVec4 U32toVec4(ImU32 c) {
  return {((c >> 0) & 0xFF) / 255.f, ((c >> 8) & 0xFF) / 255.f,
          ((c >> 16) & 0xFF) / 255.f, ((c >> 24) & 0xFF) / 255.f};
}
inline ImU32 Vec4toU32(const ImVec4 &v) {
  return IM_COL32((int)(v.x * 255), (int)(v.y * 255), (int)(v.z * 255),
                  (int)(v.w * 255));
}

struct AnimFloat {
  float cur = 0, tgt = 0, spd = 8;
  void Update(float dt) {
    cur += (tgt - cur) * std::min(1.f, spd * dt);
    if (std::abs(cur - tgt) < 0.001f)
      cur = tgt;
  }
  void Set(float v) { tgt = v; }
  void SetNow(float v) { cur = tgt = v; }
  operator float() const { return cur; }
};

// ── Globals (Extern) ────────────────────────────────────────────────
extern float g_speed, g_fov, g_zoom, g_bloom, g_walkSpeed, g_animSpeed,
    g_uiScale;
extern float g_ping, g_taskProg, g_themeInt, g_blurInt, g_killCd, g_killDist;
extern bool g_fullbright, g_wireframe, g_smoothMove, g_wallhack;
extern bool g_espBox, g_espName, g_espDist, g_espRole, g_espTracer,
    g_espOutline, g_espTask;
extern bool g_rainbow, g_spin, g_tiny, g_giant, g_dance, g_particle, g_autoPath;
extern bool g_noclip, g_chatSpam, g_devMode, g_fpsDisp, g_rgbAccent;
extern float g_accentCol[4], g_playerCol[4];
extern char g_nameBuf[64];
extern int g_hat, g_pet, g_skin, g_trail, g_emote, g_rarity;

// New feature toggles
extern bool g_noKillCd, g_infiniteEmergencies, g_alwaysMoveable,
    g_impostorVision;
extern bool g_maxReportDist, g_autoTasks, g_freezeAll, g_colorCycle;
extern bool g_antiKick, g_forceProtect, g_spamAnim, g_godmode;
extern float g_customDiscussTime, g_customVoteTime;
extern char g_chatBuf[128];

} // namespace Stara
