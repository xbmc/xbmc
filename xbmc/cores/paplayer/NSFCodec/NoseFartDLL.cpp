/*
 *      Copyright (C) 2008-2010 Team XBMC
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

#ifdef __linux__
#define __declspec(x) 
#endif
extern "C" 
{

#include "src/types.h"
#include "src/log.h"
#include "src/version.h"
#include "src/machine/nsf.h"

  long __declspec(dllexport) DLL_LoadNSF(const char* szFileName)
  {
    nsf_init();
    log_init();
    nsf_t* result = nsf_load(const_cast<char*>(szFileName),NULL,0);
    return (long)result;
  }

  void __declspec(dllexport) DLL_FreeNSF(int nsf)
  {
    nsf_t* pNsf = (nsf_t*)nsf;
    nsf_free(&pNsf);
  }

  long __declspec(dllexport) DLL_GetTitle(int nsf)
  {
    return (long)((nsf_t*)nsf)->song_name;
  }
  
  long __declspec(dllexport) DLL_GetArtist(int nsf)
  {
    return (long)((nsf_t*)nsf)->artist_name;
  }
  
  int __declspec(dllexport) DLL_StartPlayback(int nsf, int track)
  {
    nsf_playtrack((nsf_t*)nsf,track,48000,16,false);
    for (int i = 0; i < 6; i++)
      nsf_setchan((nsf_t*)nsf,i,true);
    return 1;
  }

  long __declspec(dllexport) DLL_FillBuffer(int nsf, char* buffer, int size)
  {
    nsf_t* pNsf = (nsf_t*)nsf;
    nsf_frame(pNsf);
    pNsf->process(buffer,size);
    return size*2;
  }

  void __declspec(dllexport) DLL_FrameAdvance(int nsf)
  {
    nsf_frame((nsf_t*)nsf);
  }

  int __declspec(dllexport) DLL_GetPlaybackRate(int nsf)
  {
    return ((nsf_t*)nsf)->playback_rate;
  }

  int __declspec(dllexport) DLL_GetNumberOfSongs(int nsf)
  {
    return (int)((nsf_t*)nsf)->num_songs;
  }
}
