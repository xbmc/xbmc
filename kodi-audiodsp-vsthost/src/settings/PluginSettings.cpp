/*
 * PluginSettings.cpp — Persisted user settings for the VST Host addon
 * Part of audiodsp.vsthost — Kodi Audio DSP addon
 * License: GPL-2.0-or-later
 */
#include "PluginSettings.h"

#include <fstream>
#include <sstream>
#include <algorithm>

// ---------------------------------------------------------------------------
// Constructor
// ---------------------------------------------------------------------------

PluginSettings::PluginSettings(const std::string& dataPath)
    : m_dataPath(dataPath)
{
    // Default scanner location: same directory as addon data path.
    // Callers should override this via setScannerPath() if the EXE lives
    // next to the DLL rather than in the addon data directory.
    m_scannerPath = m_dataPath + "/vstscanner.exe";
}

// ---------------------------------------------------------------------------
// Search path management
// ---------------------------------------------------------------------------

void PluginSettings::addSearchPath(const std::string& path)
{
    if (path.empty())
        return;
    // Avoid duplicates
    for (const auto& p : m_searchPaths)
        if (p == path)
            return;
    m_searchPaths.push_back(path);
}

void PluginSettings::removeSearchPath(int index)
{
    if (index < 0 || index >= static_cast<int>(m_searchPaths.size()))
        return;
    m_searchPaths.erase(m_searchPaths.begin() + index);
}

// ---------------------------------------------------------------------------
// rescan()
// ---------------------------------------------------------------------------

void PluginSettings::rescan(std::function<void(int, int, const std::string&)> progress)
{
    VSTPluginManager& mgr = VSTPluginManager::instance();
    mgr.scan(m_scannerPath, m_searchPaths, progress);
    mgr.saveCache(m_dataPath + "/plugin_cache.json");
}

// ---------------------------------------------------------------------------
// applyChainConfig()
// ---------------------------------------------------------------------------

bool PluginSettings::applyChainConfig(DSPProcessor& processor,
                                       const std::string& configName) const
{
    std::string filePath = m_dataPath + "/" + configName + ".json";
    processor.setConfigPath(m_dataPath);

    // loadConfig() reads chain.json from the processor's config path.
    // We temporarily set the config path so the right file is loaded.
    // DSPProcessor expects the config file to be named "chain.json" in the
    // config directory.  If the caller supplies a non-default config name we
    // load the file ourselves and deserialize it into the chain directly.
    if (configName == "default") {
        return processor.loadConfig();
    }

    // Non-default config: read the file and deserialize into the chain.
    std::ifstream f(filePath);
    if (!f.is_open())
        return false;

    std::ostringstream ss;
    ss << f.rdbuf();
    std::string json = ss.str();

    return processor.getChain().deserializeFromJson(json);
}

// ---------------------------------------------------------------------------
// Hand-rolled JSON helpers
// ---------------------------------------------------------------------------

