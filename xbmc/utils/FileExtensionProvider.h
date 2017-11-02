#pragma once
/*
 *      Copyright (C) 2012-2017 Team Kodi
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

#include "addons/AddonInfo.h"
#include "addons/AddonEvents.h"
#include "settings/AdvancedSettings.h"

class CFileExtensionProvider
{
public:
  CFileExtensionProvider();
  ~CFileExtensionProvider();

  /*!
   * @brief Returns a list of picture extensions
   */
  std::string GetPictureExtensions() const;

  /*!
   * @brief Returns a list of music extensions
   */
  std::string GetMusicExtensions() const;

  /*!
   * @brief Returns a list of video extensions
   */
  std::string GetVideoExtensions() const;

  /*!
   * @brief Returns a list of subtitle extensions
   */
  std::string GetSubtitleExtensions() const;

  /*!
   * @brief Returns a list of disc stub extensions
   */
  std::string GetDiscStubExtensions() const;

  /*!
   * @brief Returns a file folder extensions
   */
  std::string GetFileFolderExtensions() const;

private:
  std::string GetAddonExtensions(const ADDON::TYPE &type) const;
  std::string GetAddonFileFolderExtensions(const ADDON::TYPE &type) const;
  void SetAddonExtensions();
  void SetAddonExtensions(const ADDON::TYPE &type);

  void OnAddonEvent(const ADDON::AddonEvent& event);

  std::map<ADDON::TYPE, std::string> m_addonExtensions;
  std::shared_ptr<CAdvancedSettings> m_advancedSettings;
  std::map<ADDON::TYPE, std::string> m_addonFileFolderExtensions;
};
