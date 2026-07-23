/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "utils/StringUtils.h"

#include <string>
#include <vector>

/*!
 \brief Erase the first entry whose projected name matches, case-insensitively.
 \param entries the container to erase from
 \param name the font name to match
 \param nameOf projection from an entry to its font name
 \return true if an entry was erased

 Exists as a free template so the erase invariant can be unit tested without a
 GL context. Keeping the font and its OrigFontInfo in one entry is what makes
 the old index-desync bug unrepresentable.
 */
template<typename Entry, typename NameFn>
bool EraseFirstByName(std::vector<Entry>& entries, const std::string& name, NameFn nameOf)
{
  for (auto it = entries.begin(); it != entries.end(); ++it)
  {
    if (StringUtils::EqualsNoCase(nameOf(*it), name))
    {
      entries.erase(it);
      return true;
    }
  }

  return false;
}
