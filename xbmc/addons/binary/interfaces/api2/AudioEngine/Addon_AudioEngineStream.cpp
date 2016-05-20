/*
 *      Copyright (C) 2015-2016 Team KODI
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

#include "Addon_AudioEngineStream.h"

#include "addons/binary/ExceptionHandling.h"
#include "addons/binary/interfaces/api2/AddonInterfaceBase.h"
#include "addons/kodi-addon-dev-kit/include/kodi/api2/.internal/AddonLib_internal.hpp"
#include "cores/AudioEngine/AEFactory.h"
#include "cores/AudioEngine/Interfaces/AEStream.h"
#include "cores/AudioEngine/Utils/AEChannelData.h"
#include "utils/log.h"

using namespace ADDON;

namespace V2
{
namespace KodiAPI
{

namespace AudioEngine
{
extern "C"
{

void CAddOnAEStream::Init(struct CB_AddOnLib *interfaces)
{
  interfaces->AudioEngineStream.AEStream_GetSpace              = V2::KodiAPI::AudioEngine::CAddOnAEStream::AEStream_GetSpace;
  interfaces->AudioEngineStream.AEStream_AddData               = V2::KodiAPI::AudioEngine::CAddOnAEStream::AEStream_AddData;
  interfaces->AudioEngineStream.AEStream_GetDelay              = V2::KodiAPI::AudioEngine::CAddOnAEStream::AEStream_GetDelay;
  interfaces->AudioEngineStream.AEStream_IsBuffering           = V2::KodiAPI::AudioEngine::CAddOnAEStream::AEStream_IsBuffering;
  interfaces->AudioEngineStream.AEStream_GetCacheTime          = V2::KodiAPI::AudioEngine::CAddOnAEStream::AEStream_GetCacheTime;
  interfaces->AudioEngineStream.AEStream_GetCacheTotal         = V2::KodiAPI::AudioEngine::CAddOnAEStream::AEStream_GetCacheTotal;
  interfaces->AudioEngineStream.AEStream_Pause                 = V2::KodiAPI::AudioEngine::CAddOnAEStream::AEStream_Pause;
  interfaces->AudioEngineStream.AEStream_Resume                = V2::KodiAPI::AudioEngine::CAddOnAEStream::AEStream_Resume;
  interfaces->AudioEngineStream.AEStream_Drain                 = V2::KodiAPI::AudioEngine::CAddOnAEStream::AEStream_Drain;
  interfaces->AudioEngineStream.AEStream_IsDraining            = V2::KodiAPI::AudioEngine::CAddOnAEStream::AEStream_IsDraining;
  interfaces->AudioEngineStream.AEStream_IsDrained             = V2::KodiAPI::AudioEngine::CAddOnAEStream::AEStream_IsDrained;
  interfaces->AudioEngineStream.AEStream_Flush                 = V2::KodiAPI::AudioEngine::CAddOnAEStream::AEStream_Flush;
  interfaces->AudioEngineStream.AEStream_GetVolume             = V2::KodiAPI::AudioEngine::CAddOnAEStream::AEStream_GetVolume;
  interfaces->AudioEngineStream.AEStream_SetVolume             = V2::KodiAPI::AudioEngine::CAddOnAEStream::AEStream_SetVolume;
  interfaces->AudioEngineStream.AEStream_GetAmplification      = V2::KodiAPI::AudioEngine::CAddOnAEStream::AEStream_GetAmplification;
  interfaces->AudioEngineStream.AEStream_SetAmplification      = V2::KodiAPI::AudioEngine::CAddOnAEStream::AEStream_SetAmplification;
  interfaces->AudioEngineStream.AEStream_GetFrameSize          = V2::KodiAPI::AudioEngine::CAddOnAEStream::AEStream_GetFrameSize;
  interfaces->AudioEngineStream.AEStream_GetChannelCount       = V2::KodiAPI::AudioEngine::CAddOnAEStream::AEStream_GetChannelCount;
  interfaces->AudioEngineStream.AEStream_GetSampleRate         = V2::KodiAPI::AudioEngine::CAddOnAEStream::AEStream_GetSampleRate;
  interfaces->AudioEngineStream.AEStream_GetDataFormat         = V2::KodiAPI::AudioEngine::CAddOnAEStream::AEStream_GetDataFormat;
  interfaces->AudioEngineStream.AEStream_GetResampleRatio      = V2::KodiAPI::AudioEngine::CAddOnAEStream::AEStream_GetResampleRatio;
  interfaces->AudioEngineStream.AEStream_SetResampleRatio      = V2::KodiAPI::AudioEngine::CAddOnAEStream::AEStream_SetResampleRatio;
}

unsigned int CAddOnAEStream::AEStream_GetSpace(void* addonData, void* streamHandle)
{
  try
  {
    if (!addonData || !streamHandle)
      throw ADDON::WrongValueException("CAddOnAEStream - %s - invalid data (addonData='%p', streamHandle='%p')",
                                         __FUNCTION__, addonData, streamHandle);

    return static_cast<IAEStream*>(streamHandle)->GetSpace();
  }
  HANDLE_ADDON_EXCEPTION
  return 0;
}

unsigned int CAddOnAEStream::AEStream_AddData(void* addonData, void* streamHandle, uint8_t* const *Data, unsigned int Offset, unsigned int Frames)
{
  try
  {
    if (!addonData || !streamHandle)
      throw ADDON::WrongValueException("CAddOnAEStream - %s - invalid data (addonData='%p', streamHandle='%p')",
                                         __FUNCTION__, addonData, streamHandle);

    return static_cast<IAEStream*>(streamHandle)->AddData(Data, Offset, Frames);
  }
  HANDLE_ADDON_EXCEPTION
  return 0;
}

double CAddOnAEStream::AEStream_GetDelay(void* addonData, void* streamHandle)
{
  try
  {
    if (!addonData || !streamHandle)
      throw ADDON::WrongValueException("CAddOnAEStream - %s - invalid data (addonData='%p', streamHandle='%p')",
                                         __FUNCTION__, addonData, streamHandle);

    return static_cast<IAEStream*>(streamHandle)->GetDelay();
  }
  HANDLE_ADDON_EXCEPTION
  return 0;
}

bool CAddOnAEStream::AEStream_IsBuffering(void* addonData, void* streamHandle)
{
  try
  {
    if (!addonData || !streamHandle)
      throw ADDON::WrongValueException("CAddOnAEStream - %s - invalid data (addonData='%p', streamHandle='%p')",
                                         __FUNCTION__, addonData, streamHandle);

    return static_cast<IAEStream*>(streamHandle)->IsBuffering();
  }
  HANDLE_ADDON_EXCEPTION
  return false;
}

double CAddOnAEStream::AEStream_GetCacheTime(void* addonData, void* streamHandle)
{
  try
  {
    if (!addonData || !streamHandle)
      throw ADDON::WrongValueException("CAddOnAEStream - %s - invalid data (addonData='%p', streamHandle='%p')",
                                         __FUNCTION__, addonData, streamHandle);

    return static_cast<IAEStream*>(streamHandle)->GetCacheTime();
  }
  HANDLE_ADDON_EXCEPTION

  return -1.0;
}

double CAddOnAEStream::AEStream_GetCacheTotal(void* addonData, void* streamHandle)
{
  try
  {
    if (!addonData || !streamHandle)
      throw ADDON::WrongValueException("CAddOnAEStream - %s - invalid data (addonData='%p', streamHandle='%p')",
                                         __FUNCTION__, addonData, streamHandle);

    return static_cast<IAEStream*>(streamHandle)->GetCacheTotal();
  }
  HANDLE_ADDON_EXCEPTION

  return -1.0;
}

void CAddOnAEStream::AEStream_Pause(void* addonData, void* streamHandle)
{
  try
  {
    if (!addonData || !streamHandle)
      throw ADDON::WrongValueException("CAddOnAEStream - %s - invalid data (addonData='%p', streamHandle='%p')",
                                         __FUNCTION__, addonData, streamHandle);

    static_cast<IAEStream*>(streamHandle)->Pause();
  }
  HANDLE_ADDON_EXCEPTION
}

void CAddOnAEStream::AEStream_Resume(void* addonData, void* streamHandle)
{
  try
  {
    if (!addonData || !streamHandle)
      throw ADDON::WrongValueException("CAddOnAEStream - %s - invalid data (addonData='%p', streamHandle='%p')",
                                         __FUNCTION__, addonData, streamHandle);

    static_cast<IAEStream*>(streamHandle)->Resume();
  }
  HANDLE_ADDON_EXCEPTION
}

void CAddOnAEStream::AEStream_Drain(void* addonData, void* streamHandle, bool Wait)
{
  try
  {
    if (!addonData || !streamHandle)
      throw ADDON::WrongValueException("CAddOnAEStream - %s - invalid data (addonData='%p', streamHandle='%p')",
                                         __FUNCTION__, addonData, streamHandle);

    static_cast<IAEStream*>(streamHandle)->Drain(Wait);
  }
  HANDLE_ADDON_EXCEPTION
}

bool CAddOnAEStream::AEStream_IsDraining(void* addonData, void* streamHandle)
{
  try
  {
    if (!addonData || !streamHandle)
      throw ADDON::WrongValueException("CAddOnAEStream - %s - invalid data (addonData='%p', streamHandle='%p')",
                                         __FUNCTION__, addonData, streamHandle);

    return static_cast<IAEStream*>(streamHandle)->IsDraining();
  }
  HANDLE_ADDON_EXCEPTION

  return false;
}

bool CAddOnAEStream::AEStream_IsDrained(void* addonData, void* streamHandle)
{
  try
  {
    if (!addonData || !streamHandle)
      throw ADDON::WrongValueException("CAddOnAEStream - %s - invalid data (addonData='%p', streamHandle='%p')",
                                         __FUNCTION__, addonData, streamHandle);

    return static_cast<IAEStream*>(streamHandle)->IsDrained();
  }
  HANDLE_ADDON_EXCEPTION

  return false;
}

void CAddOnAEStream::AEStream_Flush(void* addonData, void* streamHandle)
{
  try
  {
    if (!addonData || !streamHandle)
      throw ADDON::WrongValueException("CAddOnAEStream - %s - invalid data (addonData='%p', streamHandle='%p')",
                                         __FUNCTION__, addonData, streamHandle);

    static_cast<IAEStream*>(streamHandle)->Flush();
  }
  HANDLE_ADDON_EXCEPTION
}

float CAddOnAEStream::AEStream_GetVolume(void* addonData, void* streamHandle)
{
  try
  {
    if (!addonData || !streamHandle)
      throw ADDON::WrongValueException("CAddOnAEStream - %s - invalid data (addonData='%p', streamHandle='%p')",
                                         __FUNCTION__, addonData, streamHandle);

    return static_cast<IAEStream*>(streamHandle)->GetVolume();
  }
  HANDLE_ADDON_EXCEPTION

  return -1.0f;
}

void CAddOnAEStream::AEStream_SetVolume(void* addonData, void* streamHandle, float Volume)
{
  try
  {
    if (!addonData || !streamHandle)
      throw ADDON::WrongValueException("CAddOnAEStream - %s - invalid data (addonData='%p', streamHandle='%p')",
                                         __FUNCTION__, addonData, streamHandle);

    static_cast<IAEStream*>(streamHandle)->SetVolume(Volume);
  }
  HANDLE_ADDON_EXCEPTION
}

float CAddOnAEStream::AEStream_GetAmplification(void* addonData, void* streamHandle)
{
  try
  {
    if (!addonData || !streamHandle)
      throw ADDON::WrongValueException("CAddOnAEStream - %s - invalid data (addonData='%p', streamHandle='%p')",
                                         __FUNCTION__, addonData, streamHandle);

    return static_cast<IAEStream*>(streamHandle)->GetAmplification();
  }
  HANDLE_ADDON_EXCEPTION

  return -1.0f;
}

void CAddOnAEStream::AEStream_SetAmplification(void* addonData, void* streamHandle, float Amplify)
{
  try
  {
    if (!addonData || !streamHandle)
      throw ADDON::WrongValueException("CAddOnAEStream - %s - invalid data (addonData='%p', streamHandle='%p')",
                                         __FUNCTION__, addonData, streamHandle);

    static_cast<IAEStream*>(streamHandle)->SetAmplification(Amplify);
  }
  HANDLE_ADDON_EXCEPTION
}

const unsigned int CAddOnAEStream::AEStream_GetFrameSize(void* addonData, void* streamHandle)
{
  try
  {
    if (!addonData || !streamHandle)
      throw ADDON::WrongValueException("CAddOnAEStream - %s - invalid data (addonData='%p', streamHandle='%p')",
                                         __FUNCTION__, addonData, streamHandle);

    return static_cast<IAEStream*>(streamHandle)->GetFrameSize();
  }
  HANDLE_ADDON_EXCEPTION

  return 0;
}

const unsigned int CAddOnAEStream::AEStream_GetChannelCount(void* addonData, void* streamHandle)
{
  try
  {
    if (!addonData || !streamHandle)
      throw ADDON::WrongValueException("CAddOnAEStream - %s - invalid data (addonData='%p', streamHandle='%p')",
                                         __FUNCTION__, addonData, streamHandle);

    return static_cast<IAEStream*>(streamHandle)->GetChannelCount();
  }
  HANDLE_ADDON_EXCEPTION

  return 0;
}

const unsigned int CAddOnAEStream::AEStream_GetSampleRate(void* addonData, void* streamHandle)
{
  try
  {
    if (!addonData || !streamHandle)
      throw ADDON::WrongValueException("CAddOnAEStream - %s - invalid data (addonData='%p', streamHandle='%p')",
                                         __FUNCTION__, addonData, streamHandle);

    return static_cast<IAEStream*>(streamHandle)->GetSampleRate();
  }
  HANDLE_ADDON_EXCEPTION

  return 0;
}

const int CAddOnAEStream::AEStream_GetDataFormat(void* addonData, void* streamHandle)
{
  try
  {
    if (!addonData || !streamHandle)
      throw ADDON::WrongValueException("CAddOnAEStream - %s - invalid data (addonData='%p', streamHandle='%p')",
                                         __FUNCTION__, addonData, streamHandle);

    return static_cast<IAEStream*>(streamHandle)->GetDataFormat();
  }
  HANDLE_ADDON_EXCEPTION

  return AE_FMT_INVALID;
}

double CAddOnAEStream::AEStream_GetResampleRatio(void* addonData, void* streamHandle)
{
  try
  {
    if (!addonData || !streamHandle)
      throw ADDON::WrongValueException("CAddOnAEStream - %s - invalid data (addonData='%p', streamHandle='%p')",
                                         __FUNCTION__, addonData, streamHandle);

    return static_cast<IAEStream*>(streamHandle)->GetResampleRatio();
  }
  HANDLE_ADDON_EXCEPTION

  return -1.0;
}

void CAddOnAEStream::AEStream_SetResampleRatio(void* addonData, void* streamHandle, double Ratio)
{
  try
  {
    if (!addonData || !streamHandle)
      throw ADDON::WrongValueException("CAddOnAEStream - %s - invalid data (addonData='%p', streamHandle='%p')",
                                         __FUNCTION__, addonData, streamHandle);

    static_cast<IAEStream*>(streamHandle)->SetResampleRatio(Ratio);
  }
  HANDLE_ADDON_EXCEPTION
}

} /* extern "C" */
} /* namespace AudioEngine */

} /* namespace KodiAPI */
} /* namespace V2 */
