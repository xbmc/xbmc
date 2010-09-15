#pragma once
/*
 *      Copyright (C) 2005-2010 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include <list>
#include <map>

#include "system.h"
#include "utils/CriticalSection.h"

#include "AEAudioFormat.h"
#include "AEStream.h"
#include "AESound.h"
#include "AEPacketizer.h"

typedef std::pair<CStdString, CStdString> AEDevice;
typedef std::vector<AEDevice> AEDeviceList;

/* forward declarations */
class IAEStream;
class IAESound;
class IAEPacketizer;

class IAE
{
public:
  virtual bool Initialize() = 0;
  virtual void OnSettingsChange(CStdString setting) = 0;

  virtual float GetDelay () = 0;
  virtual float GetVolume() = 0;
  virtual void  SetVolume(float volume) = 0;

  /* returns a new stream for data in the specified format */
  virtual IAEStream *GetStream(enum AEDataFormat dataFormat, unsigned int sampleRate, unsigned int channelCount, AEChLayout channelLayout, unsigned int options = 0) = 0;

  /* returns a new sound object */
  virtual IAESound *GetSound(CStdString file) = 0;
  virtual void FreeSound(IAESound *sound) = 0;
  virtual void PlaySound(IAESound *sound) = 0;
  virtual void StopSound(IAESound *sound) = 0;
  virtual bool IsPlaying(IAESound *sound) = 0;

  /* free's sounds that have expired */
  virtual void GarbageCollect() = 0;

  virtual void EnumerateOutputDevices(AEDeviceList &devices, bool passthrough) = 0;
};

