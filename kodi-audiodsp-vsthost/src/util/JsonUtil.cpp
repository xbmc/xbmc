/*
 * JsonUtil.cpp — Lightweight JSON escape/extract helpers (no external library)
 * Part of audiodsp.vsthost — Kodi Audio DSP addon
 * License: GPL-2.0-or-later
 */
#include "JsonUtil.h"

#include <cstdio>

namespace JsonUtil {

std::string escape(const std::string& s)
{
    std::string out;
    out.reserve(s.size());
    for (unsigned char c : s)
    {
        if      (c == '"')  out += "\\\"";
        else if (c == '\\') out += "\\\\";
        else if (c == '\n') out += "\\n";
        else if (c == '\r') out += "\\r";
        else if (c == '\t') out += "\\t";
        else if (c < 0x20)
        {
            char buf[8];
            std::snprintf(buf, sizeof(buf), "\\u%04x", static_cast<unsigned>(c));
            out += buf;
        }
        else
            out += static_cast<char>(c);
    }
    return out;
}

bool extractString(const std::string& json, const std::string& key,
                   size_t searchFrom, std::string& outValue, size_t& outEnd)
{
    const std::string needle = "\"" + key + "\"";
    size_t pos = json.find(needle, searchFrom);
    if (pos == std::string::npos)
        return false;

    pos += needle.size();
    pos = json.find(':', pos);
    if (pos == std::string::npos)
        return false;
    ++pos;

    // Skip whitespace
    while (pos < json.size() && (json[pos] == ' ' || json[pos] == '\t'
                                 || json[pos] == '\n' || json[pos] == '\r'))
        ++pos;

    if (pos >= json.size() || json[pos] != '"')
        return false;
    ++pos;  // skip opening quote

    std::string value;
    while (pos < json.size() && json[pos] != '"')
    {
        if (json[pos] == '\\' && pos + 1 < json.size())
        {
            ++pos;
            switch (json[pos])
            {
                case '"':  value += '"';  break;
                case '\\': value += '\\'; break;
                case 'n':  value += '\n'; break;
                case 'r':  value += '\r'; break;
                case 't':  value += '\t'; break;
                case 'u':
                    // \uXXXX — convert BMP codepoint to UTF-8 byte if in ASCII range
                    if (pos + 4 < json.size())
                    {
                        unsigned int codepoint = 0;
                        if (std::sscanf(json.c_str() + pos + 1, "%4x", &codepoint) == 1)
                        {
                            pos += 4;
                            if (codepoint <= 0x7F)
                                value += static_cast<char>(codepoint);
                            else
                                value += '?';
                        }
                        else
                        {
                            value += 'u';
                        }
                    }
                    else
                    {
                        value += 'u';
                    }
                    break;
                default:   value += json[pos]; break;
            }
        }
        else
        {
            value += json[pos];
        }
        ++pos;
    }

    if (pos < json.size() && json[pos] == '"')
        ++pos;  // skip closing quote

    outValue = value;
    outEnd   = pos;
    return true;
}

std::string extractString(const std::string& json, const std::string& key)
{
    std::string value;
    size_t end = 0;
    extractString(json, key, 0, value, end);
    return value;
}

bool extractBool(const std::string& json, const std::string& key,
                 size_t searchFrom, bool& outValue, size_t& outEnd)
{
    const std::string needle = "\"" + key + "\"";
    size_t pos = json.find(needle, searchFrom);
    if (pos == std::string::npos)
        return false;

    pos += needle.size();
    pos = json.find(':', pos);
    if (pos == std::string::npos)
        return false;
    ++pos;

    // Skip whitespace
    while (pos < json.size() && (json[pos] == ' ' || json[pos] == '\t'
                                 || json[pos] == '\n' || json[pos] == '\r'))
        ++pos;

    if (json.compare(pos, 4, "true") == 0)
    {
        outValue = true;
        outEnd   = pos + 4;
        return true;
    }
    if (json.compare(pos, 5, "false") == 0)
    {
        outValue = false;
        outEnd   = pos + 5;
        return true;
    }
    return false;
}

bool extractBool(const std::string& json, const std::string& key)
{
    bool value = false;
    size_t end = 0;
    extractBool(json, key, 0, value, end);
    return value;
}

} // namespace JsonUtil
