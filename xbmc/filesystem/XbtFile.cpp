/*
 *  Copyright (C) 2015-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "XbtFile.h"

#include "URL.h"
#include "filesystem/File.h"
#include "filesystem/XbtManager.h"
#include "guilib/TextureBundleXBT.h"
#include "guilib/XBTFReader.h"
#include "utils/StringUtils.h"

#include <algorithm>
#include <climits>
#include <cstring>
#include <memory>
#include <string>
#include <utility>

namespace XFILE
{

CXbtFile::CXbtFile()
  : m_url(),
    m_xbtfReader(nullptr),
    m_xbtfFile(),
    m_frameStartPositions(),
    m_unpackedFrames()
{ }

CXbtFile::~CXbtFile()
{
  Close();
}

bool CXbtFile::Open(const CURL& url)
{
  if (m_open)
    return false;

  CURL xbtUrl(url);
  xbtUrl.SetOptions("");

  if (!GetReaderAndFile(url, m_xbtfReader, m_xbtfFile))
    return false;

  m_url = url;
  m_open = true;

  uint64_t frameStartPosition = 0;
  const auto& frames = m_xbtfFile.GetFrames();
  for (const auto& frame : frames)
  {
    m_frameStartPositions.push_back(frameStartPosition);

    frameStartPosition += frame.GetUnpackedSize();
  }

  m_frameIndex = 0;
  m_positionWithinFrame = 0;
  m_positionTotal = 0;

  m_unpackedFrames.resize(frames.size());

  return true;
}

void CXbtFile::Close()
{
  m_unpackedFrames.clear();
  m_frameIndex = 0;
  m_positionWithinFrame = 0;
  m_positionTotal = 0;
  m_frameStartPositions.clear();
  m_open = false;
}

bool CXbtFile::Exists(const CURL& url)
{
  CXBTFFile dummy;
  return GetFile(url, dummy);
}

int64_t CXbtFile::GetPosition()
{
  if (!m_open)
    return -1;

  return m_positionTotal;
}

int64_t CXbtFile::GetLength()
{
  if (!m_open)
    return -1;

  return static_cast<int>(m_xbtfFile.GetUnpackedSize());
}

int CXbtFile::Stat(struct __stat64 *buffer)
{
  if (!m_open)
    return -1;

  return Stat(m_url, buffer);
}

int CXbtFile::Stat(const CURL& url, struct __stat64* buffer)
{
  if (!buffer)
    return -1;

  *buffer = {};

  // check if the file exists
  CXBTFReaderPtr reader;
  CXBTFFile file;
  if (!GetReaderAndFile(url, reader, file))
  {
    // check if the URL points to the XBT file itself
    if (!url.GetFileName().empty() || !CFile::Exists(url.GetHostName()))
      return -1;

    // stat the XBT file itself
    if (XFILE::CFile::Stat(url.GetHostName(), buffer) != 0)
      return -1;

    buffer->st_mode = _S_IFDIR;
    return 0;
  }

  // stat the XBT file itself
  if (XFILE::CFile::Stat(url.GetHostName(), buffer) != 0)
    return -1;

  buffer->st_size = file.GetUnpackedSize();

  return 0;
}

ssize_t CXbtFile::Read(void* lpBuf, size_t uiBufSize)
{
  if (lpBuf == nullptr || !m_open)
    return -1;

  // nothing to read
  if (m_xbtfFile.GetFrames().empty() || m_positionTotal >= GetLength())
    return 0;

  // we can't read more than is left
  if (static_cast<int64_t>(uiBufSize) > GetLength() - m_positionTotal)
    uiBufSize = static_cast<ssize_t>(GetLength() - m_positionTotal);

  // we can't read more than we can signal with the return value
  if (uiBufSize > SSIZE_MAX)
    uiBufSize = SSIZE_MAX;

  const auto& frames = m_xbtfFile.GetFrames();

  size_t remaining = uiBufSize;
  while (remaining > 0)
  {
    const CXBTFFrame& frame = frames[m_frameIndex];

    // check if we have already unpacked the current frame
    if (m_unpackedFrames[m_frameIndex].empty())
    {
      // unpack the data from the current frame
      std::vector<uint8_t> unpackedFrame =
          CTextureBundleXBT::UnpackFrame(*m_xbtfReader.get(), frame);
      if (unpackedFrame.empty())
      {
        Close();
        return -1;
      }

      m_unpackedFrames[m_frameIndex] = std::move(unpackedFrame);
    }

    // determine how many bytes we need to copy from the current frame
    uint64_t remainingBytesInFrame = frame.GetUnpackedSize() - m_positionWithinFrame;
    size_t bytesToCopy = remaining;
    if (remainingBytesInFrame <= SIZE_MAX)
      bytesToCopy = std::min(remaining, static_cast<size_t>(remainingBytesInFrame));

    // copy the data
    memcpy(lpBuf, m_unpackedFrames[m_frameIndex].data() + m_positionWithinFrame, bytesToCopy);
    m_positionWithinFrame += bytesToCopy;
    m_positionTotal += bytesToCopy;
    remaining -= bytesToCopy;

    // check if we need to go to the next frame and there is a next frame
    if (m_positionWithinFrame >= frame.GetUnpackedSize() && m_frameIndex < frames.size() - 1)
    {
      m_positionWithinFrame = 0;
      m_frameIndex += 1;
    }
  }

  return uiBufSize;
}

int64_t CXbtFile::Seek(int64_t iFilePosition, int iWhence)
{
  if (!m_open)
    return -1;

  int64_t newPosition = m_positionTotal;
  switch (iWhence)
  {
    case SEEK_SET:
      newPosition = iFilePosition;
      break;

    case SEEK_CUR:
      newPosition += iFilePosition;
      break;

    case SEEK_END:
      newPosition = GetLength() + iFilePosition;
      break;

    // unsupported seek mode
    default:
      return -1;
  }

  // can't seek before the beginning or after the end of the file
  if (newPosition < 0 || newPosition >= GetLength())
    return -1;

  // seeking backwards doesn't require additional work
  if (newPosition <= m_positionTotal)
  {
    m_positionTotal = newPosition;
    return m_positionTotal;
  }

  // when seeking forward we need to unpack all frames we seek past
  const auto& frames = m_xbtfFile.GetFrames();
  while (m_positionTotal < newPosition)
  {
    const CXBTFFrame& frame = frames[m_frameIndex];

    // check if we have already unpacked the current frame
    if (m_unpackedFrames[m_frameIndex].empty())
    {
      // unpack the data from the current frame
      std::vector<uint8_t> unpackedFrame =
          CTextureBundleXBT::UnpackFrame(*m_xbtfReader.get(), frame);
      if (unpackedFrame.empty())
      {
        Close();
        return -1;
      }

      m_unpackedFrames[m_frameIndex] = std::move(unpackedFrame);
    }

    int64_t remainingBytesToSeek = newPosition - m_positionTotal;
    // check if the new position is within the current frame
    uint64_t remainingBytesInFrame = frame.GetUnpackedSize() - m_positionWithinFrame;
    if (static_cast<uint64_t>(remainingBytesToSeek) < remainingBytesInFrame)
    {
      m_positionWithinFrame += remainingBytesToSeek;
      break;
    }

    // otherwise move to the end of the frame
    m_positionTotal += remainingBytesInFrame;
    m_positionWithinFrame += remainingBytesInFrame;

    // and go to the next frame if there is a next frame
    if (m_frameIndex < frames.size() - 1)
    {
      m_positionWithinFrame = 0;
      m_frameIndex += 1;
    }
  }

  m_positionTotal = newPosition;
  return m_positionTotal;
}

uint32_t CXbtFile::GetImageWidth() const
{
  CXBTFFrame frame;
  if (!GetFirstFrame(frame))
    return false;

  return frame.GetWidth();
}

uint32_t CXbtFile::GetImageHeight() const
{
  CXBTFFrame frame;
  if (!GetFirstFrame(frame))
    return false;

  return frame.GetHeight();
}

XB_FMT CXbtFile::GetImageFormat() const
{
  CXBTFFrame frame;
  if (!GetFirstFrame(frame))
    return XB_FMT_UNKNOWN;

  return frame.GetFormat();
}

bool CXbtFile::HasImageAlpha() const
{
  CXBTFFrame frame;
  if (!GetFirstFrame(frame))
    return false;

  return frame.HasAlpha();
}

bool CXbtFile::GetFirstFrame(CXBTFFrame& frame) const
{
  if (!m_open)
    return false;

  const auto& frames = m_xbtfFile.GetFrames();
  if (frames.empty())
    return false;

  frame = frames.at(0);
  return true;
}

bool CXbtFile::GetReader(const CURL& url, CXBTFReaderPtr& reader)
{
  CURL xbtUrl(url);
  xbtUrl.SetOptions("");

  return CXbtManager::GetInstance().GetReader(xbtUrl, reader);
}

bool CXbtFile::GetReaderAndFile(const CURL& url, CXBTFReaderPtr& reader, CXBTFFile& file)
{
  if (!GetReader(url, reader))
    return false;

  CURL xbtUrl(url);
  xbtUrl.SetOptions("");

  // CXBTFReader stores all filenames in lower case
  std::string fileName = xbtUrl.GetFileName();
  StringUtils::ToLower(fileName);

  return reader->Get(fileName, file);
}

bool CXbtFile::GetFile(const CURL& url, CXBTFFile& file)
{
  CXBTFReaderPtr reader;
  return GetReaderAndFile(url, reader, file);
}

}
