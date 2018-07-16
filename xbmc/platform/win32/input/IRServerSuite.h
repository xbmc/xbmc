/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
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
