/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "application/IApplicationComponent.h"

#include <string>

class CApplication;
class CSetting;
class IMsgTargetCallback;
class IWindowManagerCallback;

/*!
 * \brief Class handling application support for skin management.
 */
class CApplicationSkinHandling : public IApplicationComponent
{
  friend class CApplication;

public:
  CApplicationSkinHandling(IMsgTargetCallback* msgCb,
                           IWindowManagerCallback* wCb,
                           bool& bInitializing);

  void UnloadSkin();

  bool OnSettingChanged(const CSetting& setting);
  void ReloadSkin(bool confirm = false);

protected:
  bool LoadSkin(const std::string& skinID);
  bool LoadCustomWindows();

  /*!
 * \brief Called by the application main/render thread for processing
 *        operations belonging to the skin.
 */
  void ProcessSkin() const;

  bool m_saveSkinOnUnloading = true;
  bool m_confirmSkinChange = true;
  bool m_ignoreSkinSettingChanges = false;
  IMsgTargetCallback* m_msgCb;
  IWindowManagerCallback* m_wCb;
  bool& m_bInitializing;
};
