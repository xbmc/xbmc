#ifndef _ENCODERWAV_H
#define _ENCODERWAV_H

/*
 *      Copyright (C) 2005-2012 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "Encoder.h"
#include <stdio.h>

typedef struct
{
  uint8_t riff[4];         /* must be "RIFF"    */
  uint32_t len;             /* #bytes + 44 - 8   */
  uint8_t cWavFmt[8];      /* must be "WAVEfmt " */
  uint32_t dwHdrLen;
  uint16_t wFormat;
  uint16_t wNumChannels;
  uint32_t dwSampleRate;
  uint32_t dwBytesPerSec;
  uint16_t wBlockAlign;
  uint16_t wBitsPerSample;
  uint8_t cData[4];        /* must be "data"   */
  uint32_t dwDataLen;       /* #bytes           */
}
WAVHDR, *PWAVHDR, *LPWAVHDR;

class CEncoderWav : public CEncoder
{
public:
  CEncoderWav();
  virtual ~CEncoderWav() {}
  bool Init(const char* strFile, int iInChannels, int iInRate, int iInBits);
  int Encode(int nNumBytesRead, uint8_t* pbtStream);
  bool Close();
  void AddTag(int key, const char* value);

private:
  bool WriteWavHeader();

  int m_iBytesWritten;
};

#endif // _ENCODERWAV_H
