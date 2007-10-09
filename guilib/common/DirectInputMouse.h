#pragma once

#include "../system.h"  // only really need dinput.h

#include "Mouse.h"

class CDirectInputMouse : public IMouseDevice
{
public:
  CDirectInputMouse();
  ~CDirectInputMouse();
  virtual void Initialize(void *appData = NULL);
  virtual void Acquire();
  virtual bool Update(MouseState &state);
private:
  LPDIRECTINPUTDEVICE m_mouse;
};
