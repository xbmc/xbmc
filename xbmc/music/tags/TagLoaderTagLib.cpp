/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "TagLoaderTagLib.h"

#include <vector>

#include <taglib/id3v1tag.h>
#include <taglib/apetag.h>
#include <taglib/asftag.h>
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
#include "ServiceBroker.h"
#include "settings/AdvancedSettings.h"
#include "settings/SettingsComponent.h"

#if TAGLIB_MAJOR_VERSION <= 1 && TAGLIB_MINOR_VERSION < 11
#include "utils/Base64.h"
#endif

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
  for (const FLAC::Picture* const c : cover)
  {
    if (c)
    {
      tag.SetCoverArtInfo(c->data().size(), c->mimeType().to8Bit(true));
      if (art)
        art->Set(reinterpret_cast<const uint8_t*>(c->data().data()), c->data().size(), c->mimeType().to8Bit(true));
      return; // one is enough
    }
  }
}
}

bool CTagLoaderTagLib::Load(const std::string& strFileName, MUSIC_INFO::CMusicInfoTag& tag, EmbeddedArt *art /* = NULL */)
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
    else if (it->first == "WM/ArtistSortOrder")
      SetArtistSort(tag, GetASFStringList(it->second));
    else if (it->first == "WM/AlbumArtist")
      SetAlbumArtist(tag, GetASFStringList(it->second));
    else if (it->first == "WM/AlbumArtistSortOrder")
      SetAlbumArtistSort(tag, GetASFStringList(it->second));
    else if (it->first == "WM/ComposerSortOrder")
      SetComposerSort(tag, GetASFStringList(it->second));
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
      tag.SetRecordLabel(it->second.front().toString().to8Bit(true));
    else if (it->first == "WM/Script")
    {} // Known unsupported, suppress warnings
    else if (it->first == "WM/Year")
      tag.SetReleaseDate(it->second.front().toString().to8Bit(true));
    else if (it->first == "WM/OriginalReleaseYear")
      tag.SetOriginalDate(it->second.front().toString().to8Bit(true));
    else if (it->first == "WM/SetSubTitle")
      tag.SetDiscSubtitle(it->second.front().toString().to8Bit(true));
    else if (it->first == "MusicBrainz/Artist Id")
      tag.SetMusicBrainzArtistID(SplitMBID(GetASFStringList(it->second)));
    else if (it->first == "MusicBrainz/Album Id")
      tag.SetMusicBrainzAlbumID(it->second.front().toString().to8Bit(true));
    else if (it->first == "MusicBrainz/Release Group Id")
      tag.SetMusicBrainzReleaseGroupID(it->second.front().toString().to8Bit(true));
    else if (it->first == "MusicBrainz/Album Artist")
      SetAlbumArtist(tag, GetASFStringList(it->second));
    else if (it->first == "MusicBrainz/Album Artist Id")
      tag.SetMusicBrainzAlbumArtistID(SplitMBID(GetASFStringList(it->second)));
    else if (it->first == "MusicBrainz/Track Id")
      tag.SetMusicBrainzTrackID(it->second.front().toString().to8Bit(true));
    else if (it->first == "MusicBrainz/Album Status")
      tag.SetAlbumReleaseStatus(it->second.front().toString().toCString(true));
    else if (it->first == "MusicBrainz/Album Type")
      SetReleaseType(tag, GetASFStringList(it->second));
    else if (it->first == "MusicIP/PUID")
    {}
    else if (it->first == "WM/BeatsPerMinute")
      tag.SetBPM(atoi(it->second.front().toString().toCString(true)));
    else if (it->first == "replaygain_track_gain" || it->first == "REPLAYGAIN_TRACK_GAIN")
      replayGainInfo.ParseGain(ReplayGain::TRACK, it->second.front().toString().toCString(true));
    else if (it->first == "replaygain_album_gain" || it->first == "REPLAYGAIN_ALBUM_GAIN")
      replayGainInfo.ParseGain(ReplayGain::ALBUM, it->second.front().toString().toCString(true));
    else if (it->first == "replaygain_track_peak" || it->first == "REPLAYGAIN_TRACK_PEAK")
      replayGainInfo.ParsePeak(ReplayGain::TRACK, it->second.front().toString().toCString(true));
    else if (it->first == "replaygain_album_peak" || it->first == "REPLAYGAIN_ALBUM_PEAK")
      replayGainInfo.ParsePeak(ReplayGain::ALBUM, it->second.front().toString().toCString(true));
    else if (it->first == "WM/Picture")
    { // picture
      ASF::Picture pic = it->second.front().toPicture();
      tag.SetCoverArtInfo(pic.picture().size(), pic.mimeType().toCString());
      if (art)
        art->Set(reinterpret_cast<const uint8_t *>(pic.picture().data()), pic.picture().size(), pic.mimeType().toCString());
    }
    else if (CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_logLevel == LOG_LEVEL_MAX)
      CLog::Log(LOGDEBUG, "unrecognized ASF tag name: {}", it->first.toCString(true));
  }
  // artist may be specified in the ContentDescription block rather than using the 'Author' attribute.
  if (tag.GetArtist().empty())
    tag.SetArtist(asf->artist().toCString(true));

  if (!asf->comment().isEmpty())
    tag.SetComment(asf->comment().toCString(true));
  tag.SetReplayGain(replayGainInfo);
  tag.SetLoaded(true);
  return true;
}

