#ifndef FLAC_TAG_H_
#define FLAC_TAG_H_

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

//------------------------------
// CFlacTag in 2003 by JMarshall
//------------------------------
#include "VorbisTag.h"

namespace MUSIC_INFO
{

#pragma once

class CFlacTag : public CVorbisTag
{
public:
  CFlacTag(void);
  virtual ~CFlacTag(void);
  virtual bool Read(const CStdString& strFile);

protected:
  XFILE::CFile* m_file;
  void ProcessVorbisComment(const char *pBuffer, size_t bufsize);
  int ReadFlacHeader(void);    // returns the position after the STREAM_INFO metadata
  int FindFlacHeader(void);    // returns the offset in the file of the fLaC data
  unsigned int ReadUnsigned();  // reads a 32 bit unsigned int
};
}

#endif
