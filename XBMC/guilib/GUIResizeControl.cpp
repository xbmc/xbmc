#include "include.h"
#include "GUIResizeControl.h"
#include "GUIWindowManager.h"


// time to reset accelerated cursors (digital movement)
#define MOVE_TIME_OUT 500L

CGUIResizeControl::CGUIResizeControl(DWORD dwParentID, DWORD dwControlId, float posX, float posY, float width, float height, const CImage& textureFocus, const CImage& textureNoFocus)
    : CGUIControl(dwParentID, dwControlId, posX, posY, width, height)
    , m_imgFocus(dwParentID, dwControlId, posX, posY, width, height, textureFocus)
    , m_imgNoFocus(dwParentID, dwControlId, posX, posY, width, height, textureNoFocus)
{
  m_dwFrameCounter = 0;
  m_dwLastMoveTime = 0;
  m_fSpeed = 1.0;
  m_fAnalogSpeed = 2.0f; // TODO: implement correct analog speed
  m_fAcceleration = 0.2f; // TODO: implement correct computation of acceleration
  m_fMaxSpeed = 10.0;  // TODO: implement correct computation of maxspeed
  ControlType = GUICONTROL_RESIZE;
  SetLimits(0, 0, 720, 576); // defaults
}

CGUIResizeControl::~CGUIResizeControl(void)
{}

void CGUIResizeControl::Render()
{
  if (HasFocus())
  {
    DWORD dwAlphaCounter = m_dwFrameCounter + 2;
    DWORD dwAlphaChannel;
    if ((dwAlphaCounter % 128) >= 64)
      dwAlphaChannel = dwAlphaCounter % 64;
    else
      dwAlphaChannel = 63 - (dwAlphaCounter % 64);

    dwAlphaChannel += 192;
    SetAlpha( (unsigned char)dwAlphaChannel );
    m_imgFocus.SetVisible(true);
    m_imgNoFocus.SetVisible(false);
    m_dwFrameCounter++;
  }
  else
  {
    SetAlpha(0xff);
    m_imgFocus.SetVisible(false);
    m_imgNoFocus.SetVisible(true);
  }
  // render both so the visibility settings cause the frame counter to resetcorrectly
  m_imgFocus.Render();
  m_imgNoFocus.Render();
  CGUIControl::Render();
}

bool CGUIResizeControl::OnAction(const CAction &action)
{
  if (action.wID == ACTION_SELECT_ITEM)
  {
    // button selected - send message to parent
    CGUIMessage message(GUI_MSG_CLICKED, GetID(), GetParentID());
    SendWindowMessage(message);
    return true;
  }
  if (action.wID == ACTION_ANALOG_MOVE)
  {
    Resize(m_fAnalogSpeed*action.fAmount1, -m_fAnalogSpeed*action.fAmount2);
    return true;
  }
  return CGUIControl::OnAction(action);
}

void CGUIResizeControl::OnUp()
{
  UpdateSpeed(DIRECTION_UP);
  Resize(0, -m_fSpeed);
}

void CGUIResizeControl::OnDown()
{
  UpdateSpeed(DIRECTION_DOWN);
  Resize(0, m_fSpeed);
}

void CGUIResizeControl::OnLeft()
{
  UpdateSpeed(DIRECTION_LEFT);
  Resize(-m_fSpeed, 0);
}

void CGUIResizeControl::OnRight()
{
  UpdateSpeed(DIRECTION_RIGHT);
  Resize(m_fSpeed, 0);
}

bool CGUIResizeControl::OnMouseDrag(const CPoint &offset, const CPoint &point)
{
  g_Mouse.SetState(MOUSE_STATE_DRAG);
  g_Mouse.SetExclusiveAccess(GetID(), GetParentID(), point);
  Resize(offset.x, offset.y);
  return true;
}

bool CGUIResizeControl::OnMouseClick(DWORD dwButton, const CPoint &point)
{
  if (dwButton != MOUSE_LEFT_BUTTON) return false;
  g_Mouse.EndExclusiveAccess(GetID(), GetParentID());
  return true;
}

void CGUIResizeControl::UpdateSpeed(int nDirection)
{
  if (timeGetTime() - m_dwLastMoveTime > MOVE_TIME_OUT)
  {
    m_fSpeed = 1;
    m_nDirection = DIRECTION_NONE;
  }
  m_dwLastMoveTime = timeGetTime();
  if (nDirection == m_nDirection)
  { // accelerate
    m_fSpeed += m_fAcceleration;
    if (m_fSpeed > m_fMaxSpeed) m_fSpeed = m_fMaxSpeed;
  }
  else
  { // reset direction and speed
    m_fSpeed = 1;
    m_nDirection = nDirection;
  }
}

void CGUIResizeControl::PreAllocResources()
{
  CGUIControl::PreAllocResources();
  m_imgFocus.PreAllocResources();
  m_imgNoFocus.PreAllocResources();
}

void CGUIResizeControl::AllocResources()
{
  CGUIControl::AllocResources();
  m_dwFrameCounter = 0;
  m_imgFocus.AllocResources();
  m_imgNoFocus.AllocResources();
  m_width = m_imgFocus.GetWidth();
  m_height = m_imgFocus.GetHeight();
}

void CGUIResizeControl::FreeResources()
{
  CGUIControl::FreeResources();
  m_imgFocus.FreeResources();
  m_imgNoFocus.FreeResources();
}

void CGUIResizeControl::DynamicResourceAlloc(bool bOnOff)
{
  CGUIControl::DynamicResourceAlloc(bOnOff);
  m_imgFocus.DynamicResourceAlloc(bOnOff);
  m_imgNoFocus.DynamicResourceAlloc(bOnOff);
}

void CGUIResizeControl::Update()
{
  CGUIControl::Update();

  m_imgFocus.SetWidth(m_width);
  m_imgFocus.SetHeight(m_height);

  m_imgNoFocus.SetWidth(m_width);
  m_imgNoFocus.SetHeight(m_height);
}

void CGUIResizeControl::Resize(float x, float y)
{
  float width = m_width + x;
  float height = m_height + y;
  // check if we are within the bounds
  if (width < m_x1) width = m_x1;
  if (height < m_y1) height = m_y1;
  if (width > m_x2) width = m_x2;
  if (height > m_y2) height = m_y2;
  // ok, now set the default size of the resize control
  SetWidth(width);
  SetHeight(height);
}

void CGUIResizeControl::SetPosition(float posX, float posY)
{
  CGUIControl::SetPosition(posX, posY);
  m_imgFocus.SetPosition(posX, posY);
  m_imgNoFocus.SetPosition(posX, posY);
}

void CGUIResizeControl::SetAlpha(unsigned char alpha)
{
  m_imgFocus.SetAlpha(alpha);
  m_imgNoFocus.SetAlpha(alpha);
}

void CGUIResizeControl::SetColorDiffuse(D3DCOLOR color)
{
  CGUIControl::SetColorDiffuse(color);
  m_imgFocus.SetColorDiffuse(color);
  m_imgNoFocus.SetColorDiffuse(color);
}

void CGUIResizeControl::SetLimits(float x1, float y1, float x2, float y2)
{
  m_x1 = x1;
  m_y1 = y1;
  m_x2 = x2;
  m_y2 = y2;
}
