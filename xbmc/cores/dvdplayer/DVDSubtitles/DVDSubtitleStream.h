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

class CDVDInputStream;

// buffered class for subtitle reading

class CDVDSubtitleStream
{
public:
  CDVDSubtitleStream();
  virtual ~CDVDSubtitleStream();
  
  bool Open(const std::string& strFile);
  void Close();
  int Read(BYTE* buf, int buf_size);
  __int64 Seek(__int64 offset, int whence);

  char* ReadLine(char* pBuffer, int iLen);
  //wchar* ReadLineW(wchar* pBuffer, int iLen) { return NULL; };
  
protected:
  CDVDInputStream* m_pInputStream;
  BYTE* m_buffer;
  int   m_iMaxBufferSize;
  int   m_iBufferSize;
  int   m_iBufferPos;
  
  __int64 m_iStreamPos;
};

