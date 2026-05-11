#include "Common.hpp"
#include "Game.hpp"
#include "Hooks.hpp"

namespace Stara {

extern bool g_menuVisible;

// ── State ──────────────────────────────────────────────────────────
static Tab g_tab = Tab::Dashboard;
static float g_menuAlpha = 0.f;
static float g_hoverAnim[9] = {};
static float g_toggleAnim[64] = {};
static int g_toggleIdx = 0;

// Config values
static float g_speed = 1.f, g_fov = 90.f, g_zoom = 1.f, g_bloom = 0.5f;
static float g_walkSpeed = 1.f, g_animSpeed = 1.f, g_uiScale = 1.f;
static float g_ping = 50.f, g_taskProg = 0.f, g_themeInt = 1.f, g_blurInt = 0.8f;
static float g_killCd = 0.f, g_killDist = 1.0f;
static bool g_fullbright=false, g_wireframe=false, g_smoothMove=true, g_wallhack=false;
static bool g_espBox=true, g_espName=true, g_espDist=true, g_espRole=true;
static bool g_espTracer=false, g_espOutline=true, g_espTask=false;
bool g_rainbow=false, g_spin=false, g_tiny=false, g_giant=false;
bool g_dance=false, g_particle=false, g_autoPath=false;
static bool g_devMode=false, g_fpsDisp=true, g_rgbAccent=false;
static float g_accentCol[4] = {0,0.86f,1,1};
static char g_nameBuf[64] = "Stara";
static float g_playerCol[4] = {0,0.86f,1,1};
static int g_hat=0, g_pet=0, g_skin=0, g_trail=0, g_emote=0, g_rarity=0;
static float g_hue = 0.f;

static ImVec4 Accent() {
    if (g_rgbAccent) return ImGui::ColorConvertU32ToFloat4(
        IM_COL32((int)(sin(g_hue)*127+128),(int)(sin(g_hue+2.1f)*127+128),(int)(sin(g_hue+4.2f)*127+128),255));
    return {g_accentCol[0],g_accentCol[1],g_accentCol[2],g_accentCol[3]};
}

// ── Custom Widgets ─────────────────────────────────────────────────
static bool Toggle(const char* label, bool* v) {
    ImGuiWindow* w = ImGui::GetCurrentWindow();
    if (w->SkipItems) return false;
    ImGuiID id = w->GetID(label);
    float h=20, wd=38, r=h*.5f;
    ImVec2 p = w->DC.CursorPos;
    ImVec2 ls = ImGui::CalcTextSize(label);
    ImRect bb(p, ImVec2(p.x+wd+8+ls.x, p.y+std::max(h,ls.y)));
    ImGui::ItemSize(bb); if(!ImGui::ItemAdd(bb,id)) return false;
    bool hov,held; bool press=ImGui::ButtonBehavior(bb,id,&hov,&held);
    if(press) *v=!*v;
    int ti = g_toggleIdx++ % 64;
    float &ta = g_toggleAnim[ti];
    float tgt = *v?1.f:0.f;
    ta += (tgt-ta)*0.15f;
    ImDrawList* dl=w->DrawList;
    ImVec4 ac=Accent();
    ImU32 bg = *v ? IM_COL32((int)(ac.x*255*.5f),(int)(ac.y*255*.5f),(int)(ac.z*255*.5f),200) : IM_COL32(40,40,60,200);
    dl->AddRectFilled(p,{p.x+wd,p.y+h},bg,r);
    float kx = p.x+r + (wd-h)*ta;
    dl->AddCircleFilled({kx,p.y+r},r-3,IM_COL32(255,255,255,240),24);
    dl->AddText({p.x+wd+8, p.y+(h-ls.y)*.5f}, Colors::TextPrimary, label);
    return press;
}

static bool GlowBtn(const char* label, ImVec2 sz={0,0}) {
    ImVec4 ac=Accent();
    ImGui::PushStyleColor(ImGuiCol_Button,{ac.x*.15f,ac.y*.15f,ac.z*.15f,.8f});
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered,{ac.x*.25f,ac.y*.25f,ac.z*.25f,.9f});
    ImGui::PushStyleColor(ImGuiCol_ButtonActive,{ac.x*.35f,ac.y*.35f,ac.z*.35f,1});
    bool r = ImGui::Button(label, sz);
    ImGui::PopStyleColor(3);
    return r;
}

