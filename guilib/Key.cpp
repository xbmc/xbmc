#include "key.h"

CKey::CKey(void)
{
	m_bIsButton		 = false;
	m_dwButtonCode = KEY_INVALID;
	m_fThumbX			 = 0.0f;
	m_fThumbY			 = 0.0f;
}

CKey::~CKey(void)
{
}

CKey::CKey(bool bButton, DWORD dwButtonCode, float fThumbX, float fThumbY)
{
	m_fThumbX			 = fThumbX;
	m_fThumbY			 = fThumbY;
  m_bIsButton    = bButton;
  m_dwButtonCode = dwButtonCode;
}

CKey::CKey(const CKey& key)
{
  *this=key;
}

bool CKey::IsButton() const
{
  return m_bIsButton;
}

DWORD CKey::GetButtonCode() const
{
  return m_dwButtonCode;
}
const CKey& CKey::operator=(const CKey& key)
{
  if (&key==this) return *this;
  m_bIsButton			= key.m_bIsButton;
  m_dwButtonCode	= key.m_dwButtonCode;
	m_fThumbX				= key.m_fThumbX;
	m_fThumbY				= key.m_fThumbY;
  return *this;
}

float CKey::GetThumbX() const
{
	return m_fThumbX;
}

float	CKey::GetThumbY() const
{
	return m_fThumbY;
}
