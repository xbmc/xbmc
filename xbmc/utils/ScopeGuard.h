/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <functional>

namespace KODI
{
namespace UTILS
{

/*! \class CScopeGuard
    \brief Generic scopeguard designed to handle any type of handle

    This is not necessary but recommended to cut down on some typing
    using CSocketHandle = CScopeGuard<SOCKET, INVALID_SOCKET, closesocket>;

    CSocketHandle sh(closesocket, open(thingy));
 */
template<typename Handle, Handle invalid, typename Deleter>
class CScopeGuard
{

public:

  CScopeGuard(std::function<Deleter> del, Handle handle = invalid)
    : m_handle{handle}
    , m_deleter{del}
  { };

  ~CScopeGuard() noexcept
  {
    reset();
  }

  operator Handle() const
  {
    return m_handle;
  }

  operator bool() const
  {
    return m_handle != invalid;
  }

  /*! \brief  attach a new handle to this instance, if there's
              already a handle it will be closed.

      \param[in]  handle  The handle to manage
   */
  void attach(Handle handle)
  {
    reset();

    m_handle = handle;
  }

  /*! \brief release the managed handle so that it won't be auto closed

      \return The handle being managed by the guard
   */
  Handle release()
  {
    Handle h = m_handle;
    m_handle = invalid;
    return h;
  }

  /*! \brief reset the instance, closing any managed handle and setting it to invalid
   */
  void reset()
  {
    if (m_handle != invalid)
    {
      m_deleter(m_handle);
      m_handle = invalid;
    }
  }

  //Disallow default construction and copying
  CScopeGuard() = delete;
  CScopeGuard(const CScopeGuard& rhs) = delete;
  CScopeGuard& operator= (const CScopeGuard& rhs) = delete;

  //Allow moving
  CScopeGuard(CScopeGuard&& rhs)
    : m_handle{std::move(rhs.m_handle)}, m_deleter{std::move(rhs.m_deleter)}
  {
    // Bring moved-from object into released state so destructor will not do anything
    rhs.release();
  }
  CScopeGuard& operator=(CScopeGuard&& rhs)
  {
    attach(rhs.release());
    m_deleter = std::move(rhs.m_deleter);
    return *this;
  }

private:
  Handle m_handle;
  std::function<Deleter> m_deleter;
};

}
}
