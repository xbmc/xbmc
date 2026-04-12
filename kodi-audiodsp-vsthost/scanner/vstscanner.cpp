/*
 * vstscanner.cpp — Standalone out-of-process VST plugin scanner
 *
 * Enumerates .dll and .vst3 files in one or more directory trees, probes
 * each for VST2 / VST3 metadata, and writes one NDJSON object per file to
 * stdout.  Structured Exception Handling (SEH) contains crashes inside
 * individual plugin entry points so the scan continues to the next file.
 *
 * Build:  added automatically by CMakeLists.txt when this file exists.
 * Usage:  vstscanner [dir1] [dir2] ...
 *         (No args → scans default Windows VST install paths + registry path)
 */

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <filesystem>
#include <string>
#include <vector>
#include <iostream>

#include "../src/vst2/vestige/aeffectx.h"

// ---------------------------------------------------------------------------
// Forward declarations for VST3 hosting (VST3 SDK public.sdk/hosting)
// ---------------------------------------------------------------------------
#ifdef VST3SDK_AVAILABLE
#include "public.sdk/source/vst/hosting/module.h"
#include "pluginterfaces/vst/ivstaudioprocessor.h"
static constexpr const char* kVstAudioEffectClass = "Audio Module Class";
#endif

// ---------------------------------------------------------------------------
// PluginInfo — data collected for a single file
// ---------------------------------------------------------------------------

struct PluginInfo {
    std::string path;
    std::string format;    // "vst2", "vst3", "unknown"
    std::string name;
    std::string vendor;
    int numParams  = 0;
    int numInputs  = 0;
    int numOutputs = 0;
    bool hasChunk  = false;
    std::string error;
};

// ---------------------------------------------------------------------------
// dummyMaster — minimal host callback used during probe-only loading.
// Only audioMasterVersion needs a real reply; everything else returns nullptr.
// NOTE: must be a plain function (not a lambda) — used inside __try/__except.
// ---------------------------------------------------------------------------

static VstIntPtr VSTCALLBACK dummyMaster(AEffect* /*effect*/,
                                          VstInt32  opcode,
                                          VstInt32  /*index*/,
                                          VstIntPtr /*value*/,
                                          void*     /*ptr*/,
                                          float     /*opt*/)
{
    if (opcode == audioMasterVersion)
        return (VstIntPtr)2400;
    return 0;
}

// ---------------------------------------------------------------------------
// VESTENTRYPROC — typedef for the VST2 plugin entry-point function pointer.
// Must match the VSTPluginMainProc in aeffectx.h but we keep a local alias
// here so the SEH helper has no dependency on external headers.
// ---------------------------------------------------------------------------

typedef AEffect* (VSTCALLBACK* VESTENTRYPROC)(audioMasterCallback);

// ---------------------------------------------------------------------------
// callVST2MainSafe — calls the plugin entry point inside a SEH frame.
//
// CRITICAL: this MUST be a separate non-inline function.  MSVC forbids mixing
// C++ objects with destructors and __try/__except blocks in the same function.
// Any std::string / PluginInfo construction must stay outside this function.
// ---------------------------------------------------------------------------

static AEffect* callVST2MainSafe(VESTENTRYPROC proc)
{
    AEffect* result = nullptr;
    __try
    {
        result = proc(dummyMaster);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        result = nullptr;
    }
    return result;
}

// ---------------------------------------------------------------------------
// dispatchSafe — calls effect->dispatcher inside SEH so a misbehaving plugin
// cannot crash the scanner process during name/vendor queries.
// Same non-inline requirement as callVST2MainSafe.
// ---------------------------------------------------------------------------

static VstIntPtr dispatchSafe(AEffect* effect, int opcode,
                               int index, VstIntPtr value,
                               void* ptr, float opt)
{
    VstIntPtr result = 0;
    __try
    {
        result = effect->dispatcher(effect, opcode, index, value, ptr, opt);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        result = 0;
    }
    return result;
}

// ---------------------------------------------------------------------------
// loadLibrarySafe — wraps LoadLibraryA in SEH so a DLL with a broken TLS or
// DllMain that throws a structured exception does not crash the scanner.
//
// MUST be a separate non-inline function with no C++ objects in scope —
// same requirement as callVST2MainSafe.
// ---------------------------------------------------------------------------

