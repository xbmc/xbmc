
#include "stdafx.h"
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



CAudioBuffer::CAudioBuffer(int iSize)
{
	m_iLen=iSize;
	m_pBuffer = new short[iSize];
}

CAudioBuffer::~CAudioBuffer()
{
	delete [] m_pBuffer;
}

const short* CAudioBuffer::Get() const
{
	return m_pBuffer;
}

void CAudioBuffer::Set(const unsigned char* psBuffer, int iSize, int iBitsPerSample)
{
	if (iBitsPerSample == 16)
	{
		iSize/=2;
		for (int i=0; i < iSize, i<m_iLen; i++)
		{// 16 bit -> convert to short directly
			m_pBuffer[i] = ((short *)psBuffer)[i];
		}
	}
	else if (iBitsPerSample == 8)
	{
		for (int i=0; i < iSize, i<m_iLen; i++)
		{// 8 bit -> convert to signed short by multiplying by 256
			m_pBuffer[i] = ((short)((char *)psBuffer)[i]) << 8;
		}
	}
	else	// assume 24 bit data
	{
		iSize/=3;
		for (int i=0; i < iSize, i < m_iLen; i++)
		{// 24 bit -> ignore least significant byte and convert to signed short
			m_pBuffer[i] = (((int)psBuffer[3*i+1])<<0) + (((int)((char *)psBuffer)[3*i+2]) << 8);
		}
	}
	for (int i=iSize; i < m_iLen;++i) m_pBuffer[i] = 0;
}

CGUIWindowVisualisation::CGUIWindowVisualisation(void)
:CGUIWindow(0)
{ 
	m_pVisualisation= NULL;
	m_iNumBuffers   = 0;
}

CGUIWindowVisualisation::~CGUIWindowVisualisation(void)
{
}


void CGUIWindowVisualisation::OnAction(const CAction &action)
{
	switch (action.wID)
	{
		case ACTION_SHOW_INFO:
			//send the action to the overlay
			g_application.m_guiMusicOverlay.OnAction(action);
			break;

		case ACTION_SHOW_GUI:
			//send the action to the overlay so we can reset
			//the bool m_bShowInfoAlways
			g_application.m_guiMusicOverlay.OnAction(action);
			m_gWindowManager.PreviousWindow();
			break;
	}
	CGUIWindow::OnAction(action);
}

bool CGUIWindowVisualisation::OnMessage(CGUIMessage& message)
{
	switch ( message.GetMessage() )
	{
		case GUI_MSG_WINDOW_DEINIT:
		{
			CSingleLock lock(m_critSection);
			if (g_application.m_pPlayer)
				g_application.m_pPlayer->UnRegisterAudioCallback();
			if (m_pVisualisation)
			{
				OutputDebugString("Visualisation::Stop()\n");
				m_pVisualisation->Stop();
				
				OutputDebugString("delete Visualisation()\n");
				delete m_pVisualisation;
			}
			m_pVisualisation=NULL;
			m_bInitialized=false;
			ClearBuffers();

			// remove z-buffer
			RESOLUTION res = g_graphicsContext.GetVideoResolution();
			g_graphicsContext.SetVideoResolution(res, FALSE);
		}
		break;

		case GUI_MSG_WINDOW_INIT:
		{
			CGUIWindow::OnMessage(message);
			CSingleLock lock(m_critSection);
			if (m_pVisualisation)
			{
				m_pVisualisation->Stop();
				delete m_pVisualisation;
			}
			m_pVisualisation=NULL;
			if (g_application.m_pPlayer)
				g_application.m_pPlayer->UnRegisterAudioCallback();


			m_bInitialized=false;
			CVisualisationFactory factory;
			CStdString strVisz;
			OutputDebugString("Load Visualisation\n");
			strVisz.Format("Q:\\visualisations\\%s", g_stSettings.szDefaultVisualisation);
			m_pVisualisation=factory.LoadVisualisation(strVisz.c_str());
			if (m_pVisualisation) 
			{
				OutputDebugString("Visualisation::Create()\n");
				m_pVisualisation->Create();
				if (g_application.m_pPlayer)
					g_application.m_pPlayer->RegisterAudioCallback(this);
				
				// Create new audio buffers
				CreateBuffers();
			}

			// setup a z-buffer
			RESOLUTION res = g_graphicsContext.GetVideoResolution();
			g_graphicsContext.SetVideoResolution(res, TRUE);
			return true;
		}
	}
	return CGUIWindow::OnMessage(message);
}

void CGUIWindowVisualisation::OnMouse()
{
	if (g_Mouse.bClick[MOUSE_RIGHT_BUTTON])
	{	// no control found to absorb this click - go back to GUI
		CAction action;
		action.wID = ACTION_SHOW_GUI;
		OnAction(action);
		return;
	}
	if (g_Mouse.bClick[MOUSE_LEFT_BUTTON])
	{	// no control found to absorb this click - toggle the track INFO
		CAction action;
		action.wID = ACTION_SHOW_INFO;
		OnAction(action);
	}
}

