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

#include "TagLoaderTagLib.h"

#include <vector>

#include <taglib/id3v1tag.h>
#include <taglib/id3v2tag.h>
#include <taglib/apetag.h>
#include <taglib/asftag.h>
#include <taglib/xiphcomment.h>
#include <taglib/id3v1genres.h>
#include <taglib/aifffile.h>
#include <taglib/apefile.h>
#include <taglib/asffile.h>
#include <taglib/modfile.h>
#include <taglib/mp4file.h>
#include <taglib/mpegfile.h>
#include <taglib/oggfile.h>
#include <taglib/oggflacfile.h>
#include <taglib/opusfile.h>
#include <taglib/rifffile.h>
#include <taglib/speexfile.h>
#include <taglib/s3mfile.h>
#include <taglib/trueaudiofile.h>
#include <taglib/vorbisfile.h>
#include <taglib/wavfile.h>
#include <taglib/wavpackfile.h>
#include <taglib/xmfile.h>
#include <taglib/flacfile.h>
#include <taglib/itfile.h>
#include <taglib/mpcfile.h>
#include <taglib/id3v2tag.h>
#include <taglib/xiphcomment.h>
#include <taglib/mp4tag.h>

#include <taglib/textidentificationframe.h>
#include <taglib/uniquefileidentifierframe.h>
#include <taglib/popularimeterframe.h>
#include <taglib/commentsframe.h>
#include <taglib/unsynchronizedlyricsframe.h>
#include <taglib/attachedpictureframe.h>

#include <taglib/tstring.h>
#include <taglib/tpropertymap.h>

#include "TagLibVFSStream.h"
#include "MusicInfoTag.h"
#include "ReplayGain.h"
#include "utils/RegExp.h"
#include "utils/URIUtils.h"
#include "utils/log.h"
#include "utils/StringUtils.h"
#include "utils/Base64.h"
#include "settings/AdvancedSettings.h"

using namespace TagLib;
using namespace MUSIC_INFO;

namespace
{
std::vector<std::string> StringListToVectorString(const StringList& stringList)
{
  std::vector<std::string> values;
  for (const auto& it: stringList)
    values.push_back(it.to8Bit(true));
  return values;
}

std::vector<std::string> GetASFStringList(const List<ASF::Attribute>& list)
{
  std::vector<std::string> values;
  for (const auto& at: list)
    values.push_back(at.toString().to8Bit(true));
  return values;
}

std::vector<std::string> GetID3v2StringList(const ID3v2::FrameList& frameList)
{
  auto frame = dynamic_cast<const ID3v2::TextIdentificationFrame *>(frameList.front());
  if (frame)
    return StringListToVectorString(frame->fieldList());
  return std::vector<std::string>();
}

void SetFlacArt(FLAC::File *flacFile, EmbeddedArt *art, CMusicInfoTag &tag)
{
  FLAC::Picture *cover[2] = {};
  auto pictures = flacFile->pictureList();
  for (List<FLAC::Picture *>::ConstIterator i = pictures.begin(); i != pictures.end(); ++i)
  {
    FLAC::Picture *picture = *i;
    if (picture->type() == FLAC::Picture::FrontCover)
      cover[0] = picture;
    else // anything else is taken as second priority
      cover[1] = picture;
  }
  for (unsigned int i = 0; i < 2; i++)
  {
    if (cover[i])
    {
      tag.SetCoverArtInfo(cover[i]->data().size(), cover[i]->mimeType().to8Bit(true));
      if (art)
        art->set(reinterpret_cast<const uint8_t*>(cover[i]->data().data()), cover[i]->data().size(), cover[i]->mimeType().to8Bit(true));
      return; // one is enough
    }
  }
}
}

bool CTagLoaderTagLib::Load(const std::string& strFileName, MUSIC_INFO::CMusicInfoTag& tag, MUSIC_INFO::EmbeddedArt *art /* = NULL */)
{
  return Load(strFileName, tag, "", art);
}


template<>
bool CTagLoaderTagLib::ParseTag(ASF::Tag *asf, EmbeddedArt *art, CMusicInfoTag& tag)
{
  if (!asf)
    return false;

  ReplayGain replayGainInfo;
  tag.SetTitle(asf->title().to8Bit(true));
  const ASF::AttributeListMap& attributeListMap = asf->attributeListMap();
  for (ASF::AttributeListMap::ConstIterator it = attributeListMap.begin(); it != attributeListMap.end(); ++it)
  {
    if (it->first == "Author")
      SetArtist(tag, GetASFStringList(it->second));
    else if (it->first == "WM/AlbumArtist")
      SetAlbumArtist(tag, GetASFStringList(it->second));
    else if (it->first == "WM/AlbumTitle")
      tag.SetAlbum(it->second.front().toString().to8Bit(true));
    else if (it->first == "WM/TrackNumber" ||
             it->first == "WM/Track")
    {
      if (it->second.front().type() == ASF::Attribute::DWordType)
        tag.SetTrackNumber(it->second.front().toUInt());
      else
        tag.SetTrackNumber(atoi(it->second.front().toString().toCString(true)));
    }
    else if (it->first == "WM/PartOfSet")
      tag.SetDiscNumber(atoi(it->second.front().toString().toCString(true)));
    else if (it->first == "WM/Genre")
      SetGenre(tag, GetASFStringList(it->second));
    else if (it->first == "WM/Mood")
      tag.SetMood(it->second.front().toString().to8Bit(true));
    else if (it->first == "WM/Composer")
      AddArtistRole(tag, "Composer", GetASFStringList(it->second));
    else if (it->first == "WM/Conductor")
      AddArtistRole(tag, "Conductor", GetASFStringList(it->second));
    //No ASF/WMA tag from Taglib for "ensemble"
    else if (it->first == "WM/Writer")
      AddArtistRole(tag, "Lyricist", GetASFStringList(it->second));
    else if (it->first == "WM/ModifiedBy")
      AddArtistRole(tag, "Remixer", GetASFStringList(it->second));
    else if (it->first == "WM/Engineer")
      AddArtistRole(tag, "Engineer", GetASFStringList(it->second));
    else if (it->first == "WM/Producer")
      AddArtistRole(tag, "Producer", GetASFStringList(it->second));
    else if (it->first == "WM/DJMixer")
      AddArtistRole(tag, "DJMixer", GetASFStringList(it->second));
    else if (it->first == "WM/Mixer")
      AddArtistRole(tag, "mixer", GetASFStringList(it->second));
    else if (it->first == "WM/Publisher")
    {} // Known unsupported, supress warnings
    else if (it->first == "WM/AlbumArtistSortOrder")
    {} // Known unsupported, supress warnings
    else if (it->first == "WM/ArtistSortOrder")
    {} // Known unsupported, supress warnings
    else if (it->first == "WM/Script")
    {} // Known unsupported, supress warnings
    else if (it->first == "WM/Year")
      tag.SetYear(atoi(it->second.front().toString().toCString(true)));
    else if (it->first == "MusicBrainz/Artist Id")
      tag.SetMusicBrainzArtistID(SplitMBID(GetASFStringList(it->second)));
    else if (it->first == "MusicBrainz/Album Id")
      tag.SetMusicBrainzAlbumID(it->second.front().toString().to8Bit(true));
    else if (it->first == "MusicBrainz/Album Artist")
      SetAlbumArtist(tag, GetASFStringList(it->second));
    else if (it->first == "MusicBrainz/Album Artist Id")
      tag.SetMusicBrainzAlbumArtistID(SplitMBID(GetASFStringList(it->second)));
    else if (it->first == "MusicBrainz/Track Id")
      tag.SetMusicBrainzTrackID(it->second.front().toString().to8Bit(true));
    else if (it->first == "MusicBrainz/Album Status")
    {}
    else if (it->first == "MusicBrainz/Album Type")
    {}
    else if (it->first == "MusicIP/PUID")
    {}
    else if (it->first == "replaygain_track_gain")
      replayGainInfo.ParseGain(ReplayGain::TRACK, it->second.front().toString().toCString(true));
    else if (it->first == "replaygain_album_gain")
      replayGainInfo.ParseGain(ReplayGain::ALBUM, it->second.front().toString().toCString(true));
    else if (it->first == "replaygain_track_peak")
      replayGainInfo.ParsePeak(ReplayGain::TRACK, it->second.front().toString().toCString(true));
    else if (it->first == "replaygain_album_peak")
      replayGainInfo.ParsePeak(ReplayGain::ALBUM, it->second.front().toString().toCString(true));
    else if (it->first == "WM/Picture")
    { // picture
      ASF::Picture pic = it->second.front().toPicture();
      tag.SetCoverArtInfo(pic.picture().size(), pic.mimeType().toCString());
      if (art)
        art->set(reinterpret_cast<const uint8_t *>(pic.picture().data()), pic.picture().size(), pic.mimeType().toCString());
    }
    else if (g_advancedSettings.m_logLevel == LOG_LEVEL_MAX)
      CLog::Log(LOGDEBUG, "unrecognized ASF tag name: %s", it->first.toCString(true));
  }
  // artist may be specified in the ContentDescription block rather than using the 'Author' attribute.
  if (tag.GetArtist().empty())
    tag.SetArtist(asf->artist().toCString(true));

  if (asf->comment() != String::null)
    tag.SetComment(asf->comment().toCString(true));
  tag.SetReplayGain(replayGainInfo);
  tag.SetLoaded(true);
  return true;
}