static HMODULE loadLibrarySafe(const char* path)
{
    HMODULE hMod = nullptr;
    __try
    {
        hMod = LoadLibraryA(path);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        hMod = nullptr;
    }
    return hMod;
}

// ---------------------------------------------------------------------------
// detectPluginType — LoadLibrary the DLL and probe exported symbol names.
// Returns "vst3", "vst2", or "unknown".
// The caller is responsible for FreeLibrary after further processing.
// ---------------------------------------------------------------------------

static std::string detectPluginType(HMODULE hMod)
{
    if (!hMod)
        return "unknown";

    if (GetProcAddress(hMod, "GetPluginFactory"))
        return "vst3";

    if (GetProcAddress(hMod, "VSTPluginMain") ||
        GetProcAddress(hMod, "main"))
        return "vst2";

    return "unknown";
}

// ---------------------------------------------------------------------------
// scanVST2 — probe a single VST2 DLL, query metadata via the dispatcher.
// ---------------------------------------------------------------------------

static PluginInfo scanVST2(const std::string& path)
{
    PluginInfo info;
    info.path   = path;
    info.format = "vst2";

    HMODULE hMod = loadLibrarySafe(path.c_str());
    if (!hMod)
    {
        info.error = "LoadLibrary failed (error " +
                     std::to_string(GetLastError()) + ")";
        return info;
    }

    // Try both entry-point names; VSTPluginMain is preferred (VST2.4+).
    VESTENTRYPROC proc =
        reinterpret_cast<VESTENTRYPROC>(GetProcAddress(hMod, "VSTPluginMain"));
    if (!proc)
        proc = reinterpret_cast<VESTENTRYPROC>(GetProcAddress(hMod, "main"));

    if (!proc)
    {
        FreeLibrary(hMod);
        info.error = "no entry point (VSTPluginMain / main)";
        return info;
    }

    // Call the entry point inside SEH.
    AEffect* effect = callVST2MainSafe(proc);

    if (!effect || effect->magic != kEffectMagic)
    {
        FreeLibrary(hMod);
        info.error = "invalid AEffect (bad magic or null return)";
        return info;
    }

    // --- Query effect name (effGetEffectName → char[32], allocate larger) ---
    char buf[256] = {};
    dispatchSafe(effect, effGetEffectName, 0, 0, buf, 0.0f);
    info.name = buf;

    // --- Query vendor string (effGetVendorString → char[64]) ---
    memset(buf, 0, sizeof(buf));
    dispatchSafe(effect, effGetVendorString, 0, 0, buf, 0.0f);
    info.vendor = buf;

    // --- Numeric fields directly from the AEffect struct ---
    info.numParams  = effect->numParams;
    info.numInputs  = effect->numInputs;
    info.numOutputs = effect->numOutputs;
    info.hasChunk   = (effect->flags & effFlagsProgramChunks) != 0;

    // Close the plugin cleanly before unloading.
    dispatchSafe(effect, effClose, 0, 0, nullptr, 0.0f);
    FreeLibrary(hMod);
    return info;
}

// ---------------------------------------------------------------------------
// VST3 raw COM-style fallback helpers
// All structures and the SEH-guarded probe function use only POD types so
// MSVC does not emit C4509 (SEH + C++ destructors in same function).
// ---------------------------------------------------------------------------

#ifndef VST3SDK_AVAILABLE

struct Vst3PClassInfo {
    char    cid[16];
    int32_t cardinality;
    char    category[32];
    char    name[64];
};

struct Vst3PFactoryInfo {
    char    vendor[64];
    char    url[256];
    char    email[128];
    int32_t flags;
};

struct Vst3IPluginFactoryVtbl {
    void* queryInterface;
    void* addRef;
    void* release;
    long (__stdcall* getFactoryInfo)(void* self, Vst3PFactoryInfo* info);
    int32_t (__stdcall* countClasses)(void* self);
    long (__stdcall* getClassInfo)(void* self, int32_t index, Vst3PClassInfo* info);
};

