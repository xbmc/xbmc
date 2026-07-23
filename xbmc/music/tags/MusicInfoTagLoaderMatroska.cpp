/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "MusicInfoTagLoaderMatroska.h"

#include "MatroskaTagLibStream.h"
#include "MusicCodecInfoFFmpeg.h"
#include "MusicInfoTag.h"
#include "ServiceBroker.h"
#include "music/MusicEmbeddedCoverLoaderFFmpeg.h"
#include "settings/AdvancedSettings.h"
#include "settings/SettingsComponent.h"
#include "utils/EmbeddedArt.h"
#include "utils/StringUtils.h"
#include "utils/log.h"

#include <algorithm>
#include <array>
#include <cstdlib>
#include <exception>
#include <iterator>
#include <map>
#include <memory>
#include <string>
#include <tuple>
#include <vector>

#include <commons/ilog.h>
#include <taglib/audioproperties.h>
#include <taglib/matroskachapteredition.h>
#include <taglib/matroskachapters.h>
#include <taglib/matroskafile.h>
#include <taglib/matroskasimpletag.h>
#include <taglib/matroskatag.h>
#include <taglib/tlist.h>
#include <taglib/tstring.h>

using namespace MUSIC_INFO;
using namespace XFILE;
using namespace TagLib;

/*!
* Read embedded cover art from attachments using TagLib (performance issue in TagLib 2.3 
* need to be resolved for Matroska large files with large attachments over SMB/NFS.
* This should be done in TagLib 2.3.1 (there is a PR open to fix this). Once resolved, we
* can drop the FFmpeg embedded cover art loading and just use TagLib for Matroska files.
* This is a static method that can will be used once 2.3.1 is released before Piers final  

static void GetMatroskaEmbeddedCover(TagLib::Matroska::File& matroskaFile,
                                     CMusicInfoTag& tag,
                                     EmbeddedArt* art = nullptr)
{
  TagLib::Matroska::Attachments* attachments = matroskaFile.attachments();
  if (!attachments)
    return;

  const auto& attachedFiles = attachments->attachedFileList();
  for (const auto& file : attachedFiles)
  {
    std::string mimeType = file.mediaType().toCString(true);
    if (mimeType == "image/jpeg" || mimeType == "image/png" || mimeType == "image/bmp")
    {
      const TagLib::ByteVector& data = file.data();
      if (data.isEmpty())
        continue;

      tag.SetCoverArtInfo(data.size(), mimeType);
      if (art)
        art->Set(reinterpret_cast<const uint8_t*>(data.data()), data.size(), mimeType, "thumb");
      break; // just need one cover
    }
  }
}
*/

namespace
{
const std::vector<std::string> SupportedArtistMultiValueSeparators = {";", "|"};
const std::vector<std::string> SupportedMultiValueSeparators = {";", "/", "|", ","};
} // namespace

/*!
* Translate multiple single key tags (Matrosk spec) to delimited a single for internal use.
* Appends " / " + newValue to currentValue if newValue is not already present
* (case-insensitive) among the existing delimited values. The set of delimiters
* used to split currentValue depends on whether tagname refers to an artist tag.
* Returns true if the value was appended, false otherwise.
*/
static bool AppendIfNotDuplicate(std::string& currentValue,
                                 const std::string& newValue,
                                 const std::string& tagname)
{
  const std::vector<std::string>& separators = (tagname.find("ARTIST") != std::string::npos)
                                                   ? SupportedArtistMultiValueSeparators
                                                   : SupportedMultiValueSeparators;

  try
  {
    std::vector<std::string> existingValues = StringUtils::Split(currentValue, separators);

    for (auto& existing : existingValues)
    {
      StringUtils::Trim(existing);
      if (existing.empty())
        continue; // mirrors RemoveEmptyEntries
      if (StringUtils::EqualsNoCase(existing, newValue))
        return false;
    }
  }
  catch (const std::exception& ex)
  {
    CLog::Log(LOGERROR, "AppendIfNotDuplicate: {}", ex.what());
    return false;
  }

  if (currentValue.empty())
    currentValue = newValue;
  else
    currentValue += " / " + newValue;

  return true;
}

