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
#include "utils/EndianSwap.h"
#include <stdint.h>
#include <list>

/* ffmpeg re-defines this, so undef it to squash the warning */
#undef restrict

#include "Codecs/DllAvCodec.h"
#include "Codecs/DllAvFormat.h"

#ifdef __GNUC__
  #define S_PACK __attribute__((__packed__))
  #define E_PACK
#else
  #define S_PACK __pragma(pack(push, 1))
  #define E_PACK __pragma(pack(pop))
#endif

#define MAX_IEC958_PACKET 6144

class CAEPacketizerIEC958 : public IAEPacketizer
{
public:
  enum IEC958DataType
  {
    IEC958_TYPE_NULL   = 0x00,
    IEC958_TYPE_AC3    = 0x01,
    IEC958_TYPE_DTS1   = 0x0B, /*  512 samples */
    IEC958_TYPE_DTS2   = 0x0C, /* 1024 samples */
    IEC958_TYPE_DTS3   = 0x0D, /* 2048 samples */
    IEC958_TYPE_DTSHD  = 0x11,
    IEC958_TYPE_EAC3   = 0x15,
    IEC958_TYPE_TRUEHD = 0x16
  };

  virtual const char  *GetName      () { return "IEC958"; }
  virtual unsigned int GetPacketSize() { return MAX_IEC958_PACKET; }

  CAEPacketizerIEC958();
  virtual ~CAEPacketizerIEC958();
  virtual bool Initialize();
  virtual void Deinitialize();
  virtual void Reset();

  virtual int  AddData  (uint8_t *data, unsigned int size);
  virtual bool HasPacket();
  virtual int  GetPacket(uint8_t **data);
  virtual unsigned int GetSampleRate() { return m_sampleRate; }
  virtual unsigned int GetBufferSize();
private:
  S_PACK
  struct IEC958Packet
  {
    uint32_t m_preamble;
    uint16_t m_type;
    uint16_t m_length;
    uint8_t  m_data[MAX_IEC958_PACKET - 8];
  };
  E_PACK

  DllAvUtil m_dllAvUtil;
  std::list<struct IEC958Packet> m_packetBuffer;
  DECLARE_ALIGNED(16, struct IEC958Packet, m_nullPacket);

  uint8_t             m_buffer[MAX_IEC958_PACKET];
  unsigned int        m_bufferSize;
  struct IEC958Packet m_current;

  typedef unsigned int (CAEPacketizerIEC958::*SPDIFSyncFunc)(uint8_t *data, unsigned int size, unsigned int *fsize);
  typedef void (CAEPacketizerIEC958::*SPDIFPackFunc)(uint8_t *data, unsigned int fsize);

  bool           m_hasSync;
  IEC958DataType m_dataType;
  SPDIFSyncFunc  m_syncFunc;
  SPDIFPackFunc  m_packFunc;
  unsigned int   m_sampleRate;
  bool           m_dataIsLE;
  unsigned int   m_ratio;

  void SwapPacket(struct IEC958Packet &packet, const bool swapData);
  void PackAC3(uint8_t *data, unsigned int fsize);
  void PackDTS(uint8_t *data, unsigned int fsize);

  unsigned int DetectType(uint8_t *data, unsigned int size, unsigned int *fsize);
  unsigned int SyncAC3(uint8_t *data, unsigned int size, unsigned int *fsize);
  unsigned int SyncDTS(uint8_t *data, unsigned int size, unsigned int *fsize);
};

