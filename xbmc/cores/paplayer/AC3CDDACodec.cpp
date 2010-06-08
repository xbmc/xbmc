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

#include "system.h"
#include "AC3CDDACodec.h"
#ifdef HAS_AC3_CDDA_CODEC
#ifdef WIN32
#include <cdio/sector.h>
#else
#include "lib/libcdio/sector.h"
#endif

AC3CDDACodec::AC3CDDACodec() : AC3Codec()
{
  m_CodecName = "AC3 CDDA";
}

AC3CDDACodec::~AC3CDDACodec()
{
}

__int64 AC3CDDACodec::Seek(__int64 iSeekTime)
{
  //  Calculate the next full second...
  int iSeekTimeFullSec = (int)(iSeekTime + (1000 - (iSeekTime % 1000))) / 1000;

  //  ...and the logical sector on the cd...
  lsn_t lsnSeek = iSeekTimeFullSec * CDIO_CD_FRAMES_PER_SEC;

  //  ... then seek to its position...
  int iNewOffset = (int)m_file.Seek(lsnSeek * CDIO_CD_FRAMESIZE_RAW, SEEK_SET);
  m_readBufferPos = 0;

  // ... and look if we really got there.
  int iNewSeekTime = (iNewOffset / CDIO_CD_FRAMESIZE_RAW) / CDIO_CD_FRAMES_PER_SEC;
  return iNewSeekTime * 1000; // ms
}

bool AC3CDDACodec::CalculateTotalTime()
{
  m_TotalTime  = (m_file.GetLength()/CDIO_CD_FRAMESIZE_RAW)/CDIO_CD_FRAMES_PER_SEC;
  m_Bitrate    = (int)((m_file.GetLength() * 8) / m_TotalTime);
  m_TotalTime *= 1000; // ms
  return m_TotalTime > 0;
}
#endif

