
#include "GUIWindowVisualisation.h"
#include "settings.h"
#include "application.h"
#include "util.h"
#include "visualizations/VisualisationFactory.h"
#include "visualizations/fft.h"
#include "utils/singlelock.h"
#define LABEL_ROW1 10
#define LABEL_ROW2 11
#define LABEL_ROW3 12

CGUIWindowVisualisation::CGUIWindowVisualisation(void)
:CGUIWindow(0)
{ 
	m_pVisualisation=NULL;
	m_bCalculate_Freq = false;
	m_iNumBuffers = 0;
	m_iBuffer = 0;
}

CGUIWindowVisualisation::~CGUIWindowVisualisation(void)
{
}


void CGUIWindowVisualisation::OnKey(const CKey& key)
{
	if ( key.IsButton() )
	{
		switch (key.GetButtonCode() )
		{
			case KEY_BUTTON_X:
			case KEY_REMOTE_DISPLAY:
				// back 2 UI
				m_gWindowManager.ActivateWindow(0); // back 2 home
			break;
		}
	}
  CGUIWindow::OnKey(key);
}

bool CGUIWindowVisualisation::OnMessage(CGUIMessage& message)
{
	switch ( message.GetMessage() )
	{
		case GUI_MSG_WINDOW_DEINIT:
		{
			if (m_pVisualisation)
				delete m_pVisualisation;
			m_pVisualisation=NULL;
			if (g_application.m_pPlayer)
				g_application.m_pPlayer->UnRegisterAudioCallback();
		}
		break;

		case GUI_MSG_WINDOW_INIT:
		{
			m_bInitialized=false;
			CVisualisationFactory factory;
			CStdString strVisz;
			strVisz.Format("Q:\\visualisations\\%s", g_stSettings.szDefaultVisualisation);
			m_pVisualisation=factory.LoadVisualisation(strVisz.c_str());
			m_pVisualisation->Create();
			OnInitialize(2, 44100, 16);
			if (g_application.m_pPlayer)
				g_application.m_pPlayer->RegisterAudioCallback(this);
		}
	}
	return CGUIWindow::OnMessage(message);
}

void CGUIWindowVisualisation::Render()
{
	if (m_pVisualisation)
	{
		if (m_bInitialized)
		{
			/*
			short pAudioData[1152*2];
			for (int i=0; i < 1152*2; ++i)
				pAudioData[i]=rand()%255;
			OnAudioData((byte*)pAudioData, 1152*2);*/
			m_pVisualisation->Render();
			m_pVisualisation->Render();
			m_pVisualisation->Render();
		}
	}
	CGUIWindow::Render();
}


void CGUIWindowVisualisation::OnInitialize(int iChannels, int iSamplesPerSec, int iBitsPerSample)
{
	if (!m_pVisualisation) 
		return;

	m_bInitialized=true;
	m_iChannels = iChannels;
	m_iSamplesPerSec = iSamplesPerSec;
	m_iBitsPerSample = iBitsPerSample;

	// Clear our audio buffers
	ClearBuffers();
	// Start the visualisation (this loads settings etc.)
	m_pVisualisation->Start(m_iChannels, m_iSamplesPerSec, m_iBitsPerSample);
	// Create new audio buffers
	CreateBuffers();
}

void CGUIWindowVisualisation::OnAudioData(const unsigned char* pAudioData, int iAudioDataLength)
{
	if (!m_pVisualisation) 
		return;
	if (!m_bInitialized) return;
	VIS_INFO info;
	m_pVisualisation->GetInfo(&info);

	// Convert data to 16 bit shorts
	const short *sAudioData = (const short *)pAudioData;
	iAudioDataLength/=2;

	CSingleLock lock(m_critSection);

	// Save our audio data in the buffers
	for (int i=0; i < iAudioDataLength; i++)
	{
		if (i >= 2*AUDIO_BUFFER_SIZE) break;
		m_pBuffer[m_iBuffer][i]=sAudioData[i];
	}
	// Fill with zeros if necessary...
	for (int i=iAudioDataLength; i<2*AUDIO_BUFFER_SIZE; i++)
	{
		m_pBuffer[m_iBuffer][i]=0;
	}

	// Increment our delay pointer so that our data is now in sync
	m_iBuffer++;
	if (m_iBuffer >= m_iNumBuffers)
		m_iBuffer = 0;
	// Fourier transform the data if the vis wants it...
	if (info.bWantsFreq)
	{
		// Convert to floats
		for (int i=0; i<2*AUDIO_BUFFER_SIZE; i++)
			m_pFreq[i] = (float) m_pBuffer[m_iBuffer][i];
		// FFT the data
		twochanwithwindow(m_pFreq, AUDIO_BUFFER_SIZE);
		// Normalize the data
		float fMinData = (float)AUDIO_BUFFER_SIZE*AUDIO_BUFFER_SIZE*3/8*0.5*0.5;	// 3/8 for the Hann window, 0.5 as minimum amplitude
		for (int i=0; i<AUDIO_BUFFER_SIZE+2; i++)
			m_pFreq[i] /= fMinData;
		// Transfer data to our visualisation
		m_pVisualisation->AudioData(m_pBuffer[m_iBuffer], AUDIO_BUFFER_SIZE, m_pFreq, AUDIO_BUFFER_SIZE);
	}
	else
	{	// Transfer data to our visualisation
		m_pVisualisation->AudioData(m_pBuffer[m_iBuffer], AUDIO_BUFFER_SIZE, NULL, 0);
	}
	return;
}

void CGUIWindowVisualisation::CreateBuffers()
{
	// Get the number of buffers from the current vis
	VIS_INFO info;
	m_pVisualisation->GetInfo(&info);
	m_iNumBuffers = info.iSyncDelay + 1;
	if (m_iNumBuffers > MAX_AUDIO_BUFFERS)
		m_iNumBuffers = MAX_AUDIO_BUFFERS;
	if (m_iNumBuffers < 1)
		m_iNumBuffers = 1;
	// create the buffers
	for (int i=0; i<m_iNumBuffers; i++)
	{
		//		m_pBuffer[i] = new short[AUDIO_BUFFER_SIZE*2];
		for (int j=0; j<AUDIO_BUFFER_SIZE*2; j++)
			m_pBuffer[i][j] = 0;
	}
	//	m_pFreq = new float[AUDIO_BUFFER_SIZE*2];
	for (int j=0; j<AUDIO_BUFFER_SIZE*2; j++)
		m_pFreq[j] = 0.0f;
	// reset current buffer to zero
	m_iBuffer = 0;
}


void CGUIWindowVisualisation::ClearBuffers()
{
}
