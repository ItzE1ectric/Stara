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
float g_speed = 1.f;
float g_fov = 90.f, g_zoom = 1.f, g_bloom = 0.5f;
float g_walkSpeed = 1.f, g_animSpeed = 1.f, g_uiScale = 1.f;
float g_ping = 50.f, g_taskProg = 0.f, g_themeInt = 1.f,
             g_blurInt = 0.8f;
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
bool g_noKillCd = false, g_infiniteEmergencies = false, g_alwaysMoveable = false, g_impostorVision = false;
bool g_maxReportDist = false, g_autoTasks = false, g_freezeAll = false, g_colorCycle = false;
bool g_antiKick = false, g_forceProtect = false, g_spamAnim = false, g_godmode = false;
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

// ── Custom Widgets ─────────────────────────────────────────────────
static bool Toggle(const char *label, bool *v) {
  ImGuiWindow *w = ImGui::GetCurrentWindow();
  if (w->SkipItems)
    return false;
  ImGuiID id = w->GetID(label);
  float h = 20, wd = 38, r = h * .5f;
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
  int ti = g_toggleIdx++ % 64;
  float &ta = g_toggleAnim[ti];
  float tgt = *v ? 1.f : 0.f;
  ta += (tgt - ta) * 0.15f;
  ImDrawList *dl = w->DrawList;
  ImVec4 ac = Accent();
  ImU32 bg = *v ? IM_COL32((int)(ac.x * 255 * .5f), (int)(ac.y * 255 * .5f),
                           (int)(ac.z * 255 * .5f), 200)
                : IM_COL32(40, 40, 60, 200);
  dl->AddRectFilled(p, {p.x + wd, p.y + h}, bg, r);
  float kx = p.x + r + (wd - h) * ta;
  dl->AddCircleFilled({kx, p.y + r}, r - 3, IM_COL32(255, 255, 255, 240), 24);
  dl->AddText({p.x + wd + 8, p.y + (h - ls.y) * .5f}, Colors::TextPrimary,
              label);
  return press;
}

static bool GlowBtn(const char *label, ImVec2 sz = {0, 0}) {
  ImVec4 ac = Accent();
  ImGui::PushStyleColor(ImGuiCol_Button,
                        {ac.x * .15f, ac.y * .15f, ac.z * .15f, .8f});
  ImGui::PushStyleColor(ImGuiCol_ButtonHovered,
                        {ac.x * .25f, ac.y * .25f, ac.z * .25f, .9f});
  ImGui::PushStyleColor(ImGuiCol_ButtonActive,
                        {ac.x * .35f, ac.y * .35f, ac.z * .35f, 1});
  bool r = ImGui::Button(label, sz);
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
  ImGui::PushStyleColor(ImGuiCol_ChildBg, {.06f, .06f, .1f, .85f});
  ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 10);
  ImGui::BeginChild(title, {0, 0},
                    ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_Borders);
  ImDrawList *dl = ImGui::GetWindowDrawList();
  ImVec2 p = ImGui::GetWindowPos(), s = ImGui::GetWindowSize();
  ImVec4 ac = Accent();
  dl->AddRectFilledMultiColor(
      p, {p.x + s.x, p.y + 2},
      IM_COL32((int)(ac.x * 255), (int)(ac.y * 255), (int)(ac.z * 255), 150),
      IM_COL32(255, 60, 80, 80), IM_COL32(255, 60, 80, 80),
      IM_COL32((int)(ac.x * 255), (int)(ac.y * 255), (int)(ac.z * 255), 150));
  ImGui::TextColored({.94f, .94f, 1, 1}, "%s", title);
  ImGui::Spacing();
}
static void EndCard() {
  ImGui::EndChild();
  ImGui::PopStyleVar();
  ImGui::PopStyleColor();
  ImGui::Spacing();
}

static void Sep(const char *t) {
  ImGui::Spacing();
  ImGui::TextColored({.4f, .4f, .5f, 1}, "— %s —", t);
  ImGui::Spacing();
}

