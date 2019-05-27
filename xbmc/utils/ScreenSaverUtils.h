/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "addons/IAddon.h"
#include "guilib/IMsgTargetCallback.h"
#include "messaging/IMessageTarget.h"
#include "settings/lib/ISettingsHandler.h"
#include "settings/lib/ISettingCallback.h"
#include "settings/lib/ISubSettings.h"
#include "threads/Thread.h"
#include "utils/Stopwatch.h"
#include "windowing/OSScreenSaver.h"

#include <memory>

class CSetting;

namespace UTILS
{

class CScreenSaverUtils : public ISettingCallback,
                          public KODI::MESSAGING::IMessageTarget,
                          CThread
{
public:
  CScreenSaverUtils();
  ~CScreenSaverUtils();

protected:
  void OnSettingChanged(std::shared_ptr<const CSetting> setting) override;
  void OnSettingAction(std::shared_ptr<const CSetting> setting) override;

  int GetMessageMask() override;
  void OnApplicationMessage(KODI::MESSAGING::ThreadMessage* pMsg) override;

private:
  void Process() override;

  void Activate(bool force);
  void Deactivate();

  void ResetTimer();
  void StopTimer();

  bool m_screenSaverActive{false};

  KODI::WINDOWING::COSScreenSaverInhibitor m_osScreenSaverInhibitor;

  CStopWatch m_screenSaverTimer;
  CStopWatch m_timeSinceActivation;

  ADDON::AddonPtr m_pythonScreenSaver;
};

}
