/*
 *      Copyright (C) 2013-2014 Team KODI
 *      http://kodi.tv
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
 *  along with KODI; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include "addons/kodi-addon-dev-kit/include/kodi/libKODI_audioengine.h"
#include "addons/binary/interfaces/api1/AudioEngine/AddonCallbacksAudioEngine.h"

#ifdef _WIN32
#include <windows.h>
#define DLLEXPORT __declspec(dllexport)
#else
#define DLLEXPORT
#endif

using namespace std;
using namespace V1::KodiAPI::AudioEngine;

#define LIBRARY_NAME "libKODI_audioengine"

extern "C"
{

DLLEXPORT void* AudioEngine_register_me(void *hdl)
{
  CB_AudioEngineLib *cb = NULL;
  if (!hdl)
    fprintf(stderr, "%s-ERROR: AudioEngine_register_me is called with NULL handle !!!\n", LIBRARY_NAME);
  else
  {
    cb = (CB_AudioEngineLib*)((AddonCB*)hdl)->AudioEngineLib_RegisterMe(((AddonCB*)hdl)->addonData);
    if (!cb)
      fprintf(stderr, "%s-ERROR: AudioEngine_register_me can't get callback table from KODI !!!\n", LIBRARY_NAME);
  }
  return cb;
}

DLLEXPORT void AudioEngine_unregister_me(void *hdl, void* cb)
{
  if (hdl && cb)
    ((AddonCB*)hdl)->AudioEngineLib_UnRegisterMe(((AddonCB*)hdl)->addonData, (CB_AudioEngineLib*)cb);
}

// ---------------------------------------------
// CAddonAEStream implementations
// ---------------------------------------------
DLLEXPORT CAddonAEStream* AudioEngine_make_stream(void *hdl, void *cb, AudioEngineFormat& Format, unsigned int Options)
{
  if (!hdl || !cb)
  {
    fprintf(stderr, "%s-ERROR: AudioEngine_register_me is called with NULL handle !!!\n", LIBRARY_NAME);
    return NULL;
  }

  AEStreamHandle *streamHandle = ((CB_AudioEngineLib*)cb)->MakeStream(Format, Options);
  if (!streamHandle)
  {
    fprintf(stderr, "%s-ERROR: AudioEngine_make_stream MakeStream failed!\n", LIBRARY_NAME);
    return NULL;
  }

  return new CAddonAEStream(hdl, cb, streamHandle);
}

DLLEXPORT void AudioEngine_free_stream(CAddonAEStream *p)
{
  if (p)
  {
    delete p;
  }
}

DLLEXPORT bool AudioEngine_get_current_sink_Format(void *hdl, void *cb, AudioEngineFormat *SinkFormat)
{
  if (!cb)
  {
    return false;
  }
    
  return ((CB_AudioEngineLib*)cb)->GetCurrentSinkFormat(((AddonCB*)hdl)->addonData, SinkFormat);
}

CAddonAEStream::CAddonAEStream(void *Addon, void *Callbacks, AEStreamHandle *StreamHandle)
{
  m_AddonHandle = Addon;
  m_Callbacks = Callbacks;
  m_StreamHandle = StreamHandle;
}

CAddonAEStream::~CAddonAEStream()
{
  if (m_StreamHandle)
  {
    ((CB_AudioEngineLib*)m_Callbacks)->FreeStream(m_StreamHandle);
    m_StreamHandle = NULL;
  }
}

unsigned int CAddonAEStream::GetSpace()
{
  return ((CB_AudioEngineLib*)m_Callbacks)->AEStream_GetSpace(((AddonCB*)m_AddonHandle)->addonData, m_StreamHandle);
}

unsigned int CAddonAEStream::AddData(uint8_t* const *Data, unsigned int Offset, unsigned int Frames)
{
  return ((CB_AudioEngineLib*)m_Callbacks)->AEStream_AddData(((AddonCB*)m_AddonHandle)->addonData, m_StreamHandle, Data, Offset, Frames);
}

double CAddonAEStream::GetDelay()
{
  return ((CB_AudioEngineLib*)m_Callbacks)->AEStream_GetDelay(((AddonCB*)m_AddonHandle)->addonData, m_StreamHandle);
}

bool CAddonAEStream::IsBuffering()
{
  return ((CB_AudioEngineLib*)m_Callbacks)->AEStream_IsBuffering(((AddonCB*)m_AddonHandle)->addonData, m_StreamHandle);
}

double CAddonAEStream::GetCacheTime()
{
  return ((CB_AudioEngineLib*)m_Callbacks)->AEStream_GetCacheTime(((AddonCB*)m_AddonHandle)->addonData, m_StreamHandle);
}

double CAddonAEStream::GetCacheTotal()
{
  return ((CB_AudioEngineLib*)m_Callbacks)->AEStream_GetCacheTotal(((AddonCB*)m_AddonHandle)->addonData, m_StreamHandle);
}

void CAddonAEStream::Pause()
{
  return ((CB_AudioEngineLib*)m_Callbacks)->AEStream_Pause(((AddonCB*)m_AddonHandle)->addonData, m_StreamHandle);
}

void CAddonAEStream::Resume()
{
  return ((CB_AudioEngineLib*)m_Callbacks)->AEStream_Resume(((AddonCB*)m_AddonHandle)->addonData, m_StreamHandle);
}

void CAddonAEStream::Drain(bool Wait)
{
  return ((CB_AudioEngineLib*)m_Callbacks)->AEStream_Drain(((AddonCB*)m_AddonHandle)->addonData, m_StreamHandle, Wait);
}

bool CAddonAEStream::IsDraining()
{
  return ((CB_AudioEngineLib*)m_Callbacks)->AEStream_IsDraining(((AddonCB*)m_AddonHandle)->addonData, m_StreamHandle);
}

bool CAddonAEStream::IsDrained()
{
  return ((CB_AudioEngineLib*)m_Callbacks)->AEStream_IsDrained(((AddonCB*)m_AddonHandle)->addonData, m_StreamHandle);
}

void CAddonAEStream::Flush()
{
  return ((CB_AudioEngineLib*)m_Callbacks)->AEStream_Flush(((AddonCB*)m_AddonHandle)->addonData, m_StreamHandle);
}

float CAddonAEStream::GetVolume()
{
  return ((CB_AudioEngineLib*)m_Callbacks)->AEStream_GetVolume(((AddonCB*)m_AddonHandle)->addonData, m_StreamHandle);
}

void CAddonAEStream::SetVolume(float Volume)
{
  return ((CB_AudioEngineLib*)m_Callbacks)->AEStream_SetVolume(((AddonCB*)m_AddonHandle)->addonData, m_StreamHandle, Volume);
}

float CAddonAEStream::GetAmplification()
{
  return ((CB_AudioEngineLib*)m_Callbacks)->AEStream_GetAmplification(((AddonCB*)m_AddonHandle)->addonData, m_StreamHandle);
}

void CAddonAEStream::SetAmplification(float Amplify)
{
  return ((CB_AudioEngineLib*)m_Callbacks)->AEStream_SetAmplification(((AddonCB*)m_AddonHandle)->addonData, m_StreamHandle, Amplify);
}

const unsigned int CAddonAEStream::GetFrameSize() const
{
  return ((CB_AudioEngineLib*)m_Callbacks)->AEStream_GetFrameSize(((AddonCB*)m_AddonHandle)->addonData, m_StreamHandle);
}

const unsigned int CAddonAEStream::GetChannelCount() const
{
  return ((CB_AudioEngineLib*)m_Callbacks)->AEStream_GetChannelCount(((AddonCB*)m_AddonHandle)->addonData, m_StreamHandle);
}

const unsigned int CAddonAEStream::GetSampleRate() const
{
  return ((CB_AudioEngineLib*)m_Callbacks)->AEStream_GetSampleRate(((AddonCB*)m_AddonHandle)->addonData, m_StreamHandle);
}

const AEDataFormat CAddonAEStream::GetDataFormat() const
{
  return ((CB_AudioEngineLib*)m_Callbacks)->AEStream_GetDataFormat(((AddonCB*)m_AddonHandle)->addonData, m_StreamHandle);
}

double CAddonAEStream::GetResampleRatio()
{
  return ((CB_AudioEngineLib*)m_Callbacks)->AEStream_GetResampleRatio(((AddonCB*)m_AddonHandle)->addonData, m_StreamHandle);
}

void CAddonAEStream::SetResampleRatio(double Ratio)
{
  ((CB_AudioEngineLib*)m_Callbacks)->AEStream_SetResampleRatio(((AddonCB*)m_AddonHandle)->addonData, m_StreamHandle, Ratio);
}

};
