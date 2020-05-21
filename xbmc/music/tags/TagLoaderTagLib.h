/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "ImusicInfoTagLoader.h"

#include <string>
#include <vector>

class EmbeddedArt;

namespace MUSIC_INFO
{
  class CMusicInfoTag;
};

class CTagLoaderTagLib : public MUSIC_INFO::IMusicInfoTagLoader
{
public:
  CTagLoaderTagLib() = default;
  ~CTagLoaderTagLib() override = default;
  bool Load(const std::string& strFileName, MUSIC_INFO::CMusicInfoTag& tag,
            EmbeddedArt *art = nullptr) override;
  bool Load(const std::string& strFileName, MUSIC_INFO::CMusicInfoTag& tag,
            const std::string& fallbackFileExtension, EmbeddedArt *art = nullptr);

  static std::vector<std::string> SplitMBID(const std::vector<std::string> &values);
protected:
  static void SetArtist(MUSIC_INFO::CMusicInfoTag &tag, const std::vector<std::string> &values);
  static void SetArtistSort(MUSIC_INFO::CMusicInfoTag &tag, const std::vector<std::string> &values);
  static void SetArtistHints(MUSIC_INFO::CMusicInfoTag &tag, const std::vector<std::string> &values);
  static void SetAlbumArtist(MUSIC_INFO::CMusicInfoTag &tag, const std::vector<std::string> &values);
  static void SetAlbumArtistSort(MUSIC_INFO::CMusicInfoTag &tag, const std::vector<std::string> &values);
  static void SetAlbumArtistHints(MUSIC_INFO::CMusicInfoTag &tag, const std::vector<std::string> &values);
  static void SetComposerSort(MUSIC_INFO::CMusicInfoTag &tag, const std::vector<std::string> &values);
  static void SetDiscSubtitle(MUSIC_INFO::CMusicInfoTag& tag,
                              const std::vector<std::string>& values);
  static void SetGenre(MUSIC_INFO::CMusicInfoTag& tag, const std::vector<std::string>& values);
  static void SetReleaseType(MUSIC_INFO::CMusicInfoTag &tag, const std::vector<std::string> &values);
  static void AddArtistRole(MUSIC_INFO::CMusicInfoTag &tag, const std::string& strRole, const std::vector<std::string> &values);
  static void AddArtistRole(MUSIC_INFO::CMusicInfoTag &tag, const std::vector<std::string> &values);
  static void AddArtistInstrument(MUSIC_INFO::CMusicInfoTag &tag, const std::vector<std::string> &values);
  static int POPMtoXBMC(int popm);

template<typename T>
   static bool ParseTag(T *tag, EmbeddedArt *art, MUSIC_INFO::CMusicInfoTag& infoTag);
};

