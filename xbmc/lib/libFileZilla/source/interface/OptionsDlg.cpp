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

/////////////////////////////////////////////////////////////////////////////
// Dialogfeld COptionsDlg 

COptionsDlg::COptionsDlg(COptions *pInterfaceOptions)
{
	ASSERT(pInterfaceOptions);
	m_pInterfaceOptions = pInterfaceOptions;

	//Add options pages
	AddPage(m_OptionsGeneralPage, IDS_OPTIONSPAGE_GENERAL);
	  AddPage(m_OptionsGeneralWelcomemessagePage, IDS_OPTIONSPAGE_GENERAL_WELCOMEMESSAGE, &m_OptionsGeneralPage);
	AddPage(m_OptionsSecurityPage, IDS_OPTIONSPAGE_SECURITY);
	AddPage(m_OptionsMiscPage, IDS_OPTIONSPAGE_MISC);
	AddPage(m_OptionsAdminInterfacePage, IDS_OPTIONSPAGE_ADMININTERFACE);
	AddPage(m_OptionsLoggingPage, IDS_OPTIONSPAGE_LOGGING);
	AddPage(m_OptionsGSSPage, IDS_OPTIONSPAGE_GSS);
	AddPage(m_OptionsSpeedLimitPage, IDS_OPTIONSPAGE_SPEEDLIMIT);
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
	SetTitle("FileZilla Server Options");
	SetConstantText("FileZilla Server");

	m_OptionsGeneralPage.m_Port.Format("%d",GetOptionVal(OPTION_SERVERPORT));	
	m_OptionsGeneralPage.m_Threadnum.Format("%d",GetOptionVal(OPTION_THREADNUM));	
	m_OptionsGeneralPage.m_MaxUsers.Format("%d",GetOptionVal(OPTION_MAXUSERS));	
	m_OptionsGeneralPage.m_Timeout.Format("%d",GetOptionVal(OPTION_TIMEOUT));	
	m_OptionsGeneralPage.m_NoTransferTimeout.Format("%d",GetOptionVal(OPTION_NOTRANSFERTIMEOUT));
	m_OptionsGeneralPage.m_LoginTimeout.Format("%d",GetOptionVal(OPTION_LOGINTIMEOUT));
	
	m_OptionsGeneralWelcomemessagePage.m_WelcomeMessage=GetOption(OPTION_WELCOMEMESSAGE);
	
	m_OptionsSecurityPage.m_bInFxp=!GetOptionVal(OPTION_INFXP);
	m_OptionsSecurityPage.m_bOutFxp=!GetOptionVal(OPTION_OUTFXP);
	m_OptionsSecurityPage.m_bInFxpStrict=GetOptionVal(OPTION_NOINFXPSTRICT);
	m_OptionsSecurityPage.m_bOutFxpStrict=GetOptionVal(OPTION_NOOUTFXPSTRICT);

	m_OptionsMiscPage.m_bDontShowPass=GetOptionVal(OPTION_LOGSHOWPASS)?FALSE:TRUE;
	m_OptionsMiscPage.m_bStartMinimized=m_pInterfaceOptions->GetOptionVal(IOPTION_STARTMINIMIZED);
	m_OptionsMiscPage.m_bEnableCustomPASV=GetOptionVal(OPTION_CUSTOMPASVENABLE)?TRUE:FALSE;
	m_OptionsMiscPage.m_sCustomPASVString=GetOption(OPTION_CUSTOMPASVIP);
	m_OptionsMiscPage.m_CustomPasvMinPort.Format("%d", GetOptionVal(OPTION_CUSTOMPASVMINPORT));
	m_OptionsMiscPage.m_CustomPasvMaxPort.Format("%d", GetOptionVal(OPTION_CUSTOMPASVMAXPORT));
	m_OptionsMiscPage.m_TransferBufferSize.Format("%d", GetOptionVal(OPTION_BUFFERSIZE));

	m_OptionsAdminInterfacePage.m_Port.Format(_T("%d"), GetOptionVal(OPTION_ADMINPORT));
	m_OptionsAdminInterfacePage.m_IpBindings = GetOption(OPTION_ADMINIPBINDINGS);
	m_OptionsAdminInterfacePage.m_IpAddresses = GetOption(OPTION_ADMINIPADDRESSES);
	m_OptionsAdminInterfacePage.m_NewPass = GetOption(OPTION_ADMINPASS);

	m_OptionsLoggingPage.m_bEnable = GetOptionVal(OPTION_ENABLELOGGING);
	
	
	int nLimit = GetOptionVal(OPTION_LOGLIMITSIZE);
	m_OptionsLoggingPage.m_bLimit = nLimit ? TRUE : FALSE;
	if (nLimit)
		m_OptionsLoggingPage.m_LimitSize.Format("%d", nLimit);

	m_OptionsLoggingPage.m_nLogtype = GetOptionVal(OPTION_LOGTYPE);

	int nDelete = GetOptionVal(OPTION_LOGDELETETIME);
	m_OptionsLoggingPage.m_bDelete = nDelete ? TRUE : FALSE;
	if (nDelete)
		m_OptionsLoggingPage.m_DeleteTime.Format("%d", nDelete);


	m_OptionsGSSPage.m_bUseGSS = GetOptionVal(OPTION_USEGSS);
	m_OptionsGSSPage.m_bPromptPassword = GetOptionVal(OPTION_GSSPROMPTPASSWORD);

	m_OptionsSpeedLimitPage.m_DownloadSpeedLimitType = GetOptionVal(OPTION_DOWNLOADSPEEDLIMITTYPE);
	m_OptionsSpeedLimitPage.m_UploadSpeedLimitType = GetOptionVal(OPTION_UPLOADSPEEDLIMITTYPE);
	m_OptionsSpeedLimitPage.m_DownloadValue = GetOptionVal(OPTION_DOWNLOADSPEEDLIMIT);
	m_OptionsSpeedLimitPage.m_UploadValue = GetOptionVal(OPTION_UPLOADSPEEDLIMIT);

	if (DoModal()!=IDOK)
		return FALSE;

	SetOption(OPTION_SERVERPORT, atoi(m_OptionsGeneralPage.m_Port));
	SetOption(OPTION_THREADNUM, atoi(m_OptionsGeneralPage.m_Threadnum));
	SetOption(OPTION_MAXUSERS, atoi(m_OptionsGeneralPage.m_MaxUsers));
	SetOption(OPTION_TIMEOUT, atoi(m_OptionsGeneralPage.m_Timeout));
	SetOption(OPTION_NOTRANSFERTIMEOUT, atoi(m_OptionsGeneralPage.m_NoTransferTimeout));
	SetOption(OPTION_LOGINTIMEOUT, atoi(m_OptionsGeneralPage.m_LoginTimeout));

	CString msg=m_OptionsGeneralWelcomemessagePage.m_WelcomeMessage;
	std::list<CString> msgLines;
	int oldpos=0;
	msg.Replace("\r\n", "\n");
	int pos=msg.Find("\n");
	CString line;
	while (pos!=-1)
	{
		if (pos)
		{
			line = msg.Mid(oldpos, pos-oldpos);
			line=line.Left(70);
			line.TrimRight(" ");
			if (msgLines.size() || line!="")
				msgLines.push_back(line);
		}
		oldpos=pos+1;
		pos=msg.Find("\n", oldpos);
	}
	line=msg.Mid(oldpos);
	if (line!="")
		msgLines.push_back(line);
	msg="";
	for (std::list<CString>::iterator iter=msgLines.begin(); iter!=msgLines.end(); iter++)
		msg+=*iter+"\r\n";
	msg.TrimRight("\r\n");

	SetOption(OPTION_WELCOMEMESSAGE, msg);

	SetOption(OPTION_INFXP, !m_OptionsSecurityPage.m_bInFxp);
	SetOption(OPTION_OUTFXP, !m_OptionsSecurityPage.m_bOutFxp);
	SetOption(OPTION_NOINFXPSTRICT, m_OptionsSecurityPage.m_bInFxpStrict);
	SetOption(OPTION_NOOUTFXPSTRICT, m_OptionsSecurityPage.m_bOutFxpStrict);

	SetOption(OPTION_LOGSHOWPASS, m_OptionsMiscPage.m_bDontShowPass?0:1);
	m_pInterfaceOptions->SetOption(IOPTION_STARTMINIMIZED, m_OptionsMiscPage.m_bStartMinimized);
	SetOption(OPTION_CUSTOMPASVENABLE,m_OptionsMiscPage.m_bEnableCustomPASV?1:0);
	SetOption(OPTION_CUSTOMPASVIP, m_OptionsMiscPage.m_sCustomPASVString);
	SetOption(OPTION_CUSTOMPASVMINPORT, _ttoi(m_OptionsMiscPage.m_CustomPasvMinPort));
	SetOption(OPTION_CUSTOMPASVMAXPORT, _ttoi(m_OptionsMiscPage.m_CustomPasvMaxPort));
	SetOption(OPTION_BUFFERSIZE, _ttoi(m_OptionsMiscPage.m_TransferBufferSize));

	SetOption(OPTION_ADMINPORT, _ttoi(m_OptionsAdminInterfacePage.m_Port));
	SetOption(OPTION_ADMINIPBINDINGS, m_OptionsAdminInterfacePage.m_IpBindingsResult);
	SetOption(OPTION_ADMINIPADDRESSES, m_OptionsAdminInterfacePage.m_IpAddresses);
	if (m_OptionsAdminInterfacePage.m_bChangePass)
		SetOption(OPTION_ADMINPASS, m_OptionsAdminInterfacePage.m_NewPass);

	SetOption(OPTION_ENABLELOGGING, m_OptionsLoggingPage.m_bEnable);
	SetOption(OPTION_LOGLIMITSIZE, m_OptionsLoggingPage.m_bLimit ? _ttoi(m_OptionsLoggingPage.m_LimitSize) : 0);
	SetOption(OPTION_LOGTYPE, m_OptionsLoggingPage.m_nLogtype);
	SetOption(OPTION_LOGDELETETIME, m_OptionsLoggingPage.m_bDelete ? _ttoi(m_OptionsLoggingPage.m_DeleteTime) : 0);

	SetOption(OPTION_USEGSS, m_OptionsGSSPage.m_bUseGSS);
	SetOption(OPTION_GSSPROMPTPASSWORD, m_OptionsGSSPage.m_bPromptPassword);

	SetOption(OPTION_DOWNLOADSPEEDLIMITTYPE, m_OptionsSpeedLimitPage.m_DownloadSpeedLimitType);
	SetOption(OPTION_DOWNLOADSPEEDLIMIT, m_OptionsSpeedLimitPage.m_DownloadValue);
	SetOption(OPTION_UPLOADSPEEDLIMITTYPE, m_OptionsSpeedLimitPage.m_UploadSpeedLimitType);
	SetOption(OPTION_UPLOADSPEEDLIMIT, m_OptionsSpeedLimitPage.m_UploadValue);
	
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
	int port=atoi(m_OptionsGeneralPage.m_Port);
	if (port<1 || port>65535)
	{
		ShowPage(&m_OptionsGeneralPage);
		m_OptionsGeneralPage.GetDlgItem(IDC_PORT)->SetFocus();
		AfxMessageBox("Please enter a value between 1 and 65535 for the port!");
		return;
	}
	int threadnum=atoi(m_OptionsGeneralPage.m_Threadnum);
	if (threadnum<1 || threadnum>50)
	{
		ShowPage(&m_OptionsGeneralPage);
		m_OptionsGeneralPage.GetDlgItem(IDC_THREADNUM)->SetFocus();
		AfxMessageBox("Please enter a value between 1 and 50 for the number of Threads!");
		return;
	}
	
	if (m_OptionsMiscPage.m_bEnableCustomPASV)
	{
		//Ensure a valid IP address has been entered
		CString ip=m_OptionsMiscPage.m_sCustomPASVString;
		BOOL bError=FALSE;
		if (ip=="")
			bError=TRUE;
		else
		{
			int oldpos=0;
			int count=0;
			if (ip[0]=='.' || ip.Find("..")!=-1)
				bError=TRUE;
			while (count<3 && !bError)
			{
				int pos=ip.Find(".", oldpos);
				if (pos==-1 || pos>(oldpos+3) )
					bError=TRUE;
				else
				{
					count++;
					oldpos=pos+1;
				}
			}
			if (!bError)
				bError=ip.GetLength()<=oldpos;
		}
		
		if (bError)
		{
			ShowPage(&m_OptionsMiscPage);
			m_OptionsMiscPage.GetDlgItem(IDC_OPTIONS_MISC_CUSTOM_PASV_IP)->SetFocus();
			AfxMessageBox("Please enter a valid IP address!");
			return;
		}

		int nPortMin=atoi(m_OptionsMiscPage.m_CustomPasvMinPort);
		int nPortMax=atoi(m_OptionsMiscPage.m_CustomPasvMaxPort);
		if (nPortMin<1 || nPortMin>65535 || nPortMax<1 || nPortMax>65535 || nPortMin>=nPortMax)
		{
			ShowPage(&m_OptionsMiscPage);
			m_OptionsMiscPage.GetDlgItem(IDC_OPTIONS_MISC_CUSTOM_PASV_MIN_PORT)->SetFocus();
			AfxMessageBox("The port values have to be in the range from 1 to 65535. Also, the first value has to be lower than the second one.");			
			return;
		}
	}

	if (_ttoi(m_OptionsAdminInterfacePage.m_Port) < 1 || _ttoi(m_OptionsAdminInterfacePage.m_Port) > 65535)
	{
		ShowPage(&m_OptionsAdminInterfacePage);
		m_OptionsAdminInterfacePage.GetDlgItem(IDC_OPTIONS_ADMININTERFACE_PORT)->SetFocus();
		AfxMessageBox(_T("The port for the admin interface has to be in the range from 1 to 65535."));
		return;
	}

	CString bindIPs = m_OptionsAdminInterfacePage.m_IpBindings;
	CString sub;
	std::list<CString> ipBindList;
	for (int i = 0; i<bindIPs.GetLength(); i++)
	{
		char cur = bindIPs[i];
		if ((cur<'0' || cur>'9') && cur!='.')
		{
			if (sub=="" && cur=='*')
			{
				ipBindList.clear();
				ipBindList.push_back("*");
				break;
			}

			if (sub != "")
			{
				//Parse IP
				SOCKADDR_IN sockAddr;
				memset(&sockAddr,0,sizeof(sockAddr));
			
				sockAddr.sin_family = AF_INET;
				sockAddr.sin_addr.s_addr = inet_addr(sub);
			
				if (sockAddr.sin_addr.s_addr != INADDR_NONE)
				{
					sub = inet_ntoa(sockAddr.sin_addr);
					for (std::list<CString>::iterator iter = ipBindList.begin(); iter!=ipBindList.end(); iter++)
						if (*iter==sub)
							break;
					if (iter == ipBindList.end())
						ipBindList.push_back(sub);
				}
				sub = "";
			}
		}
		else
			sub += cur;
	}
	if (sub != "")
	{
		//Parse IP
		SOCKADDR_IN sockAddr;
		memset(&sockAddr,0,sizeof(sockAddr));
		
		sockAddr.sin_family = AF_INET;
		sockAddr.sin_addr.s_addr = inet_addr(sub);
		
		if (sockAddr.sin_addr.s_addr != INADDR_NONE)
		{
			sub = inet_ntoa(sockAddr.sin_addr);
			for (std::list<CString>::iterator iter = ipBindList.begin(); iter!=ipBindList.end(); iter++)
				if (*iter==sub)
					break;
			if (iter == ipBindList.end())
				ipBindList.push_back(sub);
		}
		sub = "";
	}
	bindIPs = "";
	for (std::list<CString>::iterator iter = ipBindList.begin(); iter!=ipBindList.end(); iter++)
		if (*iter != "127.0.0.1")
			bindIPs += *iter + " ";

	bindIPs.TrimRight(" ");
					
	m_OptionsAdminInterfacePage.m_IpBindingsResult = bindIPs;

	if (m_OptionsAdminInterfacePage.m_bChangePass && m_OptionsAdminInterfacePage.m_IpAddresses!="")
	{
		if (m_OptionsAdminInterfacePage.m_NewPass.GetLength() < 6)
		{
			ShowPage(&m_OptionsAdminInterfacePage);
			m_OptionsAdminInterfacePage.GetDlgItem(IDC_OPTIONS_ADMININTERFACE_NEWPASS)->SetFocus();
			AfxMessageBox(_T("The admin password has to be at least 6 characters long,"));
			return;
		}
		if (m_OptionsAdminInterfacePage.m_NewPass != m_OptionsAdminInterfacePage.m_NewPass2)
		{
			ShowPage(&m_OptionsAdminInterfacePage);
			m_OptionsAdminInterfacePage.GetDlgItem(IDC_OPTIONS_ADMININTERFACE_NEWPASS)->SetFocus();
			AfxMessageBox(_T("Admin passwords do not match."));
			return;
		}
	}
	
	CSAPrefsDialog::OnOK();
}

