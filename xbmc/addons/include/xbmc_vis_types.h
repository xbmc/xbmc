/*
 *      Copyright (C) 2005-2012 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

/*
  Common data structures shared between XBMC and XBMC's visualisations
 */

#ifndef __VISUALISATION_TYPES_H__
#define __VISUALISATION_TYPES_H__
#include <cstddef>

extern "C"
{
  struct VIS_INFO
  {
    int bWantsFreq;
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
    const char *name;
    const char *presets;
    const char *profile;
    const char *submodule;
  };

  enum VIS_ACTION
  { 
    VIS_ACTION_NONE = 0,
    VIS_ACTION_NEXT_PRESET,
    VIS_ACTION_PREV_PRESET,
    VIS_ACTION_LOAD_PRESET,
    VIS_ACTION_RANDOM_PRESET,
    VIS_ACTION_LOCK_PRESET,
    VIS_ACTION_RATE_PRESET_PLUS,
    VIS_ACTION_RATE_PRESET_MINUS,
    VIS_ACTION_UPDATE_ALBUMART,
    VIS_ACTION_UPDATE_TRACK
  };

  class VisTrack
  {
  public:
    VisTrack()
    {
      title = artist = album = albumArtist = NULL;
      genre = comment = lyrics = reserved1 = reserved2 = NULL;
      trackNumber = discNumber = duration = year = 0;
      rating = 0;
      reserved3 = reserved4 = 0;
    }

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
  };

  struct Visualisation
  {
    void (__cdecl* Start)(int iChannels, int iSamplesPerSec, int iBitsPerSample, const char* szSongName);
    void (__cdecl* AudioData)(const float* pAudioData, int iAudioDataLength, float *pFreqData, int iFreqDataLength);
    void (__cdecl* Render) ();
    void (__cdecl* GetInfo)(VIS_INFO *info);
    bool (__cdecl* OnAction)(long flags, const void *param);
    int (__cdecl* HasPresets)();
    unsigned int (__cdecl *GetPresets)(char ***presets);
    unsigned int (__cdecl *GetPreset)();
    unsigned int (__cdecl *GetSubModules)(char ***modules);
    bool (__cdecl* IsLocked)();
  };
}

#endif //__VISUALISATION_TYPES_H__
