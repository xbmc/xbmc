/*
 * DSPChain.cpp — Ordered chain of VST plugins with ping-pong buffer processing
 * Part of audiodsp.vsthost — Kodi Audio DSP addon
 * License: GPL-2.0-or-later
 */
#include "DSPChain.h"
#include "../vst2/VSTPlugin2.h"
#include "../util/JsonUtil.h"

#include <algorithm>
#include <cstring>
#include <stdexcept>
#include <sstream>
#include <cstdio>

// =============================================================================
//  Destructor
// =============================================================================

DSPChain::~DSPChain()
{
    clear();
}

// =============================================================================
//  Chain management
// =============================================================================

bool DSPChain::addPlugin(const std::string& path, IVSTPlugin::PluginFormat format)
{
    if (format != IVSTPlugin::PluginFormat::VST2)
    {
        const char* formatName = (format == IVSTPlugin::PluginFormat::VST3) ? "vst3" : "unknown";
        std::fprintf(stderr,
            "[DSPChain] addPlugin: unsupported format '%s' for '%s' (VST2 only)\n",
            formatName, path.c_str());
        return false;
    }

    ChainPlugin slot;
    slot.path   = path;
    slot.format = format;
    slot.plugin = std::make_unique<VSTPlugin2>(path);

    if (m_initialized)
    {
        if (!slot.plugin->load(m_sampleRate, m_blockSize, m_numChannels))
        {
            // Log but do not abort — caller decides whether to retry
            std::fprintf(stderr,
                "[DSPChain] addPlugin: failed to load '%s'\n", path.c_str());
            return false;
        }
    }

    m_plugins.push_back(std::move(slot));
    return true;
}

bool DSPChain::removePlugin(int index)
{
    if (index < 0 || index >= static_cast<int>(m_plugins.size()))
        return false;

    m_plugins[index].plugin->unload();
    m_plugins.erase(m_plugins.begin() + index);
    return true;
}

bool DSPChain::movePlugin(int from, int to)
{
    const int n = static_cast<int>(m_plugins.size());
    if (from < 0 || from >= n || to < 0 || to >= n || from == to)
        return false;

    if (from < to)
    {
        std::rotate(m_plugins.begin() + from,
                    m_plugins.begin() + from + 1,
                    m_plugins.begin() + to + 1);
    }
    else
    {
        std::rotate(m_plugins.begin() + to,
                    m_plugins.begin() + from,
                    m_plugins.begin() + from + 1);
    }
    return true;
}

void DSPChain::clear()
{
    for (auto& slot : m_plugins)
        slot.plugin->unload();
    m_plugins.clear();
}

// =============================================================================
//  Initialization
// =============================================================================

bool DSPChain::initialize(double sampleRate, int maxBlockSize, int numChannels)
{
    m_sampleRate  = sampleRate;
    m_blockSize   = maxBlockSize > 0 ? maxBlockSize : 1024;
    m_numChannels = numChannels > 0  ? numChannels  : 2;

    allocatePingPong();

    bool allOk = true;
    for (auto& slot : m_plugins)
    {
        if (!slot.plugin->load(m_sampleRate, m_blockSize, m_numChannels))
        {
            std::fprintf(stderr,
                "[DSPChain] initialize: failed to load plugin '%s'\n",
                slot.path.c_str());
            allOk = false;
            // Continue — remaining plugins still get a chance to load.
        }
    }

    m_initialized = true;
    return allOk;
}

void DSPChain::shutdown()
{
    for (auto& slot : m_plugins)
        slot.plugin->unload();

    m_initialized = false;
}

// =============================================================================
//  Ping-pong buffer allocation
// =============================================================================

void DSPChain::allocatePingPong()
{
    m_bufA.assign(m_numChannels, std::vector<float>(m_blockSize, 0.0f));
    m_bufB.assign(m_numChannels, std::vector<float>(m_blockSize, 0.0f));

    m_ptrA.resize(m_numChannels);
    m_ptrB.resize(m_numChannels);

    for (int ch = 0; ch < m_numChannels; ++ch)
    {
        m_ptrA[ch] = m_bufA[ch].data();
        m_ptrB[ch] = m_bufB[ch].data();
    }
}

// =============================================================================
//  Audio processing
// =============================================================================

