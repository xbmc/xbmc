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
#include "IFileTypes.h"
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
#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include <commons/ilog.h>
#include <libavformat/avformat.h>
#include <libavformat/avio.h>
#include <libavutil/dict.h>
#include <libavutil/mem.h>
#include <libavutil/rational.h>

using namespace XFILE;
using namespace MUSIC_INFO;

static int cfile_file_read(void* h, uint8_t* buf, int size)
{
  CFile* pFile = static_cast<CFile*>(h);
  return pFile->Read(buf, size);
}

static int64_t cfile_file_seek(void* h, int64_t pos, int whence)
{
  CFile* pFile = static_cast<CFile*>(h);
  if (whence == AVSEEK_SIZE)
    return pFile->GetLength();
  else
    return pFile->Seek(pos, whence & ~AVSEEK_FORCE);
}

namespace
{
/*!
 * Whether a chapter is long enough to stand as a track. Files carrying a chapter of a fraction of
 * a second happen; neither reader drops them, so that both describe a file the same way, and the
 * call is made here once for both.
 */
constexpr double MinimumTrackSeconds = 1.0;

bool IsTrack(double start, double end)
{
  return end - start >= MinimumTrackSeconds;
}
} // unnamed namespace

CAudioBookFileDirectory::~CAudioBookFileDirectory(void)
{
  if (m_fctx)
    avformat_close_input(&m_fctx);
  if (m_ioctx)
  {
    av_free(m_ioctx->buffer);
    av_free(m_ioctx);
  }
}

bool CAudioBookFileDirectory::GetDirectory(const CURL& url, CFileItemList& items)
{
  if (!m_fctx && !ContainsFiles(url))
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
    while ((tag = av_dict_get(m_fctx->metadata, "", tag, AV_DICT_IGNORE_SUFFIX)))
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
    if (!m_matroska || m_matroskaUrl != url.Get())
    {
      m_matroska = std::make_unique<MatroskaAlbum>(ReadMatroskaTags(url, m_fctx));
      m_matroskaUrl = url.Get();
    }
    if (!m_matroska->hasAlbumTags())
      return true;
    /*!
     * initially just get the (file) Album level tags to be use in subsequent tracks
     * (chapters) processed below to create Kodi music Songs
    */
    for (const auto& t : m_matroska->fileTags)
      MatroskaTagMapping::MapTag(t.first, t.second, separators, musicsep, albumtag);
  }

  std::string thumb;
  thumb = IMAGE_FILES::URLFromFile(url.Get(), "music");
  /*! Look for any embedded cover art
  * FFmpeg rather than TagLib: TagLib reads whole Matroska attachments eagerly, which is slow for
  * large attachments over SMB/NFS. Still unfixed as of TagLib 2.3.1 (it was expected there).
  */
  CMusicEmbeddedCoverLoaderFFmpeg::GetEmbeddedCover(m_fctx, albumtag);

  // now get the AudioCodec -------------------------------------
  bool haveFFmpegInfo = false;
  musicCodecInfo codec_info;
  haveFFmpegInfo = CMusicCodecInfoFFmpeg::GetMusicCodecInfo(m_fctx, codec_info);
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
      isAudioBook ? (m_fctx->chapters ? m_fctx->nb_chapters : 0) : m_matroska->chapters.size();
  int trackNumber = 0;
  bool chapter_error = false;
  for (size_t i = 0; i < chapterCount; ++i)
  {
    double start = 0.0;
    double end = 0.0;
    if (isAudioBook)
    {
      const AVChapter* chapter = m_fctx->chapters[i];
      if (!chapter || chapter->start < 0) // null or negative start time
        continue;
      start = chapter->start * av_q2d(chapter->time_base);
      end = chapter->end * av_q2d(chapter->time_base);
    }
    else
    {
      start = m_matroska->chapters[i].start;
      end = m_matroska->chapters[i].end;
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

      while ((tag = av_dict_get(m_fctx->chapters[i]->metadata, "", tag, AV_DICT_IGNORE_SUFFIX)))
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
      for (const auto& t : m_matroska->chapters[i].tags)
        MatroskaTagMapping::MapTag(t.first, t.second, separators, musicsep,
                                   *item->GetMusicInfoTag());
    }

    item->SetStartOffset(CUtil::ConvertSecsToMilliSecs(start));
    item->SetEndOffset(CUtil::ConvertSecsToMilliSecs(end));
    item->GetMusicInfoTag()->SetDuration(
        CUtil::ConvertMilliSecsToSecsInt(item->GetEndOffset() - item->GetStartOffset()));

    // Position in the album, for a track whose own tags did not number it.
    if (item->GetMusicInfoTag()->GetTrackNumber() <= 0)
      item->GetMusicInfoTag()->SetTrackNumber(trackNumber);
    item->GetMusicInfoTag()->SetLoaded(true);

    item->SetLabel(StringUtils::Format("{0:02}. {1} - {2}", trackNumber,
                                       item->GetMusicInfoTag()->GetAlbum(),
                                       item->GetMusicInfoTag()->GetTitle()));

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
  CFile file;
  if (!file.Open(url))
    return false;

  uint8_t* buffer = static_cast<uint8_t*>(av_malloc(32768));
  if (!buffer)
    return false;

  m_ioctx = avio_alloc_context(buffer, 32768, 0, &file, cfile_file_read, nullptr, cfile_file_seek);
  if (!m_ioctx)
  {
    av_free(buffer);
    return false;
  }

  m_fctx = avformat_alloc_context();
  if (!m_fctx)
  {
    av_free(m_ioctx->buffer);
    av_free(m_ioctx);
    m_ioctx = nullptr;
    return false;
  }
  m_fctx->pb = m_ioctx;
  m_fctx->flags |= AVFMT_FLAG_CUSTOM_IO;

  if (file.IoControl(IOControl::SEEK_POSSIBLE, nullptr) == 0)
    m_ioctx->seekable = 0;

  m_ioctx->max_packet_size = 32768;

  const AVInputFormat* iformat = nullptr;
  av_probe_input_buffer(m_ioctx, &iformat, url.Get().c_str(), nullptr, 0, 0);

  if (avformat_open_input(&m_fctx, url.Get().c_str(), iformat, nullptr) < 0)
  {
    if (m_fctx)
      avformat_close_input(&m_fctx);
    av_free(m_ioctx->buffer);
    av_free(m_ioctx);
    m_ioctx = nullptr;
    return false;
  }
  m_fctx->flags |= AVFMT_FLAG_NOPARSE;
  int err = avformat_find_stream_info(m_fctx, NULL);
  if (err < 0)
    CLog::Log(LOGERROR, "Can't detect codec info in file {}", url.GetRedacted());

  // m4b has no reader but FFmpeg, so its chapters are the only count there is.
  if (url.IsFileType("m4b"))
    return m_fctx->nb_chapters > 1;

  /*!
   * Ask the reader that will build the tracks how many there are, rather than FFmpeg on its
   * behalf: the two need not agree on a file whose chapters they read differently, and a file
   * turned away here is never offered to the reader that would have found an album in it. Holding
   * the result is what keeps that from costing a second parse in GetDirectory().
   */
  m_matroska = std::make_unique<MatroskaAlbum>(ReadMatroskaTags(url, m_fctx));
  m_matroskaUrl = url.Get();

  const auto tracks = std::count_if(m_matroska->chapters.begin(), m_matroska->chapters.end(),
                                    [](const ChapterTags& c) { return IsTrack(c.start, c.end); });
  return m_matroska->hasAlbumTags() && tracks > 1;
}
