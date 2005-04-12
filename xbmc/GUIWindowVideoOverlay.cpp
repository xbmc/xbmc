
#include "stdafx.h"
#include "GUIWindowVideoOverlay.h"
#include "util.h"
#include "application.h"
#include "utils/GUIInfoManager.h"

#define CONTROL_PLAYTIME  2
#define CONTROL_PLAY_LOGO   3
#define CONTROL_PAUSE_LOGO  4
#define CONTROL_INFO     5
#define CONTROL_BIG_PLAYTIME 6
#define CONTROL_FF_LOGO  7
#define CONTROL_RW_LOGO  8


CGUIWindowVideoOverlay::CGUIWindowVideoOverlay()
    : CGUIWindow(0)
{}

CGUIWindowVideoOverlay::~CGUIWindowVideoOverlay()
{}

void CGUIWindowVideoOverlay::OnWindowLoaded()
{
  CGUIWindow::OnWindowLoaded();
  // PRE1.3 make our images conditional if they're not already (backwards compatible)
  CGUIImage *pImage = (CGUIImage *)GetControl(CONTROL_PLAY_LOGO);
  if (pImage && !pImage->GetVisibleCondition()) pImage->SetVisibleCondition(4);
  pImage = (CGUIImage *)GetControl(CONTROL_PAUSE_LOGO);
  if (pImage && !pImage->GetVisibleCondition()) pImage->SetVisibleCondition(5);
  pImage = (CGUIImage *)GetControl(CONTROL_RW_LOGO);
  if (pImage && !pImage->GetVisibleCondition()) pImage->SetVisibleCondition(6);
  pImage = (CGUIImage *)GetControl(CONTROL_FF_LOGO);
  if (pImage && !pImage->GetVisibleCondition()) pImage->SetVisibleCondition(12);
  CGUILabelControl *pLabel = (CGUILabelControl *)GetControl(CONTROL_BIG_PLAYTIME);
  if (pLabel && !pLabel->GetInfo()) pLabel->SetInfo(254);
  pLabel = (CGUILabelControl *)GetControl(CONTROL_PLAYTIME);
  if (pLabel && !pLabel->GetInfo()) pLabel->SetInfo(256);
  // PRE1.3
}

void CGUIWindowVideoOverlay::Render()
{
  if (!g_application.m_pPlayer) return ;
  if (!g_application.m_pPlayer->HasVideo()) return ;

  /* TODO - Move this code to Application::Render() */
  __int64 lPTS = g_application.m_pPlayer->GetTime() / 100;
  int hh = (int)(lPTS / 36000) % 100;
  int mm = (int)((lPTS / 600) % 60);
  int ss = (int)((lPTS / 10) % 60);
  int iSpeed = g_application.GetPlaySpeed();
  if (hh == 0 && mm == 0 && ss < 5)
  {
    if (iSpeed < 1)
    {
      iSpeed = 1;
      g_application.SetPlaySpeed(iSpeed);
      g_application.m_pPlayer->SeekTime(0);
    }
  }

  CGUIWindow::Render();
}

void CGUIWindowVideoOverlay::Update()
{
  // reset the control_info control
  CGUIMessage msg1(GUI_MSG_LABEL_RESET, GetID(), CONTROL_INFO);
  OnMessage(msg1);

  CIMDBMovie movieDetails = g_infoManager.GetCurrentMovie();
  // title
  if ( movieDetails.m_strTitle.size() > 0)
  {
    CStdString strLabel = movieDetails.m_strTitle;
    CGUIMessage msg1(GUI_MSG_LABEL_ADD, GetID(), CONTROL_INFO);
    msg1.SetLabel(strLabel );
    OnMessage(msg1);
  }
  // genre
  if ( movieDetails.m_strGenre.size() > 0)
  {
    const WCHAR* wsGenre = g_localizeStrings.Get(174).c_str();
    WCHAR wsTmp[1024];
    swprintf(wsTmp, L"%s %S", wsGenre, movieDetails.m_strGenre.c_str());
    CGUIMessage msg1(GUI_MSG_LABEL_ADD, GetID(), CONTROL_INFO);
    msg1.SetLabel(wsTmp );
    OnMessage(msg1);
  }

  // year
  if ( movieDetails.m_iYear > 1900)
  {
    const WCHAR* wsYear = g_localizeStrings.Get(201).c_str();
    WCHAR wsTmp[1024];
    swprintf(wsTmp, L"%s %i", wsYear, movieDetails.m_iYear);
    CGUIMessage msg1(GUI_MSG_LABEL_ADD, GetID(), CONTROL_INFO);
    msg1.SetLabel(wsTmp );
    OnMessage(msg1);
  }

  // director
  if ( movieDetails.m_strDirector.size() > 0)
  {
    const WCHAR* wsDirector = g_localizeStrings.Get(199).c_str();
    WCHAR wsTmp[1024];
    swprintf(wsTmp, L"%s %S", wsDirector, movieDetails.m_strDirector.c_str() );
    CGUIMessage msg1(GUI_MSG_LABEL_ADD, GetID(), CONTROL_INFO);
    msg1.SetLabel(wsTmp );
    OnMessage(msg1);
  }
}
