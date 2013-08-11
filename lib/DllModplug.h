#pragma once
/*
 *      Copyright (C) 2009-2013 Team XBMC
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

#if (defined HAVE_CONFIG_H) && (!defined WIN32)
  #include "config.h"
#endif
#include "DynamicDll.h"
#include "utils/log.h"

#ifdef __linux__
#include <libmodplug/modplug.h>
#else
#include "lib/libmodplug/src/modplug.h"
#endif

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
/*  These don't exist in libmodplug under Ubuntu 8.04 (Hardy), libmodplug does not have versioning but we don't use them anyway
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
*/
};

class DllModplug : public DllDynamic, DllModplugInterface
{
  DECLARE_DLL_WRAPPER(DllModplug, DLL_PATH_MODPLUG_CODEC)
  DEFINE_METHOD2(ModPlugFile*,  ModPlug_Load,                 (const void* p1, int p2))
  DEFINE_METHOD1(void,          ModPlug_Unload,               (ModPlugFile* p1))
  DEFINE_METHOD3(int,           ModPlug_Read,                 (ModPlugFile* p1, void* p2, int p3))
  DEFINE_METHOD1(const char*,   ModPlug_GetName,              (ModPlugFile* p1))
  DEFINE_METHOD1(int,           ModPlug_GetLength,            (ModPlugFile* p1))
  DEFINE_METHOD2(void,          ModPlug_Seek,                 (ModPlugFile* p1, int p2))
  DEFINE_METHOD1(void,          ModPlug_GetSettings,          (ModPlug_Settings* p1))
  DEFINE_METHOD1(void,          ModPlug_SetSettings,          (const ModPlug_Settings* p1))
/*
  DEFINE_METHOD1(unsigned int,  ModPlug_GetMasterVolume,      (ModPlugFile* p1))
  DEFINE_METHOD2(void,          ModPlug_SetMasterVolume,      (ModPlugFile* p1, unsigned int p2))
  DEFINE_METHOD1(int,           ModPlug_GetCurrentSpeed,      (ModPlugFile* p1))
  DEFINE_METHOD1(int,           ModPlug_GetCurrentTempo,      (ModPlugFile* p1))
  DEFINE_METHOD1(int,           ModPlug_GetCurrentOrder,      (ModPlugFile* p1))
  DEFINE_METHOD1(int,           ModPlug_GetCurrentPattern,    (ModPlugFile* p1))
  DEFINE_METHOD1(int,           ModPlug_GetCurrentRow,        (ModPlugFile* p1))
  DEFINE_METHOD1(int,           ModPlug_GetPlayingChannels,   (ModPlugFile* p1))
  DEFINE_METHOD2(void,          ModPlug_SeekOrder,            (ModPlugFile* p1, int p2))
  DEFINE_METHOD1(int,           ModPlug_GetModuleType,        (ModPlugFile* p1))
  DEFINE_METHOD1(char*,         ModPlug_GetMessage,           (ModPlugFile* p1))
  DEFINE_METHOD1(unsigned int,  ModPlug_NumInstruments,       (ModPlugFile* p1))
  DEFINE_METHOD1(unsigned int,  ModPlug_NumSamples,           (ModPlugFile* p1))
  DEFINE_METHOD1(unsigned int,  ModPlug_NumPatterns,          (ModPlugFile* p1))
  DEFINE_METHOD1(unsigned int,  ModPlug_NumChannels,          (ModPlugFile* p1))
  DEFINE_METHOD3(unsigned int,  ModPlug_SampleName,           (ModPlugFile* p1, unsigned int p2, char* p3))
  DEFINE_METHOD3(unsigned int,  ModPlug_InstrumentName,       (ModPlugFile* p1, unsigned int p2, char* p3))
*/
  BEGIN_METHOD_RESOLVE()
    RESOLVE_METHOD(ModPlug_Load)
    RESOLVE_METHOD(ModPlug_Unload)
    RESOLVE_METHOD(ModPlug_Read)
    RESOLVE_METHOD(ModPlug_GetName)
    RESOLVE_METHOD(ModPlug_GetLength)
    RESOLVE_METHOD(ModPlug_Seek)
    RESOLVE_METHOD(ModPlug_GetSettings)
    RESOLVE_METHOD(ModPlug_SetSettings)
/*
    RESOLVE_METHOD(ModPlug_GetMasterVolume)
    RESOLVE_METHOD(ModPlug_SetMasterVolume)
    RESOLVE_METHOD(ModPlug_GetCurrentSpeed)
    RESOLVE_METHOD(ModPlug_GetCurrentTempo)
    RESOLVE_METHOD(ModPlug_GetCurrentOrder)
    RESOLVE_METHOD(ModPlug_GetCurrentPattern)
    RESOLVE_METHOD(ModPlug_GetCurrentRow)
    RESOLVE_METHOD(ModPlug_GetPlayingChannels)
    RESOLVE_METHOD(ModPlug_SeekOrder)
    RESOLVE_METHOD(ModPlug_GetModuleType)
    RESOLVE_METHOD(ModPlug_GetMessage)
    RESOLVE_METHOD(ModPlug_NumInstruments)
    RESOLVE_METHOD(ModPlug_NumSamples)
    RESOLVE_METHOD(ModPlug_NumPatterns)
    RESOLVE_METHOD(ModPlug_NumChannels)
    RESOLVE_METHOD(ModPlug_SampleName)
    RESOLVE_METHOD(ModPlug_InstrumentName)
*/
  END_METHOD_RESOLVE()
};