static bool Slider(const char* l, float* v, float mn, float mx, const char* fmt="%.1f") {
    ImVec4 ac=Accent();
    ImGui::PushStyleColor(ImGuiCol_SliderGrab,ac);
    ImGui::PushStyleColor(ImGuiCol_SliderGrabActive,{ac.x*1.3f,ac.y*1.3f,ac.z*1.3f,1});
    ImGui::PushStyleColor(ImGuiCol_FrameBg,{.08f,.08f,.14f,.8f});
    bool c = ImGui::SliderFloat(l,v,mn,mx,fmt);
    ImGui::PopStyleColor(3);
    return c;
}

static void Card(const char* title) {
    ImGui::PushStyleColor(ImGuiCol_ChildBg,{.06f,.06f,.1f,.85f});
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding,10);
    ImGui::BeginChild(title,{0,0},ImGuiChildFlags_AutoResizeY|ImGuiChildFlags_Borders);
    ImDrawList* dl=ImGui::GetWindowDrawList();
    ImVec2 p=ImGui::GetWindowPos(), s=ImGui::GetWindowSize();
    ImVec4 ac=Accent();
    dl->AddRectFilledMultiColor(p,{p.x+s.x,p.y+2},
        IM_COL32((int)(ac.x*255),(int)(ac.y*255),(int)(ac.z*255),150),
        IM_COL32(255,60,80,80),IM_COL32(255,60,80,80),
        IM_COL32((int)(ac.x*255),(int)(ac.y*255),(int)(ac.z*255),150));
    ImGui::TextColored({.94f,.94f,1,1}, "%s", title);
    ImGui::Spacing();
}
static void EndCard() { ImGui::EndChild(); ImGui::PopStyleVar(); ImGui::PopStyleColor(); ImGui::Spacing(); }

static void Sep(const char* t) {
    ImGui::Spacing();
    ImGui::TextColored({.4f,.4f,.5f,1}, "— %s —", t);
    ImGui::Spacing();
}

// ── Tab Renderers ──────────────────────────────────────────────────
static void TabDashboard() {
    Card("Profile");
    ImGui::Text("Player: %s", g_nameBuf);
    ImGui::SameLine(200); ImGui::TextColored({0,1,.5f,1},"Online");
    ImGui::Text("Version: %s", APP_VERSION);
    EndCard();
    Card("Session Stats");
    ImGui::Text("Games Played: 0"); ImGui::Text("Wins: 0"); ImGui::Text("Impostor Wins: 0");
    ImGui::Text("Tasks Completed: 0"); ImGui::Text("Kills: 0");
    EndCard();
    Card("Quick Actions");
    if(GlowBtn("Complete Tasks",{140,30})) Game::CompleteAllTasks();
    ImGui::SameLine();
    if(GlowBtn("Emergency Meeting",{160,30})) Game::ForceEmergencyMeeting();
    EndCard();
}

static void TabPlayer() {
    Card("Player Settings");
    Slider("Speed##p", &g_speed, 0.5f, 10.f);
    if(ImGui::IsItemDeactivatedAfterEdit()) Game::SetSpeed(g_speed);
    Slider("Ping Display##p", &g_ping, 0, 999, "%.0f ms");
    ImGui::InputText("Name##p", g_nameBuf, 64);
    if(ImGui::IsItemDeactivatedAfterEdit()) Game::SetName(g_nameBuf);
    ImGui::ColorEdit4("Color##p", g_playerCol, ImGuiColorEditFlags_NoInputs);
    EndCard();
    Card("Tasks");
    Slider("Task Progress##p", &g_taskProg, 0, 1, "%.0f%%");
    if(GlowBtn("Complete All Tasks",{180,28})) Game::CompleteAllTasks();
    EndCard();
    Card("OP Cheats");
    Slider("Kill Cooldown##op", &g_killCd, 0.f, 60.f);
    if(ImGui::IsItemDeactivatedAfterEdit()) Game::SetKillCooldown(g_killCd);
    Slider("Kill Distance##op", &g_killDist, 1.f, 50.f);
    if(ImGui::IsItemDeactivatedAfterEdit()) Game::SetKillDistance(g_killDist);
    Toggle("Wallhack / No Shadows##op", &g_wallhack);
    if(ImGui::IsItemDeactivatedAfterEdit()) Game::SetWallhack(g_wallhack);
    EndCard();
    Card("Animation");
    Toggle("Smooth Movement", &g_smoothMove);
    EndCard();
}