// ── Tab Renderers ──────────────────────────────────────────────────
static void TabDashboard() {
  Card("Profile");
  ImGui::Text("Player: %s", g_nameBuf);
  ImGui::SameLine(200);
  ImGui::TextColored({0, 1, .5f, 1}, "Online");
  ImGui::Text("Version: %s", APP_VERSION);
  EndCard();
  Card("Session Stats");
  ImGui::Text("Games Played: 0");
  ImGui::Text("Wins: 0");
  ImGui::Text("Impostor Wins: 0");
  ImGui::Text("Tasks Completed: 0");
  ImGui::Text("Kills: 0");
  EndCard();
  Card("Quick Actions");
  if (GlowBtn("Complete Tasks", {140, 30}))
    Game::CompleteAllTasks();
  ImGui::SameLine();
  if (GlowBtn("Emergency Meeting", {160, 30}))
    Game::ForceEmergencyMeeting();
  EndCard();
}

static void TabPlayer() {
  Card("Player Settings");
  Toggle("NoClip", &g_noclip);
  Slider("Speed##p", &g_speed, 0.5f, 10.f);
  if (ImGui::IsItemDeactivatedAfterEdit())
    Game::SetSpeed(g_speed);
  ImGui::InputText("Name##p", g_nameBuf, 64);
  if (ImGui::IsItemDeactivatedAfterEdit())
    Game::SetName(g_nameBuf);
  static int g_level = 100;
  ImGui::InputInt("Level##p", &g_level);
  if (ImGui::IsItemDeactivatedAfterEdit())
    Game::SetLevel(g_level);
  EndCard();

  Card("Role");
  static int selRole = 0;
  const char* roles[] = {"Crewmate","Impostor","Scientist","Engineer","Guardian Angel",
                         "Shapeshifter","Crewmate Ghost","Impostor Ghost","Noisemaker",
                         "Phantom","Tracker"};
  const int roleIds[] = {0,1,2,3,4,5,6,7,8,9,10};
  ImGui::Combo("Set Role##r", &selRole, roles, 11);
  if (GlowBtn("Apply Role", {140, 28}))
    Game::SetRole(roleIds[selRole]);
  ImGui::SameLine();
  if (GlowBtn("Revive Self", {120, 28}))
    Game::RevivePlayer();
  EndCard();

  Card("Lobby Actions");
  if (GlowBtn("Force Start Game", {180, 28}))
    Game::StartGame();
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
  Toggle("Wallhack / No Shadows##op", &g_wallhack);
  if (ImGui::IsItemDeactivatedAfterEdit())
    Game::SetWallhack(g_wallhack);
  EndCard();

  Card("Cheats");
  Toggle("Infinite Emergencies", &g_infiniteEmergencies);
  Toggle("Always Moveable", &g_alwaysMoveable);
  Toggle("Impostor Vision", &g_impostorVision);
  Toggle("Max Report Distance", &g_maxReportDist);
  Toggle("Auto Complete Tasks", &g_autoTasks);
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
      if (p.isDead) continue;
      ImGui::PushID(i);
      if (GlowBtn(p.name.c_str(), {140, 24}))
        Game::KillPlayer(i + 1);
      ImGui::SameLine();
      ImGui::TextColored(p.isImpostor ? ImVec4{1,.2f,.2f,1} : ImVec4{.5f,1,.5f,1}, "%s", p.roleName.c_str());
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
  Toggle("Fullbright", &g_fullbright);
  if (ImGui::IsItemDeactivatedAfterEdit())
    Game::SetFullbright(g_fullbright);
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
        if (p.isDead) {
          ImGui::TextColored({0.5f, 0.5f, 0.5f, 1}, "%s [DEAD]", p.name.c_str());
        } else if (p.isImpostor) {
          ImGui::TextColored({1, 0.1f, 0.1f, 1}, "%s [IMP]", p.name.c_str());
        } else {
          ImGui::Text("%s", p.name.c_str());
        }

        ImGui::TableNextColumn();
        if (p.isDead) {
           ImGui::TextColored({0.4f, 0.4f, 0.4f, 1}, "DEAD (%s)", p.roleName.c_str());
        } else {
           ImU32 roleCol = p.isImpostor ? IM_COL32(255, 100, 100, 255) : IM_COL32(100, 255, 100, 255);
           ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(roleCol), "%s", p.roleName.c_str());
        }

        ImGui::TableNextColumn();
        ImGui::Text("%.1fm", p.distance);
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
  Slider("Walk Speed##m", &g_walkSpeed, 0.5f, 10);
  if (ImGui::IsItemDeactivatedAfterEdit())
    Game::SetSpeed(g_walkSpeed);
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
    ImGui::TextColored({1,0.3f,0.3f,1}, "Not in game");
  } else {
    for (int i = 0; i < (int)Game::players.size(); i++) {
      const auto &p = Game::players[i];
      if (p.isDead) continue;
      ImGui::PushID(100 + i);
      if (GlowBtn(("Shift -> " + p.name).c_str(), {200, 24}))
        Game::ShapeshiftTo(i + 1);
      ImGui::PopID();
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
  // SystemTypes: 0=Hallway,1=Storage,2=Cafeteria,3=Reactor,4=UpperEngine,5=Navigation,
  //              6=Admin,7=Electrical,8=O2,9=Shields,10=MedBay,11=Security,12=Weapons,
  //              13=LowerEngine,14=ShortComms,15=LobbyComm
  const char* rooms[] = {"Hallway","Storage","Cafeteria","Reactor","Upper Engine",
                         "Navigation","Admin","Electrical","O2","Shields",
                         "MedBay","Security","Weapons","Lower Engine","Comms"};
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
    ImGui::TextColored({1,0.3f,0.3f,1}, "Not in game");
  } else {
    for (int i = 0; i < (int)Game::players.size(); i++) {
      const auto &p = Game::players[i];
      if (p.isDead) continue;
      ImGui::PushID(200 + i);
      if (GlowBtn(("TP -> " + p.name).c_str(), {180, 24}))
        Game::TeleportToPlayer(i + 1);
      ImGui::PopID();
    }
  }
  EndCard();

  Card("Teleport To Room (Skeld)");
  const char* skeldRooms[] = {"Cafeteria","Reactor","Navigation","MedBay","Electrical",
                              "Storage","Weapons","Upper Engine","Lower Engine"};
  for (int i = 0; i < 9; i++) {
    ImGui::PushID(300 + i);
    if (GlowBtn(skeldRooms[i], {140, 24}))
      Game::TeleportToRoom(i);
    if ((i % 3) != 2) ImGui::SameLine();
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
  const char *visors[] = {"None", "Lollipop (Crew)", "Lollipop (Imp)", "Star (Crew)", "Angery"};
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
  const char *colors[] = {"Red","Blue","Green","Pink","Orange","Yellow","Black","White","Purple","Brown","Cyan","Lime","Maroon","Rose","Banana","Gray","Tan","Coral"};
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
    }
  }
  ImGui::SameLine();
  if (GlowBtn("Reset", {80, 28})) {
    g_speed = 1.0f;
    g_fov = 90.0f;
  }
  EndCard();
  Card("Keybinds");
  ImGui::Text("Toggle Menu: INSERT");
  ImGui::Text("Unload DLL: END");
  EndCard();
}

