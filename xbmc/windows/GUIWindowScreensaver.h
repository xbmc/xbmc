#pragma once
/*
 *      Copyright (C) 2005-2017 Team Kodi
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
 *  along with Kodi; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "addons/kodi-addon-dev-kit/include/kodi/addon-instance/Screensaver.h"
#include "addons/AddonDll.h"
#include "guilib/GUIWindow.h"

class CGUIWindowScreensaver : public CGUIWindow, public ADDON::IAddonInstanceHandler
{
public:
  CGUIWindowScreensaver(void);

  virtual bool OnMessage(CGUIMessage& message);
  virtual bool OnAction(const CAction &action) { return false; } // We're just a screen saver, nothing to do here
  virtual void Render();
  virtual void Process(unsigned int currentTime, CDirtyRegionList &regions);

protected:
  virtual EVENT_RESULT OnMouseEvent(const CPoint &point, const CMouseEvent &event);

private:
  std::string m_name; /*!< To add-on sended name */
  std::string m_presets; /*!< To add-on sended preset path */
  std::string m_profile; /*!< To add-on sended profile path */

  ADDON::AddonDllPtr m_addon;
  kodi::addon::CInstanceScreensaver* m_addonInstance;
  AddonInstance_Screensaver m_struct;
};