void CGUIWindowVisualisation::Render()
{
  g_application.ResetScreenSaver();
	CSingleLock lock(m_critSection);
	if (m_pVisualisation)
	{
		if (m_bInitialized)
		{
			try
			{
				m_pVisualisation->Render();
			}
			catch(...)
			{
				OutputDebugString("ohoh\n");
			}
			return;
		}
	}
	CGUIWindow::Render();
}


void CGUIWindowVisualisation::OnInitialize(int iChannels, int iSamplesPerSec, int iBitsPerSample)
{
	CSingleLock lock(m_critSection);
	if (!m_pVisualisation) 
		return;

	m_bInitialized   = true;
	m_iChannels			 = iChannels;
	m_iSamplesPerSec = iSamplesPerSec;
	m_iBitsPerSample = iBitsPerSample;

	// Start the visualisation (this loads settings etc.)
  CStdString strFile=CUtil::GetFileName(g_application.CurrentFile());
	OutputDebugString("Visualisation::Start()\n");
	m_pVisualisation->Start(m_iChannels, m_iSamplesPerSec, m_iBitsPerSample,strFile);
	m_bInitialized=true;

}

void CGUIWindowVisualisation::OnAudioData(const unsigned char* pAudioData, int iAudioDataLength)
{
	if (!m_pVisualisation) 
		return;
	if (!m_bInitialized) return;

	CSingleLock lock(m_critSection);

	// Save our audio data in the buffers
	auto_ptr<CAudioBuffer> pBuffer ( new CAudioBuffer(2*AUDIO_BUFFER_SIZE) );
	pBuffer->Set(pAudioData,iAudioDataLength,m_iBitsPerSample);
	m_vecBuffers.push_back( pBuffer.release() );

	if ( (int)m_vecBuffers.size() < m_iNumBuffers) return;

	auto_ptr<CAudioBuffer> ptrAudioBuffer ( m_vecBuffers.front() );
	m_vecBuffers.pop_front();
	// Fourier transform the data if the vis wants it...
	if (m_bWantsFreq)
	{
		// Convert to floats
		const short* psAudioData=ptrAudioBuffer->Get();
		for (int i=0; i<2*AUDIO_BUFFER_SIZE; i++)
		{
			m_fFreq[i] = (float)psAudioData[i];
		}

		// FFT the data
		twochanwithwindow(m_fFreq, AUDIO_BUFFER_SIZE);

		// Normalize the data
		float fMinData = (float)AUDIO_BUFFER_SIZE*AUDIO_BUFFER_SIZE*3/8*0.5*0.5;	// 3/8 for the Hann window, 0.5 as minimum amplitude
		for (int i=0; i<AUDIO_BUFFER_SIZE+2; i++)
		{
			m_fFreq[i] /= fMinData;
		}

		// Transfer data to our visualisation
		try
		{
			m_pVisualisation->AudioData(ptrAudioBuffer->Get(), AUDIO_BUFFER_SIZE, m_fFreq, AUDIO_BUFFER_SIZE);
		}
		catch(...)
		{
		}
	}
	else
	{	// Transfer data to our visualisation
		try
		{
			m_pVisualisation->AudioData(ptrAudioBuffer->Get(), AUDIO_BUFFER_SIZE, NULL, 0);
		}
		catch(...)
		{
		}	}
	
	return;
}

void CGUIWindowVisualisation::CreateBuffers()
{
	CSingleLock lock(m_critSection);
	ClearBuffers();

	// Get the number of buffers from the current vis
	VIS_INFO info;
	m_pVisualisation->GetInfo(&info);
	m_iNumBuffers = info.iSyncDelay + 1;
	m_bWantsFreq  = info.bWantsFreq;
	if (m_iNumBuffers > MAX_AUDIO_BUFFERS)
		m_iNumBuffers = MAX_AUDIO_BUFFERS;
	
	if (m_iNumBuffers < 1)
		m_iNumBuffers = 1;
}


void CGUIWindowVisualisation::ClearBuffers()
{
	CSingleLock lock(m_critSection);
	m_bWantsFreq  = false;
	m_iNumBuffers = 0;
	
	while (m_vecBuffers.size() > 0)
	{
		CAudioBuffer* pAudioBuffer = m_vecBuffers.front();
		delete pAudioBuffer;
		m_vecBuffers.pop_front();
	}
	for (int j=0; j<AUDIO_BUFFER_SIZE*2; j++)
	{
		m_fFreq[j] = 0.0f;
	}
}

void CGUIWindowVisualisation::FreeResources()
{
	// Save changed settings from music OSD
	g_settings.Save();
	CGUIWindow::FreeResources();
}
