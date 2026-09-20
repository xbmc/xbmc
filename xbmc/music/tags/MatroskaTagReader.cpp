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
#include "MatroskaTagMapping.h"
#include "utils/log.h"

#include <exception>
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

// CollectFileTags() and IsTrack() compile in every configuration, so their includes sit outside
// the guard above.
#include <algorithm>
#include <array>
#include <cmath>
#include <map>
#include <string>
#include <string_view>
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
 * Sort the file's own metadata into the level each tag belongs to.
 *
 * FFmpeg has no TargetTypeValue to carry, so it prefixes a name with the TargetType the file gave
 * that level, and a slash: a TargetTypeValue 50 COMPOSER written with TargetType "ALBUM" arrives as
 * ALBUM/COMPOSER. TargetType is optional and the spec names several per level
 * (https://www.matroska.org/technical/tagging.html), so every name for the two levels that reach a
 * whole file is matched, not "ALBUM" alone.
 *
 * A prefix naming any other level describes something this reader has no place for - a collection,
 * a part - and is dropped rather than filed where it does not belong.
 */
void CollectFileTags(const AVDictionary* metadata, MatroskaAlbum& album)
{
  //! TargetTypeValue 50 and 60: what the file as a whole is part of.
  static constexpr std::array<std::string_view, 11> albumLevel = {
      "ALBUM", "OPERA",  "CONCERT", "MOVIE",  "EPISODE", "EDITION",
      "ISSUE", "VOLUME", "OPUS",    "SEASON", "SEQUEL"};
  //! TargetTypeValue 30. In the file's own metadata it names no chapter, so it describes the file.
  static constexpr std::array<std::string_view, 3> trackLevel = {"TRACK", "SONG", "CHAPTER"};

  const AVDictionaryEntry* entry = nullptr;
  while ((entry = av_dict_get(metadata, "", entry, AV_DICT_IGNORE_SUFFIX)))
  {
    const std::string key = StringUtils::ToUpper(entry->key);

    /*!
    * Not tags at all: FFmpeg reports MuxingApp as an encoder and DateUTC as a creation time, both
    * mandatory or near enough that every Matroska would look like it carries tags. Neither is
    * mapped to a music field, so dropping them costs nothing and lets what remains answer whether
    * the file was tagged.
    */
    if (key == "ENCODER" || key == "CREATION_TIME")
      continue;

    const size_t slash = key.find('/');
    if (slash == std::string::npos)
    {
      album.fileTags[key] = entry->value;
      continue;
    }

    const std::string_view prefix{key.data(), slash};
    const std::string name = key.substr(slash + 1);
    if (std::find(albumLevel.begin(), albumLevel.end(), prefix) != albumLevel.end())
      album.albumTags[name] = entry->value;
    else if (std::find(trackLevel.begin(), trackLevel.end(), prefix) != trackLevel.end())
      album.fileTags[name] = entry->value;
  }
}

/*!
 * Read with FFmpeg's demuxer, which flattens the SimpleTag hierarchy: TargetTypeValue is lost -
 * album level tags arrive prefixed with the TargetType name instead - nesting goes with it, and
 * only the last of a repeated set of SimpleTags survives (https://trac.ffmpeg.org/ticket/9641).
 *
 * Editions it does not model at all: every EditionEntry's chapters are nested into one list and
 * kept if they start after the last one kept. A second edition rerunning the same timeline
 * vanishes, one starting later does not - so unlike the TagLib reader this one can report a
 * chapter the file will not play, and cannot tell that it has.
 *
 * Compiled whether or not it is the reader this build uses, so that it cannot rot unnoticed in
 * the builds that have TagLib.
 */
MatroskaAlbum ReadWithFFmpegImpl(const AVFormatContext* fctx)
{
  MatroskaAlbum album;

  if (!fctx)
    return album;

  CollectFileTags(fctx->metadata, album);

  for (unsigned int i = 0; fctx->chapters && i < fctx->nb_chapters; ++i)
  {
    const AVChapter* chapter = fctx->chapters[i];
    if (!chapter || chapter->start < 0)
      continue;

    ChapterTags& entry = album.chapters.emplace_back();
    CollectTags(chapter->metadata, entry.tags);
    entry.start = chapter->start * av_q2d(chapter->time_base);

    /*!
     * A chapter that declared no ChapterTimeEnd never reaches here open: compute_chapters_end()
     * runs inside avformat_find_stream_info() and closes it with the next chapter's start, bounded
     * by the file's duration. That is the right answer whenever the duration is, so the end is
     * taken as given.
     *
     * Where the demuxer had to estimate a duration and estimated one below the chapter timeline,
     * what it writes instead is the bound itself, or the chapter's own start where even that
     * precedes it - an end equal to the start, a chapter of no length, which IsTrack() drops. A
     * file declaring neither its Duration nor any ChapterTimeEnd is therefore not expanded on this
     * reader, where TagLib expands it - see
     * TestMatroskaTagReaderFFmpeg.ClampsEveryEndOfAFileThatDeclaresNoLength.
     *
     * Zero is how a chapter with no end reaches IsTrack() and CAudioBookFileDirectory, which serve
     * either reader. An end that still precedes its start is written as that same zero, so it is
     * closed against the file's duration rather than dropped - the two states share a spelling,
     * and telling them apart would take a flag on ChapterTags. Only a file whose chapter
     * timestamps are already malformed reaches the second.
     */
    const double end = chapter->end * av_q2d(chapter->time_base);
    entry.end = end >= entry.start ? end : 0.0;
  }

  return album;
}

#ifdef HAS_TAGLIB_MATROSKA

/*!
* What a value already recorded may hold several values under, so that a repeat of one of them is
* recognised. MatroskaTagMapping::MultiValueSeparator is in both, being what an earlier repeat was
* joined with. The artist list omits a bare "/" because in an artist a slash is part of the name:
* AC/DC.
*/
const std::vector<std::string> SupportedArtistMultiValueSeparators = {
    ";", "|", MatroskaTagMapping::MultiValueSeparator};
const std::vector<std::string> SupportedMultiValueSeparators = {";", "/", "|", ","};

/*!
* Translate multiple single key tags (Matroska spec) into the one delimited value Kodi holds.
* Appends MatroskaTagMapping::MultiValueSeparator + newValue to currentValue unless newValue is
* already among the existing delimited values, compared case-insensitively. Which delimiters
* currentValue is split on depends on whether tagname is an artist tag.
* Returns whether the value was appended.
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
    currentValue += MatroskaTagMapping::MultiValueSeparator + newValue;

  return true;
}

/*!
* Record one tag, keeping any value the same name already carries if the name is one that holds
* several. An album level tag is told apart from a track's by which map it is handed, not by its
* name: ALBUM/ARTIST is filed with the album's tags and ARTIST with the track's, both under the
* name the file gave, and both taking several values for the same reason.
*/
void AddTagValue(std::map<std::string, std::string>& tags,
                 const std::string& name,
                 const std::string& value)
{
  const auto it = tags.find(name);
  if (it == tags.end())
  {
    tags.emplace(name, value);
    return;
  }

  if (MatroskaTagMapping::HoldsSeveralValues(name))
    AppendIfNotDuplicate(it->second, value, name);
}

//! What the Segment says about the file itself, as opposed to the album it holds.
struct FileProperties
{
  double duration = 0.0; //!< seconds
  std::string segmentTitle;
};

FileProperties ReadFileProperties(TagLib::Matroska::File& file)
{
  const TagLib::Matroska::Properties* audioProps = file.audioProperties();
  if (!audioProps)
    return {};

  // Milliseconds: lengthInSeconds() truncates, and this is what gives a last chapter with no
  // ChapterTimeEnd its end.
  return {audioProps->lengthInMilliseconds() / 1000.0, audioProps->title().to8Bit(true)};
}

//! The edition the chapters were taken from, as the tag pass needs to know it.
struct SelectedEdition
{
  unsigned long long uid = 0; //!< What a tag naming this edition carries.

  //! The chapters of the editions left behind, so that a tag naming one of them can be told apart
  //! from a tag naming a chapter this file does not have at all.
  std::set<unsigned long long> unselectedChapterUids;

  //! Where a chapter's tags live, by the ChapterUID the SimpleTags name it with.
  std::map<unsigned long long, size_t> chapterIndex;
};

/*!
* Collect one edition's chapters in file order, each keeping its display name so that a chapter
* carrying no tags of its own still has something to name its track with.
*
* A file can hold several editions - an ordered presentation cut alongside the full transfer, say -
* of which only one is what gets played. Take the one flagged default, falling back to the first, so
* the tracks come from a single running order instead of every edition's chapters concatenated.
*/
SelectedEdition CollectChapters(TagLib::Matroska::File& file, MatroskaAlbum& album)
{
  SelectedEdition selected;

  const TagLib::Matroska::Chapters* chapters = file.chapters();
  if (!chapters)
    return selected;

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
    // Zero names no chapter: it is what a chapter declaring no ChapterUID reports, and equally
    // what a tag carrying none reports. Filing it would drop every such tag as belonging to an
    // edition this file does not play.
    for (const auto& chapter : edition.chapterList())
      if (chapter.uid() > 0)
        selected.unselectedChapterUids.insert(chapter.uid());
  }

  if (!selectedEdition)
    return selected;

  selected.uid = selectedEdition->uid();
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

    // Only a chapter that named itself can be found by name; nameless ones would take turns
    // owning the key zero.
    if (chapUid > 0)
      selected.chapterIndex[chapUid] = album.chapters.size();
    ChapterTags& entry = album.chapters.emplace_back();
    entry.tags.emplace("CHAPTERNAME", chapterName);
    entry.start = static_cast<double>(chapter.timeStart()) / 1000000000.0;
    entry.end = static_cast<double>(chapter.timeEnd()) / 1000000000.0;
  }

  /*!
  * A UID the selected edition carries as well names a chapter this file does play. Editions
  * sharing chapter UIDs is legal - an ordered cut reusing the transfer's chapters is the usual
  * reason - so what is left behind is only what the selected edition does not have, and a tag
  * naming one of those is dropped in CollectSimpleTags() rather than applied to a chapter it
  * does not describe.
  */
  for (const auto& entry : selected.chapterIndex)
    selected.unselectedChapterUids.erase(entry.first);

  return selected;
}

