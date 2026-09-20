/*
 *  Copyright (C) 2005-2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "FFmpegVfsContext.h"

#include "File.h"
#include "IFileTypes.h"
#include "URL.h"
#include "utils/log.h"

#include <algorithm>
#include <cstdint>

#include <commons/ilog.h>

extern "C"
{
#include <libavformat/avformat.h>
#include <libavformat/avio.h>
#include <libavutil/mem.h>
}

using namespace XFILE;

namespace
{
//! What FFmpeg reads through, unless the file itself hands back more than that in one read.
constexpr int FFMPEG_BUFFER_SIZE = 32768;

int VfsRead(void* h, uint8_t* buf, int size)
{
  return static_cast<CFile*>(h)->Read(buf, size);
}

int64_t VfsSeek(void* h, int64_t pos, int whence)
{
  auto* file = static_cast<CFile*>(h);
  return whence == AVSEEK_SIZE ? file->GetLength() : file->Seek(pos, whence & ~AVSEEK_FORCE);
}
} // unnamed namespace

CFFmpegVfsContext::CFFmpegVfsContext() = default;

CFFmpegVfsContext::~CFFmpegVfsContext()
{
  Close();
}

void CFFmpegVfsContext::Close()
{
  // AVFMT_FLAG_CUSTOM_IO is what keeps this from closing the IO context too; it is ours to free.
  if (m_fctx)
    avformat_close_input(&m_fctx);
  if (m_ioctx)
  {
    av_free(m_ioctx->buffer);
    av_free(m_ioctx);
    m_ioctx = nullptr;
  }
  m_file.reset();
  m_haveStreamInfo = false;
}

double CFFmpegVfsContext::Duration() const
{
  if (!m_fctx || m_fctx->duration == AV_NOPTS_VALUE || m_fctx->duration <= 0)
    return 0.0;
  return static_cast<double>(m_fctx->duration) / AV_TIME_BASE;
}

bool CFFmpegVfsContext::Open(const std::string& path)
{
  Close();

  m_file = std::make_unique<CFile>();
  if (!m_file->Open(path))
  {
    Close();
    return false;
  }

  // A buffer under what one read returns only costs more reads, which over SMB or NFS is the cost
  // that matters.
  const int bufferSize = std::max(FFMPEG_BUFFER_SIZE, m_file->GetChunkSize());
  auto* buffer = static_cast<uint8_t*>(av_malloc(bufferSize));
  if (!buffer)
  {
    Close();
    return false;
  }

  m_ioctx = avio_alloc_context(buffer, bufferSize, 0, m_file.get(), VfsRead, nullptr, VfsSeek);
  if (!m_ioctx)
  {
    av_free(buffer);
    Close();
    return false;
  }
  m_ioctx->max_packet_size = bufferSize;

  // Say so rather than let FFmpeg find out by seeking on something that cannot, such as a pipe.
  if (m_file->IoControl(IOControl::SEEK_POSSIBLE, nullptr) != 1)
    m_ioctx->seekable = 0;

  m_fctx = avformat_alloc_context();
  if (!m_fctx)
  {
    Close();
    return false;
  }
  m_fctx->pb = m_ioctx;
  m_fctx->flags |= AVFMT_FLAG_CUSTOM_IO;

  const AVInputFormat* iformat = nullptr;
  av_probe_input_buffer(m_ioctx, &iformat, path.c_str(), nullptr, 0, 0);

  if (avformat_open_input(&m_fctx, path.c_str(), iformat, nullptr) < 0)
  {
    // avformat_open_input() has freed and cleared m_fctx already; the IO context is still ours.
    Close();
    return false;
  }

  // Nothing here decodes: the headers, the chapters and the attachments are all that is wanted.
  m_fctx->flags |= AVFMT_FLAG_NOPARSE;
  /*!
   * A failure here does not fail the open: the tags, the chapters and the attachments come from
   * the header and are there either way, and a caller after those is entitled to them. It is only
   * what is read off the streams that has none of it, so the outcome is recorded for
   * HasStreamInfo() to hand to whoever asks for that.
   */
  m_haveStreamInfo = avformat_find_stream_info(m_fctx, nullptr) >= 0;
  if (!m_haveStreamInfo)
    CLog::Log(LOGERROR, "CFFmpegVfsContext: can't detect codec info in file {}",
              CURL::GetRedacted(path));

  return true;
}
