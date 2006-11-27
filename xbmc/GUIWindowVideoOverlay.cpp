
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
    : CGUIDialog(WINDOW_VIDEO_OVERLAY, "VideoOverlay.xml")
{
  m_renderOrder = 0;
  m_visibleCondition = SKIN_HAS_VIDEO_OVERLAY;
}

CGUIWindowVideoOverlay::~CGUIWindowVideoOverlay()
{}

void CGUIWindowVideoOverlay::Render()
{
  if (m_gWindowManager.GetActiveWindow() == WINDOW_FULLSCREEN_VIDEO)
  { // close immediately
    Close(true);
    return;
  }
  CGUIDialog::Render();
}

void CGUIWindowVideoOverlay::SetDefaults()
{
  CGUIDialog::SetDefaults();
  m_renderOrder = 0;
  m_visibleCondition = SKIN_HAS_VIDEO_OVERLAY;
}