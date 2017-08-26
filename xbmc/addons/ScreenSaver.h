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
#pragma once

#include "addons/kodi-addon-dev-kit/include/kodi/addon-instance/Screensaver.h"
#include "addons/binary-addons/AddonInstanceHandler.h"

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
