/*
 *  Copyright (C) 2005-2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "AudioBookFileDirectory.h"

#include "FileItem.h"
#include "FileItemList.h"
#include "ServiceBroker.h"
#include "URL.h"
#include "Util.h"
#include "dbwrappers/Database.h"
#include "filesystem/File.h"
#include "imagefiles/ImageFileURL.h"
#include "music/MusicEmbeddedCoverLoaderFFmpeg.h"
#include "music/tags/MatroskaTagMapping.h"
#include "music/tags/MatroskaTagReader.h"
#include "music/tags/MusicCodecInfoFFmpeg.h"
#include "music/tags/MusicInfoTag.h"
#include "resources/LocalizeStrings.h"
#include "resources/ResourcesComponent.h"
#include "settings/AdvancedSettings.h"
#include "settings/SettingsComponent.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/log.h"

#include <algorithm>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include <commons/ilog.h>

extern "C"
{
#include <libavformat/avformat.h>
#include <libavutil/dict.h>
#include <libavutil/rational.h>
}

using namespace XFILE;
using namespace MUSIC_INFO;

bool CAudioBookFileDirectory::GetDirectory(const CURL& url, CFileItemList& items)
{
  // Every call brings its own URL, which need not be the one already open. Reopening is what
  // ContainsFiles() does, so ask it again.
  if ((!m_read || m_read->url != url.GetWithoutUserDetails()) && !ContainsFiles(url))
    return true;

  AVFormatContext* const fctx = m_demux.FormatContext();

  // An m4b is read off the context and has no other reader, so without one there is nothing here.
  if (!fctx && url.IsFileType("m4b"))
    return true;

  std::string title;
  std::string author;
  std::string album;
  std::string desc;

  std::vector<std::string> separators{" feat. ", " ft. ", " Feat. ", " Ft. ",  ";", ":",
                                      "|",       "#",     "/",       " with ", "&"};
  const std::string musicsep =
      CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_musicItemSeparator;
  if (musicsep.find_first_of(";/,&|#") == std::string::npos)
    separators.push_back(musicsep); // add custom music separator from as.xml

  const bool isAudioBook = url.IsFileType("m4b");

  // Some tags are relevant to the whole album - these are read first
  CMusicInfoTag albumtag;

  if (isAudioBook)
  {
    AVDictionaryEntry* tag = nullptr;
    while ((tag = av_dict_get(fctx->metadata, "", tag, AV_DICT_IGNORE_SUFFIX)))
    {
      if (StringUtils::CompareNoCase(tag->key, "title") == 0)
        title = tag->value;
      else if (StringUtils::CompareNoCase(tag->key, "album") == 0)
        album = tag->value;
      else if (StringUtils::CompareNoCase(tag->key, "artist") == 0)
        author = tag->value;
      else if (StringUtils::CompareNoCase(tag->key, "description") == 0)
        desc = tag->value;
    }
  }
  else
  {
    if (!m_read->album.hasAlbumTags())
      return true;
    /*!
     * initially just get the (file) Album level tags to be use in subsequent tracks
     * (chapters) processed below to create Kodi music Songs
    */
    /*!
     * Album level first, then what the file says about itself, so that a Segment title names the
     * song where the album's title only stood in. Each track's own tags come later still.
     */
    for (const auto& t : m_read->album.albumTags)
      MatroskaTagMapping::MapTag(t.first, t.second, MatroskaTagMapping::TagLevel::Album, separators,
                                 musicsep, albumtag);
    for (const auto& t : m_read->album.fileTags)
      MatroskaTagMapping::MapTag(t.first, t.second, MatroskaTagMapping::TagLevel::File, separators,
                                 musicsep, albumtag);
  }

  std::string thumb;
  thumb = IMAGE_FILES::URLFromFile(url.Get(), "music");
  /*! Look for any embedded cover art
  * FFmpeg rather than TagLib: TagLib reads whole Matroska attachments eagerly, which is slow for
  * large attachments over SMB/NFS. Still unfixed as of TagLib 2.3.1 (it was expected there).
  */
  CMusicEmbeddedCoverLoaderFFmpeg::GetEmbeddedCover(fctx, albumtag);

  // now get the AudioCodec -------------------------------------
  bool haveFFmpegInfo = false;
  musicCodecInfo codec_info;
  // Only where the streams were read: without that the fields below are zeroes rather than
  // measurements, and writing them into the tag states a sample rate the file never gave.
  haveFFmpegInfo =
      m_demux.HasStreamInfo() && CMusicCodecInfoFFmpeg::GetMusicCodecInfo(fctx, codec_info);
  if (haveFFmpegInfo) // use data from FFmpeg (taglib 2.3 does not support some codecs)
  {
    albumtag.SetBitRate(codec_info.bitRate);
    albumtag.SetSampleRate(codec_info.sampleRate);
    /*!
    * Additional Music properties (next PR - Add Album Codec Support to Music)
    * albumtag.SetBitsPerSample(codec_info.bitsPerSample);
    * albumtag.SetCodec(codec_info.codecName); // e.g. 'truehd_atmos', 'dts_ma', 'dts_hd', etc
    */
    albumtag.SetNoOfChannels(codec_info.channels);
    albumtag.SetDuration(codec_info.duration);
  }

  /*!
   * The chapters come from whichever reader read the file: ReadMatroskaTags for Matroska,
   * FFmpeg for an audiobook, which has no other reader. Taking the play ranges from one list and
   * the tags from the other pairs a chapter's tags with a different chapter's range as soon as the
   * two disagree on how many chapters there are - which they do over a file holding several
   * editions, or once either of them has dropped a chapter too short to be a track.
   */
  const size_t chapterCount =
      isAudioBook ? (fctx->chapters ? fctx->nb_chapters : 0) : m_read->album.chapters.size();
  int trackNumber = 0;
  bool chapter_error = false;
  for (size_t i = 0; i < chapterCount; ++i)
  {
    double start = 0.0;
    double end = 0.0;
    if (isAudioBook)
    {
      const AVChapter* chapter = fctx->chapters[i];
      if (!chapter || chapter->start < 0) // null or negative start time
        continue;
      start = chapter->start * av_q2d(chapter->time_base);
      end = chapter->end * av_q2d(chapter->time_base);
      /*!
       * m4b has no reader but FFmpeg, so nothing has closed its chapters on the way here. Where
       * the demuxer could measure the file, compute_chapters_end() closed them inside
       * avformat_find_stream_info(); where it could not, a chapter arrives ending before it
       * starts. Close it against the file's duration, which is what CloseOpenEndedChapters() does
       * for the reader paths - an end that is no end would otherwise pass IsTrack() and give the
       * track the whole book to play.
       */
      if (end <= start)
        end = m_demux.Duration();

      /*!
       * And where even that cannot close it, the chapter is dropped rather than kept. IsTrack()
       * keeps an open chapter because on the reader paths an end nothing supplied means one
       * nothing could measure. Here it means the opposite: FFmpeg clamped this chapter itself, so
       * the file stated an end and stated a bad one, and no later pass will give it a better.
       */
      if (end <= start)
      {
        CLog::Log(LOGWARNING,
                  "CAudioBookFileDirectory: Chapter with no usable end detected when scanning {} "
                  "Most likely this file needs the chapters correcting",
                  url.GetRedacted());
        chapter_error = true;
        continue;
      }
    }
    else
    {
      start = m_read->album.chapters[i].start;
      end = m_read->album.chapters[i].end;
    }

    if (!IsTrack(start, end))
    {
      CLog::Log(LOGWARNING,
                "CAudioBookFileDirectory: Tiny chapter of size {}s detected when scanning {} Most "
                "likely this file needs the chapters correcting",
                end - start, url.GetRedacted());
      chapter_error = true;
      continue;
    }

    // Numbers the tracks that are kept, so a dropped chapter leaves no gap in the album.
    ++trackNumber;

    std::shared_ptr<CFileItem> item(new CFileItem(url.Get(), false));
    *item->GetMusicInfoTag() = albumtag;

    if (isAudioBook)
    {
      AVDictionaryEntry* tag = nullptr;
      std::string chaptitle = StringUtils::Format(
          CServiceBroker::GetResourcesComponent().GetLocalizeStrings().Get(25010), trackNumber);
      std::string chapauthor;
      std::string chapalbum;

      while ((tag = av_dict_get(fctx->chapters[i]->metadata, "", tag, AV_DICT_IGNORE_SUFFIX)))
      {
        if (StringUtils::CompareNoCase(tag->key, "title") == 0)
          chaptitle = tag->value;
        else if (StringUtils::CompareNoCase(tag->key, "artist") == 0)
          chapauthor = tag->value;
        else if (StringUtils::CompareNoCase(tag->key, "album") == 0)
          chapalbum = tag->value;
      }
      item->GetMusicInfoTag()->SetTitle(chaptitle);
      item->GetMusicInfoTag()->SetAlbum(chapalbum.empty() ? album.empty() ? title : album
                                                          : chapalbum);
      item->GetMusicInfoTag()->SetArtist(chapauthor.empty() ? author : chapauthor);
      if (!desc.empty())
        item->GetMusicInfoTag()->SetComment(desc);
    }
    else
    {
      /*!
       * Drop the album level track number before reading the chapter's own tags, so that what
       * remains afterwards came from this chapter. Leaving it would read as the chapter having
       * numbered itself and suppress the positional fallback below on every track.
       */
      item->GetMusicInfoTag()->SetTrackNumber(0);

      // process chapter tags for this track, in file order
      for (const auto& t : m_read->album.chapters[i].tags)
        MatroskaTagMapping::MapTag(t.first, t.second, MatroskaTagMapping::TagLevel::Track,
                                   separators, musicsep, *item->GetMusicInfoTag());
    }

    item->SetStartOffset(CUtil::ConvertSecsToMilliSecs(start));
    item->SetEndOffset(CUtil::ConvertSecsToMilliSecs(end));
    // An unset end offset plays to the end of the file, which is what a chapter nothing could
    // close does. It has no length of its own to state, so the album's duration stands.
    if (item->GetEndOffset() > item->GetStartOffset())
      item->GetMusicInfoTag()->SetDuration(
          CUtil::ConvertMilliSecsToSecsInt(item->GetEndOffset() - item->GetStartOffset()));

    // Position in the album, for a track whose own tags did not number it.
    if (item->GetMusicInfoTag()->GetTrackNumber() <= 0)
      item->GetMusicInfoTag()->SetTrackNumber(trackNumber);
    item->GetMusicInfoTag()->SetLoaded(true);

    // The number the track ended up with, which is the chapter's own where it gave one.
    item->SetLabel(StringUtils::Format(
        "{0:02}. {1} - {2}", item->GetMusicInfoTag()->GetTrackNumber(),
        item->GetMusicInfoTag()->GetAlbum(), item->GetMusicInfoTag()->GetTitle()));

    item->SetProperty("item_start", item->GetStartOffset());
    item->SetProperty("audio_bookmark", item->GetStartOffset());
    if (!thumb.empty() && !chapter_error)
      item->SetArt("thumb", thumb);
    items.Add(item);
  }
  return true;
}

