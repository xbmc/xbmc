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
#include <taglib/matroskaproperties.h>
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
* Tags that carry several values, which the Matroska spec writes as a SimpleTag repeated per value
* (https://www.matroska.org/technical/tagging.html). Kodi holds multiple values in one delimited
* string, so a repeat concatenates rather than replaces.
*/
constexpr std::array<const char*, 21> MULTIPLE_VALUE_TAGS = {"ALBUMARTISTS",
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
* Record one tag, keeping any value the same name already carries if the name is one that holds
* several. The key is what the map is filed under and the name is what decides that, which differ
* for an album level tag: ALBUM/ARTIST is filed apart from the track's, but takes several values
* for the same reason ARTIST does.
*/
void AddTagValue(std::map<std::string, std::string>& tags,
                 const std::string& key,
                 const std::string& name,
                 const std::string& value)
{
  const auto it = tags.find(key);
  if (it == tags.end())
  {
    tags.emplace(key, value);
    return;
  }

  if (std::find(std::begin(MULTIPLE_VALUE_TAGS), std::end(MULTIPLE_VALUE_TAGS), name) !=
      std::end(MULTIPLE_VALUE_TAGS))
    AppendIfNotDuplicate(it->second, value, name);
}

/*!
* The file's own properties: how long it runs, and the Segment title.
*
* The Segment title names the file rather than its album, and FFmpeg's demuxer reports it as an
* unprefixed title. Say the same, so that a file holding one song is titled the same way whichever
* reader read it.
*/
double ReadFileProperties(TagLib::Matroska::File& file,
                          std::map<std::string, std::string>& fileTags)
{
  const TagLib::Matroska::Properties* audioProps = file.audioProperties();
  if (!audioProps)
    return 0.0;

  const std::string segmentTitle = audioProps->title().to8Bit(true);
  if (!segmentTitle.empty())
    fileTags["TITLE"] = segmentTitle;

  return static_cast<double>(audioProps->lengthInSeconds());
}

/*!
* Collect one edition's chapters in file order, each keeping its display name so that a chapter
* carrying no tags of its own still has something to name its track with.
*
* A file can hold several editions - an ordered presentation cut alongside the full transfer, say -
* of which only one is what gets played. Take the one flagged default, falling back to the first, so
* the tracks come from a single running order instead of every edition's chapters concatenated.
*
* \param editionUid The edition the chapters came from, which its tags name.
* \param unselectedChapterUids The chapters of the editions left behind, so that a tag naming one of
*        them can be told apart from a tag naming a chapter this file does not have at all.
* \param chapterIndex Where a chapter's tags live, by the ChapterUID the SimpleTags name it with.
*/
void CollectChapters(TagLib::Matroska::File& file,
                     MatroskaAlbum& album,
                     unsigned long long& editionUid,
                     std::set<unsigned long long>& unselectedChapterUids,
                     std::map<unsigned long long, size_t>& chapterIndex)
{
  const TagLib::Matroska::Chapters* chapters = file.chapters();
  if (!chapters)
    return;

  const TagLib::Matroska::Chapters::ChapterEditionList& editions = chapters->chapterEditionList();
  const TagLib::Matroska::ChapterEdition* selectedEdition = nullptr;
  for (const auto& edition : editions)
  {
    if (!selectedEdition || edition.isDefault())
      selectedEdition = &edition;
    if (edition.isDefault())
      break;
  }

  for (const auto& edition : editions)
  {
    if (&edition == selectedEdition)
      continue;
    for (const auto& chapter : edition.chapterList())
      unselectedChapterUids.insert(chapter.uid());
  }

  if (!selectedEdition)
    return;

  editionUid = selectedEdition->uid();
  for (const auto& chapter : selectedEdition->chapterList())
  {
    const unsigned long long chapUid = chapter.uid();

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

/*!
* Give an end to any chapter that declares none, which is out of spec but written anyway: the next
* chapter's start, or the file duration for the last one.
*/
void FillMissingEndTimes(MatroskaAlbum& album, double fileDuration)
{
  for (size_t i = 0; i < album.chapters.size(); ++i)
  {
    if (album.chapters[i].end > 0.0)
      continue;
    album.chapters[i].end =
        (i + 1 < album.chapters.size()) ? album.chapters[i + 1].start : fileDuration;
  }
}

/*!
* Sort every SimpleTag onto the album or onto a chapter.
*
* Album level tags go first so that a TargetTypeValue 30 tag reaching the album finds one already
* there rather than establishing it. A tag naming an edition belongs to that edition alone: files
* with several carry one TITLE each, and taking whichever came first names the album after an
* edition that is not the one being read. A zero EditionUID applies to all editions.
*/
void CollectSimpleTags(const TagLib::Matroska::SimpleTagsList& list,
                       MatroskaAlbum& album,
                       unsigned long long editionUid,
                       const std::set<unsigned long long>& unselectedChapterUids,
                       const std::map<unsigned long long, size_t>& chapterIndex)
{
  auto& fileTags = album.fileTags;

  auto namesAnotherEdition = [editionUid](const TagLib::Matroska::SimpleTag& tag)
  { return tag.editionUid() != 0 && tag.editionUid() != editionUid; };
  auto isAlbumLevel = [](unsigned long long targetTypeValue)
  { return targetTypeValue == 50 || targetTypeValue == 60; }; // 60 is MP3tag's concert

  for (const TagLib::Matroska::SimpleTag& tag : list)
  {
    if (!isAlbumLevel(tag.targetTypeValue()) || namesAnotherEdition(tag))
      continue;

    /*!
    * A name says what the tag is, the TargetTypeValue says which level it applies to, and only
    * both together say which Kodi field it means: an ARTIST at 50 is the album's, at 30 the
    * track's. TagLib hands over the name alone, so the level is written into the key here, in
    * the ALBUM/ form FFmpeg's demuxer produces, and MatroskaTagMapping reads one shape from
    * either reader.
    */
    const std::string name = StringUtils::ToUpper(tag.name().to8Bit(true));
    AddTagValue(fileTags, "ALBUM/" + name, name, tag.toString().to8Bit(true));
  }

  for (const TagLib::Matroska::SimpleTag& tag : list)
  {
    const unsigned long long targetTypeValue = tag.targetTypeValue();
    if (isAlbumLevel(targetTypeValue) || namesAnotherEdition(tag))
      continue;

    const std::string name = StringUtils::ToUpper(tag.name().to8Bit(true));
    const std::string value = tag.toString().to8Bit(true);

    /*!
    * A tag with no TargetTypeValue describes the file: taggers that ignore the spec write album
    * metadata that way, and dropping it would lose the lot. Its TITLE names both the album and the
    * file, neither overwriting what a level-bearing tag already established.
    */
    if (targetTypeValue == 0)
    {
      if (name == "TITLE")
      {
        fileTags.emplace("ALBUM", value);
        fileTags.emplace("TITLE", value);
      }
      else
      {
        AddTagValue(fileTags, name, name, value);
      }
      continue;
    }

    if (targetTypeValue != 30)
      continue;

    /*!
    * A tag naming a chapter of an edition that was not selected describes a track this file will
    * not produce. Neither merging it into a chapter it does not describe nor promoting it to the
    * album is right, so it goes no further.
    */
    const unsigned long long chapterUid = tag.chapterUid();
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

    // Either the file has no chapters at all, or names one it does not contain. Either way the tag
    // describes the file rather than a track of it.
    AddTagValue(target ? target->tags : fileTags, name, name, value);
  }
}

/*!
* Name a track after its chapter's display name where nothing better named it.
*
* A chapter carrying only a ChapterDisplay name still names its track - taggers that write chapter
* names rather than per-chapter tags are common. The TargetTypeValue 30 TITLE read above says the
* same thing more precisely, so it keeps precedence and this only fills the gap.
*/
void FillTitlesFromDisplayNames(MatroskaAlbum& album)
{
  for (auto& chapter : album.chapters)
  {
    const auto chapterName = chapter.tags.find("CHAPTERNAME");
    if (chapterName == chapter.tags.end() || chapterName->second.empty())
      continue;
    chapter.tags.emplace("TITLE", chapterName->second);
  }
}

/*!
 * Read with TagLib, which follows the whole SimpleTag hierarchy: TargetTypeValue, EditionUID and
 * ChapterUID all survive, so album level tags stay apart from a chapter's own, repeated tags all
 * arrive, and both stay tied to the edition they belong to.
 */
MatroskaAlbum ReadWithTagLib(const CURL& url)
{
  MatroskaAlbum album;

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

    const double fileDuration = ReadFileProperties(*matroskaFile, album.fileTags);

    unsigned long long editionUid = 0;
    std::set<unsigned long long> unselectedChapterUids;
    std::map<unsigned long long, size_t> chapterIndex;
    CollectChapters(*matroskaFile, album, editionUid, unselectedChapterUids, chapterIndex);
    FillMissingEndTimes(album, fileDuration);

    CollectSimpleTags(matroskatag->simpleTagsList(), album, editionUid, unselectedChapterUids,
                      chapterIndex);
    FillTitlesFromDisplayNames(album);

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