/*!
* Give an end to any chapter that declares none: the next chapter's start, or the file duration
* for the last one. ChapterTimeEnd is optional in the spec and taggers leave it out.
*
* Only an end that comes after the start is one. The Segment's Duration is optional too, and a
* file that wrote neither says nothing about where its last chapter stops - that one stays unset,
* meaning it runs to the end of the file.
*/
void FillMissingEndTimes(MatroskaAlbum& album, double fileDuration)
{
  for (size_t i = 0; i < album.chapters.size(); ++i)
  {
    if (album.chapters[i].end > 0.0)
      continue;
    const double end = (i + 1 < album.chapters.size()) ? album.chapters[i + 1].start : fileDuration;
    if (end > album.chapters[i].start)
      album.chapters[i].end = end;
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
                       const SelectedEdition& edition)
{
  auto& fileTags = album.fileTags;

  auto namesAnotherEdition = [&edition](const TagLib::Matroska::SimpleTag& tag)
  { return tag.editionUid() != 0 && tag.editionUid() != edition.uid; };
  // An edition, volume or opus tag describes a grouping above the album, which for one file is
  // still the whole file - so it lands on the album rather than nowhere.
  auto isAlbumLevel = [](TagLib::Matroska::SimpleTag::TargetTypeValue level)
  {
    return level == TagLib::Matroska::SimpleTag::Album ||
           level == TagLib::Matroska::SimpleTag::Edition;
  };

  for (const TagLib::Matroska::SimpleTag& tag : list)
  {
    if (!isAlbumLevel(tag.targetTypeValue()) || namesAnotherEdition(tag))
      continue;

    const std::string name = StringUtils::ToUpper(tag.name().to8Bit(true));
    AddTagValue(album.albumTags, name, tag.toString().to8Bit(true));
  }

  for (const TagLib::Matroska::SimpleTag& tag : list)
  {
    const TagLib::Matroska::SimpleTag::TargetTypeValue level = tag.targetTypeValue();
    if (isAlbumLevel(level) || namesAnotherEdition(tag))
      continue;

    const std::string name = StringUtils::ToUpper(tag.name().to8Bit(true));
    const std::string value = tag.toString().to8Bit(true);

    /*!
    * A tag with no TargetTypeValue describes the file: taggers that ignore the spec write album
    * metadata that way, and dropping it would lose the lot.
    */
    if (level == TagLib::Matroska::SimpleTag::None)
    {
      AddTagValue(fileTags, name, value);
      continue;
    }

    if (level != TagLib::Matroska::SimpleTag::Track)
      continue;

    /*!
    * A tag naming a chapter of an edition that was not selected describes a track this file will
    * not produce. Neither merging it into a chapter it does not describe nor promoting it to the
    * album is right, so it goes no further.
    */
    const unsigned long long chapterUid = tag.chapterUid();
    if (chapterUid > 0 && edition.unselectedChapterUids.count(chapterUid) != 0)
      continue;

    /*!
    * A tag with no ChapterUID describes the only track there is - MP3tag writes song tags that
    * way - so a single chapter file takes it. One naming a chapter goes to that chapter, and one
    * naming a chapter this file does not have falls back to the album. Zero is what TagLib
    * reports for a tag carrying no ChapterUID at all; every other value names a chapter.
    *
    * Which is why a named chapter is looked up whatever the file's chapter count. Taking the only
    * chapter first would give it every track level tag, including one naming a chapter that is
    * not it - a stale UID left by a tagger, or one belonging to a file this was cut from - and
    * the fallback above is the whole reason a name is read at all.
    */
    ChapterTags* target = nullptr;
    if (chapterUid > 0)
    {
      if (const auto it = edition.chapterIndex.find(chapterUid); it != edition.chapterIndex.end())
        target = &album.chapters[it->second];
    }
    else if (album.chapters.size() == 1)
      target = &album.chapters.front();

    // Either the file has no chapters at all, or names one it does not contain. Either way the tag
    // describes the file rather than a track of it.
    AddTagValue(target ? target->tags : fileTags, name, value);
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

    const FileProperties properties = ReadFileProperties(*matroskaFile);

    const SelectedEdition edition = CollectChapters(*matroskaFile, album);
    FillMissingEndTimes(album, properties.duration);

    CollectSimpleTags(matroskatag->simpleTagsList(), album, edition);

    /*!
    * The Segment title names the file rather than its album, and FFmpeg's demuxer reports it as an
    * unprefixed title. Say the same, so that a file holding one song is titled the same way
    * whichever reader read it.
    *
    * After the tags rather than before: a TITLE SimpleTag carrying no level is filed here too, and
    * a tag the file states outright says more about it than the container's own title. Seeding
    * first would leave AddTagValue() to drop the explicit one, TITLE being a name that holds a
    * single value.
    */
    if (!properties.segmentTitle.empty())
      album.fileTags.emplace("TITLE", properties.segmentTitle);
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

namespace
{
//! Below this a chapter is an artefact rather than a song.
constexpr long long MinimumTrackMilliseconds = 1000;
} // namespace

void MUSIC_INFO::CloseOpenEndedChapters(MatroskaAlbum& album, double fileDuration)
{
  for (auto& chapter : album.chapters)
    if (chapter.end <= 0.0 && fileDuration > chapter.start)
      chapter.end = fileDuration;
}

bool MUSIC_INFO::IsTrack(double start, double end)
{
  // Still open after CloseOpenEndedChapters() means nothing could measure it. Nothing says it is
  // an artefact either, so it is kept.
  if (end <= 0.0)
    return true;

  // Rounded to milliseconds, as fine as a chapter is ever written. Seconds are scaled from
  // nanosecond ticks, and the difference of two such doubles lands just under the whole second
  // often enough to drop chapters that are exactly one second long.
  return std::llround((end - start) * 1000.0) >= MinimumTrackMilliseconds;
}

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
