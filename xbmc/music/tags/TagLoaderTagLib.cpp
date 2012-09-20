/*
 *      Copyright (C) 2005-2012 Team XBMC
 *      http://www.xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "TagLoaderTagLib.h"

#include <vector>

#include <taglib/id3v1tag.h>
#include <taglib/id3v2tag.h>
#include <taglib/apetag.h>
#include <taglib/xiphcomment.h>

#include <taglib/textidentificationframe.h>
#include <taglib/uniquefileidentifierframe.h>
#include <taglib/popularimeterframe.h>
#include <taglib/commentsframe.h>
#include <taglib/unsynchronizedlyricsframe.h>
#include <taglib/attachedpictureframe.h>

#undef byte
#include <taglib/tstring.h>
#include <taglib/tpropertymap.h>

#include "TagLibVFSStream.h"
#include "MusicInfoTag.h"
#include "utils/URIUtils.h"
#include "utils/log.h"
#include "utils/StringUtils.h"
#include "settings/AdvancedSettings.h"

using namespace std;
using namespace TagLib;
using namespace MUSIC_INFO;

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
    values.push_back(it->toCString(true));
  return values;
}

bool CTagLoaderTagLib::Load(const string& strFileName, CMusicInfoTag& tag, EmbeddedArt *art /* = NULL */)
{  
  CStdString strExtension;
  URIUtils::GetExtension(strFileName, strExtension);
  strExtension.ToLower();
  strExtension.TrimLeft('.');

  if (strExtension.IsEmpty())
    return false;

  TagLibVFSStream*           stream = new TagLibVFSStream(strFileName, true);
  if (!stream)
  {
    CLog::Log(LOGERROR, "could not create TagLib VFS stream for: %s", strFileName.c_str());
    return false;
  }
  
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
    id3v2 = mpegFile->ID3v2Tag(false);
    ape = mpegFile->APETag(false);
  }
  else if (oggFlacFile)
    xiph = dynamic_cast<Ogg::XiphComment *>(oggFlacFile->tag());
  else if (oggVorbisFile)
    xiph = dynamic_cast<Ogg::XiphComment *>(oggVorbisFile->tag());
  else if (ttaFile)
    id3v2 = ttaFile->ID3v2Tag(false);
  else if (wvFile)
    ape = wvFile->APETag(false);
  else    // This is a catch all to get generic information for other files types (s3m, xm, it, mod, etc)
    generic = file->tag();

  if (file->audioProperties())
    tag.SetDuration(file->audioProperties()->length());

  if (ape && !g_advancedSettings.m_prioritiseAPEv2tags)
    ParseAPETag(ape, art, tag);
  else if (asf)
    ParseASF(asf, art, tag);
  else if (id3v2)
    ParseID3v2Tag(id3v2, art, tag);
  else if (generic)
    ParseGenericTag(generic, art, tag);
  else if (mp4)
    ParseMP4Tag(mp4, art, tag);
  else if (xiph)
    ParseXiphComment(xiph, art, tag);

  // Add APE tags over the top of ID3 tags if we want to prioritize them
  if (ape && g_advancedSettings.m_prioritiseAPEv2tags)
    ParseAPETag(ape, art, tag);

  if (!tag.GetTitle().IsEmpty() || !tag.GetArtist().empty() || !tag.GetAlbum().IsEmpty())
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

  const ASF::AttributeListMap& attributeListMap = asf->attributeListMap();
  for (ASF::AttributeListMap::ConstIterator it = attributeListMap.begin(); it != attributeListMap.end(); ++it)
  {
    if (it->first == "Author")                           tag.SetArtist(GetASFStringList(it->second));
    else if (it->first == "WM/AlbumArtist")              tag.SetAlbumArtist(GetASFStringList(it->second));
    else if (it->first == "WM/AlbumTitle")               tag.SetAlbum(it->second.front().toString().toCString());
    else if (it->first == "WM/TrackNumber")              tag.SetTrackNumber(it->second.front().toUInt());
    else if (it->first == "WM/PartOfSet")                tag.SetPartOfSet(it->second.front().toUInt());
    else if (it->first == "WM/Genre")                    tag.SetGenre(GetASFStringList(it->second));
    else if (it->first == "WM/AlbumArtistSortOrder")     {} // Known unsupported, supress warnings
    else if (it->first == "WM/ArtistSortOrder")          {} // Known unsupported, supress warnings
    else if (it->first == "WM/Script")                   {} // Known unsupported, supress warnings
    else if (it->first == "MusicBrainz/Artist Id")       tag.SetMusicBrainzArtistID(it->second.front().toString().toCString());
    else if (it->first == "MusicBrainz/Album Id")        tag.SetMusicBrainzAlbumID(it->second.front().toString().toCString());
    else if (it->first == "MusicBrainz/Album Artist Id") tag.SetMusicBrainzAlbumArtistID(it->second.front().toString().toCString());
    else if (it->first == "MusicBrainz/Track Id")        tag.SetMusicBrainzTrackID(it->second.front().toString().toCString());
    else if (it->first == "MusicBrainz/Album Status")    {}
    else if (it->first == "MusicBrainz/Album Type")      {}
    else if (it->first == "MusicIP/PUID")                {}
    else
      CLog::Log(LOGDEBUG, "unrecognized ASF tag name: %s", it->first.toCString());
  }
  tag.SetLoaded(true);
  return true;
}

