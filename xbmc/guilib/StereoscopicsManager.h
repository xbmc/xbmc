/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

/*!
 * @file StereoscopicsManager.cpp
 * @brief This class acts as container for stereoscopic related functions
 */

#pragma once

#include <stdlib.h>
#include "settings/lib/ISettingCallback.h"
#include "guilib/IMsgTargetCallback.h"
#include "rendering/RenderSystem.h"

class CAction;

enum STEREOSCOPIC_PLAYBACK_MODE
{
  STEREOSCOPIC_PLAYBACK_MODE_ASK,
  STEREOSCOPIC_PLAYBACK_MODE_PREFERRED,
  STEREOSCOPIC_PLAYBACK_MODE_MONO,

  STEREOSCOPIC_PLAYBACK_MODE_IGNORE = 100,
};

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
  RENDER_STEREO_MODE GetStereoMode(void);
  void SetStereoModeByUser(const RENDER_STEREO_MODE &mode);
  void SetStereoMode(const RENDER_STEREO_MODE &mode);
  RENDER_STEREO_MODE GetNextSupportedStereoMode(const RENDER_STEREO_MODE &currentMode, int step = 1);
  std::string DetectStereoModeByString(const std::string &needle);
  RENDER_STEREO_MODE GetStereoModeByUserChoice(const std::string &heading = "");
  RENDER_STEREO_MODE GetStereoModeOfPlayingVideo(void);
  const std::string &GetLabelForStereoMode(const RENDER_STEREO_MODE &mode) const;
  RENDER_STEREO_MODE GetPreferredPlaybackMode(void);
  int ConvertVideoToGuiStereoMode(const std::string &mode);
  /**
   * @brief will convert a string representation into a GUI stereo mode
   * @param mode The string to convert
   * @return -1 if not found, otherwise the according int of the RENDER_STEREO_MODE enum
   */
  int ConvertStringToGuiStereoMode(const std::string &mode);
  const char* ConvertGuiStereoModeToString(const RENDER_STEREO_MODE &mode);
  /**
   * @brief Converts a stereoscopics related action/command from Builtins and JsonRPC into the according cAction ID.
   * @param command The command/action
   * @param parameter The parameter of the command
   * @return The integer of the according cAction or -1 if not valid
   */
  CAction ConvertActionCommandToAction(const std::string &command, const std::string &parameter);
  std::string NormalizeStereoMode(const std::string &mode);
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

  RENDER_STEREO_MODE m_stereoModeSetByUser;
  RENDER_STEREO_MODE m_lastStereoModeSetByUser;
};
