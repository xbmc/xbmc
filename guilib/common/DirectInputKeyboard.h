#ifndef KEYBOARD_H
#define KEYBOARD_H

class CKeyboard
{
public:
  CKeyboard();
  ~CKeyboard();

  void Initialize(HWND hWnd);
  void Acquire();
  void Update();
  bool GetShift() { return m_bShift;};
  bool GetCtrl() { return m_bCtrl;};
  bool GetAlt() { return m_bAlt;};
  char GetAscii() { return m_cAscii;};
  BYTE GetKey() { return m_VKey;};

private:
  inline bool KeyDown(unsigned char key) const { return (m_keystate[key] & 0x80) ? true : false; };
  LPDIRECTINPUTDEVICE m_keyboard;
  unsigned char m_keystate[256];
  unsigned int m_keyDownLastFrame;

  bool m_bShift;
  bool m_bCtrl;
  bool m_bAlt;
  char m_cAscii;
  BYTE m_VKey;

  bool m_bInitialized;
  bool m_bKeyDown;
};

extern CKeyboard g_Keyboard;

#endif

