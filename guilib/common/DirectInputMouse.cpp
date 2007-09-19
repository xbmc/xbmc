#include "../include.h"
#include "DirectInputMouse.h"
#include "DirectInput.h"
#include "../key.h"

#include "../../Tools/Win32/XBMC_PC.h"
extern CXBMC_PC *g_xbmcPC;

CMouse g_Mouse; // global

CMouse::CMouse()
{
  m_mouse = NULL;
  m_bInitialized = false;
  m_dwExclusiveWindowID = WINDOW_INVALID;
  m_dwExclusiveControlID = WINDOW_INVALID;
  m_dwState = MOUSE_STATE_NORMAL;
  m_bActive = false;
}

CMouse::~CMouse()
{
  if (m_mouse)
    m_mouse->Release();
}

void CMouse::Initialize(HWND hWnd)
{
  if (m_bInitialized)
    return;

  if (FAILED(g_directInput.Initialize(hWnd)))
    return;

  if (FAILED(g_directInput.Get()->CreateDevice(GUID_SysMouse, &m_mouse, NULL)))
    return;
  
  if (FAILED(m_mouse->SetDataFormat(&c_dfDIMouse)))
    return;
  
  if (FAILED(m_mouse->SetCooperativeLevel(hWnd, DISCL_FOREGROUND | DISCL_NONEXCLUSIVE)))
    return;

  m_bInitialized = true;

//  Acquire();
  // Set the default resolution (PAL)
  SetResolution(720, 576, 1, 1);
}

void CMouse::Update()
{
  bool bMouseMoved(false);
  DIMOUSESTATE mouseState;
  memset(&mouseState, 0, sizeof(mouseState));
  if (S_OK == m_mouse->GetDeviceState(sizeof(DIMOUSESTATE), (LPVOID)&mouseState))
  {
    cMickeyX = (char)mouseState.lX;
    cMickeyY = (char)mouseState.lY;
    cWheel = (char)mouseState.lZ;
    POINT point;
    if (!g_xbmcPC->GetCursorPos(point))
    { // outside our client rect
      m_bActive = false;
      return;
    }
    if (point.x < 0) point.x = 0; if (point.x > m_iMaxX) point.x = m_iMaxX;
    if (point.y < 0) point.y = 0; if (point.y > m_iMaxY) point.y = m_iMaxY;
    bMouseMoved = (posX != (float)point.x || posY != (float)point.y);
    posX = (float)point.x;
    posY = (float)point.y;
  }

  // Check if we have an update...
  if (bMouseMoved)
  {
//    posX += (int)((float)cMickeyX * m_fSpeedX); if (posX < 0) posX = 0; if (posX > m_iMaxX) posX = m_iMaxX;
//    posY += (int)((float)cMickeyY * m_fSpeedY); if (posY < 0) posY = 0; if (posY > m_iMaxY) posY = m_iMaxY;
    m_bActive = true;
    dwLastActiveTime = timeGetTime();
  }
  else
  {
    cMickeyX = 0;
    cMickeyY = 0;
    cWheel = 0;
    // check how long we've been inactive
    if (timeGetTime() - dwLastActiveTime > MOUSE_ACTIVE_LENGTH) m_bActive = false;
  }
  // Fill in the public members
  bDown[MOUSE_LEFT_BUTTON] = mouseState.rgbButtons[0] != 0;
  bDown[MOUSE_RIGHT_BUTTON] = mouseState.rgbButtons[1] != 0;
  bDown[MOUSE_MIDDLE_BUTTON] = mouseState.rgbButtons[2] != 0;
  bDown[MOUSE_EXTRA_BUTTON1] = mouseState.rgbButtons[3] != 0;
  bDown[MOUSE_EXTRA_BUTTON2] = false;
  // Perform the click mapping (for single + double click detection)
  bool bNothingDown = true;
  for (int i = 0; i < 5; i++)
  {
    bClick[i] = false;
    bDoubleClick[i] = false;
    bHold[i] = false;
    if (bDown[i])
    {
      bNothingDown = false;
      if (bLastDown[i])
      { // start of hold
        bHold[i] = true;
      }
      else
      {
        if (timeGetTime() - dwLastClickTime[i] < MOUSE_DOUBLE_CLICK_LENGTH)
        { // Double click
          bDoubleClick[i] = true;
        }
        else
        { // Mouse down
        }
      }
    }
    else
    {
      if (bLastDown[i])
      { // Mouse up
        bNothingDown = false;
        bClick[i] = true;
        dwLastClickTime[i] = timeGetTime();
      }
      else
      { // no change
      }
    }
    bLastDown[i] = bDown[i];
  }
  if (bNothingDown)
  { // reset mouse pointer
    SetState(MOUSE_STATE_NORMAL);
  }
}

void CMouse::SetResolution(int iXmax, int iYmax, float fXspeed, float fYspeed)
{
  m_iMaxX = iXmax;
  m_iMaxY = iYmax;
  m_fSpeedX = fXspeed;
  m_fSpeedY = fYspeed;
  // reset the coordinates
  posX = m_iMaxX * 0.5f;
  posY = m_iMaxY * 0.5f;
}

// IsActive - returns true if we have been active in the last MOUSE_ACTIVE_LENGTH period
bool CMouse::IsActive() const
{
  return m_bActive;
}

// turns off mouse activation
void CMouse::SetInactive()
{
  m_bActive = false;
}

bool CMouse::HasMoved() const
{
  return (cMickeyX && cMickeyY);
}

void CMouse::SetExclusiveAccess(DWORD dwControlID, DWORD dwWindowID, const CPoint &point)
{
  m_dwExclusiveControlID = dwControlID;
  m_dwExclusiveWindowID = dwWindowID;
  m_exclusiveOffset = point - CPoint(posX, posY);
}

void CMouse::EndExclusiveAccess(DWORD dwControlID, DWORD dwWindowID)
{
  if (m_dwExclusiveControlID == dwControlID && m_dwExclusiveWindowID == dwWindowID)
    SetExclusiveAccess(WINDOW_INVALID, WINDOW_INVALID, CPoint(posX, posY));
}

void CMouse::Acquire()
{
  if (m_mouse)
    m_mouse->Acquire();
}