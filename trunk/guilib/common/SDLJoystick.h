#ifndef SDL_JOYSTICK_H
#define SDL_JOYSTICK_H

#include "../system.h" // for HAS_SDL_JOYSTICK
#include <vector>
#include <string>

#define JACTIVE_BUTTON 0x00000001
#define JACTIVE_AXIS   0x00000002
#define JACTIVE_HAT    0x00000004
#define JACTIVE_NONE   0x00000000

#ifdef HAS_SDL_JOYSTICK

#include <SDL/SDL_joystick.h>
#include <SDL/SDL_events.h>

#define MAX_AXES 64


// Class to manage all connected joysticks

class CJoystick
{
public:
  CJoystick();

  void Initialize(HWND hwnd);
  void Reset(bool axis=false);
  void CalibrateAxis(SDL_Joystick *joy);
  void ResetAxis(int axisId) { m_Amount[axisId] = 0; }
  void Update();
  void Update(SDL_Event& event);
  float GetAmount(int axis)
  {
    if (m_Amount[axis]>0)
      return (float)(m_Amount[axis]-m_SafeRange)/(32768.0f-(float)m_SafeRange);
    return (float)(m_Amount[axis]+m_SafeRange)/(32768.0f-(float)m_SafeRange);
  }
  float GetAmount()
  {
    return GetAmount(m_AxisId);
  }
  bool GetButton (int& id, bool consider_repeat=true);
  bool GetAxis (int &id) { if (!IsAxisActive()) return false; id=m_AxisId; return true; }
  bool GetHat (int &id, int &position, bool consider_repeat=true);
  std::string GetJoystick() { return (m_JoyId>-1)?m_JoystickNames[m_JoyId]:""; }
  int GetAxisWithMaxAmount();
  void SetSafeRange(int val) { m_SafeRange=(val>32767)?32767:val; }

private:
  void SetAxisActive(bool active=true) { m_ActiveFlags = active?(m_ActiveFlags|JACTIVE_AXIS):(m_ActiveFlags&(~JACTIVE_AXIS)); }
  void SetButtonActive(bool active=true) { m_ActiveFlags = active?(m_ActiveFlags|JACTIVE_BUTTON):(m_ActiveFlags&(~JACTIVE_BUTTON)); }
  void SetHatActive(bool active=true) { m_ActiveFlags = active?(m_ActiveFlags|JACTIVE_HAT):(m_ActiveFlags&(~JACTIVE_HAT)); }
  bool IsButtonActive() { return (m_ActiveFlags & JACTIVE_BUTTON) == JACTIVE_BUTTON; }
  bool IsAxisActive() { return (m_ActiveFlags & JACTIVE_AXIS) == JACTIVE_AXIS; }
  bool IsHatActive() { return (m_ActiveFlags & JACTIVE_HAT) == JACTIVE_HAT; }

  int m_Amount[MAX_AXES];
  int m_DefaultAmount[MAX_AXES];
  int m_AxisId;
  int m_ButtonId;
  uint8_t m_HatState;
 	int m_HatId; 
  int m_JoyId;
  int m_NumAxes;
  int m_SafeRange; // dead zone
  uint32_t m_pressTicksButton;
  uint32_t m_pressTicksHat;
  uint8_t m_ActiveFlags;
  std::vector<SDL_Joystick*> m_Joysticks;
  std::vector<std::string> m_JoystickNames;
};

extern CJoystick g_Joystick;

#endif

#endif
