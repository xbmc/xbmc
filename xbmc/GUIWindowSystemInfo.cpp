#include "guiwindowsysteminfo.h"
#include "settings.h"
#include "guiWindowManager.h"
#include "localizestrings.h"
#include "xbox/iosupport.h"
#include <ConIo.h>

extern char g_szTitleIP[32];
CGUIWindowSystemInfo::CGUIWindowSystemInfo(void)
:CGUIWindow(0)
{
}

CGUIWindowSystemInfo::~CGUIWindowSystemInfo(void)
{
}

void CGUIWindowSystemInfo::OnKey(const CKey& key)
{
  if (key.IsButton())
  {
    if ( key.GetButtonCode() == KEY_BUTTON_BACK  || key.GetButtonCode() == KEY_REMOTE_BACK)
    {
      m_gWindowManager.ActivateWindow(0); // back 2 home
      return;
    }
  }
  CGUIWindow::OnKey(key);
}

bool CGUIWindowSystemInfo::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
    case GUI_MSG_WINDOW_DEINIT:
    break;

    case GUI_MSG_WINDOW_INIT:
		{
			m_dwlastTime=0;
		}
		break;
	}
	return CGUIWindow::OnMessage(message);
}

void CGUIWindowSystemInfo::Render()
{
	GetValues();
	CGUIWindow::Render();
}

