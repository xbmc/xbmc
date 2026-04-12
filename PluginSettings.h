#pragma once
/*
 * PluginSettings.h — Persisted user settings for the VST Host addon
 * Part of audiodsp.vsthost — Kodi Audio DSP addon
 * License: GPL-2.0-or-later
 */
#include "../dsp/DSPChain.h"
#include "../dsp/DSPProcessor.h"
#include "VSTPluginManager.h"
#include <string>
#include <vector>
#include <functional>

/// Manages persisting and loading the user's plugin chain configuration
/// and search path preferences.
///
/// All settings are stored in `{dataPath}/settings.json`.
/// The plugin discovery cache is stored in `{dataPath}/plugin_cache.json`.
///
/// Typical usage:
///   PluginSettings settings(addonDataPath);
///   settings.loadSettings();
///   settings.rescan();
///   settings.applyChainConfig(processor, "default");
class PluginSettings {
public:
    /// Construct with the path to the addon's writable data directory.
    /// No I/O is performed in the constructor; call loadSettings() explicitly.
    explicit PluginSettings(const std::string& dataPath);

    // -------------------------------------------------------------------------
    // VST search path management
    // -------------------------------------------------------------------------

    /// Append a directory to the scan search path list.
    void addSearchPath(const std::string& path);

    /// Remove the search path at \p index (0-based).  No-op if out of range.
    void removeSearchPath(int index);

    /// All configured VST plugin search directories.
    const std::vector<std::string>& getSearchPaths() const { return m_searchPaths; }

    // -------------------------------------------------------------------------
    // Scanner EXE location
    // -------------------------------------------------------------------------

    /// Override the path to vstscanner.exe.
    /// By default this is the same directory as the addon DLL.
    void setScannerPath(const std::string& path) { m_scannerPath = path; }

    /// Current path to vstscanner.exe.
    const std::string& getScannerPath() const { return m_scannerPath; }

    // -------------------------------------------------------------------------
    // Active chain config name
    // -------------------------------------------------------------------------

    void setActiveConfigName(const std::string& name) { m_activeConfigName = name; }
    const std::string& getActiveConfigName() const { return m_activeConfigName; }

    // -------------------------------------------------------------------------
    // Plugin scanning
    // -------------------------------------------------------------------------

    /// Run vstscanner.exe with the current search paths, update the in-memory
    /// plugin list, and persist the result to plugin_cache.json.
    ///
    /// @param progress  Optional callback forwarded to VSTPluginManager::scan().
    void rescan(std::function<void(int, int, const std::string&)> progress = nullptr);

    // -------------------------------------------------------------------------
    // Chain configuration
    // -------------------------------------------------------------------------

    /// Load the chain config file named \p configName from the data directory
    /// and apply it to \p processor.
    ///
    /// The config file is expected at `{dataPath}/{configName}.json`.
    ///
    /// @return true on success, false if the file cannot be read or parsed.
    bool applyChainConfig(DSPProcessor& processor, const std::string& configName) const;

    // -------------------------------------------------------------------------
    // Settings persistence
    // -------------------------------------------------------------------------

    /// Load settings from `{dataPath}/settings.json`.
    /// @return true on success; false if the file does not exist or is malformed.
    bool loadSettings();

    /// Save current settings to `{dataPath}/settings.json`.
    /// @return true on success.
    bool saveSettings() const;

    // -------------------------------------------------------------------------
    // Accessors
    // -------------------------------------------------------------------------

    const std::string& getDataPath() const { return m_dataPath; }

private:
    std::string              m_dataPath;
    std::string              m_scannerPath;
    std::string              m_activeConfigName = "default";
    std::vector<std::string> m_searchPaths;
};
