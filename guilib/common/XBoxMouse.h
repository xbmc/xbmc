#pragma once

#include "Mouse.h"

class CXBoxMouse : public IMouseDevice
{
public:
  CXBoxMouse();
  ~CXBoxMouse();
  virtual void Initialize(void *appData = NULL);
  virtual void Acquire() {};
  virtual bool Update(MouseState &state);
private:
  XINPUT_STATE m_MouseState[4*2];   // one for each port
  HANDLE m_hMouseDevice[4*2];  // handle to each device
  DWORD m_dwLastMousePacket[4*2]; // last packet received from mouse
  DWORD m_dwMousePort; // mask of ports that currently hold a mouse
  XINPUT_STATE m_CurrentState;
};