int CTagLoaderTagLib::POPMtoXBMC(int popm)
{
  // Ratings:
  // FROM: http://www.mediamonkey.com/forum/viewtopic.php?f=7&t=40532&start=30#p391067
  // The following schemes are used by the other POPM-compatible players:
  // WMP/Vista: "Windows Media Player 9 Series" ratings:
  //   1 = 1, 2 = 64, 3=128, 4=196 (not 192), 5=255
  // MediaMonkey (v4.2.1): "no@email" ratings:
  //   0.5=13, 1=1, 1.5=54, 2=64, 2.5=118,
  //   3=128, 3.5=186, 4=196, 4.5=242, 5=255
  //   Note 1 star written as 1 while half a star is 13, a higher value
  // Accommodate these mapped values in a scale from 0-255
  if (popm == 0) return 0;
  if (popm == 1) return 2;
  if (popm < 23) return 1;
  if (popm < 32) return 2;
  if (popm < 64) return 3;
  if (popm < 96) return 4;
  if (popm < 128) return 5;
  if (popm < 160) return 6;
  if (popm < 196) return 7;
  if (popm < 224) return 8;
  if (popm < 255) return 9;
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
  tag.SetGenre(id3v1->genre().to8Bit(true), true);
  tag.SetYear(id3v1->year());
  tag.SetTrackNumber(id3v1->track());
  return true;
}