namespace {

/// Escape a string for embedding in a JSON value.
std::string jsonEscape(const std::string& s)
{
    std::string out;
    out.reserve(s.size() + 4);
    for (unsigned char c : s) {
        if      (c == '"')  out += "\\\"";
        else if (c == '\\') out += "\\\\";
        else if (c == '\n') out += "\\n";
        else if (c == '\r') out += "\\r";
        else if (c == '\t') out += "\\t";
        else if (c < 0x20) {
            char buf[8];
            snprintf(buf, sizeof(buf), "\\u%04x", static_cast<unsigned>(c));
            out += buf;
        } else {
            out += static_cast<char>(c);
        }
    }
    return out;
}

/// Extract a single top-level string field from a simple JSON object.
std::string jsonExtractString(const std::string& json, const std::string& key)
{
    std::string search = "\"" + key + "\":\"";
    size_t start = json.find(search);
    if (start == std::string::npos)
        return {};
    start += search.size();
    size_t end = json.find('"', start);
    // Skip escaped quotes
    while (end != std::string::npos && end > 0 && json[end - 1] == '\\')
        end = json.find('"', end + 1);
    if (end == std::string::npos)
        return {};

    std::string raw = json.substr(start, end - start);
    // Unescape common sequences
    std::string result;
    result.reserve(raw.size());
    for (size_t i = 0; i < raw.size(); ++i) {
        if (raw[i] == '\\' && i + 1 < raw.size()) {
            char next = raw[i + 1];
            if      (next == '"')  { result += '"';  ++i; }
            else if (next == '\\') { result += '\\'; ++i; }
            else if (next == 'n')  { result += '\n'; ++i; }
            else if (next == 'r')  { result += '\r'; ++i; }
            else if (next == 't')  { result += '\t'; ++i; }
            else                   { result += raw[i]; }
        } else {
            result += raw[i];
        }
    }
    return result;
}

/// Extract all string elements from a JSON array field.
/// Handles simple quoted strings; does not parse nested objects.
std::vector<std::string> jsonExtractStringArray(const std::string& json,
                                                  const std::string& key)
{
    std::vector<std::string> result;
    std::string search = "\"" + key + "\"";
    size_t pos = json.find(search);
    if (pos == std::string::npos)
        return result;

    // Find the opening bracket of the array.
    size_t bracket = json.find('[', pos + search.size());
    if (bracket == std::string::npos)
        return result;
    size_t end = json.find(']', bracket);
    if (end == std::string::npos)
        return result;

    std::string array = json.substr(bracket + 1, end - bracket - 1);

    // Extract each quoted string element.
    size_t i = 0;
    while (i < array.size()) {
        size_t q1 = array.find('"', i);
        if (q1 == std::string::npos) break;
        size_t q2 = array.find('"', q1 + 1);
        while (q2 != std::string::npos && q2 > 0 && array[q2 - 1] == '\\')
            q2 = array.find('"', q2 + 1);
        if (q2 == std::string::npos) break;

        std::string raw = array.substr(q1 + 1, q2 - q1 - 1);
        // Unescape
        std::string unescaped;
        unescaped.reserve(raw.size());
        for (size_t j = 0; j < raw.size(); ++j) {
            if (raw[j] == '\\' && j + 1 < raw.size()) {
                char next = raw[j + 1];
                if      (next == '"')  { unescaped += '"';  ++j; }
                else if (next == '\\') { unescaped += '\\'; ++j; }
                else if (next == 'n')  { unescaped += '\n'; ++j; }
                else if (next == 'r')  { unescaped += '\r'; ++j; }
                else if (next == 't')  { unescaped += '\t'; ++j; }
                else                   { unescaped += raw[j]; }
            } else {
                unescaped += raw[j];
            }
        }
        result.push_back(std::move(unescaped));
        i = q2 + 1;
    }
    return result;
}

} // anonymous namespace

// ---------------------------------------------------------------------------
// loadSettings()
// ---------------------------------------------------------------------------
//
// Expected file format (settings.json):
//
//   {
//     "scannerPath": "C:\\path\\to\\vstscanner.exe",
//     "searchPaths": ["C:\\VSTPlugins\\", "C:\\VST3\\"],
//     "activeConfig": "default"
//   }

bool PluginSettings::loadSettings()
{
    std::string filePath = m_dataPath + "/settings.json";
    std::ifstream f(filePath);
    if (!f.is_open())
        return false;

    std::ostringstream ss;
    ss << f.rdbuf();
    std::string json = ss.str();

    std::string scannerPath = jsonExtractString(json, "scannerPath");
    if (!scannerPath.empty())
        m_scannerPath = scannerPath;

    std::string activeConfig = jsonExtractString(json, "activeConfig");
    if (!activeConfig.empty())
        m_activeConfigName = activeConfig;

    m_searchPaths = jsonExtractStringArray(json, "searchPaths");

    return true;
}

// ---------------------------------------------------------------------------
// saveSettings()
// ---------------------------------------------------------------------------

bool PluginSettings::saveSettings() const
{
    std::string filePath = m_dataPath + "/settings.json";
    std::ofstream f(filePath);
    if (!f.is_open())
        return false;

    f << "{\n";
    f << "  \"scannerPath\": \"" << jsonEscape(m_scannerPath) << "\",\n";
    f << "  \"searchPaths\": [";
    for (size_t i = 0; i < m_searchPaths.size(); ++i) {
        if (i > 0) f << ", ";
        f << "\"" << jsonEscape(m_searchPaths[i]) << "\"";
    }
    f << "],\n";
    f << "  \"activeConfig\": \"" << jsonEscape(m_activeConfigName) << "\"\n";
    f << "}\n";

    return f.good();
}