/*!
* Used by Matroska files with no chapters (most common) or with a single (one song)
* Typically these are Matroska files split by Chapter start times with each chapter having
* song tags but no chapter name or chapter tags.
*/
bool CMusicInfoTagLoaderMatroska::Load(const std::string& strFileName,
                                       CMusicInfoTag& tag,
                                       EmbeddedArt* art)
{
  tag.SetLoaded(false);

  MatroskaTagLibStream matroskaStream(strFileName);
  if (!matroskaStream.open())
    return false;

  std::vector<std::string> separators{";", " feat. ", " ft. ", " Feat. ", " Ft. ", ":",
                                      "|", "#",       "/",     " with ",  "&"};
  std::string musicsep =
      CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_musicItemSeparator;
  if (musicsep.find_first_of(";/,&|#") == std::string::npos)
    separators.push_back(musicsep);

  // Get tags, chapters, embedded cover art, and duration in one call
  // (single file parse — avoids opening the Matroska file twice)
  std::map<std::string, std::string> fileTags;
  std::map<unsigned long long, std::map<std::string, std::string>> chapterTags;
  std::vector<std::tuple<unsigned long long, std::string, double, double, unsigned long long>>
      chapterOrder;
  GetMatroskaMusicTags(strFileName, matroskaStream, fileTags, chapterTags, chapterOrder, &tag, art);

  if (fileTags.empty())
    return true;
  for (const auto& t : fileTags)
    ParseTag(t.first, t.second, separators, musicsep, tag);
  /*!
  * now process the Chapter (track) if the Matroska file has a chapter
  * there is usually no chapters but there could be one, if > 1 
  * the file is processed as a whole album by CAudioBookFileDirectory
  */
  if (!chapterOrder.empty())
  {
    auto it = chapterTags.find(std::get<0>(chapterOrder[0]));
    if (it != chapterTags.end())
    {
      for (const auto& t : it->second)
        ParseTag(t.first, t.second, separators, musicsep, tag);
    }
  }

  // Look for any embedded cover art
  CMusicEmbeddedCoverLoaderFFmpeg::GetEmbeddedCover(strFileName, tag, art);

  // Get Codec data using FFmpeg (taglib not accurate for all codecs yet - v2.3)
  bool haveFFmpegInfo = false;
  musicCodecInfo codec_info;
  haveFFmpegInfo = CMusicCodecInfoFFmpeg::GetMusicCodecInfo(strFileName, codec_info);
  if (haveFFmpegInfo)
  {
    tag.SetBitRate(codec_info.bitRate);
    tag.SetSampleRate(codec_info.sampleRate);
    /*!
    * Additional Music properties (next PR)
    * albumtag.SetBitsPerSample(codec_info.bitsPerSample);
    * albumtag.SetCodec(codec_info.codecName); // e.g. 'truehd_atmos', 'dts_ma', 'dts_hd', etc
    */
    tag.SetNoOfChannels(codec_info.channels);
    tag.SetDuration(codec_info.duration);
  }

  if (!tag.GetAlbum().empty() || !tag.GetTitle().empty())
    tag.SetLoaded(true);

  return true;
}