template<>
bool CTagLoaderTagLib::ParseTag(ID3v2::Tag *id3v2, EmbeddedArt *art, MUSIC_INFO::CMusicInfoTag& tag)
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
    else if (it->first == "TSOP")   SetArtistSort(tag, GetID3v2StringList(it->second));
    else if (it->first == "TALB")   tag.SetAlbum(it->second.front()->toString().to8Bit(true));
    else if (it->first == "TPE2")   SetAlbumArtist(tag, GetID3v2StringList(it->second));
    else if (it->first == "TSO2")   SetAlbumArtistSort(tag, GetID3v2StringList(it->second));
    else if (it->first == "TSOC")   SetComposerSort(tag, GetID3v2StringList(it->second));
    else if (it->first == "TIT2")   tag.SetTitle(it->second.front()->toString().to8Bit(true));
    else if (it->first == "TCON")   SetGenre(tag, GetID3v2StringList(it->second));
    else if (it->first == "TRCK")
      tag.SetTrackNumber(
          static_cast<int>(strtol(it->second.front()->toString().toCString(true), nullptr, 10)));
    else if (it->first == "TPOS")
      tag.SetDiscNumber(
          static_cast<int>(strtol(it->second.front()->toString().toCString(true), nullptr, 10)));
    else if (it->first == "TDOR" || it->first == "TORY") // TDOR - ID3v2.4, TORY - ID3v2.3
      tag.SetOriginalDate(it->second.front()->toString().to8Bit(true));
    else if (it->first == "TDAT")   {} // empty as taglib has moved the value to TDRC
    else if (it->first == "TCMP")   tag.SetCompilation((strtol(it->second.front()->toString().toCString(true), nullptr, 10) == 0) ? false : true);
    else if (it->first == "TENC")   {} // EncodedBy
    else if (it->first == "TCOM")   AddArtistRole(tag, "Composer", GetID3v2StringList(it->second));
    else if (it->first == "TPE3")   AddArtistRole(tag, "Conductor", GetID3v2StringList(it->second));
    else if (it->first == "TEXT")   AddArtistRole(tag, "Lyricist", GetID3v2StringList(it->second));
    else if (it->first == "TPE4")   AddArtistRole(tag, "Remixer", GetID3v2StringList(it->second));
    else if (it->first == "TPUB")   tag.SetRecordLabel(it->second.front()->toString().to8Bit(true));
    else if (it->first == "TCOP")   {} // Copyright message
    else if (it->first == "TDRC")  // taglib concatenates TYER & TDAT into this field if v2.3
      tag.SetReleaseDate(it->second.front()->toString().to8Bit(true));
    else if (it->first == "TDRL")   {} // Not set by Picard or used in community generally
    else if (it->first == "TDTG")   {} // Tagging time
    else if (it->first == "TLAN")   {} // Languages
    else if (it->first == "TMOO")   tag.SetMood(it->second.front()->toString().to8Bit(true));
    else if (it->first == "TSST")
      tag.SetDiscSubtitle(it->second.front()->toString().to8Bit(true));
    else if (it->first == "TBPM")
      tag.SetBPM(
          static_cast<int>(strtol(it->second.front()->toString().toCString(true), nullptr, 10)));
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
        else if (desc == "MUSICBRAINZ RELEASE GROUP ID")
          tag.SetMusicBrainzReleaseGroupID(stringList.front().to8Bit(true));
        else if (desc == "MUSICBRAINZ ALBUM ARTIST ID")
          tag.SetMusicBrainzAlbumArtistID(SplitMBID(StringListToVectorString(stringList)));
        else if (desc == "MUSICBRAINZ ALBUM ARTIST")
          SetAlbumArtist(tag, StringListToVectorString(stringList));
        else if (desc == "MUSICBRAINZ ALBUM TYPE")
          SetReleaseType(tag, StringListToVectorString(stringList));
        else if (desc == "MUSICBRAINZ ALBUM STATUS")
          tag.SetAlbumReleaseStatus(stringList.front().to8Bit(true));
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
        else if (desc == "ALBUMARTISTSORT" || desc == "ALBUM ARTIST SORT")
          SetAlbumArtistSort(tag, StringListToVectorString(stringList));
        else if (desc == "ARTISTS")
          SetArtistHints(tag, StringListToVectorString(stringList));
        else if (desc == "ALBUMARTISTS" || desc == "ALBUM ARTISTS")
          SetAlbumArtistHints(tag, StringListToVectorString(stringList));
        else if (desc == "WRITER")  // How Picard >1.3 tags writer in ID3
          AddArtistRole(tag, "Writer", StringListToVectorString(stringList));
        else if (desc == "COMPOSERSORT" || desc == "COMPOSER SORT")
          SetComposerSort(tag, StringListToVectorString(stringList));
        else if (desc == "MOOD")
          tag.SetMood(stringList.front().to8Bit(true));
        else if (CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_logLevel == LOG_LEVEL_MAX)
          CLog::Log(LOGDEBUG, "unrecognized user text tag detected: TXXX:{}",
                    frame->description().toCString(true));
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
      // Loop through and process the musician credits list
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
        auto ufid = dynamic_cast<ID3v2::UniqueFileIdentifierFrame*> (*ut);
        if (ufid && ufid->owner() == "http://musicbrainz.org")
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
            CLog::Log(LOGDEBUG, "unrecognized ratings schema detected: {}",
                      popFrame->email().toCString(true));
          tag.SetUserrating(POPMtoXBMC(popFrame->rating()));
        }
      }
    else if (CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_logLevel == LOG_LEVEL_MAX)
      CLog::Log(LOGDEBUG, "unrecognized ID3 frame detected: {}{}{}{}", it->first[0], it->first[1],
                it->first[2], it->first[3]);
  } // for

  // Process the extracted picture frames; 0 = CoverArt, 1 = Other, 2 = First Found picture
  for (const ID3v2::AttachedPictureFrame* const picture : pictures)
    if (picture)
    {
      std::string  mime =            picture->mimeType().to8Bit(true);
#if (TAGLIB_MAJOR_VERSION >= 2)
      unsigned int size =            picture->picture().size();
#else
      TagLib::uint size =            picture->picture().size();
#endif
      tag.SetCoverArtInfo(size, mime);
      if (art)
        art->Set(reinterpret_cast<const uint8_t*>(picture->picture().data()), size, mime);

      // Stop after we find the first picture for now.
      break;
    }


  if (!id3v2->comment().isEmpty())
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
      SetArtist(tag, StringListToVectorString(it->second.values()));
    else if (it->first == "ARTISTSORT")
      SetArtistSort(tag, StringListToVectorString(it->second.values()));
    else if (it->first == "ARTISTS")
      SetArtistHints(tag, StringListToVectorString(it->second.values()));
    else if (it->first == "ALBUMARTIST" || it->first == "ALBUM ARTIST")
      SetAlbumArtist(tag, StringListToVectorString(it->second.values()));
    else if (it->first == "ALBUMARTISTSORT")
      SetAlbumArtistSort(tag, StringListToVectorString(it->second.values()));
    else if (it->first == "ALBUMARTISTS" || it->first == "ALBUM ARTISTS")
      SetAlbumArtistHints(tag, StringListToVectorString(it->second.values()));
    else if (it->first == "COMPOSERSORT")
      SetComposerSort(tag, StringListToVectorString(it->second.values()));
    else if (it->first == "ALBUM")
      tag.SetAlbum(it->second.toString().to8Bit(true));
    else if (it->first == "TITLE")
      tag.SetTitle(it->second.toString().to8Bit(true));
    else if (it->first == "TRACKNUMBER" || it->first == "TRACK")
      tag.SetTrackNumber(it->second.toString().toInt());
    else if (it->first == "DISCNUMBER" || it->first == "DISC")
      tag.SetDiscNumber(it->second.toString().toInt());
    else if (it->first == "YEAR")
      tag.SetReleaseDate(it->second.toString().to8Bit(true));
    else if (it->first == "DISCSUBTITLE")
      tag.SetDiscSubtitle(it->second.toString().to8Bit(true));
    else if (it->first == "ORIGINALYEAR")
      tag.SetOriginalDate(it->second.toString().to8Bit(true));
    else if (it->first == "GENRE")
      SetGenre(tag, StringListToVectorString(it->second.values()));
    else if (it->first == "MOOD")
      tag.SetMood(it->second.toString().to8Bit(true));
    else if (it->first == "COMMENT")
      tag.SetComment(it->second.toString().to8Bit(true));
    else if (it->first == "CUESHEET")
      tag.SetCueSheet(it->second.toString().to8Bit(true));
    else if (it->first == "ENCODEDBY")
    {}
    else if (it->first == "COMPOSER")
      AddArtistRole(tag, "Composer", StringListToVectorString(it->second.values()));
    else if (it->first == "CONDUCTOR")
      AddArtistRole(tag, "Conductor", StringListToVectorString(it->second.values()));
    else if (it->first == "BAND")
      AddArtistRole(tag, "Band", StringListToVectorString(it->second.values()));
    else if (it->first == "ENSEMBLE")
      AddArtistRole(tag, "Ensemble", StringListToVectorString(it->second.values()));
    else if (it->first == "LYRICIST")
      AddArtistRole(tag, "Lyricist", StringListToVectorString(it->second.values()));
    else if (it->first == "WRITER")
      AddArtistRole(tag, "Writer", StringListToVectorString(it->second.values()));
    else if ((it->first == "MIXARTIST") || (it->first == "REMIXER"))
      AddArtistRole(tag, "Remixer", StringListToVectorString(it->second.values()));
    else if (it->first == "ARRANGER")
      AddArtistRole(tag, "Arranger", StringListToVectorString(it->second.values()));
    else if (it->first == "ENGINEER")
      AddArtistRole(tag, "Engineer", StringListToVectorString(it->second.values()));
    else if (it->first == "PRODUCER")
      AddArtistRole(tag, "Producer", StringListToVectorString(it->second.values()));
    else if (it->first == "DJMIXER")
      AddArtistRole(tag, "DJMixer", StringListToVectorString(it->second.values()));
    else if (it->first == "MIXER")
      AddArtistRole(tag, "Mixer", StringListToVectorString(it->second.values()));
    else if (it->first == "PERFORMER")
      // Picard uses PERFORMER tag as musician credits list formatted "name (instrument)"
      AddArtistInstrument(tag, StringListToVectorString(it->second.values()));
    else if (it->first == "LABEL")
      tag.SetRecordLabel(it->second.toString().to8Bit(true));
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
      tag.SetMusicBrainzArtistID(SplitMBID(StringListToVectorString(it->second.values())));
    else if (it->first == "MUSICBRAINZ_ALBUMARTISTID")
      tag.SetMusicBrainzAlbumArtistID(SplitMBID(StringListToVectorString(it->second.values())));
    else if (it->first == "MUSICBRAINZ_ALBUMARTIST")
      SetAlbumArtist(tag, StringListToVectorString(it->second.values()));
    else if (it->first == "MUSICBRAINZ_ALBUMID")
      tag.SetMusicBrainzAlbumID(it->second.toString().to8Bit(true));
    else if (it->first == "MUSICBRAINZ_RELEASEGROUPID")
      tag.SetMusicBrainzReleaseGroupID(it->second.toString().to8Bit(true));
    else if (it->first == "MUSICBRAINZ_TRACKID")
      tag.SetMusicBrainzTrackID(it->second.toString().to8Bit(true));
    else if (it->first == "MUSICBRAINZ_ALBUMTYPE")
      SetReleaseType(tag, StringListToVectorString(it->second.values()));
    else if (it->first == "BPM")
      tag.SetBPM(it->second.toString().toInt());
    else if (it->first == "MUSICBRAINZ_ALBUMSTATUS")
      tag.SetAlbumReleaseStatus(it->second.toString().to8Bit(true));
    else if (it->first == "COVER ART (FRONT)")
    {
      TagLib::ByteVector tdata = it->second.binaryData();
      // The image data follows a null byte, which can optionally be preceded by a filename
      const uint offset = tdata.find('\0') + 1;
      ByteVector bv(tdata.data() + offset, tdata.size() - offset);
      // Infer the mimetype
      std::string mime{};
      if (bv.startsWith("\xFF\xD8\xFF"))
        mime = "image/jpeg";
      else if (bv.startsWith("\x89\x50\x4E\x47"))
        mime = "image/png";
      else if (bv.startsWith("\x47\x49\x46\x38"))
        mime = "image/gif";
      else if (bv.startsWith("\x42\x4D"))
        mime = "image/bmp";
      if ((offset > 0) && (offset <= tdata.size()) && (mime.size() > 0))
      {
        tag.SetCoverArtInfo(bv.size(), mime);
        if (art)
          art->Set(reinterpret_cast<const uint8_t*>(bv.data()), bv.size(), mime);
      }
    }
    else if (CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_logLevel == LOG_LEVEL_MAX)
      CLog::Log(LOGDEBUG, "unrecognized APE tag: {}", it->first.toCString(true));
  }

  tag.SetReplayGain(replayGainInfo);
  return true;
}

