#include "stdafx.h"
#include "GUIWindowSystemInfo.h"
#include "xbox/iosupport.h"
#include <ConIo.h>
#include "Utils/FanController.h"
#include "cores/DllLoader/dll.h"
#include "Utils/GUIInfoManager.h"

// PRE1.3
#include "GUILabelControl.h"
// PRE1.3

CGUIWindowSystemInfo::CGUIWindowSystemInfo(void)
    : CGUIWindow(0)
{}

CGUIWindowSystemInfo::~CGUIWindowSystemInfo(void)
{}

void CGUIWindowSystemInfo::OnAction(const CAction &action)
{
  if (action.wID == ACTION_PREVIOUS_MENU)
  {
    m_gWindowManager.PreviousWindow();
    return ;
  }
  CGUIWindow::OnAction(action);
}

//lpParam will get the version info, wchar format
DWORD WINAPI GetMPlayerVersionW( LPVOID lpParam )
{
  wchar_t wszVersion[50];
  wchar_t wszCompileDate[50];
  const char* (__cdecl* pMplayerGetVersion)();
  const char* (__cdecl* pMplayerGetCompileDate)();
  wszVersion[0] = 0; wszCompileDate[0] = 0;
  DllLoader* mplayerDll;
  // if(!g_guiSettings.GetBool("MyVideos.AlternateMPlayer"))
  //  {
  mplayerDll = new DllLoader("Q:\\system\\players\\mplayer\\mplayer.dll", true);
  /*  }
    else
    {
      mplayerDll = new DllLoader("Q:\\mplayer\\mplayer.dll",true);
    }*/

  if ( mplayerDll->Parse() )
  {
    //try to resolve and call mplayer_getversion
    if (mplayerDll->ResolveExport("mplayer_getversion", (void**)&pMplayerGetVersion))
      mbstowcs(wszVersion, pMplayerGetVersion(), sizeof(wszVersion));

    //try to resolve and call mplayer_getcompiledate
    if (mplayerDll->ResolveExport("mplayer_getcompiledate", (void**)&pMplayerGetCompileDate))
      mbstowcs(wszCompileDate, pMplayerGetCompileDate(), sizeof(wszCompileDate));

    //see if we have compiledate and or version
    if (wszVersion[0] != 0 && wszCompileDate[0] != 0)
      _snwprintf((wchar_t *)lpParam, 50, L"%s (%s)", wszVersion, wszCompileDate);
    else if (wszVersion[0] != 0)
      _snwprintf((wchar_t *)lpParam, 50, L"%s", wszVersion);
  }
  delete mplayerDll;
  mplayerDll = NULL;
  return 0;
}

bool CGUIWindowSystemInfo::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
  case GUI_MSG_WINDOW_DEINIT:
    break;

  case GUI_MSG_WINDOW_INIT:
    {
      m_dwFPSTime = timeGetTime();
      m_dwFrames = 0;
      m_fFPS = 0.0f;

      //Get the version from the dll in a seperate thread.

      m_wszMPlayerVersion[0] = 0;

      HANDLE hThread = CreateThread(NULL, 0,

                                    GetMPlayerVersionW,   // thread function

                                    &m_wszMPlayerVersion,  // argument to thread function

                                    0, NULL);



      if (hThread != NULL)

        CloseHandle( hThread );

    }
    break;
  }
  return CGUIWindow::OnMessage(message);
}

void CGUIWindowSystemInfo::Render()
{

  GetValues();
  CGUIWindow::Render();
  DWORD dwTimeSpan = timeGetTime() - m_dwFPSTime;
  if (dwTimeSpan >= 1000)
  {
    m_fFPS = ((float)m_dwFrames * 1000.0f) / ((float)dwTimeSpan);
    m_dwFPSTime = timeGetTime();
    m_dwFrames = 0;
  }
  m_dwFrames++;

}