bool CAudioBookFileDirectory::Exists(const CURL& url)
{
  return CFile::Exists(url);
}

bool CAudioBookFileDirectory::ContainsFiles(const CURL& url)
{
  // Drop the previous file before opening the next: a failure here must not leave one file's tags
  // beside another's context.
  m_read.reset();
  const bool opened = m_demux.Open(url.Get());

  // From here on the context is this URL's, which is what m_read records for GetDirectory().
  m_read = CachedRead{url.GetWithoutUserDetails(), {}};

  // m4b has no reader but FFmpeg, so its chapters are the only count there is - and no context is
  // no chapters.
  if (url.IsFileType("m4b"))
    return opened && m_demux.FormatContext()->nb_chapters > 1;

  /*!
   * Ask the reader that will build the tracks how many there are, rather than FFmpeg on its
   * behalf: the two need not agree on a file whose chapters they read differently, and a file
   * turned away here is never offered to the reader that would have found an album in it. Holding
   * the result is what keeps that from costing a second parse in GetDirectory().
   *
   * Which is why a failed open does not turn one away either. Where the reader is TagLib it opens
   * the file itself, so a file FFmpeg would not probe is one it can still find an album in; where
   * it is FFmpeg it takes the null context for the empty album it is.
   */
  m_read->album = ReadMatroskaTags(url, m_demux.FormatContext());

  // Before counting: an open chapter cannot be told from a trailing artefact until it is closed.
  CloseOpenEndedChapters(m_read->album, m_demux.Duration());

  const auto tracks = std::count_if(m_read->album.chapters.begin(), m_read->album.chapters.end(),
                                    [](const ChapterTags& c) { return IsTrack(c.start, c.end); });
  return m_read->album.hasAlbumTags() && tracks > 1;
}
