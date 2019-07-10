/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "../DVDCodecs/Overlay/DVDOverlay.h"
#include "DVDSubtitleLineCollection.h"
#include "DVDSubtitleStream.h"

#include <memory>
#include <stdio.h>
#include <string>

class CDVDStreamInfo;

class CDVDSubtitleParser
{
public:
  virtual ~CDVDSubtitleParser() = default;
  virtual bool Open(CDVDStreamInfo &hints) = 0;
  virtual void Dispose() = 0;
  virtual void Reset() = 0;
  virtual CDVDOverlay* Parse(double iPts) = 0;
};

class CDVDSubtitleParserCollection
  : public CDVDSubtitleParser
{
public:
  explicit CDVDSubtitleParserCollection(const std::string& strFile) : m_filename(strFile) {}
  ~CDVDSubtitleParserCollection() override = default;
  CDVDOverlay* Parse(double iPts) override
  {
    CDVDOverlay* o = m_collection.Get(iPts);
    if(o == NULL)
      return o;
    return o->Clone();
  }
  void Reset() override { m_collection.Reset(); }
  void Dispose() override { m_collection.Clear(); }

protected:
  CDVDSubtitleLineCollection m_collection;
  std::string m_filename;
};

class CDVDSubtitleParserText
     : public CDVDSubtitleParserCollection
{
public:
  CDVDSubtitleParserText(std::unique_ptr<CDVDSubtitleStream> && stream, const std::string& filename)
    : CDVDSubtitleParserCollection(filename)
		, m_pStream(std::move(stream))
  {
  }

  ~CDVDSubtitleParserText() override = default;

protected:
  using CDVDSubtitleParserCollection::Open;
  bool Open()
  {
    if(m_pStream)
    {
      if(m_pStream->Seek(0, SEEK_SET) == 0)
        return true;
    }
    else
      m_pStream.reset(new CDVDSubtitleStream());

    return m_pStream->Open(m_filename);
  }

  std::unique_ptr<CDVDSubtitleStream> m_pStream;
};
