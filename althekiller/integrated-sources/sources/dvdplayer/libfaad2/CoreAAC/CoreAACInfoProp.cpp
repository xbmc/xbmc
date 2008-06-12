#include <stdio.h>
#include <streams.h>
#include "ICoreAAC.h"
#include "CoreAACInfoProp.h"
#include "resource.h"

// ----------------------------------------------------------------------------

CUnknown *WINAPI CCoreAACInfoProp::CreateInstance(LPUNKNOWN punk, HRESULT *phr)
{
	CCoreAACInfoProp *pNewObject = new CCoreAACInfoProp(punk, phr);
	if (!pNewObject)
		*phr = E_OUTOFMEMORY;
	return pNewObject;
}

// ----------------------------------------------------------------------------

CCoreAACInfoProp::CCoreAACInfoProp(LPUNKNOWN pUnk, HRESULT *phr) :
	CBasePropertyPage(NAME("Info"), pUnk, IDD_DIALOG_INFO, IDS_INFO),
	m_pICoreAACDec(NULL),
	m_fWindowInActive(TRUE)
{
	
}

// ----------------------------------------------------------------------------

CCoreAACInfoProp::~CCoreAACInfoProp()
{
	
}

// ----------------------------------------------------------------------------

HRESULT CCoreAACInfoProp::OnConnect(IUnknown *pUnknown)
{
	if (pUnknown == NULL)
	{
		return E_POINTER;
	}
	
	ASSERT(m_pICoreAACDec == NULL);

	// Ask the filter for it's control interface		
	HRESULT hr = pUnknown->QueryInterface(IID_ICoreAACDec,reinterpret_cast<void**>(&m_pICoreAACDec));
	if(FAILED(hr))
	{
		return hr;
	}
	
	ASSERT(m_pICoreAACDec);

	m_pICoreAACDec->get_DownMatrix(&m_DownMatrix);

	return S_OK;
}

// ----------------------------------------------------------------------------

HRESULT CCoreAACInfoProp::OnDisconnect()
{
	// Release the interface
	if (m_pICoreAACDec == NULL) {
		return E_UNEXPECTED;
	}
	m_pICoreAACDec->Release();
	m_pICoreAACDec = NULL;
	return NOERROR;
}

// ----------------------------------------------------------------------------

HRESULT CCoreAACInfoProp::OnActivate()
{		
	char msgFormat[255];
	m_fWindowInActive = FALSE;
	
	char* profileName = NULL;
	m_pICoreAACDec->get_ProfileName(&profileName);
	SetDlgItemText(m_hwnd, IDC_LABEL_PROFILE, profileName);
	
	int sampleRate = 0;
	m_pICoreAACDec->get_SampleRate(&sampleRate);
	wsprintf(msgFormat, "%d Hz", sampleRate);
	SetDlgItemText(m_hwnd, IDC_LABEL_SAMPLERATE, msgFormat);
	
	int channels = 0;
	m_pICoreAACDec->get_Channels(&channels);
	wsprintf(msgFormat, "%d", channels);
	SetDlgItemText(m_hwnd, IDC_LABEL_CHANNELS, msgFormat);
	
	int bps = 0;
	m_pICoreAACDec->get_BitsPerSample(&bps);
	wsprintf(msgFormat, "%d", bps);
	SetDlgItemText(m_hwnd, IDC_LABEL_BPS, msgFormat);
	
	CheckDlgButton(m_hwnd,IDC_CHECK_DOWNMIX, m_DownMatrix ? BST_CHECKED : BST_UNCHECKED);

	RefreshDisplay(m_hwnd);
	SetTimer(m_hwnd,0,1000,NULL);
	
	return S_OK;
}

// ----------------------------------------------------------------------------

HRESULT CCoreAACInfoProp::OnDeactivate()
{
	KillTimer(m_hwnd,0);
	m_fWindowInActive = TRUE;
	return S_OK;
}

// ----------------------------------------------------------------------------

HRESULT CCoreAACInfoProp::OnApplyChanges(void)
{
	m_pICoreAACDec->set_DownMatrix(m_DownMatrix);
	return S_OK;
}

// ----------------------------------------------------------------------------

BOOL CCoreAACInfoProp::OnReceiveMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if(m_fWindowInActive)
		return FALSE;
	
	switch(uMsg)
	{
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDC_CHECK_DOWNMIX:
			m_DownMatrix = (IsDlgButtonChecked(hwnd,IDC_CHECK_DOWNMIX) == BST_CHECKED) ? true : false;
			SetDirty();
			break;
		}
		break;
		
		case WM_TIMER:
			RefreshDisplay(hwnd);
			return (LRESULT) 1;
	}
	
	return CBasePropertyPage::OnReceiveMessage(hwnd, uMsg, wParam, lParam);			
}

// ----------------------------------------------------------------------------

void CCoreAACInfoProp::RefreshDisplay(HWND hwnd)
{
	static char msgFormat[16];
	
	int bitrate = 0;		
	m_pICoreAACDec->get_Bitrate(&bitrate);
	if(bitrate)
		wsprintf(msgFormat, "%d kbps", bitrate/1000);
	else
		wsprintf(msgFormat, "-");
	SetDlgItemText(hwnd, IDC_LABEL_BITRATE, msgFormat);
	
	unsigned int frames_decoded = 0;
	m_pICoreAACDec->get_FramesDecoded(&frames_decoded);
	if(frames_decoded)
		wsprintf(msgFormat, "%d", frames_decoded);
	else
		wsprintf(msgFormat, "-");		
	SetDlgItemText(hwnd, IDC_LABEL_FRAMES_DECODED, msgFormat);
	
}

// ----------------------------------------------------------------------------

void CCoreAACInfoProp::SetDirty()
{
    m_bDirty = TRUE;
    if (m_pPageSite)
        m_pPageSite->OnStatusChange(PROPPAGESTATUS_DIRTY);
}

// ----------------------------------------------------------------------------