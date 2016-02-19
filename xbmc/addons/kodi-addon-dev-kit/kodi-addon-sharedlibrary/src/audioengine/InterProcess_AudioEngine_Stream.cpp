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

extern "C"
{

  void* AE_MakeStream(AudioEngineFormat Format, unsigned int Options)
  {
    return g_interProcess.m_Callbacks->AudioEngine.make_stream(g_interProcess.m_Handle, Format, Options);
  }

  void AE_FreeStream(void* hdl)
  {
    g_interProcess.m_Callbacks->AudioEngine.free_stream(g_interProcess.m_Handle, hdl);
  }

  unsigned int AE_Stream_GetSpace(void* hdl)
  {
    return g_interProcess.m_Callbacks->AudioEngineStream.AEStream_GetSpace(g_interProcess.m_Handle, hdl);
  }

  unsigned int AE_Stream_AddData(void* hdl, uint8_t* const *Data, unsigned int Offset, unsigned int Frames, unsigned int planes)
  {
    return g_interProcess.m_Callbacks->AudioEngineStream.AEStream_AddData(g_interProcess.m_Handle, hdl, Data, Offset, Frames);
  }

  double AE_Stream_GetDelay(void* hdl)
  {
    return g_interProcess.m_Callbacks->AudioEngineStream.AEStream_GetDelay(g_interProcess.m_Handle, hdl);
  }

  bool AE_Stream_IsBuffering(void* hdl)
  {
    return g_interProcess.m_Callbacks->AudioEngineStream.AEStream_IsBuffering(g_interProcess.m_Handle, hdl);
  }

  double AE_Stream_GetCacheTime(void* hdl)
  {
    return g_interProcess.m_Callbacks->AudioEngineStream.AEStream_GetCacheTime(g_interProcess.m_Handle, hdl);
  }

  double AE_Stream_GetCacheTotal(void* hdl)
  {
    return g_interProcess.m_Callbacks->AudioEngineStream.AEStream_GetCacheTotal(g_interProcess.m_Handle, hdl);
  }

  void AE_Stream_Pause(void* hdl)
  {
    return g_interProcess.m_Callbacks->AudioEngineStream.AEStream_Pause(g_interProcess.m_Handle, hdl);
  }

  void AE_Stream_Resume(void* hdl)
  {
    return g_interProcess.m_Callbacks->AudioEngineStream.AEStream_Resume(g_interProcess.m_Handle, hdl);
  }

  void AE_Stream_Drain(void* hdl, bool Wait)
  {
    return g_interProcess.m_Callbacks->AudioEngineStream.AEStream_Drain(g_interProcess.m_Handle, hdl, Wait);
  }

  bool AE_Stream_IsDraining(void* hdl)
  {
    return g_interProcess.m_Callbacks->AudioEngineStream.AEStream_IsDraining(g_interProcess.m_Handle, hdl);
  }

  bool AE_Stream_IsDrained(void* hdl)
  {
    return g_interProcess.m_Callbacks->AudioEngineStream.AEStream_IsDrained(g_interProcess.m_Handle, hdl);
  }

  void AE_Stream_Flush(void* hdl)
  {
    return g_interProcess.m_Callbacks->AudioEngineStream.AEStream_Flush(g_interProcess.m_Handle, hdl);
  }

  float AE_Stream_GetVolume(void* hdl)
  {
    return g_interProcess.m_Callbacks->AudioEngineStream.AEStream_GetVolume(g_interProcess.m_Handle, hdl);
  }

  void AE_Stream_SetVolume(void* hdl, float Volume)
  {
    return g_interProcess.m_Callbacks->AudioEngineStream.AEStream_SetVolume(g_interProcess.m_Handle, hdl, Volume);
  }

  float AE_Stream_GetAmplification(void* hdl)
  {
    return g_interProcess.m_Callbacks->AudioEngineStream.AEStream_GetAmplification(g_interProcess.m_Handle, hdl);
  }

  void AE_Stream_SetAmplification(void* hdl, float Amplify)
  {
    return g_interProcess.m_Callbacks->AudioEngineStream.AEStream_SetAmplification(g_interProcess.m_Handle, hdl, Amplify);
  }

  const unsigned int AE_Stream_GetFrameSize(void* hdl)
  {
    return g_interProcess.m_Callbacks->AudioEngineStream.AEStream_GetFrameSize(g_interProcess.m_Handle, hdl);
  }

  const unsigned int AE_Stream_GetChannelCount(void* hdl)
  {
    return g_interProcess.m_Callbacks->AudioEngineStream.AEStream_GetChannelCount(g_interProcess.m_Handle, hdl);
  }

  const unsigned int AE_Stream_GetSampleRate(void* hdl)
  {
    return g_interProcess.m_Callbacks->AudioEngineStream.AEStream_GetSampleRate(g_interProcess.m_Handle, hdl);
  }

  const AEDataFormat AE_Stream_GetDataFormat(void* hdl)
  {
    return g_interProcess.m_Callbacks->AudioEngineStream.AEStream_GetDataFormat(g_interProcess.m_Handle, hdl);
  }

  double AE_Stream_GetResampleRatio(void* hdl)
  {
    return g_interProcess.m_Callbacks->AudioEngineStream.AEStream_GetResampleRatio(g_interProcess.m_Handle, hdl);
  }

  void AE_Stream_SetResampleRatio(void* hdl, double Ratio)
  {
    g_interProcess.m_Callbacks->AudioEngineStream.AEStream_SetResampleRatio(g_interProcess.m_Handle, hdl, Ratio);
  }

}; /* extern "C" */