struct Vst3IPluginFactory {
    Vst3IPluginFactoryVtbl* vtbl;
};

typedef Vst3IPluginFactory* (__cdecl* GetPluginFactoryProc)();

/// POD result returned by scanVST3Raw (no std::string — SEH-safe).
struct Vst3RawResult {
    char    name[64];      // plugin name from first Audio Module Class entry
    char    vendor[64];    // vendor from factory info
    char    error[128];    // empty on success
    bool    found;         // true if an Audio Module Class was located
};

/// SEH-guarded VST3 raw probe.  MUST hold only POD locals — no C++ objects.
/// The caller (scanVST3) builds std::string results from the returned struct.
static Vst3RawResult scanVST3Raw(const char* path)
{
    Vst3RawResult result = {};

    HMODULE hMod = loadLibrarySafe(path);
    if (!hMod)
    {
        // Can't build std::to_string here — use snprintf into POD buffer.
        DWORD err = GetLastError();
        snprintf(result.error, sizeof(result.error),
                 "LoadLibrary failed (error %lu)", static_cast<unsigned long>(err));
        return result;
    }

    GetPluginFactoryProc gpf =
        reinterpret_cast<GetPluginFactoryProc>(
            GetProcAddress(hMod, "GetPluginFactory"));

    if (!gpf)
    {
        FreeLibrary(hMod);
        snprintf(result.error, sizeof(result.error),
                 "GetPluginFactory export not found");
        return result;
    }

    Vst3IPluginFactory* factory = nullptr;
    __try
    {
        factory = gpf();
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        factory = nullptr;
    }

    if (!factory || !factory->vtbl)
    {
        FreeLibrary(hMod);
        snprintf(result.error, sizeof(result.error),
                 "GetPluginFactory returned null");
        return result;
    }

    Vst3PFactoryInfo factInfo = {};
    __try
    {
        factory->vtbl->getFactoryInfo(factory, &factInfo);
    }
    __except (EXCEPTION_EXECUTE_HANDLER) { /* ignore */ }

    int32_t numClasses = 0;
    __try
    {
        numClasses = factory->vtbl->countClasses(factory);
    }
    __except (EXCEPTION_EXECUTE_HANDLER) { numClasses = 0; }

    for (int32_t i = 0; i < numClasses; ++i)
    {
        Vst3PClassInfo ci = {};
        long hr = 0;
        __try
        {
            hr = factory->vtbl->getClassInfo(factory, i, &ci);
        }
        __except (EXCEPTION_EXECUTE_HANDLER) { hr = -1; }

        if (hr != 0) continue;
        if (strncmp(ci.category, "Audio Module Class", 32) != 0) continue;

        strncpy_s(result.name,   sizeof(result.name),   ci.name,          _TRUNCATE);
        strncpy_s(result.vendor, sizeof(result.vendor), factInfo.vendor,  _TRUNCATE);
        result.found = true;
        break;
    }

    if (!result.found)
    {
        // Copy vendor even if no audio class found; caller can still report it.
        strncpy_s(result.vendor, sizeof(result.vendor), factInfo.vendor, _TRUNCATE);
        if (numClasses > 0)
            snprintf(result.error, sizeof(result.error),
                     "no Audio Module Class found in factory");
    }

    __try
    {
        factory->vtbl->release(factory);
    }
    __except (EXCEPTION_EXECUTE_HANDLER) { /* ignore */ }

    FreeLibrary(hMod);
    return result;
}

#endif // !VST3SDK_AVAILABLE

// ---------------------------------------------------------------------------
// scanVST3 — probe a VST3 bundle.
//
// When the VST3 SDK hosting library is linked in (vst3_hosting target),
// uses VST3::Hosting::Module to enumerate class infos without instantiating
// any audio processor.
//
// Without the SDK uses the raw COM fallback (scanVST3Raw above).
// ---------------------------------------------------------------------------