template<>
bool CTagLoaderTagLib::ParseTag(Ogg::XiphComment *xiph, EmbeddedArt *art, CMusicInfoTag& tag)
{
  if (!xiph)
    return false;

#if TAGLIB_MAJOR_VERSION <= 1 && TAGLIB_MINOR_VERSION < 11
  FLAC::Picture pictures[3];
#endif
  ReplayGain replayGainInfo;

  const Ogg::FieldListMap& fieldListMap = xiph->fieldListMap();
  for (Ogg::FieldListMap::ConstIterator it = fieldListMap.begin(); it != fieldListMap.end(); ++it)
  {
    if (it->first == "ARTIST")
      SetArtist(tag, StringListToVectorString(it->second));
    else if (it->first == "ARTISTSORT")
      SetArtistSort(tag, StringListToVectorString(it->second));
    else if (it->first == "ARTISTS")
      SetArtistHints(tag, StringListToVectorString(it->second));
    else if (it->first == "ALBUMARTIST" || it->first == "ALBUM ARTIST")
      SetAlbumArtist(tag, StringListToVectorString(it->second));
    else if (it->first == "ALBUMARTISTSORT" || it->first == "ALBUM ARTIST SORT")
      SetAlbumArtistSort(tag, StringListToVectorString(it->second));
    else if (it->first == "ALBUMARTISTS" || it->first == "ALBUM ARTISTS")
      SetAlbumArtistHints(tag, StringListToVectorString(it->second));
    else if (it->first == "COMPOSERSORT")
      SetComposerSort(tag, StringListToVectorString(it->second));
    else if (it->first == "ALBUM")
      tag.SetAlbum(it->second.front().to8Bit(true));
    else if (it->first == "TITLE")
      tag.SetTitle(it->second.front().to8Bit(true));
    else if (it->first == "TRACKNUMBER")
      tag.SetTrackNumber(it->second.front().toInt());
    else if (it->first == "DISCNUMBER")
      tag.SetDiscNumber(it->second.front().toInt());
    else if (it->first == "YEAR" || it->first == "DATE")
      tag.AddReleaseDate(it->second.front().to8Bit(true));
    else if (it->first == "GENRE")
      SetGenre(tag, StringListToVectorString(it->second));
    else if (it->first == "MOOD")
      tag.SetMood(it->second.front().to8Bit(true));
    else if (it->first == "COMMENT")
      tag.SetComment(it->second.front().to8Bit(true));
    else if (it->first == "ORIGINALYEAR" || it->first == "ORIGINALDATE")
      tag.AddOriginalDate(it->second.front().to8Bit(true));
    else if (it->first == "CUESHEET")
      tag.SetCueSheet(it->second.front().to8Bit(true));
    else if (it->first == "DISCSUBTITLE")
      tag.SetDiscSubtitle(it->second.front().to8Bit(true));
    else if (it->first == "ENCODEDBY")
    {} // Known but unsupported, suppress warnings
    else if (it->first == "COMPOSER")
      AddArtistRole(tag, "Composer", StringListToVectorString(it->second));
    else if (it->first == "CONDUCTOR")
      AddArtistRole(tag, "Conductor", StringListToVectorString(it->second));
    else if (it->first == "BAND")
      AddArtistRole(tag, "Band", StringListToVectorString(it->second));
    else if (it->first == "ENSEMBLE")
      AddArtistRole(tag, "Ensemble", StringListToVectorString(it->second));
    else if (it->first == "LYRICIST")
      AddArtistRole(tag, "Lyricist", StringListToVectorString(it->second));
    else if (it->first == "WRITER")
      AddArtistRole(tag, "Writer", StringListToVectorString(it->second));
    else if ((it->first == "MIXARTIST") || (it->first == "REMIXER"))
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
      tag.SetRecordLabel(it->second.front().to8Bit(true));
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
    else if (it->first == "MUSICBRAINZ_RELEASEGROUPID")
      tag.SetMusicBrainzReleaseGroupID(it->second.front().to8Bit(true));
    else if (it->first == "MUSICBRAINZ_TRACKID")
      tag.SetMusicBrainzTrackID(it->second.front().to8Bit(true));
    else if (it->first == "RELEASETYPE")
      SetReleaseType(tag, StringListToVectorString(it->second));
    else if (it->first == "BPM")
      tag.SetBPM(strtol(it->second.front().toCString(true), nullptr, 10));
    else if (it->first == "RELEASESTATUS")
      tag.SetAlbumReleaseStatus(it->second.front().toCString(true));
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
    else if (CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_logLevel == LOG_LEVEL_MAX)
      CLog::Log(LOGDEBUG, "unrecognized XipComment name: {}", it->first.toCString(true));
  }

#if TAGLIB_MAJOR_VERSION <= 1 && TAGLIB_MINOR_VERSION < 11
  // Process the extracted picture frames; 0 = CoverArt, 1 = Other, 2 = COVERART/COVERARTMIME
  for (int i = 0; i < 3; ++i)
    if (pictures[i].data().size())
    {
      std::string mime = pictures[i].mimeType().toCString();
      if (mime.compare(0, 6, "image/") != 0)
        continue;
#if (TAGLIB_MAJOR_VERSION >= 2)
      unsigned int size =            pictures[i].data().size();
#else
      TagLib::uint size =            pictures[i].data().size();
#endif
      tag.SetCoverArtInfo(size, mime);
      if (art)
        art->Set(reinterpret_cast<const uint8_t*>(pictures[i].data().data()), size, mime);

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
  for (const FLAC::Picture* const c : cover)
  {
    if (c)
    {
      tag.SetCoverArtInfo(c->data().size(), c->mimeType().to8Bit(true));
      if (art)
        art->Set(reinterpret_cast<const uint8_t*>(c->data().data()), c->data().size(), c->mimeType().to8Bit(true));
      break; // one is enough
    }
  }
#endif

  if (!xiph->comment().isEmpty())
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
  const MP4::ItemMap itemMap = mp4->itemMap();
  for (auto it = itemMap.begin(); it != itemMap.end(); ++it)
  {
    if (it->first == "\251nam")
      tag.SetTitle(it->second.toStringList().front().to8Bit(true));
    else if (it->first == "\251ART")
      SetArtist(tag, StringListToVectorString(it->second.toStringList()));
    else if (it->first == "soar")
      SetArtistSort(tag, StringListToVectorString(it->second.toStringList()));
    else if (it->first == "----:com.apple.iTunes:ARTISTS")
      SetArtistHints(tag, StringListToVectorString(it->second.toStringList()));
    else if (it->first == "\251alb")
      tag.SetAlbum(it->second.toStringList().front().to8Bit(true));
    else if (it->first == "aART")
      SetAlbumArtist(tag, StringListToVectorString(it->second.toStringList()));
    else if (it->first == "soaa")
      SetAlbumArtistSort(tag, StringListToVectorString(it->second.toStringList()));
    else if (it->first == "----:com.apple.iTunes:albumartists" ||
             it->first == "----:com.apple.iTunes:ALBUMARTISTS")
      SetAlbumArtistHints(tag, StringListToVectorString(it->second.toStringList()));
    else if (it->first == "soco")
      SetComposerSort(tag, StringListToVectorString(it->second.toStringList()));
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
      tag.SetRecordLabel(it->second.toStringList().front().to8Bit(true));
    else if (it->first == "----:com.apple.iTunes:DISCSUBTITLE")
      tag.SetDiscSubtitle(it->second.toStringList().front().to8Bit(true));
    else if (it->first == "cpil")
      tag.SetCompilation(it->second.toBool());
    else if (it->first == "trkn")
      tag.SetTrackNumber(it->second.toIntPair().first);
    else if (it->first == "disk")
      tag.SetDiscNumber(it->second.toIntPair().first);
    else if (it->first == "\251day")
      tag.SetReleaseDate(it->second.toStringList().front().to8Bit(true));
    else if (it->first == "----:com.apple.iTunes:originaldate")
      tag.SetOriginalDate(it->second.toStringList().front().to8Bit(true));
    else if (it->first == "----:com.apple.iTunes:replaygain_track_gain" ||
             it->first == "----:com.apple.iTunes:REPLAYGAIN_TRACK_GAIN")
      replayGainInfo.ParseGain(ReplayGain::TRACK, it->second.toStringList().front().toCString());
    else if (it->first == "----:com.apple.iTunes:replaygain_album_gain" ||
             it->first == "----:com.apple.iTunes:REPLAYGAIN_ALBUM_GAIN")
      replayGainInfo.ParseGain(ReplayGain::ALBUM, it->second.toStringList().front().toCString());
    else if (it->first == "----:com.apple.iTunes:replaygain_track_peak" ||
             it->first == "----:com.apple.iTunes:REPLAYGAIN_TRACK_PEAK")
      replayGainInfo.ParsePeak(ReplayGain::TRACK, it->second.toStringList().front().toCString());
    else if (it->first == "----:com.apple.iTunes:replaygain_album_peak" ||
             it->first == "----:com.apple.iTunes:REPLAYGAIN_ALBUM_PEAK")
      replayGainInfo.ParsePeak(ReplayGain::ALBUM, it->second.toStringList().front().toCString());
    else if (it->first == "----:com.apple.iTunes:MusicBrainz Artist Id")
      tag.SetMusicBrainzArtistID(SplitMBID(StringListToVectorString(it->second.toStringList())));
    else if (it->first == "----:com.apple.iTunes:MusicBrainz Album Artist Id")
      tag.SetMusicBrainzAlbumArtistID(SplitMBID(StringListToVectorString(it->second.toStringList())));
    else if (it->first == "----:com.apple.iTunes:MusicBrainz Album Artist")
      SetAlbumArtist(tag, StringListToVectorString(it->second.toStringList()));
    else if (it->first == "----:com.apple.iTunes:MusicBrainz Album Id")
      tag.SetMusicBrainzAlbumID(it->second.toStringList().front().to8Bit(true));
    else if (it->first == "----:com.apple.iTunes:MusicBrainz Release Group Id")
      tag.SetMusicBrainzReleaseGroupID(it->second.toStringList().front().to8Bit(true));
    else if (it->first == "----:com.apple.iTunes:MusicBrainz Track Id")
      tag.SetMusicBrainzTrackID(it->second.toStringList().front().to8Bit(true));
    else if (it->first == "----:com.apple.iTunes:MusicBrainz Album Type")
      SetReleaseType(tag, StringListToVectorString(it->second.toStringList()));
    else if (it->first == "----:com.apple.iTunes:MusicBrainz Album Status")
      tag.SetAlbumReleaseStatus(it->second.toStringList().front().to8Bit(true));
    else if (it->first == "tmpo")
      tag.SetBPM(it->second.toIntPair().first);
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
          art->Set(reinterpret_cast<const uint8_t *>(pt->data().data()), pt->data().size(), mime);
        break; // one is enough
      }
    }
  }

  if (!mp4->comment().isEmpty())
    tag.SetComment(mp4->comment().toCString(true));

  tag.SetReplayGain(replayGainInfo);
  return true;
}