bool CTagLoaderTagLib::ParseID3v2Tag(ID3v2::Tag *id3v2, EmbeddedArt *art, CMusicInfoTag& tag)
{
  // Notes:
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

  //  tag.SetURL(strFile);
  if (!id3v2) return false;

  ID3v2::AttachedPictureFrame *pictures[3] = {};
  const ID3v2::FrameListMap& frameListMap = id3v2->frameListMap();
  for (ID3v2::FrameListMap::ConstIterator it = frameListMap.begin(); it != frameListMap.end(); ++it)
  {
    if      (it->first == "TPE1")   tag.SetArtist(GetID3v2StringList(it->second));
    else if (it->first == "TALB")   tag.SetAlbum(it->second.front()->toString().toCString(true));
    else if (it->first == "TPE2")   tag.SetAlbumArtist(GetID3v2StringList(it->second));
    else if (it->first == "TIT2")   tag.SetTitle(it->second.front()->toString().toCString(true));
    else if (it->first == "TCON")   tag.SetGenre(it->second.front()->toString().toCString(true));
    else if (it->first == "TRCK")   tag.SetTrackNumber(strtol(it->second.front()->toString().toCString(true), NULL, 10));
    else if (it->first == "TPOS")   tag.SetPartOfSet(strtol(it->second.front()->toString().toCString(true), NULL, 10));
    else if (it->first == "TYER")   tag.SetYear(strtol(it->second.front()->toString().toCString(true), NULL, 10));
    else if (it->first == "TCMP")   tag.SetCompilation((strtol(it->second.front()->toString().toCString(true), NULL, 10) == 0) ? false : true);
    else if (it->first == "TENC")   {} // EncodedBy
    else if (it->first == "USLT")
      // Loop through any lyrics frames. Could there be multiple frames, how to choose?
      for (ID3v2::FrameList::ConstIterator lt = it->second.begin(); lt != it->second.end(); ++lt)
      {
        ID3v2::UnsynchronizedLyricsFrame *lyricsFrame = dynamic_cast<ID3v2::UnsynchronizedLyricsFrame *> (*lt);
        if (lyricsFrame)           
          tag.SetLyrics(lyricsFrame->text().toCString());
      }
    else if (it->first == "COMM")
      // Loop through and look for the main (no description) comment
      for (ID3v2::FrameList::ConstIterator ct = it->second.begin(); ct != it->second.end(); ++ct)
      {
        ID3v2::CommentsFrame *commentsFrame = dynamic_cast<ID3v2::CommentsFrame *> (*ct);
        if (commentsFrame && commentsFrame->description().isEmpty())
          tag.SetComment(commentsFrame->text().toCString());
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
        if      (frame->description() == "MusicBrainz Artist Id")       tag.SetMusicBrainzArtistID(stringList.front().toCString());
        else if (frame->description() == "MusicBrainz Album Id")        tag.SetMusicBrainzAlbumID(stringList.front().toCString());
        else if (frame->description() == "MusicBrainz Album Artist Id") tag.SetMusicBrainzAlbumArtistID(stringList.front().toCString());
        else if (frame->description() == "replaygain_track_gain")       tag.SetReplayGainTrackGain((int)(atof(stringList.front().toCString()) * 100 + 0.5));
        else if (frame->description() == "replaygain_album_gain")       tag.SetReplayGainAlbumGain((int)(atof(stringList.front().toCString()) * 100 + 0.5));
        else if (frame->description() == "replaygain_track_peak")       tag.SetReplayGainTrackPeak((float)atof(stringList.front().toCString()));
        else if (frame->description() == "replaygain_album_peak")       tag.SetReplayGainAlbumPeak((float)atof(stringList.front().toCString()));
        else
          CLog::Log(LOGDEBUG, "unrecognized user text tag detected: TXXX:%s", frame->description().toCString());
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
        else if (popFrame->email() == "Windows Media Player 9 Series" && tag.GetRating() == '0')  
          tag.SetRating(popFrame->rating() / 51 + '0');
        else if (popFrame->email() == "no@email" && tag.GetRating() == '0')                       
          tag.SetRating(popFrame->rating() / 51 + '0');
        else if (popFrame->email() == "quodlibet@lists.sacredchao.net" && tag.GetRating() == '0') 
          tag.SetRating(popFrame->rating() / 51 + '0');
        else
          CLog::Log(LOGDEBUG, "unrecognized ratings schema detected: %s", popFrame->email().toCString());
      }
    else
      CLog::Log(LOGDEBUG, "unrecognized ID3 frame detected: %c%c%c%c", it->first[0], it->first[1], it->first[2], it->first[3]);
  } // for

  // Process the extracted picture frames; 0 = CoverArt, 1 = Other, 2 = First Found picture
  for (int i = 0; i < 3; ++i)
    if (pictures[i])
    {
      string      mime =             pictures[i]->mimeType().toCString();
      TagLib::uint size =            pictures[i]->picture().size();
      uint8_t*    data  = (uint8_t*) pictures[i]->picture().data();
      tag.SetCoverArtInfo(size, mime);
      if (art)
        art->set(data, size, mime);
      
      // Stop after we find the first picture for now.
      break;
    }
  return true;
}

