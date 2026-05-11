#pragma once
#include <string>
#include <vector>
#include <map>
#include <unordered_map>
#include <functional>
#include <memory>
#include <cmath>
#include <algorithm>
#include <chrono>
#include <random>
#include <fstream>
#include <sstream>
#include <array>
#include <numbers>
#include <thread>
#include <mutex>
#include <atomic>

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <d3d11.h>
#include <dxgi.h>
#include <dwmapi.h>

#include "imgui.h"
#include "imgui_internal.h"
#include "imgui_impl_dx11.h"
#include "imgui_impl_win32.h"
#include "MinHook.h"
#include <nlohmann/json.hpp>

namespace Stara {

constexpr const char* APP_NAME    = "Among Stara Client";
constexpr const char* APP_VERSION = "v1.0.0";

namespace Colors {
    constexpr ImU32 Cyan           = IM_COL32(0, 220, 255, 255);
    constexpr ImU32 CyanDim        = IM_COL32(0, 220, 255, 120);
    constexpr ImU32 Red            = IM_COL32(255, 60, 80, 255);
    constexpr ImU32 RedDim         = IM_COL32(255, 60, 80, 120);
    constexpr ImU32 BgDark         = IM_COL32(12, 12, 18, 240);
    constexpr ImU32 BgPanel        = IM_COL32(18, 18, 28, 220);
    constexpr ImU32 BgCard         = IM_COL32(24, 24, 36, 200);
    constexpr ImU32 BgSidebar      = IM_COL32(14, 14, 22, 245);
    constexpr ImU32 TextPrimary    = IM_COL32(240, 240, 255, 255);
    constexpr ImU32 TextSecondary  = IM_COL32(160, 160, 190, 255);
    constexpr ImU32 TextDim        = IM_COL32(100, 100, 130, 200);
    constexpr ImU32 Border         = IM_COL32(50, 50, 80, 100);
    constexpr ImU32 Success        = IM_COL32(50, 255, 120, 255);
    constexpr ImU32 Warning        = IM_COL32(255, 200, 50, 255);
    constexpr ImU32 Epic           = IM_COL32(180, 60, 255, 255);
}

namespace Ease {
    inline float OutCubic(float t) { return 1.f - std::pow(1.f-t, 3.f); }
    inline float Lerp(float a, float b, float t) { return a + (b-a)*t; }
    inline ImVec4 LerpColor(const ImVec4& a, const ImVec4& b, float t) {
        return {Lerp(a.x,b.x,t), Lerp(a.y,b.y,t), Lerp(a.z,b.z,t), Lerp(a.w,b.w,t)};
    }
}

enum class Tab { Dashboard=0, Player, Visuals, ESP, Movement, Fun, Troll, Cosmetics, Settings, Count };
inline const char* TabName(Tab t) {
    constexpr const char* n[] = {"Dashboard","Player","Visuals","ESP","Movement","Fun","Troll","Cosmetics","Settings"};
    return n[(int)t];
}

inline ImVec4 U32toVec4(ImU32 c) {
    return {((c>>0)&0xFF)/255.f, ((c>>8)&0xFF)/255.f, ((c>>16)&0xFF)/255.f, ((c>>24)&0xFF)/255.f};
}
inline ImU32 Vec4toU32(const ImVec4& v) {
    return IM_COL32((int)(v.x*255),(int)(v.y*255),(int)(v.z*255),(int)(v.w*255));
}

struct AnimFloat {
    float cur=0, tgt=0, spd=8;
    void Update(float dt) { cur += (tgt-cur)*std::min(1.f, spd*dt); if(std::abs(cur-tgt)<0.001f) cur=tgt; }
    void Set(float v) { tgt=v; }
    void SetNow(float v) { cur=tgt=v; }
    operator float() const { return cur; }
};

} // namespace Stara
