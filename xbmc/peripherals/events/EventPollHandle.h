/*
 *  Copyright (C) 2016-2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

namespace PERIPHERALS
{
class CEventPollHandle;

/*!
 * \ingroup peripherals
 *
 * \brief Callback implemented by event scanner
 */
class IEventPollCallback
{
public:
  virtual ~IEventPollCallback(void) = default;

  virtual void Activate(CEventPollHandle& handle) = 0;
  virtual void Deactivate(CEventPollHandle& handle) = 0;
  virtual void HandleEvents(bool bWait) = 0;
  virtual void Release(CEventPollHandle& handle) = 0;
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
  CEventPollHandle(IEventPollCallback& callback);

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
  IEventPollCallback& m_callback;
};
} // namespace PERIPHERALS
