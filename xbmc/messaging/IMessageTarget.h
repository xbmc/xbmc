/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

namespace KODI
{
namespace MESSAGING
{
class ThreadMessage;

/*!
 * \class IMessageTarget IMessageTarget.h "messaging/IMessageTarget.h"
 * \brief A class wishing to receive messages should implement this
 *        and call \sa CApplicationMessenger::RegisterReceiver
 *        to start receiving messages
 */
class IMessageTarget
{
public:
  virtual ~IMessageTarget() = default;
  /*!
   * \brief Should return the message mask that it wishes to receive
   *        messages for
   *
   * The message mask is defined in "messaging/ApplicationMessenger.h"
   * pick the next one available when creating a new
   */
  virtual int GetMessageMask() = 0;

  /*!
   * \brief This gets called whenever a message matching the registered
   *        message mask is processed.
   *
   * There are no ordering guarantees here so implementations should never
   * rely on a certain ordering of messages.
   *
   * Cleaning up any pointers stored in the message payload is not specified
   * and is decided by the implementer of the message.
   * In general prefer to delete any data in this method to keep the callsites cleaner
   * and simpler but if data is to be passed back it's perfectly valid to handle it any way
   * that fits the situation as long as it's documented along with the message.
   *
   * To return a simple value the result parameter of \sa ThreadMessage can be used
   * as it will be used as the return value for \sa CApplicationMessenger::SendMsg.
   * It is up to the implementer to decide if this is to be used and it should be documented
   * along with any new message implemented.
   */
  virtual void OnApplicationMessage(ThreadMessage* msg) = 0;
};
}
}
