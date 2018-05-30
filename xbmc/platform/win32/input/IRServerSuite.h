/*
 *      Copyright (C) 2005-2013 Team XBMC
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

#include "IrssMessage.h"
#include "input/IRTranslator.h"
#include "threads/Thread.h"
#include "threads/Event.h"

#include <winsock2.h>
#include <string>

class CIRServerSuite : public CThread
{
public:
  CIRServerSuite();
  virtual ~CIRServerSuite();
  void Initialize();

protected:
  virtual void Process();
  bool ReadNext();

private:
  bool SendPacket(CIrssMessage& message);
  int ReadPacket(CIrssMessage& message);
  int ReadN(char *buffer, int n);
  bool WriteN(const char *buffer, int n);
  bool Connect(bool logMessages);
  void Close();
  bool HandleRemoteEvent(CIrssMessage& message);

  bool m_bInitialized;
  bool m_isConnecting;
  int m_profileId;
  SOCKET m_socket;
  CEvent m_event;
  CCriticalSection m_critSection;
  CIRTranslator m_irTranslator;
};
