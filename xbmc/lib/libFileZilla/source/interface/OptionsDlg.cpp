// FileZilla Server - a Windows ftp server

// Copyright (C) 2002 - Tim Kosse <tim.kosse@gmx.de>

// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

// Options.cpp: Implementierungsdatei
//

#include "stdafx.h"
#include "version.h"
#include "filezilla server.h"
#include "OptionsDlg.h"
#include "..\OptionTypes.h"
#include "Options.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#include "OptionsPage.h"
#include "OptionsGeneralPage.h"
#include "OptionsGeneralWelcomemessagePage.h"
#include "OptionsPasvPage.h"
#include "OptionsSecurityPage.h"
#include "OptionsMiscPage.h"
#include "OptionsAdminInterfacePage.h"
#include "OptionsLoggingPage.h"
#include "OptionsGSSPage.h"
#include "OptionsSpeedLimitPage.h"


/////////////////////////////////////////////////////////////////////////////
// Dialogfeld COptionsDlg 

COptionsDlg::COptionsDlg(COptions *pInterfaceOptions)
{
	ASSERT(pInterfaceOptions);
	m_pInterfaceOptions = pInterfaceOptions;

	//Add options pages
	COptionsPage *page;

	page = new COptionsGeneralPage(this);
	AddPage(*page, IDS_OPTIONSPAGE_GENERAL);
	m_PageList.push_back(page);

		page = new COptionsGeneralWelcomemessagePage(this);
		AddPage(*page, IDS_OPTIONSPAGE_GENERAL_WELCOMEMESSAGE, m_PageList.back());
		m_PageList.push_back(page);

	page = new COptionsPasvPage(this);
	AddPage(*page, IDS_OPTIONSPAGE_PASV);
	m_PageList.push_back(page);

	page = new COptionsSecurityPage(this);
	AddPage(*page, IDS_OPTIONSPAGE_SECURITY);
	m_PageList.push_back(page);

	page = new COptionsMiscPage(this);
	AddPage(*page, IDS_OPTIONSPAGE_MISC);
	m_PageList.push_back(page);

	page = new COptionsAdminInterfacePage(this);
	AddPage(*page, IDS_OPTIONSPAGE_ADMININTERFACE);
	m_PageList.push_back(page);

	page = new COptionsLoggingPage(this);
	AddPage(*page, IDS_OPTIONSPAGE_LOGGING);
	m_PageList.push_back(page);

	page = new COptionsGSSPage(this);
	AddPage(*page, IDS_OPTIONSPAGE_GSS);
	m_PageList.push_back(page);

	m_pOptionsSpeedLimitPage = new COptionsSpeedLimitPage(this);
	AddPage(*m_pOptionsSpeedLimitPage, IDS_OPTIONSPAGE_SPEEDLIMIT);
	m_PageList.push_back(m_pOptionsSpeedLimitPage);

}

COptionsDlg::~COptionsDlg()
{
}

