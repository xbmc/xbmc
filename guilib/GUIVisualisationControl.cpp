#include "GUIVisualisationControl.h"
#include "GUIWindowManager.h"
#include "GUIUserMessages.h"
#include "visualizations/Visualisation.h"
#include "utils/AddonManager.h"
#include "Util.h"
#include "utils/log.h"
#include "utils/SingleLock.h"
#include "GUISettings.h"

using namespace std;
using namespace ADDON;

#define LABEL_ROW1 10
#define LABEL_ROW2 11
#define LABEL_ROW3 12

CGUIVisualisationControl::CGUIVisualisationControl(int parentID, int controlID, float posX, float posY, float width, float height)
    : CGUIControl(parentID, controlID, posX, posY, width, height)
{
  m_bInitialized = false;
  m_currentVis = "";
  ControlType = GUICONTROL_VISUALISATION;
}

CGUIVisualisationControl::CGUIVisualisationControl(const CGUIVisualisationControl &from)
: CGUIControl(from)
{
  m_bInitialized = false;
  m_currentVis = "";
  ControlType = GUICONTROL_VISUALISATION;
}

CGUIVisualisationControl::~CGUIVisualisationControl(void)
{
}

void CGUIVisualisationControl::FreeVisualisation()
{
  if (!m_bInitialized) return;
  m_bInitialized = false;
  // tell our app that we're going
  CGUIMessage msg(GUI_MSG_VISUALISATION_UNLOADING, m_controlID, 0);
  g_windowManager.SendMessage(msg);

  CLog::Log(LOGDEBUG, "FreeVisualisation() started");
  if (m_addon)
  {
    g_graphicsContext.CaptureStateBlock();
    m_addon->Stop();
    g_graphicsContext.ApplyStateBlock();
    m_addon->Destroy();
  }
  CLog::Log(LOGDEBUG, "FreeVisualisation() done");
}

void CGUIVisualisationControl::LoadVisualisation()
{
  m_bInitialized = false;

  AddonPtr addon;
  if (!CAddonMgr::Get()->GetAddon(ADDON_VIZ, g_guiSettings.GetString("musicplayer.visualisation"), addon))
      return;

  m_addon = boost::dynamic_pointer_cast<CVisualisation>(addon);
  m_currentVis = m_addon->Name();

  if (!m_addon)
    return;

  g_graphicsContext.CaptureStateBlock();
  float x = g_graphicsContext.ScaleFinalXCoord(GetXPosition(), GetYPosition());
  float y = g_graphicsContext.ScaleFinalYCoord(GetXPosition(), GetYPosition());
  float w = g_graphicsContext.ScaleFinalXCoord(GetXPosition() + GetWidth(), GetYPosition() + GetHeight()) - x;
  float h = g_graphicsContext.ScaleFinalYCoord(GetXPosition() + GetWidth(), GetYPosition() + GetHeight()) - y;
  if (x < 0) x = 0;
  if (y < 0) y = 0;
  if (x + w > g_graphicsContext.GetWidth()) w = g_graphicsContext.GetWidth() - x;
  if (y + h > g_graphicsContext.GetHeight()) h = g_graphicsContext.GetHeight() - y;

  if (m_addon->Create((int)(x+0.5f), (int)(y+0.5f), (int)(w+0.5f), (int)(h+0.5f)))
  {
    g_graphicsContext.ApplyStateBlock();
    VerifyGLState();
    m_bInitialized = true;
  }

  // tell our app that we're back
  //TODO need to pass shrd_ptr<Vis> instead CGUIMessage msg(GUI_MSG_VISUALISATION_LOADED, 0, 0, 0, 0, m_pVisualisation);
  //g_windowManager.SendMessage(msg);
}

void CGUIVisualisationControl::UpdateVisibility(const CGUIListItem *item)
{
  CGUIControl::UpdateVisibility(item);
  if (!IsVisible() && m_bInitialized)
    FreeVisualisation();
}

void CGUIVisualisationControl::Render()
{
  if (!m_addon || !m_currentVis.Equals(g_guiSettings.GetString("musicplayer.visualisation")))
  { // check if we need to load
    LoadVisualisation();
    CGUIControl::Render();
    return;
  }
  
  if (m_bInitialized)
  {
    // set the viewport - note: We currently don't have any control over how
    // the visualisation renders, so the best we can do is attempt to define
    // a viewport??
    g_graphicsContext.SetViewPort(m_posX, m_posY, m_width, m_height);
    try
    {
      g_graphicsContext.CaptureStateBlock();
      m_addon->Render();
      g_graphicsContext.ApplyStateBlock();
    }
    catch (...)
    {
      CLog::Log(LOGERROR, "Exception in Visualisation::Render()");
    }
    // clear the viewport
    g_graphicsContext.RestoreViewPort();
  }

  CGUIControl::Render();
}

bool CGUIVisualisationControl::OnAction(const CAction &action)
{
  if (!m_addon) return false;
  VIS_ACTION visAction = VIS_ACTION_NONE;
  if (action.id == ACTION_VIS_PRESET_NEXT)
    visAction = VIS_ACTION_NEXT_PRESET;
  else if (action.id == ACTION_VIS_PRESET_PREV)
    visAction = VIS_ACTION_PREV_PRESET;
  else if (action.id == ACTION_VIS_PRESET_LOCK)
    visAction = VIS_ACTION_LOCK_PRESET;
  else if (action.id == ACTION_VIS_PRESET_RANDOM)
    visAction = VIS_ACTION_RANDOM_PRESET;
  else if (action.id == ACTION_VIS_RATE_PRESET_PLUS)
    visAction = VIS_ACTION_RATE_PRESET_PLUS;
  else if (action.id == ACTION_VIS_RATE_PRESET_MINUS)
    visAction = VIS_ACTION_RATE_PRESET_MINUS;

  return m_addon->OnAction(visAction);
}

bool CGUIVisualisationControl::UpdateTrack()
{
  bool handled = false;

  if (m_addon)
  {
    return m_addon->UpdateTrack();
  }
  return false;
}

bool CGUIVisualisationControl::OnMessage(CGUIMessage &message)
{
  if (message.GetMessage() == GUI_MSG_GET_VISUALISATION)
  { //TODO viz pointers issue
    message.SetPointer(m_addon.get());
    return true;
  }
  else if (message.GetMessage() == GUI_MSG_VISUALISATION_ACTION)
  {
    CAction action;
    action.id = message.GetParam1();
    return OnAction(action);
  }
  else if (message.GetMessage() == GUI_MSG_PLAYBACK_STARTED)
  {
    if (IsVisible() && m_addon && m_addon->UpdateTrack()) return true;
  }
  return CGUIControl::OnMessage(message);
}

void CGUIVisualisationControl::FreeResources()
{
  FreeVisualisation();
  CGUIControl::FreeResources();
}

bool CGUIVisualisationControl::OnMouseOver(const CPoint &point)
{
  // unfocusable, so return true
  CGUIControl::OnMouseOver(point);
  return false;
}

bool CGUIVisualisationControl::CanFocus() const
{ // unfocusable
  return false;
}

bool CGUIVisualisationControl::CanFocusFromPoint(const CPoint &point, CGUIControl **control, CPoint &controlPoint) const
{ // mouse is allowed to focus this control, but it doesn't actually receive focus
  controlPoint = point;
  m_transform.InverseTransformPosition(controlPoint.x, controlPoint.y);
  if (HitTest(controlPoint))
  {
    *control = (CGUIControl *)this;
    return true;
  }
  *control = NULL;
  return false;
}
