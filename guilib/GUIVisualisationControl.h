#pragma once

#include "GUIControl.h"
#include "../xbmc/cores/IAudioCallback.h"

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
  virtual ~CGUIVisualisationControl(void);
  virtual void Render();
  virtual void UpdateVisibility(void *pParam = NULL);
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
  bool UpdateAlbumArt();
  CStdString      m_currentVis;
  CVisualisation* m_pVisualisation;

  int m_iChannels;
  int m_iSamplesPerSec;
  int m_iBitsPerSample;
  list<CAudioBuffer*> m_vecBuffers;
  int m_iNumBuffers;        // Number of Audio buffers
  bool m_bWantsFreq;
  float m_fFreq[2*AUDIO_BUFFER_SIZE];         // Frequency data
  bool m_bCalculate_Freq;       // True if the vis wants freq data
  bool m_bInitialized;
  CStdString m_AlbumThumb;
};
