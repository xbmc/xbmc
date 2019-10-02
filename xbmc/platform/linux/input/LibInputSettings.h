/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "settings/Settings.h"
#include "settings/lib/ISettingCallback.h"
#include "settings/lib/ISettingsHandler.h"

#include <memory>
#include <vector>

class CLibInputHandler;
struct StringSettingOption;

class CLibInputSettings : public ISettingCallback, public ISettingsHandler
{
public:
  static const std::string SETTING_INPUT_LIBINPUTKEYBOARDLAYOUT;

  void OnSettingChanged(std::shared_ptr<const CSetting> setting) override;
  static void SettingOptionsKeyboardLayoutsFiller(std::shared_ptr<const CSetting> setting, std::vector<StringSettingOption> &list, std::string &current, void *data);

  CLibInputSettings(CLibInputHandler *handler);
  ~CLibInputSettings() override;

private:
  CLibInputHandler *m_libInputHandler{nullptr};
};
