/* 
 *  Copyright (C) 2003-2006 Gabest
 *  http://www.gabest.org
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
 *  along with GNU Make; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA. 
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "stdafx.h"
#include "Stream.h"
#include <stdio.h>

namespace ssf
{
  Stream::Stream()
    : m_encoding(none)
    , m_line(0)
    , m_col(-1)
  {
    
  }

  Stream::~Stream()
  {
  }

  void Stream::ThrowError(LPCTSTR fmt, ...)
  {
    va_list args;
    va_start(args, fmt);
    int len = _vsctprintf(fmt, args) + 1;
    CStdString str;
    if(len > 0) _vstprintf_s(str.GetBufferSetLength(len), len, fmt, args);
    va_end(args);

    throw Exception(_T("Error (Ln %d Col %d): %s"), m_line+1, m_col+1, str);
  }

  bool Stream::IsWhiteSpace(int c, LPCWSTR morechars)
  {
    return c != 0xa0 && iswspace(c) || morechars && wcschr(morechars, (WCHAR)c);
  }

  //

  InputStream::InputStream()
  {
    
  }

  InputStream::~InputStream()
  {
  }

  int InputStream::NextChar()
  {
    if(m_encoding == none)
    {
      m_encoding = unknown;

      switch(NextByte())
      {
      case 0xef: 
        if(NextByte() == 0xbb && NextByte() == 0xbf) m_encoding = utf8;
        break;
      case 0xff: 
        if(NextByte() == 0xfe) m_encoding = utf16le;
        break;
      case 0xfe:
        if(NextByte() == 0xff) m_encoding = utf16be;
        break;
      }
    }

    if(m_encoding == unknown)
    {
      throw Exception(_T("unknown character encoding, missing BOM"));
    }

    int i, c;

    int cur = NextByte();

    switch(m_encoding)
    {
    case utf8: 
      for(i = 7; i >= 0 && (cur & (1 << i)); i--);
      cur &= (1 << i) - 1;
      while(++i < 7) {c = NextByte(); if(c == EOS) {cur = EOS; break;} cur = (cur << 6) | (c & 0x3f);}
      break;
    case utf16le: 
      c = NextByte();
      if(c == EOS) {cur = EOS; break;}
      cur = (c << 8) | cur;
      break;
    case utf16be: 
      c = NextByte();
      if(c == EOS) {cur = EOS; break;}
      cur = cur | (c << 8);
      break;
    case wchar:
      break;
    }

    return cur;
  }

  int InputStream::PushChar()
  {
    int c = NextChar();
    m_queue.push_back(c);
    return c;
  }

  int InputStream::PopChar()
  {
    if(m_queue.empty()) ThrowError(_T("fatal stream error"));

    int c = m_queue.front(); m_queue.pop_front();

    if(c != EOS)
    {
      if(c == '\n') {m_line++; m_col = -1;}
      m_col++;
    }

    return c;
  }

  int InputStream::PeekChar()
  {
    while(m_queue.size() < 2) PushChar();

    ASSERT(m_queue.size() == 2);

    if(m_queue.front() == '/' && m_queue.back() == '/')
    {
      while(!m_queue.empty()) PopChar();
      int c;
      do {PushChar(); c = PopChar();} while(!(c == '\n' || c == EOS));
      return PeekChar();
    }
    else if(m_queue.front() == '/' && m_queue.back() == '*')
    {
      while(!m_queue.empty()) PopChar();
      int c1, c2;
      PushChar();
      do {c2 = PushChar(); c1 = PopChar();} while(!((c1 == '*' && c2 == '/') || c1 == EOS));
      PopChar();
      return PeekChar();
    }

    return m_queue.front();
  }

  int InputStream::GetChar()
  {
    if(m_queue.size() < 2) PeekChar();
    return PopChar();
  }

  int InputStream::SkipWhiteSpace(LPCWSTR morechars)
  {
    int c = PeekChar();
    for(; IsWhiteSpace(c, morechars); c = PeekChar()) 
      GetChar();
    return c;
  }

  // FileInputStream

  FileInputStream::FileInputStream(const TCHAR* fn) 
    : m_file(NULL)
  {
    if(_tfopen_s(&m_file, fn, _T("r")) != 0) ThrowError(_T("cannot open file '%s'"), fn);
  }

  FileInputStream::~FileInputStream()
  {
    if(m_file) {fclose(m_file); m_file = NULL;}
  }

  int FileInputStream::NextByte()
  {
    if(!m_file) ThrowError(_T("file pointer is NULL"));
    return fgetc(m_file);
  }

  // MemoryInputStream

  MemoryInputStream::MemoryInputStream(BYTE* pBytes, int len, bool fCopy, bool fFree)
    : m_pBytes(NULL)
    , m_pos(0)
    , m_len(len)
  {
    if(fCopy)
    {
      m_pBytes = DNew BYTE[len];
      if(m_pBytes) memcpy(m_pBytes, pBytes, len);
      m_fFree = true;
    }
    else
    {
      m_pBytes = pBytes;
      m_fFree = fFree;
    }

    if(!m_pBytes) ThrowError(_T("memory stream pointer is NULL"));
  }

  MemoryInputStream::~MemoryInputStream()
  {
    if(m_fFree) delete [] m_pBytes;
    m_pBytes = NULL;
  }

  int MemoryInputStream::NextByte()
  {
    if(!m_pBytes) ThrowError(_T("memory stream pointer is NULL"));
    if(m_pos >= m_len) return Stream::EOS;
    return (int)m_pBytes[m_pos++];
  }

  // WCharInputStream
  
  WCharInputStream::WCharInputStream(CStdStringW str)
    : m_str(str)
    , m_pos(0)
  {
    m_encoding = Stream::wchar; // HACK: it should return real bytes from NextByte (two per wchar_t), but this way it's a lot more simple...
  }

  int WCharInputStream::NextByte()
  {
    if(m_pos >= m_str.GetLength()) return Stream::EOS;
    return m_str[m_pos++];
  }

  // OutputStream

  OutputStream::OutputStream(encoding_t e)
  {
    m_encoding = e;
    m_bof = true;
  }

  OutputStream::~OutputStream()
  {
  }

  void OutputStream::PutChar(WCHAR c)
  {
    if(m_bof)
    {
      m_bof = false;

      switch(m_encoding)
      {
      case utf8:
      case utf16le: 
      case utf16be:
        PutChar(0xfeff);
        break;
      }
    }

    switch(m_encoding)
    {
    case utf8: 
      if(0 <= c && c < 0x80) // 0xxxxxxx
      {
        NextByte(c);
      }
      else if(0x80 <= c && c < 0x800) // 110xxxxx 10xxxxxx
      {
        NextByte(0xc0 | ((c<<2)&0x1f));
        NextByte(0x80 | ((c<<0)&0x3f));
      }
      else if(0x800 <= c && c < 0xFFFF) // 1110xxxx 10xxxxxx 10xxxxxx
      {
        NextByte(0xe0 | ((c<<4)&0x0f));
        NextByte(0x80 | ((c<<2)&0x3f));
        NextByte(0x80 | ((c<<0)&0x3f));
      }
      else
      {
        NextByte('?');
      }
      break;
    case utf16le:
      NextByte(c & 0xff);
      NextByte((c >> 8) & 0xff);
      break;
    case utf16be: 
      NextByte((c >> 8) & 0xff);
      NextByte(c & 0xff);
      break;
    case wchar:
      NextByte(c);
      break;
    }
  }

  void OutputStream::PutString(LPCWSTR fmt, ...)
  {
    CStdStringW str;

    va_list args;
    va_start(args, fmt);
    int len = _vscwprintf(fmt, args) + 1;
    if(len > 0) vswprintf_s(str.GetBufferSetLength(len), len, fmt, args);
    va_end(args);

    LPCWSTR s = str;
    while(*s) PutChar(*s++);
  }

  // WCharOutputStream

  WCharOutputStream::WCharOutputStream()
    : OutputStream(wchar)
  {
  }

  void WCharOutputStream::NextByte(int b)
  {
    m_str += (WCHAR)b;
  }

  // DebugOutputStream

  DebugOutputStream::DebugOutputStream()
    : OutputStream(wchar)
  {
  }

  DebugOutputStream::~DebugOutputStream()
  {
    //TRACE(_T("%s\n"), m_str);
  }

  void DebugOutputStream::NextByte(int b)
  {
    if(b == '\n') {m_str.Empty();}
    else if(b != '\r') m_str += (WCHAR)b;
  }
}