void CGUIWindowSystemInfo::GetValues()
{
  WCHAR wszText[1024];
  // time build:
  {
    const WCHAR *psztext = g_localizeStrings.Get(144).c_str();
    const WCHAR *pszbuild = g_localizeStrings.Get(6).c_str();
    WCHAR wszDate[32];
    mbstowcs(wszDate, __DATE__, sizeof(wszDate));
    swprintf(wszText, L"%s %s (%s)\n%s", psztext, pszbuild, wszDate, m_wszMPlayerVersion);

    SET_CONTROL_LABEL(5, wszText);
  }

  // time current
  CStdStringW strDateTime;
  strDateTime.Format(L"%s %s", g_infoManager.GetTime(true), g_infoManager.GetDate(true));
  SET_CONTROL_LABEL(4, strDateTime);

  {
    XNADDR net_stat;
    WCHAR wzmac_addr[32];
    const WCHAR *psztext = g_localizeStrings.Get(146).c_str();
    const WCHAR *psztype;
    if (XNetGetTitleXnAddr(&net_stat) & XNET_GET_XNADDR_DHCP)
      psztype = g_localizeStrings.Get(148).c_str();
    else
      psztype = g_localizeStrings.Get(147).c_str();
    {
      swprintf(wszText, L"%s %s", psztext, psztype);
      SET_CONTROL_LABEL(6, wszText);
    }

    {
      psztext = g_localizeStrings.Get(149).c_str();
      swprintf(wzmac_addr, L"%s: %02X:%02X:%02X:%02X:%02X:%02X", psztext, net_stat.abEnet[0], net_stat.abEnet[1], net_stat.abEnet[2], net_stat.abEnet[3], net_stat.abEnet[4], net_stat.abEnet[5]);

      SET_CONTROL_LABEL(7, wzmac_addr);
    }

    {
      const WCHAR* pszHalf = g_localizeStrings.Get(152).c_str();
      const WCHAR* pszFull = g_localizeStrings.Get(153).c_str();
      const WCHAR* pszLink = g_localizeStrings.Get(151).c_str();
      const WCHAR* pszNoLink = g_localizeStrings.Get(159).c_str();
      DWORD dwnetstatus = XNetGetEthernetLinkStatus();
      WCHAR linkstatus[64];
      wcscpy(linkstatus, pszLink);
      if (dwnetstatus & XNET_ETHERNET_LINK_ACTIVE)
      {
        if (dwnetstatus & XNET_ETHERNET_LINK_100MBPS)
          wcscat(linkstatus, L"100mbps ");
        if (dwnetstatus & XNET_ETHERNET_LINK_10MBPS)
          wcscat(linkstatus, L"10mbps ");
        if (dwnetstatus & XNET_ETHERNET_LINK_FULL_DUPLEX)
          wcscat(linkstatus, pszFull);
        if (dwnetstatus & XNET_ETHERNET_LINK_HALF_DUPLEX)
          wcscat(linkstatus, pszHalf);
      }
      else
        wcscat(linkstatus, pszNoLink);

      SET_CONTROL_LABEL(9, linkstatus);
    }
  }

  {

    CIoSupport m_pIOhelp;
    WCHAR wsztraystate[64];
    const WCHAR *pszDrive = g_localizeStrings.Get(155).c_str();
    swprintf(wsztraystate, L"%s D: ", pszDrive);
    const WCHAR* pszStatus;
    switch (m_pIOhelp.GetTrayState())
    {
    case TRAY_OPEN:
      pszStatus = g_localizeStrings.Get(162).c_str();
      break;
    case DRIVE_NOT_READY:
      pszStatus = g_localizeStrings.Get(163).c_str();
      break;
    case TRAY_CLOSED_NO_MEDIA:
      pszStatus = g_localizeStrings.Get(164).c_str();
      break;
    case TRAY_CLOSED_MEDIA_PRESENT:
      pszStatus = g_localizeStrings.Get(165).c_str();
      break;
    }
    WCHAR wszStatus[128];
    swprintf(wszStatus, L"%s %s", wsztraystate, pszStatus);
    SET_CONTROL_LABEL(11, wszStatus);
  }
  {

    WCHAR wszText[128];
    RESOLUTION res = g_graphicsContext.GetVideoResolution();
    swprintf(wszText, L"%ix%i %S %02.2f Hz.",
             g_settings.m_ResInfo[res].iWidth,
             g_settings.m_ResInfo[res].iHeight,
             g_settings.m_ResInfo[res].strMode,
             m_fFPS);
    SET_CONTROL_LABEL(14, wszText);
  }

  {
    WCHAR wszText[128];
    MEMORYSTATUS stat;
    const WCHAR* pszFreeMem = g_localizeStrings.Get(158).c_str();
    GlobalMemoryStatus(&stat);
    swprintf(wszText, L"%s %i/%iMB", pszFreeMem, stat.dwAvailPhys / (1024*1024),
             stat.dwTotalPhys / (1024*1024) );
    SET_CONTROL_LABEL(15, wszText);
  }
}

void CGUIWindowSystemInfo::OnWindowLoaded()
{
  CGUIWindow::OnWindowLoaded();
  // PRE1.3 - setup labels -> infolabels
  CGUILabelControl *pLabel = (CGUILabelControl *)GetControl(2); // cpu
  if (pLabel && !pLabel->GetInfo()) pLabel->SetInfo(112);
  pLabel = (CGUILabelControl *)GetControl(3); // gpu
  if (pLabel && !pLabel->GetInfo()) pLabel->SetInfo(113);
  pLabel = (CGUILabelControl *)GetControl(16); // fan
  if (pLabel && !pLabel->GetInfo()) pLabel->SetInfo(114);
  pLabel = (CGUILabelControl *)GetControl(8);  // ip
  if (pLabel && !pLabel->GetInfo()) pLabel->SetInfo(190);
  pLabel = (CGUILabelControl *)GetControl(10);  // free c
  if (pLabel && !pLabel->GetInfo()) pLabel->SetInfo(115);
  pLabel = (CGUILabelControl *)GetControl(12);  // free e
  if (pLabel && !pLabel->GetInfo()) pLabel->SetInfo(117);
  pLabel = (CGUILabelControl *)GetControl(13);  // free f
  if (pLabel && !pLabel->GetInfo()) pLabel->SetInfo(118);
  pLabel = (CGUILabelControl *)GetControl(17);  // free g
  if (pLabel && !pLabel->GetInfo()) pLabel->SetInfo(119);
  // PRE1.3
}