CSAPrefsSubDlg* COptionsDlg::GetCurPage()
{
	int iPage=m_iCurPage;
	// show the new one
	if ((iPage >= 0) && (iPage < m_pages.size()))
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
		if ((p-pData)>=dwDataLength)
			return FALSE;
		int nType = *p++;
		if (!nType)
		{
			if ((p-pData+2) >= dwDataLength)
				return 2;
			int len= *p * 256 + p[1];
			p+=2;
			if ((p-pData+len)>dwDataLength)
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
			if ((p-pData+8)>dwDataLength)
				return FALSE;
			m_OptionsCache[i].nType = 1;
			memcpy(&m_OptionsCache[i].value, p, 8);
			p+=8;
		}
		else
			return FALSE;
	}

	if ((p-pData+2)>dwDataLength)
		return FALSE;
	num = *p++ << 8;
	num |= *p++;
	for (i=0; i<num; i++)
	{
		CSpeedLimit limit;
		p = limit.ParseBuffer(p, dwDataLength - (p - pData));
		if (!p)
			return FALSE;
		m_OptionsSpeedLimitPage.m_DownloadSpeedLimits.push_back(limit);
	}
	
	if ((p-pData+2)>dwDataLength)
		return FALSE;
	num = *p++ << 8;
	num |= *p++;
	for (i=0; i<num; i++)
	{
		CSpeedLimit limit;
		p = limit.ParseBuffer(p, dwDataLength - (p - pData));
		if (!p)
			return FALSE;
		m_OptionsSpeedLimitPage.m_UploadSpeedLimits.push_back(limit);
	}
	return TRUE;
}

