
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

void CAudioBuffer::Set(const short* psBuffer, int iSize)
{
	for (int i=0; i < iSize; ++i)
	{
		if (i < m_iLen) m_pBuffer[i] = psBuffer[i];
	}
	for (i=iSize; i < m_iLen;++i) m_pBuffer[i] = 0;
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
		case ACTION_SHOW_GUI:
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
			return true;
		}
	}
	return CGUIWindow::OnMessage(message);
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
				Sleep(16);
        

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
	
	// Convert data to 16 bit shorts
	const short *sAudioData = (const short *)pAudioData;
	iAudioDataLength/=2;

	// Save our audio data in the buffers
	auto_ptr<CAudioBuffer> pBuffer ( new CAudioBuffer(2*AUDIO_BUFFER_SIZE) );
	pBuffer->Set(sAudioData,iAudioDataLength);
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