void CMusicInfoTagLoaderMatroska::ParseTag(const std::string& key,
                                           const std::string& value,
                                           std::vector<std::string>& separators,
                                           const std::string& musicsep,
                                           CMusicInfoTag& tag)
{
  /*!
  * Matroska Tag spec does not allow storing multi values in a single tag, but some tools
  * do it anyway using a delimiter. So we need to split the value using the separator and
  * then join it back using the music item separator from as.xml if needed.
  */
  if (key == "ALBUM")
    tag.SetAlbum(value);
  else if (key == "ARTIST")
    // tag.SetArtist(StringUtils::Join(StringUtils::Split(value, separators), musicsep));
    tag.SetArtist(value);
  else if (key == "ARTISTS")
    tag.SetMusicBrainzArtistHints(StringUtils::Split(value, separators));
  else if (key == "ALBUMARTISTS" || key == "ALBUM_ARTISTS")
    tag.SetAlbumArtist(value);
  else if (key == "ALBUMARTIST" || key == "ALBUM_ARTIST")
    tag.SetAlbumArtist(StringUtils::Join(StringUtils::Split(value, separators), musicsep));
  else if (key == "TITLE")
    tag.SetTitle(value);
  else if (key == "PART_NUMBER" || key == "TRACK")
  {
    try
    {
      tag.SetTrackNumber(std::stoi(value));
    }
    catch (const std::exception&)
    {
    }
  }
  else if (key == "DISC" || key == "DISCNUMBER")
  {
    try
    {
      tag.SetDiscNumber(std::stoi(value));
    }
    catch (const std::exception&)
    {
    }
  }
  else if (key == "GENRE")
    tag.SetGenre(StringUtils::Split(value, musicsep), true);
  else if (key == "COMPILATION")
    tag.SetCompilation(true);
  else if (key == "DATE" || key == "DATE_RELEASED" || key == "YEAR")
    tag.SetReleaseDate(value);
  else if (key == "DATE_RECORDED" || key == "ORIGINALDATE" || key == "ORIGINALYEAR" ||
           key == "ORIGYEAR")
    tag.SetOriginalDate(value);
  else if (key == "MOOD")
    tag.SetMood(StringUtils::Join(StringUtils::Split(value, separators), musicsep));
  // genre could be comma delimited or not. Temporarily add the comma just in case.
  // true trims any whitespace around the genre(s)
  else if (key == "COMMENT")
    tag.SetComment(value);
  else if (key == "ARTIST-SORT" || key == "ARTISTSORT")
    tag.SetArtistSort(StringUtils::Join(StringUtils::Split(value, separators), musicsep));
  else if (key == "ALBUMARTISTSORT" || key == "SORT_ALBUM_ARTIST")
    tag.SetAlbumArtistSort(StringUtils::Join(StringUtils::Split(value, separators), musicsep));
  else if (key == "COMPOSERSORT")
    tag.SetComposerSort(StringUtils::Join(StringUtils::Split(value, separators), musicsep));
  else if (key == "DISCSUBTITLE" || key == "SUBTITLE" || key == "SETSUBTITLE")
    tag.SetDiscSubtitle(value);
  else if (key == "MUSICBRAINZ_ARTISTID")
    tag.SetMusicBrainzArtistID(StringUtils::Split(value, separators));
  else if (key == "MUSICBRAINZ_ALBUMID")
    tag.SetMusicBrainzAlbumID(value);
  else if (key == "MUSICBRAINZ_RELEASEGROUPID")
    tag.SetMusicBrainzReleaseGroupID(value);
  else if (key == "MUSICBRAINZ_ALBUMARTISTID")
    tag.SetMusicBrainzAlbumArtistID(StringUtils::Split(value, separators));
  else if (key == "MUSICBRAINZ_TRACKID")
    tag.SetMusicBrainzTrackID(value);
  else if (key == "MUSICBRAINZ_ALBUMARTIST")
  {
    // tag.SetAlbumArtist(value);
  }
  else if (key == "MUSICBRAINZ_ALBUMTYPE")
    tag.SetMusicBrainzReleaseType(value);
  else if (key == "MUSICBRAINZ_ALBUMSTATUS")
    tag.SetAlbumReleaseStatus(value);
  else if (key == "ENCODED_BY" || key == "LANGUAGE")
  {
  }
  else if (key == "LABEL" || key == "PUBLISHER")
    tag.SetRecordLabel(value);
  else if (key == "CATALOGNUMBER")
  {
  } // No database field yet
  else if (key == "COPYRIGHT")
  {
  } // Copyright message
  else if (key == "WRITER")
    tag.AddArtistRole("Writer", StringUtils::Split(value, separators));
  else if (key == "PERFORMER")
  {
    std::vector<std::string> tagdata = StringUtils::Split(value, separators);
    AddRole(tagdata, separators, tag);
  }
  else if (key == "ARRANGER")
  {
    std::vector<std::string> tagdata = StringUtils::Split(value, separators);
    AddRole(tagdata, separators, tag);
  }
  else if (key == "REMIXED_BY" || key == "REMIXEDBY")
    tag.AddArtistRole("Remixer", StringUtils::Split(value, separators));
  else if (key == "MIXED_BY" || key == "MIXER")
    tag.AddArtistRole("Mixer", StringUtils::Split(value, separators));
  else if (key == "LYRICIST")
    tag.AddArtistRole("Lyricist", StringUtils::Split(value, separators));
  else if (key == "COMPOSER")
    tag.AddArtistRole("Composer", StringUtils::Split(value, separators));
  else if (key == "CONDUCTOR")
    tag.AddArtistRole("Conductor", StringUtils::Split(value, separators));
  else if (key == "ENGINEER")
    tag.AddArtistRole("Engineer", StringUtils::Split(value, separators));
  else if (key == "PRODUCER")
    tag.AddArtistRole("Producer", StringUtils::Split(value, separators));
  else if (key == "BAND")
    tag.AddArtistRole("Band", StringUtils::Split(value, separators));
  // comma separated list of role, person
  else if (key == "INVOLVEDPEOPLE" || key == "ACTOR")
  {
    std::vector<std::string> tagdata = StringUtils::Split(value, ",");

    AddCommaDelimitedString(tagdata, separators, tag);
  }
  else if (key == "INSTRUMENTS")
  {
    std::vector<std::string> tagdata = StringUtils::Split(value, ",");

    AddCommaDelimitedString(tagdata, separators, tag);
  }
}

