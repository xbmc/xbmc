#include "include.h"
#include "DirectInput.h"

CDirectInput g_directInput;

CDirectInput::CDirectInput()
{
  m_lpdi = NULL;
  m_initialized = false;
}

CDirectInput::~CDirectInput()
{
  if (m_lpdi)
    m_lpdi->Release();
  m_lpdi = NULL;
}

HRESULT CDirectInput::Initialize(HWND hWnd)
{
  if (m_initialized)
    return S_OK;

  if (FAILED(DirectInput8Create(GetModuleHandle(NULL), DIRECTINPUT_VERSION, 
    IID_IDirectInput8, (void**)&m_lpdi, NULL)))
    return -1;
  
  m_initialized = true;
  return S_OK;
}

LPDIRECTINPUT CDirectInput::Get() const
{
  return m_lpdi;
}
