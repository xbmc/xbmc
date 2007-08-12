#ifndef SDL_JOYSTICK_H
#define SDL_JOYSTICK_H

#include "../system.h"

#ifdef HAS_SDL_JOYSTICK

// Class to manage all connected joysticks

class CJoystick
{
public:
  CJoystick();

  void Initialize(HWND hwnd);
  void Reset();
  void Update(SDL_Event& event);
  float GetAmount() { return m_Amount; }
  WORD GetButton () { return m_Button; }
  WORD GetAxis () { return m_Axis; }
  string& GetAction () { return m_Action; }

private:
  float m_Amount;
  WORD m_Button;
  WORD m_Axis;
  string m_Action;
  vector<SDL_Joystick*> m_Joysticks;
  vector<string> m_JoystickNames;
};

extern CJoystick g_Joystick;

#endif

#endif
