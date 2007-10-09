#include "../include.h"
#include "Mouse.h"

#ifdef _XBOX
#include "XBoxMouse.h"
#else
#include "DirectInputMouse.h"
#endif

#include "../key.h"
#include "../GraphicContext.h"

#define MOUSE_DOUBLE_CLICK_LENGTH 500L
#define MOUSE_ACTIVE_LENGTH   5000L

CMouse g_Mouse; // global

CMouse::CMouse()
{
  m_mouseDevice = NULL;
  m_exclusiveWindowID = WINDOW_INVALID;
  m_exclusiveControlID = WINDOW_INVALID;
  m_pointerState = MOUSE_STATE_NORMAL;
  memset(&m_mouseState, 0, sizeof(m_mouseState));
}

CMouse::~CMouse()
{
  if (m_mouseDevice)
    delete m_mouseDevice;
}

void CMouse::Initialize(void *appData)
{
  if (m_mouseDevice)
    return; // nothing to do

  // create the mouse device
#ifdef _XBOX
  m_mouseDevice = new CXBoxMouse();
#else
  m_mouseDevice = new CDirectInputMouse();
#endif

  if (m_mouseDevice)
    m_mouseDevice->Initialize(appData);

  // Set the default resolution (PAL)
  SetResolution(720, 576, 1, 1);
}

void CMouse::Update()
{
  // update our state from the mouse device
  if (m_mouseDevice && m_mouseDevice->Update(m_mouseState))
  {
    // check our position is not out of bounds
    if (m_mouseState.x < 0) m_mouseState.x = 0;
    if (m_mouseState.y < 0) m_mouseState.y = 0;
    if (m_mouseState.x > m_maxX) m_mouseState.x = m_maxX;
    if (m_mouseState.y > m_maxY) m_mouseState.y = m_maxY;
    m_mouseState.active = true;
    m_lastActiveTime = timeGetTime();
  }
  else
  {
    m_mouseState.dx = 0;
    m_mouseState.dy = 0;
    m_mouseState.dz = 0;
    // check how long we've been inactive
    if (timeGetTime() - m_lastActiveTime > MOUSE_ACTIVE_LENGTH)
      m_mouseState.active = false;
  }

  // Perform the click mapping (for single + double click detection)
  bool bNothingDown = true;
  for (int i = 0; i < 5; i++)
  {
    bClick[i] = false;
    bDoubleClick[i] = false;
    bHold[i] = false;
    if (m_mouseState.button[i])
    {
      bNothingDown = false;
      if (m_lastDown[i])
      { // start of hold
        bHold[i] = true;
      }
      else
      {
        if (timeGetTime() - m_lastClickTime[i] < MOUSE_DOUBLE_CLICK_LENGTH)
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
      if (m_lastDown[i])
      { // Mouse up
        bNothingDown = false;
        bClick[i] = true;
        m_lastClickTime[i] = timeGetTime();
      }
      else
      { // no change
      }
    }
    m_lastDown[i] = m_mouseState.button[i];
  }
  if (bNothingDown)
  { // reset mouse pointer
    SetState(MOUSE_STATE_NORMAL);
  }
}

void CMouse::SetResolution(int maxX, int maxY, float speedX, float speedY)
{
  m_maxX = maxX;
  m_maxY = maxY;

  // speed is currently unused
  m_speedX = speedX;
  m_speedY = speedY;

  // reset the coordinates
  m_mouseState.x = m_maxX / 2;
  m_mouseState.y = m_maxY / 2;
}

// IsActive - returns true if we have been active in the last MOUSE_ACTIVE_LENGTH period
bool CMouse::IsActive() const
{
  return m_mouseState.active;
}

// turns off mouse activation
void CMouse::SetInactive()
{
  m_mouseState.active = false;
}

bool CMouse::HasMoved() const
{
  return (m_mouseState.dx || m_mouseState.dy);
}

CPoint CMouse::GetLocation() const
{
  return CPoint((float)m_mouseState.x, (float)m_mouseState.y);
}

void CMouse::SetLocation(CPoint &point)
{
  m_mouseState.x = (int)point.x;
  m_mouseState.y = (int)point.y;
}

CPoint CMouse::GetLastMove() const
{
  return CPoint(m_mouseState.dx, m_mouseState.dy);
}

char CMouse::GetWheel() const
{
  return m_mouseState.dz;
}

void CMouse::SetExclusiveAccess(DWORD dwControlID, DWORD dwWindowID, const CPoint &point)
{
  m_exclusiveControlID = dwControlID;
  m_exclusiveWindowID = dwWindowID;
  // convert posX, posY to screen coords...
  // NOTE: This relies on the window resolution having been set correctly beforehand in CGUIWindow::OnMouseAction()
  CPoint mouseCoords(GetLocation());
  g_graphicsContext.InvertFinalCoords(mouseCoords.x, mouseCoords.y);
  m_exclusiveOffset = point - mouseCoords;
}

void CMouse::EndExclusiveAccess(DWORD dwControlID, DWORD dwWindowID)
{
  if (m_exclusiveControlID == dwControlID && m_exclusiveWindowID == dwWindowID)
    SetExclusiveAccess(WINDOW_INVALID, WINDOW_INVALID, CPoint(0, 0));
}

void CMouse::Acquire()
{
  if (m_mouseDevice)
    m_mouseDevice->Acquire();
}