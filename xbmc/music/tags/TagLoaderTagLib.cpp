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
#include <taglib/xiphcomment.h>
#include <taglib/id3v1genres.h>

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
#include "utils/CharsetConverter.h"
#include "utils/Base64.h"
#include "settings/AdvancedSettings.h"

using namespace std;
using namespace TagLib;
using namespace MUSIC_INFO;

template<class T>
class TagStringHandler : public T
{
public:
  TagStringHandler() {}
  virtual ~TagStringHandler() {}
  virtual String parse(const ByteVector &data) const
  {
    std::string strSource(data.data(), data.size());
    std::string strUTF8;
    g_charsetConverter.unknownToUTF8(strSource, strUTF8);
    return String(strUTF8, String::UTF8);
  }
};

static const TagStringHandler<ID3v1::StringHandler> ID3v1StringHandler;
static const TagStringHandler<ID3v2::Latin1StringHandler> ID3v2StringHandler;

CTagLoaderTagLib::CTagLoaderTagLib()
{
}

CTagLoaderTagLib::~CTagLoaderTagLib()
{
  
}

static const vector<string> StringListToVectorString(const StringList& stringList)
{
  vector<string> values;
  for (StringList::ConstIterator it = stringList.begin(); it != stringList.end(); ++it)
    values.push_back(it->to8Bit(true));
  return values;
}

bool CTagLoaderTagLib::Load(const std::string& strFileName, MUSIC_INFO::CMusicInfoTag& tag, MUSIC_INFO::EmbeddedArt *art /* = NULL */)
{
  return Load(strFileName, tag, "", art);
}

