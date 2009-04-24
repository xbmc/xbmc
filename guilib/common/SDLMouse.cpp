#include "include.h"
#include "SDLMouse.h"
#include "../Key.h"

#ifdef HAS_SDL

CSDLMouse::CSDLMouse()
{
  m_visibleCursor = NULL;
  m_hiddenCursor = NULL;
}

CSDLMouse::~CSDLMouse()
{
  SDL_SetCursor(m_visibleCursor);
  if (m_hiddenCursor)
    SDL_FreeCursor(m_hiddenCursor);
}

void CSDLMouse::Initialize(void *appData)
{
  // save the current cursor so it can be restored
  m_visibleCursor = SDL_GetCursor();

  // create a transparent cursor
  Uint8 data[8];
  Uint8 mask[8];
  memset(data, 0, sizeof(data));
  memset(mask, 0, sizeof(mask));
  m_hiddenCursor = SDL_CreateCursor(data, mask, 8, 8, 0, 0);
  SDL_SetCursor(m_hiddenCursor);
}

bool CSDLMouse::Update(MouseState &state)
{
  bool bMouseMoved(false);
  int x=0, y=0;
  if (0 == (SDL_GetAppState() & SDL_APPMOUSEFOCUS))
    return false;
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

void CSDLMouse::ShowPointer(bool show)
{
  SDL_SetCursor(show ? m_visibleCursor : m_hiddenCursor);
}

#endif
