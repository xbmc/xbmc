#include "GUIWindowMusicOverlay.h"
#include "settings.h"
#include "guiWindowManager.h"
#include "localizestrings.h"
#include "util.h"
#include "stdstring.h"
#include "application.h"

#define CONTROL_PLAYTIME		2

CGUIWindowMusicOverlay::CGUIWindowMusicOverlay()
:CGUIWindow(0)
{
}

CGUIWindowMusicOverlay::~CGUIWindowMusicOverlay()
{
}


void CGUIWindowMusicOverlay::OnKey(const CKey& key)
{
  CGUIWindow::OnKey(key);
}

bool CGUIWindowMusicOverlay::OnMessage(CGUIMessage& message)
{
  return CGUIWindow::OnMessage(message);
}


void CGUIWindowMusicOverlay::Render()
{
	if (!g_application.m_pPlayer) return;
	__int64 lPTS=g_application.m_pPlayer->GetPTS();
  //int hh = (lPTS / 36000) % 100;
  int mm = (int)((lPTS / 600) % 60);
  int ss = (int)((lPTS /  10) % 60);
  //int f1 = lPTS % 10;
	
	char szTime[32];
	sprintf(szTime,"%02.2i:%02.2i",mm,ss);
	CGUIMessage msg(GUI_MSG_LABEL_SET, GetID(), CONTROL_PLAYTIME); 
	msg.SetLabel(szTime); 
	OnMessage(msg); 

	CGUIWindow::Render();
}