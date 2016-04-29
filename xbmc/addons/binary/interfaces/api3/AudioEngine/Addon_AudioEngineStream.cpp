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
#include "addons/binary/interfaces/api3/AddonInterfaceBase.h"
#include "addons/binary/interfaces/api2/AudioEngine/Addon_AudioEngineStream.h"
#include "addons/kodi-addon-dev-kit/include/kodi/api3/.internal/AddonLib_internal.hpp"

namespace V3
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

} /* extern "C" */
} /* namespace AudioEngine */

} /* namespace KodiAPI */
} /* namespace V3 */
