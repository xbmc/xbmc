#include "GUIVisualisationControl.h"
#include "GUIWindowManager.h"
#include "GUIUserMessages.h"
#include "visualizations/Visualisation.h"
#include "utils/AddonManager.h"
#include "Util.h"
#include "utils/log.h"
#include "utils/SingleLock.h"
#include "GUISettings.h"
#include "FileSystem/SpecialProtocol.h"

using namespace std;
using namespace ADDON;

#define LABEL_ROW1 10
#define LABEL_ROW2 11
#define LABEL_ROW3 12

CGUIVisualisationControl::CGUIVisualisationControl(int parentID, int controlID, float posX, float posY, float width, float height)
    : CGUIControl(parentID, controlID, posX, posY, width, height)
{
  ControlType = GUICONTROL_VISUALISATION;
}

CGUIVisualisationControl::CGUIVisualisationControl(const CGUIVisualisationControl &from)
: CGUIControl(from)
{
  ControlType = GUICONTROL_VISUALISATION;
}

void CGUIVisualisationControl::LoadVisualisation(VizPtr &viz)
{
  if (!viz)
    return;

  CSingleLock lock(m_rendering);
  g_graphicsContext.CaptureStateBlock();
  float x = g_graphicsContext.ScaleFinalXCoord(GetXPosition(), GetYPosition());
  float y = g_graphicsContext.ScaleFinalYCoord(GetXPosition(), GetYPosition());
  float w = g_graphicsContext.ScaleFinalXCoord(GetXPosition() + GetWidth(), GetYPosition() + GetHeight()) - x;
  float h = g_graphicsContext.ScaleFinalYCoord(GetXPosition() + GetWidth(), GetYPosition() + GetHeight()) - y;
  if (x < 0) x = 0;
  if (y < 0) y = 0;
  if (x + w > g_graphicsContext.GetWidth()) w = g_graphicsContext.GetWidth() - x;
  if (y + h > g_graphicsContext.GetHeight()) h = g_graphicsContext.GetHeight() - y;

  if (viz->Create((int)(x+0.5f), (int)(y+0.5f), (int)(w+0.5f), (int)(h+0.5f)))
  {
    g_graphicsContext.ApplyStateBlock();
    VerifyGLState();
    m_addon = viz;
  }

  // tell our app that we're back
  CGUIMessage msg(GUI_MSG_VISUALISATION_LOADED, 0, 0, 0, 0);
  msg.SetPointer((void*)m_addon.get());
  g_windowManager.SendMessage(msg);
}

void CGUIVisualisationControl::UpdateVisibility(const CGUIListItem *item)
{
  // if made invisible, start timer, only free addonptr after
  // some period, configurable by window class
  CGUIControl::UpdateVisibility(item);
  if (!IsVisible())
    FreeResources();
}

void CGUIVisualisationControl::Render()
{
  CSingleLock lock(m_rendering);
  if (m_addon)
  {
    // set the viewport - note: We currently don't have any control over how
    // the visualisation renders, so the best we can do is attempt to define
    // a viewport??
    g_graphicsContext.SetViewPort(m_posX, m_posY, m_width, m_height);
    g_graphicsContext.CaptureStateBlock();
    m_addon->Render();
    g_graphicsContext.ApplyStateBlock();
    g_graphicsContext.RestoreViewPort();
  }

  CGUIControl::Render();
}

void CGUIVisualisationControl::FreeResources()
{
  if (!m_addon) return;

  // tell our app that we're going
  CGUIMessage msg(GUI_MSG_VISUALISATION_UNLOADING, m_controlID, 0);
  g_windowManager.SendMessage(msg);
  CLog::Log(LOGDEBUG, "FreeVisualisation() started");

  g_graphicsContext.CaptureStateBlock(); //TODO locking
  m_addon->Stop();
  g_graphicsContext.ApplyStateBlock();

  CGUIControl::FreeResources();
  CLog::Log(LOGDEBUG, "FreeVisualisation() done");
}

bool CGUIVisualisationControl::CanFocusFromPoint(const CPoint &point) const
{ // mouse is allowed to focus this control, but it doesn't actually receive focus
  return IsVisible() && HitTest(point);
}