int CTagLoaderTagLib::POPMtoXBMC(int popm)
{
  // Ratings:
  // FROM: http://thiagoarrais.com/repos/banshee/src/Core/Banshee.Core/Banshee.Streaming/StreamRatingTagger.cs
  // The following schemes are used by the other POPM-compatible players:
  // WMP/Vista: "Windows Media Player 9 Series" ratings:
  //   1 = 1, 2 = 64, 3=128, 4=196 (not 192), 5=255
  // MediaMonkey: "no@email" ratings:
  //   0.5=26, 1=51, 1.5=76, 2=102, 2.5=128,
  //   3=153, 3.5=178, 4=204, 4.5=230, 5=255
  // Quod Libet: "quodlibet@lists.sacredchao.net" ratings
  //   (but that email can be changed):
  //   arbitrary scale from 0-255
  if (popm == 0) return 0;
  if (popm < 0x19) return 1;
  if (popm < 0x32) return 2;
  if (popm < 0x4b) return 3;
  if (popm < 0x64) return 4;
  if (popm < 0x7d) return 5;
  if (popm < 0x96) return 6;
  if (popm < 0xaf) return 7;
  if (popm < 0xc8) return 8;
  if (popm < 0xe1) return 9;
  else return 10;
}

template<>
bool CTagLoaderTagLib::ParseTag(ID3v1::Tag *id3v1, EmbeddedArt *art, CMusicInfoTag& tag)
{
  if (!id3v1) return false;
  tag.SetTitle(id3v1->title().to8Bit(true));
  tag.SetArtist(id3v1->artist().to8Bit(true));
  tag.SetAlbum(id3v1->album().to8Bit(true));
  tag.SetComment(id3v1->comment().to8Bit(true));
  tag.SetGenre(id3v1->genre().to8Bit(true));
  tag.SetYear(id3v1->year());
  tag.SetTrackNumber(id3v1->track());
  return true;
}

