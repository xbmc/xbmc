/*
*      Copyright (C) 2007-2012 Team XBMC
*      http://www.xbmc.org
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

#ifndef LIRC_H
#define LIRC_H

#include "system.h"
#include "utils/StdString.h"

class CRemoteControl
{
public:
  CRemoteControl();
  ~CRemoteControl();
  void Initialize();
  void Disconnect();
  void Reset();
  void Update();
  WORD GetButton();
  /*! \brief retrieve the time in milliseconds that the button has been held
   \return time in milliseconds the button has been down
   */
  unsigned int GetHoldTime() const;
  void setDeviceName(const CStdString& value);
  void setUsed(bool value);
  bool IsInUse() const { return m_used; }
  bool IsInitialized() const { return m_bInitialized; }
  void AddSendCommand(const CStdString& command);

private:
  int     m_fd;
  int     m_inotify_fd;
  int     m_inotify_wd;
  int     m_lastInitAttempt;
  int     m_initRetryPeriod;
  FILE*   m_file;
  unsigned int m_holdTime;
  int32_t m_button;
  char    m_buf[128];
  bool    m_bInitialized;
  bool    m_used;
  bool    m_bLogConnectFailure;
  uint32_t    m_firstClickTime;
  CStdString  m_deviceName;
  bool        CheckDevice();
  CStdString  m_sendData;
  bool        m_inReply;
  int         m_nrSending;
};

extern CRemoteControl g_RemoteControl;

#endif