static void TabVisuals() {
    Card("Rendering");
    Toggle("Fullbright", &g_fullbright);
    if(ImGui::IsItemDeactivatedAfterEdit()) Game::SetFullbright(g_fullbright);
    Toggle("Wireframe View", &g_wireframe);
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
    Card("ESP Preview");
    ImGui::TextColored(Accent(), "All ESP renders in preview panels only");
    ImGui::Spacing();
    Toggle("Player Box", &g_espBox);
    Toggle("Nametag", &g_espName);
    Toggle("Distance", &g_espDist);
    Toggle("Role Color", &g_espRole);
    Toggle("Tracer Lines", &g_espTracer);
    Toggle("Outline", &g_espOutline);
    Toggle("Task Markers", &g_espTask);
    EndCard();
    // ESP preview panel
    Card("##espPreview");
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImVec2 p = ImGui::GetCursorScreenPos();
    float pw=280, ph=200;
    dl->AddRectFilled(p,{p.x+pw,p.y+ph},IM_COL32(8,8,14,220),6);
    // Draw a sample "player" in the preview
    float cx=p.x+pw/2, cy=p.y+ph/2;
    float bw=30, bh=50;
    if(g_espBox) dl->AddRect({cx-bw,cy-bh},{cx+bw,cy+bh}, Colors::Cyan, 2, 0, 1.5f);
    if(g_espName) dl->AddText({cx-20,cy-bh-16}, Colors::TextPrimary, "Player");
    if(g_espDist) dl->AddText({cx-10,cy+bh+4}, Colors::TextSecondary, "15m");
    if(g_espTracer) dl->AddLine({p.x+pw/2,p.y+ph},{cx,cy+bh}, Colors::Cyan, 1);
    if(g_espOutline) {
        for(float i=3;i>0;i-=1.f)
            dl->AddRect({cx-bw-i,cy-bh-i},{cx+bw+i,cy+bh+i},IM_COL32(0,220,255,(int)(20*i)),2);
    }
    ImGui::Dummy({pw,ph});
    EndCard();
}

static void TabMovement() {
    Card("Movement");
    Slider("Walk Speed##m", &g_walkSpeed, 0.5f, 10);
    if(ImGui::IsItemDeactivatedAfterEdit()) Game::SetSpeed(g_walkSpeed);
    Toggle("Smooth Movement##m", &g_smoothMove);
    Slider("Animation Speed##m", &g_animSpeed, 0.1f, 5);
    Toggle("Auto Path##m", &g_autoPath);
    EndCard();
    Card("Teleport");
    static float tpX=0, tpY=0;
    ImGui::InputFloat("X##tp",&tpX); ImGui::InputFloat("Y##tp",&tpY);
    if(GlowBtn("Teleport",{100,28})) Game::TeleportTo(tpX,tpY);
    EndCard();
}

static void TabFun() {
    Card("Character Effects");
    Toggle("Rainbow Character", &g_rainbow);
    Toggle("Spin Preview", &g_spin);
    Toggle("Tiny Character", &g_tiny);
    Toggle("Giant Character", &g_giant);
    Toggle("Dance Animation", &g_dance);
    Toggle("Particle Effects", &g_particle);
    EndCard();
    Card("Emotes");
    const char* emotes[] = {"Wave","Dance","Clap","Dab","Flex"};
    for(int i=0;i<5;i++) { if(i) ImGui::SameLine(); if(GlowBtn(emotes[i],{60,28})) {} }
    EndCard();
}

