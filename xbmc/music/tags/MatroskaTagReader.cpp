/*
 *  Copyright (C) 2005-2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "MatroskaTagReader.h"

#include "TagLibVersion.h"
#include "URL.h"
#include "utils/StringUtils.h"
#ifdef HAS_TAGLIB_MATROSKA
#include "MatroskaTagLibStream.h"
#include "utils/log.h"

#include <algorithm>
#include <array>
#include <cstdlib>
#include <exception>
#include <iterator>
#include <memory>
#include <set>

#include <commons/ilog.h>
#include <taglib/audioproperties.h>
#include <taglib/matroskachapteredition.h>
#include <taglib/matroskachapters.h>
#include <taglib/matroskafile.h>
#include <taglib/matroskasimpletag.h>
#include <taglib/matroskatag.h>
#include <taglib/tlist.h>
#include <taglib/tstring.h>
#endif

#include <map>
#include <string>
#include <tuple>
#include <vector>

extern "C"
{
#include <libavformat/avformat.h>
#include <libavutil/dict.h>
#include <libavutil/rational.h>
}

using namespace MUSIC_INFO;
#ifdef HAS_TAGLIB_MATROSKA
using namespace TagLib;
#endif

namespace
{
//! Harvest one FFmpeg metadata dictionary into the map MatroskaTagMapping expects.
void CollectTags(const AVDictionary* metadata, std::map<std::string, std::string>& tags)
{
  const AVDictionaryEntry* entry = nullptr;
  while ((entry = av_dict_get(metadata, "", entry, AV_DICT_IGNORE_SUFFIX)))
    tags[StringUtils::ToUpper(entry->key)] = entry->value;
}

/*!
 * Read with FFmpeg's demuxer, which flattens the SimpleTag hierarchy: TargetTypeValue is lost -
 * album level tags arrive under an ALBUM_ prefix instead - nesting goes with it, only the last of
 * a repeated set of SimpleTags survives (https://trac.ffmpeg.org/ticket/9641), and only the
 * default edition's chapters are reported. A subset of what TagLib returns, not a different thing.
 *
 * Compiled whether or not it is the reader this build uses, so that it cannot rot unnoticed in
 * the builds that have TagLib.
 */
MatroskaAlbum ReadWithFFmpegImpl(const AVFormatContext* fctx)
{
  MatroskaAlbum album;

  if (!fctx)
    return album;

  CollectTags(fctx->metadata, album.fileTags);

  for (unsigned int i = 0; fctx->chapters && i < fctx->nb_chapters; ++i)
  {
    const AVChapter* chapter = fctx->chapters[i];
    if (!chapter || chapter->start < 0)
      continue;

    ChapterTags& entry = album.chapters.emplace_back();
    CollectTags(chapter->metadata, entry.tags);
    entry.start = chapter->start * av_q2d(chapter->time_base);
    entry.end = chapter->end * av_q2d(chapter->time_base);
  }

  return album;
}

#ifdef HAS_TAGLIB_MATROSKA

const std::vector<std::string> SupportedArtistMultiValueSeparators = {";", "|"};
const std::vector<std::string> SupportedMultiValueSeparators = {";", "/", "|", ","};

