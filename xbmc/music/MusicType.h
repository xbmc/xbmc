/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */
#pragma once
#include <algorithm>
#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

class AudioType
{
public:
  enum class Content : uint8_t
  {
    Album = 0,
    Single,
    AudioBook,
    Podcast,
    Concert
  };

  // Implicit construction from enum - allows: AudioType x = AudioType::Content::Album;
  constexpr AudioType(Content type = Content::Album) noexcept : m_type(type) {}

  // Implicit conversion to enum - allows switch/case and comparisons
  constexpr operator Content() const noexcept { return m_type; }

  // Explicit getter if preferred over implicit conversion
  constexpr Content GetContent() const noexcept { return m_type; }

  // Instance method for converting to string
  constexpr std::string_view ToString() const noexcept
  {
    auto it = std::ranges::find(releaseTypes, m_type, &ReleaseTypeInfo::type);
    return it != releaseTypes.end() ? it->name : std::string_view{"album"};
  }

  // Static factory from string
  static constexpr std::optional<AudioType> FromString(std::string_view str) noexcept
  {
    auto it = std::ranges::find(releaseTypes, str, &ReleaseTypeInfo::name);
    return it != releaseTypes.end() ? std::optional{AudioType{it->type}} : std::nullopt;
  }

  static constexpr std::string_view ToString(Content type) noexcept
  {
    return AudioType(type).ToString();
  }

  // Convenience: owning std::string version for APIs requiring null-terminated strings (e.g. c_str()).
  std::string ToStdString() const { return std::string{ToString()}; }
  static std::string ToStdString(Content type) { return std::string{ToString(type)}; }

  // Comparisons (defaulted generates all 6 operators: ==, !=, <, <=, >, >=)
  constexpr auto operator<=>(const AudioType&) const noexcept = default;

private:
  Content m_type;

  struct ReleaseTypeInfo
  {
    Content type;
    std::string_view name;
  };

  static constexpr std::array releaseTypes{
      ReleaseTypeInfo{Content::Album, "album"}, ReleaseTypeInfo{Content::Single, "single"},
      ReleaseTypeInfo{Content::AudioBook, "audiobook"},
      ReleaseTypeInfo{Content::Podcast, "podcast"}, ReleaseTypeInfo{Content::Concert, "concert"}};
};