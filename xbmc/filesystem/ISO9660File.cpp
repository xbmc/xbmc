/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ISO9660File.h"

#include "URL.h"

#include <cmath>

using namespace XFILE;

CISO9660File::CISO9660File() : m_iso(new ISO9660::IFS())
{
}

bool CISO9660File::Open(const CURL& url)
{
  if (m_iso && m_stat)
    return true;

  if (!m_iso->open(url.GetHostName().c_str()))
    return false;

  m_stat.reset(m_iso->stat(url.GetFileName().c_str()));

  if (!m_stat)
    return false;

  if (!m_stat->p_stat)
    return false;

  m_start = m_stat->p_stat->lsn;
  m_current = 0;

  return true;
}

int CISO9660File::Stat(const CURL& url, struct __stat64* buffer)
{
  if (!m_iso)
    return -1;

  if (!m_stat)
    return -1;

  if (!m_stat->p_stat)
    return -1;

  if (!buffer)
    return -1;

  *buffer = {};
  buffer->st_size = m_stat->p_stat->size;

  switch (m_stat->p_stat->type)
  {
    case 2:
      buffer->st_mode = S_IFDIR;
      break;
    case 1:
    default:
      buffer->st_mode = S_IFREG;
      break;
  }

  return 0;
}

ssize_t CISO9660File::Read(void* buffer, size_t size)
{
  const int maxSize = std::min(size, static_cast<size_t>(GetLength()));
  const int blocks = std::ceil(maxSize / ISO_BLOCKSIZE);

  if (m_current > std::ceil(GetLength() / ISO_BLOCKSIZE))
    return -1;

  auto read = m_iso->seek_read(buffer, m_start + m_current, blocks);

  m_current += blocks;

  return read;
}

int64_t CISO9660File::Seek(int64_t filePosition, int whence)
{
  int block = std::floor(filePosition / ISO_BLOCKSIZE);

  switch (whence)
  {
    case SEEK_SET:
      m_current = block;
      break;
    case SEEK_CUR:
      m_current += block;
      break;
    case SEEK_END:
      m_current = std::ceil(GetLength() / ISO_BLOCKSIZE) + block;
      break;
  }

  return m_current * ISO_BLOCKSIZE;
}

int64_t CISO9660File::GetLength()
{
  return m_stat->p_stat->size;
}

int64_t CISO9660File::GetPosition()
{
  return m_current * ISO_BLOCKSIZE;
}

bool CISO9660File::Exists(const CURL& url)
{
  return Open(url);
}

int CISO9660File::GetChunkSize()
{
  return ISO_BLOCKSIZE;
}
