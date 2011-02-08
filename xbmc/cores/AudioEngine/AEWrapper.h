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

#include "system.h"
#include "threads/SharedSection.h"

#include "AE.h"
#include "AESoundWrapper.h"

/**
 * CAEWrapper class
 * Wraps the AE interface so that it can be changed on the fly
 */
class CAEWrapper : public IAE
{
protected:
  friend class CAEFactory;
  virtual bool Initialize();
  bool SetEngine(IAE *ae);  

public:
  CAEWrapper();
  virtual ~CAEWrapper();


  virtual void OnSettingsChange(CStdString setting);
  virtual float GetVolume();
  virtual void  SetVolume(float volume);
  virtual IAEStream *GetStream(enum AEDataFormat dataFormat, unsigned int sampleRate, unsigned int channelCount, AEChLayout channelLayout, unsigned int options = 0);
  virtual IAEStream *AlterStream(IAEStream *stream, enum AEDataFormat dataFormat, unsigned int sampleRate, unsigned int channelCount, AEChLayout channelLayout, unsigned int options = 0);
  virtual IAESound *GetSound(CStdString file);
  virtual void FreeSound(IAESound *sound);
  virtual void GarbageCollect();
  virtual void EnumerateOutputDevices(AEDeviceList &devices, bool passthrough);
  virtual bool SupportsRaw();

  /* THIS IS FOR INTERNAL ENGINE USE ONLY */
  IAE *GetEngine();

private:
  CSharedSection m_lock;
  IAE *m_ae;

  std::list<CAESoundWrapper*> m_sounds;
};

extern CAEWrapper AE;
