/*
 *  Copyright (C) 2018-2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

namespace PERIPHERALS
{
class CEventLockHandle;

/*!
 * \ingroup peripherals
 *
 * \brief Callback implemented by event scanner
 */
class IEventLockCallback
{
public:
  virtual ~IEventLockCallback(void) = default;

  virtual void ReleaseLock(CEventLockHandle& handle) = 0;
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
  CEventLockHandle(IEventLockCallback& callback);

  /*!
   * \brief Handle is automatically released when this class is destructed
   */
  ~CEventLockHandle(void);

private:
  // Construction parameters
  IEventLockCallback& m_callback;
};
} // namespace PERIPHERALS
