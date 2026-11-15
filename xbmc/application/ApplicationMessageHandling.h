/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "application/AppInboundProtocol.h"
#include "threads/Timer.h"

#include <memory>

class CApplication;
class CGUIMessage;
namespace KODI::MESSAGING
{
class ThreadMessage;
struct ThreadMessageCallback;
}

class CApplicationMessageHandling : public CAppInboundProtocol
{
public:
  explicit CApplicationMessageHandling(CApplication& app);
  ~CApplicationMessageHandling();

  void OnApplicationMessage(KODI::MESSAGING::ThreadMessage* pMsg);
  bool OnMessage(const CGUIMessage& message);

private:
  static void ReloadSkinIfNeeded(void* data);
  void ScheduleSkinReload();
  void ScheduleSkinReloadAfterPlayback();

  CApplication& m_app;
  std::unique_ptr<KODI::MESSAGING::ThreadMessageCallback> m_reloadSkinCallback;
  CTimer m_skinReloadTimer;
};
