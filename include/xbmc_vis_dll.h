#pragma once
/*
 *      Copyright (C) 2005-2010 Team XBMC
 *      http://www.xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#ifndef __XBMC_VIS_H__
#define __XBMC_VIS_H__

#include "xbmc_addon_dll.h"
#include "xbmc_vis_types.h"

#include <ctype.h>
#if defined(_LINUX) && !defined(__APPLE__)
#include <sys/sysinfo.h>
#elif _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif
#include <sys/stat.h>
#include <errno.h>

int htoi(const char *str) /* Convert hex string to integer */
{
  unsigned int digit, number = 0;
  while (*str)
  {
    if (isdigit(*str))
      digit = *str - '0';
    else
      digit = tolower(*str)-'a'+10;
    number<<=4;
    number+=digit;
    str++;
  }
  return number;
}

extern "C"
{
  // exports for d3d hacks
#ifndef HAS_SDL_OPENGL
  void d3dSetTextureStageState( int x, DWORD dwY, DWORD dwZ);
  void d3dSetRenderState(DWORD dwY, DWORD dwZ);
#endif

#ifdef HAS_SDL_OPENGL
#ifndef D3DCOLOR_RGBA
#define D3DCOLOR_RGBA(r,g,b,a) (r||(g<<8)||(b<<16)||(a<<24))
#endif
#endif

  // Functions that your visualisation must implement
  void Start(int iChannels, int iSamplesPerSec, int iBitsPerSample, const char* szSongName);
  void AudioData(const short* pAudioData, int iAudioDataLength, float *pFreqData, int iFreqDataLength);
  void Render();
  void Stop();
  bool OnAction(long action, const void *param);
  void GetInfo(VIS_INFO* pInfo);
  unsigned int GetPresets(char ***presets);
  unsigned GetPreset();
  bool IsLocked();

  // function to export the above structure to XBMC
  void __declspec(dllexport) get_addon(struct Visualisation* pVisz)
  {
    pVisz->Start = Start;
    pVisz->AudioData = AudioData;
    pVisz->Render = Render;
    pVisz->Stop = Stop;
    pVisz->OnAction = OnAction;
    pVisz->GetInfo = GetInfo;
    pVisz->GetPresets = GetPresets;
    pVisz->GetPreset = GetPreset;
    pVisz->IsLocked = IsLocked;
  };
};

#endif