template<>
bool CTagLoaderTagLib::ParseTag(ID3v2::Tag *id3v2, MUSIC_INFO::EmbeddedArt *art, MUSIC_INFO::CMusicInfoTag& tag)
{
  if (!id3v2) return false;
  ReplayGain replayGainInfo;

  ID3v2::AttachedPictureFrame *pictures[3] = {};
  const ID3v2::FrameListMap& frameListMap = id3v2->frameListMap();
  for (ID3v2::FrameListMap::ConstIterator it = frameListMap.begin(); it != frameListMap.end(); ++it)
  {
    // It is possible that the taglist is empty. In that case no useable values can be extracted.
    // and we should skip the tag.
    if (it->second.isEmpty()) continue;

    if      (it->first == "TPE1")   SetArtist(tag, GetID3v2StringList(it->second));
    else if (it->first == "TALB")   tag.SetAlbum(it->second.front()->toString().to8Bit(true));
    else if (it->first == "TPE2")   SetAlbumArtist(tag, GetID3v2StringList(it->second));
    else if (it->first == "TIT2")   tag.SetTitle(it->second.front()->toString().to8Bit(true));
    else if (it->first == "TCON")   SetGenre(tag, GetID3v2StringList(it->second));
    else if (it->first == "TRCK")   tag.SetTrackNumber(strtol(it->second.front()->toString().toCString(true), nullptr, 10));
    else if (it->first == "TPOS")   tag.SetDiscNumber(strtol(it->second.front()->toString().toCString(true), nullptr, 10));
    else if (it->first == "TYER")   tag.SetYear(strtol(it->second.front()->toString().toCString(true), nullptr, 10));
    else if (it->first == "TCMP")   tag.SetCompilation((strtol(it->second.front()->toString().toCString(true), nullptr, 10) == 0) ? false : true);
    else if (it->first == "TENC")   {} // EncodedBy
    else if (it->first == "TCOM")   AddArtistRole(tag, "Composer", GetID3v2StringList(it->second));
    else if (it->first == "TPE3")   AddArtistRole(tag, "Conductor", GetID3v2StringList(it->second));
    else if (it->first == "TEXT")   AddArtistRole(tag, "Lyricist", GetID3v2StringList(it->second));
    else if (it->first == "TPE4")   AddArtistRole(tag, "Remixer", GetID3v2StringList(it->second));
    else if (it->first == "TPUB")   {} // Publisher. Known unsupported, supress warnings
    else if (it->first == "TCOP")   {} // Copyright message
    else if (it->first == "TDRC")   tag.SetYear(strtol(it->second.front()->toString().toCString(true), nullptr, 10));
    else if (it->first == "TDRL")   tag.SetYear(strtol(it->second.front()->toString().toCString(true), nullptr, 10));
    else if (it->first == "TDTG")   {} // Tagging time
    else if (it->first == "TLAN")   {} // Languages
    else if (it->first == "TMOO")   tag.SetMood(it->second.front()->toString().to8Bit(true));
    else if (it->first == "USLT")
      // Loop through any lyrics frames. Could there be multiple frames, how to choose?
      for (ID3v2::FrameList::ConstIterator lt = it->second.begin(); lt != it->second.end(); ++lt)
      {
        auto lyricsFrame = dynamic_cast<ID3v2::UnsynchronizedLyricsFrame *> (*lt);
        if (lyricsFrame)
          tag.SetLyrics(lyricsFrame->text().to8Bit(true));
      }
    else if (it->first == "COMM")
      // Loop through and look for the main (no description) comment
      for (ID3v2::FrameList::ConstIterator ct = it->second.begin(); ct != it->second.end(); ++ct)
      {
        ID3v2::CommentsFrame *commentsFrame = dynamic_cast<ID3v2::CommentsFrame *> (*ct);
        if (commentsFrame && commentsFrame->description().isEmpty())
          tag.SetComment(commentsFrame->text().to8Bit(true));
      }
    else if (it->first == "TXXX")
      // Loop through and process the UserTextIdentificationFrames
      for (ID3v2::FrameList::ConstIterator ut = it->second.begin(); ut != it->second.end(); ++ut)
      {
        ID3v2::UserTextIdentificationFrame *frame = dynamic_cast<ID3v2::UserTextIdentificationFrame *> (*ut);
        if (!frame) continue;

        // First field is the same as the description
        StringList stringList = frame->fieldList();
        if (stringList.size() == 1) continue;
        stringList.erase(stringList.begin());
        String desc = frame->description().upper();
        if      (desc == "MUSICBRAINZ ARTIST ID")
          tag.SetMusicBrainzArtistID(SplitMBID(StringListToVectorString(stringList)));
        else if (desc == "MUSICBRAINZ ALBUM ID")
          tag.SetMusicBrainzAlbumID(stringList.front().to8Bit(true));
        else if (desc == "MUSICBRAINZ ALBUM ARTIST ID")
          tag.SetMusicBrainzAlbumArtistID(SplitMBID(StringListToVectorString(stringList)));
        else if (desc == "MUSICBRAINZ ALBUM ARTIST")
          SetAlbumArtist(tag, StringListToVectorString(stringList));
        else if (desc == "REPLAYGAIN_TRACK_GAIN")
          replayGainInfo.ParseGain(ReplayGain::TRACK, stringList.front().toCString(true));
        else if (desc == "REPLAYGAIN_ALBUM_GAIN")
          replayGainInfo.ParseGain(ReplayGain::ALBUM, stringList.front().toCString(true));
        else if (desc == "REPLAYGAIN_TRACK_PEAK")
          replayGainInfo.ParsePeak(ReplayGain::TRACK, stringList.front().toCString(true));
        else if (desc == "REPLAYGAIN_ALBUM_PEAK")
          replayGainInfo.ParsePeak(ReplayGain::ALBUM, stringList.front().toCString(true));
        else if (desc == "ALBUMARTIST" || desc == "ALBUM ARTIST")
          SetAlbumArtist(tag, StringListToVectorString(stringList));
        else if (desc == "ARTISTS")
          SetArtistHints(tag, StringListToVectorString(stringList));
        else if (desc == "ALBUMARTISTS" || desc == "ALBUM ARTISTS")
          SetAlbumArtistHints(tag, StringListToVectorString(stringList));
        else if (desc == "MOOD")
          tag.SetMood(stringList.front().to8Bit(true));
        else if (g_advancedSettings.m_logLevel == LOG_LEVEL_MAX)
          CLog::Log(LOGDEBUG, "unrecognized user text tag detected: TXXX:%s", frame->description().toCString(true));
      }
    else if (it->first == "TIPL")
      // Loop through and process the involved people list 
      // For example Arranger, Engineer, Producer, DJMixer or Mixer
      // In fieldlist every odd field is a function, and every even is an artist or a comma delimited list of artists.
      for (ID3v2::FrameList::ConstIterator ip = it->second.begin(); ip != it->second.end(); ++ip)
      {
        auto tiplframe = dynamic_cast<ID3v2::TextIdentificationFrame*> (*ip);
        if (tiplframe)
          AddArtistRole(tag, StringListToVectorString(tiplframe->fieldList()));
      }
    else if (it->first == "TMCL")
      // Loop through and process the musicain credits list
      // It is a mapping between the instrument and the person that played it, but also includes "orchestra" or "soloist".
      // In fieldlist every odd field is an instrument, and every even is an artist or a comma delimited list of artists.
      for (ID3v2::FrameList::ConstIterator ip = it->second.begin(); ip != it->second.end(); ++ip)
      {
        auto tiplframe = dynamic_cast<ID3v2::TextIdentificationFrame*> (*ip);
        if (tiplframe)
          AddArtistRole(tag, StringListToVectorString(tiplframe->fieldList()));
      }
    else if (it->first == "UFID")
      // Loop through any UFID frames and set them
      for (ID3v2::FrameList::ConstIterator ut = it->second.begin(); ut != it->second.end(); ++ut)
      {
        auto ufid = reinterpret_cast<ID3v2::UniqueFileIdentifierFrame*> (*ut);
        if (ufid->owner() == "http://musicbrainz.org")
        {
          // MusicBrainz pads with a \0, but the spec requires binary, be cautious
          char cUfid[64];
          int max_size = std::min(static_cast<int>(ufid->identifier().size()), 63);
          strncpy(cUfid, ufid->identifier().data(), max_size);
          cUfid[max_size] = '\0';
          tag.SetMusicBrainzTrackID(cUfid);
        }
      }
    else if (it->first == "APIC")
      // Loop through all pictures and store the frame pointers for the picture types we want
      for (ID3v2::FrameList::ConstIterator pi = it->second.begin(); pi != it->second.end(); ++pi)
      {
        auto pictureFrame = dynamic_cast<ID3v2::AttachedPictureFrame *> (*pi);
        if (!pictureFrame) continue;

        if      (pictureFrame->type() == ID3v2::AttachedPictureFrame::FrontCover) pictures[0] = pictureFrame;
        else if (pictureFrame->type() == ID3v2::AttachedPictureFrame::Other)      pictures[1] = pictureFrame;
        else if (pi == it->second.begin())                                        pictures[2] = pictureFrame;
      }
    else if (it->first == "POPM")
      // Loop through and process ratings
      for (ID3v2::FrameList::ConstIterator ct = it->second.begin(); ct != it->second.end(); ++ct)
      {
        auto popFrame = dynamic_cast<ID3v2::PopularimeterFrame *> (*ct);
        if (!popFrame) continue;

        // @xbmc.org ratings trump others (of course)
        if      (popFrame->email() == "ratings@xbmc.org")
          tag.SetUserrating(popFrame->rating() / 51); //! @todo wtf? Why 51 find some explanation, somewhere...
        else if (tag.GetUserrating() == 0)
        {
          if (popFrame->email() != "Windows Media Player 9 Series" &&
              popFrame->email() != "Banshee" &&
              popFrame->email() != "no@email" &&
              popFrame->email() != "quodlibet@lists.sacredchao.net" &&
              popFrame->email() != "rating@winamp.com")
            CLog::Log(LOGDEBUG, "unrecognized ratings schema detected: %s", popFrame->email().toCString(true));
          tag.SetUserrating(POPMtoXBMC(popFrame->rating()));
        }
      }
    else if (g_advancedSettings.m_logLevel == LOG_LEVEL_MAX)
      CLog::Log(LOGDEBUG, "unrecognized ID3 frame detected: %c%c%c%c", it->first[0], it->first[1], it->first[2], it->first[3]);
  } // for

  // Process the extracted picture frames; 0 = CoverArt, 1 = Other, 2 = First Found picture
  for (int i = 0; i < 3; ++i)
    if (pictures[i])
    {
      std::string  mime =            pictures[i]->mimeType().to8Bit(true);
      TagLib::uint size =            pictures[i]->picture().size();
      tag.SetCoverArtInfo(size, mime);
      if (art)
        art->set(reinterpret_cast<const uint8_t*>(pictures[i]->picture().data()), size, mime);

      // Stop after we find the first picture for now.
      break;
    }


  if (id3v2->comment() != String::null)
    tag.SetComment(id3v2->comment().toCString(true));

  tag.SetReplayGain(replayGainInfo);
  return true;
}

