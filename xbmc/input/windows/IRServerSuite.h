#pragma once
/*
 *      Copyright (C) 2005-2013 Team XBMC
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

#include <winsock2.h>
#include <string>

#include "IrssMessage.h"
#include "threads/Thread.h"
#include "threads/Event.h"

class CRemoteControl : CThread
{
public:
  CRemoteControl();
  virtual ~CRemoteControl();
  void Initialize();
  void Disconnect();
  void Reset();
  void Update();
  WORD GetButton();
  unsigned int GetHoldTime() const;
  bool IsInitialized() {return m_bInitialized;}

  //lirc stuff, not implemented
  bool IsInUse() {return false;}
  void SetEnabled(bool) { }
  void SetDeviceName(const std::string&) { }
  void AddSendCommand(const std::string&) {}

protected:
  virtual void Process();

private:
  WORD  m_button;
  bool  m_bInitialized;
  SOCKET m_socket;
  bool m_isConnecting;
  std::string m_deviceName;
  std::string m_keyCode;
  CEvent m_event;

  bool SendPacket(CIrssMessage& message);
  bool ReadPacket(CIrssMessage& message);
  int  ReadN(char *buffer, int n);
  bool WriteN(const char *buffer, int n);
  bool Connect(bool logMessages);
  void Close();

  bool HandleRemoteEvent(CIrssMessage& message);
};
