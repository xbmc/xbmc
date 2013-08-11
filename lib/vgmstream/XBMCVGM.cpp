/*
 *      Copyright (C) 2008-2013 Team XBMC
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

#ifndef _LINUX
#include <windows.h>
#else
#define __cdecl
#define __declspec(x)
#endif

extern "C"
{
  #include "src/vgmstream.h"
  
  long __declspec(dllexport) DLL_Init()
  {
    return 1;
  }

  long __declspec(dllexport) DLL_LoadVGM(const char* szFileName, int* sampleRate, int* sampleSize, int* channels)
  {
    VGMSTREAM* result;

    if ((result = init_vgmstream(szFileName)) == NULL)
      return 0;

    *sampleRate = result->sample_rate;
    *sampleSize = 16;
    *channels = result->channels;
    return (long)result;
  }

  void __declspec(dllexport) DLL_FreeVGM(long vgm)
  {
    close_vgmstream((VGMSTREAM*)vgm);
  }

  int __declspec(dllexport) DLL_FillBuffer(long vgm, char* szBuffer, int iSize)
  {
    VGMSTREAM* song = (VGMSTREAM*)vgm;
    render_vgmstream((sample*)szBuffer,iSize/(2*song->channels),(VGMSTREAM*)vgm);
    
    return iSize;
  }

  unsigned long __declspec(dllexport) DLL_Seek(long vgm, unsigned long timepos)
  {
    VGMSTREAM* song = (VGMSTREAM*)vgm;
    int16_t* buffer = new int16_t[576*song->channels];
    long samples_to_do = (long)timepos * song->sample_rate / 1000L;
    if (samples_to_do < song->current_sample )
       reset_vgmstream(song);
    else
      samples_to_do -= song->current_sample;
      
    while (samples_to_do > 0)
    {
      long l = samples_to_do>576?576:samples_to_do;
      render_vgmstream(buffer,l,song);
      samples_to_do -= l;
    }
    delete[] buffer;

    return timepos;
  }

  unsigned long __declspec(dllexport) DLL_GetLength(long vgm)
  {
    VGMSTREAM* song = (VGMSTREAM*)vgm;

    return song->num_samples/song->sample_rate*1000;
  }
}