template<>
bool CTagLoaderTagLib::ParseTag(APE::Tag *ape, EmbeddedArt *art, CMusicInfoTag& tag)
{
  if (!ape)
    return false;

  ReplayGain replayGainInfo;
  const APE::ItemListMap itemListMap = ape->itemListMap();
  for (APE::ItemListMap::ConstIterator it = itemListMap.begin(); it != itemListMap.end(); ++it)
  {
    if (it->first == "ARTIST")
      SetArtist(tag, StringListToVectorString(it->second.toStringList()));
    else if (it->first == "ARTISTS")
      SetArtistHints(tag, StringListToVectorString(it->second.toStringList()));
    else if (it->first == "ALBUMARTIST" || it->first == "ALBUM ARTIST")
      SetAlbumArtist(tag, StringListToVectorString(it->second.toStringList()));
    else if (it->first == "ALBUMARTISTS" || it->first == "ALBUM ARTISTS")
      SetAlbumArtistHints(tag, StringListToVectorString(it->second.toStringList()));
    else if (it->first == "ALBUM")
      tag.SetAlbum(it->second.toString().to8Bit(true));
    else if (it->first == "TITLE")
      tag.SetTitle(it->second.toString().to8Bit(true));
    else if (it->first == "TRACKNUMBER" || it->first == "TRACK")
      tag.SetTrackNumber(it->second.toString().toInt());
    else if (it->first == "DISCNUMBER" || it->first == "DISC")
      tag.SetDiscNumber(it->second.toString().toInt());
    else if (it->first == "YEAR")
      tag.SetYear(it->second.toString().toInt());
    else if (it->first == "GENRE")
      SetGenre(tag, StringListToVectorString(it->second.toStringList()));
    else if (it->first == "MOOD")
      tag.SetMood(it->second.toString().to8Bit(true));
    else if (it->first == "COMMENT")
      tag.SetComment(it->second.toString().to8Bit(true));
    else if (it->first == "CUESHEET")
      tag.SetCueSheet(it->second.toString().to8Bit(true));
    else if (it->first == "ENCODEDBY")
    {}
    else if (it->first == "COMPOSER")
      AddArtistRole(tag, "Composer", StringListToVectorString(it->second.toStringList()));
    else if (it->first == "CONDUCTOR")
      AddArtistRole(tag, "Conductor", StringListToVectorString(it->second.toStringList()));
    else if ((it->first == "BAND") || (it->first == "ENSEMBLE"))
      AddArtistRole(tag, "Orchestra", StringListToVectorString(it->second.toStringList()));
    else if (it->first == "LYRICIST")
      AddArtistRole(tag, "Lyricist", StringListToVectorString(it->second.toStringList()));
    else if (it->first == "MIXARTIST")   
      AddArtistRole(tag, "Remixer", StringListToVectorString(it->second.toStringList()));
    else if (it->first == "ARRANGER") 
      AddArtistRole(tag, "Arranger", StringListToVectorString(it->second.toStringList()));
    else if (it->first == "ENGINEER") 
      AddArtistRole(tag, "Engineer", StringListToVectorString(it->second.toStringList()));
    else if (it->first == "PRODUCER") 
      AddArtistRole(tag, "Producer", StringListToVectorString(it->second.toStringList()));
    else if (it->first == "DJMIXER") 
      AddArtistRole(tag, "DJMixer", StringListToVectorString(it->second.toStringList()));
    else if (it->first == "MIXER")
      AddArtistRole(tag, "Mixer", StringListToVectorString(it->second.toStringList()));
    else if (it->first == "PERFORMER")
      // Picard uses PERFORMER tag as musician credits list formatted "name (instrument)"
      AddArtistInstrument(tag, StringListToVectorString(it->second.toStringList()));
    else if (it->first == "LABEL")   
    {} // Publisher. Known unsupported, supress warnings
    else if (it->first == "COMPILATION")
      tag.SetCompilation(it->second.toString().toInt() == 1);
    else if (it->first == "LYRICS")
      tag.SetLyrics(it->second.toString().to8Bit(true));
    else if (it->first == "REPLAYGAIN_TRACK_GAIN")
      replayGainInfo.ParseGain(ReplayGain::TRACK, it->second.toString().toCString(true));
    else if (it->first == "REPLAYGAIN_ALBUM_GAIN")
      replayGainInfo.ParseGain(ReplayGain::ALBUM, it->second.toString().toCString(true));
    else if (it->first == "REPLAYGAIN_TRACK_PEAK")
      replayGainInfo.ParsePeak(ReplayGain::TRACK, it->second.toString().toCString(true));
    else if (it->first == "REPLAYGAIN_ALBUM_PEAK")
      replayGainInfo.ParsePeak(ReplayGain::ALBUM, it->second.toString().toCString(true));
    else if (it->first == "MUSICBRAINZ_ARTISTID")
      tag.SetMusicBrainzArtistID(SplitMBID(StringListToVectorString(it->second.toStringList())));
    else if (it->first == "MUSICBRAINZ_ALBUMARTISTID")
      tag.SetMusicBrainzAlbumArtistID(SplitMBID(StringListToVectorString(it->second.toStringList())));
    else if (it->first == "MUSICBRAINZ_ALBUMARTIST")
      SetAlbumArtist(tag, StringListToVectorString(it->second.toStringList()));
    else if (it->first == "MUSICBRAINZ_ALBUMID")
      tag.SetMusicBrainzAlbumID(it->second.toString().to8Bit(true));
    else if (it->first == "MUSICBRAINZ_TRACKID")
      tag.SetMusicBrainzTrackID(it->second.toString().to8Bit(true));
    else if (g_advancedSettings.m_logLevel == LOG_LEVEL_MAX)
      CLog::Log(LOGDEBUG, "unrecognized APE tag: %s", it->first.toCString(true));
  }

  tag.SetReplayGain(replayGainInfo);
  return true;
}