void COptionsDlg::DoDataExchange(CDataExchange* pDX)
{
	CSAPrefsDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(COptionsDlg)
		// HINWEIS: Der Klassen-Assistent fügt hier DDX- und DDV-Aufrufe ein
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(COptionsDlg, CSAPrefsDialog)
	//{{AFX_MSG_MAP(COptionsDlg)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// Behandlungsroutinen für Nachrichten COptionsDlg 

BOOL COptionsDlg::Show()
{
	SetConstantText("FileZilla Server");

	std::list<COptionsPage *>::iterator iter;
	for (iter = m_PageList.begin(); iter != m_PageList.end(); iter++)
		(*iter)->LoadData();

	if (DoModal()!=IDOK)
		return FALSE;

	for (iter = m_PageList.begin(); iter != m_PageList.end(); iter++)
		(*iter)->SaveData();

	return TRUE;
}

BOOL COptionsDlg::OnInitDialog() 
{
	CSAPrefsDialog::OnInitDialog();
		
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX-Eigenschaftenseiten sollten FALSE zurückgeben
}

void COptionsDlg::SetOption(int nOptionID, int value)
{
	m_OptionsCache[nOptionID-1].nType = 1;
	m_OptionsCache[nOptionID-1].value = value;
}

void COptionsDlg::SetOption(int nOptionID, CString value)
{
	m_OptionsCache[nOptionID-1].nType = 0;
	m_OptionsCache[nOptionID-1].str = value;
}

CString COptionsDlg::GetOption(int nOptionID)
{
	ASSERT(nOptionID>0 && nOptionID<=OPTIONS_NUM);
	ASSERT(!m_Options[nOptionID-1].nType);

	return m_OptionsCache[nOptionID-1].str;
}

int COptionsDlg::GetOptionVal(int nOptionID)
{
	ASSERT(nOptionID>0 && nOptionID<=OPTIONS_NUM);
	ASSERT(m_Options[nOptionID-1].nType == 1);
	
	return m_OptionsCache[nOptionID-1].value;
}

void COptionsDlg::OnOK() 
{
	if (!UpdateData(true))
		return;
	if (!GetCurPage()->UpdateData(TRUE))
		return;

	for (std::list<COptionsPage *>::iterator iter = m_PageList.begin(); iter != m_PageList.end(); iter++)
		if (!(*iter)->IsDataValid())
			return;
		
	CSAPrefsDialog::OnOK();
}

CSAPrefsSubDlg* COptionsDlg::GetCurPage()
{
	int iPage = m_iCurPage;
	// show the new one
	if ((iPage >= 0) && (iPage < (int)m_pages.size()))
	{
		pageStruct *pPS = m_pages[iPage];
		ASSERT(pPS);

		if (pPS)
		{
			ASSERT(pPS->pDlg);
			if (pPS->pDlg)
			{
				return pPS->pDlg;
			}
		}
	}
	return NULL;
}

bool COptionsDlg::AddPage(CSAPrefsSubDlg &page, UINT nCaptionID, CSAPrefsSubDlg *pDlgParent /*= NULL*/)
{
	CString str;
	str.LoadString(nCaptionID);
	return CSAPrefsDialog::AddPage(page,str,pDlgParent);
}

bool COptionsDlg::IsNumeric(LPCTSTR str)
{
	if (!str)
		return false;
	LPCTSTR p=str;
	while(*p)
	{
		if (*p<'0' || *p>'9')
		{
			return false;
		}
		p++;
	}
	return true;
}

BOOL COptionsDlg::Init(unsigned char *pData, DWORD dwDataLength)
{
	unsigned char *p = pData;
	int i;
	int num = *p * 256 + p[1];
	p+=2;
	if (num!=OPTIONS_NUM)
		return FALSE;
	
	for (i=0; i<num; i++)
	{
		if (static_cast<DWORD>(p-pData) >= dwDataLength)
			return FALSE;
		int nType = *p++;
		if (!nType)
		{
			if (static_cast<DWORD>(p-pData+2) >= dwDataLength)
				return 2;
			int len= *p * 256 + p[1];
			p+=2;
			if (static_cast<DWORD>(p-pData+len)>dwDataLength)
				return FALSE;
			m_OptionsCache[i].nType = 0;
			char *pBuffer = m_OptionsCache[i].str.GetBuffer(len);
			memcpy(pBuffer, p, len);
			m_OptionsCache[i].str = p;
			m_OptionsCache[i].str.ReleaseBuffer(len);
			p+=len;
		}
		else if (nType == 1)
		{
			if (static_cast<DWORD>(p-pData+8)>dwDataLength)
				return FALSE;
			m_OptionsCache[i].nType = 1;
			memcpy(&m_OptionsCache[i].value, p, 8);
			p+=8;
		}
		else
			return FALSE;
	}

	if (static_cast<DWORD>(p-pData+2) > dwDataLength)
		return FALSE;
	num = *p++ << 8;
	num |= *p++;
	for (i=0; i<num; i++)
	{
		CSpeedLimit limit;
		p = limit.ParseBuffer(p, dwDataLength - (p - pData));
		if (!p)
			return FALSE;
		m_pOptionsSpeedLimitPage->m_DownloadSpeedLimits.push_back(limit);
	}
	
	if (static_cast<DWORD>(p-pData+2) > dwDataLength)
		return FALSE;
	num = *p++ << 8;
	num |= *p++;
	for (i=0; i<num; i++)
	{
		CSpeedLimit limit;
		p = limit.ParseBuffer(p, dwDataLength - (p - pData));
		if (!p)
			return FALSE;
		m_pOptionsSpeedLimitPage->m_UploadSpeedLimits.push_back(limit);
	}
	return TRUE;
}

BOOL COptionsDlg::GetAsCommand(char **pBuffer, DWORD *nBufferLength)
{
	DWORD len = 2;
	int i;
	for (i = 0; i < OPTIONS_NUM; i++)
	{
		len+=1;
		if (!m_Options[i].nType)
			len+=GetOption(i+1).GetLength()+2;
		else
			len+=8;
	}
	len += 4; //Number of rules
	SPEEDLIMITSLIST::const_iterator iter;
	for (iter = m_pOptionsSpeedLimitPage->m_DownloadSpeedLimits.begin(); iter != m_pOptionsSpeedLimitPage->m_DownloadSpeedLimits.end(); iter++)
		len += iter->GetRequiredBufferLen();
	for (iter = m_pOptionsSpeedLimitPage->m_UploadSpeedLimits.begin(); iter != m_pOptionsSpeedLimitPage->m_UploadSpeedLimits.end(); iter++)
		len += iter->GetRequiredBufferLen();

	*pBuffer = new char[len];
	char *p=*pBuffer;
	*p++ = OPTIONS_NUM/256;
	*p++ = OPTIONS_NUM%256;
	for (i=0; i<OPTIONS_NUM; i++)
	{
		*p++ = m_Options[i].nType;
		switch(m_Options[i].nType) {
		case 0:
			{
				CString str = GetOption(i+1);
				*p++ = str.GetLength() / 256;
				*p++ = str.GetLength() % 256;
				memcpy(p, str, str.GetLength());
				p+=str.GetLength();
			}
			break;
		case 1:
			{
				_int64 value = GetOptionVal(i+1);
				memcpy(p, &value, 8);
				p+=8;
			}
			break;
		default:
			ASSERT(FALSE);
		}
	}

	*p++ = m_pOptionsSpeedLimitPage->m_DownloadSpeedLimits.size() >> 8;
	*p++ = m_pOptionsSpeedLimitPage->m_DownloadSpeedLimits.size() % 256;
	for (iter = m_pOptionsSpeedLimitPage->m_DownloadSpeedLimits.begin(); iter != m_pOptionsSpeedLimitPage->m_DownloadSpeedLimits.end(); iter++)
		p = iter->FillBuffer(p);
	
	*p++ = m_pOptionsSpeedLimitPage->m_UploadSpeedLimits.size() >> 8;
	*p++ = m_pOptionsSpeedLimitPage->m_UploadSpeedLimits.size() % 256;
	for (iter = m_pOptionsSpeedLimitPage->m_UploadSpeedLimits.begin(); iter != m_pOptionsSpeedLimitPage->m_UploadSpeedLimits.end(); iter++)
		p = iter->FillBuffer(p);

	*nBufferLength = len;

	return TRUE;
}