// ── Apply Theme ────────────────────────────────────────────────────
static void ApplyTheme() {
  ImGuiStyle &s = ImGui::GetStyle();
  s.WindowPadding = {12, 12};
  s.FramePadding = {8, 5};
  s.ItemSpacing = {8, 6};
  s.ScrollbarSize = 10;
  s.WindowBorderSize = 0;
  s.ChildBorderSize = 0;
  s.WindowRounding = 10;
  s.ChildRounding = 10;
  s.FrameRounding = 6;
  s.PopupRounding = 8;
  s.ScrollbarRounding = 12;
  s.GrabRounding = 6;
  ImVec4 *c = s.Colors;
  c[ImGuiCol_WindowBg] = {.047f, .047f, .07f, .94f};
  c[ImGuiCol_ChildBg] = {0, 0, 0, 0};
  c[ImGuiCol_PopupBg] = {.07f, .07f, .11f, .95f};
  c[ImGuiCol_Border] = {.2f, .2f, .31f, .4f};
  c[ImGuiCol_FrameBg] = {.08f, .08f, .12f, .8f};
  c[ImGuiCol_FrameBgHovered] = {.12f, .12f, .18f, .9f};
  c[ImGuiCol_FrameBgActive] = {.14f, .14f, .22f, 1};
  c[ImGuiCol_ScrollbarBg] = {.05f, .05f, .08f, .5f};
  c[ImGuiCol_Text] = {.94f, .94f, 1, 1};
  c[ImGuiCol_TextDisabled] = {.4f, .4f, .5f, .8f};
  ImVec4 ac = Accent();
  c[ImGuiCol_CheckMark] = ac;
  c[ImGuiCol_SliderGrab] = ac;
  c[ImGuiCol_Button] = {ac.x * .15f, ac.y * .15f, ac.z * .15f, .8f};
  c[ImGuiCol_ButtonHovered] = {ac.x * .25f, ac.y * .25f, ac.z * .25f, .9f};
  c[ImGuiCol_ButtonActive] = {ac.x * .35f, ac.y * .35f, ac.z * .35f, 1};
  c[ImGuiCol_Header] = {ac.x * .15f, ac.y * .15f, ac.z * .15f, .6f};
  c[ImGuiCol_HeaderHovered] = {ac.x * .2f, ac.y * .2f, ac.z * .2f, .8f};
  c[ImGuiCol_Separator] = {.2f, .2f, .31f, .4f};
}

