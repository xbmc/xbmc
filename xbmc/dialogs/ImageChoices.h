/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <charconv>
#include <cstddef>
#include <optional>
#include <string>
#include <string_view>

//! \brief The paths of the entries offered alongside image files by
//! CGUIDialogFileBrowser::ShowAndGetImage(), which stand for a choice rather than an image.
namespace KODI::IMAGE_CHOICE
{

inline constexpr std::string_view CURRENT = "thumb://Current";
inline constexpr std::string_view LOCAL = "thumb://Local";
inline constexpr std::string_view EMBEDDED = "thumb://Embedded";
inline constexpr std::string_view THUMB = "thumb://Thumb";
inline constexpr std::string_view NONE = "thumb://None";
inline constexpr std::string_view REMOTE = "thumb://Remote";

//! The entry the file browser adds itself, to browse for an image file
inline constexpr std::string_view BROWSE = "image://Browse";

//! \brief The entry for the remote image at \p index.
inline std::string RemoteOf(std::size_t index)
{
  return std::string{REMOTE} + std::to_string(index);
}

//! \brief The index of the remote image \p path is the entry for, if it is one.
inline std::optional<std::size_t> RemoteIndexOf(std::string_view path)
{
  if (!path.starts_with(REMOTE))
    return std::nullopt;

  std::size_t index{};
  const char* const last{path.data() + path.size()};
  const auto [end, error] = std::from_chars(path.data() + REMOTE.size(), last, index);
  if (error != std::errc{} || end != last)
    return std::nullopt;
  return index;
}

} // namespace KODI::IMAGE_CHOICE
