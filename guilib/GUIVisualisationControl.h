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

#include "GUIControl.h"
#include "cores/IAudioCallback.h"

// forward definitions
class CVisualisation;

#define AUDIO_BUFFER_SIZE 512 // MUST BE A POWER OF 2!!!
#define MAX_AUDIO_BUFFERS 16

class CAudioBuffer
{
public:
  CAudioBuffer(int iSize);
  virtual ~CAudioBuffer();
  const short* Get() const;
  void Set(const unsigned char* psBuffer, int iSize, int iBitsPerSample);
private:
  CAudioBuffer();
  short* m_pBuffer;
  int m_iLen;
};

class CGUIVisualisationControl :
      public CGUIControl, public IAudioCallback
{
public:
  CGUIVisualisationControl(DWORD dwParentID, DWORD dwControlId, float posX, float posY, float width, float height);
  CGUIVisualisationControl(const CGUIVisualisationControl &from);
  virtual ~CGUIVisualisationControl(void);
  virtual CGUIVisualisationControl *Clone() const { return new CGUIVisualisationControl(*this); };

  virtual void Render();
  virtual void UpdateVisibility(const CGUIListItem *item = NULL);
  virtual void FreeResources();
  virtual void OnInitialize(int iChannels, int iSamplesPerSec, int iBitsPerSample);
  virtual void OnAudioData(const unsigned char* pAudioData, int iAudioDataLength);
  virtual bool OnAction(const CAction &action);
  virtual bool OnMessage(CGUIMessage& message);
  virtual bool OnMouseOver(const CPoint &point);
  virtual bool CanFocus() const;
  virtual bool CanFocusFromPoint(const CPoint &point, CGUIControl **control, CPoint &controlPoint) const;

  CVisualisation *GetVisualisation();
private:
  void FreeVisualisation();
  void LoadVisualisation();
  void CreateBuffers();
  void ClearBuffers();
  bool UpdateTrack();
  CStdString      m_currentVis;
  CVisualisation* m_pVisualisation;

  int m_iChannels;
  int m_iSamplesPerSec;
  int m_iBitsPerSample;
  std::list<CAudioBuffer*> m_vecBuffers;
  int m_iNumBuffers;        // Number of Audio buffers
  bool m_bWantsFreq;
  float m_fFreq[2*AUDIO_BUFFER_SIZE];         // Frequency data
  bool m_bCalculate_Freq;       // True if the vis wants freq data
  bool m_bInitialized;
  CStdString m_AlbumThumb;
};
