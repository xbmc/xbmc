/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DataFile.h"

#include "DataURI.h"

#include <algorithm>
#include <cstring>
#include <limits>

using namespace XFILE;

namespace
{
int FillStat(const size_t dataSize, struct __stat64* buffer)
{
  if (buffer == nullptr)
    return -1;

  *buffer = {};
  buffer->st_mode = _S_IFREG;
  buffer->st_size = static_cast<decltype(buffer->st_size)>(dataSize);
  return 0;
}
} // namespace

bool CDataFile::Open(const CURL& url)
{
  Close();

  if (!DataURI::Materialize(url, m_data))
  {
    Close();
    return false;
  }

  m_isOpen = true;
  return true;
}

bool CDataFile::Exists(const CURL& url)
{
  size_t decodedSize = 0;
  return DataURI::Validate(url, decodedSize);
}

int CDataFile::Stat(const CURL& url, struct __stat64* buffer)
{
  if (buffer == nullptr)
    return -1;

  size_t decodedSize = 0;
  if (!DataURI::Validate(url, decodedSize))
    return -1;

  return FillStat(decodedSize, buffer);
}

int CDataFile::Stat(struct __stat64* buffer)
{
  if (!m_isOpen || buffer == nullptr)
    return -1;

  return FillStat(m_data.size(), buffer);
}

ssize_t CDataFile::Read(void* buffer, size_t size)
{
  if (!m_isOpen || (buffer == nullptr && size != 0))
    return -1;

  if (m_position >= static_cast<int64_t>(m_data.size()))
    return 0;

  const size_t available = m_data.size() - static_cast<size_t>(m_position);
  const size_t readSize = std::min(size, available);
  if (readSize != 0)
    std::memcpy(buffer, m_data.data() + m_position, readSize);
  m_position += static_cast<int64_t>(readSize);
  return static_cast<ssize_t>(readSize);
}

int64_t CDataFile::Seek(int64_t position, int whence)
{
  if (!m_isOpen)
    return -1;

  int64_t base = 0;
  switch (whence)
  {
    case SEEK_SET:
      break;
    case SEEK_CUR:
      base = m_position;
      break;
    case SEEK_END:
      base = static_cast<int64_t>(m_data.size());
      break;
    default:
      return -1;
  }

  if ((position > 0 && base > std::numeric_limits<int64_t>::max() - position) ||
      (position < 0 && base < std::numeric_limits<int64_t>::min() - position))
    return -1;

  const int64_t newPosition = base + position;
  if (newPosition < 0)
    return -1;

  m_position = newPosition;
  return m_position;
}

void CDataFile::Close()
{
  m_data.clear();
  m_position = 0;
  m_isOpen = false;
}

int64_t CDataFile::GetPosition()
{
  return m_position;
}

int64_t CDataFile::GetLength()
{
  return static_cast<int64_t>(m_data.size());
}
