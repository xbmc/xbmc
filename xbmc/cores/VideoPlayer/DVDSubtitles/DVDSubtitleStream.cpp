/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DVDSubtitleStream.h"

#include "DVDInputStreams/DVDFactoryInputStream.h"
#include "DVDInputStreams/DVDInputStream.h"
#include "utils/CharsetConverter.h"
#include "utils/CharsetDetection.h"
#include "utils/URIUtils.h"
#include "utils/Utf8Utils.h"
#include "utils/log.h"

#include <cstdio>
#include <cstring>
#include <memory>

CDVDSubtitleStream::CDVDSubtitleStream() = default;

CDVDSubtitleStream::~CDVDSubtitleStream() = default;

bool CDVDSubtitleStream::Open(const std::string& strFile)
{
  CFileItem item(strFile, false);
  item.SetContentLookup(false);
  std::shared_ptr<CDVDInputStream> pInputStream(CDVDFactoryInputStream::CreateInputStream(NULL, item));
  if (pInputStream && pInputStream->Open())
  {
    // prepare buffer
    size_t totalread = 0;
    std::vector<uint8_t> buf(1024);

    if (URIUtils::HasExtension(strFile, ".sub") && IsIncompatible(pInputStream.get(), buf, &totalread))
    {
      CLog::Log(LOGDEBUG,
                "{}: file {} seems to be a vob sub"
                "file without an idx file, skipping it",
                __FUNCTION__, CURL::GetRedacted(pInputStream->GetFileName()));
      buf.clear();
      return false;
    }

    static const size_t chunksize = 64 * 1024;

    int read;
    do
    {
      if (totalread == buf.size())
        buf.resize(buf.size() + chunksize);

      read = pInputStream->Read(buf.data() + totalread, static_cast<int>(buf.size() - totalread));
      if (read > 0)
        totalread += read;
    } while (read > 0);

    if (!totalread)
      return false;

    std::string tmpStr(reinterpret_cast<char*>(buf.data()), totalread);
    buf.clear();

    std::string enc(CCharsetDetection::GetBomEncoding(tmpStr));
    if (enc == "UTF-8" || (enc.empty() && CUtf8Utils::isValidUtf8(tmpStr)))
      m_subtitleData = tmpStr;
    else if (!enc.empty())
    {
      std::string converted;
      g_charsetConverter.ToUtf8(enc, tmpStr, converted);
      if (converted.empty())
        return false;

      m_subtitleData = converted;
    }
    else
    {
      std::string converted;
      g_charsetConverter.subtitleCharsetToUtf8(tmpStr, converted);
      if (converted.empty())
        return false;

      m_subtitleData = converted;
    }

    m_arrayParser.Reset(m_subtitleData.c_str(), m_subtitleData.size());
    return true;
  }

  return false;
}

bool CDVDSubtitleStream::IsIncompatible(CDVDInputStream* pInputStream,
                                        std::vector<uint8_t>& buf,
                                        size_t* bytesRead)
{
  if (!pInputStream)
    return true;

  static const uint8_t vobsub[] = { 0x00, 0x00, 0x01, 0xBA };

  int read = pInputStream->Read(buf.data(), static_cast<int>(buf.size()));

  if (read < 0)
  {
    return true;
  }
  else
  {
    *bytesRead = (size_t)read;
  }

  if (read >= 4)
  {
    if (!std::memcmp(buf.data(), vobsub, 4))
      return true;
  }

  return false;
}

std::string CDVDSubtitleStream::Read(int length)
{
  return m_arrayParser.ReadNextString(length);
}

bool CDVDSubtitleStream::Seek(int offset)
{
  return m_arrayParser.SetPosition(offset);
}

bool CDVDSubtitleStream::ReadLine(std::string& line)
{
  return m_arrayParser.ReadNextLine(line);
}