static PluginInfo scanVST3(const std::string& path)
{
    PluginInfo info;
    info.path   = path;
    info.format = "vst3";

#ifdef VST3SDK_AVAILABLE
    // ---- Full VST3 SDK path ----
    std::string errMsg;
    auto module = VST3::Hosting::Module::create(path, errMsg);
    if (!module)
    {
        info.error = errMsg.empty() ? "VST3 module load failed" : errMsg;
        return info;
    }

    auto factory = module->getFactory();
    for (int i = 0; i < factory.numClasses(); ++i)
    {
        auto classInfo = factory.classInfos()[i];
        if (classInfo.category() != kVstAudioEffectClass)
            continue;

        info.name   = classInfo.name();
        info.vendor = classInfo.vendor();
        // numParams / numInputs / numOutputs require instantiation —
        // omitted intentionally to keep the scanner crash-safe.
        break;
    }
#else
    // ---- Fallback: raw COM-style vtable probe (SEH in separate function) ----
    Vst3RawResult raw = scanVST3Raw(path.c_str());
    info.name   = raw.name;
    info.vendor = raw.vendor;
    if (raw.error[0] != '\0')
        info.error = raw.error;
#endif // VST3SDK_AVAILABLE

    return info;
}

// ---------------------------------------------------------------------------
// scanFile — dispatch to scanVST2 or scanVST3 based on detected type.
// For .dll files we must LoadLibrary once to detect the type, then the
// appropriate scan function loads it again.  Two loads is safer than trying
// to reuse the module handle across the detect/scan boundary.
// ---------------------------------------------------------------------------

static PluginInfo scanFile(const std::string& path)
{
    PluginInfo info;
    info.path = path;

    // .vst3 bundles are always VST3 — skip the detection load.
    {
        std::filesystem::path p(path);
        std::string ext = p.extension().string();
        // Normalise to lower case for comparison.
        for (char& c : ext) c = static_cast<char>(tolower(static_cast<unsigned char>(c)));
        if (ext == ".vst3")
            return scanVST3(path);
    }

    // For .dll: load (SEH-guarded), detect type, unload, call the appropriate scanner.
    HMODULE hMod = loadLibrarySafe(path.c_str());

    if (!hMod)
    {
        info.format = "unknown";
        info.error  = "load failed (error " + std::to_string(GetLastError()) + ")";
        return info;
    }

    std::string type = detectPluginType(hMod);
    FreeLibrary(hMod);

    if (type == "vst2")
        return scanVST2(path);
    if (type == "vst3")
        return scanVST3(path);

    info.format = "unknown";
    info.error  = "no recognised VST entry point";
    return info;
}

// ---------------------------------------------------------------------------
// toJson — serialise a PluginInfo to a single-line JSON object (NDJSON).
// No external JSON library — manual character escaping.
// ---------------------------------------------------------------------------

static std::string toJson(const PluginInfo& info)
{
    auto esc = [](const std::string& s) -> std::string {
        std::string r;
        r.reserve(s.size() + 4);
        for (unsigned char c : s)
        {
            switch (c)
            {
                case '"':  r += "\\\""; break;
                case '\\': r += "\\\\"; break;
                case '\n': r += "\\n";  break;
                case '\r': r += "\\r";  break;
                case '\t': r += "\\t";  break;
                default:
                    if (c < 0x20)
                    {
                        // Encode control characters as \uXXXX.
                        char hex[8];
                        snprintf(hex, sizeof(hex), "\\u%04x", c);
                        r += hex;
                    }
                    else
                    {
                        r += static_cast<char>(c);
                    }
                    break;
            }
        }
        return r;
    };

    std::string j;
    j.reserve(256);
    j += '{';
    j += "\"path\":\""      + esc(info.path)   + "\",";
    j += "\"format\":\""    + esc(info.format)  + "\",";
    j += "\"name\":\""      + esc(info.name)    + "\",";
    j += "\"vendor\":\""    + esc(info.vendor)  + "\",";
    j += "\"numParams\":"   + std::to_string(info.numParams)  + ',';
    j += "\"numInputs\":"   + std::to_string(info.numInputs)  + ',';
    j += "\"numOutputs\":"  + std::to_string(info.numOutputs) + ',';
    j += "\"hasChunk\":"    + (info.hasChunk ? "true" : "false") + ',';
    j += "\"error\":";
    if (info.error.empty())
        j += "null";
    else
        j += '"' + esc(info.error) + '"';
    j += '}';
    return j;
}

