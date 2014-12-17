#pragma once
/*
 *      Copyright (C) 2005-2013 Team XBMC
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

#include "FileItem.h"
#include "PlayerCoreFactory.h"

class CRegExp;
class TiXmlElement;

class CPlayerSelectionRule
{
public:
  CPlayerSelectionRule(TiXmlElement* rule);
  virtual ~CPlayerSelectionRule();

  //bool Matches(const CFileItem& item) const;
  //std::string GetPlayerName() const;
  void GetPlayers(const CFileItem& item, VECPLAYERCORES &vecCores);

private:
  static int GetTristate(const char* szValue);
  static bool CompileRegExp(const std::string& str, CRegExp& regExp);
  static bool MatchesRegExp(const std::string& str, CRegExp& regExp);
  void Initialize(TiXmlElement* pRule);
  PLAYERCOREID GetPlayerCore();

  std::string m_name;

  int m_tAudio;
  int m_tVideo;
  int m_tInternetStream;
  int m_tRemote;

  int m_tBD;
  int m_tDVD;
  int m_tDVDFile;
  int m_tDVDImage;

  std::string m_protocols;
  std::string m_fileTypes;
  std::string m_mimeTypes;
  std::string m_fileName;

  bool m_bStreamDetails;
  std::string m_audioCodec;
  std::string m_audioChannels;
  std::string m_videoCodec;
  std::string m_videoResolution;
  std::string m_videoAspect;

  std::string m_playerName;
  PLAYERCOREID m_playerCoreId;

  std::vector<CPlayerSelectionRule *> vecSubRules;
};
