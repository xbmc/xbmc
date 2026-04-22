/*
 * VSTPluginManager.cpp — Out-of-process VST plugin discovery manager
 * Part of audiodsp.vsthost — Kodi Audio DSP addon
 * License: GPL-2.0-or-later
 */
#include "VSTPluginManager.h"
#include "../util/JsonUtil.h"

#include <windows.h>
#include <fstream>
#include <sstream>
#include <stdexcept>

// ---------------------------------------------------------------------------
// Singleton
// ---------------------------------------------------------------------------

VSTPluginManager& VSTPluginManager::instance()
{
    static VSTPluginManager s_instance;
    return s_instance;
}

// ---------------------------------------------------------------------------
// Internal JSON field helpers (hand-rolled; no external library dependency)
// ---------------------------------------------------------------------------

static int extractIntField(const std::string& json, const std::string& key)
{
    std::string search = "\"" + key + "\":";
    size_t pos = json.find(search);
    if (pos == std::string::npos)
        return 0;
    pos += search.size();
    // Skip leading whitespace
    while (pos < json.size() && (json[pos] == ' ' || json[pos] == '\t'))
        ++pos;
    try {
        return std::stoi(json.substr(pos));
    } catch (...) {
        return 0;
    }
}

// ---------------------------------------------------------------------------
// parseJsonLine — convert a single NDJSON line to PluginInfo
// ---------------------------------------------------------------------------

/*static*/ PluginInfo VSTPluginManager::parseJsonLine(const std::string& line)
{
    PluginInfo info;
    info.path       = JsonUtil::extractString(line, "path");
    info.format     = JsonUtil::extractString(line, "format");
    info.name       = JsonUtil::extractString(line, "name");
    info.vendor     = JsonUtil::extractString(line, "vendor");
    info.numParams  = extractIntField(line, "numParams");
    info.numInputs  = extractIntField(line, "numInputs");
    info.numOutputs = extractIntField(line, "numOutputs");
    info.hasChunk   = JsonUtil::extractBool(line, "hasChunk");

    // The "error" field may be a quoted string or the literal null.
    size_t errSearch = line.find("\"error\":");
    if (errSearch != std::string::npos) {
        size_t afterColon = errSearch + 8; // length of "\"error\":"
        while (afterColon < line.size() && (line[afterColon] == ' ' || line[afterColon] == '\t'))
            ++afterColon;
        if (afterColon < line.size() && line[afterColon] == '"') {
            // Quoted string — extract it
            info.error = JsonUtil::extractString(line, "error");
        }
        // else: literal null → info.error stays empty (success)
    }

    return info;
}

// ---------------------------------------------------------------------------
// pluginInfoToJson — serialize one PluginInfo to a compact JSON object
// ---------------------------------------------------------------------------

/*static*/ std::string VSTPluginManager::pluginInfoToJson(const PluginInfo& info)
{
    std::string result;
    result += "{";
    result += "\"path\":\""       + JsonUtil::escape(info.path)   + "\",";
    result += "\"format\":\""     + JsonUtil::escape(info.format) + "\",";
    result += "\"name\":\""       + JsonUtil::escape(info.name)   + "\",";
    result += "\"vendor\":\""     + JsonUtil::escape(info.vendor) + "\",";
    result += "\"numParams\":"    + std::to_string(info.numParams)  + ",";
    result += "\"numInputs\":"    + std::to_string(info.numInputs)  + ",";
    result += "\"numOutputs\":"   + std::to_string(info.numOutputs) + ",";
    result += "\"hasChunk\":"     + std::string(info.hasChunk ? "true" : "false") + ",";
    if (info.error.empty())
        result += "\"error\":null";
    else
        result += "\"error\":\"" + JsonUtil::escape(info.error) + "\"";
    result += "}";
    return result;
}

// ---------------------------------------------------------------------------
// scan() — launch vstscanner.exe, read NDJSON from stdout pipe
// ---------------------------------------------------------------------------

