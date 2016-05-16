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

#include "ImusicInfoTagLoader.h"

#include <string>
#include <vector>

namespace MUSIC_INFO
{
  class CMusicInfoTag;
  class EmbeddedArt;
};

class CTagLoaderTagLib : public MUSIC_INFO::IMusicInfoTagLoader
{
public:
  CTagLoaderTagLib() = default;
  virtual ~CTagLoaderTagLib() = default;
  bool                   Load(const std::string& strFileName, MUSIC_INFO::CMusicInfoTag& tag, MUSIC_INFO::EmbeddedArt *art = nullptr) override;
  bool                   Load(const std::string& strFileName, MUSIC_INFO::CMusicInfoTag& tag, const std::string& fallbackFileExtension, MUSIC_INFO::EmbeddedArt *art = NULL);

  static std::vector<std::string> SplitMBID(const std::vector<std::string> &values);
protected:
  static void SetArtist(MUSIC_INFO::CMusicInfoTag &tag, const std::vector<std::string> &values);
  static void SetArtistHints(MUSIC_INFO::CMusicInfoTag &tag, const std::vector<std::string> &values);
  static void SetAlbumArtist(MUSIC_INFO::CMusicInfoTag &tag, const std::vector<std::string> &values);
  static void SetAlbumArtistHints(MUSIC_INFO::CMusicInfoTag &tag, const std::vector<std::string> &values);
  static void SetGenre(MUSIC_INFO::CMusicInfoTag &tag, const std::vector<std::string> &values);
  static void AddArtistRole(MUSIC_INFO::CMusicInfoTag &tag, const std::string& strRole, const std::vector<std::string> &values);
  static void AddArtistRole(MUSIC_INFO::CMusicInfoTag &tag, const std::vector<std::string> &values);
  static void AddArtistInstrument(MUSIC_INFO::CMusicInfoTag &tag, const std::vector<std::string> &values);
  static int POPMtoXBMC(int popm);

template<typename T>
   static bool ParseTag(T *tag, MUSIC_INFO::EmbeddedArt *art, MUSIC_INFO::CMusicInfoTag& infoTag);
};

