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

#include <string>
#include <vector>

#include "guilib/GUIWindow.h"

class CGUIWindowSystemInfo : public CGUIWindow
{
public:
  CGUIWindowSystemInfo(void);
  ~CGUIWindowSystemInfo(void) override;
  bool OnMessage(CGUIMessage& message) override;
  void FrameMove() override;
private:
  int  m_section;
  void ResetLabels();
  void SetControlLabel(int id, const char *format, int label, int info);
  std::vector<std::string> m_diskUsage;
};

