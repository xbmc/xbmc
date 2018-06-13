/*
 *      Copyright (C) 2018 Team Kodi
 *      http://kodi.tv
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
 *  along with this Program; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */
#pragma once

namespace PERIPHERALS
{
  class CEventLockHandle;

  /*!
   * \brief Callback implemented by event scanner
   */
  class IEventLockCallback
  {
  public:
    virtual ~IEventLockCallback(void) = default;

    virtual void ReleaseLock(CEventLockHandle &handle) = 0;
  };

  /*!
   * \brief Handle returned by the event scanner to disable event processing
   *
   * When held, this disables event processing.
   */
  class CEventLockHandle
  {
  public:
    /*!
     * \brief Create an event lock handle
     */
    CEventLockHandle(IEventLockCallback &callback);

    /*!
     * \brief Handle is automatically released when this class is destructed
     */
    ~CEventLockHandle(void);

  private:
    // Construction parameters
    IEventLockCallback &m_callback;
  };
}
