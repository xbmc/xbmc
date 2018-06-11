/*
 *      Copyright (C) 2016-2017 Team Kodi
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
  class CEventPollHandle;

  /*!
   * \brief Callback implemented by event scanner
   */
  class IEventPollCallback
  {
  public:
    virtual ~IEventPollCallback(void) = default;

    virtual void Activate(CEventPollHandle &handle) = 0;
    virtual void Deactivate(CEventPollHandle &handle) = 0;
    virtual void HandleEvents(bool bWait) = 0;
    virtual void Release(CEventPollHandle &handle) = 0;
  };

  /*!
   * \brief Handle returned by the event scanner to control scan timing
   *
   * When held, this allows one to control the timing of when events are
   * handled.
   */
  class CEventPollHandle
  {
  public:
    /*!
     * \brief Create an active polling handle
     */
    CEventPollHandle(IEventPollCallback &callback);

    /*!
     * \brief Handle is automatically released when this class is destructed
     */
    ~CEventPollHandle();

    /*!
     * \brief Activate handle
     */
    void Activate();

    /*!
     * \brief Deactivate handle
     */
    void Deactivate();

    /*!
     * \brief Trigger a scan for events
     *
     * \param bWait If true, this blocks until all events are handled
     */
    void HandleEvents(bool bWait);

  private:
    IEventPollCallback &m_callback;
  };
}
