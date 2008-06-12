#include <stdio.h>
#include <streams.h>
#include "CoreAACAboutProp.h"
#include "resource.h"

#include "../faad2/include/faad.h"
#define FILTER_VERSION "1.0b4"

// ----------------------------------------------------------------------------

CUnknown *WINAPI CCoreAACAboutProp::CreateInstance(LPUNKNOWN punk, HRESULT *phr)
{
	CCoreAACAboutProp *pNewObject = new CCoreAACAboutProp(punk, phr);
	if (!pNewObject)
		*phr = E_OUTOFMEMORY;
	return pNewObject;
}

// ----------------------------------------------------------------------------

CCoreAACAboutProp::CCoreAACAboutProp(LPUNKNOWN pUnk, HRESULT *phr) :
	CBasePropertyPage(NAME("About"), pUnk, IDD_ABOUT, IDS_ABOUT)
{
		
}

// ----------------------------------------------------------------------------

CCoreAACAboutProp::~CCoreAACAboutProp()
{
	
}	

// ----------------------------------------------------------------------------

HRESULT CCoreAACAboutProp::OnActivate()
{		
	SetDlgItemText(m_hwnd, IDC_STATIC_VERSION, 
		"Version " FILTER_VERSION " - ("__DATE__", "__TIME__")");
	SetDlgItemText(m_hwnd, IDC_STATIC_FAAD_VERSION, 
		"FAAD2 " FAAD2_VERSION " - Freeware Advanced Audio Decoder");
	return S_OK;
}

// ----------------------------------------------------------------------------

BOOL CCoreAACAboutProp::OnReceiveMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	return CBasePropertyPage::OnReceiveMessage(hwnd, uMsg, wParam, lParam);		
}

// ----------------------------------------------------------------------------