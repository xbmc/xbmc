#pragma once
/*
*      Copyright (C) 2005-2017 Team Kodi
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
*  along with Kodi; see the file COPYING.  If not, see
*  <http://www.gnu.org/licenses/>.
*
*/

class CGUIMessage;

namespace KODI
{
namespace MESSAGING
{
namespace HELPERS
{
  /**
   * \brief SendGUIMessage sends a message to the UI thread and wait's for completion
   * 
   * Senders SHOULD NOT hold a lock when sending a message as it may easily cause deadlocks.
   * Prefer to use \sa PostGUIMessage unless requiring a result back or wanting to make sure
   * message is processed before proceeding.
   * 
   * \param [in] message \sa CGUIMessage to send
   * \return true for success, false for failure
   * \sa PostGUIMessage
   */
  bool SendGUIMessage(CGUIMessage& message);

  /**
   * \brief SendGUIMessage sends a message to the UI thread and wait's for completion
   * 
   * Senders SHOULD NOT hold a lock when sending a message as it may easily cause deadlocks.
   * Prefer to use \sa PostGUIMessage unless requiring a result back or wanting to make sure
   * message is processed before proceeding.
   * 
   * \param [in] message message id defined in GUIMessage.h 
   * \param [in] senderID window id of the sender, if it's from a window or control
   * \param [in] destID window/control id the message is intended for
   * \param [in] param1 optional value, meaning is defined by the message
   * \param [in] param2 optional value, meaning is defined by the message
   * \return true for success, false for failure
   * \sa PostGUIMessage
   */
  bool SendGUIMessage(int message, int senderID, int destID, int param1 = 0, int param2 = 0);

  /**
   * \brief SendGUIMessage sends a message to the UI thread and wait's for completion
   * 
   * Senders SHOULD NOT hold a lock when sending a message as it may easily cause deadlocks.
   * Prefer to use \sa PostGUIMessage unless requiring a result back or wanting to make sure
   * message is processed before proceeding.
   * 
   * \param [in] message message id defined in GUIMessage.h 
   * \param [in] window window/control id the message is intended for
   * \return true for success, false for failure
   * \sa PostGUIMessage
   */
  bool SendGUIMessage(CGUIMessage& message, int window);

  /**
   * \brief PostGUIMessage adds a GUI message to the message queue and returns directly
   * 
   * No return values are available, if synchronicity or a return value is required, see
   * \sa SendGUIMessage
   * 
   * \param [in] message \sa CGUIMessage to post
   * \sa SendGUIMessage
   */
  void PostGUIMessage(CGUIMessage& message);

  /**
   * \brief PostGUIMessage adds a GUI message to the message queue and returns directly
   * 
   * No return values are available, if synchronicity or a return value is required, see
   * \sa SendGUIMessage
   * 
   * \param [in] message message id defined in GUIMessage.h 
   * \param [in] senderID window id of the sender, if it's from a window or control
   * \param [in] destID window/control id the message is intended for
   * \param [in] param1 optional value, meaning is defined by the message
   * \param [in] param2 optional value, meaning is defined by the message
   * \sa SendGUIMessage
   */
  void PostGUIMessage(int message, int senderID, int destID, int param1 = 0, int param2 = 0);

  /**
   * \brief PostGUIMessage adds a GUI message to the message queue and returns directly
   * 
   * No return values are available, if synchronicity or a return value is required, see
   * \sa SendGUIMessage
   * 
   * \param [in] message \sa CGUIMessage to post
   * \param [in] window the window id to post the message to
   * \sa SendGUIMessage
   */
  void PostGUIMessage(CGUIMessage& message, int window);
}
}
}