void CMusicInfoTagLoaderMatroska::AddRole(const std::vector<std::string>& data,
                                          const std::vector<std::string>& separators,
                                          CMusicInfoTag& musictag)
{
  if (!data.empty())
  {
    for (size_t i = 0; i + 1 < data.size(); i += 2)
    {
      std::vector<std::string> roles = StringUtils::Split(data[i], separators);
      for (auto& role : roles)
      {
        StringUtils::Trim(role);
        StringUtils::ToCapitalize(role);
        musictag.AddArtistRole(role, StringUtils::Split(data[i + 1], separators));
      }
    }
  }
}

void CMusicInfoTagLoaderMatroska::AddCommaDelimitedString(
    const std::vector<std::string>& data,
    const std::vector<std::string>& separators,
    CMusicInfoTag& musictag)
{
  if (!data.empty())
  {
    for (size_t i = 0; i + 1 < data.size(); i += 2)
    {
      std::vector<std::string> roles = StringUtils::Split(data[i], separators);
      for (auto& role : roles)
      {
        StringUtils::Trim(role);
        StringUtils::ToCapitalize(role);
        musictag.AddArtistRole(role, StringUtils::Split(data[i + 1], ","));
      }
    }
  }
}

/*!
* Used by Matroska files with multiple chapters. Each chapter is a separate song.
* Static overload for external callers (e.g. AudioBookFileDirectory).
* Opens its own MatroskaTagLibStream and delegates to the shared-stream overload.
*/
void CMusicInfoTagLoaderMatroska::GetMatroskaMusicTags(
    const std::string& fileName,
    std::map<std::string, std::string>& fileTags,
    std::map<unsigned long long, std::map<std::string, std::string>>& chapterTags,
    std::vector<std::tuple<unsigned long long, std::string, double, double, unsigned long long>>&
        chapterOrder,
    CMusicInfoTag* coverTag)
{
  MatroskaTagLibStream matroskaStream(fileName);
  if (!matroskaStream.open())
  {
    fileTags.clear();
    chapterTags.clear();
    chapterOrder.clear();
    return;
  }
  GetMatroskaMusicTags(fileName, matroskaStream, fileTags, chapterTags, chapterOrder, coverTag);
}