bool CTagLoaderTagLib::Load(const std::string& strFileName, CMusicInfoTag& tag, const std::string& fallbackFileExtension, MUSIC_INFO::EmbeddedArt *art /* = NULL */)
{  
  std::string strExtension = URIUtils::GetExtension(strFileName);
  StringUtils::ToLower(strExtension);
  StringUtils::TrimLeft(strExtension, ".");

  if (strExtension.empty())
  {
    strExtension = fallbackFileExtension;
    if (strExtension.empty())
      return false;
    StringUtils::ToLower(strExtension);
  }

  TagLibVFSStream*           stream = new TagLibVFSStream(strFileName, true);
  if (!stream)
  {
    CLog::Log(LOGERROR, "could not create TagLib VFS stream for: %s", strFileName.c_str());
    return false;
  }
  
  ID3v1::Tag::setStringHandler(&ID3v1StringHandler);
  ID3v2::Tag::setLatin1StringHandler(&ID3v2StringHandler);
  TagLib::File*              file = NULL;
  TagLib::APE::File*         apeFile = NULL;
  TagLib::ASF::File*         asfFile = NULL;
  TagLib::FLAC::File*        flacFile = NULL;
  TagLib::IT::File*          itFile = NULL;
  TagLib::Mod::File*         modFile = NULL;
  TagLib::MP4::File*         mp4File = NULL;
  TagLib::MPC::File*         mpcFile = NULL;
  TagLib::MPEG::File*        mpegFile = NULL;
  TagLib::Ogg::Vorbis::File* oggVorbisFile = NULL;
  TagLib::Ogg::FLAC::File*   oggFlacFile = NULL;
  TagLib::S3M::File*         s3mFile = NULL;
  TagLib::TrueAudio::File*   ttaFile = NULL;
  TagLib::WavPack::File*     wvFile = NULL;
  TagLib::XM::File*          xmFile = NULL;
  TagLib::RIFF::WAV::File *  wavFile = NULL;
  TagLib::RIFF::AIFF::File * aiffFile = NULL;

  if (strExtension == "ape")
    file = apeFile = new APE::File(stream);
  else if (strExtension == "asf" || strExtension == "wmv" || strExtension == "wma")
    file = asfFile = new ASF::File(stream);
  else if (strExtension == "flac")
    file = flacFile = new FLAC::File(stream, ID3v2::FrameFactory::instance());
  else if (strExtension == "it")
    file = itFile = new IT::File(stream);
  else if (strExtension == "mod" || strExtension == "module" || strExtension == "nst" || strExtension == "wow")
    file = modFile = new Mod::File(stream);
  else if (strExtension == "mp4" || strExtension == "m4a" || 
           strExtension == "m4r" || strExtension == "m4b" || 
           strExtension == "m4p" || strExtension == "3g2")
    file = mp4File = new MP4::File(stream);
  else if (strExtension == "mpc")
    file = mpcFile = new MPC::File(stream);
  else if (strExtension == "mp3" || strExtension == "aac")
    file = mpegFile = new MPEG::File(stream, ID3v2::FrameFactory::instance());
  else if (strExtension == "s3m")
    file = s3mFile = new S3M::File(stream);
  else if (strExtension == "tta")
    file = ttaFile = new TrueAudio::File(stream, ID3v2::FrameFactory::instance());
  else if (strExtension == "wv")
    file = wvFile = new WavPack::File(stream);
  else if (strExtension == "aif" || strExtension == "aiff")
    file = aiffFile = new RIFF::AIFF::File(stream);
  else if (strExtension == "wav")
    file = wavFile = new RIFF::WAV::File(stream);
  else if (strExtension == "xm")
    file = xmFile = new XM::File(stream);
  else if (strExtension == "ogg")
    file = oggVorbisFile = new Ogg::Vorbis::File(stream);
  else if (strExtension == "oga") // Leave this madness until last - oga container can have Vorbis or FLAC
  {
    file = oggFlacFile = new Ogg::FLAC::File(stream);
    if (!file || !file->isValid())
    {
      delete file;
      oggFlacFile = NULL;
      file = oggVorbisFile = new Ogg::Vorbis::File(stream);
    }
  }

  if (!file || !file->isOpen())
  {
    delete file;
    delete stream;
    CLog::Log(LOGDEBUG, "file could not be opened for tag reading");
    return false;
  }

  APE::Tag *ape = NULL;
  ASF::Tag *asf = NULL;
  MP4::Tag *mp4 = NULL;
  ID3v1::Tag *id3v1 = NULL;
  ID3v2::Tag *id3v2 = NULL;
  Ogg::XiphComment *xiph = NULL;
  Tag *generic = NULL;

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
  else if (ttaFile)
    id3v2 = ttaFile->ID3v2Tag(false);
  else if (aiffFile)
    id3v2 = aiffFile->tag();
  else if (wavFile)
    id3v2 = wavFile->tag();
  else if (wvFile)
    ape = wvFile->APETag(false);
  else if (mpcFile)
    ape = mpcFile->APETag(false);
  else    // This is a catch all to get generic information for other files types (s3m, xm, it, mod, etc)
    generic = file->tag();

  if (file->audioProperties())
    tag.SetDuration(file->audioProperties()->length());

  if (asf)
    ParseASF(asf, art, tag);
  if (id3v1)
    ParseID3v1Tag(id3v1, art, tag);
  if (id3v2)
    ParseID3v2Tag(id3v2, art, tag);
  if (generic)
    ParseGenericTag(generic, art, tag);
  if (mp4)
    ParseMP4Tag(mp4, art, tag);
  if (xiph) // xiph tags override id3v2 tags in badly tagged FLACs
    ParseXiphComment(xiph, art, tag);
  if (ape && (!id3v2 || g_advancedSettings.m_prioritiseAPEv2tags)) // ape tags override id3v2 if we're prioritising them
    ParseAPETag(ape, art, tag);

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

bool CTagLoaderTagLib::ParseASF(ASF::Tag *asf, EmbeddedArt *art, CMusicInfoTag& tag)
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
        art->set((const uint8_t *)pic.picture().data(), pic.picture().size(), pic.mimeType().toCString());
    }
    else if (g_advancedSettings.m_logLevel == LOG_LEVEL_MAX)
      CLog::Log(LOGDEBUG, "unrecognized ASF tag name: %s", it->first.toCString(true));
  }
  // artist may be specified in the ContentDescription block rather than using the 'Author' attribute.
  if (tag.GetArtist().empty())
    tag.SetArtist(asf->artist().toCString(true));

  tag.SetReplayGain(replayGainInfo);
  tag.SetLoaded(true);
  return true;
}

char POPMtoXBMC(int popm)
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
  if (popm == 0) return '0';
  if (popm < 0x40) return '1';
  if (popm < 0x80) return '2';
  if (popm < 0xc0) return '3';
  if (popm < 0xff) return '4';
  return '5';
}

