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

#include <taglib/aifffile.h>
#include <taglib/apefile.h>
#include <taglib/asffile.h>
#include <taglib/flacfile.h>
#include <taglib/itfile.h>
#include <taglib/modfile.h>
#include <taglib/mpcfile.h>
#include <taglib/mp4file.h>
#include <taglib/mpegfile.h>
#include <taglib/oggfile.h>
#include <taglib/oggflacfile.h>
#include <taglib/rifffile.h>
#include <taglib/speexfile.h>
#include <taglib/s3mfile.h>
#include <taglib/trueaudiofile.h>
#include <taglib/vorbisfile.h>
#include <taglib/wavfile.h>
#include <taglib/wavpackfile.h>
#include <taglib/xmfile.h>

#include <taglib/id3v2tag.h>
#include <taglib/xiphcomment.h>
#include <taglib/mp4tag.h>
#include "ImusicInfoTagLoader.h"

namespace MUSIC_INFO
{
  class CMusicInfoTag;
  class EmbeddedArt;
};

class CTagLoaderTagLib : public MUSIC_INFO::IMusicInfoTagLoader
{
public:
  CTagLoaderTagLib();
  virtual ~CTagLoaderTagLib();
  virtual bool                   Load(const std::string& strFileName, MUSIC_INFO::CMusicInfoTag& tag, MUSIC_INFO::EmbeddedArt *art = NULL);
  bool                           Load(const std::string& strFileName, MUSIC_INFO::CMusicInfoTag& tag, const std::string& fallbackFileExtension, MUSIC_INFO::EmbeddedArt *art = NULL);

  static const std::vector<std::string> SplitMBID(const std::vector<std::string> &values);
protected:
  static void SetArtist(MUSIC_INFO::CMusicInfoTag &tag, const std::vector<std::string> &values);
  static void SetArtistHints(MUSIC_INFO::CMusicInfoTag &tag, const std::vector<std::string> &values);
  static void SetAlbumArtist(MUSIC_INFO::CMusicInfoTag &tag, const std::vector<std::string> &values);
  static void SetAlbumArtistHints(MUSIC_INFO::CMusicInfoTag &tag, const std::vector<std::string> &values);
  static void SetGenre(MUSIC_INFO::CMusicInfoTag &tag, const std::vector<std::string> &values);
  static char POPMtoXBMC(int popm);

template<typename T>
   static bool ParseTag(T *tag, MUSIC_INFO::EmbeddedArt *art, MUSIC_INFO::CMusicInfoTag& infoTag);
private:
  bool                           Open(const std::string& strFileName, bool readOnly);
  static const std::vector<std::string> GetASFStringList(const TagLib::List<TagLib::ASF::Attribute>& list);
  static const std::vector<std::string> GetID3v2StringList(const TagLib::ID3v2::FrameList& frameList);
  void                           SetFlacArt(TagLib::FLAC::File *flacFile, MUSIC_INFO::EmbeddedArt *art, MUSIC_INFO::CMusicInfoTag &tag);
};

