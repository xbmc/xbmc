#pragma once
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

#include "kodi/api2/.internal/AddonLib_internal.hpp"

#include <string>

extern "C"
{

  struct CKODIAddon_InterProcess_AudioEngine_Stream
  {

    void* AE_MakeStream(AudioEngineFormat Format, unsigned int Options);
    void AE_FreeStream(void* hdl);
    unsigned int AE_Stream_GetSpace(void* hdl);
    unsigned int AE_Stream_AddData(void* hdl, uint8_t* const *Data, unsigned int Offset, unsigned int Frames, unsigned int planes);
    double AE_Stream_GetDelay(void* hdl);
    bool AE_Stream_IsBuffering(void* hdl);
    double AE_Stream_GetCacheTime(void* hdl);
    double AE_Stream_GetCacheTotal(void* hdl);
    void AE_Stream_Pause(void* hdl);
    void AE_Stream_Resume(void* hdl);
    void AE_Stream_Drain(void* hdl, bool Wait);
    bool AE_Stream_IsDraining(void* hdl);
    bool AE_Stream_IsDrained(void* hdl);
    void AE_Stream_Flush(void* hdl);
    float AE_Stream_GetVolume(void* hdl);
    void AE_Stream_SetVolume(void* hdl, float Volume);
    float AE_Stream_GetAmplification(void* hdl);
    void AE_Stream_SetAmplification(void* hdl, float Amplify);
    const unsigned int AE_Stream_GetFrameSize(void* hdl);
    const unsigned int AE_Stream_GetChannelCount(void* hdl);
    const unsigned int AE_Stream_GetSampleRate(void* hdl);
    const AEDataFormat AE_Stream_GetDataFormat(void* hdl);
    double AE_Stream_GetResampleRatio(void* hdl);
    void AE_Stream_SetResampleRatio(void* hdl, double Ratio);

  };

}; /* extern "C" */