template<>
bool CTagLoaderTagLib::ParseTag(Ogg::XiphComment *xiph, EmbeddedArt *art, CMusicInfoTag& tag)
{
  if (!xiph)
    return false;

  FLAC::Picture pictures[3];
  ReplayGain replayGainInfo;

  const Ogg::FieldListMap& fieldListMap = xiph->fieldListMap();
  for (Ogg::FieldListMap::ConstIterator it = fieldListMap.begin(); it != fieldListMap.end(); ++it)
  {
    if (it->first == "ARTIST")
      SetArtist(tag, StringListToVectorString(it->second));
    else if (it->first == "ARTISTS")
      SetArtistHints(tag, StringListToVectorString(it->second));
    else if (it->first == "ALBUMARTIST" || it->first == "ALBUM ARTIST")
      SetAlbumArtist(tag, StringListToVectorString(it->second));
    else if (it->first == "ALBUMARTISTS" || it->first == "ALBUM ARTISTS")
      SetAlbumArtistHints(tag, StringListToVectorString(it->second));
    else if (it->first == "ALBUM")
      tag.SetAlbum(it->second.front().to8Bit(true));
    else if (it->first == "TITLE")
      tag.SetTitle(it->second.front().to8Bit(true));
    else if (it->first == "TRACKNUMBER")
      tag.SetTrackNumber(it->second.front().toInt());
    else if (it->first == "DISCNUMBER")
      tag.SetDiscNumber(it->second.front().toInt());
    else if (it->first == "YEAR")
      tag.SetYear(it->second.front().toInt());
    else if (it->first == "DATE")
      tag.SetYear(it->second.front().toInt());
    else if (it->first == "GENRE")
      SetGenre(tag, StringListToVectorString(it->second));
    else if (it->first == "MOOD")
      tag.SetMood(it->second.front().to8Bit(true));
    else if (it->first == "COMMENT")
      tag.SetComment(it->second.front().to8Bit(true));
    else if (it->first == "CUESHEET")
      tag.SetCueSheet(it->second.front().to8Bit(true));
    else if (it->first == "ENCODEDBY")
    {} // Known but unsupported, supress warnings
    else if (it->first == "COMPOSER")
      AddArtistRole(tag, "Composer", StringListToVectorString(it->second));
    else if (it->first == "CONDUCTOR")
      AddArtistRole(tag, "Conductor", StringListToVectorString(it->second));
    else if ((it->first == "BAND") || (it->first == "ENSEMBLE"))
      AddArtistRole(tag, "Orchestra", StringListToVectorString(it->second));
    else if (it->first == "LYRICIST")
      AddArtistRole(tag, "Lyricist", StringListToVectorString(it->second));
    else if (it->first == "MIXARTIST")
      AddArtistRole(tag, "Remixer", StringListToVectorString(it->second));
    else if (it->first == "ARRANGER")
      AddArtistRole(tag, "Arranger", StringListToVectorString(it->second));
    else if (it->first == "ENGINEER")
      AddArtistRole(tag, "Engineer", StringListToVectorString(it->second));
    else if (it->first == "PRODUCER")
      AddArtistRole(tag, "Producer", StringListToVectorString(it->second));
    else if (it->first == "DJMIXER")
      AddArtistRole(tag, "DJMixer", StringListToVectorString(it->second));
    else if (it->first == "MIXER")
      AddArtistRole(tag, "Mixer", StringListToVectorString(it->second));
    else if (it->first == "PERFORMER")
      // Picard uses PERFORMER tag as musician credits list formatted "name (instrument)"
      AddArtistInstrument(tag, StringListToVectorString(it->second));
    else if (it->first == "LABEL")
    {} // Publisher. Known unsupported, supress warnings
    else if (it->first == "COMPILATION")
      tag.SetCompilation(it->second.front().toInt() == 1);
    else if (it->first == "LYRICS")
      tag.SetLyrics(it->second.front().to8Bit(true));
    else if (it->first == "REPLAYGAIN_TRACK_GAIN")
      replayGainInfo.ParseGain(ReplayGain::TRACK, it->second.front().toCString(true));
    else if (it->first == "REPLAYGAIN_ALBUM_GAIN")
      replayGainInfo.ParseGain(ReplayGain::ALBUM, it->second.front().toCString(true));
    else if (it->first == "REPLAYGAIN_TRACK_PEAK")
      replayGainInfo.ParsePeak(ReplayGain::TRACK, it->second.front().toCString(true));
    else if (it->first == "REPLAYGAIN_ALBUM_PEAK")
      replayGainInfo.ParsePeak(ReplayGain::ALBUM, it->second.front().toCString(true));
    else if (it->first == "MUSICBRAINZ_ARTISTID")
      tag.SetMusicBrainzArtistID(SplitMBID(StringListToVectorString(it->second)));
    else if (it->first == "MUSICBRAINZ_ALBUMARTISTID")
      tag.SetMusicBrainzAlbumArtistID(SplitMBID(StringListToVectorString(it->second)));
    else if (it->first == "MUSICBRAINZ_ALBUMARTIST")
      SetAlbumArtist(tag, StringListToVectorString(it->second));
    else if (it->first == "MUSICBRAINZ_ALBUMID")
      tag.SetMusicBrainzAlbumID(it->second.front().to8Bit(true));
    else if (it->first == "MUSICBRAINZ_TRACKID")
      tag.SetMusicBrainzTrackID(it->second.front().to8Bit(true));
    else if (it->first == "RATING")
    {
      // Vorbis ratings are a mess because the standard forgot to mention anything about them.
      // If you want to see how emotive the issue is and the varying standards, check here:
      // http://forums.winamp.com/showthread.php?t=324512
      // The most common standard in that thread seems to be a 0-100 scale for 1-5 stars.
      // So, that's what we'll support for now.
      int iUserrating = it->second.front().toInt();
      if (iUserrating > 0 && iUserrating <= 100)
        tag.SetUserrating((iUserrating / 10));
    }
#if TAGLIB_MAJOR_VERSION <= 1 && TAGLIB_MINOR_VERSION < 11
    else if (it->first == "METADATA_BLOCK_PICTURE")
    {
      const char* b64 = it->second.front().toCString();
      std::string decoded_block = Base64::Decode(b64, it->second.front().size());
      ByteVector bv(decoded_block.data(), decoded_block.size());
      TagLib::FLAC::Picture* pictureFrame = new TagLib::FLAC::Picture(bv);

      if      (pictureFrame->type() == FLAC::Picture::FrontCover) pictures[0].parse(bv);
      else if (pictureFrame->type() == FLAC::Picture::Other)      pictures[1].parse(bv);

      delete pictureFrame;
    }
    else if (it->first == "COVERART")
    {
      const char* b64 = it->second.front().toCString();
      std::string decoded_block = Base64::Decode(b64, it->second.front().size());
      ByteVector bv(decoded_block.data(), decoded_block.size());
      pictures[2].setData(bv);
      // Assume jpeg
      if (pictures[2].mimeType().isEmpty())
        pictures[2].setMimeType("image/jpeg");
    }
    else if (it->first == "COVERARTMIME")
    {
      pictures[2].setMimeType(it->second.front());
    }
#endif
    else if (g_advancedSettings.m_logLevel == LOG_LEVEL_MAX)
      CLog::Log(LOGDEBUG, "unrecognized XipComment name: %s", it->first.toCString(true));
  }

#if TAGLIB_MAJOR_VERSION <= 1 && TAGLIB_MINOR_VERSION < 11
  // Process the extracted picture frames; 0 = CoverArt, 1 = Other, 2 = COVERART/COVERARTMIME
  for (int i = 0; i < 3; ++i)
    if (pictures[i].data().size())
    {
      std::string mime = pictures[i].mimeType().toCString();
      if (mime.compare(0, 6, "image/") != 0)
        continue;
      TagLib::uint size =            pictures[i].data().size();
      tag.SetCoverArtInfo(size, mime);
      if (art)
        art->set(reinterpret_cast<const uint8_t*>(pictures[i].data().data()), size, mime);

      break;
    }
#else
  auto pictureList = xiph->pictureList();
  FLAC::Picture *cover[2] = {};

  for (auto i: pictureList)
  {
    FLAC::Picture *picture = i;
    if (picture->type() == FLAC::Picture::FrontCover)
      cover[0] = picture;
    else // anything else is taken as second priority
      cover[1] = picture;
  }
  for (unsigned int i = 0; i < 2; i++)
  {
    if (cover[i])
    {
      tag.SetCoverArtInfo(cover[i]->data().size(), cover[i]->mimeType().to8Bit(true));
      if (art)
        art->set(reinterpret_cast<const uint8_t*>(cover[i]->data().data()), cover[i]->data().size(), cover[i]->mimeType().to8Bit(true));
      break; // one is enough
    }
  }
#endif

  if (xiph->comment() != String::null)
    tag.SetComment(xiph->comment().toCString(true));

  tag.SetReplayGain(replayGainInfo);
  return true;
}