static void TabTroll() {
    Card("Visual Trolls (Preview Only)");
    ImGui::TextColored({1,.3f,.3f,1},"These are visual-only previews");
    ImGui::Spacing();
    static bool showDisc=false, showEmerg=false, showSabo=false, showFlicker=false;
    if(GlowBtn("Disconnect Popup",{160,28})) showDisc=!showDisc;
    if(GlowBtn("Emergency Meeting",{160,28})) showEmerg=!showEmerg;
    if(GlowBtn("Sabotage Warning",{160,28})) showSabo=!showSabo;
    if(GlowBtn("Lights Flicker",{160,28})) showFlicker=!showFlicker;
    EndCard();
    // Popups
    if(showDisc) {
        ImGui::SetNextWindowSize({300,120});
        if(ImGui::Begin("##disc",nullptr,ImGuiWindowFlags_NoTitleBar|ImGuiWindowFlags_NoResize)){
            ImGui::TextColored({1,.3f,.3f,1},"Disconnected from server");
            ImGui::Text("You were banned from the room.");
            if(GlowBtn("OK##d",{80,28})) showDisc=false;
        } ImGui::End();
    }
    if(showEmerg) {
        ImGui::SetNextWindowSize({300,100});
        if(ImGui::Begin("##emerg",nullptr,ImGuiWindowFlags_NoTitleBar|ImGuiWindowFlags_NoResize)){
            ImGui::TextColored({1,.8f,0,1},"EMERGENCY MEETING!");
            ImGui::Text("Called by: Stara");
            if(GlowBtn("Dismiss##e",{80,28})) showEmerg=false;
        } ImGui::End();
    }
}

static void TabCosmetics() {
    Card("Character Preview");
    ImDrawList* dl=ImGui::GetWindowDrawList();
    ImVec2 p=ImGui::GetCursorScreenPos();
    // Draw Among Us character silhouette
    float cx=p.x+60, cy=p.y+70;
    float t = (float)ImGui::GetTime();
    if(g_spin) cx += sin(t*2)*5;
    ImU32 bodyCol = IM_COL32((int)(g_playerCol[0]*255),(int)(g_playerCol[1]*255),(int)(g_playerCol[2]*255),255);
    dl->AddRectFilled({cx-20,cy-30},{cx+20,cy+25},bodyCol,12); // body
    dl->AddRectFilled({cx-25,cy-10},{cx-18,cy+15},bodyCol,4);  // backpack
    dl->AddRectFilled({cx-15,cy-25},{cx+15,cy-10},IM_COL32(180,220,255,200),8); // visor
    dl->AddRectFilled({cx-12,cy+25},{cx-2,cy+40},bodyCol,3);   // left leg
    dl->AddRectFilled({cx+2,cy+25},{cx+12,cy+40},bodyCol,3);   // right leg
    ImGui::Dummy({120,100});
    EndCard();
    Card("Cosmetics");
    const char* hats[]={"None","Crown","Top Hat","Beanie","Horns"};
    const char* pets[]={"None","Mini Crewmate","Dog","Cat","Robot"};
    const char* skins[]={"None","Suit","Astronaut","Military","Mech"};
    const char* trails[]={"None","Stars","Fire","Rainbow","Smoke"};
    const char* rarities[]={"All","Common","Rare","Epic","Legendary"};
    ImGui::Combo("Hat##c", &g_hat, hats, 5);
    ImGui::Combo("Pet##c", &g_pet, pets, 5);
    ImGui::Combo("Skin##c", &g_skin, skins, 5);
    ImGui::Combo("Trail##c", &g_trail, trails, 5);
    ImGui::Combo("Rarity##c", &g_rarity, rarities, 5);
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
    if(GlowBtn("Save Config",{120,28})) {}
    ImGui::SameLine();
    if(GlowBtn("Load Config",{120,28})) {}
    ImGui::SameLine();
    if(GlowBtn("Reset",{80,28})) {}
    EndCard();
    Card("Keybinds");
    ImGui::Text("Toggle Menu: INSERT");
    ImGui::Text("Unload DLL: END");
    EndCard();
}

