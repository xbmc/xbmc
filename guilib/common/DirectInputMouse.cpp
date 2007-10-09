#include "../include.h"
#include "DirectInputMouse.h"
#include "DirectInput.h"

#include "../../Tools/Win32/XBMC_PC.h"
extern CXBMC_PC *g_xbmcPC;

CDirectInputMouse::CDirectInputMouse()
{
  m_mouse = NULL;
}

CDirectInputMouse::~CDirectInputMouse()
{
  if (m_mouse)
    m_mouse->Release();
}

void CDirectInputMouse::Initialize(void *appData)
{
  ASSERT(appData);

  HWND hWnd = *(HWND *)appData;

  if (FAILED(g_directInput.Initialize(hWnd)))
    return;

  if (FAILED(g_directInput.Get()->CreateDevice(GUID_SysMouse, &m_mouse, NULL)))
    return;
  
  if (FAILED(m_mouse->SetDataFormat(&c_dfDIMouse)))
    return;
  
  if (FAILED(m_mouse->SetCooperativeLevel(hWnd, DISCL_FOREGROUND | DISCL_NONEXCLUSIVE)))
    return;
}

bool CDirectInputMouse::Update(MouseState &state)
{
  bool bMouseMoved(false);
  DIMOUSESTATE mouseState;
  memset(&mouseState, 0, sizeof(mouseState));
  if (S_OK == m_mouse->GetDeviceState(sizeof(DIMOUSESTATE), (LPVOID)&mouseState))
  {
    state.dx = (char)mouseState.lX;
    state.dy = (char)mouseState.lY;
    state.dz = (char)mouseState.lZ;
    POINT point;
    if (!g_xbmcPC->GetCursorPos(point))
    { // outside our client rect
      state.active = false;
      return false;
    }
    bMouseMoved = (state.x != (float)point.x || state.y != (float)point.y);
    state.x = point.x;
    state.y = point.y;

    state.button[MOUSE_LEFT_BUTTON] = mouseState.rgbButtons[0] != 0;
    state.button[MOUSE_RIGHT_BUTTON] = mouseState.rgbButtons[1] != 0;
    state.button[MOUSE_MIDDLE_BUTTON] = mouseState.rgbButtons[2] != 0;
    state.button[MOUSE_EXTRA_BUTTON1] = mouseState.rgbButtons[3] != 0;
    state.button[MOUSE_EXTRA_BUTTON2] = false;
  }
  return bMouseMoved;
}

void CDirectInputMouse::Acquire()
{
  if (m_mouse)
    m_mouse->Acquire();
}
