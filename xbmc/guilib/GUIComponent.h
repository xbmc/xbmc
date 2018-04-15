/*
 *      Copyright (C) 2005-2013 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#pragma once

#include <memory>
#include <string>

class CGUIWindowManager;
class CGUITextureManager;
class CGUILargeTextureManager;
class CStereoscopicsManager;
class CGUIInfoManager;

class CGUIComponent
{
public:
  CGUIComponent();
  virtual ~CGUIComponent();
  void Init();
  void Deinit();

  CGUIWindowManager& GetWindowManager();
  CGUITextureManager& GetTextureManager();
  CGUILargeTextureManager& GetLargeTextureManager();
  CStereoscopicsManager &GetStereoscopicsManager();
  CGUIInfoManager &GetInfoManager();

  bool ConfirmDelete(std::string path);

protected:
  // members are pointers in order to avoid includes
  std::unique_ptr<CGUIWindowManager> m_pWindowManager;
  std::unique_ptr<CGUITextureManager> m_pTextureManager;
  std::unique_ptr<CGUILargeTextureManager> m_pLargeTextureManager;
  std::unique_ptr<CStereoscopicsManager> m_stereoscopicsManager;
  std::unique_ptr<CGUIInfoManager> m_guiInfoManager;
};
