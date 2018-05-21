#pragma once

/*
 *      Copyright (C) 2005-present Team Kodi
 *      http://kodi.tv
 *
 *  Kodi is free software: you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Kodi is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Kodi. If not, see <https://www.gnu.org/licenses/>.
 *
 */

#include "guilib/GUIDialog.h"
#ifdef TARGET_POSIX
#include "platform/linux/LinuxResourceCounter.h"
#endif

class CGUITextLayout;

class CGUIWindowDebugInfo :
      public CGUIDialog
{
public:
  CGUIWindowDebugInfo();
  ~CGUIWindowDebugInfo() override;
  void Process(unsigned int currentTime, CDirtyRegionList &dirtyregions) override;
  void Render() override;
  bool OnMessage(CGUIMessage &message) override;
protected:
  void UpdateVisibility() override;
private:
  CGUITextLayout *m_layout;
#ifdef TARGET_POSIX
  CLinuxResourceCounter m_resourceCounter;
#endif
};
