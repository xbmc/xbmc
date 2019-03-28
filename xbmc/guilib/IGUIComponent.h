/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <string>

class CGUIWindowManager;
class CGUITextureManager;
class CGUILargeTextureManager;
class CStereoscopicsManager;
class CGUIInfoManager;
class CGUIColorManager;
class CGUIAudioManager;
class IMsgTargetCallback;

class IGUIComponent
{
public:

  virtual void Init() = 0;
  virtual void Deinit() = 0;

  virtual CGUIWindowManager& GetWindowManager() = 0;
  virtual CGUITextureManager& GetTextureManager() = 0;
  virtual CGUILargeTextureManager& GetLargeTextureManager() = 0;
  virtual CStereoscopicsManager &GetStereoscopicsManager() = 0;
  virtual CGUIInfoManager &GetInfoManager() = 0;
  virtual CGUIColorManager &GetColorManager() = 0;
  virtual CGUIAudioManager &GetAudioManager() = 0;

  virtual bool ConfirmDelete(std::string path) = 0;
  
  virtual void AddMsgTarget(IMsgTargetCallback* pMsgTarget) = 0;

  virtual ~IGUIComponent() = default;
};
