#pragma once
#include "guiwindow.h"
#include "guiwindowmanager.h"
#include "visualizations/Visualisation.h"
#include "cores/IAudioCallback.h"
#include "utils/CriticalSection.h"

#define AUDIO_BUFFER_SIZE 1024	// MUST BE A POWER OF 2!!!
#define MAX_AUDIO_BUFFERS 32

class CGUIWindowVisualisation :
  public CGUIWindow, public IAudioCallback
{
public:
  CGUIWindowVisualisation(void);
  virtual ~CGUIWindowVisualisation(void);
  virtual bool    OnMessage(CGUIMessage& message);
  virtual void    OnKey(const CKey& key);
	virtual void		Render();
	virtual void		OnInitialize(int iChannels, int iSamplesPerSec, int iBitsPerSample);
	virtual void		OnAudioData(const unsigned char* pAudioData, int iAudioDataLength);

private:
	void						CreateBuffers();
	void						ClearBuffers();
	CVisualisation* m_pVisualisation;

	int								m_iChannels;
	int								m_iSamplesPerSec;
	int								m_iBitsPerSample;
	short							m_pBuffer[MAX_AUDIO_BUFFERS][2*AUDIO_BUFFER_SIZE];		// Audio data buffers
	int								m_iBuffer;									// Current buffer
	int								m_iNumBuffers;								// Number of Audio buffers
	float							m_pFreq[2*AUDIO_BUFFER_SIZE];									// Frequency data
	bool							m_bCalculate_Freq;							// True if the vis wants freq data
	CCriticalSection	m_critSection;
	bool							m_bInitialized;
};