// SEH filter: runs during the stack walk, before any unwind destructors.
// Logs the offending plugin path and SEH exception code, then signals that
// the exception should propagate to the next enclosing __try (Layer 2 in
// CActiveAEDSP::MasterProcess).
//
// Must be a free function — MSVC C4509 prohibits __try/__except in any scope
// that contains C++ objects with non-trivial destructors.
static int logSlotCrashAndEscalate(EXCEPTION_POINTERS* ep, const char* pluginPath)
{
    const unsigned code = (ep && ep->ExceptionRecord)
        ? static_cast<unsigned>(ep->ExceptionRecord->ExceptionCode)
        : 0u;
    std::fprintf(stderr,
        "[DSPChain] plugin crashed (SEH 0x%08X) — escalating to disable all DSP: %s\n",
        code,
        pluginPath ? pluginPath : "<unknown>");
    return EXCEPTION_CONTINUE_SEARCH;  // propagate → caught by Layer 2
}

// Wrapper that applies the per-slot SEH filter and then re-raises.
// The __except body is never entered; the filter always returns CONTINUE_SEARCH.
// pluginPath must be captured from host-heap before the __try (see process()).
//
// Must be a free function — same C4509 constraint as logSlotCrashAndEscalate.
static int callSlotProcessSafe(IVSTPlugin* p, float** in, float** out,
                                int samples, const char* pluginPath)
{
    if (!p)
        return 0;  // programming error — no plugin to call
    int result = 0;
    __try {
        result = p->process(in, out, samples);
    } __except (logSlotCrashAndEscalate(GetExceptionInformation(), pluginPath)) {
        // Never reached — filter always returns EXCEPTION_CONTINUE_SEARCH.
    }
    return result;
}

int DSPChain::process(float** in, float** out, int samples)
{
    // Clamp to the allocated buffer size to prevent heap overflow.
    const int safeSamples = (samples > m_blockSize) ? m_blockSize : samples;

    if (m_plugins.empty())
    {
        // Passthrough — no plugins in chain.
        for (int ch = 0; ch < m_numChannels; ++ch)
            std::memcpy(out[ch], in[ch], static_cast<size_t>(safeSamples) * sizeof(float));
        return samples;
    }

    // Copy Kodi's input buffers into the A side of the ping-pong pair.
    for (int ch = 0; ch < m_numChannels; ++ch)
        std::memcpy(m_ptrA[ch], in[ch], static_cast<size_t>(safeSamples) * sizeof(float));

    // current = "input side"; next = "output side".
    // After each plugin we swap so that the previous output becomes the next input.
    std::vector<float*>* current = &m_ptrA;
    std::vector<float*>* next    = &m_ptrB;

    for (auto& slot : m_plugins)
    {
        // Capture the path as a plain C string before the __try.
        // slot.path lives on the host heap (unaffected by a plugin-side crash),
        // so c_str() is safe to capture here and pass into callSlotProcessSafe.
        const char* pluginPath = slot.path.c_str();
        const int processed = callSlotProcessSafe(
            slot.plugin.get(), current->data(), next->data(), safeSamples, pluginPath);
        if (processed == 0)
        {
            // Plugin produced no output (unloaded/bypass failure) — copy input to output.
            for (int ch = 0; ch < m_numChannels; ++ch)
                std::memcpy((*next)[ch], (*current)[ch], static_cast<size_t>(safeSamples) * sizeof(float));
        }
        std::swap(current, next);
    }

    // current now points to the buffer holding the final output.
    for (int ch = 0; ch < m_numChannels; ++ch)
        std::memcpy(out[ch], (*current)[ch], static_cast<size_t>(safeSamples) * sizeof(float));

    return samples;
}

// =============================================================================
//  Parameter access
// =============================================================================

void DSPChain::setPluginParameter(int pluginIndex, int paramIndex, float value)
{
    if (pluginIndex < 0 || pluginIndex >= static_cast<int>(m_plugins.size()))
        return;
    m_plugins[pluginIndex].plugin->setParameter(paramIndex, value);
}

float DSPChain::getPluginParameter(int pluginIndex, int paramIndex) const
{
    if (pluginIndex < 0 || pluginIndex >= static_cast<int>(m_plugins.size()))
        return 0.0f;
    return m_plugins[pluginIndex].plugin->getParameter(paramIndex);
}

int DSPChain::getPluginParameterCount(int pluginIndex) const
{
    if (pluginIndex < 0 || pluginIndex >= static_cast<int>(m_plugins.size()))
        return 0;
    return m_plugins[pluginIndex].plugin->getParameterCount();
}

