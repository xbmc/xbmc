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

#include "InterProcess_AudioEngine_Stream.h"
#include "InterProcess.h"
#include "RequestPacket.h"
#include "ResponsePacket.h"

#include <p8-platform/util/StringUtils.h>
#include <iostream>       // std::cerr

using namespace P8PLATFORM;

extern "C"
{

  void* CKODIAddon_InterProcess_AudioEngine_Stream::AE_MakeStream(AudioEngineFormat format, unsigned int Options)
  {
    try
    {
      uint64_t retPtr;
      CKODIAddon_InterProcess *session = g_threadMap.at(std::this_thread::get_id());
      CRequestPacket vrp(KODICall_AudioEngine_Stream_MakeStream, session);
      vrp.push(API_INT,          &format.m_dataFormat);
      vrp.push(API_UNSIGNED_INT, &format.m_sampleRate);
      vrp.push(API_UNSIGNED_INT, &format.m_encodedRate);
      vrp.push(API_UNSIGNED_INT, &format.m_channelCount);
      for (unsigned int i = 0; i < AE_CH_MAX; ++i)
        vrp.push(API_INT, &format.m_channels[i]);
      vrp.push(API_UNSIGNED_INT, &format.m_frames);
      vrp.push(API_UNSIGNED_INT, &format.m_frameSize);
      vrp.push(API_UNSIGNED_INT, &Options);
      uint32_t retCode = PROCESS_ADDON_CALL_WITH_RETURN_POINTER(session, vrp, &retPtr);
      if (retCode != API_SUCCESS)
        throw retCode;
      void* ptr = (void*)retPtr;
      return ptr;
    }
    PROCESS_HANDLE_EXCEPTION;
    return nullptr;
  }

  void CKODIAddon_InterProcess_AudioEngine_Stream::AE_FreeStream(void* hdl)
  {
    try
    {
      uint64_t ptr = (uint64_t)hdl;
      CKODIAddon_InterProcess *session = g_threadMap.at(std::this_thread::get_id());
      CRequestPacket vrp(KODICall_AudioEngine_Stream_FreeStream, session);
      vrp.push(API_UINT64_T, &ptr);
      uint32_t retCode = PROCESS_ADDON_CALL(session, vrp);
      if (retCode != API_SUCCESS)
        throw retCode;
    }
    PROCESS_HANDLE_EXCEPTION;
  }

  unsigned int CKODIAddon_InterProcess_AudioEngine_Stream::AE_Stream_GetSpace(void* hdl)
  {
    try
    {
      unsigned int ret;
      uint64_t ptr = (uint64_t)hdl;
      CKODIAddon_InterProcess *session = g_threadMap.at(std::this_thread::get_id());
      CRequestPacket vrp(KODICall_AudioEngine_Stream_GetSpace, session);
      vrp.push(API_UINT64_T, &ptr);
      uint32_t retCode = PROCESS_ADDON_CALL_WITH_RETURN_UNSIGNED_INTEGER(session, vrp, &ret);
      if (retCode != API_SUCCESS)
        throw retCode;
      return ret;
    }
    PROCESS_HANDLE_EXCEPTION;
    return -1;
  }

  unsigned int CKODIAddon_InterProcess_AudioEngine_Stream::AE_Stream_AddData(void* hdl, uint8_t* const *data, unsigned int offset, unsigned int frames, unsigned int planes)
  {
    try
    {
      unsigned int ret;
      uint64_t ptr = (uint64_t)hdl;
      CKODIAddon_InterProcess *session = g_threadMap.at(std::this_thread::get_id());
      CRequestPacket vrp(KODICall_AudioEngine_Stream_AddData, session);
      vrp.push(API_UINT64_T, &ptr);
      vrp.push(API_UNSIGNED_INT, &offset);
      vrp.push(API_UNSIGNED_INT, &frames);
      vrp.push(API_UNSIGNED_INT, &planes);
      for (unsigned int i = 0; i < planes; ++i)
      {
        for (unsigned int j = 0; j < frames; ++j)
          vrp.push(API_UINT8_T, data[i]+j);
      }
      uint32_t retCode = PROCESS_ADDON_CALL_WITH_RETURN_UNSIGNED_INTEGER(session, vrp, &ret);
      if (retCode != API_SUCCESS)
        throw retCode;
      return ret;
    }
    PROCESS_HANDLE_EXCEPTION;
    return -1;
  }

  double CKODIAddon_InterProcess_AudioEngine_Stream::AE_Stream_GetDelay(void* hdl)
  {
    try
    {
      double delay;
      uint64_t ptr = (uint64_t)hdl;
      CKODIAddon_InterProcess *session = g_threadMap.at(std::this_thread::get_id());
      CRequestPacket vrp(KODICall_AudioEngine_Stream_GetDelay, session);
      vrp.push(API_UINT64_T, &ptr);
      uint32_t retCode = PROCESS_ADDON_CALL_WITH_RETURN_DOUBLE(session, vrp, &delay);
      if (retCode != API_SUCCESS)
        throw retCode;
      return delay;
    }
    PROCESS_HANDLE_EXCEPTION;
    return 0.0;
  }

  bool CKODIAddon_InterProcess_AudioEngine_Stream::AE_Stream_IsBuffering(void* hdl)
  {
    try
    {
      bool isBuffering;
      uint64_t ptr = (uint64_t)hdl;
      CKODIAddon_InterProcess *session = g_threadMap.at(std::this_thread::get_id());
      CRequestPacket vrp(KODICall_AudioEngine_Stream_IsBuffering, session);
      vrp.push(API_UINT64_T, &ptr);
      uint32_t retCode = PROCESS_ADDON_CALL_WITH_RETURN_BOOLEAN(session, vrp, &isBuffering);
      if (retCode != API_SUCCESS)
        throw retCode;
      return isBuffering;
    }
    PROCESS_HANDLE_EXCEPTION;
    return false;
  }

  double CKODIAddon_InterProcess_AudioEngine_Stream::AE_Stream_GetCacheTime(void* hdl)
  {
    try
    {
      double time;
      uint64_t ptr = (uint64_t)hdl;
      CKODIAddon_InterProcess *session = g_threadMap.at(std::this_thread::get_id());
      CRequestPacket vrp(KODICall_AudioEngine_Stream_GetCacheTime, session);
      vrp.push(API_UINT64_T, &ptr);
      uint32_t retCode = PROCESS_ADDON_CALL_WITH_RETURN_DOUBLE(session, vrp, &time);
      if (retCode != API_SUCCESS)
        throw retCode;
      return time;
    }
    PROCESS_HANDLE_EXCEPTION;
    return 0.0;
  }

  double CKODIAddon_InterProcess_AudioEngine_Stream::AE_Stream_GetCacheTotal(void* hdl)
  {
    try
    {
      double time;
      uint64_t ptr = (uint64_t)hdl;
      CKODIAddon_InterProcess *session = g_threadMap.at(std::this_thread::get_id());
      CRequestPacket vrp(KODICall_AudioEngine_Stream_GetCacheTotal, session);
      vrp.push(API_UINT64_T, &ptr);
      uint32_t retCode = PROCESS_ADDON_CALL_WITH_RETURN_DOUBLE(session, vrp, &time);
      if (retCode != API_SUCCESS)
        throw retCode;
      return time;
    }
    PROCESS_HANDLE_EXCEPTION;
    return 0.0;
  }

  void CKODIAddon_InterProcess_AudioEngine_Stream::AE_Stream_Pause(void* hdl)
  {
    try
    {
      uint64_t ptr = (uint64_t)hdl;
      CKODIAddon_InterProcess *session = g_threadMap.at(std::this_thread::get_id());
      CRequestPacket vrp(KODICall_AudioEngine_Stream_Pause, session);
      vrp.push(API_UINT64_T, &ptr);
      uint32_t retCode = PROCESS_ADDON_CALL(session, vrp);
      if (retCode != API_SUCCESS)
        throw retCode;
    }
    PROCESS_HANDLE_EXCEPTION;
  }

  void CKODIAddon_InterProcess_AudioEngine_Stream::AE_Stream_Resume(void* hdl)
  {
    try
    {
      uint64_t ptr = (uint64_t)hdl;
      CKODIAddon_InterProcess *session = g_threadMap.at(std::this_thread::get_id());
      CRequestPacket vrp(KODICall_AudioEngine_Stream_Resume, session);
      vrp.push(API_UINT64_T, &ptr);
      uint32_t retCode = PROCESS_ADDON_CALL(session, vrp);
      if (retCode != API_SUCCESS)
        throw retCode;
    }
    PROCESS_HANDLE_EXCEPTION;
  }

  void CKODIAddon_InterProcess_AudioEngine_Stream::AE_Stream_Drain(void* hdl, bool wait)
  {
    try
    {
      uint64_t ptr = (uint64_t)hdl;
      CKODIAddon_InterProcess *session = g_threadMap.at(std::this_thread::get_id());
      CRequestPacket vrp(KODICall_AudioEngine_Stream_Drain, session);
      vrp.push(API_UINT64_T, &ptr);
      vrp.push(API_BOOLEAN, &wait);
      uint32_t retCode = PROCESS_ADDON_CALL(session, vrp);
      if (retCode != API_SUCCESS)
        throw retCode;
    }
    PROCESS_HANDLE_EXCEPTION;
  }

  bool CKODIAddon_InterProcess_AudioEngine_Stream::AE_Stream_IsDraining(void* hdl)
  {
    try
    {
      bool isDraining;
      uint64_t ptr = (uint64_t)hdl;
      CKODIAddon_InterProcess *session = g_threadMap.at(std::this_thread::get_id());
      CRequestPacket vrp(KODICall_AudioEngine_Stream_IsDraining, session);
      vrp.push(API_UINT64_T, &ptr);
      uint32_t retCode = PROCESS_ADDON_CALL_WITH_RETURN_BOOLEAN(session, vrp, &isDraining);
      if (retCode != API_SUCCESS)
        throw retCode;
      return isDraining;
    }
    PROCESS_HANDLE_EXCEPTION;
    return false;
  }

  bool CKODIAddon_InterProcess_AudioEngine_Stream::AE_Stream_IsDrained(void* hdl)
  {
    try
    {
      bool isDrained;
      uint64_t ptr = (uint64_t)hdl;
      CKODIAddon_InterProcess *session = g_threadMap.at(std::this_thread::get_id());
      CRequestPacket vrp(KODICall_AudioEngine_Stream_IsDrained, session);
      vrp.push(API_UINT64_T, &ptr);
      uint32_t retCode = PROCESS_ADDON_CALL_WITH_RETURN_BOOLEAN(session, vrp, &isDrained);
      if (retCode != API_SUCCESS)
        throw retCode;
      return isDrained;
    }
    PROCESS_HANDLE_EXCEPTION;
    return false;
  }

  void CKODIAddon_InterProcess_AudioEngine_Stream::AE_Stream_Flush(void* hdl)
  {
    try
    {
      uint64_t ptr = (uint64_t)hdl;
      CKODIAddon_InterProcess *session = g_threadMap.at(std::this_thread::get_id());
      CRequestPacket vrp(KODICall_AudioEngine_Stream_Flush, session);
      vrp.push(API_UINT64_T, &ptr);
      uint32_t retCode = PROCESS_ADDON_CALL(session, vrp);
      if (retCode != API_SUCCESS)
        throw retCode;
    }
    PROCESS_HANDLE_EXCEPTION;
  }

  float CKODIAddon_InterProcess_AudioEngine_Stream::AE_Stream_GetVolume(void* hdl)
  {
    try
    {
      float volume;
      uint64_t ptr = (uint64_t)hdl;
      CKODIAddon_InterProcess *session = g_threadMap.at(std::this_thread::get_id());
      CRequestPacket vrp(KODICall_AudioEngine_Stream_GetVolume, session);
      vrp.push(API_UINT64_T, &ptr);
      uint32_t retCode = PROCESS_ADDON_CALL_WITH_RETURN_FLOAT(session, vrp, &volume);
      if (retCode != API_SUCCESS)
        throw retCode;
      return volume;
    }
    PROCESS_HANDLE_EXCEPTION;
    return 0.0f;
  }

  void CKODIAddon_InterProcess_AudioEngine_Stream::AE_Stream_SetVolume(void* hdl, float volume)
  {
    try
    {
      uint64_t ptr = (uint64_t)hdl;
      CKODIAddon_InterProcess *session = g_threadMap.at(std::this_thread::get_id());
      CRequestPacket vrp(KODICall_AudioEngine_Stream_SetVolume, session);
      vrp.push(API_UINT64_T, &ptr);
      vrp.push(API_FLOAT, &volume);
      uint32_t retCode = PROCESS_ADDON_CALL(session, vrp);
      if (retCode != API_SUCCESS)
        throw retCode;
    }
    PROCESS_HANDLE_EXCEPTION;
  }

  float CKODIAddon_InterProcess_AudioEngine_Stream::AE_Stream_GetAmplification(void* hdl)
  {
    try
    {
      float amplification;
      uint64_t ptr = (uint64_t)hdl;
      CKODIAddon_InterProcess *session = g_threadMap.at(std::this_thread::get_id());
      CRequestPacket vrp(KODICall_AudioEngine_Stream_GetAmplification, session);
      vrp.push(API_UINT64_T, &ptr);
      uint32_t retCode = PROCESS_ADDON_CALL_WITH_RETURN_FLOAT(session, vrp, &amplification);
      if (retCode != API_SUCCESS)
        throw retCode;
      return amplification;
    }
    PROCESS_HANDLE_EXCEPTION;
    return 0.0f;
  }

  void CKODIAddon_InterProcess_AudioEngine_Stream::AE_Stream_SetAmplification(void* hdl, float amplify)
  {
    try
    {
      uint64_t ptr = (uint64_t)hdl;
      CKODIAddon_InterProcess *session = g_threadMap.at(std::this_thread::get_id());
      CRequestPacket vrp(KODICall_AudioEngine_Stream_SetAmplification, session);
      vrp.push(API_UINT64_T, &ptr);
      vrp.push(API_FLOAT, &amplify);
      uint32_t retCode = PROCESS_ADDON_CALL(session, vrp);
      if (retCode != API_SUCCESS)
        throw retCode;
    }
    PROCESS_HANDLE_EXCEPTION;
  }

  const unsigned int CKODIAddon_InterProcess_AudioEngine_Stream::AE_Stream_GetFrameSize(void* hdl)
  {
    try
    {
      unsigned int frameSize;
      uint64_t ptr = (uint64_t)hdl;
      CKODIAddon_InterProcess *session = g_threadMap.at(std::this_thread::get_id());
      CRequestPacket vrp(KODICall_AudioEngine_Stream_GetFrameSize, session);
      vrp.push(API_UINT64_T, &ptr);
      uint32_t retCode = PROCESS_ADDON_CALL_WITH_RETURN_UNSIGNED_INTEGER(session, vrp, &frameSize);
      if (retCode != API_SUCCESS)
        throw retCode;
      return frameSize;
    }
    PROCESS_HANDLE_EXCEPTION;
    return 0;
  }

  const unsigned int CKODIAddon_InterProcess_AudioEngine_Stream::AE_Stream_GetChannelCount(void* hdl)
  {
    try
    {
      unsigned int channelCount;
      uint64_t ptr = (uint64_t)hdl;
      CKODIAddon_InterProcess *session = g_threadMap.at(std::this_thread::get_id());
      CRequestPacket vrp(KODICall_AudioEngine_Stream_GetChannelCount, session);
      vrp.push(API_UINT64_T, &ptr);
      uint32_t retCode = PROCESS_ADDON_CALL_WITH_RETURN_UNSIGNED_INTEGER(session, vrp, &channelCount);
      if (retCode != API_SUCCESS)
        throw retCode;
      return channelCount;
    }
    PROCESS_HANDLE_EXCEPTION;
    return 0;
  }

  const unsigned int CKODIAddon_InterProcess_AudioEngine_Stream::AE_Stream_GetSampleRate(void* hdl)
  {
    try
    {
      unsigned int sampleRate;
      uint64_t ptr = (uint64_t)hdl;
      CKODIAddon_InterProcess *session = g_threadMap.at(std::this_thread::get_id());
      CRequestPacket vrp(KODICall_AudioEngine_Stream_GetSampleRate, session);
      vrp.push(API_UINT64_T, &ptr);
      uint32_t retCode = PROCESS_ADDON_CALL_WITH_RETURN_UNSIGNED_INTEGER(session, vrp, &sampleRate);
      if (retCode != API_SUCCESS)
        throw retCode;
      return sampleRate;
    }
    PROCESS_HANDLE_EXCEPTION;
    return 0;
  }

  const AEDataFormat CKODIAddon_InterProcess_AudioEngine_Stream::AE_Stream_GetDataFormat(void* hdl)
  {
    try
    {
      int dataFormat;
      uint64_t ptr = (uint64_t)hdl;
      CKODIAddon_InterProcess *session = g_threadMap.at(std::this_thread::get_id());
      CRequestPacket vrp(KODICall_AudioEngine_Stream_GetDataFormat, session);
      vrp.push(API_UINT64_T, &ptr);
      uint32_t retCode = PROCESS_ADDON_CALL_WITH_RETURN_INTEGER(session, vrp, &dataFormat);
      if (retCode != API_SUCCESS)
        throw retCode;
      return (AEDataFormat)dataFormat;
    }
    PROCESS_HANDLE_EXCEPTION;
    return AE_FMT_INVALID;
  }

  double CKODIAddon_InterProcess_AudioEngine_Stream::AE_Stream_GetResampleRatio(void* hdl)
  {
    try
    {
      double ratio;
      uint64_t ptr = (uint64_t)hdl;
      CKODIAddon_InterProcess *session = g_threadMap.at(std::this_thread::get_id());
      CRequestPacket vrp(KODICall_AudioEngine_Stream_GetResampleRatio, session);
      vrp.push(API_UINT64_T, &ptr);
      uint32_t retCode = PROCESS_ADDON_CALL_WITH_RETURN_DOUBLE(session, vrp, &ratio);
      if (retCode != API_SUCCESS)
        throw retCode;
      return ratio;
    }
    PROCESS_HANDLE_EXCEPTION;
    return 0.0;
  }

  void CKODIAddon_InterProcess_AudioEngine_Stream::AE_Stream_SetResampleRatio(void* hdl, double ratio)
  {
    try
    {
      uint64_t ptr = (uint64_t)hdl;
      CKODIAddon_InterProcess *session = g_threadMap.at(std::this_thread::get_id());
      CRequestPacket vrp(KODICall_AudioEngine_Stream_SetResampleRatio, session);
      vrp.push(API_UINT64_T, &ptr);
      vrp.push(API_DOUBLE, &ratio);
      uint32_t retCode = PROCESS_ADDON_CALL(session, vrp);
      if (retCode != API_SUCCESS)
        throw retCode;
    }
    PROCESS_HANDLE_EXCEPTION;
  }

}; /* extern "C" */
