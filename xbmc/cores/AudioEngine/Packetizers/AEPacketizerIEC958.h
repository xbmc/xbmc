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

#include "AEPacketizer.h"
#include <stdint.h>
#include <list>

/* ffmpeg re-defines this, so undef it to squash the warning */
#undef restrict

#include "Codecs/DllAvCodec.h"
#include "Codecs/DllAvFormat.h"

#define MAX_IEC958_PACKET 6114

class CAEPacketizerIEC958 : public IAEPacketizer
{
public:
  typedef enum
  {
    SPDIF_FMT_INVALID = -1,
    SPDIF_FMT_AC3,
    SPDIF_FMT_DTS,
    SPDIF_FMT_AAC
  } SPDIFFormat;

  virtual const char *GetName() { return "IEC958"; }

  CAEPacketizerIEC958();
  virtual ~CAEPacketizerIEC958();
  virtual bool Initialize();
  virtual void Deinitialize();
  virtual void Reset();

  virtual int AddData(uint8_t *data, unsigned int size);
  virtual int GetData(uint8_t **data);
private:
  DllAvUtil m_dllAvUtil;
  DECLARE_ALIGNED(16, uint8_t, m_packetData[MAX_IEC958_PACKET]);
  unsigned int m_packetSize;
  bool         m_hasPacket;

  typedef unsigned int (CAEPacketizerIEC958::*SPDIFSyncFunc)(uint8_t *data, unsigned int size, unsigned int *fsize);
  typedef void (CAEPacketizerIEC958::*SPDIFPackFunc)(uint8_t *data, unsigned int fsize);

  SPDIFFormat   m_dataType;
  SPDIFSyncFunc m_syncFunc;
  SPDIFPackFunc m_packFunc;

  void PackAC3(uint8_t *data, unsigned int fsize);
  void PackDTS(uint8_t *data, unsigned int fsize);
  void PackAAC(uint8_t *data, unsigned int fsize);

  unsigned int DetectType(uint8_t *data, unsigned int size, unsigned int *fsize);
  unsigned int SyncAC3(uint8_t *data, unsigned int size, unsigned int *fsize);
  unsigned int SyncDTS(uint8_t *data, unsigned int size, unsigned int *fsize);
  unsigned int SyncAAC(uint8_t *data, unsigned int size, unsigned int *fsize);
};