/*!
* Translate multiple single key tags (Matrosk spec) to delimited a single for internal use.
* Appends " / " + newValue to currentValue if newValue is not already present
* (case-insensitive) among the existing delimited values. The set of delimiters
* used to split currentValue depends on whether tagname refers to an artist tag.
* Returns true if the value was appended, false otherwise.
*/
bool AppendIfNotDuplicate(std::string& currentValue,
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
 * Read with TagLib, which follows the whole SimpleTag hierarchy: TargetTypeValue, EditionUID and
 * ChapterUID all survive, so album level tags stay apart from a chapter's own, repeated tags all
 * arrive, and both stay tied to the edition they belong to.
 */
MatroskaAlbum ReadWithTagLib(const CURL& url)
{
  MatroskaAlbum album;
  auto& fileTags = album.fileTags;

  //! Where a chapter's tags live, by the ChapterUID the SimpleTags name it with.
  std::map<unsigned long long, size_t> chapterIndex;

  MatroskaTagLibStream matroskaStream(url.Get());
  if (!matroskaStream.open())
    return album;

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
      return album;

    double fileDuration = 0.0;
    TagLib::AudioProperties* audioProps = matroskaFile->audioProperties();
    if (audioProps)
      fileDuration = static_cast<double>(audioProps->lengthInSeconds());

    /*!
    * Collect the chapters in file order, each keeping its display name, so that a chapter which
    * carries no tags of its own still has something to name its track with.
    * Micro chapters (less than 1 second long) are skipped as they are not
    * real tracks/songs — they can occur in some Matroska files as artifacts.
    */
    unsigned long long editionUid = 0;
    std::set<unsigned long long> unselectedChapterUids;
    TagLib::Matroska::Chapters* chapters = matroskaFile->chapters();
    if (chapters)
    {
      /*!
      * A file can hold several editions - an ordered presentation cut alongside the full
      * transfer, say - of which only one is what gets played. Take the one flagged default,
      * falling back to the first, so the tracks come from a single running order instead of
      * every edition's chapters concatenated.
      */
      const TagLib::Matroska::Chapters::ChapterEditionList& editions =
          chapters->chapterEditionList();
      const TagLib::Matroska::ChapterEdition* selectedEdition = nullptr;
      for (const auto& edition : editions)
      {
        if (!selectedEdition || edition.isDefault())
          selectedEdition = &edition;
        if (edition.isDefault())
          break;
      }

      /*!
      * The chapters of the editions left behind, so that a tag naming one of them can be told
      * apart from a tag naming a chapter this file does not have at all.
      */
      for (const auto& edition : editions)
      {
        if (&edition == selectedEdition)
          continue;
        for (const auto& chapter : edition.chapterList())
          unselectedChapterUids.insert(chapter.uid());
      }

      if (selectedEdition)
      {
        editionUid = selectedEdition->uid();
        for (const auto& chapter : selectedEdition->chapterList())
        {
          unsigned long long chapUid = chapter.uid();

          std::string chapterName;
          if (chapUid > 0 && !chapter.displayList().isEmpty())
          {
            // Match VB behavior: keep the last display name
            for (const auto& display : chapter.displayList())
              chapterName = display.string().toCString(true);
          }

          chapterIndex[chapUid] = album.chapters.size();
          ChapterTags& entry = album.chapters.emplace_back();
          entry.tags.emplace("CHAPTERNAME", chapterName);
          entry.start = static_cast<double>(chapter.timeStart()) / 1000000000.0;
          entry.end = static_cast<double>(chapter.timeEnd()) / 1000000000.0;
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
    if (!album.chapters.empty())
    {
      for (size_t i = 0; i < album.chapters.size(); ++i)
      {
        if (album.chapters[i].end > 0.0)
          continue;
        // the next chapter's start, or the file duration for the last one
        album.chapters[i].end =
            (i + 1 < album.chapters.size()) ? album.chapters[i + 1].start : fileDuration;
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
        /*!
        * A tag naming an edition belongs to that edition alone. Files with several editions carry
        * one such TITLE each, and taking whichever came first in the file names the album after an
        * edition that is not the one being read. A zero EditionUID applies to all editions.
        */
        if (tag.editionUid() != 0 && tag.editionUid() != editionUid)
          continue;

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

      // A tag naming another edition describes tracks this file will not produce - see Pass 1.
      if (tag.editionUid() != 0 && tag.editionUid() != editionUid)
        continue;

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
        /*!
        * A tag naming a chapter of an edition that was not selected describes a track this file
        * will not produce. Neither merging it into a chapter it does not describe nor promoting
        * it to the album is right, so it goes no further.
        */
        if (unselectedChapterUids.count(chapterUid) != 0)
          continue;

        /*!
        * A tag with no ChapterUID describes the only track there is - MP3tag writes song tags that
        * way - so a single chapter file takes it. One naming a chapter goes to that chapter, and
        * one naming a chapter this file does not have falls back to the album.
        */
        ChapterTags* target = nullptr;
        if (album.chapters.size() == 1)
          target = &album.chapters.front();
        else if (chapterUid > 1)
        {
          if (const auto it = chapterIndex.find(chapterUid); it != chapterIndex.end())
            target = &album.chapters[it->second];
        }

        if (target)
        {
          auto it = target->tags.find(TagName);
          if (it == target->tags.end())
            target->tags.emplace(TagName, TagValue);
          else if (std::find(std::begin(MULTIPLE_VALUE_TAGS), std::end(MULTIPLE_VALUE_TAGS),
                             TagName) != std::end(MULTIPLE_VALUE_TAGS))
            AppendIfNotDuplicate(it->second, TagValue, TagName);
        }
        else
        {
          // Either the file has no chapters at all, or names one it does not contain. Either way
          // the tag describes the file rather than a track of it.
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

    /*!
    * A chapter carrying only a ChapterDisplay name still names its track - taggers that write
    * chapter names rather than per-chapter tags are common. The TargetTypeValue 30 TITLE read
    * above says the same thing more precisely, so it keeps precedence and this only fills the gap.
    */
    for (auto& chapter : album.chapters)
    {
      auto& chapterTagList = chapter.tags;
      const auto chapterName = chapterTagList.find("CHAPTERNAME");
      if (chapterName == chapterTagList.end() || chapterName->second.empty())
        continue;
      if (chapterTagList.find("TITLE") == chapterTagList.end())
        chapterTagList.emplace("TITLE", chapterName->second);
    }

    // bufferedStream and matroskaFile are destroyed when scope exits.
  }
  catch (const std::exception& e)
  {
    CLog::Log(LOGERROR, "ReadWithTagLib: Exception while reading Matroska tags: {} {}",
              url.GetRedacted(), e.what());
  }

  return album;
}

#endif // TagLib >= 2.3.1
} // unnamed namespace

MatroskaAlbum MUSIC_INFO::ReadMatroskaTagsWithFFmpeg(const CURL& /*url*/,
                                                     const AVFormatContext* fctx)
{
  return ReadWithFFmpegImpl(fctx);
}

#ifdef HAS_TAGLIB_MATROSKA
MatroskaAlbum MUSIC_INFO::ReadMatroskaTagsWithTagLib(const CURL& url,
                                                     const AVFormatContext* /*fctx*/)
{
  return ReadWithTagLib(url);
}
#endif

MatroskaAlbum MUSIC_INFO::ReadMatroskaTags(const CURL& url, const AVFormatContext* fctx)
{
  // TagLib wherever the version floor allows it, FFmpeg below that: fewer tags rather than none.
#ifdef HAS_TAGLIB_MATROSKA
  return ReadMatroskaTagsWithTagLib(url, fctx);
#else
  return ReadMatroskaTagsWithFFmpeg(url, fctx);
#endif
}