BOOL COptionsDlg::GetAsCommand(char **pBuffer, DWORD *nBufferLength)
{
	DWORD len = 2;
	for (int i=0; i<OPTIONS_NUM; i++)
	{
		len+=1;
		if (!m_Options[i].nType)
			len+=GetOption(i+1).GetLength()+2;
		else
			len+=8;
	}
	len += 4; //Number of rules
	SPEEDLIMITSLIST::const_iterator iter;
	for (iter = m_OptionsSpeedLimitPage.m_DownloadSpeedLimits.begin(); iter != m_OptionsSpeedLimitPage.m_DownloadSpeedLimits.end(); iter++)
		len += iter->GetRequiredBufferLen();
	for (iter = m_OptionsSpeedLimitPage.m_UploadSpeedLimits.begin(); iter != m_OptionsSpeedLimitPage.m_UploadSpeedLimits.end(); iter++)
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

	*p++ = m_OptionsSpeedLimitPage.m_DownloadSpeedLimits.size() >> 8;
	*p++ = m_OptionsSpeedLimitPage.m_DownloadSpeedLimits.size() % 256;
	for (iter = m_OptionsSpeedLimitPage.m_DownloadSpeedLimits.begin(); iter != m_OptionsSpeedLimitPage.m_DownloadSpeedLimits.end(); iter++)
		p = iter->FillBuffer(p);
	
	*p++ = m_OptionsSpeedLimitPage.m_UploadSpeedLimits.size() >> 8;
	*p++ = m_OptionsSpeedLimitPage.m_UploadSpeedLimits.size() % 256;
	for (iter = m_OptionsSpeedLimitPage.m_UploadSpeedLimits.begin(); iter != m_OptionsSpeedLimitPage.m_UploadSpeedLimits.end(); iter++)
		p = iter->FillBuffer(p);

	*nBufferLength = len;

	return TRUE;
}
