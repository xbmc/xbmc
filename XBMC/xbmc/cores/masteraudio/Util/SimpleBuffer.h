/*
 *      Copyright (C) 2009 Team XBMC
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

#ifndef __SIMPLE_BUFFER_H__
#define __SIMPLE_BUFFER_H__

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class CSimpleBuffer
{
public:
  CSimpleBuffer();
  virtual ~CSimpleBuffer();
  bool Initialize(size_t size);
  size_t Write(void* pData, size_t len);
  void* GetData(size_t* pBytesRead);
  size_t GetLen();
  size_t GetMaxLen();
  size_t GetSpace();
  size_t ShiftUp(size_t bytesToShift);
  void Empty();
  void* Lock(size_t len);
  void Unlock(size_t bytesWritten);
private:
  BYTE* m_pBuffer;
  size_t m_BufferSize;
  size_t m_BufferOffset;
  bool m_Locked;
};

#endif // __SIMPLE_BUFFER_H__