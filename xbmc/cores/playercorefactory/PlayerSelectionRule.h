/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "PlayerCoreFactory.h"

#include <string>
#include <vector>

class CFileItem;
class CRegExp;
class TiXmlElement;

class CPlayerSelectionRule
{
public:
  explicit CPlayerSelectionRule(TiXmlElement* rule);
  virtual ~CPlayerSelectionRule() = default;

  void GetPlayers(const CFileItem& item, std::vector<std::string>&validPlayers, std::vector<std::string>&players);

private:
  static int GetTristate(const char* szValue);
  static bool CompileRegExp(const std::string& str, CRegExp& regExp);
  static bool MatchesRegExp(const std::string& str, CRegExp& regExp);
  void Initialize(TiXmlElement* pRule);

  std::string m_name;

  int m_tAudio;
  int m_tVideo;
  int m_tGame;
  int m_tInternetStream;
  int m_tRemote;

  int m_tBD;
  int m_tDVD;
  int m_tDVDFile;
  int m_tDiscImage;

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
  std::string m_hdrType;

  std::string m_playerName;

  std::vector<std::unique_ptr<CPlayerSelectionRule>> vecSubRules;
};
