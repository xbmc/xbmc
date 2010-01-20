#ifndef _CCDDARIPPER_H
#define _CCDDARIPPER_H

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

#include "CDDAReader.h"
#include "Encoder.h"

namespace MUSIC_INFO
{
  class CMusicInfoTag;
}

class CCDDARipper
{
public:
  CCDDARipper();
  virtual ~CCDDARipper();

  bool RipTrack(CFileItem* pItem);
  bool RipCD();

private:
  bool Init(const CStdString& strTrackFile, const CStdString& strFile, MUSIC_INFO::CMusicInfoTag* infoTag = NULL);
  bool DeInit();
  int RipChunk(int& nPercent);
  bool Rip(const CStdString& strTrackFile, const CStdString& strFileName, MUSIC_INFO::CMusicInfoTag& infoTag);
  const char* GetExtension(int iEncoder);
  CStdString GetTrackName(CFileItem *item, int LegalType);

  CEncoder* m_pEncoder;
  CCDDAReader m_cdReader;
};

#endif // _CCDDARIPPERMP3_H
