/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "addons/binary-addons/AddonInstanceHandler.h"
#include "addons/kodi-addon-dev-kit/include/kodi/addon-instance/Screensaver.h"

namespace ADDON
{

class CScreenSaver : public IAddonInstanceHandler
{
public:
  explicit CScreenSaver(BinaryAddonBasePtr addonBase);
  ~CScreenSaver() override;

  bool Start();
  void Stop();
  void Render();

private:
  std::string m_name; /*!< To add-on sended name */
  std::string m_presets; /*!< To add-on sended preset path */
  std::string m_profile; /*!< To add-on sended profile path */

  AddonInstance_Screensaver m_struct;
};

} /* namespace ADDON */
