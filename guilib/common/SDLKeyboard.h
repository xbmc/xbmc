#ifndef SDL_KEYBOARD_H
#define SDL_KEYBOARD_H

#include "../system.h"

#ifdef HAS_SDL

class CLowLevelKeyboard
{
public:
  CLowLevelKeyboard();

  void Initialize(HWND hwnd);
  void Reset();
  void Update(SDL_Event& event);
  bool GetShift() { return m_bShift;};
  bool GetCtrl() { return m_bCtrl;};
  bool GetAlt() { return m_bAlt;};
  bool GetRAlt() { return m_bRAlt;};
  char GetAscii() { return m_cAscii;}; // FIXME should be replaced completly by GetUnicode() 
  WCHAR GetUnicode() { return m_wUnicode;};
  BYTE GetKey() { return m_VKey;};

private:
  bool m_bShift;
  bool m_bCtrl;
  bool m_bAlt;
  bool m_bRAlt;
  char m_cAscii;
  WCHAR m_wUnicode;
  BYTE m_VKey;
#ifdef HAS_SDL_JOYSTICK
  SDL_Joystick* m_pJoy;
#endif
};

#endif

#endif
