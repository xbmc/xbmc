#ifndef SDL_JOYSTICK_H
#define SDL_JOYSTICK_H

#include "../system.h"

#ifdef HAS_SDL_JOYSTICK

#define MAX_AXES 64

#define AXIS_POSITIVE 0x00000000
#define AXIS_NEGATIVE 0xf0000000

// Class to manage all connected joysticks

class CJoystick
{
public:
  CJoystick();

  void Initialize(HWND hwnd);
  void Reset(bool axis=false);
  void Update(SDL_Event& event);
  float GetAmount(int axis) { if (axis>=0 && axis<MAX_AXES) return m_Amount[axis]; return 0.0f; }
  float GetAmount() { return m_Amount[m_AxisId]; }
  WORD GetButton () { return m_ButtonId; }
  WORD GetAxis () { return m_AxisId; }
  string GetJoystick() { return (m_JoyId>-1)?m_JoystickNames[m_JoyId]:""; }
  int GetAxisWithMaxAmount();

private:
  float m_Amount[MAX_AXES];
  int m_AxisId;
  int m_ButtonId;
  int m_JoyId;
  int m_NumAxes;
  vector<SDL_Joystick*> m_Joysticks;
  vector<string> m_JoystickNames;
};

extern CJoystick g_Joystick;

#endif

#endif
