#include "SDLMouse.h"
#include "../include.h"
#include "../Key.h"

#ifdef HAS_SDL

CSDLMouse::CSDLMouse()
{
}

CSDLMouse::~CSDLMouse()
{
}

void CSDLMouse::Initialize(void *appData)
{
  SDL_ShowCursor(0);
}

bool CSDLMouse::Update(MouseState &state)
{
  bool bMouseMoved(false);
  int x=0, y=0;
  Uint8 mouseState = SDL_GetRelativeMouseState(&x, &y);
  state.dx = (char)x;
  state.dy = (char)y;
  bMouseMoved = x || y ;
  
  // Check if we have an update...
  if (bMouseMoved)
  {
    mouseState = SDL_GetMouseState(&x, &y);
  
    state.x = x; 
    if (state.x < 0) 
      state.x = 0; 

    state.y = y; 
    if (state.y < 0) 
      state.y = 0; 
  }
  else
  {
    state.dx = 0;
    state.dy = 0;
  }

  // Fill in the public members
  state.button[MOUSE_LEFT_BUTTON] = (mouseState & SDL_BUTTON(1)) == SDL_BUTTON(1);
  state.button[MOUSE_RIGHT_BUTTON] = (mouseState & SDL_BUTTON(3)) == SDL_BUTTON(3);
  state.button[MOUSE_MIDDLE_BUTTON] = (mouseState & SDL_BUTTON(2)) == SDL_BUTTON(2);
  state.button[MOUSE_EXTRA_BUTTON1] = (mouseState & SDL_BUTTON(4)) == SDL_BUTTON(4);
  state.button[MOUSE_EXTRA_BUTTON2] = (mouseState & SDL_BUTTON(5)) == SDL_BUTTON(5);
  
  return bMouseMoved;
}

void CSDLMouse::Acquire()
{
}

#endif
