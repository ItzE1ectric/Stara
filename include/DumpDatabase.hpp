#pragma once
#include "Common.hpp"

namespace Stara::DumpDatabase {

struct MethodEntry {
  std::string className;
  std::string methodName;
  uintptr_t rva = 0;
};

struct FieldEntry {
  std::string className;
  std::string fieldName;
  uint32_t offset = 0;
};

bool AutoLoad();
bool LoadFromPath(const std::string &path);
bool IsLoaded();

uintptr_t GetMethodRva(const std::string &className,
                       const std::string &methodName,
                       uintptr_t fallback = 0);
uint32_t GetFieldOffset(const std::string &className,
                        const std::string &fieldName,
                        uint32_t fallback = 0);

size_t MethodCount();
size_t FieldCount();
size_t ClassCount();
const std::string &LoadedPath();

std::vector<MethodEntry> GetMethodEntries(size_t maxCount = 0);
std::vector<FieldEntry> GetFieldEntries(size_t maxCount = 0);

} // namespace Stara::DumpDatabase
