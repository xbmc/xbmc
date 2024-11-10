/*
 *  Copyright (C) 2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

class CFileItem;

namespace PVR::UTILS
{
/*!
 * @brief Check whether meta data can be extracted for streams provided by the given item.
 * @param item The item.
 * @return True if meta data can be extracted, false otherwise.
 */
bool ProvidesStreamForMetaDataExtraction(const CFileItem& item);

} // namespace PVR::UTILS
