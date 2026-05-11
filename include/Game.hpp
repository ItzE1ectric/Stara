#pragma once
#include "Common.hpp"

namespace Stara {

// Among Us IL2CPP game data structures and functions
namespace Game {

// Resolved at runtime via IL2CPP exports
inline uintptr_t gameAssembly = 0;

// IL2CPP function typedefs
using il2cpp_domain_get_t        = void* (*)();
using il2cpp_domain_get_assemblies_t = void** (*)(void* domain, size_t* count);
using il2cpp_class_from_name_t   = void* (*)(void* image, const char* ns, const char* name);
using il2cpp_class_get_methods_t = void* (*)(void* klass, void** iter);
using il2cpp_method_get_name_t   = const char* (*)(void* method);
using il2cpp_class_get_fields_t  = void* (*)(void* klass, void** iter);
using il2cpp_field_get_name_t    = const char* (*)(void* field);
using il2cpp_field_get_offset_t  = size_t (*)(void* field);
using il2cpp_string_new_t        = void* (*)(const char* str);
using il2cpp_assembly_get_image_t = void* (*)(void* assembly);
using il2cpp_class_get_field_from_name_t = void* (*)(void* klass, const char* name);
using il2cpp_field_static_get_value_t = void (*)(void* field, void* value);
using il2cpp_field_set_value_t = void (*)(void* obj, void* field, void* value);
using il2cpp_field_get_value_t = void (*)(void* obj, void* field, void* value);
using il2cpp_class_get_method_from_name_t = void* (*)(void* klass, const char* name, int argsCount);
using il2cpp_runtime_invoke_t = void* (*)(void* method, void* obj, void** params, void** exc);

// Function pointers resolved at init
inline il2cpp_domain_get_t         il2cpp_domain_get = nullptr;
inline il2cpp_domain_get_assemblies_t il2cpp_domain_get_assemblies = nullptr;
inline il2cpp_class_from_name_t    il2cpp_class_from_name = nullptr;
inline il2cpp_class_get_methods_t  il2cpp_class_get_methods = nullptr;
inline il2cpp_method_get_name_t    il2cpp_method_get_name = nullptr;
inline il2cpp_string_new_t         il2cpp_string_new = nullptr;
inline il2cpp_assembly_get_image_t il2cpp_assembly_get_image = nullptr;
inline il2cpp_field_get_offset_t   il2cpp_field_get_offset = nullptr;
inline il2cpp_class_get_field_from_name_t il2cpp_class_get_field_from_name = nullptr;
inline il2cpp_field_static_get_value_t il2cpp_field_static_get_value = nullptr;
inline il2cpp_field_set_value_t il2cpp_field_set_value = nullptr;
inline il2cpp_field_get_value_t il2cpp_field_get_value = nullptr;
inline il2cpp_class_get_method_from_name_t il2cpp_class_get_method_from_name = nullptr;
inline il2cpp_runtime_invoke_t il2cpp_runtime_invoke = nullptr;

// Game classes (resolved at runtime)
struct PlayerControl {
    static inline void* klass = nullptr;
    static inline size_t offset_moveable = 0x38;
    static inline size_t offset_MyPhysics = 0x94;
    static inline size_t offset_NetTransform = 0x98;
};

struct PlayerPhysics {
    static inline void* klass = nullptr;
    static inline size_t offset_Speed = 0x34;
    static inline size_t offset_GhostSpeed = 0x38;
    static inline size_t offset_body = 0x40;
};

struct GameData {
    static inline void* klass = nullptr;
};

struct ShipStatus {
    static inline void* klass = nullptr;
};

struct AmongUsClient {
    static inline void* klass = nullptr;
};

// Player info for ESP/visuals
struct PlayerInfo {
    std::string name = "Player";
    ImVec4 color = {1,1,1,1};
    bool isDead = false;
    bool isImpostor = false;
    float x = 0, y = 0;
    float distance = 0;
    int tasksDone = 0;
    int totalTasks = 0;
};

// Runtime state
inline std::vector<PlayerInfo> players;
inline bool isInGame = false;
inline bool isImpostor = false;
inline float localX = 0, localY = 0;

// Init IL2CPP function pointers
bool Init();
void Update(); // called each frame to refresh player data

// Game manipulation (all write to local state or IL2CPP)
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
void SetCharacterScale(float scale);

// Among Us player color palette
inline ImVec4 GetAmongUsColor(int id) {
    const ImVec4 colors[] = {
        {0.78f,0.17f,0.16f,1}, // Red
        {0.07f,0.18f,0.90f,1}, // Blue
        {0.11f,0.50f,0.18f,1}, // Green
        {0.93f,0.55f,0.76f,1}, // Pink
        {0.94f,0.55f,0.13f,1}, // Orange
        {0.94f,0.90f,0.25f,1}, // Yellow
        {0.24f,0.24f,0.24f,1}, // Black
        {0.84f,0.84f,0.84f,1}, // White
        {0.38f,0.14f,0.56f,1}, // Purple
        {0.44f,0.31f,0.20f,1}, // Brown
        {0.00f,0.93f,0.93f,1}, // Cyan
        {0.44f,0.78f,0.22f,1}, // Lime
    };
    if (id < 0 || id >= 12) return {1,1,1,1};
    return colors[id];
}

} // namespace Game
} // namespace Stara
