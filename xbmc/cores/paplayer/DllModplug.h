#pragma once

/*
 *      Copyright (C) 2009 Team XBMC
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

#if (defined HAVE_CONFIG_H) && (!defined WIN32)
  #include "config.h"
#endif
#include "DynamicDll.h"
#include "utils/log.h"

#include "lib/libmodplug/src/modplug.h"

class DllModplugInterface
{
public:
  virtual ~DllModplugInterface() {}
  virtual ModPlugFile* ModPlug_Load(const void* data, int size)=0;
  virtual void ModPlug_Unload(ModPlugFile* file)=0;
  virtual int  ModPlug_Read(ModPlugFile* file, void* buffer, int size)=0;
  virtual const char* ModPlug_GetName(ModPlugFile* file)=0;
  virtual int ModPlug_GetLength(ModPlugFile* file)=0;
  virtual void ModPlug_Seek(ModPlugFile* file, int millisecond)=0;
  virtual void ModPlug_GetSettings(ModPlug_Settings* settings)=0;
  virtual void ModPlug_SetSettings(const ModPlug_Settings* settings)=0;
  virtual unsigned int ModPlug_GetMasterVolume(ModPlugFile* file)=0;
  virtual void ModPlug_SetMasterVolume(ModPlugFile* file,unsigned int cvol)=0;
  virtual int ModPlug_GetCurrentSpeed(ModPlugFile* file)=0;
  virtual int ModPlug_GetCurrentTempo(ModPlugFile* file)=0;
  virtual int ModPlug_GetCurrentOrder(ModPlugFile* file)=0;
  virtual int ModPlug_GetCurrentPattern(ModPlugFile* file)=0;
  virtual int ModPlug_GetCurrentRow(ModPlugFile* file)=0;
  virtual int ModPlug_GetPlayingChannels(ModPlugFile* file)=0;
  virtual void ModPlug_SeekOrder(ModPlugFile* file,int order)=0;
  virtual int ModPlug_GetModuleType(ModPlugFile* file)=0;
  virtual char* ModPlug_GetMessage(ModPlugFile* file)=0;
  virtual unsigned int ModPlug_NumInstruments(ModPlugFile* file)=0;
  virtual unsigned int ModPlug_NumSamples(ModPlugFile* file)=0;
  virtual unsigned int ModPlug_NumPatterns(ModPlugFile* file)=0;
  virtual unsigned int ModPlug_NumChannels(ModPlugFile* file)=0;
  virtual unsigned int ModPlug_SampleName(ModPlugFile* file, unsigned int qual, char* buff)=0;
  virtual unsigned int ModPlug_InstrumentName(ModPlugFile* file, unsigned int qual, char* buff)=0;
  virtual ModPlugNote* ModPlug_GetPattern(ModPlugFile* file, int pattern, unsigned int* numrows)=0;
  virtual void ModPlug_InitMixerCallback(ModPlugFile* file,ModPlugMixerProc proc)=0;
  virtual void ModPlug_UnloadMixerCallback(ModPlugFile* file)=0;
};

class DllModplug : public DllDynamic, DllModplugInterface
{
public:
  virtual ~DllModplug() {}
  virtual ModPlugFile* ModPlug_Load(const void* data, int size)
    { return ::ModPlug_Load(data, size); }
  virtual void ModPlug_Unload(ModPlugFile* file)
    { return ::ModPlug_Unload(file); }
  virtual int  ModPlug_Read(ModPlugFile* file, void* buffer, int size)
    { return ::ModPlug_Read(file, buffer, size); }
  virtual const char* ModPlug_GetName(ModPlugFile* file)
    { return ::ModPlug_GetName(file); }
  virtual int ModPlug_GetLength(ModPlugFile* file)
    { return ::ModPlug_GetLength(file); }
  virtual void ModPlug_Seek(ModPlugFile* file, int millisecond)
    { return ::ModPlug_Seek(file, millisecond); }
  virtual void ModPlug_GetSettings(ModPlug_Settings* settings)
    { return ::ModPlug_GetSettings(settings); }
  virtual void ModPlug_SetSettings(const ModPlug_Settings* settings)
    { return ::ModPlug_SetSettings(settings); }
  virtual unsigned int ModPlug_GetMasterVolume(ModPlugFile* file)
    { return ::ModPlug_GetMasterVolume(file); }
  virtual void ModPlug_SetMasterVolume(ModPlugFile* file,unsigned int cvol)
    { return ::ModPlug_SetMasterVolume(file, cvol); }
  virtual int ModPlug_GetCurrentSpeed(ModPlugFile* file)
    { return ::ModPlug_GetCurrentSpeed(file); }
  virtual int ModPlug_GetCurrentTempo(ModPlugFile* file)
    { return ::ModPlug_GetCurrentTempo(file); }
  virtual int ModPlug_GetCurrentOrder(ModPlugFile* file)
    { return ::ModPlug_GetCurrentOrder(file); }
  virtual int ModPlug_GetCurrentPattern(ModPlugFile* file)
    { return ::ModPlug_GetCurrentPattern(file); }
  virtual int ModPlug_GetCurrentRow(ModPlugFile* file)
    { return ::ModPlug_GetCurrentRow(file); }
  virtual int ModPlug_GetPlayingChannels(ModPlugFile* file)
    { return ::ModPlug_GetPlayingChannels(file); }
  virtual void ModPlug_SeekOrder(ModPlugFile* file,int order)
    { return ::ModPlug_SeekOrder(file, order); }
  virtual int ModPlug_GetModuleType(ModPlugFile* file)
    { return ::ModPlug_GetModuleType(file); }
  virtual char* ModPlug_GetMessage(ModPlugFile* file)
    { return ::ModPlug_GetMessage(file); }
  virtual unsigned int ModPlug_NumInstruments(ModPlugFile* file)
    { return ::ModPlug_NumInstruments(file); }
  virtual unsigned int ModPlug_NumSamples(ModPlugFile* file)
    { return ::ModPlug_NumSamples(file); }
  virtual unsigned int ModPlug_NumPatterns(ModPlugFile* file)
    { return ::ModPlug_NumPatterns(file); }
  virtual unsigned int ModPlug_NumChannels(ModPlugFile* file)
    { return ::ModPlug_NumChannels(file); }
  virtual unsigned int ModPlug_SampleName(ModPlugFile* file, unsigned int qual, char* buff)
    { return ::ModPlug_SampleName(file, qual, buff); }
  virtual unsigned int ModPlug_InstrumentName(ModPlugFile* file, unsigned int qual, char* buff)
    { return ::ModPlug_InstrumentName(file, qual, buff); }
  virtual ModPlugNote* ModPlug_GetPattern(ModPlugFile* file, int pattern, unsigned int* numrows)
    { return ::ModPlug_GetPattern(file, pattern, numrows); }
  virtual void ModPlug_InitMixerCallback(ModPlugFile* file,ModPlugMixerProc proc)
    { return ::ModPlug_InitMixerCallback(file, proc); }
  virtual void ModPlug_UnloadMixerCallback(ModPlugFile* file)
    { return ::ModPlug_UnloadMixerCallback(file); }

  // DLL faking.
  virtual bool ResolveExports() { return true; }
  virtual bool Load() {
    CLog::Log(LOGDEBUG, "DllModplug: Using libmodplug library");
    return true;
  }
  virtual void Unload() {}
};
