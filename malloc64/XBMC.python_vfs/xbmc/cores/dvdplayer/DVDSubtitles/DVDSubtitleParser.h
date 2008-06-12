#pragma once

/*
 *      Copyright (C) 2005-2008 Team XBMC
 *      http://www.xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "DVDCodecs/Overlay/DVDOverlay.h"
#include "DVDSubtitleStream.h"

class CDVDStreamInfo;

class CDVDSubtitleParser
{
public:
  CDVDSubtitleParser(CDVDSubtitleStream* pStream, const std::string& strFile)
  {
    m_pStream = pStream;
    m_strFileName = strFile;
  }
  
  virtual ~CDVDSubtitleParser()
  {
  }
  
  virtual bool Open(CDVDStreamInfo &hints) = 0;
  virtual void Dispose() = 0;
  virtual void Reset() = 0;
  
  virtual CDVDOverlay* Parse(double iPts) = 0;
  
protected:
  CDVDSubtitleStream* m_pStream;
  std::string m_strFileName;
};

