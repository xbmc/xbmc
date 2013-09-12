#pragma once
/*
 *      Copyright (C) 2013 Team XBMC
 *      http://xbmc.org
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

#include <string>

#include "MediaSource.h"
#include "settings/ISettingsHandler.h"

class TiXmlNode;

class CMediaSourceSettings : public ISettingsHandler
{
public:
  static CMediaSourceSettings& Get();

  static std::string GetSourcesFile();
  
  virtual void OnSettingsLoaded();
  virtual void OnSettingsUnloaded();

  bool Load();
  bool Load(const std::string &file);
  bool Save();
  bool Save(const std::string &file) const;
  void Clear();

  VECSOURCES* GetSources(const std::string &type);
  const std::string& GetDefaultSource(const std::string &type) const;
  void SetDefaultSource(const std::string &type, const std::string &source);

  bool UpdateSource(const std::string &strType, const std::string &strOldName, const std::string &strUpdateChild, const std::string &strUpdateValue);
  bool DeleteSource(const std::string &strType, const std::string &strName, const std::string &strPath, bool virtualSource = false);
  bool AddShare(const std::string &type, const CMediaSource &share);
  bool UpdateShare(const std::string &type, const std::string &oldName, const CMediaSource &share);

protected:
  CMediaSourceSettings();
  CMediaSourceSettings(const CMediaSourceSettings&);
  CMediaSourceSettings& operator=(CMediaSourceSettings const&);
  virtual ~CMediaSourceSettings();

private:
  bool GetSource(const std::string &category, const TiXmlNode *source, CMediaSource &share);
  void GetSources(const TiXmlNode* pRootElement, const std::string& strTagName, VECSOURCES& items, std::string& strDefault);
  bool SetSources(TiXmlNode *root, const char *section, const VECSOURCES &shares, const std::string &defaultPath) const;

  VECSOURCES m_programSources;
  VECSOURCES m_pictureSources;
  VECSOURCES m_fileSources;
  VECSOURCES m_musicSources;
  VECSOURCES m_videoSources;
  VECSOURCES m_gameSources;

  std::string m_defaultProgramSource;
  std::string m_defaultMusicSource;
  std::string m_defaultPictureSource;
  std::string m_defaultFileSource;
};