std::string DSPChain::getPluginParameterName(int pluginIndex, int paramIndex) const
{
    if (pluginIndex < 0 || pluginIndex >= static_cast<int>(m_plugins.size()))
        return {};
    return m_plugins[pluginIndex].plugin->getParameterName(paramIndex);
}

// =============================================================================
//  Queries
// =============================================================================

int DSPChain::getTotalLatencySamples() const
{
    int total = 0;
    for (const auto& slot : m_plugins)
    {
        if (!slot.plugin->isBypassed())
            total += slot.plugin->getLatencySamples();
    }
    return total;
}

IVSTPlugin* DSPChain::getPlugin(int index)
{
    if (index < 0 || index >= static_cast<int>(m_plugins.size()))
        return nullptr;
    return m_plugins[index].plugin.get();
}

// =============================================================================
//  Base64 helpers
// =============================================================================

static const char kBase64Chars[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

std::string DSPChain::base64Encode(const std::vector<uint8_t>& data)
{
    std::string out;
    if (data.empty())
        return out;

    out.reserve(((data.size() + 2) / 3) * 4);

    size_t i = 0;
    const size_t sz = data.size();

    while (i + 2 < sz)
    {
        uint32_t triple = (static_cast<uint32_t>(data[i])   << 16)
                        | (static_cast<uint32_t>(data[i+1]) <<  8)
                        |  static_cast<uint32_t>(data[i+2]);
        out += kBase64Chars[(triple >> 18) & 0x3F];
        out += kBase64Chars[(triple >> 12) & 0x3F];
        out += kBase64Chars[(triple >>  6) & 0x3F];
        out += kBase64Chars[(triple >>  0) & 0x3F];
        i += 3;
    }

    if (i + 1 == sz)
    {
        uint32_t triple = static_cast<uint32_t>(data[i]) << 16;
        out += kBase64Chars[(triple >> 18) & 0x3F];
        out += kBase64Chars[(triple >> 12) & 0x3F];
        out += '=';
        out += '=';
    }
    else if (i + 2 == sz)
    {
        uint32_t triple = (static_cast<uint32_t>(data[i])   << 16)
                        | (static_cast<uint32_t>(data[i+1]) <<  8);
        out += kBase64Chars[(triple >> 18) & 0x3F];
        out += kBase64Chars[(triple >> 12) & 0x3F];
        out += kBase64Chars[(triple >>  6) & 0x3F];
        out += '=';
    }

    return out;
}

static int base64CharToValue(char c)
{
    if (c >= 'A' && c <= 'Z') return c - 'A';
    if (c >= 'a' && c <= 'z') return c - 'a' + 26;
    if (c >= '0' && c <= '9') return c - '0' + 52;
    if (c == '+')              return 62;
    if (c == '/')              return 63;
    return -1;  // padding or invalid
}

std::vector<uint8_t> DSPChain::base64Decode(const std::string& str)
{
    std::vector<uint8_t> out;
    if (str.empty())
        return out;

    // Filter out any whitespace / newlines that may appear in wrapped base64.
    std::string clean;
    clean.reserve(str.size());
    for (char c : str)
        if (c != '\n' && c != '\r' && c != ' ' && c != '\t')
            clean += c;

    const size_t len = clean.size();
    if (len % 4 != 0)
        return out;  // malformed

    out.reserve((len / 4) * 3);

    for (size_t i = 0; i < len; i += 4)
    {
        int v0 = base64CharToValue(clean[i]);
        int v1 = base64CharToValue(clean[i+1]);
        int v2 = base64CharToValue(clean[i+2]);
        int v3 = base64CharToValue(clean[i+3]);

        if (v0 < 0 || v1 < 0)
            break;  // corrupt data

        uint32_t triple = (static_cast<uint32_t>(v0) << 18)
                        | (static_cast<uint32_t>(v1) << 12);

        out.push_back(static_cast<uint8_t>((triple >> 16) & 0xFF));

        if (clean[i+2] != '=')
        {
            triple |= static_cast<uint32_t>(v2) << 6;
            out.push_back(static_cast<uint8_t>((triple >> 8) & 0xFF));
        }
        if (clean[i+3] != '=')
        {
            triple |= static_cast<uint32_t>(v3);
            out.push_back(static_cast<uint8_t>(triple & 0xFF));
        }
    }

    return out;
}

// =============================================================================
//  serializeToJson
// =============================================================================

std::string DSPChain::serializeToJson() const
{
    std::ostringstream ss;
    ss << "{\n";
    ss << "  \"version\": 1,\n";
    ss << "  \"sampleRate\": " << m_sampleRate << ",\n";
    ss << "  \"numChannels\": " << m_numChannels << ",\n";
    ss << "  \"plugins\": [\n";

    for (size_t i = 0; i < m_plugins.size(); ++i)
    {
        const ChainPlugin& slot = m_plugins[i];
        const bool isLast = (i == m_plugins.size() - 1);

        const char* fmt = "vst2";

        // Save plugin state (best-effort — empty on unloaded plugins).
        std::vector<uint8_t> stateBytes;
        if (slot.plugin->isLoaded())
            stateBytes = slot.plugin->saveState();
        const std::string stateB64 = base64Encode(stateBytes);

        ss << "    {\n";
        ss << "      \"path\": \""     << JsonUtil::escape(slot.path) << "\",\n";
        ss << "      \"format\": \""   << fmt                   << "\",\n";
        ss << "      \"bypassed\": "   << (slot.plugin->isBypassed() ? "true" : "false") << ",\n";
        ss << "      \"state\": \""    << stateB64              << "\"\n";
        ss << "    }";
        if (!isLast) ss << ",";
        ss << "\n";
    }

    ss << "  ]\n";
    ss << "}\n";
    return ss.str();
}

// =============================================================================
//  deserializeFromJson  (simple hand-rolled parser for the controlled format)
// =============================================================================

bool DSPChain::deserializeFromJson(const std::string& json)
{
    // Clear existing chain before loading.
    clear();

    // Locate the "plugins" array.
    const std::string pluginsKey = "\"plugins\"";
    size_t pos = json.find(pluginsKey);
    if (pos == std::string::npos)
        return false;

    pos += pluginsKey.size();
    pos = json.find('[', pos);
    if (pos == std::string::npos)
        return false;
    ++pos;  // skip '['

    // Iterate over plugin objects {…}
    while (true)
    {
        // Skip whitespace and commas between objects.
        while (pos < json.size() &&
               (json[pos] == ' ' || json[pos] == '\t' ||
                json[pos] == '\n' || json[pos] == '\r' ||
                json[pos] == ','))
            ++pos;

        if (pos >= json.size())
            break;

        if (json[pos] == ']')
            break;  // end of array

        if (json[pos] != '{')
            break;  // unexpected

        // Find closing brace of this object.
        size_t objStart = pos;
        int    depth    = 0;
        size_t objEnd   = pos;
        for (size_t k = pos; k < json.size(); ++k)
        {
            if      (json[k] == '{') ++depth;
            else if (json[k] == '}') { --depth; if (depth == 0) { objEnd = k; break; } }
        }

        const std::string obj = json.substr(objStart, objEnd - objStart + 1);

        // Extract fields from this object.
        std::string pathStr, formatStr, stateStr;
        bool        bypassed = false;
        size_t      dummy    = 0;

        if (!JsonUtil::extractString(obj, "path",   0, pathStr,   dummy)) { pos = objEnd + 1; continue; }
        if (!JsonUtil::extractString(obj, "format", 0, formatStr, dummy)) { pos = objEnd + 1; continue; }
        JsonUtil::extractBool  (obj, "bypassed", 0, bypassed,  dummy);
        JsonUtil::extractString(obj, "state",    0, stateStr,  dummy);

        if (formatStr != "vst2")
        {
            std::fprintf(stderr,
                "[DSPChain] deserializeFromJson: unsupported format '%s' for '%s' (VST2 only)\n",
                formatStr.c_str(), pathStr.c_str());
            pos = objEnd + 1;
            continue;
        }

        const IVSTPlugin::PluginFormat fmt = IVSTPlugin::PluginFormat::VST2;

        if (!addPlugin(pathStr, fmt))
        {
            std::fprintf(stderr,
                "[DSPChain] deserializeFromJson: failed to add plugin '%s'\n",
                pathStr.c_str());
            pos = objEnd + 1;
            continue;
        }

        // Set bypassed flag on the newly added slot.
        ChainPlugin& slot = m_plugins.back();
        slot.plugin->setBypassed(bypassed);

        // Restore plugin state.
        if (!stateStr.empty())
        {
            const std::vector<uint8_t> stateBytes = base64Decode(stateStr);
            if (!stateBytes.empty() && slot.plugin->isLoaded())
                slot.plugin->loadState(stateBytes);
        }

        pos = objEnd + 1;
    }

    return true;
}
