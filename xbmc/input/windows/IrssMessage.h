#pragma once
/*
 *      Copyright (C) 2005-2012 Team XBMC
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

#include "utils/StdString.h"

  /// <summary>
  /// Type of message.
  /// </summary>
  enum IRSS_MessageType
  {
    /// <summary>
    /// Unknown message type.
    /// </summary>
    IRSSMT_Unknown = 0,

    /// <summary>
    /// Register Client.
    /// </summary>
    IRSSMT_RegisterClient = 1,
    /// <summary>
    /// Unregister Client.
    /// </summary>
    IRSSMT_UnregisterClient = 2,

    /// <summary>
    /// Register Repeater.
    /// </summary>
    IRSSMT_RegisterRepeater = 3,
    /// <summary>
    /// Unregister Repeater.
    /// </summary>
    IRSSMT_UnregisterRepeater = 4,

    /// <summary>
    /// Learn IR Command.
    /// </summary>
    IRSSMT_LearnIR = 5,
    /// <summary>
    /// Blast IR Command.
    /// </summary>
    IRSSMT_BlastIR = 6,

    /// <summary>
    /// Error.
    /// </summary>
    IRSSMT_Error = 7,

    /// <summary>
    /// Server Shutdown.
    /// </summary>
    IRSSMT_ServerShutdown = 8,
    /// <summary>
    /// Server Suspend.
    /// </summary>
    IRSSMT_ServerSuspend = 9,
    /// <summary>
    /// Server Resume
    /// </summary>
    IRSSMT_ServerResume = 10,

    /// <summary>
    /// Remote Event.
    /// </summary>
    IRSSMT_RemoteEvent = 11,
    /// <summary>
    /// Keyboard Event.
    /// </summary>
    IRSSMT_KeyboardEvent = 12,
    /// <summary>
    /// Mouse Event.
    /// </summary>
    IRSSMT_MouseEvent = 13,

    /// <summary>
    /// Forward a Remote Event.
    /// </summary>
    IRSSMT_ForwardRemoteEvent = 14,
    /// <summary>
    /// Forward a Keyboard Event.
    /// </summary>
    IRSSMT_ForwardKeyboardEvent = 15,
    /// <summary>
    /// Forward a Mouse Event.
    /// </summary>
    IRSSMT_ForwardMouseEvent = 16,

    /// <summary>
    /// Available Receivers.
    /// </summary>
    IRSSMT_AvailableReceivers = 17,
    /// <summary>
    /// Available Blasters.
    /// </summary>
    IRSSMT_AvailableBlasters = 18,
    /// <summary>
    /// Active Receivers.
    /// </summary>
    IRSSMT_ActiveReceivers = 19,
    /// <summary>
    /// Active Blasters.
    /// </summary>
    IRSSMT_ActiveBlasters = 20,
    /// <summary>
    /// Detected Receivers.
    /// </summary>
    IRSSMT_DetectedReceivers = 21,
    /// <summary>
    /// Detected Blasters.
    /// </summary>
    IRSSMT_DetectedBlasters = 22,
  };

  /// <summary>
  /// Flags to determine more information about the message.
  /// </summary>
  enum IRSS_MessageFlags
  {
    /// <summary>
    /// No Flags.
    /// </summary>
    IRSSMF_None            = 0x0000,

    /// <summary>
    /// Message is a Request.
    /// </summary>
    IRSSMF_Request         = 0x0001,
    /// <summary>
    /// Message is a Response to a received Message.
    /// </summary>
    IRSSMF_Response        = 0x0002,
    /// <summary>
    /// Message is a Notification.
    /// </summary>
    IRSSMF_Notify          = 0x0004,

    /// <summary>
    /// Operation Success.
    /// </summary>
    IRSSMF_Success         = 0x0008,
    /// <summary>
    /// Operation Failure.
    /// </summary>
    IRSSMF_Failure         = 0x0010,
    /// <summary>
    /// Operation Time-Out.
    /// </summary>
    IRSSMF_Timeout         = 0x0020,

    //IRSSMF_Error           = 0x0040,

    //IRSSMF_DataString      = 0x0080,
    //IRSSMF_DataBytes       = 0x0100,

    //IRSSMF_ForceRespond    = 0x0200,

    /// <summary>
    /// Force the recipient not to respond.
    /// </summary>
    IRSSMF_ForceNotRespond = 0x0400,
  };

class CIrssMessage
{
public:
  CIrssMessage();
  CIrssMessage(IRSS_MessageType type, uint32_t flags);
  CIrssMessage(IRSS_MessageType type, uint32_t flags, char* data, int size);
  CIrssMessage(IRSS_MessageType type, uint32_t flags, const CStdString& data);
  ~CIrssMessage();

  void SetDataAsBytes(char* data, int size);
  void SetDataAsString(const CStdString& data);
  char* ToBytes(int& size);
  void SetType(IRSS_MessageType type);
  void SetFlags(uint32_t flags);
  IRSS_MessageType GetType() {return m_type;}
  uint32_t GetFlags() {return m_flags;}
  char* GetData() {return m_data;}
  uint32_t GetDataSize() {return m_dataSize;}
  static bool FromBytes(char* from, int size, CIrssMessage& message);

private:
  IRSS_MessageType m_type;
  uint32_t m_flags;

  char* m_data;
  int m_dataSize;

  void FreeData();
};
