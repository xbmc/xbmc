/*
 *      Copyright (C) 2009 phi2039
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
  bool Initialize(unsigned int maxData);
  unsigned int Write(void* pData, size_t len);
  void* GetData(unsigned int* pBytesRead);
  unsigned int GetLen();
  unsigned int GetMaxLen();
  unsigned int GetSpace();
  unsigned int ShiftUp(unsigned int bytesToShift);
  void Empty();
  void* Lock(size_t len);
  void Unlock(size_t bytesWritten);
private:
  BYTE* m_pBuffer;
  unsigned int m_BufferSize;
  unsigned int m_BufferOffset;
  bool m_Locked;
};

#endif // __SIMPLE_BUFFER_H__