/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "CheatPack.h"

#include "URL.h"
#include "filesystem/File.h"
#include "utils/CharsetConverter.h"
#include "utils/HTMLUtil.h"
#include "utils/StringUtils.h"
#include "utils/log.h"

#include <algorithm>
#include <cstdlib>
#include <map>
#include <vector>

using namespace KODI::GAME;

namespace
{
//! Cheat files run to a few megabytes at the top end. Read past what will be
//! offered, so a large file gives its first cheats rather than none at all.
constexpr int64_t MAX_CHEAT_FILE_SIZE = 16 * 1024 * 1024;

//! The most cheats to take from one file. A list longer than this is not
//! readable, and carrying it costs frames and memory for rows nobody reaches.
constexpr unsigned int MAX_CHEATS = 1024;

//! \brief Resolve the HTML entities a cheat file's text is written with
//!
//! Descriptions come from web sources and carry the entities that came with
//! them, which would otherwise be shown as written: &quot;A&quot; for a quoted
//! button, &amp; between two words.
std::string DecodeEntities(const std::string& text)
{
  if (text.find('&') == std::string::npos)
    return text;

  std::wstring wide;
  g_charsetConverter.utf8ToW(text, wide, false);

  std::wstring decoded;
  HTML::CHTMLUtil::ConvertHTMLToW(wide, decoded);

  std::string result;
  g_charsetConverter.wToUTF8(decoded, result);
  return result;
}

std::string Unquote(std::string value)
{
  StringUtils::Trim(value);
  if (value.size() >= 2 && value.front() == '"' && value.back() == '"')
    value = value.substr(1, value.size() - 2);
  return value;
}
} // namespace

CCheatPack CCheatPack::Load(const std::string& path)
{
  CCheatPack pack;

  XFILE::CFile file;
  if (!file.Open(path))
    return pack;

  const int64_t length = file.GetLength();
  if (length <= 0 || length > MAX_CHEAT_FILE_SIZE)
  {
    CLog::Log(LOGERROR, "CCheatPack: refusing \"{}\", size {} bytes", CURL::GetRedacted(path),
              length);
    return pack;
  }

  std::vector<char> buffer(static_cast<size_t>(length));
  const ssize_t read = file.Read(buffer.data(), buffer.size());
  if (read < 0 || static_cast<size_t>(read) != buffer.size())
  {
    CLog::Log(LOGERROR, "CCheatPack: read {} of {} bytes from \"{}\"", read, buffer.size(),
              CURL::GetRedacted(path));
    return pack;
  }

  // Gather every key first. The file is not required to declare a cheat's
  // fields together, or in order.
  std::map<std::string, std::string> keys;
  for (const std::string& line :
       StringUtils::Split(std::string(buffer.begin(), buffer.end()), "\n"))
  {
    const size_t separator = line.find('=');
    if (separator == std::string::npos)
      continue;

    std::string key = line.substr(0, separator);
    StringUtils::Trim(key);
    if (key.empty() || key.front() == '#')
      continue;

    keys[key] = Unquote(line.substr(separator + 1));
  }

  const auto count = keys.find("cheats");
  if (count == keys.end())
    return pack;

  const unsigned int declared =
      static_cast<unsigned int>(std::strtoul(count->second.c_str(), nullptr, 10));
  if (declared == 0)
    return pack;

  unsigned int skipped = 0;
  for (unsigned int index = 0; index < std::min(declared, MAX_CHEATS); ++index)
  {
    const std::string prefix = StringUtils::Format("cheat{}_", index);

    const auto code = keys.find(prefix + "code");
    if (code == keys.end() || code->second.empty())
    {
      // RetroArch also writes cheats as an address and a value for its own
      // engine to poke. Those have no code to hand a core, so they are counted
      // and left out rather than offered as cheats that do nothing.
      ++skipped;
      continue;
    }

    Cheat cheat;
    cheat.code = code->second;

    const auto description = keys.find(prefix + "desc");
    cheat.description =
        description != keys.end() ? DecodeEntities(description->second) : "";

    const auto longDescription = keys.find(prefix + "long_desc");
    if (longDescription != keys.end())
      cheat.longDescription = DecodeEntities(longDescription->second);

    const auto enabled = keys.find(prefix + "enable");
    cheat.enabled = enabled != keys.end() && StringUtils::EqualsNoCase(enabled->second, "true");

    pack.m_cheats.emplace_back(std::move(cheat));
  }

  CLog::Log(LOGDEBUG, "CCheatPack: \"{}\" declares {} cheat(s), {} usable", CURL::GetRedacted(path),
            declared, pack.m_cheats.size());

  if (declared > MAX_CHEATS)
    CLog::Log(LOGINFO, "CCheatPack: only the first {} of {} cheats are offered", MAX_CHEATS,
              declared);

  if (skipped > 0)
    CLog::Log(LOGDEBUG, "CCheatPack: {} cheat(s) have no code for the core to apply", skipped);

  return pack;
}