bool CTagLoaderTagLib::ParseID3v1Tag(ID3v1::Tag *id3v1, EmbeddedArt *art, CMusicInfoTag& tag)
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

bool CTagLoaderTagLib::ParseID3v2Tag(ID3v2::Tag *id3v2, EmbeddedArt *art, CMusicInfoTag& tag)
{
  //  tag.SetURL(strFile);
  if (!id3v2) return false;

  ReplayGain replayGainInfo;

  ID3v2::AttachedPictureFrame *pictures[3] = {};
  const ID3v2::FrameListMap& frameListMap = id3v2->frameListMap();
  for (ID3v2::FrameListMap::ConstIterator it = frameListMap.begin(); it != frameListMap.end(); ++it)
  {
    if      (it->first == "TPE1")   SetArtist(tag, GetID3v2StringList(it->second));
    else if (it->first == "TALB")   tag.SetAlbum(it->second.front()->toString().to8Bit(true));
    else if (it->first == "TPE2")   SetAlbumArtist(tag, GetID3v2StringList(it->second));
    else if (it->first == "TIT2")   tag.SetTitle(it->second.front()->toString().to8Bit(true));
    else if (it->first == "TCON")   SetGenre(tag, GetID3v2StringList(it->second));
    else if (it->first == "TRCK")   tag.SetTrackNumber(strtol(it->second.front()->toString().toCString(true), NULL, 10));
    else if (it->first == "TPOS")   tag.SetDiscNumber(strtol(it->second.front()->toString().toCString(true), NULL, 10));
    else if (it->first == "TYER")   tag.SetYear(strtol(it->second.front()->toString().toCString(true), NULL, 10));
    else if (it->first == "TCMP")   tag.SetCompilation((strtol(it->second.front()->toString().toCString(true), NULL, 10) == 0) ? false : true);
    else if (it->first == "TENC")   {} // EncodedBy
    else if (it->first == "TCOP")   {} // Copyright message
    else if (it->first == "TDRC")   tag.SetYear(strtol(it->second.front()->toString().toCString(true), NULL, 10));
    else if (it->first == "TDRL")   tag.SetYear(strtol(it->second.front()->toString().toCString(true), NULL, 10));
    else if (it->first == "TDTG")   {} // Tagging time
    else if (it->first == "TLAN")   {} // Languages
    else if (it->first == "TMOO")   tag.SetMood(it->second.front()->toString().to8Bit(true));
    else if (it->first == "USLT")
      // Loop through any lyrics frames. Could there be multiple frames, how to choose?
      for (ID3v2::FrameList::ConstIterator lt = it->second.begin(); lt != it->second.end(); ++lt)
      {
        ID3v2::UnsynchronizedLyricsFrame *lyricsFrame = dynamic_cast<ID3v2::UnsynchronizedLyricsFrame *> (*lt);
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
        else if (desc == "ALBUMARTIST")
          SetAlbumArtist(tag, StringListToVectorString(stringList));
        else if (desc == "ALBUM ARTIST")
          SetAlbumArtist(tag, StringListToVectorString(stringList));
        else if (desc == "MOOD")
          tag.SetMood(stringList.front().to8Bit(true));
        else if (g_advancedSettings.m_logLevel == LOG_LEVEL_MAX)
          CLog::Log(LOGDEBUG, "unrecognized user text tag detected: TXXX:%s", frame->description().toCString(true));
      }
    else if (it->first == "UFID")
      // Loop through any UFID frames and set them
      for (ID3v2::FrameList::ConstIterator ut = it->second.begin(); ut != it->second.end(); ++ut)
      {
        ID3v2::UniqueFileIdentifierFrame *ufid = reinterpret_cast<ID3v2::UniqueFileIdentifierFrame*> (*ut);
        if (ufid->owner() == "http://musicbrainz.org")
        {
          // MusicBrainz pads with a \0, but the spec requires binary, be cautious
          char cUfid[64];
          int max_size = std::min((int)ufid->identifier().size(), 63);
          strncpy(cUfid, ufid->identifier().data(), max_size);
          cUfid[max_size] = '\0';
          tag.SetMusicBrainzTrackID(cUfid);
        }
      }
    else if (it->first == "APIC")
      // Loop through all pictures and store the frame pointers for the picture types we want
      for (ID3v2::FrameList::ConstIterator pi = it->second.begin(); pi != it->second.end(); ++pi)
      {
        ID3v2::AttachedPictureFrame *pictureFrame = dynamic_cast<ID3v2::AttachedPictureFrame *> (*pi);
        if (!pictureFrame) continue;
        
        if      (pictureFrame->type() == ID3v2::AttachedPictureFrame::FrontCover) pictures[0] = pictureFrame;
        else if (pictureFrame->type() == ID3v2::AttachedPictureFrame::Other)      pictures[1] = pictureFrame;
        else if (pi == it->second.begin())                                        pictures[2] = pictureFrame;
      }
    else if (it->first == "POPM")
      // Loop through and process ratings
      for (ID3v2::FrameList::ConstIterator ct = it->second.begin(); ct != it->second.end(); ++ct)
      {
        ID3v2::PopularimeterFrame *popFrame = dynamic_cast<ID3v2::PopularimeterFrame *> (*ct);
        if (!popFrame) continue;
        
        // @xbmc.org ratings trump others (of course)
        if      (popFrame->email() == "ratings@xbmc.org")
          tag.SetRating(popFrame->rating() / 51 + '0');
        else if (tag.GetRating() == '0')
        {
          if (popFrame->email() != "Windows Media Player 9 Series" &&
              popFrame->email() != "Banshee" &&
              popFrame->email() != "no@email" &&
              popFrame->email() != "quodlibet@lists.sacredchao.net" &&
              popFrame->email() != "rating@winamp.com")
            CLog::Log(LOGDEBUG, "unrecognized ratings schema detected: %s", popFrame->email().toCString(true));
          tag.SetRating(POPMtoXBMC(popFrame->rating()));
        }
      }
    else if (g_advancedSettings.m_logLevel == LOG_LEVEL_MAX)
      CLog::Log(LOGDEBUG, "unrecognized ID3 frame detected: %c%c%c%c", it->first[0], it->first[1], it->first[2], it->first[3]);
  } // for

  // Process the extracted picture frames; 0 = CoverArt, 1 = Other, 2 = First Found picture
  for (int i = 0; i < 3; ++i)
    if (pictures[i])
    {
      string      mime =             pictures[i]->mimeType().to8Bit(true);
      TagLib::uint size =            pictures[i]->picture().size();
      tag.SetCoverArtInfo(size, mime);
      if (art)
        art->set((const uint8_t*)pictures[i]->picture().data(), size, mime);
      
      // Stop after we find the first picture for now.
      break;
    }

  tag.SetReplayGain(replayGainInfo);
  return true;
}

