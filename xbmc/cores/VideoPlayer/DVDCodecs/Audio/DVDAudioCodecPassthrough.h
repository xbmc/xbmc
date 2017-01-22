#pragma once

/*
 *      Copyright (C) 2010-2013 Team XBMC
 *      http://xbmc.org
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

#include <list>
#include <memory>

#include "system.h"
#include "DVDAudioCodec.h"
#include "cores/AudioEngine/Utils/AEAudioFormat.h"
#include "cores/AudioEngine/Utils/AEStreamInfo.h"
#include "cores/AudioEngine/Utils/AEBitstreamPacker.h"

class CProcessInfo;

class CDVDAudioCodecPassthrough : public CDVDAudioCodec
{
public:
  CDVDAudioCodecPassthrough(CProcessInfo &processInfo);
  virtual ~CDVDAudioCodecPassthrough();

  virtual bool Open(CDVDStreamInfo &hints, CDVDCodecOptions &options) override;
  virtual void Dispose() override;
  virtual int AddData(const DemuxPacket &packet) override;
  virtual void GetData(DVDAudioFrame &frame) override;
  virtual int GetData(uint8_t** dst) override;
  virtual void Reset() override;
  virtual AEAudioFormat GetFormat() override{ return m_format; }
  virtual bool NeedPassthrough() override { return true; }
  virtual const char* GetName() override { return "passthrough"; }
  virtual int GetBufferSize() override;

private:
  CAEStreamParser m_parser;
  uint8_t* m_buffer;
  unsigned int m_bufferSize;
  unsigned int m_dataSize;
  AEAudioFormat m_format;
  uint8_t m_backlogBuffer[61440];
  unsigned int m_backlogSize;
  double m_currentPts;
  double m_nextPts;

  // TrueHD specifics
  std::unique_ptr<uint8_t[]> m_trueHDBuffer;
  unsigned int m_trueHDoffset;
};

