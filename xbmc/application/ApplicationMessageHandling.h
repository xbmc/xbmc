/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "application/AppInboundProtocol.h"

class CApplication;
namespace KODI::MESSAGING
{
class ThreadMessage;
}

class CApplicationMessageHandling : public CAppInboundProtocol
{
public:
  explicit CApplicationMessageHandling(CApplication& app);

  void OnApplicationMessage(KODI::MESSAGING::ThreadMessage* pMsg);

private:
  CApplication& m_app;
};