bool CTagLoaderTagLib::ParseAPETag(APE::Tag *ape, EmbeddedArt *art, CMusicInfoTag& tag)
{
  if (!ape)
    return false;

  const APE::ItemListMap itemListMap = ape->itemListMap();
  for (APE::ItemListMap::ConstIterator it = itemListMap.begin(); it != itemListMap.end(); ++it)
  {
    if (it->first == "ARTIST")                         tag.SetArtist(StringListToVectorString(it->second.toStringList()));
    else if (it->first == "ALBUM ARTIST")              tag.SetAlbumArtist(StringListToVectorString(it->second.toStringList()));
    else if (it->first == "ALBUM")                     tag.SetAlbum(it->second.toString().toCString());
    else if (it->first == "TITLE")                     tag.SetTitle(it->second.toString().toCString());
    else if (it->first == "TRACKNUMBER")               tag.SetTrackNumber(it->second.toString().toInt());
    else if (it->first == "DISCNUMBER")                tag.SetPartOfSet(it->second.toString().toInt());
    else if (it->first == "YEAR")                      tag.SetYear(it->second.toString().toInt());
    else if (it->first == "GENRE")                     tag.SetGenre(StringListToVectorString(it->second.toStringList()));
    else if (it->first == "COMMENT")                   tag.SetComment(it->second.toString().toCString());
    else if (it->first == "ENCODEDBY")                 {}
    else if (it->first == "COMPILATION")               tag.SetCompilation(it->second.toString().toInt() == 1);
    else if (it->first == "LYRICS")                    tag.SetLyrics(it->second.toString().toCString());
    else if (it->first == "REPLAYGAIN_TRACK_GAIN")     tag.SetReplayGainTrackGain((int)(atof(it->second.toString().toCString()) * 100 + 0.5));
    else if (it->first == "REPLAYGAIN_ALBUM_GAIN")     tag.SetReplayGainAlbumGain((int)(atof(it->second.toString().toCString()) * 100 + 0.5));
    else if (it->first == "REPLAYGAIN_TRACK_PEAK")     tag.SetReplayGainTrackPeak((float)atof(it->second.toString().toCString()));
    else if (it->first == "REPLAYGAIN_ALBUM_PEAK")     tag.SetReplayGainAlbumPeak((float)atof(it->second.toString().toCString()));
    else if (it->first == "MUSICBRAINZ_ARTISTID")      tag.SetMusicBrainzArtistID(it->second.toString().toCString());
    else if (it->first == "MUSICBRAINZ_ALBUMARTISTID") tag.SetMusicBrainzAlbumArtistID(it->second.toString().toCString());
    else if (it->first == "MUSICBRAINZ_ALBUMID")       tag.SetMusicBrainzAlbumID(it->second.toString().toCString());
    else if (it->first == "MUSICBRAINZ_TRACKID")       tag.SetMusicBrainzTrackID(it->second.toString().toCString());
    else
      CLog::Log(LOGDEBUG, "unrecognized APE tag: %s", it->first.toCString());
  }

  return true;
}

