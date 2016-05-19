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

#include "InterProcess.h"
#include KITINCLUDE(ADDON_API_LEVEL, audioengine/Stream.hpp)

API_NAMESPACE

namespace KodiAPI
{

namespace AudioEngine
{

  CStream::CStream(
      AudioEngineFormat   Format,
      unsigned int        Options)
  {
    m_StreamHandle = g_interProcess.m_Callbacks->AudioEngine.make_stream(g_interProcess.m_Handle, Format, Options);
    if (!m_StreamHandle)
      fprintf(stderr, "libKODI_audioengine-ERROR: make_stream failed!\n");
    m_planes = g_interProcess.m_Callbacks->AudioEngineStream.AEStream_GetChannelCount(g_interProcess.m_Handle, m_StreamHandle);
  }

  CStream::~CStream()
  {
    g_interProcess.m_Callbacks->AudioEngine.free_stream(g_interProcess.m_Handle, m_StreamHandle);
  }

  unsigned int CStream::GetSpace()
  {
    return g_interProcess.m_Callbacks->AudioEngineStream.AEStream_GetSpace(g_interProcess.m_Handle, m_StreamHandle);
  }

  unsigned int CStream::AddData(uint8_t* const *Data, unsigned int Offset, unsigned int Frames)
  {
    return g_interProcess.m_Callbacks->AudioEngineStream.AEStream_AddData(g_interProcess.m_Handle, m_StreamHandle, Data, Offset, Frames);
  }

  double CStream::GetDelay()
  {
    return g_interProcess.m_Callbacks->AudioEngineStream.AEStream_GetDelay(g_interProcess.m_Handle, m_StreamHandle);
  }

  bool CStream::IsBuffering()
  {
    return g_interProcess.m_Callbacks->AudioEngineStream.AEStream_IsBuffering(g_interProcess.m_Handle, m_StreamHandle);
  }

  double CStream::GetCacheTime()
  {
    return g_interProcess.m_Callbacks->AudioEngineStream.AEStream_GetCacheTime(g_interProcess.m_Handle, m_StreamHandle);
  }

  double CStream::GetCacheTotal()
  {
    return g_interProcess.m_Callbacks->AudioEngineStream.AEStream_GetCacheTotal(g_interProcess.m_Handle, m_StreamHandle);
  }

  void CStream::Pause()
  {
    return g_interProcess.m_Callbacks->AudioEngineStream.AEStream_Pause(g_interProcess.m_Handle, m_StreamHandle);
  }

  void CStream::Resume()
  {
    return g_interProcess.m_Callbacks->AudioEngineStream.AEStream_Resume(g_interProcess.m_Handle, m_StreamHandle);
  }

  void CStream::Drain(bool Wait)
  {
    return g_interProcess.m_Callbacks->AudioEngineStream.AEStream_Drain(g_interProcess.m_Handle, m_StreamHandle, Wait);
  }

  bool CStream::IsDraining()
  {
    return g_interProcess.m_Callbacks->AudioEngineStream.AEStream_IsDraining(g_interProcess.m_Handle, m_StreamHandle);
  }

  bool CStream::IsDrained()
  {
    return g_interProcess.m_Callbacks->AudioEngineStream.AEStream_IsDrained(g_interProcess.m_Handle, m_StreamHandle);
  }

  void CStream::Flush()
  {
    return g_interProcess.m_Callbacks->AudioEngineStream.AEStream_Flush(g_interProcess.m_Handle, m_StreamHandle);
  }

  float CStream::GetVolume()
  {
    return g_interProcess.m_Callbacks->AudioEngineStream.AEStream_GetVolume(g_interProcess.m_Handle, m_StreamHandle);
  }

  void CStream::SetVolume(float Volume)
  {
    return g_interProcess.m_Callbacks->AudioEngineStream.AEStream_SetVolume(g_interProcess.m_Handle, m_StreamHandle, Volume);
  }

  float CStream::GetAmplification()
  {
    return g_interProcess.m_Callbacks->AudioEngineStream.AEStream_GetAmplification(g_interProcess.m_Handle, m_StreamHandle);
  }

  void CStream::SetAmplification(float Amplify)
  {
    return g_interProcess.m_Callbacks->AudioEngineStream.AEStream_SetAmplification(g_interProcess.m_Handle, m_StreamHandle, Amplify);
  }

  const unsigned int CStream::GetFrameSize() const
  {
    return g_interProcess.m_Callbacks->AudioEngineStream.AEStream_GetFrameSize(g_interProcess.m_Handle, m_StreamHandle);
  }

  const unsigned int CStream::GetChannelCount() const
  {
    return g_interProcess.m_Callbacks->AudioEngineStream.AEStream_GetChannelCount(g_interProcess.m_Handle, m_StreamHandle);
  }

  const unsigned int CStream::GetSampleRate() const
  {
    return g_interProcess.m_Callbacks->AudioEngineStream.AEStream_GetSampleRate(g_interProcess.m_Handle, m_StreamHandle);
  }

  const AEDataFormat CStream::GetDataFormat() const
  {
    return (AEDataFormat)g_interProcess.m_Callbacks->AudioEngineStream.AEStream_GetDataFormat(g_interProcess.m_Handle, m_StreamHandle);
  }

  double CStream::GetResampleRatio()
  {
    return g_interProcess.m_Callbacks->AudioEngineStream.AEStream_GetResampleRatio(g_interProcess.m_Handle, m_StreamHandle);
  }

  void CStream::SetResampleRatio(double Ratio)
  {
    g_interProcess.m_Callbacks->AudioEngineStream.AEStream_SetResampleRatio(g_interProcess.m_Handle, m_StreamHandle, Ratio);
  }

} /* namespace AudioEngine */
} /* namespace KodiAPI */

END_NAMESPACE()
