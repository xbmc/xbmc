#pragma once

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
#include "DVDAudioCodec.h"
#include "Packetizers/AEPacketizerIEC958.h"

class CDVDAudioCodecPassthrough : public CDVDAudioCodec
{
public:
  CDVDAudioCodecPassthrough();
  virtual ~CDVDAudioCodecPassthrough();

  virtual bool Open(CDVDStreamInfo &hints, CDVDCodecOptions &options);
  virtual void Dispose();
  virtual int  Decode(BYTE* pData, int iSize);
  virtual int  GetData(BYTE** dst);
  virtual void Reset();
  virtual int  GetChannels() { return 2; }
  virtual AEChLayout GetChannelMap() { static enum AEChannel map[3] = {AE_CH_FL, AE_CH_FR, AE_CH_NULL}; return map; }
  virtual int  GetSampleRate   () { return m_packetizer.GetSampleRate(); }
  virtual int  GetBitsPerSample() { return 16;            }
  virtual bool NeedPassthrough () { return true;          }
  virtual const char* GetName  () { return "passthrough"; }
private:
  CAEPacketizerIEC958 m_packetizer;
};