// ---------------------------------------------------------------------------
// enumeratePaths — recursively walk searchPaths, collect .dll and .vst3 items.
// A path ending in ".vst3" is treated as a bundle entry point even if it is a
// directory (the VST3 bundle format uses a directory named *.vst3).
// ---------------------------------------------------------------------------

static std::vector<std::string> enumeratePaths(
    const std::vector<std::string>& searchPaths)
{
    std::vector<std::string> result;

    for (const auto& root : searchPaths)
    {
        std::error_code ec;
        std::filesystem::path rootPath(root);

        if (!std::filesystem::exists(rootPath, ec))
            continue;

        // If the root itself is a .vst3 bundle directory, add it directly.
        {
            std::string ext = rootPath.extension().string();
            for (char& c : ext) c = static_cast<char>(
                tolower(static_cast<unsigned char>(c)));
            if (ext == ".vst3")
            {
                result.push_back(rootPath.string());
                continue;
            }
        }

        for (auto it = std::filesystem::recursive_directory_iterator(
                            rootPath,
                            std::filesystem::directory_options::skip_permission_denied,
                            ec);
             it != std::filesystem::recursive_directory_iterator();
             it.increment(ec))
        {
            if (ec) { ec.clear(); continue; }

            const auto& entry = *it;

            // Get extension, lower-cased.
            std::string ext = entry.path().extension().string();
            for (char& c : ext) c = static_cast<char>(
                tolower(static_cast<unsigned char>(c)));

            if (ext == ".dll")
            {
                // Only add regular files (skip symlinks to dirs, etc.)
                if (entry.is_regular_file(ec))
                    result.push_back(entry.path().string());
                ec.clear();
            }
            else if (ext == ".vst3")
            {
                // A .vst3 item may be a file (old single-file format) or a
                // directory (bundle). Add it and do not descend into it.
                result.push_back(entry.path().string());
                if (entry.is_directory(ec))
                    it.disable_recursion_pending();
                ec.clear();
            }
        }
    }

    return result;
}

// ---------------------------------------------------------------------------
// readRegistryVSTPath — attempt to read the VST2 plugin path from the Windows
// registry key HKLM\SOFTWARE\VST\VSTPluginsPath.
// Returns an empty string on failure.
// ---------------------------------------------------------------------------

static std::string readRegistryVSTPath()
{
    HKEY hKey = nullptr;
    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, "SOFTWARE\\VST",
                      0, KEY_READ, &hKey) != ERROR_SUCCESS)
        return {};

    char buf[MAX_PATH] = {};
    DWORD size = sizeof(buf);
    LONG rc = RegQueryValueExA(hKey, "VSTPluginsPath",
                               nullptr, nullptr,
                               reinterpret_cast<LPBYTE>(buf), &size);
    RegCloseKey(hKey);

    if (rc == ERROR_SUCCESS && buf[0] != '\0')
        return std::string(buf);

    return {};
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    // Build the list of search paths.
    std::vector<std::string> searchPaths;

    if (argc > 1)
    {
        // Paths supplied on the command line take precedence.
        for (int i = 1; i < argc; ++i)
            searchPaths.emplace_back(argv[i]);
    }
    else
    {
        // Default well-known Windows VST install directories.
        searchPaths.emplace_back("C:\\Program Files\\VSTPlugins");
        searchPaths.emplace_back("C:\\Program Files\\Steinberg\\VSTPlugins");
        searchPaths.emplace_back("C:\\Program Files\\Common Files\\VST3");
        searchPaths.emplace_back("C:\\Program Files\\Common Files\\Steinberg\\VST3");

        // Also check the per-machine registry entry.
        std::string regPath = readRegistryVSTPath();
        if (!regPath.empty())
            searchPaths.push_back(regPath);
    }

    // Collect all candidate files.
    std::vector<std::string> files = enumeratePaths(searchPaths);

    // Scan each file and emit one JSON line per result.
    for (const auto& filePath : files)
    {
        PluginInfo info = scanFile(filePath);
        std::cout << toJson(info) << '\n';
        std::cout.flush();  // flush per line so callers can stream the output
    }

    return 0;
}
