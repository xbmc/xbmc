/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "cores/VideoPlayer/Buffers/DmaBufIdentity.h"

#include <cstdint>
#include <initializer_list>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace DRMPRIME
{

//! \brief Ioctl-free bookkeeping for one caller-owned kernel object per dma-buf identity.
class CDmaBufIdentityCache
{
public:
  CDmaBufIdentityCache(size_t maxEntries, std::string_view name)
    : m_maxEntries(maxEntries),
      m_name(name)
  {
  }

  //! \brief Handle for an exact identity+salt match, 0 on miss; a same-memory mismatch dooms the entry.
  uint32_t Lookup(const DmaBufIdentity& identity, uint64_t salt = 0);

  //! \brief Register a nonzero handle for identity+salt.
  void Insert(const DmaBufIdentity& identity, uint32_t handle, uint64_t salt = 0);

  //! \brief Handles to destroy now (doomed plus LRU overflow); protected handles are never returned.
  std::vector<uint32_t> Reap(std::span<const uint32_t> protectedHandles);
  std::vector<uint32_t> Reap(std::initializer_list<uint32_t> protectedHandles)
  {
    return Reap(std::span<const uint32_t>(protectedHandles.begin(), protectedHandles.size()));
  }

  //! \brief Invalidate every entry; handles surface through later Reap calls, honoring protection.
  void InvalidateAll();

  //! \brief Every live and doomed handle; the cache is emptied.
  std::vector<uint32_t> TakeAll();

  size_t Size() const { return m_entries.size(); }

private:
  struct Entry
  {
    DmaBufIdentity identity;
    uint64_t salt{0};
    uint32_t handle{0};
    uint64_t lastUse{0};
  };

  std::vector<Entry> m_entries;
  std::vector<uint32_t> m_doomed;
  uint64_t m_useCounter{0};
  size_t m_maxEntries;
  std::string m_name;
  bool m_warnedEviction{false};
};

} // namespace DRMPRIME