template<>
bool CTagLoaderTagLib::ParseTag(Tag *genericTag, EmbeddedArt *art, CMusicInfoTag& tag)
{
  if (!genericTag)
    return false;

  PropertyMap properties = genericTag->properties();
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

void CTagLoaderTagLib::SetArtistSort(CMusicInfoTag &tag, const std::vector<std::string> &values)
{
  // ARTISTSORT/TSOP tag is often a single string, when not take union of values
  if (values.size() == 1)
    tag.SetArtistSort(values[0]);
  else
    tag.SetArtistSort(StringUtils::Join(values, CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_musicItemSeparator));
}

void CTagLoaderTagLib::SetArtistHints(CMusicInfoTag &tag, const std::vector<std::string> &values)
{
  if (values.size() == 1)
    tag.SetMusicBrainzArtistHints(StringUtils::Split(values[0], CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_musicItemSeparator));
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

void CTagLoaderTagLib::SetAlbumArtistSort(CMusicInfoTag &tag, const std::vector<std::string> &values)
{
  // ALBUMARTISTSORT/TSOP tag is often a single string, when not take union of values
  if (values.size() == 1)
    tag.SetAlbumArtistSort(values[0]);
  else
    tag.SetAlbumArtistSort(StringUtils::Join(values, CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_musicItemSeparator));
}

void CTagLoaderTagLib::SetAlbumArtistHints(CMusicInfoTag &tag, const std::vector<std::string> &values)
{
  if (values.size() == 1)
    tag.SetMusicBrainzAlbumArtistHints(StringUtils::Split(values[0], CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_musicItemSeparator));
  else
    tag.SetMusicBrainzAlbumArtistHints(values);
}

void CTagLoaderTagLib::SetComposerSort(CMusicInfoTag &tag, const std::vector<std::string> &values)
{
  // COMPOSRSORT/TSOC tag is often a single string, when not take union of values
  if (values.size() == 1)
    tag.SetComposerSort(values[0]);
  else
    tag.SetComposerSort(StringUtils::Join(values, CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_musicItemSeparator));
}

void CTagLoaderTagLib::SetGenre(CMusicInfoTag &tag, const std::vector<std::string> &values)
{
  /*
   TagLib doesn't resolve ID3v1 genre numbers in the case were only
   a number is specified, thus this workaround.
   */
  std::vector<std::string> genres;
  for (const std::string& i : values)
  {
    std::string genre = i;
    if (StringUtils::IsNaturalNumber(genre))
    {
      int number = strtol(i.c_str(), nullptr, 10);
      if (number >= 0 && number < 256)
        genre = ID3v1::genre(number).to8Bit(true);
    }
    genres.push_back(genre);
  }
  if (genres.size() == 1)
    tag.SetGenre(genres[0], true);
  else
    tag.SetGenre(genres, true);
}

void CTagLoaderTagLib::SetReleaseType(CMusicInfoTag &tag, const std::vector<std::string> &values)
{
  if (values.size() == 1)
    tag.SetMusicBrainzReleaseType(values[0]);
  else
    tag.SetMusicBrainzReleaseType(StringUtils::Join(values, CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_musicItemSeparator));
}

void CTagLoaderTagLib::AddArtistRole(CMusicInfoTag &tag, const std::string& strRole, const std::vector<std::string> &values)
{
  if (values.size() == 1)
    tag.AddArtistRole(strRole, values[0]);
  else
    tag.AddArtistRole(strRole, values);
}

void CTagLoaderTagLib::SetDiscSubtitle(CMusicInfoTag& tag, const std::vector<std::string>& values)
{
  if (values.size() == 1)
    tag.SetDiscSubtitle(values[0]);
  else
    tag.SetDiscSubtitle(std::string());
}

void CTagLoaderTagLib::AddArtistRole(CMusicInfoTag &tag, const std::vector<std::string> &values)
{
  // Values contains role, name pairs (as in ID3 standard for TIPL or TMCL tags)
  // Every odd entry is a function (e.g. Producer, Arranger etc.) or instrument (e.g. Orchestra, Vocal, Piano)
  // and every even is an artist or a comma delimited list of artists.

  if (values.size() % 2 != 0) // Must contain an even number of entries
    return;

  // Vector of possible separators
  const std::vector<std::string> separators{ ";", "/", ",", "&", " and " };

  for (size_t i = 0; i + 1 < values.size(); i += 2)
  {
    std::vector<std::string> roles;
    //Split into individual roles
    roles = StringUtils::Split(values[i], separators);
    for (auto role : roles)
    {
      StringUtils::Trim(role);
      StringUtils::ToCapitalize(role);
      tag.AddArtistRole(role, StringUtils::Split(values[i + 1], ","));
    }
  }
}

void CTagLoaderTagLib::AddArtistInstrument(CMusicInfoTag &tag, const std::vector<std::string> &values)
{
  /* Values is a musician credits list, each entry is artist name followed by instrument (or function)
     e.g. violin, drums, background vocals, solo, orchestra etc. in brackets. This is how Picard uses
     the PERFORMER tag. Multiple instruments may be in one tag
     e.g "Pierre Marchand (bass, drum machine and hammond organ)",
     these will be separated into individual roles.
     If there is not a pair of brackets then role is "performer" by default, and the whole entry is
     taken as artist name.
  */
  // Vector of possible separators
  const std::vector<std::string> separators{";", "/", ",", "&", " and "};

  for (size_t i = 0; i < values.size(); ++i)
  {
    std::vector<std::string> roles;
    std::string strArtist = values[i];
    size_t firstLim = values[i].find_first_of('(');
    size_t lastLim = values[i].find_last_of(')');
    if (lastLim != std::string::npos && firstLim != std::string::npos && firstLim < lastLim - 1)
    {
      //Pair of brackets with something between them
      strArtist.erase(firstLim, lastLim - firstLim + 1);
      std::string strRole = values[i].substr(firstLim + 1, lastLim - firstLim - 1);
      //Split into individual roles
      roles = StringUtils::Split(strRole, separators);
    }
    StringUtils::Trim(strArtist);
    if (roles.empty())
      tag.AddArtistRole("Performer", strArtist);
    else
      for (auto role : roles)
      {
        StringUtils::Trim(role);
        StringUtils::ToCapitalize(role);
        tag.AddArtistRole(role, strArtist);
      }
  }
}

bool CTagLoaderTagLib::Load(const std::string& strFileName, CMusicInfoTag& tag, const std::string& fallbackFileExtension, EmbeddedArt *art /* = NULL */)
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
    CLog::Log(LOGERROR, "could not create TagLib VFS stream for: {}", strFileName);
    return false;
  }

  long file_length = stream->length();

  if (file_length == 0) // a stream returns zero as the length
  {
    delete stream; // scrap this instance
    return false; // and quit without attempting to read non-existent tags
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
    else if (strExtension == "mp4" || strExtension == "m4a" || strExtension == "m4v" ||
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
        file = nullptr;
        oggFlacFile = nullptr;
        file = oggVorbisFile = new Ogg::Vorbis::File(stream);
      }
    }
  }
  catch (const std::exception& ex)
  {
    CLog::Log(LOGERROR, "Taglib exception: {}", ex.what());
  }

  if (!file || !file->isOpen())
  {
    delete file;
    delete stream;
    CLog::Log(LOGDEBUG, "file {} could not be opened for tag reading", strFileName);
    return false;
  }

  APE::Tag *ape = nullptr;
  ASF::Tag *asf = nullptr;
  MP4::Tag *mp4 = nullptr;
  ID3v1::Tag *id3v1 = nullptr;
  ID3v2::Tag *id3v2 = nullptr;
  Ogg::XiphComment *xiph = nullptr;
  Tag *genericTag = nullptr;

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
    xiph = oggFlacFile->tag();
  else if (oggVorbisFile)
    xiph = oggVorbisFile->tag();
  else if (oggOpusFile)
    xiph = oggOpusFile->tag();
  else if (ttaFile)
    id3v2 = ttaFile->ID3v2Tag(false);
  else if (aiffFile)
    id3v2 = aiffFile->tag();
  else if (wavFile)
    id3v2 = wavFile->ID3v2Tag();
  else if (wvFile)
    ape = wvFile->APETag(false);
  else if (mpcFile)
    ape = mpcFile->APETag(false);
  else    // This is a catch all to get generic information for other files types (s3m, xm, it, mod, etc)
    genericTag = file->tag();

  if (file->audioProperties())
  {
    tag.SetDuration(file->audioProperties()->length());
    tag.SetBitRate(file->audioProperties()->bitrate());
    tag.SetNoOfChannels(file->audioProperties()->channels());
    tag.SetSampleRate(file->audioProperties()->sampleRate());
  }

  if (asf)
    ParseTag(asf, art, tag);
  if (id3v1)
    ParseTag(id3v1, art, tag);
  if (id3v2)
    ParseTag(id3v2, art, tag);
  if (genericTag)
    ParseTag(genericTag, art, tag);
  if (mp4)
    ParseTag(mp4, art, tag);
  if (xiph) // xiph tags override id3v2 tags in badly tagged FLACs
    ParseTag(xiph, art, tag);
  if (ape && (!id3v2 || CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_prioritiseAPEv2tags)) // ape tags override id3v2 if we're prioritising them
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
