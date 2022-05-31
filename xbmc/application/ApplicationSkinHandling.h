/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <string>

class CApplicationPlayer;
class CSetting;
class IMsgTargetCallback;
class IWindowManagerCallback;

/*!
 * \brief Class handling application support for skin management.
 */
class CApplicationSkinHandling
{
public:
  explicit CApplicationSkinHandling(CApplicationPlayer& appPlayer);

  void UnloadSkin();

  bool OnSettingChanged(const CSetting& setting);

protected:
  bool LoadSkin(const std::string& skinID, IMsgTargetCallback* msgCb, IWindowManagerCallback* wCb);
  bool LoadCustomWindows();
  void ReloadSkin(bool confirm, IMsgTargetCallback* msgCb, IWindowManagerCallback* wCb);

  CApplicationPlayer& m_appPlayer;
  bool m_saveSkinOnUnloading = true;
  bool m_confirmSkinChange = true;
  bool m_ignoreSkinSettingChanges = false;
};