bool CTagLoaderTagLib::ParseAPETag(APE::Tag *ape, EmbeddedArt *art, CMusicInfoTag& tag)
{
  if (!ape)
    return false;

  ReplayGain replayGainInfo;
  const APE::ItemListMap itemListMap = ape->itemListMap();
  for (APE::ItemListMap::ConstIterator it = itemListMap.begin(); it != itemListMap.end(); ++it)
  {
    if (it->first == "ARTIST")
      SetArtist(tag, StringListToVectorString(it->second.toStringList()));
    else if (it->first == "ALBUM ARTIST" || it->first == "ALBUMARTIST")
      SetAlbumArtist(tag, StringListToVectorString(it->second.toStringList()));
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
    else if (it->first == "COMMENT")
      tag.SetComment(it->second.toString().to8Bit(true));
    else if (it->first == "CUESHEET")
      tag.SetCueSheet(it->second.toString().to8Bit(true));
    else if (it->first == "ENCODEDBY")
    {}
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

bool CTagLoaderTagLib::ParseXiphComment(Ogg::XiphComment *xiph, EmbeddedArt *art, CMusicInfoTag& tag)
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
    else if (it->first == "ALBUMARTIST")
      SetAlbumArtist(tag, StringListToVectorString(it->second));
    else if (it->first == "ALBUM ARTIST")
      SetAlbumArtist(tag, StringListToVectorString(it->second));
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
    else if (it->first == "COMMENT")
      tag.SetComment(it->second.front().to8Bit(true));
    else if (it->first == "CUESHEET")
      tag.SetCueSheet(it->second.front().to8Bit(true));
    else if (it->first == "ENCODEDBY")
    {}
    else if (it->first == "ENSEMBLE")
    {}
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
      int iRating = it->second.front().toInt();
      if (iRating > 0 && iRating <= 100)
        tag.SetRating((iRating / 20) + '0');
    }
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
    else if (g_advancedSettings.m_logLevel == LOG_LEVEL_MAX)
      CLog::Log(LOGDEBUG, "unrecognized XipComment name: %s", it->first.toCString(true));
  }

  // Process the extracted picture frames; 0 = CoverArt, 1 = Other, 2 = COVERART/COVERARTMIME
  for (int i = 0; i < 3; ++i)
    if (pictures[i].data().size())
    {
      string      mime =             pictures[i].mimeType().toCString();
      if (mime.compare(0, 6, "image/") != 0)
        continue;
      TagLib::uint size =            pictures[i].data().size();
      tag.SetCoverArtInfo(size, mime);
      if (art)
        art->set((const uint8_t*)pictures[i].data().data(), size, mime);

      break;
    }

  tag.SetReplayGain(replayGainInfo);
  return true;
}

