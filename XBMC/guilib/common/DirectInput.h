#ifndef DIRECTINPUT_H
#define DIRECTINPUT_H

class CDirectInput
{
public:
  CDirectInput();
  ~CDirectInput();

  HRESULT Initialize(HWND hWnd);
  LPDIRECTINPUT Get() const;

private:
  LPDIRECTINPUT m_lpdi;
  bool m_initialized;
};

extern CDirectInput g_directInput;

#endif
