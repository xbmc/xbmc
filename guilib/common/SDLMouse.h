#ifndef SDL_MOUSE_H
#define SDL_MOUSE_H

#include "../system.h"
#include "../Geometry.h"
#include "Mouse.h"

#ifdef HAS_SDL

#define MOUSE_DOUBLE_CLICK_LENGTH 500L
#define MOUSE_ACTIVE_LENGTH   5000L

// states
// button ids
#define MOUSE_LEFT_BUTTON 0
#define MOUSE_RIGHT_BUTTON 1
#define MOUSE_MIDDLE_BUTTON 2
#define MOUSE_EXTRA_BUTTON1 3
#define MOUSE_EXTRA_BUTTON2 4

class CSDLMouse : public IMouseDevice
{
public:
  CSDLMouse();
  virtual ~CSDLMouse();

  virtual void Initialize(void *appData = NULL);
  virtual void Acquire();
  virtual bool Update(MouseState &state);

private:

};
#endif

#endif
