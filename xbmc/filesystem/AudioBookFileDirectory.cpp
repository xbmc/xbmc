/*
 *  Copyright (C) 2014 Arne Morten Kvarving
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "AudioBookFileDirectory.h"

#include "FileItem.h"
#include "FileItemList.h"
#include "TextureDatabase.h"
#include "URL.h"
#include "Util.h"
#include "cores/FFmpeg.h"
#include "filesystem/File.h"
#include "guilib/LocalizeStrings.h"
#include "music/tags/MusicInfoTag.h"
#include "utils/StringUtils.h"

using namespace XFILE;

static int cfile_file_read(void *h, uint8_t* buf, int size)
{
  CFile* pFile = static_cast<CFile*>(h);
  return pFile->Read(buf, size);
}

static int64_t cfile_file_seek(void *h, int64_t pos, int whence)
{
  CFile* pFile = static_cast<CFile*>(h);
  if(whence == AVSEEK_SIZE)
    return pFile->GetLength();
  else
    return pFile->Seek(pos, whence & ~AVSEEK_FORCE);
}

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

bool CAudioBookFileDirectory::GetDirectory(const CURL& url,
                                           CFileItemList &items)
{
  if (!m_fctx && !ContainsFiles(url))
    return true;

  std::string title;
  std::string author;
  std::string album;

  AVDictionaryEntry* tag=nullptr;
  while ((tag = av_dict_get(m_fctx->metadata, "", tag, AV_DICT_IGNORE_SUFFIX)))
  {
    if (StringUtils::CompareNoCase(tag->key, "title") == 0)
      title = tag->value;
    else if (StringUtils::CompareNoCase(tag->key, "album") == 0)
      album = tag->value;
    else if (StringUtils::CompareNoCase(tag->key, "artist") == 0)
      author = tag->value;
  }

  std::string thumb;
  if (m_fctx->nb_chapters > 1)
    thumb = CTextureUtils::GetWrappedImageURL(url.Get(), "music");

  for (size_t i=0;i<m_fctx->nb_chapters;++i)
  {
    tag=nullptr;
    std::string chaptitle = StringUtils::Format(g_localizeStrings.Get(25010), i + 1);
    std::string chapauthor;
    std::string chapalbum;
    while ((tag=av_dict_get(m_fctx->chapters[i]->metadata, "", tag, AV_DICT_IGNORE_SUFFIX)))
    {
      if (StringUtils::CompareNoCase(tag->key, "title") == 0)
        chaptitle = tag->value;
      else if (StringUtils::CompareNoCase(tag->key, "artist") == 0)
        chapauthor = tag->value;
      else if (StringUtils::CompareNoCase(tag->key, "album") == 0)
        chapalbum = tag->value;
    }
    CFileItemPtr item(new CFileItem(url.Get(),false));
    item->GetMusicInfoTag()->SetTrackNumber(i+1);
    item->GetMusicInfoTag()->SetLoaded(true);
    item->GetMusicInfoTag()->SetTitle(chaptitle);
    if (album.empty())
      item->GetMusicInfoTag()->SetAlbum(title);
    else if (chapalbum.empty())
      item->GetMusicInfoTag()->SetAlbum(album);
    else
      item->GetMusicInfoTag()->SetAlbum(chapalbum);
    if (chapauthor.empty())
      item->GetMusicInfoTag()->SetArtist(author);
    else
      item->GetMusicInfoTag()->SetArtist(chapauthor);

    item->SetLabel(StringUtils::Format("{0:02}. {1} - {2}", i + 1,
                                       item->GetMusicInfoTag()->GetAlbum(),
                                       item->GetMusicInfoTag()->GetTitle()));
    item->SetStartOffset(CUtil::ConvertSecsToMilliSecs(m_fctx->chapters[i]->start *
                                                       av_q2d(m_fctx->chapters[i]->time_base)));
    item->SetEndOffset(m_fctx->chapters[i]->end * av_q2d(m_fctx->chapters[i]->time_base));
    int compare = m_fctx->streams[0]->duration * av_q2d(m_fctx->streams[0]->time_base);
    if (item->GetEndOffset() < 0 || item->GetEndOffset() > compare)
    {
      if (i < m_fctx->nb_chapters-1)
        item->SetEndOffset(m_fctx->chapters[i + 1]->start *
                           av_q2d(m_fctx->chapters[i + 1]->time_base));
      else
        item->SetEndOffset(compare);
    }
    item->SetEndOffset(CUtil::ConvertSecsToMilliSecs(item->GetEndOffset()));
    item->GetMusicInfoTag()->SetDuration(
        CUtil::ConvertMilliSecsToSecsInt(item->GetEndOffset() - item->GetStartOffset()));
    item->SetProperty("item_start", item->GetStartOffset());
    if (!thumb.empty())
      item->SetArt("thumb", thumb);
    items.Add(item);
  }

  return true;
}

bool CAudioBookFileDirectory::Exists(const CURL& url)
{
  return CFile::Exists(url) && ContainsFiles(url);
}

bool CAudioBookFileDirectory::ContainsFiles(const CURL& url)
{
  CFile file;
  if (!file.Open(url))
    return false;

  uint8_t* buffer = (uint8_t*)av_malloc(32768);
  m_ioctx = avio_alloc_context(buffer, 32768, 0, &file, cfile_file_read,
                               nullptr, cfile_file_seek);

  m_fctx = avformat_alloc_context();
  m_fctx->pb = m_ioctx;

  if (file.IoControl(IOCTRL_SEEK_POSSIBLE, nullptr) == 0)
    m_ioctx->seekable = 0;

  m_ioctx->max_packet_size = 32768;

  const AVInputFormat* iformat = nullptr;
  av_probe_input_buffer(m_ioctx, &iformat, url.Get().c_str(), nullptr, 0, 0);

  bool contains = false;
  if (avformat_open_input(&m_fctx, url.Get().c_str(), iformat, nullptr) < 0)
  {
    if (m_fctx)
      avformat_close_input(&m_fctx);
    av_free(m_ioctx->buffer);
    av_free(m_ioctx);
    return false;
  }

  contains = m_fctx->nb_chapters > 1;

  return contains;
}
