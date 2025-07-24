/*
 *  Copyright (C) 2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <cstdint>
#include <string>
#include <string_view>

class AudioType
{
public:
  enum class Type : uint8_t
  {
    Album = 0,
    Single,
    AudioBook
  };

  static Type FromString(const std::string_view str)
  {
    for (const auto& info : releaseTypes)
      if (str == info.name)
        return info.type;
    return Type::Album; // default fallback
  }

  static std::string ToString(Type type)
  {
    for (const auto& info : releaseTypes)
      if (info.type == type)
        return info.name;
    return "album"; // default fallback
  }

private:
  struct ReleaseTypeInfo
  {
    Type type;
    const char* name;
  };

  static constexpr ReleaseTypeInfo releaseTypes[] = {
      {Type::Album, "album"}, {Type::Single, "single"}, {Type::AudioBook, "audiobook"}};
};
