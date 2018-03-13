/*
 *      Copyright (C) 2017 Team Kodi
 *      http://kodi.tv
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
 *  along with Kodi; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "VideoTagLoaderFFmpeg.h"

#include "addons/Scraper.h"
#include "FileItem.h"
#include "cores/FFmpeg.h"
#include "filesystem/File.h"
#include "NfoFile.h"
#include "utils/StringUtils.h"
#include "video/VideoInfoTag.h"

using namespace XFILE;

static int vfs_file_read(void *h, uint8_t* buf, int size)
{
  CFile* pFile = static_cast<CFile*>(h);
  return pFile->Read(buf, size);
}

static int64_t vfs_file_seek(void *h, int64_t pos, int whence)
{
  CFile* pFile = static_cast<CFile*>(h);
  if (whence == AVSEEK_SIZE)
    return pFile->GetLength();
  else
    return pFile->Seek(pos, whence & ~AVSEEK_FORCE);
}

CVideoTagLoaderFFmpeg::CVideoTagLoaderFFmpeg(const CFileItem& item,
                                             const ADDON::ScraperPtr& info,
                                             bool lookInFolder)
  : IVideoInfoTagLoader(item, info, lookInFolder)
  , m_info(info)
{
  m_file = new CFile;
  if (!m_file->Open(m_item.GetPath()))
  {
    delete m_file;
    m_file = nullptr;
    return;
  }

  int blockSize = m_file->GetChunkSize();
  int bufferSize = blockSize > 1 ? blockSize : 4096;
  uint8_t* buffer = (uint8_t*)av_malloc(bufferSize);
  m_ioctx = avio_alloc_context(buffer, bufferSize, 0,
                               m_file, vfs_file_read, nullptr,
                               vfs_file_seek);

  m_fctx = avformat_alloc_context();
  m_fctx->pb = m_ioctx;

  if (m_file->IoControl(IOCTRL_SEEK_POSSIBLE, nullptr) != 1)
    m_ioctx->seekable = 0;

  AVInputFormat* iformat = nullptr;
  av_probe_input_buffer(m_ioctx, &iformat, m_item.GetPath().c_str(), nullptr, 0, 0);
  if (avformat_open_input(&m_fctx, m_item.GetPath().c_str(), iformat, nullptr) < 0)
  {
    delete m_file;
    m_file = nullptr;
  }
}

CVideoTagLoaderFFmpeg::~CVideoTagLoaderFFmpeg()
{
  if (m_fctx)
    avformat_close_input(&m_fctx);
  if (m_ioctx)
  {
    av_free(m_ioctx->buffer);
    av_free(m_ioctx);
  }
  delete m_file;
}

bool CVideoTagLoaderFFmpeg::HasInfo() const
{
  if (!m_file)
    return false;

  for (size_t i = 0; i < m_fctx->nb_streams; ++i)
  {
    AVDictionaryEntry* avtag;
    avtag = av_dict_get(m_fctx->streams[i]->metadata, "filename", nullptr, AV_DICT_IGNORE_SUFFIX);
    if (avtag && strcmp(avtag->value,"kodi-metadata") == 0)
    {
      m_metadata_stream = i;
      return true;
    }
    else if (avtag && strcmp(avtag->value,"kodi-override-metadata") == 0)
    {
      m_metadata_stream = i;
      m_override_data = true;
      return true;
    }
  }

  AVDictionaryEntry* avtag = nullptr;
  if (m_item.IsType(".mkv"))
  {
    avtag = av_dict_get(m_fctx->metadata, "IMDBURL", nullptr, AV_DICT_IGNORE_SUFFIX);
    if (!avtag)
      avtag = av_dict_get(m_fctx->metadata, "TMDBURL", nullptr, AV_DICT_IGNORE_SUFFIX);
    if (!avtag)
      avtag = av_dict_get(m_fctx->metadata, "TITLE", nullptr, AV_DICT_IGNORE_SUFFIX);
  } else if (m_item.IsType(".mp4") || m_item.IsType(".avi"))
    avtag = av_dict_get(m_fctx->metadata, "title", nullptr, AV_DICT_IGNORE_SUFFIX);

  return avtag != nullptr;
}

CInfoScanner::INFO_TYPE CVideoTagLoaderFFmpeg::Load(CVideoInfoTag& tag,
                                                    bool, std::vector<EmbeddedArt>* art)
{
  if (m_item.IsType(".mkv"))
    return LoadMKV(tag, art);
  else if (m_item.IsType(".mp4"))
    return LoadMP4(tag, art);
  else if (m_item.IsType(".avi"))
    return LoadAVI(tag, art);
  else
    return CInfoScanner::NO_NFO;

}

CInfoScanner::INFO_TYPE CVideoTagLoaderFFmpeg::LoadMKV(CVideoInfoTag& tag,
                                                       std::vector<EmbeddedArt>* art)
{
  // embedded art
  for (size_t i = 0; i < m_fctx->nb_streams; ++i)
  {
    if ((m_fctx->streams[i]->disposition & AV_DISPOSITION_ATTACHED_PIC) == 0)
      continue;
    AVDictionaryEntry* avtag;
    avtag = av_dict_get(m_fctx->streams[i]->metadata, "filename", nullptr, AV_DICT_IGNORE_SUFFIX);
    std::string value;
    if (avtag)
      value =  avtag->value;
    avtag = av_dict_get(m_fctx->streams[i]->metadata, "mimetype", nullptr, AV_DICT_IGNORE_SUFFIX);
    if (!value.empty() && avtag)
    {
      std::string type;
      if (value == "fanart.png" || value == "fanart.jpg")
        type = "fanart";
      else if (value == "cover.png" || value == "cover.jpg")
        type = "poster";
      else if (value == "small_cover.png" || value == "small_cover.jpg")
        type = "thumb";
      if (type.empty())
        continue;
      size_t size = m_fctx->streams[i]->attached_pic.size;
      if (art)
        art->emplace_back(EmbeddedArt(m_fctx->streams[i]->attached_pic.data,
                                      size, avtag->value, type));
      else
        tag.m_coverArt.emplace_back(EmbeddedArtInfo(size, avtag->value, type));
    }
  }

  if (m_metadata_stream != -1)
  {
    CNfoFile nfo;
    auto* data = m_fctx->streams[m_metadata_stream]->codecpar->extradata;
    const char* content = reinterpret_cast<const char*>(data);
    nfo.GetDetails(tag, content);
    if (!m_override_data)
      return CInfoScanner::FULL_NFO;
  }

  AVDictionaryEntry* avtag = nullptr;
  bool hastag = false;
  while ((avtag = av_dict_get(m_fctx->metadata, "", avtag, AV_DICT_IGNORE_SUFFIX)))
  {
    if (strcasecmp(avtag->key, "title") == 0)
      tag.SetTitle(avtag->value);
    else if (strcasecmp(avtag->key, "director") == 0)
    {
      std::vector<std::string> dirs = StringUtils::Split(avtag->value, " / ");
      tag.SetDirector(dirs);
    }
    else if (strcasecmp(avtag->key, "date_released") == 0)
      tag.SetYear(atoi(avtag->value));
    hastag = true;
  }

  return hastag ? CInfoScanner::TITLE_NFO : CInfoScanner::NO_NFO;
}

// https://wiki.multimedia.cx/index.php/FFmpeg_Metadata
CInfoScanner::INFO_TYPE CVideoTagLoaderFFmpeg::LoadMP4(CVideoInfoTag& tag,
                                                       std::vector<EmbeddedArt>* art)
{
  bool hasfull = false;
  AVDictionaryEntry* avtag = nullptr;
  // If either description or synopsis is found, assume user wants to use the tag info only
  while ((avtag = av_dict_get(m_fctx->metadata, "", avtag, AV_DICT_IGNORE_SUFFIX)))
  {
    if (strcmp(avtag->key, "title") == 0)
      tag.SetTitle(avtag->value);
    else if (strcmp(avtag->key, "composer") == 0)
      tag.SetWritingCredits(StringUtils::Split(avtag->value, " / "));
    else if (strcmp(avtag->key, "genre") == 0)
      tag.SetGenre(StringUtils::Split(avtag->value, " / "));
    else if (strcmp(avtag->key,"date") == 0)
      tag.SetYear(atoi(avtag->value));
    else if (strcmp(avtag->key, "description") == 0)
    {
      tag.SetPlotOutline(avtag->value);
      hasfull = true;
    }
    else if (strcmp(avtag->key, "synopsis") == 0)
    {
      tag.SetPlot(avtag->value);
      hasfull = true;
    }
    else if (strcmp(avtag->key, "track") == 0)
      tag.m_iTrack = std::stoi(avtag->value);
    else if (strcmp(avtag->key, "album") == 0)
      tag.SetAlbum(avtag->value);
    else if (strcmp(avtag->key, "artist") == 0)
      tag.SetArtist(StringUtils::Split(avtag->value, " / "));
  }

  for (size_t i = 0; i < m_fctx->nb_streams; ++i)
  {
    if ((m_fctx->streams[i]->disposition & AV_DISPOSITION_ATTACHED_PIC) == 0)
      continue;

    size_t size = m_fctx->streams[i]->attached_pic.size;
    const std::string type = "poster";
    if (art)
      art->emplace_back(EmbeddedArt(m_fctx->streams[i]->attached_pic.data,
                                    size, "image/png", type));
    else
      tag.m_coverArt.emplace_back(EmbeddedArtInfo(size, "image/png", type));
  }

  return hasfull ? CInfoScanner::FULL_NFO : CInfoScanner::TITLE_NFO;
}

// https://wiki.multimedia.cx/index.php/FFmpeg_Metadata#AVI
CInfoScanner::INFO_TYPE CVideoTagLoaderFFmpeg::LoadAVI(CVideoInfoTag& tag,
                                                       std::vector<EmbeddedArt>* art)
{
  AVDictionaryEntry* avtag = nullptr;
  while ((avtag = av_dict_get(m_fctx->metadata, "", avtag, AV_DICT_IGNORE_SUFFIX)))
  {
    if (strcmp(avtag->key, "title") == 0)
      tag.SetTitle(avtag->value);
    else if (strcmp(avtag->key,"date") == 0)
      tag.SetYear(atoi(avtag->value));
  }

  return CInfoScanner::TITLE_NFO;
}