bool CTagLoaderTagLib::ParseXiphComment(Ogg::XiphComment *xiph, EmbeddedArt *art, CMusicInfoTag& tag)
{
  if (!xiph)
    return false;

  const Ogg::FieldListMap& fieldListMap = xiph->fieldListMap();
  for (Ogg::FieldListMap::ConstIterator it = fieldListMap.begin(); it != fieldListMap.end(); ++it)
  {
    if (it->first == "ARTIST")                         tag.SetArtist(StringListToVectorString(it->second));
    else if (it->first == "ALBUMARTIST")               tag.SetAlbumArtist(StringListToVectorString(it->second));
    else if (it->first == "ALBUM")                     tag.SetAlbum(it->second.front().toCString());
    else if (it->first == "TITLE")                     tag.SetTitle(it->second.front().toCString());
    else if (it->first == "TRACKNUMBER")               tag.SetTrackNumber(it->second.front().toInt());
    else if (it->first == "DISCNUMBER")                tag.SetPartOfSet(it->second.front().toInt());
    else if (it->first == "YEAR")                      tag.SetYear(it->second.front().toInt());
    else if (it->first == "GENRE")                     tag.SetGenre(StringListToVectorString(it->second));
    else if (it->first == "COMMENT")                   tag.SetComment(it->second.front().toCString());
    else if (it->first == "ENCODEDBY")                 {}
    else if (it->first == "COMPILATION")               tag.SetCompilation(it->second.front().toInt() == 1);
    else if (it->first == "LYRICS")                    tag.SetLyrics(it->second.front().toCString());
    else if (it->first == "REPLAYGAIN_TRACK_GAIN")     tag.SetReplayGainTrackGain((int)(atof(it->second.front().toCString()) * 100 + 0.5));
    else if (it->first == "REPLAYGAIN_ALBUM_GAIN")     tag.SetReplayGainAlbumGain((int)(atof(it->second.front().toCString()) * 100 + 0.5));
    else if (it->first == "REPLAYGAIN_TRACK_PEAK")     tag.SetReplayGainTrackPeak((float)atof(it->second.front().toCString()));
    else if (it->first == "REPLAYGAIN_ALBUM_PEAK")     tag.SetReplayGainAlbumPeak((float)atof(it->second.front().toCString()));
    else if (it->first == "MUSICBRAINZ_ARTISTID")      tag.SetMusicBrainzArtistID(it->second.front().toCString());
    else if (it->first == "MUSICBRAINZ_ALBUMARTISTID") tag.SetMusicBrainzAlbumArtistID(it->second.front().toCString());
    else if (it->first == "MUSICBRAINZ_ALBUMID")       tag.SetMusicBrainzAlbumID(it->second.front().toCString());
    else if (it->first == "MUSICBRAINZ_TRACKID")       tag.SetMusicBrainzTrackID(it->second.front().toCString());
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
    else
      CLog::Log(LOGDEBUG, "unrecognized XipComment name: %s", it->first.toCString());
  }

  return true;
}

