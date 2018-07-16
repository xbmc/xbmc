/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <assert.h>
#include <atomic>

template<typename T> struct IDVDResourceCounted
{
  IDVDResourceCounted() : m_refs(1) {}
  virtual ~IDVDResourceCounted() = default;

  IDVDResourceCounted(const IDVDResourceCounted &) = delete;
  IDVDResourceCounted &operator=(const IDVDResourceCounted &) = delete;

  virtual T*  Acquire()
  {
    ++m_refs;
    return static_cast<T*>(this);
  }

  virtual long Release()
  {
    long count = --m_refs;
    assert(count >= 0);
    if (count == 0)
      delete static_cast<T*>(this);
    return count;
  }
  std::atomic<long> m_refs;
};
