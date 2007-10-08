#include "stdafx.h" 
// Screensaver.cpp: implementation of the CScreenSaver class.
//
//////////////////////////////////////////////////////////////////////

#include "../Application.h"
#include "ScreenSaver.h" 


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CScreenSaver::CScreenSaver(struct ScreenSaver* pScr, DllScreensaver* pDll, const CStdString& strScreenSaverName)
    : m_pDll(pDll)
    , m_pScr(pScr)
    , m_strScreenSaverName(strScreenSaverName)
{}

CScreenSaver::~CScreenSaver()
{
}

void CScreenSaver::Create()
{
  // pass it the screen width,height
  // and the name of the screensaver
  int iWidth = g_graphicsContext.GetWidth();
  int iHeight = g_graphicsContext.GetHeight();

  char szTmp[129];
  sprintf(szTmp, "scr create:%ix%i %s\n", iWidth, iHeight, m_strScreenSaverName.c_str());
  OutputDebugString(szTmp);

  float pixelRatio = g_settings.m_ResInfo[g_graphicsContext.GetVideoResolution()].fPixelRatio;
#ifndef HAS_SDL
  m_pScr->Create(g_graphicsContext.Get3DDevice(), iWidth, iHeight, m_strScreenSaverName.c_str(), pixelRatio);
#else
  m_pScr->Create(0, iWidth, iHeight, m_strScreenSaverName.c_str(), pixelRatio);
#endif
}

void CScreenSaver::Start()
{
  // notify screen saver that they should start
  m_pScr->Start();
}

void CScreenSaver::Render()
{
  // ask screensaver to render itself
  m_pScr->Render();
}

void CScreenSaver::Stop()
{
  // ask screensaver to cleanup
  m_pScr->Stop();
}


void CScreenSaver::GetInfo(SCR_INFO *info)
{
  // get info from screensaver
  m_pScr->GetInfo(info);
}
