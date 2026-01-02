/*
 *  Copyright (C) 2005-2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "EdlParser.h"

#include <memory>
#include <vector>

class CFileItem;

namespace EDL
{

/*!
 * @brief Factory for creating EDL parsers
 *
 * Provides access to available EDL parsers based on the file item context.
 * CEdl uses this factory to get applicable parsers without knowing which
 * specific parser types exist or the rules for when each applies.
 */
class CEdlParserFactory
{
public:
  /*!
   * @brief Get all EDL parsers applicable for the given file item
   *
   * Returns file-based parsers for local/LAN items, or metadata-based
   * parsers (like PVR) for other items. The factory handles all context
   * logic internally.
   *
   * @param item The file item to get parsers for
   * @return Vector of applicable parser instances
   */
  static std::vector<std::unique_ptr<IEdlParser>> GetEdlParsersForItem(const CFileItem& item);
};

} // namespace EDL
