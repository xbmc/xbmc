#ifndef DINPUT_KEYBOARD_H
#define DINPUT_KEYBOARD_H

#ifndef HAS_SDL

class CLowLevelKeyboard
{
public:
  CLowLevelKeyboard();
  ~CLowLevelKeyboard();

  void Initialize(HWND hWnd);
  void Acquire();
  void Update();
  bool GetShift() { return m_bShift;};
  bool GetCtrl() { return m_bCtrl;};
  bool GetAlt() { return m_bAlt;};
  bool GetRAlt() { return m_bRAlt;};
  char GetAscii() { return m_cAscii;}; // FIXME should be replaced completly by GetUnicode() 
  WCHAR GetUnicode() { return GetAscii();}; // FIXME HELPME is there any unicode feature available?
  BYTE GetKey() { return m_VKey;};

private:
  inline bool KeyDown(unsigned char key) const { return (m_keystate[key] & 0x80) ? true : false; };
  LPDIRECTINPUTDEVICE m_keyboard;
  unsigned char m_keystate[256];
  int m_keyDownLastFrame;

  bool m_bShift;
  bool m_bCtrl;
  bool m_bAlt;
  bool m_bRAlt;
  char m_cAscii;
  BYTE m_VKey;

  bool m_bInitialized;
  bool m_bKeyDown;
};

#endif

#endif
