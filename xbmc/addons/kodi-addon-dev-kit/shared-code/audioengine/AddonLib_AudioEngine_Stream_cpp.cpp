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
#include "kodi/api2/audioengine/Stream.hpp"

namespace V2
{
namespace KodiAPI
{

namespace AudioEngine
{

  CStream::CStream(
      AudioEngineFormat   Format,
      unsigned int        Options)
  {
    m_StreamHandle = g_interProcess.AE_MakeStream(Format, Options);
    if (!m_StreamHandle)
      fprintf(stderr, "libKODI_audioengine-ERROR: make_stream failed!\n");
    m_planes = g_interProcess.AE_Stream_GetChannelCount(m_StreamHandle);
  }

  CStream::~CStream()
  {
    g_interProcess.AE_FreeStream(m_StreamHandle);
  }

  unsigned int CStream::GetSpace()
  {
    return g_interProcess.AE_Stream_GetSpace(m_StreamHandle);
  }

  unsigned int CStream::AddData(uint8_t* const *Data, unsigned int Offset, unsigned int Frames)
  {
    return g_interProcess.AE_Stream_AddData(m_StreamHandle, Data, Offset, Frames, m_planes);
  }

  double CStream::GetDelay()
  {
    return g_interProcess.AE_Stream_GetDelay(m_StreamHandle);
  }

  bool CStream::IsBuffering()
  {
    return g_interProcess.AE_Stream_IsBuffering(m_StreamHandle);
  }

  double CStream::GetCacheTime()
  {
    return g_interProcess.AE_Stream_GetCacheTime(m_StreamHandle);
  }

  double CStream::GetCacheTotal()
  {
    return g_interProcess.AE_Stream_GetCacheTotal(m_StreamHandle);
  }

  void CStream::Pause()
  {
    return g_interProcess.AE_Stream_Pause(m_StreamHandle);
  }

  void CStream::Resume()
  {
    return g_interProcess.AE_Stream_Resume(m_StreamHandle);
  }

  void CStream::Drain(bool Wait)
  {
    return g_interProcess.AE_Stream_Drain(m_StreamHandle, Wait);
  }

  bool CStream::IsDraining()
  {
    return g_interProcess.AE_Stream_IsDraining(m_StreamHandle);
  }

  bool CStream::IsDrained()
  {
    return g_interProcess.AE_Stream_IsDrained(m_StreamHandle);
  }

  void CStream::Flush()
  {
    return g_interProcess.AE_Stream_Flush(m_StreamHandle);
  }

  float CStream::GetVolume()
  {
    return g_interProcess.AE_Stream_GetVolume(m_StreamHandle);
  }

  void CStream::SetVolume(float Volume)
  {
    return g_interProcess.AE_Stream_SetVolume(m_StreamHandle, Volume);
  }

  float CStream::GetAmplification()
  {
    return g_interProcess.AE_Stream_GetAmplification(m_StreamHandle);
  }

  void CStream::SetAmplification(float Amplify)
  {
    return g_interProcess.AE_Stream_SetAmplification(m_StreamHandle, Amplify);
  }

  const unsigned int CStream::GetFrameSize() const
  {
    return g_interProcess.AE_Stream_GetFrameSize(m_StreamHandle);
  }

  const unsigned int CStream::GetChannelCount() const
  {
    return g_interProcess.AE_Stream_GetChannelCount(m_StreamHandle);
  }

  const unsigned int CStream::GetSampleRate() const
  {
    return g_interProcess.AE_Stream_GetSampleRate(m_StreamHandle);
  }

  const AEDataFormat CStream::GetDataFormat() const
  {
    return g_interProcess.AE_Stream_GetDataFormat(m_StreamHandle);
  }

  double CStream::GetResampleRatio()
  {
    return g_interProcess.AE_Stream_GetResampleRatio(m_StreamHandle);
  }

  void CStream::SetResampleRatio(double Ratio)
  {
    g_interProcess.AE_Stream_SetResampleRatio(m_StreamHandle, Ratio);
  }

}; /* namespace AudioEngine */

}; /* namespace KodiAPI */
}; /* namespace V2 */
