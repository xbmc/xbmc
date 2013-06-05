/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://www.xbmc.org
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

/*!
 * @file StereoscopicsManager.cpp
 * @brief This class acts as container for stereoscopic related functions
 */

#pragma once

#include <stdlib.h>
#include "settings/ISettingCallback.h"
#include "guilib/IMsgTargetCallback.h"
#include "rendering/RenderSystem.h"

class CAction;

class CStereoscopicsManager : public ISettingCallback,
                              public IMsgTargetCallback
{
public:
  CStereoscopicsManager(void);
  virtual ~CStereoscopicsManager(void);

  /*!
   * @return static instance of CStereoscopicsManager
   */
  static CStereoscopicsManager& Get(void);

  void Initialize(void);
  bool HasStereoscopicSupport(void);
  void SetStereoMode(const RENDER_STEREO_MODE &mode);
  RENDER_STEREO_MODE GetStereoMode(void);
  RENDER_STEREO_MODE GetNextSupportedStereoMode(const RENDER_STEREO_MODE &currentMode, int step = 1);
  std::string DetectStereoModeByString(const std::string &needle);
  RENDER_STEREO_MODE ConvertVideoToGuiStereoMode(const std::string &mode);
  RENDER_STEREO_MODE GetStereoModeByUserChoice(const CStdString& heading = "");
  RENDER_STEREO_MODE GetStereoModeOfPlayingVideo(void);
  CStdString GetLabelForStereoMode(const RENDER_STEREO_MODE &mode);
  RENDER_STEREO_MODE GetPreferredPlaybackMode(void);

  virtual void OnSettingChanged(const CSetting *setting);
  virtual bool OnMessage(CGUIMessage &message);
  /*!
   * @brief Handle 3D specific cActions
   * @param action The action to process
   * @return True if action could be handled, false otherwise.
   */
  bool OnAction(const CAction &action);

private:
  void ApplyStereoMode(const RENDER_STEREO_MODE &mode, bool notify = true);
  void OnPlaybackStarted(void);
  void OnPlaybackStopped(void);

  RENDER_STEREO_MODE m_lastStereoMode;
};
