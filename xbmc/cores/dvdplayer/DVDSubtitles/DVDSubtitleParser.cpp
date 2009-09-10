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

#include "stdafx.h"
#include "DVDSubtitleParser.h"
#include "DVDSubtitleStream.h"
#include "utils/CharsetConverter.h"

bool CDVDSubtitleParserText::Open()
{
  if(m_pStream)
  {
    if(!(m_pStream->Seek(0, SEEK_SET) == 0))
    {
      m_pStream->Close();
      return false;
    }
  }
  else
  {
    m_pStream = new CDVDSubtitleStream();
    if (!(m_pStream->Open(m_filename)))
      return false;
  }
  
  unsigned char buffer[16384];
  int size_read = 0;
  size_read = m_pStream->Read(buffer,3);
  bool isUTF8 = false;
  if (buffer[0] == 0xEF && buffer[1] == 0xBB && buffer[2] == 0xBF)
    isUTF8 = true;
  else
  {
    buffer[size_read] = '\0';
    m_stringstream << buffer;
  }
  while( (size_read = m_pStream->Read(buffer, sizeof(buffer)-1) ) > 0 )
  {
    buffer[size_read] = '\0';
    m_stringstream << buffer;
  }

  if (!isUTF8)
    isUTF8 = g_charsetConverter.isValidUtf8(m_stringstream.str());

  if (!isUTF8)
  {
    CStdStringW strUTF16;
    CStdString strUTF8;
    g_charsetConverter.subtitleCharsetToW(m_stringstream.str(), strUTF16);
    g_charsetConverter.wToUTF8(strUTF16,strUTF8);
    m_stringstream.str("");
    m_stringstream << strUTF8;
  }
  
  return true;
}

CDVDSubtitleParserText::~CDVDSubtitleParserText()
{
  if(m_pStream)
  {
    m_pStream->Close();
    delete m_pStream;
  }
}

