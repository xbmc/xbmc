/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DmaBufIdentityCache.h"

#include <algorithm>

namespace DRMPRIME
{

uint32_t CDmaBufIdentityCache::Lookup(const DmaBufIdentity& identity, uint64_t salt)
{
  for (auto it = m_entries.begin(); it != m_entries.end(); ++it)
  {
    if (it->salt == salt && it->identity == identity)
    {
      it->lastUse = ++m_useCounter;
      return it->handle;
    }

    // same memory in a new shape: the old object can never be valid again
    if (it->salt == salt && it->identity.SameMemory(identity))
    {
      m_doomed.push_back(it->handle);
      m_entries.erase(it);
      return 0;
    }
  }
  return 0;
}

void CDmaBufIdentityCache::Insert(const DmaBufIdentity& identity, uint32_t handle, uint64_t salt)
{
  if (!handle)
    return;

  m_entries.push_back(Entry{identity, salt, handle, ++m_useCounter});
}

std::vector<uint32_t> CDmaBufIdentityCache::Reap(uint32_t protectA, uint32_t protectB)
{
  std::vector<uint32_t> reaped;

  auto protectedHandle = [&](uint32_t handle) { return handle == protectA || handle == protectB; };

  auto it = m_doomed.begin();
  while (it != m_doomed.end())
  {
    if (protectedHandle(*it))
    {
      ++it;
      continue;
    }
    reaped.push_back(*it);
    it = m_doomed.erase(it);
  }

  while (m_entries.size() > m_maxEntries)
  {
    auto victim = m_entries.end();
    for (auto candidate = m_entries.begin(); candidate != m_entries.end(); ++candidate)
    {
      if (protectedHandle(candidate->handle))
        continue;
      if (victim == m_entries.end() || candidate->lastUse < victim->lastUse)
        victim = candidate;
    }
    if (victim == m_entries.end())
      break;

    reaped.push_back(victim->handle);
    m_entries.erase(victim);
  }

  return reaped;
}

std::vector<uint32_t> CDmaBufIdentityCache::TakeAll()
{
  std::vector<uint32_t> all = std::move(m_doomed);
  m_doomed.clear();
  for (const Entry& entry : m_entries)
    all.push_back(entry.handle);
  m_entries.clear();
  return all;
}

} // namespace DRMPRIME
