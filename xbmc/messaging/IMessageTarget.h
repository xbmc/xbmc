#pragma once
/*
*      Copyright (C) 2005-2015 Team XBMC
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