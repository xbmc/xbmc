/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace XFILE
{
class CFile;
}

namespace KODI::PICTURE::XMP
{
/*!
 * \brief Read the XMP packet near the start of a JPEG, from the file's current position.
 * \return The packet, or empty if there is none
 */
std::string Read(XFILE::CFile& file);

/*!
 * \brief Get a property's value, written either as an attribute or as an element.
 */
std::string_view GetValue(std::string_view xmp, std::string_view name);

/*!
 * \brief Get a property's value as a whole number, or 0 if it's missing.
 */
uint64_t GetUnsigned(std::string_view xmp, std::string_view name);

struct ContainerItem
{
  std::string semantic;
  uint64_t length{0};
  uint64_t padding{0};
};

/*!
 * \brief Get the items of a Container:Directory, in the order they are stored in the file.
 */
std::vector<ContainerItem> GetContainerItems(std::string_view xmp);
} // namespace KODI::PICTURE::XMP