void  CGUIWindowSystemInfo::GetValues()
{
	if(timeGetTime() - m_dwlastTime >= 1000)
	{
		m_dwlastTime =timeGetTime();
		_outp(0xc004, (0x4c<<1)|0x01);
    _outp(0xc008, 0x00);
    _outpw(0xc000, _inpw(0xc000));
    _outp(0xc002, (0) ? 0x0b : 0x0a);
    while ((_inp(0xc000) & 8));
    mbtemp = _inpw(0xc006);

	_outp(0xc004, (0x4c<<1)|0x01);
    _outp(0xc008, 0x01);
    _outpw(0xc000, _inpw(0xc000));
    _outp(0xc002, (0) ? 0x0b : 0x0a);
    while ((_inp(0xc000) & 8));
    cputemp = _inpw(0xc006);

	_outp(0xc004, (0x4c<<1)|0x01);
    _outp(0xc008, 0x10);
    _outpw(0xc000, _inpw(0xc000));
    _outp(0xc002, (0) ? 0x0b : 0x0a);
    while ((_inp(0xc000) & 8));
    cpudec = _inpw(0xc006);

		if (cpudec<10)
			cpudec = cpudec * 100;
		if (cpudec<100)
			cpudec = cpudec *10; 
	}
	WCHAR CPUText[32];
	WCHAR GPUText[32];
	WCHAR wszText[1024];
	if (cpudec == 0)
		swprintf(CPUText, L"%u.000c", cputemp);	
	else
		swprintf(CPUText, L"%u.%uc", cputemp,cpudec);	
	swprintf(GPUText, L"%u.000c", mbtemp);	
	
	{
		const WCHAR *psztext=g_localizeStrings.Get(140).c_str();
		swprintf(wszText,L"%s %s", psztext,CPUText);
	
		CGUIMessage msg(GUI_MSG_LABEL_SET,GetID(),2,0,0,(void*)wszText);
    g_graphicsContext.SendMessage(msg); 
	}
	{
		const WCHAR *psztext=g_localizeStrings.Get(141).c_str();
		swprintf(wszText,L"%s %s", psztext,GPUText);
	
		CGUIMessage msg(GUI_MSG_LABEL_SET,GetID(),3,0,0,(void*)wszText);
    g_graphicsContext.SendMessage(msg); 
	}

	// time build:
	{
		const WCHAR *psztext=g_localizeStrings.Get(144).c_str();
		const WCHAR *pszbuild=g_localizeStrings.Get(6).c_str();
		swprintf(wszText,L"%s %s", psztext,pszbuild);
	
		CGUIMessage msg(GUI_MSG_LABEL_SET,GetID(),5,0,0,(void*)wszText);
    g_graphicsContext.SendMessage(msg); 
	}

	// time current
	{
		const WCHAR *pszCurrent=g_localizeStrings.Get(143).c_str();
		WCHAR wszTime[32];
    SYSTEMTIME time;
    GetLocalTime(&time);
    swprintf(wszTime,L"%s: %d:%02d:%02d %d/%d/%d",pszCurrent,time.wHour,time.wMinute,time.wSecond,time.wDay,time.wMonth,time.wYear);
	
		CGUIMessage msg(GUI_MSG_LABEL_SET,GetID(),4,0,0,(void*)wszTime);
    g_graphicsContext.SendMessage(msg); 
	}

	{
		XNADDR net_stat;
		WCHAR wzmac_addr[32];
		WCHAR wzIP[32];
		const WCHAR *psztext=g_localizeStrings.Get(146).c_str();
		const WCHAR *psztype;
		if(XNetGetTitleXnAddr(&net_stat) & XNET_GET_XNADDR_DHCP)
			psztype=g_localizeStrings.Get(148).c_str();
		else
			psztype=g_localizeStrings.Get(147).c_str();
		{
			swprintf(wszText,L"%s %s", psztext,psztype);
			CGUIMessage msg(GUI_MSG_LABEL_SET,GetID(),6,0,0,(void*)wszText);
			g_graphicsContext.SendMessage(msg);
		}

		{
			psztext=g_localizeStrings.Get(149).c_str();
			swprintf(wzmac_addr,L"%s: %02X:%02X:%02X:%02X:%02X:%02X",psztext,net_stat.abEnet[0],net_stat.abEnet[1],net_stat.abEnet[2],net_stat.abEnet[3],net_stat.abEnet[4],net_stat.abEnet[5]);

			CGUIMessage msg(GUI_MSG_LABEL_SET,GetID(),7,0,0,(void*)wzmac_addr);
			g_graphicsContext.SendMessage(msg);
		}
		{

			const WCHAR* pszIP=g_localizeStrings.Get(150).c_str();
			swprintf(wzIP,L"%s: %S",pszIP,g_szTitleIP);
			CGUIMessage msg(GUI_MSG_LABEL_SET,GetID(),8,0,0,(void*)wzIP);
			g_graphicsContext.SendMessage(msg);
		}

		{
			const WCHAR* pszHalf=g_localizeStrings.Get(152).c_str();
			const WCHAR* pszFull=g_localizeStrings.Get(153).c_str();
			const WCHAR* pszLink=g_localizeStrings.Get(151).c_str();
			const WCHAR* pszNoLink=g_localizeStrings.Get(159).c_str();
			DWORD dwnetstatus = XNetGetEthernetLinkStatus();
			WCHAR linkstatus[64];
			wcscpy(linkstatus,pszLink);
			if (dwnetstatus & XNET_ETHERNET_LINK_ACTIVE)
			{
				if (dwnetstatus & XNET_ETHERNET_LINK_100MBPS)
					wcscat(linkstatus,L"100mbps ");
				if (dwnetstatus & XNET_ETHERNET_LINK_10MBPS)
					wcscat(linkstatus,L"10mbps ");
				if (dwnetstatus & XNET_ETHERNET_LINK_FULL_DUPLEX)
					wcscat(linkstatus,pszFull);
				if (dwnetstatus & XNET_ETHERNET_LINK_HALF_DUPLEX)
					wcscat(linkstatus,pszHalf);
			} 
			else
				wcscat(linkstatus,pszNoLink);

			CGUIMessage msg(GUI_MSG_LABEL_SET,GetID(),9,0,0,(void*)linkstatus);
			g_graphicsContext.SendMessage(msg);
		}
	}


	{
		ULARGE_INTEGER lTotalFreeBytes;
		WCHAR wszHD[64];
		
		const WCHAR *pszDrive=g_localizeStrings.Get(155).c_str();
		const WCHAR *pszFree=g_localizeStrings.Get(160).c_str();
		const WCHAR *pszUnavailable=g_localizeStrings.Get(161).c_str();
		if (GetDiskFreeSpaceEx( "C:\\", NULL, NULL, &lTotalFreeBytes))
		{
			swprintf(wszHD, L"%s C: %u Mb", pszDrive,lTotalFreeBytes.QuadPart/1048576); //To make it MB
			wcscat(wszHD,pszFree);
		} 
		else {
			swprintf(wszHD, L"%s C: ",pszDrive);
			wcscat(wszHD,pszUnavailable);
		}
		{
			CGUIMessage msg(GUI_MSG_LABEL_SET,GetID(),10,0,0,(void*)wszHD);
			g_graphicsContext.SendMessage(msg);
		}
		if (GetDiskFreeSpaceEx( "E:\\", NULL, NULL, &lTotalFreeBytes))
		{
			swprintf(wszHD, L"%s E: %u Mb ", pszDrive,lTotalFreeBytes.QuadPart/1048576); //To make it MB
			wcscat(wszHD,pszFree);
		} 
		else {
			swprintf(wszHD, L"%s E: ",pszDrive);
			wcscat(wszHD,pszUnavailable);
		}
		{
			CGUIMessage msg(GUI_MSG_LABEL_SET,GetID(),12,0,0,(void*)wszHD);
			g_graphicsContext.SendMessage(msg);
		}
		if (GetDiskFreeSpaceEx( "F:\\", NULL, NULL, &lTotalFreeBytes))
		{
			swprintf(wszHD, L"%s F: %u Mb ", pszDrive,lTotalFreeBytes.QuadPart/1048576); //To make it MB
			wcscat(wszHD,pszFree);
		} 
		else {
			swprintf(wszHD, L"%s F: ",pszDrive);
			wcscat(wszHD,pszUnavailable);
		}
		{
			CGUIMessage msg(GUI_MSG_LABEL_SET,GetID(),13,0,0,(void*)wszHD);
			g_graphicsContext.SendMessage(msg);
		}
	}
	{

			CIoSupport m_pIOhelp;
			WCHAR wsztraystate[64];
			const WCHAR *pszDrive=g_localizeStrings.Get(155).c_str();
			swprintf(wsztraystate,L"%s D: ",pszDrive);
			const WCHAR* pszStatus;
			switch (m_pIOhelp.GetTrayState())
			{
			case TRAY_OPEN:
				pszStatus=g_localizeStrings.Get(162).c_str();
				break;
			case DRIVE_NOT_READY:
				pszStatus=g_localizeStrings.Get(163).c_str();
				break;
			case TRAY_CLOSED_NO_MEDIA:
				pszStatus=g_localizeStrings.Get(164).c_str();
				break;
			case TRAY_CLOSED_MEDIA_PRESENT:
				pszStatus=g_localizeStrings.Get(165).c_str();
				break;
			}
			WCHAR wszStatus[128];
			swprintf(wszStatus,L"%s %s", wsztraystate,pszStatus);
			CGUIMessage msg(GUI_MSG_LABEL_SET,GetID(),11,0,0,(void*)wszStatus);
			g_graphicsContext.SendMessage(msg);
	}
	{
		WCHAR wszText[128];
		swprintf(wszText,L"%ix%i%s",g_graphicsContext.GetWidth(),g_graphicsContext.GetHeight(),(g_graphicsContext.IsWidescreen() ? L"@16:9":L"@4:3"));
		CGUIMessage msg(GUI_MSG_LABEL_SET,GetID(),14,0,0,(void*)wszText);
		g_graphicsContext.SendMessage(msg);
	}

	{
		WCHAR wszText[128];
		MEMORYSTATUS stat;
		const WCHAR* pszFreeMem=g_localizeStrings.Get(158).c_str();
		GlobalMemoryStatus(&stat);
		swprintf(wszText,L"%s %i/%iMB",pszFreeMem,stat.dwAvailPhys  /(1024*1024),
																					stat.dwTotalPhys  /(1024*1024)  );
		CGUIMessage msg(GUI_MSG_LABEL_SET,GetID(),15,0,0,(void*)wszText);
		g_graphicsContext.SendMessage(msg);	
	}


}