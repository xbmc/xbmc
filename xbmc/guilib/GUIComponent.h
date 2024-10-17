/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <memory>
#include <string>

class CGUIWindowManager;
class CGUITextureManager;
class CGUILargeTextureManager;
class CGUITextureCallbackManager;
class CStereoscopicsManager;
class CGUIInfoManager;
class CGUIColorManager;
class CGUIAudioManager;
class CGUIAnnouncementHandlerContainer;

class CGUIComponent
{
public:
  CGUIComponent();
  explicit CGUIComponent(bool);
  virtual ~CGUIComponent();
  void Init();
  void Deinit();

  CGUIWindowManager& GetWindowManager();
  CGUITextureManager& GetTextureManager();
  CGUILargeTextureManager& GetLargeTextureManager();
  CGUITextureCallbackManager& GetTextureCallbackManager();
  CStereoscopicsManager &GetStereoscopicsManager();
  CGUIInfoManager &GetInfoManager();
  CGUIColorManager &GetColorManager();
  CGUIAudioManager &GetAudioManager();

  bool ConfirmDelete(const std::string& path);

protected:
  // members are pointers in order to avoid includes
  std::unique_ptr<CGUIWindowManager> m_pWindowManager;
  std::unique_ptr<CGUITextureManager> m_pTextureManager;
  std::unique_ptr<CGUILargeTextureManager> m_pLargeTextureManager;
  std::unique_ptr<CGUITextureCallbackManager> m_pTextureCallbackManager;
  std::unique_ptr<CStereoscopicsManager> m_stereoscopicsManager;
  std::unique_ptr<CGUIInfoManager> m_guiInfoManager;
  std::unique_ptr<CGUIColorManager> m_guiColorManager;
  std::unique_ptr<CGUIAudioManager> m_guiAudioManager;
  std::unique_ptr<CGUIAnnouncementHandlerContainer> m_announcementHandlerContainer;
};
