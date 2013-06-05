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
#include "rendering/RenderSystem.h"

class CStereoscopicsManager : public ISettingCallback
{
public:
  CStereoscopicsManager(void);
  virtual ~CStereoscopicsManager(void);

  /*!
   * @return static instance of CStereoscopicsManager
   */
  static CStereoscopicsManager& Get(void);

  bool HasStereoscopicSupport(void);
  void SetStereoMode(const RENDER_STEREO_MODE &mode);
  RENDER_STEREO_MODE GetStereoMode(void);
  CStdString GetLabelForStereoMode(const RENDER_STEREO_MODE &mode);
  virtual void OnSettingChanged(const CSetting *setting);

private:
  void ApplyStereoMode(const RENDER_STEREO_MODE &mode, bool notify = true);
};
