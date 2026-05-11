#pragma once
#include "Common.hpp"
#include <vector>
#include <string>

namespace Stara::Game {
    // IL2CPP Types
    typedef void* (*il2cpp_domain_get_t)();
    typedef void** (*il2cpp_domain_get_assemblies_t)(void* domain, size_t* size);
    typedef void* (*il2cpp_class_from_name_t)(void* image, const char* namespaze, const char* name);
    typedef void* (*il2cpp_class_get_methods_t)(void* klass, void** iter);
    typedef const char* (*il2cpp_method_get_name_t)(void* method);
    typedef void* (*il2cpp_string_new_t)(const char* str);
    typedef void* (*il2cpp_assembly_get_image_t)(void* assembly);
    typedef uint32_t (*il2cpp_field_get_offset_t)(void* field);
    typedef void* (*il2cpp_class_get_field_from_name_t)(void* klass, const char* name);
    typedef void (*il2cpp_field_static_get_value_t)(void* field, void* value);
    typedef void (*il2cpp_field_set_value_t)(void* obj, void* field, void* value);
    typedef void (*il2cpp_field_get_value_t)(void* obj, void* field, void* value);
    typedef void* (*il2cpp_class_get_method_from_name_t)(void* klass, const char* name, int argsCount);
    typedef void* (*il2cpp_runtime_invoke_t)(void* method, void* obj, void** params, void** exc);
    typedef void* (*il2cpp_thread_attach_t)(void* domain);

    // Globals
    inline uintptr_t gameAssembly = 0;
    inline bool isInGame = false;
    inline float localX = 0, localY = 0;
    inline bool isImpostor = false;

    struct PlayerInfo {
        std::string name;
        float x, y;
        bool isImpostor;
        bool isDead;
        float distance;
        std::string roleName;
    };

    inline std::vector<PlayerInfo> players;

    // IL2CPP Functions
    inline il2cpp_domain_get_t il2cpp_domain_get;
    inline il2cpp_domain_get_assemblies_t il2cpp_domain_get_assemblies;
    inline il2cpp_class_from_name_t il2cpp_class_from_name;
    inline il2cpp_class_get_methods_t il2cpp_class_get_methods;
    inline il2cpp_method_get_name_t il2cpp_method_get_name;
    inline il2cpp_string_new_t il2cpp_string_new;
    inline il2cpp_assembly_get_image_t il2cpp_assembly_get_image;
    inline il2cpp_field_get_offset_t il2cpp_field_get_offset;
    inline il2cpp_class_get_field_from_name_t il2cpp_class_get_field_from_name;
    inline il2cpp_field_static_get_value_t il2cpp_field_static_get_value;
    inline il2cpp_field_set_value_t il2cpp_field_set_value;
    inline il2cpp_field_get_value_t il2cpp_field_get_value;
    inline il2cpp_class_get_method_from_name_t il2cpp_class_get_method_from_name;
    inline il2cpp_runtime_invoke_t il2cpp_runtime_invoke;
    inline il2cpp_thread_attach_t il2cpp_thread_attach;

    // Class Pointers
    namespace PlayerControl { inline void* klass = nullptr; }
    namespace PlayerPhysics { inline void* klass = nullptr; }
    namespace GameData      { inline void* klass = nullptr; }
    namespace ShipStatus    { inline void* klass = nullptr; }
    namespace AmongUsClient { inline void* klass = nullptr; }
    namespace GameOptionsManager { inline void* klass = nullptr; }
    namespace Transform { inline void* klass = nullptr; }
    namespace Behaviour { inline void* klass = nullptr; }
    namespace NetworkedPlayerInfo { inline void* klass = nullptr; }

    // Core Functions
    bool Init();
    void Update();
    void DrawESP(ImDrawList* drawList);

    // Cheats
    void SetSpeed(float speed);
    void SetFullbright(bool enabled);
    void CompleteAllTasks();
    void ForceEmergencyMeeting();
    void SetPlayerColor(int colorId);
    void TeleportTo(float x, float y);
    void SetName(const char* name);
    void SetKillCooldown(float time);
    void SetKillDistance(float dist);
    void SetWallhack(bool enabled);
    void SetHat(int hatId);
    void SetPet(int petId);
    void SetSkin(int skinId);
    void SetCharacterScale(float scale);
    void PlayAnimation(uint8_t animId);
    
    // New Cheats
    void SpamChat(const char* text);
    void EndGame();
    void StartGame();
    void TeleportToRoom(int roomId);
    void SetRole(int roleType);
    void KillPlayer(int playerIndex);
    void KillAllPlayers();
    void ReportBody(int playerIndex);
    void SetVisor(int visorId);
    void SetNamePlate(int npId);
    void SetLevel(int level);
    void RevivePlayer();
    void ShapeshiftTo(int playerIndex);
    void Vanish();
    void Appear();
    void EnterVent(int ventId);
    void ExitVent(int ventId);
    void CloseDoors(int roomType);
    void RepairSabotage(int systemType);
    void TeleportToPlayer(int playerIndex);
    void ProtectPlayer(int playerIndex);

} // namespace Stara::Game