template<>
bool CTagLoaderTagLib::ParseTag(MP4::Tag *mp4, EmbeddedArt *art, CMusicInfoTag& tag)
{
  if (!mp4)
    return false;

  ReplayGain replayGainInfo;
  MP4::ItemListMap& itemListMap = mp4->itemListMap();
  for (MP4::ItemListMap::ConstIterator it = itemListMap.begin(); it != itemListMap.end(); ++it)
  {
    if (it->first == "\251nam")
      tag.SetTitle(it->second.toStringList().front().to8Bit(true));
    else if (it->first == "\251ART")
      SetArtist(tag, StringListToVectorString(it->second.toStringList()));
    else if (it->first == "----:com.apple.iTunes:ARTISTS")
      SetArtistHints(tag, StringListToVectorString(it->second.toStringList()));
    else if (it->first == "\251alb")
      tag.SetAlbum(it->second.toStringList().front().to8Bit(true));
    else if (it->first == "aART")
      SetAlbumArtist(tag, StringListToVectorString(it->second.toStringList()));
    else if (it->first == "----:com.apple.iTunes:ALBUMARTISTS")
      SetAlbumArtistHints(tag, StringListToVectorString(it->second.toStringList()));
    else if (it->first == "\251gen")
      SetGenre(tag, StringListToVectorString(it->second.toStringList()));
    else if (it->first == "----:com.apple.iTunes:MOOD")
      tag.SetMood(it->second.toStringList().front().to8Bit(true));
    else if (it->first == "\251cmt")
      tag.SetComment(it->second.toStringList().front().to8Bit(true));
    else if (it->first == "\251wrt")
      AddArtistRole(tag, "Composer", StringListToVectorString(it->second.toStringList()));
    else if (it->first == "----:com.apple.iTunes:CONDUCTOR")
      AddArtistRole(tag, "Conductor", StringListToVectorString(it->second.toStringList()));
    //No MP4 standard tag for "ensemble"
    else if (it->first == "----:com.apple.iTunes:LYRICIST")
      AddArtistRole(tag, "Lyricist", StringListToVectorString(it->second.toStringList()));
    else if (it->first == "----:com.apple.iTunes:REMIXER")
      AddArtistRole(tag, "Remixer", StringListToVectorString(it->second.toStringList()));
    else if (it->first == "----:com.apple.iTunes:ENGINEER")
      AddArtistRole(tag, "Engineer", StringListToVectorString(it->second.toStringList()));
    else if (it->first == "----:com.apple.iTunes:PRODUCER")
      AddArtistRole(tag, "Producer", StringListToVectorString(it->second.toStringList()));
    else if (it->first == "----:com.apple.iTunes:DJMIXER")
      AddArtistRole(tag, "DJMixer", StringListToVectorString(it->second.toStringList()));
    else if (it->first == "----:com.apple.iTunes:MIXER")
      AddArtistRole(tag, "Mixer", StringListToVectorString(it->second.toStringList()));
    //No MP4 standard tag for musician credits
    else if (it->first == "----:com.apple.iTunes:LABEL")
    {} // Publisher. Known unsupported, supress warnings
    else if (it->first == "cpil")
      tag.SetCompilation(it->second.toBool());
    else if (it->first == "trkn")
      tag.SetTrackNumber(it->second.toIntPair().first);
    else if (it->first == "disk")
      tag.SetDiscNumber(it->second.toIntPair().first);
    else if (it->first == "\251day")
      tag.SetYear(it->second.toStringList().front().toInt());
    else if (it->first == "----:com.apple.iTunes:replaygain_track_gain")
      replayGainInfo.ParseGain(ReplayGain::TRACK, it->second.toStringList().front().toCString());
    else if (it->first == "----:com.apple.iTunes:replaygain_album_gain")
      replayGainInfo.ParseGain(ReplayGain::ALBUM, it->second.toStringList().front().toCString());
    else if (it->first == "----:com.apple.iTunes:replaygain_track_peak")
      replayGainInfo.ParsePeak(ReplayGain::TRACK, it->second.toStringList().front().toCString());
    else if (it->first == "----:com.apple.iTunes:replaygain_album_peak")
      replayGainInfo.ParsePeak(ReplayGain::ALBUM, it->second.toStringList().front().toCString());
    else if (it->first == "----:com.apple.iTunes:MusicBrainz Artist Id")
      tag.SetMusicBrainzArtistID(SplitMBID(StringListToVectorString(it->second.toStringList())));
    else if (it->first == "----:com.apple.iTunes:MusicBrainz Album Artist Id")
      tag.SetMusicBrainzAlbumArtistID(SplitMBID(StringListToVectorString(it->second.toStringList())));
    else if (it->first == "----:com.apple.iTunes:MusicBrainz Album Artist")
      SetAlbumArtist(tag, StringListToVectorString(it->second.toStringList()));
    else if (it->first == "----:com.apple.iTunes:MusicBrainz Album Id")
      tag.SetMusicBrainzAlbumID(it->second.toStringList().front().to8Bit(true));
    else if (it->first == "----:com.apple.iTunes:MusicBrainz Track Id")
      tag.SetMusicBrainzTrackID(it->second.toStringList().front().to8Bit(true));
    else if (it->first == "covr")
    {
      MP4::CoverArtList coverArtList = it->second.toCoverArtList();
      for (MP4::CoverArtList::ConstIterator pt = coverArtList.begin(); pt != coverArtList.end(); ++pt)
      {
        std::string mime;
        switch (pt->format())
        {
          case MP4::CoverArt::PNG:
            mime = "image/png";
            break;
          case MP4::CoverArt::JPEG:
            mime = "image/jpeg";
            break;
          default:
            break;
        }
        if (mime.empty())
          continue;
        tag.SetCoverArtInfo(pt->data().size(), mime);
        if (art)
          art->set(reinterpret_cast<const uint8_t *>(pt->data().data()), pt->data().size(), mime);
        break; // one is enough
      }
    }
  }

  if (mp4->comment() != String::null)
    tag.SetComment(mp4->comment().toCString(true));

  tag.SetReplayGain(replayGainInfo);
  return true;
}