/*!
*  use TagLib to read hierarchy of tags in file and populate album and chapter
* (track) tags. this creates a map of chapterUid to track tags for each chapter.
*/
void CMusicInfoTagLoaderMatroska::GetMatroskaMusicTags(
    const std::string& fileName,
    MatroskaTagLibStream& matroskaStream,
    std::map<std::string, std::string>& fileTags,
    std::map<unsigned long long, std::map<std::string, std::string>>& chapterTags,
    std::vector<std::tuple<unsigned long long, std::string, double, double, unsigned long long>>&
        chapterOrder,
    CMusicInfoTag* coverTag,
    EmbeddedArt* art)
{
  fileTags.clear();
  chapterTags.clear();
  chapterOrder.clear();

  std::unique_ptr<TagLib::Matroska::File> matroskaFile;
  Matroska::Tag* matroskatag = nullptr;
  try
  {
    // MatroskaTagLibStream provides a 512 KiB read-ahead buffer and deferred seeks
    matroskaFile = std::make_unique<TagLib::Matroska::File>(&matroskaStream, true,
                                                            TagLib::AudioProperties::Fast);
    if (matroskaFile->isValid())
      matroskatag = matroskaFile->tag(true);
    if (!matroskatag)
      return;

    /* 
    * Read embedded cover art from attachments (performance issue in Taglib 2.3 need to be resolved
    * This should be done in TagLib 2.3.1 (there is a PR open to fix this). Once resolved, we
    * can drop the FFmpeg embedded cover art loading and just use TagLib for Matroska files.
    * if (coverTag)
    *  GetMatroskaEmbeddedCover(*matroskaFile, *coverTag, art)
    */

    double fileDuration = 0.0;
    TagLib::AudioProperties* audioProps = matroskaFile->audioProperties();
    if (audioProps)
      fileDuration = static_cast<double>(audioProps->lengthInSeconds());

    /*!
    * First get all chapters and get the chapter name for each chapter and store
    * it in the chapterTags map. Then we have chapter name for each chapter
    * (track) if Chapters are not tagged.
    * Micro chapters (less than 1 second long) are skipped as they are not
    * real tracks/songs — they can occur in some Matroska files as artifacts.
    */
    int chapterCount = 0;
    TagLib::Matroska::Chapters* chapters = matroskaFile->chapters();
    if (chapters)
    {
      const TagLib::Matroska::Chapters::ChapterEditionList& editions =
          chapters->chapterEditionList();
      for (const auto& edition : editions)
      {
        unsigned long long editionUid = edition.uid();
        for (const auto& chapter : edition.chapterList())
        {
          unsigned long long chapUid = chapter.uid();

          // Skip micro chapters less than 1 second long
          long long durationNs = std::abs(static_cast<long long>(chapter.timeEnd()) -
                                          static_cast<long long>(chapter.timeStart()));
          if (durationNs < 1000000000LL)
            continue;

          std::string chapterName;
          if (chapUid > 0 && !chapter.displayList().isEmpty())
          {
            // Match VB behavior: keep the last display name
            for (const auto& display : chapter.displayList())
              chapterName = display.string().toCString(true);
          }

          std::map<std::string, std::string> chapterTagList = {{"CHAPTERNAME", chapterName}};
          chapterTags[chapUid] = chapterTagList;

          double startTimeSecs = static_cast<double>(chapter.timeStart()) / 1000000000.0;
          double endTimeSecs = static_cast<double>(chapter.timeEnd()) / 1000000000.0;
          chapterOrder.push_back(
              std::make_tuple(chapUid, chapterName, startTimeSecs, endTimeSecs, editionUid));
          chapterCount++;
        }
      }
    }

    /*!
    * Parsing Matroska tags create a dummy chapter if no chapters are present
    * to hold song tags for later processing for Kodi internal tags.
    * Some taggers like MP3tag save song tags as chapter tags with
    * TargetTypeValue 30 but no ChapterUid, so need to save these somewhere.
    *
    * If chapters exist, fix any that have no end time set (endTime <= 0):
    *  (out of spec but some taggers do this, Kodi neds to deal with this internally)
    *  - use the next chapter's start time, or
    *  - use the file duration for the last chapter.
    */
    constexpr unsigned long long DummyChapterUid = 999000999000999;
    if (chapterCount == 0)
    {
      chapterOrder.push_back(
          std::make_tuple(DummyChapterUid, std::string("SongTags"), 0.0, 0.0, 0ULL));
      std::map<std::string, std::string> chapterTagList = {{"CHAPTERNAME", "SongTags"}};
      chapterTags[DummyChapterUid] = chapterTagList;
    }
    else
    {
      for (size_t i = 0; i < chapterOrder.size(); ++i)
      {
        double endTime = std::get<3>(chapterOrder[i]);
        if (endTime <= 0.0)
        {
          double newEndTime;
          if (i + 1 < chapterOrder.size())
          {
            // use next chapter's start time
            newEndTime = std::get<2>(chapterOrder[i + 1]);
          }
          else
          {
            // last chapter — use the file duration
            newEndTime = fileDuration;
          }
          chapterOrder[i] = std::make_tuple(std::get<0>(chapterOrder[i]), // uid
                                            std::get<1>(chapterOrder[i]), // name
                                            std::get<2>(chapterOrder[i]), // startTime
                                            newEndTime, // fixed endTime
                                            std::get<4>(chapterOrder[i])); // editionUid
        }
      }
    }

    /*!
    * Define tags that support multiple values and need to be concatenated into a
    * single internal Kodi tag with a separator if more than one value is
    * present. This is needed to support multiple same key tags (Matroska spec)
    */
    static constexpr std::array<const char*, 21> MULTIPLE_VALUE_TAGS = {"ALBUMARTISTS",
                                                                        "ALBUMARTISTSORT",
                                                                        "ARTIST",
                                                                        "ARTISTS",
                                                                        "ARTISTSORT",
                                                                        "ARRANGER",
                                                                        "BAND",
                                                                        "COMPOSER",
                                                                        "COMPOSERSORT",
                                                                        "CONDUCTOR",
                                                                        "ENGINEER",
                                                                        "GENRE",
                                                                        "LYRICIST",
                                                                        "MIXER",
                                                                        "MOOD",
                                                                        "MUSICBRAINZ_ALBUMARTISTID",
                                                                        "MUSICBRAINZ_ARTISTID",
                                                                        "PERFORMER",
                                                                        "PRODUCER",
                                                                        "REMIXED",
                                                                        "WRITER"};

    /*!
    * Read all simple tags and group them by file (album or song files with no
    * chapters) or by chapter/track (if target type value is 30).
    * Delimiter separated lists are outside the Matroska spec
    * (see https://www.matroska.org/technical/tagging.html) it states to use
    * multiple simple tags for eg 2 or more composers. To ensure Kodi can use
    * multiple same name tags need create a single tag with multiple values in
    * a delimited string (Kodi handles multiple values with
    * delimited strings).
    *
    * Two pass approach:
    * Pass 1: Process album-level tags (targetTypeValue == 50) first so album
    *         metadata is established before track-level tags are processed.
    *         Special handling for TITLE tag which maps to ALBUM in Kodi.
    * Pass 2: Process file-level (targetTypeValue == 0) and chapter/song
    *         (targetTypeValue == 30) tags.
    */
    std::string TagName;
    std::string TagValue;
    const TagLib::Matroska::SimpleTagsList& list = matroskatag->simpleTagsList();
    // Pass 1: Process album-level tags (targetTypeValue == 50)
    for (const TagLib::Matroska::SimpleTag& tag : list)
    {
      if (tag.targetTypeValue() == 50 || tag.targetTypeValue() == 60)
      {
        TagName = StringUtils::ToUpper(tag.name().to8Bit(true));
        TagValue = tag.toString().to8Bit(true);
        /*!
        * TITLE with targetTypeValue 50 is the Album title in Matroska spec
        * ALBUM was used in Kodi 21.3 for ffmpeg tag reding compatibility
        * targetTypeValue 60 used by MP3Tag for concerts, maps to ALBUM in Kodi music
        */
        if (TagName == "TITLE")
        {
          if (fileTags.find("ALBUM") == fileTags.end())
            fileTags["ALBUM"] = TagValue;
          if (fileTags.find("TITLE") == fileTags.end())
            fileTags["TITLE"] = TagValue;
        }
        else if (fileTags.find(TagName) == fileTags.end())
        {
          fileTags[TagName] = TagValue;
        }
        else
        {
          if (std::find(std::begin(MULTIPLE_VALUE_TAGS), std::end(MULTIPLE_VALUE_TAGS), TagName) !=
              std::end(MULTIPLE_VALUE_TAGS))
          {
            std::string currentValue = fileTags[TagName];
            if (AppendIfNotDuplicate(currentValue, TagValue, TagName))
              fileTags[TagName] = currentValue;
          }
        }
      }
    }

    // Pass 2: Process remaining tags (file-level and chapter/song tags)
    for (const TagLib::Matroska::SimpleTag& tag : list)
    {
      unsigned long long chapterUid = tag.chapterUid();
      std::string TagName = StringUtils::ToUpper(tag.name().to8Bit(true));
      unsigned long long targetTypeValue = tag.targetTypeValue();

      if (targetTypeValue == 50 || targetTypeValue == 60)
        continue; // already processed in Pass 1

      TagName = StringUtils::ToUpper(tag.name().to8Bit(true));
      TagValue = tag.toString().to8Bit(true);

      /*!
      * No targetTypeValue should be considered as an 'Album' level tag to avoid losing metadata
      * for files that don't follow the Matroska spec and don't set targetTypeValue.
      */
      if (targetTypeValue == 0)
      {
        if (TagName == "TITLE")
        {
          if (fileTags.find("ALBUM") == fileTags.end())
            fileTags["ALBUM"] = TagValue;
          if (fileTags.find("TITLE") == fileTags.end())
            fileTags["TITLE"] = TagValue;
        }
        else
        {
          if (fileTags.find(TagName) == fileTags.end())
          {
            fileTags[TagName] = TagValue;
          }
          else
          {
            if (std::find(std::begin(MULTIPLE_VALUE_TAGS), std::end(MULTIPLE_VALUE_TAGS),
                          TagName) != std::end(MULTIPLE_VALUE_TAGS))
            {
              std::string currentValue = fileTags[TagName];
              if (AppendIfNotDuplicate(currentValue, TagValue, TagName))
                fileTags[TagName] = currentValue;
            }
          }
        }
      }
      else if (targetTypeValue == 30)
      {
        if (chapterCount == 1)
        {
          // Single chapter: route to the only chapter with duplicate check
          unsigned long long firstChapterUid = std::get<0>(chapterOrder[0]);
          auto firstIt = chapterTags.find(firstChapterUid);
          if (firstIt != chapterTags.end())
          {
            auto& chapterTagList = firstIt->second;
            auto it = chapterTagList.find(TagName);
            if (it == chapterTagList.end())
            {
              chapterTagList.emplace(TagName, TagValue);
            }
            else
            {
              if (std::find(std::begin(MULTIPLE_VALUE_TAGS), std::end(MULTIPLE_VALUE_TAGS),
                            TagName) != std::end(MULTIPLE_VALUE_TAGS))
              {
                AppendIfNotDuplicate(it->second, TagValue, TagName);
              }
            }
          }
        }
        else if (chapterUid > 1)
        {
          // Multiple chapters: route to the chapter with matching UID
          auto chapterIt = chapterTags.find(chapterUid);
          if (chapterIt != chapterTags.end())
          {
            auto& chapterTagList = chapterIt->second;
            auto it = chapterTagList.find(TagName);
            if (it == chapterTagList.end())
            {
              chapterTagList.emplace(TagName, TagValue);
            }
            else
            {
              if (std::find(std::begin(MULTIPLE_VALUE_TAGS), std::end(MULTIPLE_VALUE_TAGS),
                            TagName) != std::end(MULTIPLE_VALUE_TAGS))
              {
                AppendIfNotDuplicate(it->second, TagValue, TagName);
              }
            }
          }
          else
          {
            // so this chapter was not in the Chapters element. Fall back to fileTags.
            if (fileTags.find(TagName) == fileTags.end())
            {
              fileTags[TagName] = TagValue;
            }
            else
            {
              if (std::find(std::begin(MULTIPLE_VALUE_TAGS), std::end(MULTIPLE_VALUE_TAGS),
                            TagName) != std::end(MULTIPLE_VALUE_TAGS))
              {
                std::string currentValue = fileTags[TagName];
                if (AppendIfNotDuplicate(currentValue, TagValue, TagName))
                  fileTags[TagName] = currentValue;
              }
            }
          }
        }
      }
    }

    // bufferedStream and matroskaFile are destroyed when scope exits.
  }
  catch (const std::exception& e)
  {
    CLog::Log(LOGERROR, "GetMatroskaMusicTags: Exception while reading Matroska tags: {} {}",
              fileName, e.what());
  }
}
