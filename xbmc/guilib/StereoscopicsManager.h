/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: LGPL-2.1-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

/*!
 * @file StereoscopicsManager.cpp
 * @brief This class acts as container for stereoscopic related functions
 */

#include <stdlib.h>
#include "settings/lib/ISettingCallback.h"
#include "guilib/IMsgTargetCallback.h"
#include "rendering/RenderSystemTypes.h"

class CAction;
class CDataCacheCore;
class CGUIWindowManager;
class CSettings;

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
  CStereoscopicsManager();

  ~CStereoscopicsManager(void) override;

  void Initialize();

  RENDER_STEREO_MODE GetStereoMode(void) const;
  std::string DetectStereoModeByString(const std::string &needle) const;
  std::string GetLabelForStereoMode(const RENDER_STEREO_MODE &mode) const;

  void SetStereoMode(const RENDER_STEREO_MODE &mode);

  static const char* ConvertGuiStereoModeToString(const RENDER_STEREO_MODE &mode);
  /**
   * @brief Converts a stereoscopics related action/command from Builtins and JsonRPC into the according cAction ID.
   * @param command The command/action
   * @param parameter The parameter of the command
   * @return The integer of the according cAction or -1 if not valid
   */
  static CAction ConvertActionCommandToAction(const std::string &command, const std::string &parameter);
  static std::string NormalizeStereoMode(const std::string &mode);

  void OnSettingChanged(std::shared_ptr<const CSetting> setting) override;
  void OnStreamChange();
  bool OnMessage(CGUIMessage &message) override;
  /*!
   * @brief Handle 3D specific cActions
   * @param action The action to process
   * @return True if action could be handled, false otherwise.
   */
  bool OnAction(const CAction &action);

private:
  RENDER_STEREO_MODE GetNextSupportedStereoMode(const RENDER_STEREO_MODE &currentMode, int step = 1) const;
  RENDER_STEREO_MODE GetStereoModeByUserChoice() const;
  RENDER_STEREO_MODE GetStereoModeOfPlayingVideo(void) const;
  RENDER_STEREO_MODE GetPreferredPlaybackMode(void) const;
  std::string GetVideoStereoMode() const;
  bool IsVideoStereoscopic() const;

  void SetStereoModeByUser(const RENDER_STEREO_MODE &mode);

  void ApplyStereoMode(const RENDER_STEREO_MODE &mode, bool notify = true);
  void OnPlaybackStopped(void);

  /**
   * @brief will convert a string representation into a GUI stereo mode
   * @param mode The string to convert
   * @return -1 if not found, otherwise the according int of the RENDER_STEREO_MODE enum
   */
  static int ConvertStringToGuiStereoMode(const std::string &mode);
  static int ConvertVideoToGuiStereoMode(const std::string &mode);

  // Construction parameters
  std::shared_ptr<CSettings> m_settings;

  // Stereoscopic parameters
  RENDER_STEREO_MODE m_stereoModeSetByUser;
  RENDER_STEREO_MODE m_lastStereoModeSetByUser;
};
