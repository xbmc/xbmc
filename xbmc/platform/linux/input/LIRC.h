/*
*      Copyright (C) 2007-2013 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#pragma once

#include <string>
#include <atomic>

#include "system.h"
#include "input/remote/IRemoteControl.h"
#include "threads/Thread.h"
#include "threads/Event.h"

class CRemoteControl : public KODI::REMOTE::IRemoteControl, CThread
{
public:
  CRemoteControl();
  ~CRemoteControl() override;
  void Initialize() override;
  void Disconnect() override;
  void Reset() override;
  void Update() override;
  uint16_t GetButton() const override;
  /*! \brief retrieve the time in milliseconds that the button has been held
   \return time in milliseconds the button has been down
   */
  uint32_t GetHoldTimeMs() const override;
  void SetDeviceName(const std::string& name) override;
  void SetEnabled(bool bEnabled) override;
  bool IsInUse() const override { return m_used; }
  bool IsInitialized() const override { return m_bInitialized; }
  void AddSendCommand(const std::string& command) override;
  std::string GetMapFile() override;

  static IRemoteControl* CreateInstance();
  static void Register();

protected:
  void Process() override;

  bool Connect(struct sockaddr_un addr, bool logMessages);

private:
  int     m_fd;
#ifdef HAVE_INOTIFY
  int     m_inotify_fd;
  int     m_inotify_wd;
#endif
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