template<>
bool CTagLoaderTagLib::ParseTag(Tag *generic, EmbeddedArt *art, CMusicInfoTag& tag)
{
  if (!generic)
    return false;

  PropertyMap properties = generic->properties();
  for (PropertyMap::ConstIterator it = properties.begin(); it != properties.end(); ++it)
  {
    if (it->first == "ARTIST")
      SetArtist(tag, StringListToVectorString(it->second));
    else if (it->first == "ALBUM")
      tag.SetAlbum(it->second.front().to8Bit(true));
    else if (it->first == "TITLE")
      tag.SetTitle(it->second.front().to8Bit(true));
    else if (it->first == "TRACKNUMBER")
      tag.SetTrackNumber(it->second.front().toInt());
    else if (it->first == "YEAR")
      tag.SetYear(it->second.front().toInt());
    else if (it->first == "GENRE")
      SetGenre(tag, StringListToVectorString(it->second));
    else if (it->first == "COMMENT")
      tag.SetComment(it->second.front().to8Bit(true));
  }

  return true;
}




void CTagLoaderTagLib::SetArtist(CMusicInfoTag &tag, const std::vector<std::string> &values)
{
  if (values.size() == 1)
    tag.SetArtist(values[0]);
  else
    // Fill both artist vector and artist desc from tag value.
    // Note desc may not be empty as it could have been set by previous parsing of ID3v2 before APE
    tag.SetArtist(values, true); 
}

void CTagLoaderTagLib::SetArtistHints(CMusicInfoTag &tag, const std::vector<std::string> &values)
{
  if (values.size() == 1)
    tag.SetMusicBrainzArtistHints(StringUtils::Split(values[0], g_advancedSettings.m_musicItemSeparator));
  else
    tag.SetMusicBrainzArtistHints(values);
}

std::vector<std::string> CTagLoaderTagLib::SplitMBID(const std::vector<std::string> &values)
{
  if (values.empty() || values.size() > 1)
    return values;

  // Picard, and other taggers use a heap of different separators.  We use a regexp to detect
  // MBIDs to make sure we hit them all...
  std::vector<std::string> ret;
  std::string value = values[0];
  StringUtils::ToLower(value);
  CRegExp reg;
  if (reg.RegComp("([[:xdigit:]]{8}-[[:xdigit:]]{4}-[[:xdigit:]]{4}-[[:xdigit:]]{4}-[[:xdigit:]]{12})"))
  {
    int pos = -1;
    while ((pos = reg.RegFind(value, pos+1)) > -1)
      ret.push_back(reg.GetMatch(1));
  }
  return ret;
}

void CTagLoaderTagLib::SetAlbumArtist(CMusicInfoTag &tag, const std::vector<std::string> &values)
{
  if (values.size() == 1)
    tag.SetAlbumArtist(values[0]);
  else
    // Fill both artist vector and artist desc from tag value.
    // Note desc may not be empty as it could have been set by previous parsing of ID3v2 before APE
    tag.SetAlbumArtist(values, true);
}

void CTagLoaderTagLib::SetAlbumArtistHints(CMusicInfoTag &tag, const std::vector<std::string> &values)
{
  if (values.size() == 1)
    tag.SetMusicBrainzAlbumArtistHints(StringUtils::Split(values[0], g_advancedSettings.m_musicItemSeparator));
  else
    tag.SetMusicBrainzAlbumArtistHints(values);
}

void CTagLoaderTagLib::SetGenre(CMusicInfoTag &tag, const std::vector<std::string> &values)
{
  /*
   TagLib doesn't resolve ID3v1 genre numbers in the case were only
   a number is specified, thus this workaround.
   */
  std::vector<std::string> genres;
  for (std::vector<std::string>::const_iterator i = values.begin(); i != values.end(); ++i)
  {
    std::string genre = *i;
    if (StringUtils::IsNaturalNumber(genre))
    {
      int number = strtol(i->c_str(), nullptr, 10);
      if (number >= 0 && number < 256)
        genre = ID3v1::genre(number).to8Bit(true);
    }
    genres.push_back(genre);
  }
  if (genres.size() == 1)
    tag.SetGenre(genres[0]);
  else
    tag.SetGenre(genres);
}

void CTagLoaderTagLib::AddArtistRole(CMusicInfoTag &tag, const std::string& strRole, const std::vector<std::string> &values)
{
  if (values.size() == 1)
    tag.AddArtistRole(strRole, values[0]);
  else
    tag.AddArtistRole(strRole, values);
}

void CTagLoaderTagLib::AddArtistRole(CMusicInfoTag &tag, const std::vector<std::string> &values)
{
  // Values contains role, name pairs (as in ID3 standard for TIPL or TMCL tags)
  // Every odd entry is a function (e.g. Producer, Arranger etc.) or instrument (e.g. Orchestra, Vocal, Piano)
  // and every even is an artist or a comma delimited list of artists.

  if (values.size() % 2 != 0) // Must contain an even number of entries 
    return;

  for (size_t i = 0; i + 1 < values.size(); i += 2)
    tag.AddArtistRole(values[i], StringUtils::Split(values[i + 1], ","));
}

void CTagLoaderTagLib::AddArtistInstrument(CMusicInfoTag &tag, const std::vector<std::string> &values)
{
  // Values is a musician credits list, each entry is artist name followed by instrument (or function)
  // e.g. violin, drums, background vocals, solo, orchestra etc. in brackets. This is how Picard uses PERFORMER tag.
  // If there is not a pair of brackets then role is "performer" by default, and the whole entry is 
  // taken as artist name.
  
  for (size_t i = 0; i < values.size(); ++i)
  {
    std::string strRole = "Performer";
    std::string strArtist = values[i];
    size_t firstLim = values[i].find_first_of("(");
    size_t lastLim = values[i].find_last_of(")");
    if (lastLim != std::string::npos && firstLim != std::string::npos && firstLim < lastLim - 1)
    {
      //Pair of brackets with something between them
      strRole = values[i].substr(firstLim + 1, lastLim - firstLim - 1);
      StringUtils::Trim(strRole);
      strArtist.erase(firstLim, lastLim - firstLim + 1);
    }
    StringUtils::Trim(strArtist);
    tag.AddArtistRole(strRole, strArtist);
  }
}

