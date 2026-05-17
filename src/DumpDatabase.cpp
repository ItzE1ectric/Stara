#include "DumpDatabase.hpp"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <unordered_set>

namespace Stara::DumpDatabase {
namespace {
namespace fs = std::filesystem;

struct State {
  bool loaded = false;
  std::string loadedPath;
  std::unordered_map<std::string, uintptr_t> methodRvas;
  std::unordered_map<std::string, uint32_t> fieldOffsets;
  std::unordered_set<std::string> classes;
};

static State g_state;
static std::mutex g_lock;
static const std::string g_emptyString;

static std::string Trim(const std::string &s) {
  size_t b = 0;
  while (b < s.size() &&
         (s[b] == ' ' || s[b] == '\t' || s[b] == '\r' || s[b] == '\n'))
    b++;
  size_t e = s.size();
  while (e > b && (s[e - 1] == ' ' || s[e - 1] == '\t' || s[e - 1] == '\r' ||
                   s[e - 1] == '\n'))
    e--;
  return s.substr(b, e - b);
}

static std::string BuildKey(const std::string &className,
                            const std::string &member) {
  return className + "::" + member;
}

static bool ParseHexAfter(const std::string &line, const std::string &marker,
                          uintptr_t &out) {
  size_t p = line.find(marker);
  if (p == std::string::npos)
    return false;
  p += marker.size();
  size_t e = p;
  while (e < line.size()) {
    char c = line[e];
    bool hex = (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') ||
               (c >= 'A' && c <= 'F');
    if (!hex)
      break;
    e++;
  }
  if (e <= p)
    return false;
  try {
    out = (uintptr_t)std::stoull(line.substr(p, e - p), nullptr, 16);
    return true;
  } catch (...) {
    return false;
  }
}

static std::string ParseTypeName(const std::string &line) {
  if (line.find("//") == 0)
    return {};

  static const char *tokens[] = {" class ", " struct ", " enum "};
  size_t pos = std::string::npos;
  size_t tokLen = 0;
  for (const char *t : tokens) {
    size_t p = line.find(t);
    if (p != std::string::npos) {
      pos = p + strlen(t);
      tokLen = strlen(t);
      break;
    }
  }
  if (pos == std::string::npos || tokLen == 0)
    return {};

  while (pos < line.size() && (line[pos] == ' ' || line[pos] == '\t'))
    pos++;
  if (pos >= line.size())
    return {};

  size_t end = pos;
  while (end < line.size()) {
    char c = line[end];
    if (c == ' ' || c == '\t' || c == ':' || c == '{')
      break;
    end++;
  }
  if (end <= pos)
    return {};
  std::string name = line.substr(pos, end - pos);
  if (name == "<Module>")
    return {};
  return name;
}

static std::string ParseMethodNameFromSignature(const std::string &line) {
  if (line.empty() || line[0] == '/' || line[0] == '[')
    return {};
  size_t paren = line.find('(');
  if (paren == std::string::npos)
    return {};

  std::string prefix = Trim(line.substr(0, paren));
  if (prefix.empty())
    return {};

  size_t sp = prefix.find_last_of(" \t");
  if (sp == std::string::npos || sp + 1 >= prefix.size())
    return {};

  std::string name = prefix.substr(sp + 1);
  // Filter obvious non-method lines.
  if (name == "if" || name == "for" || name == "while")
    return {};
  return name;
}

static bool
ParseFieldLine(const std::string &className, const std::string &line,
               std::unordered_map<std::string, uint32_t> &outFields) {
  size_t marker = line.find("// 0x");
  if (marker == std::string::npos)
    return false;
  size_t semi = line.find(';');
  if (semi == std::string::npos || semi > marker)
    return false;

  std::string left = Trim(line.substr(0, semi));
  if (left.find('(') != std::string::npos)
    return false;
  if (left.find('=') != std::string::npos)
    return false;

  size_t sp = left.find_last_of(" \t");
  if (sp == std::string::npos || sp + 1 >= left.size())
    return false;
  std::string fieldName = left.substr(sp + 1);
  if (fieldName.empty())
    return false;

  uintptr_t off = 0;
  if (!ParseHexAfter(line, "0x", off))
    return false;

  std::string key = BuildKey(className, fieldName);
  if (!outFields.count(key))
    outFields[key] = (uint32_t)off;
  return true;
}

static bool ParseDumpFile(const std::string &path, State &dst) {
  std::ifstream in(path);
  if (!in.is_open())
    return false;

  State tmp;
  tmp.loaded = false;
  tmp.loadedPath = path;

  std::string currentClass;
  uintptr_t pendingRva = 0;
  bool hasPendingRva = false;

  std::string line;
  while (std::getline(in, line)) {
    std::string t = Trim(line);
    if (t.empty())
      continue;

    std::string cls = ParseTypeName(t);
    if (!cls.empty()) {
      currentClass = cls;
      tmp.classes.insert(currentClass);
      hasPendingRva = false;
      continue;
    }

    if (!currentClass.empty())
      ParseFieldLine(currentClass, t, tmp.fieldOffsets);

    if (t.rfind("// RVA:", 0) == 0) {
      uintptr_t rva = 0;
      if (ParseHexAfter(t, "RVA: 0x", rva)) {
        pendingRva = rva;
        hasPendingRva = true;
      } else {
        hasPendingRva = false;
      }
      continue;
    }

    if (!hasPendingRva || currentClass.empty())
      continue;

    std::string methodName = ParseMethodNameFromSignature(t);
    if (methodName.empty())
      continue;

    std::string key = BuildKey(currentClass, methodName);
    if (!tmp.methodRvas.count(key))
      tmp.methodRvas[key] = pendingRva;

    hasPendingRva = false;
  }

  if (tmp.methodRvas.empty())
    return false;

  tmp.loaded = true;
  dst = std::move(tmp);
  return true;
}

static std::vector<fs::path> BuildSearchRoots() {
  std::vector<fs::path> roots;
  std::error_code ec;

  roots.push_back(fs::current_path(ec));

  char exeBuf[MAX_PATH] = {};
  DWORD exeLen = GetModuleFileNameA(nullptr, exeBuf, MAX_PATH);
  if (exeLen > 0 && exeLen < MAX_PATH)
    roots.push_back(fs::path(std::string(exeBuf, exeLen)).parent_path());

  HMODULE self = nullptr;
  if (GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                             GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                         (LPCSTR)&BuildSearchRoots, &self)) {
    char dllBuf[MAX_PATH] = {};
    DWORD dllLen = GetModuleFileNameA(self, dllBuf, MAX_PATH);
    if (dllLen > 0 && dllLen < MAX_PATH)
      roots.push_back(fs::path(std::string(dllBuf, dllLen)).parent_path());
  }

  HMODULE ga = GetModuleHandleA("GameAssembly.dll");
  if (ga) {
    char gaBuf[MAX_PATH] = {};
    DWORD gaLen = GetModuleFileNameA(ga, gaBuf, MAX_PATH);
    if (gaLen > 0 && gaLen < MAX_PATH)
      roots.push_back(fs::path(std::string(gaBuf, gaLen)).parent_path());
  }

  // Add parents to improve hit rate when running from build folders.
  size_t initial = roots.size();
  for (size_t i = 0; i < initial; i++) {
    fs::path p = roots[i];
    for (int up = 0; up < 4; up++) {
      if (!p.has_parent_path())
        break;
      p = p.parent_path();
      roots.push_back(p);
    }
  }

  // De-duplicate.
  std::vector<fs::path> uniq;
  std::unordered_set<std::string> seen;
  for (auto &r : roots) {
    std::string s = r.string();
    if (s.empty())
      continue;
    if (seen.insert(s).second)
      uniq.push_back(r);
  }
  return uniq;
}

static bool FindNewestDumpCs(std::string &outPath) {
  std::vector<fs::path> roots = BuildSearchRoots();
  std::error_code ec;
  bool found = false;
  fs::file_time_type newest{};
  fs::path bestPath;

  auto checkFile = [&](const fs::path &p) {
    std::error_code fec;
    if (!fs::exists(p, fec) || !fs::is_regular_file(p, fec))
      return;
    fs::file_time_type ft = fs::last_write_time(p, fec);
    if (fec)
      return;
    if (!found || ft > newest) {
      newest = ft;
      bestPath = p;
      found = true;
    }
  };

  // Optional direct override for deterministic loading.
  char envBuf[MAX_PATH * 2] = {};
  DWORD n = GetEnvironmentVariableA("STARA_DUMP_PATH", envBuf,
                                    (DWORD)std::size(envBuf));
  if (n > 0 && n < std::size(envBuf))
    checkFile(fs::path(envBuf));

  for (const fs::path &root : roots) {
    checkFile(root / "dump.cs");
    checkFile(root / "Game Info Dumps" / "dump.cs");

    fs::path dumpsRoot = root / "Game Info Dumps";
    if (fs::exists(dumpsRoot, ec) && fs::is_directory(dumpsRoot, ec)) {
      fs::recursive_directory_iterator it(
          dumpsRoot, fs::directory_options::skip_permission_denied, ec);
      fs::recursive_directory_iterator end;
      for (; it != end; it.increment(ec)) {
        if (ec)
          break;
        if (it.depth() > 5) {
          it.disable_recursion_pending();
          continue;
        }
        const fs::path &p = it->path();
        if (p.filename() == "dump.cs")
          checkFile(p);
      }
    }
  }

  if (!found)
    return false;
  outPath = bestPath.string();
  return true;
}

} // namespace

bool LoadFromPath(const std::string &path) {
  std::lock_guard<std::mutex> guard(g_lock);
  State parsed;
  if (!ParseDumpFile(path, parsed))
    return false;
  g_state = std::move(parsed);
  printf("[+] DumpDatabase loaded: %s (%zu methods, %zu fields, %zu types)\n",
         g_state.loadedPath.c_str(), g_state.methodRvas.size(),
         g_state.fieldOffsets.size(), g_state.classes.size());
  return true;
}

bool AutoLoad() {
  std::string path;
  if (!FindNewestDumpCs(path))
    return false;
  return LoadFromPath(path);
}

bool IsLoaded() {
  std::lock_guard<std::mutex> guard(g_lock);
  return g_state.loaded;
}

uintptr_t GetMethodRva(const std::string &className,
                       const std::string &methodName, uintptr_t fallback) {
  std::lock_guard<std::mutex> guard(g_lock);
  if (!g_state.loaded)
    return fallback;
  auto it = g_state.methodRvas.find(BuildKey(className, methodName));
  if (it == g_state.methodRvas.end())
    return fallback;
  return it->second;
}

uint32_t GetFieldOffset(const std::string &className,
                        const std::string &fieldName, uint32_t fallback) {
  std::lock_guard<std::mutex> guard(g_lock);
  if (!g_state.loaded)
    return fallback;
  auto it = g_state.fieldOffsets.find(BuildKey(className, fieldName));
  if (it == g_state.fieldOffsets.end())
    return fallback;
  return it->second;
}

size_t MethodCount() {
  std::lock_guard<std::mutex> guard(g_lock);
  return g_state.methodRvas.size();
}

size_t FieldCount() {
  std::lock_guard<std::mutex> guard(g_lock);
  return g_state.fieldOffsets.size();
}

size_t ClassCount() {
  std::lock_guard<std::mutex> guard(g_lock);
  return g_state.classes.size();
}

const std::string &LoadedPath() {
  std::lock_guard<std::mutex> guard(g_lock);
  if (!g_state.loaded)
    return g_emptyString;
  return g_state.loadedPath;
}

std::vector<MethodEntry> GetMethodEntries(size_t maxCount) {
  std::lock_guard<std::mutex> guard(g_lock);
  std::vector<MethodEntry> out;
  if (!g_state.loaded)
    return out;

  out.reserve(g_state.methodRvas.size());
  for (const auto &kv : g_state.methodRvas) {
    MethodEntry e;
    size_t sep = kv.first.find("::");
    if (sep != std::string::npos) {
      e.className = kv.first.substr(0, sep);
      e.methodName = kv.first.substr(sep + 2);
    } else {
      e.className = "Unknown";
      e.methodName = kv.first;
    }
    e.rva = kv.second;
    out.push_back(std::move(e));
  }

  std::sort(out.begin(), out.end(),
            [](const MethodEntry &a, const MethodEntry &b) {
              if (a.className != b.className)
                return a.className < b.className;
              if (a.methodName != b.methodName)
                return a.methodName < b.methodName;
              return a.rva < b.rva;
            });

  if (maxCount > 0 && out.size() > maxCount)
    out.resize(maxCount);
  return out;
}

std::vector<FieldEntry> GetFieldEntries(size_t maxCount) {
  std::lock_guard<std::mutex> guard(g_lock);
  std::vector<FieldEntry> out;
  if (!g_state.loaded)
    return out;

  out.reserve(g_state.fieldOffsets.size());
  for (const auto &kv : g_state.fieldOffsets) {
    FieldEntry e;
    size_t sep = kv.first.find("::");
    if (sep != std::string::npos) {
      e.className = kv.first.substr(0, sep);
      e.fieldName = kv.first.substr(sep + 2);
    } else {
      e.className = "Unknown";
      e.fieldName = kv.first;
    }
    e.offset = kv.second;
    out.push_back(std::move(e));
  }

  std::sort(out.begin(), out.end(),
            [](const FieldEntry &a, const FieldEntry &b) {
              if (a.className != b.className)
                return a.className < b.className;
              if (a.fieldName != b.fieldName)
                return a.fieldName < b.fieldName;
              return a.offset < b.offset;
            });

  if (maxCount > 0 && out.size() > maxCount)
    out.resize(maxCount);
  return out;
}

} // namespace Stara::DumpDatabase
