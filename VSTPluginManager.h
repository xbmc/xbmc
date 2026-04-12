#pragma once
/*
 * VSTPluginManager.h — Out-of-process VST plugin discovery manager
 * Part of audiodsp.vsthost — Kodi Audio DSP addon
 * License: GPL-2.0-or-later
 */
#include "../plugin/IVSTPlugin.h"
#include <string>
#include <vector>
#include <functional>

/// Metadata describing a single scanned VST plugin.
/// Matches the NDJSON output format of vstscanner.exe (see task6_scanner.md).
struct PluginInfo {
    std::string path;       ///< Absolute path to the plugin file
    std::string format;     ///< "vst2", "vst3", or "unknown"
    std::string name;       ///< Plugin display name
    std::string vendor;     ///< Vendor/manufacturer string
    int         numParams  = 0;
    int         numInputs  = 0;
    int         numOutputs = 0;
    bool        hasChunk   = false;
    std::string error;      ///< Empty string on success; human-readable message on failure
};

/// Singleton that manages VST plugin discovery via the out-of-process
/// vstscanner.exe.  Results are cached in-memory and optionally persisted
/// to a JSON file on disk.
///
/// Thread safety: scan() / loadCache() / saveCache() must be called from a
/// single settings thread.  getPlugins() / getFailedPlugins() are read-only
/// after the scan completes and are safe to read from any thread.
class VSTPluginManager {
public:
    /// Return the singleton instance.
    static VSTPluginManager& instance();

    // -------------------------------------------------------------------------
    // Scan
    // -------------------------------------------------------------------------

    /// Launch vstscanner.exe out-of-process and collect results.
    ///
    /// @param scannerExePath   Absolute path to vstscanner.exe.
    /// @param searchPaths      Directories to scan.  Pass an empty vector to
    ///                         let the scanner use its built-in default paths.
    /// @param progressCallback Optional callback invoked for each scanned
    ///                         plugin.  Arguments: (linesProcessed, -1,
    ///                         currentPluginName).  May be nullptr.
    void scan(const std::string& scannerExePath,
              const std::vector<std::string>& searchPaths,
              std::function<void(int, int, const std::string&)> progressCallback = nullptr);

    // -------------------------------------------------------------------------
    // Result access (valid after scan() or loadCache())
    // -------------------------------------------------------------------------

    /// All plugins whose scan succeeded (error field was empty / null).
    const std::vector<PluginInfo>& getPlugins()       const { return m_plugins; }

    /// All plugins whose scan produced an error.
    const std::vector<PluginInfo>& getFailedPlugins() const { return m_failed;  }

    // -------------------------------------------------------------------------
    // Cache persistence
    // -------------------------------------------------------------------------

    /// Load scan results from a JSON cache file written by saveCache().
    /// Replaces any existing in-memory results.
    /// @return true on success, false if the file cannot be read or parsed.
    bool loadCache(const std::string& cacheFilePath);

    /// Persist current scan results to a JSON cache file.
    /// @return true on success.
    bool saveCache(const std::string& cacheFilePath) const;

    // -------------------------------------------------------------------------
    // Housekeeping
    // -------------------------------------------------------------------------

    /// Discard all in-memory scan results.
    void clear() { m_plugins.clear(); m_failed.clear(); }

private:
    VSTPluginManager() = default;

    std::vector<PluginInfo> m_plugins;   ///< Successfully scanned plugins
    std::vector<PluginInfo> m_failed;    ///< Plugins that produced errors

    /// Parse a single NDJSON line (one JSON object) into a PluginInfo.
    static PluginInfo parseJsonLine(const std::string& line);

    /// Serialize a single PluginInfo to a compact JSON object string.
    static std::string pluginInfoToJson(const PluginInfo& info);
};
