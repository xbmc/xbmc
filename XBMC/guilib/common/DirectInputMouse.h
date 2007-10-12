#ifndef DINPUT_MOUSE_H
#define DINPUT_MOUSE_H

#ifndef HAS_SDL

#include "../system.h"  // only really need dinput.h

#define MOUSE_DOUBLE_CLICK_LENGTH 500L
#define MOUSE_ACTIVE_LENGTH   5000L
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
#endif

#endif