bool CTagLoaderTagLib::ParseMP4Tag(MP4::Tag *mp4, EmbeddedArt *art, CMusicInfoTag& tag)
{
  if (!mp4)
    return false;

  MP4::ItemListMap& itemListMap = mp4->itemListMap();
  for (MP4::ItemListMap::ConstIterator it = itemListMap.begin(); it != itemListMap.end(); ++it)
  {
    if (it->first == "\251nam")      tag.SetTitle(it->second.toStringList().front().toCString());
    else if (it->first == "\251ART") tag.SetArtist(StringListToVectorString(it->second.toStringList()));
    else if (it->first == "\251alb") tag.SetAlbum(it->second.toStringList().front().toCString());
    else if (it->first == "aART")    tag.SetAlbumArtist(StringListToVectorString(it->second.toStringList()));
    else if (it->first == "\251gen") tag.SetGenre(StringListToVectorString(it->second.toStringList()));
    else if (it->first == "\251cmt") tag.SetComment(it->second.toStringList().front().toCString());
    else if (it->first == "cpil")    tag.SetCompilation(it->second.toBool());
    else if (it->first == "trkn")    tag.SetTrackNumber(it->second.toIntPair().first);
    else if (it->first == "disk")    tag.SetPartOfSet(it->second.toIntPair().first);
    else if (it->first == "\251day") tag.SetYear(it->second.toStringList().front().toInt());
    else if (it->first == "----:com.apple.iTunes:MusicBrainz Artist Id")
      tag.SetMusicBrainzArtistID(it->second.toStringList().front().toCString());
    else if (it->first == "----:com.apple.iTunes:MusicBrainz Album Artist Id")
      tag.SetMusicBrainzAlbumArtistID(it->second.toStringList().front().toCString());
    else if (it->first == "----:com.apple.iTunes:MusicBrainz Album Id")
      tag.SetMusicBrainzAlbumID(it->second.toStringList().front().toCString());
    else if (it->first == "----:com.apple.iTunes:MusicBrainz Track Id")
      tag.SetMusicBrainzTrackID(it->second.toStringList().front().toCString());
    else if (it->first == "covr")
    {
      MP4::CoverArtList coverArtList = it->second.toCoverArtList();
      for (MP4::CoverArtList::ConstIterator pt = coverArtList.begin(); pt != coverArtList.end(); ++pt)
      {
        string   mime =             pt->format() == MP4::CoverArt::PNG ? "image/png" : "image/jpeg";
        size_t   size =             pt->data().size();
        uint8_t* data = (uint8_t *) pt->data().data();
        tag.SetCoverArtInfo(size, mime);
        if (art)
          art->set(data, size, mime);
      }
    }
  }

  return true;
}

bool CTagLoaderTagLib::ParseGenericTag(Tag *generic, EmbeddedArt *art, CMusicInfoTag& tag)
{
  if (!generic)
    return false;

  PropertyMap properties = generic->properties();
  for (PropertyMap::ConstIterator it = properties.begin(); it != properties.end(); ++it)
  {
    if (it->first == "ARTIST")                         tag.SetArtist(StringListToVectorString(it->second));
    else if (it->first == "ALBUM")                     tag.SetArtist(it->second.front().toCString());
    else if (it->first == "TITLE")                     tag.SetTitle(it->second.front().toCString());
    else if (it->first == "TRACKNUMBER")               tag.SetTrackNumber(it->second.front().toInt());
    else if (it->first == "YEAR")                      tag.SetYear(it->second.front().toInt());
    else if (it->first == "GENRE")                     tag.SetGenre(StringListToVectorString(it->second));
    else if (it->first == "COMMENT")                   tag.SetGenre(it->second.front().toCString());
  }

  return true;
}

const vector<string> CTagLoaderTagLib::GetASFStringList(const List<ASF::Attribute>& list)
{
  vector<string> values;
  for (List<ASF::Attribute>::ConstIterator at = list.begin(); at != list.end(); ++at)
    values.push_back(at->toString().toCString());
  return values;
}

const vector<string> CTagLoaderTagLib::GetID3v2StringList(const ID3v2::FrameList& frameList) const
{
  const ID3v2::TextIdentificationFrame *frame = dynamic_cast<ID3v2::TextIdentificationFrame *>(frameList.front());
  if (frame)
    return StringListToVectorString(frame->fieldList());
  return vector<string>();
}
