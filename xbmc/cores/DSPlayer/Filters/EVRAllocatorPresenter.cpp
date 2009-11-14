#include "EVRAllocatorPresenter.h"
#include "utils/log.h"
CEVRAllocatorPresenter::CEVRAllocatorPresenter(HRESULT& hr, HWND wnd, CStdString &_Error,IDirect3D9* d3d, IDirect3DDevice9* d3dd)
: ISubPicAllocatorPresenterImpl(wnd,hr, &_Error)
{
}

CEVRAllocatorPresenter::~CEVRAllocatorPresenter()
{
}

STDMETHODIMP CEVRAllocatorPresenter::ProcessMessage(MFVP_MESSAGE_TYPE eMessage, ULONG_PTR ulParam)
{
	HRESULT						hr = S_OK;

	switch (eMessage)
	{
	case MFVP_MESSAGE_BEGINSTREAMING :			// The EVR switched from stopped to paused. The presenter should allocate resources
		ResetStats();		
		CLog::Log(LOGDEBUG,"EVR: MFVP_MESSAGE_BEGINSTREAMING");
		break;

	case MFVP_MESSAGE_CANCELSTEP :				// Cancels a frame step
		CLog::Log(LOGDEBUG,"EVR: MFVP_MESSAGE_CANCELSTEP");
		CompleteFrameStep (true);
		break;

	case MFVP_MESSAGE_ENDOFSTREAM :				// All input streams have ended. 
		CLog::Log(LOGDEBUG,"EVR: MFVP_MESSAGE_ENDOFSTREAM");
		m_bPendingMediaFinished = true;
		break;

	case MFVP_MESSAGE_ENDSTREAMING :			// The EVR switched from running or paused to stopped. The presenter should free resources
		CLog::Log(LOGDEBUG,"EVR: MFVP_MESSAGE_ENDSTREAMING");
		break;

	case MFVP_MESSAGE_FLUSH :					// The presenter should discard any pending samples
		SetEvent(m_hEvtFlush);
		m_bEvtFlush = true;
		CLog::Log(LOGDEBUG,"EVR: MFVP_MESSAGE_FLUSH");
		while (WaitForSingleObject(m_hEvtFlush, 1) == WAIT_OBJECT_0);
		break;

	case MFVP_MESSAGE_INVALIDATEMEDIATYPE :		// The mixer's output format has changed. The EVR will initiate format negotiation, as described previously
		/*
			1) The EVR sets the media type on the reference stream.
			2) The EVR calls IMFVideoPresenter::ProcessMessage on the presenter with the MFVP_MESSAGE_INVALIDATEMEDIATYPE message.
			3) The presenter sets the media type on the mixer's output stream.
			4) The EVR sets the media type on the substreams.
		*/
		m_bPendingRenegotiate = true;
		while (*((volatile bool *)&m_bPendingRenegotiate))
			Sleep(1);
		break;

	case MFVP_MESSAGE_PROCESSINPUTNOTIFY :		// One input stream on the mixer has received a new sample
//		GetImageFromMixer();
		break;

	case MFVP_MESSAGE_STEP :					// Requests a frame step.
		CLog::Log(LOGDEBUG,"EVR: MFVP_MESSAGE_STEP");
		m_nStepCount = ulParam;
		hr = S_OK;
		break;

	default :
		ASSERT (FALSE);
		break;
	}
	return hr;
}

STDMETHODIMP CEVRAllocatorPresenter::GetCurrentMediaType(__deref_out  IMFVideoMediaType **ppMediaType)
{
    HRESULT hr = S_OK;
    CAutoLock lock(this);  // Hold the critical section.

    CheckPointer (ppMediaType, E_POINTER);
    CheckHR (CheckShutdown());

    if (m_pMediaType == NULL)
        CheckHR(MF_E_NOT_INITIALIZED);

    CheckHR(m_pMediaType->QueryInterface( __uuidof(IMFVideoMediaType), (void**)&ppMediaType));

    return hr;
}

void CEVRAllocatorPresenter::ResetStats()
{
	m_pcFrames			= 0;
	m_nDroppedUpdate	= 0;
	m_pcFramesDrawn		= 0;
	m_piAvg				= 0;
	m_piDev				= 0;
}

void CEVRAllocatorPresenter::CompleteFrameStep(bool bCancel)
{
}