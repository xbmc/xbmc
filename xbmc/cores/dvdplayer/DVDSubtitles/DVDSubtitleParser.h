#pragma once

/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "../DVDCodecs/Overlay/DVDOverlay.h"
#include "DVDSubtitleStream.h"
#include "DVDSubtitleLineCollection.h"

#include <string>
#include <stdio.h>

class CDVDStreamInfo;

class CDVDSubtitleParser
{
public:
  virtual ~CDVDSubtitleParser() {}
  virtual bool Open(CDVDStreamInfo &hints) = 0;
  virtual void Dispose() = 0;
  virtual void Reset() = 0;
  virtual CDVDOverlay* Parse(double iPts) = 0;
};

class CDVDSubtitleParserCollection
  : public CDVDSubtitleParser
{
public:
  CDVDSubtitleParserCollection(const std::string& strFile) : m_filename(strFile) {}
  virtual ~CDVDSubtitleParserCollection() { }
  virtual CDVDOverlay* Parse(double iPts)
  {
    CDVDOverlay* o = m_collection.Get(iPts);
    if(o == NULL)
      return o;
    return o->Clone();
  }
  virtual void         Reset()            { m_collection.Reset(); }
  virtual void         Dispose()          { m_collection.Clear(); }

protected:
  CDVDSubtitleLineCollection m_collection;
  std::string                m_filename;
};

class CDVDSubtitleParserText
     : public CDVDSubtitleParserCollection
{
public:
  CDVDSubtitleParserText(CDVDSubtitleStream* stream, const std::string& filename)
    : CDVDSubtitleParserCollection(filename)
  {
    m_pStream  = stream;
  }

  virtual ~CDVDSubtitleParserText()
  {
    if(m_pStream)
      delete m_pStream;
  }

protected:

  bool Open()
  {
    if(m_pStream)
    {
      if(m_pStream->Seek(0, SEEK_SET) == 0)
        return true;
    }
    else
      m_pStream = new CDVDSubtitleStream();

    return m_pStream->Open(m_filename);
  }

  CDVDSubtitleStream* m_pStream;
};
