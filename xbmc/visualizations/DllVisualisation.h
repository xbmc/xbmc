#pragma once
/*
 *      Copyright (C) 2005-2008 Team XBMC
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
#include "DynamicDll.h"

struct VIS_INFO 
{ 
  bool bWantsFreq; 
  int iSyncDelay; 
  //  int iAudioDataLength; 
  //  int iFreqDataLength; 
}; 
 
// The VisSetting class for GUI settings for vis. 
class VisSetting 
{ 
public: 
  enum SETTING_TYPE { NONE=0, CHECK, SPIN }; 
  VisSetting() 
  { 
    name = NULL; 
    current = 0; 
    type = NONE; 
  }; 
  ~VisSetting() 
  { 
    if (name) 
      delete[] name; 
    name = NULL; 
  } 
  SETTING_TYPE type; 
  char *name; 
  int  current; 
  std::vector<const char *> entry; 
}; 
 
struct Visualisation 
{ 
public: 
  void (__cdecl* Create)(LPDIRECT3DDEVICE8 pd3dDevice, int iPosX, int iPosY, int iWidth, int iHeight, const char* szVisualisation, float pixelRatio); 
  void (__cdecl* Start)(int iChannels, int iSamplesPerSec, int iBitsPerSample, const char* szSongName); 
  void (__cdecl* AudioData)(short* pAudioData, int iAudioDataLength, float *pFreqData, int iFreqDataLength); 
  void (__cdecl* Render) (); 
  void (__cdecl* Stop)(); 
  void (__cdecl* GetInfo)(VIS_INFO *info); 
  bool (__cdecl* OnAction)(long flags, void *param); 
  void (__cdecl *GetSettings)(std::vector<VisSetting> **vecSettings); 
  void (__cdecl *UpdateSetting)(int num); 
  void (__cdecl *GetPresets)(char ***pPresets, int *currentPreset, int *numPresets, bool *locked); 
}; 

class DllVisualisationInterface
{
public:
  void GetModule(struct Visualisation* pScr);
};

class DllVisualisation : public DllDynamic, DllVisualisationInterface
{
  DECLARE_DLL_WRAPPER_TEMPLATE(DllVisualisation)
  DEFINE_METHOD1(void, GetModule, (struct Visualisation* p1))
  BEGIN_METHOD_RESOLVE()
    RESOLVE_METHOD_RENAME(get_module,GetModule)
  END_METHOD_RESOLVE()
};