// ── Apply Theme ────────────────────────────────────────────────────
static void ApplyTheme() {
    ImGuiStyle& s = ImGui::GetStyle();
    s.WindowPadding={12,12}; s.FramePadding={8,5}; s.ItemSpacing={8,6};
    s.ScrollbarSize=10; s.WindowBorderSize=0; s.ChildBorderSize=0;
    s.WindowRounding=10; s.ChildRounding=10; s.FrameRounding=6;
    s.PopupRounding=8; s.ScrollbarRounding=12; s.GrabRounding=6;
    ImVec4* c=s.Colors;
    c[ImGuiCol_WindowBg]={.047f,.047f,.07f,.94f};
    c[ImGuiCol_ChildBg]={0,0,0,0};
    c[ImGuiCol_PopupBg]={.07f,.07f,.11f,.95f};
    c[ImGuiCol_Border]={.2f,.2f,.31f,.4f};
    c[ImGuiCol_FrameBg]={.08f,.08f,.12f,.8f};
    c[ImGuiCol_FrameBgHovered]={.12f,.12f,.18f,.9f};
    c[ImGuiCol_FrameBgActive]={.14f,.14f,.22f,1};
    c[ImGuiCol_ScrollbarBg]={.05f,.05f,.08f,.5f};
    c[ImGuiCol_Text]={.94f,.94f,1,1};
    c[ImGuiCol_TextDisabled]={.4f,.4f,.5f,.8f};
    ImVec4 ac=Accent();
    c[ImGuiCol_CheckMark]=ac;
    c[ImGuiCol_SliderGrab]=ac;
    c[ImGuiCol_Button]={ac.x*.15f,ac.y*.15f,ac.z*.15f,.8f};
    c[ImGuiCol_ButtonHovered]={ac.x*.25f,ac.y*.25f,ac.z*.25f,.9f};
    c[ImGuiCol_ButtonActive]={ac.x*.35f,ac.y*.35f,ac.z*.35f,1};
    c[ImGuiCol_Header]={ac.x*.15f,ac.y*.15f,ac.z*.15f,.6f};
    c[ImGuiCol_HeaderHovered]={ac.x*.2f,ac.y*.2f,ac.z*.2f,.8f};
    c[ImGuiCol_Separator]={.2f,.2f,.31f,.4f};
}

