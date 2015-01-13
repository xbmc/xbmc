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
#include "TagLibVFSStream.h"
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

  const std::vector<std::string> SplitMBID(const std::vector<std::string> &values);
private:
  bool                           Open(const std::string& strFileName, bool readOnly);
  const std::vector<std::string> GetASFStringList(const TagLib::List<TagLib::ASF::Attribute>& list);
  const std::vector<std::string> GetID3v2StringList(const TagLib::ID3v2::FrameList& frameList) const;

  bool                           ParseAPETag(TagLib::APE::Tag *ape, MUSIC_INFO::EmbeddedArt *art, MUSIC_INFO::CMusicInfoTag& tag);
  bool                           ParseASF(TagLib::ASF::Tag *asf, MUSIC_INFO::EmbeddedArt *art, MUSIC_INFO::CMusicInfoTag& tag);
  bool                           ParseID3v1Tag(TagLib::ID3v1::Tag *id3v1, MUSIC_INFO::EmbeddedArt *art, MUSIC_INFO::CMusicInfoTag& tag);
  bool                           ParseID3v2Tag(TagLib::ID3v2::Tag *id3v2, MUSIC_INFO::EmbeddedArt *art, MUSIC_INFO::CMusicInfoTag& tag);
  bool                           ParseXiphComment(TagLib::Ogg::XiphComment *id3v2, MUSIC_INFO::EmbeddedArt *art, MUSIC_INFO::CMusicInfoTag& tag);
  bool                           ParseMP4Tag(TagLib::MP4::Tag *mp4, MUSIC_INFO::EmbeddedArt *art, MUSIC_INFO::CMusicInfoTag& tag);
  bool                           ParseGenericTag(TagLib::Tag *generic, MUSIC_INFO::EmbeddedArt *art, MUSIC_INFO::CMusicInfoTag& tag);
  void                           SetFlacArt(TagLib::FLAC::File *flacFile, MUSIC_INFO::EmbeddedArt *art, MUSIC_INFO::CMusicInfoTag &tag);
  void                           SetArtist(MUSIC_INFO::CMusicInfoTag &tag, const std::vector<std::string> &values);
  void                           SetAlbumArtist(MUSIC_INFO::CMusicInfoTag &tag, const std::vector<std::string> &values);
  void                           SetGenre(MUSIC_INFO::CMusicInfoTag &tag, const std::vector<std::string> &values);
};
