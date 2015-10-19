/*
 *      Copyright (C) 2015 Team Kodi
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
 *  along with Kodi; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#ifndef LIRC_H
#define LIRC_H

#include <string>
#include <atomic>

#include "system.h"
#include "threads/Thread.h"
#include "threads/Event.h"

class CRemoteControl :  CThread
{
public:
  CRemoteControl();
  virtual ~CRemoteControl();
  void Initialize();
  void Disconnect();
  void Reset();
  void Update();
  WORD GetButton();
  /*! \brief retrieve the time in milliseconds that the button has been held
   \return time in milliseconds the button has been down
   */
  unsigned int GetHoldTime() const;
  void SetDeviceName(const std::string& value);
  void SetEnabled(bool value);
  bool IsInUse() const { return m_used; }
  bool IsInitialized() const { return m_bInitialized; }
  void AddSendCommand(const std::string& command);

protected:
  virtual void Process();

  bool Connect(struct sockaddr_un addr, bool logMessages);

private:
  int     m_fd;
  int     m_inotify_fd;
  int     m_inotify_wd;
  FILE*   m_file;
  unsigned int m_holdTime;
  int32_t m_button;

  std::atomic<bool> m_bInitialized;
  std::atomic<bool> m_inReply;
  std::atomic<int>  m_nrSending;

  bool    m_used;
  uint32_t    m_firstClickTime;
  std::string  m_deviceName;
  bool        CheckDevice();
  std::string  m_sendData;
  CEvent      m_event;
  CCriticalSection m_CS;

};

#endif
