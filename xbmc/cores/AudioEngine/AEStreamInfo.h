#pragma once
/*
 *      Copyright (C) 2005-2010 Team XBMC
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

#include "AEPackIEC61937.h"
#include <stdint.h>
#include <list>

/* ffmpeg re-defines this, so undef it to squash the warning */
#undef restrict
#include "DllAvCodec.h"
#include "DllAvFormat.h"

class CAEStreamInfo
{
public:
  enum DataType
  {
    STREAM_TYPE_NULL,
    STREAM_TYPE_AC3,
    STREAM_TYPE_DTS_512,
    STREAM_TYPE_DTS_1024,
    STREAM_TYPE_DTS_2048,
    STREAM_TYPE_DTSHD,
    STREAM_TYPE_EAC3,
    STREAM_TYPE_MLP,
    STREAM_TYPE_TRUEHD
  };

  CAEStreamInfo();
  ~CAEStreamInfo();

  int AddData(uint8_t *data, unsigned int size, uint8_t **buffer = NULL, unsigned int *bufferSize = 0);

  unsigned int            IsValid       () { return m_hasSync   ; }
  unsigned int            GetSampleRate () { return m_sampleRate; }
  unsigned int            GetFrameSize  () { return m_fsize     ; }
  enum DataType           GetDataType   () { return m_dataType  ; }
  bool                    IsLittleEndian() { return m_dataIsLE  ; }
  CAEPackIEC61937::PackFunc GetPackFunc   () { return m_packFunc  ; }
private:
  DllAvUtil m_dllAvUtil;

  uint8_t      m_buffer[MAX_IEC61937_PACKET];
  unsigned int m_bufferSize;
  unsigned int m_skipBytes;

  typedef unsigned int (CAEStreamInfo::*ParseFunc)(uint8_t *data, unsigned int size);

  unsigned int              m_needBytes;
  ParseFunc                 m_syncFunc;
  bool                      m_hasSync;
  unsigned int              m_sampleRate;
  unsigned int              m_fsize;
  unsigned int              m_repeat;
  int                       m_substreams;       /* used for TrueHD  */
  AVCRC                     m_crcTrueHD[1024];  /* TrueHD crc table */
  DataType                  m_dataType;
  bool                      m_dataIsLE;
  CAEPackIEC61937::PackFunc m_packFunc;

  void GetPacket(uint8_t **buffer, unsigned int *bufferSize);
  unsigned int DetectType(uint8_t *data, unsigned int size);
  unsigned int SyncAC3   (uint8_t *data, unsigned int size);
  unsigned int SyncDTS   (uint8_t *data, unsigned int size);
  unsigned int SyncTrueHD(uint8_t *data, unsigned int size);
};

