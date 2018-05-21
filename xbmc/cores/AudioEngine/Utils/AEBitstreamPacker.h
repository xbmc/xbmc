#pragma once
/*
 *      Copyright (C) 2010-present Team Kodi
 *      http://kodi.tv
 *
 *  Kodi is free software: you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Kodi is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Kodi. If not, see <https://www.gnu.org/licenses/>.
 *
 */

#include <stdint.h>
#include <list>
#include "AEPackIEC61937.h"
#include "AEChannelInfo.h"

class CAEStreamInfo;

class CAEBitstreamPacker
{
public:
  CAEBitstreamPacker();
  ~CAEBitstreamPacker();

  void Pack(CAEStreamInfo &info, uint8_t* data, int size);
  bool PackPause(CAEStreamInfo &info, unsigned int millis, bool iecBursts);
  void Reset();
  uint8_t* GetBuffer();
  unsigned int GetSize();
  static unsigned int GetOutputRate(CAEStreamInfo &info);
  static CAEChannelInfo GetOutputChannelMap(CAEStreamInfo &info);

private:
  void PackTrueHD(CAEStreamInfo &info, uint8_t* data, int size);
  void PackDTSHD(CAEStreamInfo &info, uint8_t* data, int size);
  void PackEAC3(CAEStreamInfo &info, uint8_t* data, int size);

  /* we keep the trueHD and dtsHD buffers separate so that we can handle a fast stream switch */
  uint8_t      *m_trueHD;
  unsigned int  m_trueHDPos;

  uint8_t      *m_dtsHD;
  unsigned int  m_dtsHDSize;

  uint8_t      *m_eac3;
  unsigned int  m_eac3Size;
  unsigned int  m_eac3FramesCount;
  unsigned int  m_eac3FramesPerBurst;

  unsigned int  m_dataSize;
  uint8_t       m_packedBuffer[MAX_IEC61937_PACKET];
  unsigned int m_pauseDuration;
};

