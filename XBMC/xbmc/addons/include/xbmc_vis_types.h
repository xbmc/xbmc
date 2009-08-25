/*
 *      Copyright (C) 2005-2009 Team XBMC
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

/*
  Common data structures shared between XBMC and XBMC's visualisations
 */

#ifndef __VISUALISATION_TYPES_H__
#define __VISUALISATION_TYPES_H__

#include "xbmc_addon_types.h"

extern "C"
{
  ////////////////////////////////////////////////////////////////////
  // The VIS_INFO structure to tell XBMC what data you need.
  ////////////////////////////////////////////////////////////////////
  struct VIS_INFO
  {
    bool bWantsFreq;
    int iSyncDelay;
  };

  struct VIS_PROPS
  {
    void *device;
    int x;
    int y;
    int width;
    int height;
    float pixelRatio;
    char *name;
  };

  typedef struct PresetsList
  {
    char**          Presets;
    int             numPresets;
    int             currentPreset;
    bool            locked;
  } PresetsList;

  typedef struct VisTrack
  {
    const char *title;
    const char *artist;
    const char *album;
    const char *albumArtist;
    const char *genre;
    const char *comment;
    const char *lyrics;
    const char *reserved1;
    const char *reserved2;

    int        trackNumber;
    int        discNumber;
    int        duration;
    int        year;
    char       rating;
    int        reserved3;
    int        reserved4;
  } VisTrack;

  struct Visualisation
  {
    void (__cdecl* Init)(void* unused, int iPosX, int iPosY, int iWidth, int iHeight, const char* szVisualisation, float pixelRatio);
    void (__cdecl* Start)(int iChannels, int iSamplesPerSec, int iBitsPerSample, const char* szSongName);
    void (__cdecl* AudioData)(const short* pAudioData, int iAudioDataLength, float *pFreqData, int iFreqDataLength);
    void (__cdecl* Render) ();
    void (__cdecl* Stop)();
    void (__cdecl* GetInfo)(VIS_INFO *info);
    bool (__cdecl* OnAction)(long flags, void *param);
    void (__cdecl *GetPresets)(char ***pPresets, int *currentPreset, int *numPresets, bool *locked);
  };

}

#endif //__VISUALISATION_TYPES_H__
