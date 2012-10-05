#pragma once
/*
 *      Copyright (C) 2011-2012 Team XBMC
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

#include "AEAudioFormat.h"
#include "Interfaces/AE.h"
#include "ICoreAudioSource.h"

class ICoreAudioAEHAL;
class CAUOutputDevice;

/**
 * ICoreAudioAEHAL Interface
 */
class ICoreAudioAEHAL
{
protected:
  ICoreAudioAEHAL() {}
  virtual ~ICoreAudioAEHAL() {}

public:
  virtual bool   Initialize(ICoreAudioSource *ae, bool passThrough, AEAudioFormat &format, AEDataFormat rawDataFormat, std::string &device, float initVolume) = 0;
  virtual void   Deinitialize() = 0;
  virtual void   EnumerateOutputDevices(AEDeviceList &devices, bool passthrough) = 0;
  //virtual CAUOutputDevice *DestroyUnit(CAUOutputDevice *outputUnit);
  //virtual CAUOutputDevice *CreateUnit(ICoreAudioSource *pSource, AEAudioFormat &format);
  //virtual void  SetDirectInput(ICoreAudioSource *pSource, AEAudioFormat &format);
  virtual void   Stop() = 0;
  virtual bool   Start() = 0;
  virtual double GetDelay() = 0;
  virtual void   SetVolume(float volume) = 0;
};