// ── Main Render ────────────────────────────────────────────────────
void RenderMenu() {
    float dt = ImGui::GetIO().DeltaTime;
    g_toggleIdx = 0;

    // RGB hue
    if(g_rgbAccent) { g_hue += dt*2; if(g_hue>6.28f) g_hue-=6.28f; }

    // Menu fade
    float tgt = g_menuVisible ? 1.f : 0.f;
    g_menuAlpha += (tgt - g_menuAlpha) * std::min(1.f, 8.f * dt);
    if(g_menuAlpha < 0.01f) return;

    ApplyTheme();
    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, g_menuAlpha);

    // FPS
    if(g_fpsDisp) {
        ImGui::GetForegroundDrawList()->AddText({10,10},
            IM_COL32(0,220,255,(int)(200*g_menuAlpha)),
            (std::string("FPS: ") + std::to_string((int)(1.f/dt))).c_str());
    }

    // Background particles
    {
        ImDrawList* dl = ImGui::GetBackgroundDrawList();
        static struct { float x,y,vx,vy,s,a; } pts[60];
        static bool pinit = false;
        if(!pinit) {
            pinit=true;
            for(auto& p : pts) {
                p.x=rand()%1920; p.y=rand()%1080;
                p.vx=(rand()%100-50)*.1f; p.vy=(rand()%100-50)*.08f - .3f;
                p.s=1+rand()%3; p.a=.2f+.3f*(rand()%100)/100.f;
            }
        }
        for(auto& p : pts) {
            p.x+=p.vx*dt*20; p.y+=p.vy*dt*20;
            if(p.x<0) p.x=1920; if(p.x>1920) p.x=0;
            if(p.y<0) p.y=1080; if(p.y>1080) p.y=0;
            ImU32 c = IM_COL32(0,180,255,(int)(p.a*60*g_menuAlpha));
            dl->AddCircleFilled({p.x,p.y}, p.s*1.5f, c, 12);
        }
    }

    // Main window
    ImGui::SetNextWindowSize({820,520}, ImGuiCond_FirstUseEver);
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoCollapse;

    if(ImGui::Begin("##StaraMain", nullptr, flags)) {
        ImDrawList* dl = ImGui::GetWindowDrawList();
        ImVec2 wp = ImGui::GetWindowPos(), ws = ImGui::GetWindowSize();
        ImVec4 ac = Accent();

        // Title bar
        dl->AddRectFilled(wp, {wp.x+ws.x, wp.y+36}, IM_COL32(10,10,16,240), 10, ImDrawFlags_RoundCornersTop);
        dl->AddRectFilledMultiColor(wp, {wp.x+ws.x, wp.y+2},
            IM_COL32((int)(ac.x*255),(int)(ac.y*255),(int)(ac.z*255),200),
            IM_COL32(255,60,80,120), IM_COL32(255,60,80,120),
            IM_COL32((int)(ac.x*255),(int)(ac.y*255),(int)(ac.z*255),200));
        dl->AddText({wp.x+12, wp.y+10}, Colors::TextPrimary, APP_NAME);
        dl->AddText({wp.x+ws.x-80, wp.y+10}, Colors::TextSecondary, APP_VERSION);

        // Footer
        dl->AddRectFilled({wp.x, wp.y+ws.y-24}, {wp.x+ws.x, wp.y+ws.y}, IM_COL32(8,8,14,220), 10, ImDrawFlags_RoundCornersBottom);
        dl->AddText({wp.x+12, wp.y+ws.y-18}, Colors::TextDim, "Among Stara Client | github.com/stara");

        ImGui::SetCursorPosY(40);

        // Sidebar + Content
        ImGui::BeginChild("##sidebar", {160, ws.y-68}, false);
        ImGui::Spacing();
        for(int i=0; i<(int)Tab::Count; i++) {
            Tab t = (Tab)i;
            bool sel = (g_tab == t);
            float &ha = g_hoverAnim[i];
            ha += ((sel||ImGui::IsMouseHoveringRect(
                ImGui::GetCursorScreenPos(),
                {ImGui::GetCursorScreenPos().x+156, ImGui::GetCursorScreenPos().y+32}
            )) ? 1.f : 0.f - ha) * dt * 10;
            ha = std::clamp(ha, 0.f, 1.f);

            ImVec2 cp = ImGui::GetCursorScreenPos();
            ImDrawList* sdl = ImGui::GetWindowDrawList();

            // Hover bg
            if(ha > 0.01f) {
                sdl->AddRectFilled(cp, {cp.x+156, cp.y+32},
                    IM_COL32((int)(ac.x*255*.1f),(int)(ac.y*255*.1f),(int)(ac.z*255*.1f),(int)(60*ha)), 6);
            }
            // Active bar
            if(sel) {
                sdl->AddRectFilled({cp.x,cp.y+4},{cp.x+3,cp.y+28},
                    IM_COL32((int)(ac.x*255),(int)(ac.y*255),(int)(ac.z*255),255), 2);
            }

            ImGui::PushStyleColor(ImGuiCol_Button, {0,0,0,0});
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, {0,0,0,0});
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, {0,0,0,0});
            if(ImGui::Button(TabName(t), {156, 32})) g_tab = t;
            ImGui::PopStyleColor(3);
        }
        ImGui::EndChild();

        ImGui::SameLine();

        // Separator line
        {
            ImVec2 sp = ImGui::GetCursorScreenPos();
            dl->AddLine({sp.x, wp.y+40}, {sp.x, wp.y+ws.y-24}, IM_COL32(50,50,80,100));
        }

        ImGui::SameLine();

        // Content
        ImGui::BeginChild("##content", {0,ws.y-68}, false);
        ImGui::Spacing();

        switch(g_tab) {
            case Tab::Dashboard: TabDashboard(); break;
            case Tab::Player:    TabPlayer(); break;
            case Tab::Visuals:   TabVisuals(); break;
            case Tab::ESP:       TabESP(); break;
            case Tab::Movement:  TabMovement(); break;
            case Tab::Fun:       TabFun(); break;
            case Tab::Troll:     TabTroll(); break;
            case Tab::Cosmetics: TabCosmetics(); break;
            case Tab::Settings:  TabSettings(); break;
            default: break;
        }

        ImGui::EndChild();
    }
    ImGui::End();

    ImGui::PopStyleVar();
}

} // namespace Stara
