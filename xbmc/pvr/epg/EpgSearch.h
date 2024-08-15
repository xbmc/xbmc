/*
 *  Copyright (C) 2023 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "threads/CriticalSection.h"

#include <memory>
#include <vector>

namespace PVR
{
class CPVREpgInfoTag;
class CPVREpgSearchFilter;

class CPVREpgSearch
{
public:
  CPVREpgSearch() = delete;

  /*!
   * @brief ctor.
   * @param filter The filter defining the search criteria.
   */
  explicit CPVREpgSearch(CPVREpgSearchFilter& filter) : m_filter(filter) {}

  /*!
   * @brief Execute the search.
   */
  void Execute();

  /*!
   * @brief Get the last search results.
   * @return the results.
   */
  const std::vector<std::shared_ptr<CPVREpgInfoTag>>& GetResults() const;

private:
  mutable CCriticalSection m_critSection;
  CPVREpgSearchFilter& m_filter;
  std::vector<std::shared_ptr<CPVREpgInfoTag>> m_results;
};
} // namespace PVR
