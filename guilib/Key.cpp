#include "include.h"
#include "Key.h"


CKey::CKey(void)
{
  m_dwButtonCode = KEY_INVALID;
  m_bLeftTrigger = 0;
  m_bRightTrigger = 0;
  m_fLeftThumbX = 0.0f;
  m_fLeftThumbY = 0.0f;
  m_fRightThumbX = 0.0f;
  m_fRightThumbY = 0.0f;
  m_fRepeat = 0.0f;
  m_fromHttpApi = false;
}

CKey::~CKey(void)
{}

CKey::CKey(DWORD dwButtonCode, BYTE bLeftTrigger, BYTE bRightTrigger, float fLeftThumbX, float fLeftThumbY, float fRightThumbX, float fRightThumbY, float fRepeat)
{
  m_bLeftTrigger = bLeftTrigger;
  m_bRightTrigger = bRightTrigger;
  m_fLeftThumbX = fLeftThumbX;
  m_fLeftThumbY = fLeftThumbY;
  m_fRightThumbX = fRightThumbX;
  m_fRightThumbY = fRightThumbY;
  m_dwButtonCode = dwButtonCode;
  m_fRepeat = fRepeat;
  m_fromHttpApi = false;
}

CKey::CKey(const CKey& key)
{
  *this = key;
}

DWORD CKey::GetButtonCode() const // for backwards compatibility only
{
  return m_dwButtonCode;
}

DWORD CKey::GetUnicode() const
{  
  if (m_dwButtonCode>=KEY_ASCII && m_dwButtonCode < KEY_UNICODE) // will need to change when Unicode is fully implemented
    return m_dwButtonCode-KEY_ASCII;
  else
    return 0;
}

const CKey& CKey::operator=(const CKey& key)
{
  if (&key == this) return * this;
  m_bLeftTrigger = key.m_bLeftTrigger;
  m_bRightTrigger = key.m_bRightTrigger;
  m_dwButtonCode = key.m_dwButtonCode;
  m_fLeftThumbX = key.m_fLeftThumbX;
  m_fLeftThumbY = key.m_fLeftThumbY;
  m_fRightThumbX = key.m_fRightThumbX;
  m_fRightThumbY = key.m_fRightThumbY;
  m_fRepeat = key.m_fRepeat;
  m_fromHttpApi = key.m_fromHttpApi;
  return *this;
}

BYTE CKey::GetLeftTrigger() const
{
  return m_bLeftTrigger;
}

BYTE CKey::GetRightTrigger() const
{
  return m_bRightTrigger;
}

float CKey::GetLeftThumbX() const
{
  return m_fLeftThumbX;
}

float CKey::GetLeftThumbY() const
{
  return m_fLeftThumbY;
}


float CKey::GetRightThumbX() const
{
  return m_fRightThumbX;
}

float CKey::GetRightThumbY() const
{
  return m_fRightThumbY;
}

bool CKey::FromKeyboard() const
{
  return (m_dwButtonCode >= KEY_VKEY && m_dwButtonCode != KEY_INVALID);
}

bool CKey::IsAnalogButton() const
{
  if ((GetButtonCode() > 261 && GetButtonCode() < 270) || (GetButtonCode() > 279 && GetButtonCode() < 284))
    return true;

  return false;
}

bool CKey::IsIRRemote() const
{
  if (GetButtonCode() < 256)
    return true;
  return false;
}

float CKey::GetRepeat() const
{
  return m_fRepeat;
}

bool CKey::GetFromHttpApi() const
{
  return m_fromHttpApi;
}

void CKey::SetFromHttpApi(bool bFromHttpApi)
{
	m_fromHttpApi = bFromHttpApi;
}