// ── Main Render ────────────────────────────────────────────────────
void RenderMenu() {
  float dt = ImGui::GetIO().DeltaTime;
  g_toggleIdx = 0;

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

  ApplyTheme();
  ImGui::PushStyleVar(ImGuiStyleVar_Alpha, g_menuAlpha);

  // FPS
  if (g_fpsDisp) {
    ImGui::GetForegroundDrawList()->AddText(
        {10, 10}, IM_COL32(0, 220, 255, (int)(200 * g_menuAlpha)),
        (std::string("FPS: ") + std::to_string((int)(1.f / dt))).c_str());
  }

  // Background particles
  {
    ImDrawList *dl = ImGui::GetBackgroundDrawList();
    static struct {
      float x, y, vx, vy, s, a;
    } pts[60];
    static bool pinit = false;
    if (!pinit) {
      pinit = true;
      for (auto &p : pts) {
        p.x = rand() % 1920;
        p.y = rand() % 1080;
        p.vx = (rand() % 100 - 50) * .1f;
        p.vy = (rand() % 100 - 50) * .08f - .3f;
        p.s = 1 + rand() % 3;
        p.a = .2f + .3f * (rand() % 100) / 100.f;
      }
    }
    for (auto &p : pts) {
      p.x += p.vx * dt * 20;
      p.y += p.vy * dt * 20;
      if (p.x < 0)
        p.x = 1920;
      if (p.x > 1920)
        p.x = 0;
      if (p.y < 0)
        p.y = 1080;
      if (p.y > 1080)
        p.y = 0;
      ImU32 c = IM_COL32(0, 180, 255, (int)(p.a * 60 * g_menuAlpha));
      dl->AddCircleFilled({p.x, p.y}, p.s * 1.5f, c, 12);
    }
  }

  // Main window
  ImGui::SetNextWindowSize({820, 520}, ImGuiCond_FirstUseEver);
  ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar |
                           ImGuiWindowFlags_NoScrollbar |
                           ImGuiWindowFlags_NoCollapse;

  if (ImGui::Begin("##StaraMain", nullptr, flags)) {
    ImDrawList *dl = ImGui::GetWindowDrawList();
    ImVec2 wp = ImGui::GetWindowPos(), ws = ImGui::GetWindowSize();
    ImVec4 ac = Accent();

    // Title bar
    dl->AddRectFilled(wp, {wp.x + ws.x, wp.y + 36}, IM_COL32(10, 10, 16, 240),
                      10, ImDrawFlags_RoundCornersTop);
    dl->AddRectFilledMultiColor(
        wp, {wp.x + ws.x, wp.y + 2},
        IM_COL32((int)(ac.x * 255), (int)(ac.y * 255), (int)(ac.z * 255), 200),
        IM_COL32(255, 60, 80, 120), IM_COL32(255, 60, 80, 120),
        IM_COL32((int)(ac.x * 255), (int)(ac.y * 255), (int)(ac.z * 255), 200));
    dl->AddText({wp.x + 12, wp.y + 10}, Colors::TextPrimary, APP_NAME);
    dl->AddText({wp.x + ws.x - 80, wp.y + 10}, Colors::TextSecondary,
                APP_VERSION);

    // Footer
    dl->AddRectFilled({wp.x, wp.y + ws.y - 24}, {wp.x + ws.x, wp.y + ws.y},
                      IM_COL32(8, 8, 14, 220), 10,
                      ImDrawFlags_RoundCornersBottom);
    dl->AddText({wp.x + 12, wp.y + ws.y - 18}, Colors::TextDim,
                "Among Stara Client | github.com/stara");

    ImGui::SetCursorPosY(40);

    // Sidebar + Content
    ImGui::BeginChild("##sidebar", {160, ws.y - 68}, false);
    ImGui::Spacing();
    for (int i = 0; i < (int)Tab::Count; i++) {
      Tab t = (Tab)i;
      bool sel = (g_tab == t);
      float &ha = g_hoverAnim[i];
      ha += ((sel ||
              ImGui::IsMouseHoveringRect(ImGui::GetCursorScreenPos(),
                                         {ImGui::GetCursorScreenPos().x + 156,
                                          ImGui::GetCursorScreenPos().y + 32}))
                 ? 1.f
                 : 0.f - ha) *
            dt * 10;
      ha = std::clamp(ha, 0.f, 1.f);

      ImVec2 cp = ImGui::GetCursorScreenPos();
      ImDrawList *sdl = ImGui::GetWindowDrawList();

      // Hover bg
      if (ha > 0.01f) {
        sdl->AddRectFilled(cp, {cp.x + 156, cp.y + 32},
                           IM_COL32((int)(ac.x * 255 * .1f),
                                    (int)(ac.y * 255 * .1f),
                                    (int)(ac.z * 255 * .1f), (int)(60 * ha)),
                           6);
      }
      // Active bar
      if (sel) {
        sdl->AddRectFilled({cp.x, cp.y + 4}, {cp.x + 3, cp.y + 28},
                           IM_COL32((int)(ac.x * 255), (int)(ac.y * 255),
                                    (int)(ac.z * 255), 255),
                           2);
      }

      ImGui::PushStyleColor(ImGuiCol_Button, {0, 0, 0, 0});
      ImGui::PushStyleColor(ImGuiCol_ButtonHovered, {0, 0, 0, 0});
      ImGui::PushStyleColor(ImGuiCol_ButtonActive, {0, 0, 0, 0});
      if (ImGui::Button(TabName(t), {156, 32}))
        g_tab = t;
      ImGui::PopStyleColor(3);
    }
    ImGui::EndChild();

    ImGui::SameLine();

    // Separator line
    {
      ImVec2 sp = ImGui::GetCursorScreenPos();
      dl->AddLine({sp.x, wp.y + 40}, {sp.x, wp.y + ws.y - 24},
                  IM_COL32(50, 50, 80, 100));
    }

    ImGui::SameLine();

    // Content
    ImGui::BeginChild("##content", {0, ws.y - 68}, false);
    ImGui::Spacing();

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

    ImGui::EndChild();
  }
  ImGui::End();

  ImGui::PopStyleVar();
}

} // namespace Stara
