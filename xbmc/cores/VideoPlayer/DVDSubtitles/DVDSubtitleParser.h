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
  virtual bool Open(CDVDStreamInfo& hints) = 0;
  virtual void Reset() = 0;
  virtual std::shared_ptr<CDVDOverlay> Parse(double iPts) = 0;
  virtual const std::string& GetName() const = 0;
};

class CDVDSubtitleParserCollection
  : public CDVDSubtitleParser
{
public:
  explicit CDVDSubtitleParserCollection(const std::string& strFile) : m_filename(strFile) {}
  ~CDVDSubtitleParserCollection() override = default;
  std::shared_ptr<CDVDOverlay> Parse(double iPts) override
  {
    std::shared_ptr<CDVDOverlay> o = m_collection.Get(iPts);
    if(o == NULL)
      return o;
    return o->Clone();
  }
  void Reset() override { m_collection.Reset(); }

protected:
  CDVDSubtitleLineCollection m_collection;
  std::string m_filename;
};

class CDVDSubtitleParserText
     : public CDVDSubtitleParserCollection
{
public:
  CDVDSubtitleParserText(std::unique_ptr<CDVDSubtitleStream>&& stream,
                         const std::string& filename,
                         const char* name)
    : CDVDSubtitleParserCollection(filename), m_pStream(std::move(stream)), m_parserName(name)
  {
  }

  ~CDVDSubtitleParserText() override = default;

  /*
   * \brief Returns parser name
   */
  const std::string& GetName() const override { return m_parserName; }

protected:
  using CDVDSubtitleParserCollection::Open;
  bool Open()
  {
    if(m_pStream)
    {
      if (m_pStream->Seek(0))
        return true;
    }
    else
      m_pStream = std::make_unique<CDVDSubtitleStream>();

    return m_pStream->Open(m_filename);
  }

  std::unique_ptr<CDVDSubtitleStream> m_pStream;
  std::string m_parserName;
};