void VSTPluginManager::scan(
    const std::string& scannerExePath,
    const std::vector<std::string>& searchPaths,
    std::function<void(int, int, const std::string&)> progressCallback)
{
    m_plugins.clear();
    m_failed.clear();

    // Build command line: "scanner.exe" "path1" "path2" ...
    std::string cmdLine = "\"" + scannerExePath + "\"";
    for (const auto& p : searchPaths)
        cmdLine += " \"" + p + "\"";

    // Convert UTF-8 command line to wide-char for CreateProcessW.
    int wlen = MultiByteToWideChar(CP_UTF8, 0, cmdLine.c_str(), -1, nullptr, 0);
    std::vector<wchar_t> wCmd(static_cast<size_t>(wlen));
    MultiByteToWideChar(CP_UTF8, 0, cmdLine.c_str(), -1, wCmd.data(), wlen);

    // Create an anonymous pipe so we can read the child's stdout.
    SECURITY_ATTRIBUTES sa{};
    sa.nLength              = sizeof(SECURITY_ATTRIBUTES);
    sa.lpSecurityDescriptor = nullptr;
    sa.bInheritHandle       = TRUE;   // child inherits the write end

    HANDLE hReadPipe  = INVALID_HANDLE_VALUE;
    HANDLE hWritePipe = INVALID_HANDLE_VALUE;
    if (!CreatePipe(&hReadPipe, &hWritePipe, &sa, 0))
        return;

    // Make the read end non-inheritable so the child does not get it.
    SetHandleInformation(hReadPipe, HANDLE_FLAG_INHERIT, 0);

    STARTUPINFOW si{};
    si.cb         = sizeof(STARTUPINFOW);
    si.dwFlags    = STARTF_USESTDHANDLES;
    si.hStdOutput = hWritePipe;
    si.hStdError  = GetStdHandle(STD_ERROR_HANDLE);
    si.hStdInput  = GetStdHandle(STD_INPUT_HANDLE);

    PROCESS_INFORMATION pi{};
    BOOL created = CreateProcessW(
        nullptr,        // lpApplicationName (derive from command line)
        wCmd.data(),    // lpCommandLine
        nullptr,        // lpProcessAttributes
        nullptr,        // lpThreadAttributes
        TRUE,           // bInheritHandles — pipe handles are inherited
        CREATE_NO_WINDOW,
        nullptr,        // lpEnvironment (inherit parent)
        nullptr,        // lpCurrentDirectory (inherit parent)
        &si,
        &pi);

    // Parent no longer needs the write end — close it so ReadFile will see EOF.
    CloseHandle(hWritePipe);

    if (!created) {
        CloseHandle(hReadPipe);
        return;
    }

    // Read NDJSON lines from the child's stdout.
    std::string lineBuffer;
    char chunk[4096];
    DWORD bytesRead  = 0;
    int   lineCount  = 0;

    while (ReadFile(hReadPipe, chunk, sizeof(chunk) - 1, &bytesRead, nullptr)
           && bytesRead > 0)
    {
        chunk[bytesRead] = '\0';
        lineBuffer += chunk;

        size_t pos;
        while ((pos = lineBuffer.find('\n')) != std::string::npos) {
            std::string line = lineBuffer.substr(0, pos);
            // Strip trailing CR (Windows line endings)
            if (!line.empty() && line.back() == '\r')
                line.pop_back();
            lineBuffer = lineBuffer.substr(pos + 1);

            if (!line.empty()) {
                PluginInfo info = parseJsonLine(line);
                ++lineCount;
                if (info.error.empty())
                    m_plugins.push_back(std::move(info));
                else
                    m_failed.push_back(std::move(info));

                if (progressCallback) {
                    const std::string& displayName = m_plugins.empty()
                        ? (m_failed.empty() ? "" : m_failed.back().name)
                        : m_plugins.back().name;
                    progressCallback(lineCount, -1, displayName);
                }
            }
        }
    }

    // Process any remaining partial line (scanner didn't end with newline)
    if (!lineBuffer.empty()) {
        PluginInfo info = parseJsonLine(lineBuffer);
        ++lineCount;
        if (info.error.empty())
            m_plugins.push_back(std::move(info));
        else
            m_failed.push_back(std::move(info));

        if (progressCallback) {
            const std::string& displayName = m_plugins.empty()
                ? (m_failed.empty() ? "" : m_failed.back().name)
                : m_plugins.back().name;
            progressCallback(lineCount, -1, displayName);
        }
    }

    WaitForSingleObject(pi.hProcess, INFINITE);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    CloseHandle(hReadPipe);
}

// ---------------------------------------------------------------------------
// saveCache() — write a JSON array of all plugin results
// ---------------------------------------------------------------------------

bool VSTPluginManager::saveCache(const std::string& cacheFilePath) const
{
    std::ofstream f(cacheFilePath);
    if (!f.is_open())
        return false;

    f << "{\n";
    f << "  \"plugins\": [\n";

    // Write successfully scanned plugins, then failed ones.
    bool first = true;
    auto writeEntry = [&](const PluginInfo& info) {
        if (!first) f << ",\n";
        f << "    " << pluginInfoToJson(info);
        first = false;
    };

    for (const auto& info : m_plugins)
        writeEntry(info);
    for (const auto& info : m_failed)
        writeEntry(info);

    f << "\n  ]\n";
    f << "}\n";
    return f.good();
}

// ---------------------------------------------------------------------------
// loadCache() — read a JSON file produced by saveCache()
// ---------------------------------------------------------------------------

bool VSTPluginManager::loadCache(const std::string& cacheFilePath)
{
    std::ifstream f(cacheFilePath);
    if (!f.is_open())
        return false;

    // Read entire file into a string.
    std::ostringstream ss;
    ss << f.rdbuf();
    std::string content = ss.str();

    m_plugins.clear();
    m_failed.clear();

    // Locate the "plugins" array.
    size_t arrStart = content.find("\"plugins\"");
    if (arrStart == std::string::npos)
        return false;
    arrStart = content.find('[', arrStart);
    if (arrStart == std::string::npos)
        return false;
    size_t arrEnd = content.find(']', arrStart);
    if (arrEnd == std::string::npos)
        return false;

    std::string array = content.substr(arrStart + 1, arrEnd - arrStart - 1);

    // Walk through brace-delimited objects in the array.
    size_t pos = 0;
    while (pos < array.size()) {
        size_t objStart = array.find('{', pos);
        if (objStart == std::string::npos)
            break;

        // Find matching closing brace, handling nested braces if any.
        int depth = 0;
        size_t objEnd = std::string::npos;
        for (size_t i = objStart; i < array.size(); ++i) {
            if (array[i] == '{') ++depth;
            else if (array[i] == '}') {
                --depth;
                if (depth == 0) { objEnd = i; break; }
            }
        }
        if (objEnd == std::string::npos)
            break;

        std::string obj = array.substr(objStart, objEnd - objStart + 1);
        PluginInfo info = parseJsonLine(obj);

        if (info.error.empty())
            m_plugins.push_back(std::move(info));
        else
            m_failed.push_back(std::move(info));

        pos = objEnd + 1;
    }

    return true;
}
