#pragma once
/*
 * JsonUtil.h — Lightweight JSON escape/extract helpers (no external library)
 * Part of audiodsp.vsthost — Kodi Audio DSP addon
 * License: GPL-2.0-or-later
 */
#include <string>
#include <cstdint>

namespace JsonUtil {

/// Escape a string value for embedding inside a JSON double-quoted field.
/// Handles all control characters, backslash, and double-quote.
std::string escape(const std::string& s);

/// Extract the value of a simple string field from a JSON object.
/// Searches for: "key": "value" (whitespace-tolerant).
/// Returns empty string if not found.
/// @param searchFrom  Start searching from this offset.
/// @param outEnd      Updated to the character after the closing quote.
bool extractString(const std::string& json, const std::string& key,
                   size_t searchFrom, std::string& outValue, size_t& outEnd);

/// Convenience overload — always searches from the beginning.
std::string extractString(const std::string& json, const std::string& key);

/// Extract the value of a boolean field: "key": true/false
bool extractBool(const std::string& json, const std::string& key,
                 size_t searchFrom, bool& outValue, size_t& outEnd);

/// Convenience overload — always searches from the beginning.
/// Returns false (default) if the field is not found.
bool extractBool(const std::string& json, const std::string& key);

} // namespace JsonUtil
