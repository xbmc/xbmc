#pragma once

/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include <assert.h>
#include <atomic>

template<typename T> struct IDVDResourceCounted
{
  IDVDResourceCounted() : m_refs(1) {}
  virtual ~IDVDResourceCounted() {}

  IDVDResourceCounted(const IDVDResourceCounted &) = delete;
  IDVDResourceCounted &operator=(const IDVDResourceCounted &) = delete;

  virtual T*   Acquire()
  {
    ++m_refs;
    return (T*)this;
  }

  virtual long Release()
  {
    long count = --m_refs;
    assert(count >= 0);
    if (count == 0) delete (T*)this;
    return count;
  }
  std::atomic<long> m_refs;
};
