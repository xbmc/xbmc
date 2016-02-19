/*
 *      Copyright (C) 2016 Team KODI
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

#include "AddonExe_AudioEngine_Stream.h"
#include "addons/Addon.h"
#include "addons/binary/callbacks/api2/AudioEngine/AudioEngineCB_General.h"
#include "addons/binary/callbacks/api2/AudioEngine/AudioEngineCB_Stream.h"

namespace V2
{
namespace KodiAPI
{

bool CAddonExeCB_AudioEngine_Stream::AE_MakeStream(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp)
{
  unsigned int options;
  AudioEngineFormat format;
  req.pop(API_INT,          &format.m_dataFormat);
  req.pop(API_UNSIGNED_INT, &format.m_sampleRate);
  req.pop(API_UNSIGNED_INT, &format.m_encodedRate);
  req.pop(API_UNSIGNED_INT, &format.m_channelCount);
  for (unsigned int i = 0; i < AE_CH_MAX; ++i)
    req.pop(API_INT, &format.m_channels[i]);
  req.pop(API_UNSIGNED_INT, &format.m_frames);
  req.pop(API_UNSIGNED_INT, &format.m_frameSize);
  req.pop(API_UNSIGNED_INT, &options);
  AEStreamHandle* hdl = AudioEngine::CAddOnAEGeneral::make_stream(addon, format, options);
  PROCESS_ADDON_RETURN_CALL_WITH_POINTER(API_SUCCESS, hdl);
  return true;
}

bool CAddonExeCB_AudioEngine_Stream::AE_FreeStream(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp)
{
  uint64_t ptr;
  req.pop(API_UINT64_T, &ptr);
  AudioEngine::CAddOnAEGeneral::free_stream(addon, (AEStreamHandle*)ptr);
  PROCESS_ADDON_RETURN_CALL(API_SUCCESS);
  return true;
}

bool CAddonExeCB_AudioEngine_Stream::AE_Stream_GetSpace(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp)
{
  uint64_t ptr;
  req.pop(API_UINT64_T, &ptr);
  unsigned int space = AudioEngine::CAddOnAEStream::AEStream_GetSpace(addon, (AEStreamHandle*)ptr);
  PROCESS_ADDON_RETURN_CALL_WITH_UNSIGNED_INT(API_SUCCESS, &space);
  return true;
}

bool CAddonExeCB_AudioEngine_Stream::AE_Stream_AddData(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp)
{
  uint64_t ptr;
  unsigned int offset;
  unsigned int frames;
  unsigned int planes;
  req.pop(API_UINT64_T, &ptr);
  req.pop(API_UNSIGNED_INT, &offset);
  req.pop(API_UNSIGNED_INT, &frames);
  req.pop(API_UNSIGNED_INT, &planes);
  uint8_t** data = new uint8_t*[planes];
  for (unsigned int i = 0; i < planes; ++i)
  {
    data[i] = new uint8_t[frames];
    for (unsigned int j = 0; j < frames; ++j)
      req.pop(API_UINT8_T, data[i]+j);
  }
  unsigned int ret = AudioEngine::CAddOnAEStream::AEStream_AddData(addon, (AEStreamHandle*)ptr, data, offset, frames);
  PROCESS_ADDON_RETURN_CALL_WITH_UNSIGNED_INT(API_SUCCESS, &ret);
  for (unsigned int i = 0; i < planes; ++i)
  {
    delete[] data[i];
  }
  delete[] data;
  return true;
}

bool CAddonExeCB_AudioEngine_Stream::AE_Stream_GetDelay(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp)
{
  uint64_t ptr;
  req.pop(API_UINT64_T, &ptr);
  double delay = AudioEngine::CAddOnAEStream::AEStream_GetDelay(addon, (AEStreamHandle*)ptr);
  PROCESS_ADDON_RETURN_CALL_WITH_DOUBLE(API_SUCCESS, &delay);
  return true;
}

bool CAddonExeCB_AudioEngine_Stream::AE_Stream_IsBuffering(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp)
{
  uint64_t ptr;
  req.pop(API_UINT64_T, &ptr);
  bool isBuffering = AudioEngine::CAddOnAEStream::AEStream_IsBuffering(addon, (AEStreamHandle*)ptr);
  PROCESS_ADDON_RETURN_CALL_WITH_BOOLEAN(API_SUCCESS, &isBuffering);
  return true;
}

bool CAddonExeCB_AudioEngine_Stream::AE_Stream_GetCacheTime(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp)
{
  uint64_t ptr;
  req.pop(API_UINT64_T, &ptr);
  double cacheTime = AudioEngine::CAddOnAEStream::AEStream_GetCacheTime(addon, (AEStreamHandle*)ptr);
  PROCESS_ADDON_RETURN_CALL_WITH_DOUBLE(API_SUCCESS, &cacheTime);
  return true;
}

bool CAddonExeCB_AudioEngine_Stream::AE_Stream_GetCacheTotal(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp)
{
  uint64_t ptr;
  req.pop(API_UINT64_T, &ptr);
  double cacheTotal = AudioEngine::CAddOnAEStream::AEStream_GetCacheTotal(addon, (AEStreamHandle*)ptr);
  PROCESS_ADDON_RETURN_CALL_WITH_DOUBLE(API_SUCCESS, &cacheTotal);
  return true;
}

bool CAddonExeCB_AudioEngine_Stream::AE_Stream_Pause(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp)
{
  uint64_t ptr;
  req.pop(API_UINT64_T, &ptr);
  AudioEngine::CAddOnAEStream::AEStream_Pause(addon, (AEStreamHandle*)ptr);
  PROCESS_ADDON_RETURN_CALL(API_SUCCESS);
  return true;
}

bool CAddonExeCB_AudioEngine_Stream::AE_Stream_Resume(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp)
{
  uint64_t ptr;
  req.pop(API_UINT64_T, &ptr);
  AudioEngine::CAddOnAEStream::AEStream_Resume(addon, (AEStreamHandle*)ptr);
  PROCESS_ADDON_RETURN_CALL(API_SUCCESS);
  return true;
}

bool CAddonExeCB_AudioEngine_Stream::AE_Stream_Drain(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp)
{
  bool drain;
  uint64_t ptr;
  req.pop(API_UINT64_T, &ptr);
  req.pop(API_BOOLEAN, &drain);
  AudioEngine::CAddOnAEStream::AEStream_Drain(addon, (AEStreamHandle*)ptr, drain);
  PROCESS_ADDON_RETURN_CALL(API_SUCCESS);
  return true;
}

bool CAddonExeCB_AudioEngine_Stream::AE_Stream_IsDraining(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp)
{
  uint64_t ptr;
  req.pop(API_UINT64_T, &ptr);
  bool draining = AudioEngine::CAddOnAEStream::AEStream_IsDraining(addon, (AEStreamHandle*)ptr);
  PROCESS_ADDON_RETURN_CALL_WITH_BOOLEAN(API_SUCCESS, &draining);
  return true;
}

bool CAddonExeCB_AudioEngine_Stream::AE_Stream_IsDrained(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp)
{
  uint64_t ptr;
  req.pop(API_UINT64_T, &ptr);
  bool draining = AudioEngine::CAddOnAEStream::AEStream_IsDrained(addon, (AEStreamHandle*)ptr);
  PROCESS_ADDON_RETURN_CALL_WITH_BOOLEAN(API_SUCCESS, &draining);
  return true;
}

bool CAddonExeCB_AudioEngine_Stream::AE_Stream_Flush(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp)
{
  uint64_t ptr;
  req.pop(API_UINT64_T, &ptr);
  AudioEngine::CAddOnAEStream::AEStream_Flush(addon, (AEStreamHandle*)ptr);
  PROCESS_ADDON_RETURN_CALL(API_SUCCESS);
  return true;
}

bool CAddonExeCB_AudioEngine_Stream::AE_Stream_GetVolume(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp)
{
  uint64_t ptr;
  req.pop(API_UINT64_T, &ptr);
  float volume = AudioEngine::CAddOnAEStream::AEStream_GetVolume(addon, (AEStreamHandle*)ptr);
  PROCESS_ADDON_RETURN_CALL_WITH_FLOAT(API_SUCCESS, &volume);
  return true;
}

bool CAddonExeCB_AudioEngine_Stream::AE_Stream_SetVolume(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp)
{
  float volume;
  uint64_t ptr;
  req.pop(API_UINT64_T, &ptr);
  req.pop(API_FLOAT, &volume);
  AudioEngine::CAddOnAEStream::AEStream_SetVolume(addon, (AEStreamHandle*)ptr, volume);
  PROCESS_ADDON_RETURN_CALL(API_SUCCESS);
  return true;
}

bool CAddonExeCB_AudioEngine_Stream::AE_Stream_GetAmplification(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp)
{
  uint64_t ptr;
  req.pop(API_UINT64_T, &ptr);
  float amplification = AudioEngine::CAddOnAEStream::AEStream_GetAmplification(addon, (AEStreamHandle*)ptr);
  PROCESS_ADDON_RETURN_CALL_WITH_FLOAT(API_SUCCESS, &amplification);
  return true;
}

bool CAddonExeCB_AudioEngine_Stream::AE_Stream_SetAmplification(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp)
{
  float amplification;
  uint64_t ptr;
  req.pop(API_UINT64_T, &ptr);
  req.pop(API_FLOAT, &amplification);
  AudioEngine::CAddOnAEStream::AEStream_SetAmplification(addon, (AEStreamHandle*)ptr, amplification);
  PROCESS_ADDON_RETURN_CALL(API_SUCCESS);
  return true;
}

bool CAddonExeCB_AudioEngine_Stream::AE_Stream_GetFrameSize(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp)
{
  uint64_t ptr;
  req.pop(API_UINT64_T, &ptr);
  unsigned int frameSize = AudioEngine::CAddOnAEStream::AEStream_GetFrameSize(addon, (AEStreamHandle*)ptr);
  PROCESS_ADDON_RETURN_CALL_WITH_UNSIGNED_INT(API_SUCCESS, &frameSize);
  return true;
}

bool CAddonExeCB_AudioEngine_Stream::AE_Stream_GetChannelCount(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp)
{
  uint64_t ptr;
  req.pop(API_UINT64_T, &ptr);
  unsigned int channelcount = AudioEngine::CAddOnAEStream::AEStream_GetChannelCount(addon, (AEStreamHandle*)ptr);
  PROCESS_ADDON_RETURN_CALL_WITH_UNSIGNED_INT(API_SUCCESS, &channelcount);
  return true;
}

bool CAddonExeCB_AudioEngine_Stream::AE_Stream_GetSampleRate(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp)
{
  uint64_t ptr;
  req.pop(API_UINT64_T, &ptr);
  unsigned int sampleRate = AudioEngine::CAddOnAEStream::AEStream_GetSampleRate(addon, (AEStreamHandle*)ptr);
  PROCESS_ADDON_RETURN_CALL_WITH_UNSIGNED_INT(API_SUCCESS, &sampleRate);
  return true;
}

bool CAddonExeCB_AudioEngine_Stream::AE_Stream_GetDataFormat(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp)
{
  uint64_t ptr;
  req.pop(API_UINT64_T, &ptr);
  int dataFormat = AudioEngine::CAddOnAEStream::AEStream_GetDataFormat(addon, (AEStreamHandle*)ptr);
  PROCESS_ADDON_RETURN_CALL_WITH_INT(API_SUCCESS, &dataFormat);
  return true;
}

bool CAddonExeCB_AudioEngine_Stream::AE_Stream_GetResampleRatio(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp)
{
  uint64_t ptr;
  req.pop(API_UINT64_T, &ptr);
  double resampleRatio = AudioEngine::CAddOnAEStream::AEStream_GetSampleRate(addon, (AEStreamHandle*)ptr);
  PROCESS_ADDON_RETURN_CALL_WITH_DOUBLE(API_SUCCESS, &resampleRatio);
  return true;
}

bool CAddonExeCB_AudioEngine_Stream::AE_Stream_SetResampleRatio(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp)
{
  double resampleRatio;
  uint64_t ptr;
  req.pop(API_UINT64_T, &ptr);
  req.pop(API_DOUBLE, &resampleRatio);
  AudioEngine::CAddOnAEStream::AEStream_SetResampleRatio(addon, (AEStreamHandle*)ptr, resampleRatio);
  PROCESS_ADDON_RETURN_CALL(API_SUCCESS);
  return true;
}

}; /* namespace KodiAPI */
}; /* namespace V2 */
