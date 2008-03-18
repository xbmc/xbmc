#include "include.h"
#include "GUIMoverControl.h"
#include "GUIWindowManager.h"
#include "ActionManager.h"


// time to reset accelerated cursors (digital movement)
#define MOVE_TIME_OUT 500L

CGUIMoverControl::CGUIMoverControl(DWORD dwParentID, DWORD dwControlId, float posX, float posY, float width, float height, const CImage& textureFocus, const CImage& textureNoFocus)
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
  ControlType = GUICONTROL_MOVER;
  SetLimits(0, 0, 720, 576); // defaults
  SetLocation(0, 0, false);  // defaults
}

CGUIMoverControl::~CGUIMoverControl(void)
{}

void CGUIMoverControl::Render()
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

bool CGUIMoverControl::OnAction(const CAction &action)
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
    //  if (m_dwAllowedDirections == ALLOWED_DIRECTIONS_UPDOWN)
    //   Move(0, (int)(-m_fAnalogSpeed*action.fAmount2));
    //  else if (m_dwAllowedDirections == ALLOWED_DIRECTIONS_LEFTRIGHT)
    //   Move((int)(m_fAnalogSpeed*action.fAmount1), 0);
    //  else // ALLOWED_DIRECTIONS_ALL
    Move((int)(m_fAnalogSpeed*action.fAmount1), (int)( -m_fAnalogSpeed*action.fAmount2));
    return true;
  }
  // base class
  return CGUIControl::OnAction(action);
}

void CGUIMoverControl::OnUp()
{
  // if (m_dwAllowedDirections == ALLOWED_DIRECTIONS_LEFTRIGHT) return;
  UpdateSpeed(DIRECTION_UP);
  Move(0, (int) - m_fSpeed);
}

void CGUIMoverControl::OnDown()
{
  // if (m_dwAllowedDirections == ALLOWED_DIRECTIONS_LEFTRIGHT) return;
  UpdateSpeed(DIRECTION_DOWN);
  Move(0, (int)m_fSpeed);
}

void CGUIMoverControl::OnLeft()
{
  // if (m_dwAllowedDirections == ALLOWED_DIRECTIONS_UPDOWN) return;
  UpdateSpeed(DIRECTION_LEFT);
  Move((int) - m_fSpeed, 0);
}

void CGUIMoverControl::OnRight()
{
  // if (m_dwAllowedDirections == ALLOWED_DIRECTIONS_UPDOWN) return;
  UpdateSpeed(DIRECTION_RIGHT);
  Move((int)m_fSpeed, 0);
}

bool CGUIMoverControl::OnMouseDrag(const CPoint &offset, const CPoint &point)
{
  g_Mouse.SetState(MOUSE_STATE_DRAG);
  g_Mouse.SetExclusiveAccess(GetID(), GetParentID(), point);
  Move((int)offset.x, (int)offset.y);
  return true;
}

bool CGUIMoverControl::OnMouseClick(DWORD dwButton, const CPoint &point)
{
  if (dwButton != MOUSE_LEFT_BUTTON) return false;
  g_Mouse.EndExclusiveAccess(GetID(), GetParentID());
  return true;
}

void CGUIMoverControl::UpdateSpeed(int nDirection)
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

void CGUIMoverControl::PreAllocResources()
{
  CGUIControl::PreAllocResources();
  m_imgFocus.PreAllocResources();
  m_imgNoFocus.PreAllocResources();
}

void CGUIMoverControl::AllocResources()
{
  CGUIControl::AllocResources();
  m_dwFrameCounter = 0;
  m_imgFocus.AllocResources();
  m_imgNoFocus.AllocResources();
  float width = m_width ? m_width : m_imgFocus.GetWidth();
  float height = m_height ? m_height : m_imgFocus.GetHeight();
  SetWidth(width);
  SetHeight(height);
}

void CGUIMoverControl::FreeResources()
{
  CGUIControl::FreeResources();
  m_imgFocus.FreeResources();
  m_imgNoFocus.FreeResources();
}

void CGUIMoverControl::DynamicResourceAlloc(bool bOnOff)
{
  CGUIControl::DynamicResourceAlloc(bOnOff);
  m_imgFocus.DynamicResourceAlloc(bOnOff);
  m_imgNoFocus.DynamicResourceAlloc(bOnOff);
}

void CGUIMoverControl::Update()
{
  CGUIControl::Update();

  m_imgFocus.SetWidth(m_width);
  m_imgFocus.SetHeight(m_height);

  m_imgNoFocus.SetWidth(m_width);
  m_imgNoFocus.SetHeight(m_height);
}

void CGUIMoverControl::Move(int iX, int iY)
{
  int iLocX = m_iLocationX + iX;
  int iLocY = m_iLocationY + iY;
  // check if we are within the bounds
  if (iLocX < m_iX1) iLocX = m_iX1;
  if (iLocY < m_iY1) iLocY = m_iY1;
  if (iLocX > m_iX2) iLocX = m_iX2;
  if (iLocY > m_iY2) iLocY = m_iY2;
  // ok, now set the location of the mover
  SetLocation(iLocX, iLocY);
}

void CGUIMoverControl::SetLocation(int iLocX, int iLocY, bool bSetPosition)
{
  if (bSetPosition) SetPosition(GetXPosition() + iLocX - m_iLocationX, GetYPosition() + iLocY - m_iLocationY);
  m_iLocationX = iLocX;
  m_iLocationY = iLocY;
}

void CGUIMoverControl::SetPosition(float posX, float posY)
{
  CGUIControl::SetPosition(posX, posY);
  m_imgFocus.SetPosition(posX, posY);
  m_imgNoFocus.SetPosition(posX, posY);
}

void CGUIMoverControl::SetAlpha(unsigned char alpha)
{
  m_imgFocus.SetAlpha(alpha);
  m_imgNoFocus.SetAlpha(alpha);
}

void CGUIMoverControl::SetColorDiffuse(D3DCOLOR color)
{
  CGUIControl::SetColorDiffuse(color);
  m_imgFocus.SetColorDiffuse(color);
  m_imgNoFocus.SetColorDiffuse(color);
}

void CGUIMoverControl::SetLimits(int iX1, int iY1, int iX2, int iY2)
{
  m_iX1 = iX1;
  m_iY1 = iY1;
  m_iX2 = iX2;
  m_iY2 = iY2;
}
