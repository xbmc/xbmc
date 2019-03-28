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
#include <vector>
#include "IMsgHandler.h"
#include "IGUIComponent.h"

class CGUIWindowManager;
class CGUITextureManager;
class CGUILargeTextureManager;
class CStereoscopicsManager;
class CGUIInfoManager;
class CGUIColorManager;
class CGUIAudioManager;
class IMsgTargetCallback;
class CGUIMessage;

class CGUIComponent : public IMsgHandler, public IGUIComponent
{
public:
  CGUIComponent();
  virtual ~CGUIComponent();
  void Init() override;
  void Deinit() override;

  CGUIWindowManager& GetWindowManager() override;
  CGUITextureManager& GetTextureManager() override;
  CGUILargeTextureManager& GetLargeTextureManager() override;
  CStereoscopicsManager &GetStereoscopicsManager() override;
  CGUIInfoManager &GetInfoManager() override;
  CGUIColorManager &GetColorManager() override;
  CGUIAudioManager &GetAudioManager() override;

  bool ConfirmDelete(std::string path) override;

  void AddMsgTarget(IMsgTargetCallback* pMsgTarget) override;
  
  bool ProcessMsgHooks(CGUIMessage& message) override;
  
protected:

  // members are pointers in order to avoid includes
  std::unique_ptr<CGUIWindowManager> m_pWindowManager;
  std::unique_ptr<CGUITextureManager> m_pTextureManager;
  std::unique_ptr<CGUILargeTextureManager> m_pLargeTextureManager;
  std::unique_ptr<CStereoscopicsManager> m_stereoscopicsManager;
  std::unique_ptr<CGUIInfoManager> m_guiInfoManager;
  std::unique_ptr<CGUIColorManager> m_guiColorManager;
  std::unique_ptr<CGUIAudioManager> m_guiAudioManager;

  std::vector<IMsgTargetCallback*> m_msgTargets;
};
