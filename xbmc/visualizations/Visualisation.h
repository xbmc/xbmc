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
// Visualisation.h: interface for the CVisualisation class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_Visualisation_H__99B9A52D_ED09_4540_A887_162A68217A31__INCLUDED_)
#define AFX_Visualisation_H__99B9A52D_ED09_4540_A887_162A68217A31__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
#include "Key.h"
#include "DllVisualisation.h"

class CVisualisation
{
public:
  enum VIS_ACTION { VIS_ACTION_NONE = 0,
                    VIS_ACTION_NEXT_PRESET,
                    VIS_ACTION_PREV_PRESET,
                    VIS_ACTION_LOAD_PRESET,
                    VIS_ACTION_RANDOM_PRESET,
                    VIS_ACTION_LOCK_PRESET,
                    VIS_ACTION_RATE_PRESET_PLUS,
                    VIS_ACTION_RATE_PRESET_MINUS,
                    VIS_ACTION_UPDATE_ALBUMART};
  CVisualisation(struct Visualisation* pVisz, DllVisualisation* pDll, const CStdString& strVisualisationName);
  ~CVisualisation();

  void Create(int posx, int posy, int width, int height);
  void Start(int iChannels, int iSamplesPerSec, int iBitsPerSample, const CStdString strSongName);
  void AudioData(const short* pAudioData, int iAudioDataLength, float *pFreqData, int iFreqDataLength);
  void Render();
  void Stop();
  void GetInfo(VIS_INFO *info);
  bool OnAction(VIS_ACTION action, void *param = NULL);
  void GetSettings(std::vector<VisSetting> **vecSettings);
  void UpdateSetting(int num);
  void GetPresets(char ***pPresets, int *currentPreset, int *numPresets, bool *locked);
  void GetCurrentPreset(char **pPreset, bool *locked);
  bool IsLocked();
  char *GetPreset();

protected:
  std::auto_ptr<struct Visualisation> m_pVisz;
  std::auto_ptr<DllVisualisation> m_pDll;
  CStdString m_strVisualisationName;
};


#endif // !defined(AFX_Visualisation_H__99B9A52D_ED09_4540_A887_162A68217A31__INCLUDED_)