bool CTagLoaderTagLib::ParseMP4Tag(MP4::Tag *mp4, EmbeddedArt *art, CMusicInfoTag& tag)
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
    else if (it->first == "\251alb")
      tag.SetAlbum(it->second.toStringList().front().to8Bit(true));
    else if (it->first == "aART")
      SetAlbumArtist(tag, StringListToVectorString(it->second.toStringList()));
    else if (it->first == "\251gen")
      SetGenre(tag, StringListToVectorString(it->second.toStringList()));
    else if (it->first == "\251cmt")
      tag.SetComment(it->second.toStringList().front().to8Bit(true));
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
        string mime;
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
          art->set((const uint8_t *)pt->data().data(), pt->data().size(), mime);
        break; // one is enough
      }
    }
  }

  tag.SetReplayGain(replayGainInfo);
  return true;
}

bool CTagLoaderTagLib::ParseGenericTag(Tag *generic, EmbeddedArt *art, CMusicInfoTag& tag)
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

void CTagLoaderTagLib::SetFlacArt(FLAC::File *flacFile, EmbeddedArt *art, CMusicInfoTag &tag)
{
  FLAC::Picture *cover[2] = {};
  List<FLAC::Picture *> pictures = flacFile->pictureList();
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
        art->set((const uint8_t*)cover[i]->data().data(), cover[i]->data().size(), cover[i]->mimeType().to8Bit(true));
      return; // one is enough
    }
  }
}

const vector<string> CTagLoaderTagLib::GetASFStringList(const List<ASF::Attribute>& list)
{
  vector<string> values;
  for (List<ASF::Attribute>::ConstIterator at = list.begin(); at != list.end(); ++at)
    values.push_back(at->toString().to8Bit(true));
  return values;
}

const vector<string> CTagLoaderTagLib::GetID3v2StringList(const ID3v2::FrameList& frameList) const
{
  const ID3v2::TextIdentificationFrame *frame = dynamic_cast<ID3v2::TextIdentificationFrame *>(frameList.front());
  if (frame)
    return StringListToVectorString(frame->fieldList());
  return vector<string>();
}

void CTagLoaderTagLib::SetArtist(CMusicInfoTag &tag, const vector<string> &values)
{
  if (values.size() == 1)
    tag.SetArtist(values[0]);
  else
    tag.SetArtist(values);
}

const std::vector<std::string> CTagLoaderTagLib::SplitMBID(const std::vector<std::string> &values)
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

void CTagLoaderTagLib::SetAlbumArtist(CMusicInfoTag &tag, const vector<string> &values)
{
  if (values.size() == 1)
    tag.SetAlbumArtist(values[0]);
  else
    tag.SetAlbumArtist(values);
}

void CTagLoaderTagLib::SetGenre(CMusicInfoTag &tag, const vector<string> &values)
{
  /*
   TagLib doesn't resolve ID3v1 genre numbers in the case were only
   a number is specified, thus this workaround.
   */
  vector<string> genres;
  for (vector<string>::const_iterator i = values.begin(); i != values.end(); ++i)
  {
    string genre = *i;
    if (StringUtils::IsNaturalNumber(genre))
    {
      int number = strtol(i->c_str(), NULL, 10);
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