bool CTagLoaderTagLib::Load(const std::string& strFileName, CMusicInfoTag& tag, const std::string& fallbackFileExtension, MUSIC_INFO::EmbeddedArt *art /* = NULL */)
{
  std::string strExtension = URIUtils::GetExtension(strFileName);
  StringUtils::TrimLeft(strExtension, ".");

  if (strExtension.empty())
  {
    strExtension = fallbackFileExtension;
    if (strExtension.empty())
      return false;
  }

  StringUtils::ToLower(strExtension);
  TagLibVFSStream*           stream = new TagLibVFSStream(strFileName, true);
  if (!stream)
  {
    CLog::Log(LOGERROR, "could not create TagLib VFS stream for: %s", strFileName.c_str());
    return false;
  }

  TagLib::File*              file = nullptr;
  TagLib::APE::File*         apeFile = nullptr;
  TagLib::ASF::File*         asfFile = nullptr;
  TagLib::FLAC::File*        flacFile = nullptr;
  TagLib::MP4::File*         mp4File = nullptr;
  TagLib::MPC::File*         mpcFile = nullptr;
  TagLib::MPEG::File*        mpegFile = nullptr;
  TagLib::Ogg::Vorbis::File* oggVorbisFile = nullptr;
  TagLib::Ogg::FLAC::File*   oggFlacFile = nullptr;
  TagLib::Ogg::Opus::File*   oggOpusFile = nullptr;
  TagLib::TrueAudio::File*   ttaFile = nullptr;
  TagLib::WavPack::File*     wvFile = nullptr;
  TagLib::RIFF::WAV::File *  wavFile = nullptr;
  TagLib::RIFF::AIFF::File * aiffFile = nullptr;

  try
  {
    if (strExtension == "ape")
      file = apeFile = new APE::File(stream);
    else if (strExtension == "asf" || strExtension == "wmv" || strExtension == "wma")
      file = asfFile = new ASF::File(stream);
    else if (strExtension == "flac")
      file = flacFile = new FLAC::File(stream, ID3v2::FrameFactory::instance());
    else if (strExtension == "it")
      file = new IT::File(stream);
    else if (strExtension == "mod" || strExtension == "module" || strExtension == "nst" || strExtension == "wow")
      file = new Mod::File(stream);
    else if (strExtension == "mp4" || strExtension == "m4a" ||
             strExtension == "m4r" || strExtension == "m4b" ||
             strExtension == "m4p" || strExtension == "3g2")
      file = mp4File = new MP4::File(stream);
    else if (strExtension == "mpc")
      file = mpcFile = new MPC::File(stream);
    else if (strExtension == "mp3" || strExtension == "aac")
      file = mpegFile = new MPEG::File(stream, ID3v2::FrameFactory::instance());
    else if (strExtension == "s3m")
      file = new S3M::File(stream);
    else if (strExtension == "tta")
      file = ttaFile = new TrueAudio::File(stream, ID3v2::FrameFactory::instance());
    else if (strExtension == "wv")
      file = wvFile = new WavPack::File(stream);
    else if (strExtension == "aif" || strExtension == "aiff")
      file = aiffFile = new RIFF::AIFF::File(stream);
    else if (strExtension == "wav")
      file = wavFile = new RIFF::WAV::File(stream);
    else if (strExtension == "xm")
      file = new XM::File(stream);
    else if (strExtension == "ogg")
      file = oggVorbisFile = new Ogg::Vorbis::File(stream);
    else if (strExtension == "opus")
      file = oggOpusFile = new Ogg::Opus::File(stream);
    else if (strExtension == "oga") // Leave this madness until last - oga container can have Vorbis or FLAC
    {
      file = oggFlacFile = new Ogg::FLAC::File(stream);
      if (!file || !file->isValid())
      {
        delete file;
        oggFlacFile = nullptr;
        file = oggVorbisFile = new Ogg::Vorbis::File(stream);
      }
    }
  }
  catch (const std::exception& ex)
  {
    CLog::Log(LOGERROR, "Taglib exception: %s", ex.what());
  }

  if (!file || !file->isOpen())
  {
    delete file;
    delete stream;
    CLog::Log(LOGDEBUG, "file %s could not be opened for tag reading", strFileName.c_str());
    return false;
  }

  APE::Tag *ape = nullptr;
  ASF::Tag *asf = nullptr;
  MP4::Tag *mp4 = nullptr;
  ID3v1::Tag *id3v1 = nullptr;
  ID3v2::Tag *id3v2 = nullptr;
  Ogg::XiphComment *xiph = nullptr;
  Tag *generic = nullptr;

  if (apeFile)
    ape = apeFile->APETag(false);
  else if (asfFile)
    asf = asfFile->tag();
  else if (flacFile)
  {
    xiph = flacFile->xiphComment(false);
    id3v2 = flacFile->ID3v2Tag(false);
  }
  else if (mp4File)
    mp4 = mp4File->tag();
  else if (mpegFile)
  {
    id3v1 = mpegFile->ID3v1Tag(false);
    id3v2 = mpegFile->ID3v2Tag(false);
    ape = mpegFile->APETag(false);
  }
  else if (oggFlacFile)
    xiph = dynamic_cast<Ogg::XiphComment *>(oggFlacFile->tag());
  else if (oggVorbisFile)
    xiph = dynamic_cast<Ogg::XiphComment *>(oggVorbisFile->tag());
  else if (oggOpusFile)
    xiph = dynamic_cast<Ogg::XiphComment *>(oggOpusFile->tag());
  else if (ttaFile)
    id3v2 = ttaFile->ID3v2Tag(false);
  else if (aiffFile)
    id3v2 = aiffFile->tag();
  else if (wavFile)
#if TAGLIB_MAJOR_VERSION > 1 || TAGLIB_MINOR_VERSION > 8
    id3v2 = wavFile->ID3v2Tag();
#else
    id3v2 = wavFile->tag();
#endif
  else if (wvFile)
    ape = wvFile->APETag(false);
  else if (mpcFile)
    ape = mpcFile->APETag(false);
  else    // This is a catch all to get generic information for other files types (s3m, xm, it, mod, etc)
    generic = file->tag();

  if (file->audioProperties())
    tag.SetDuration(file->audioProperties()->length());

  if (asf)
    ParseTag(asf, art, tag);
  if (id3v1)
    ParseTag(id3v1, art, tag);
  if (id3v2)
    ParseTag(id3v2, art, tag);
  if (generic)
    ParseTag(generic, art, tag);
  if (mp4)
    ParseTag(mp4, art, tag);
  if (xiph) // xiph tags override id3v2 tags in badly tagged FLACs
    ParseTag(xiph, art, tag);
  if (ape && (!id3v2 || g_advancedSettings.m_prioritiseAPEv2tags)) // ape tags override id3v2 if we're prioritising them
    ParseTag(ape, art, tag);

  // art for flac files is outside the tag
  if (flacFile)
    SetFlacArt(flacFile, art, tag);

  if (!tag.GetTitle().empty() || !tag.GetArtist().empty() || !tag.GetAlbum().empty())
    tag.SetLoaded();
  tag.SetURL(strFileName);

  delete file;
  delete stream;

  return true;
}
