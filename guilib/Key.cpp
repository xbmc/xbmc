#include "key.h"

CKey::CKey(void)
{
	m_bIsButton		 = false;
	m_dwButtonCode = KEY_INVALID;
	m_fLeftThumbX	 = 0.0f;
	m_fLeftThumbY	 = 0.0f;
	m_fRightThumbX = 0.0f;
	m_fRightThumbY = 0.0f;
}

CKey::~CKey(void)
{
}

CKey::CKey(bool bButton, DWORD dwButtonCode, float fLeftThumbX, float fLeftThumbY, float fRightThumbX, float fRightThumbY)
{
	m_fLeftThumbX	 = fLeftThumbX;
	m_fLeftThumbY	 = fLeftThumbY;
	m_fRightThumbX = fRightThumbX;
	m_fRightThumbY = fRightThumbY;
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
	m_fLeftThumbX	  = key.m_fLeftThumbX;
	m_fLeftThumbY		= key.m_fLeftThumbY;
	m_fRightThumbX	= key.m_fRightThumbX;
	m_fRightThumbY	= key.m_fRightThumbY;
  return *this;
}

float CKey::GetLeftThumbX() const
{
	return m_fLeftThumbX;
}

float	CKey::GetLeftThumbY() const
{
	return m_fLeftThumbY;
}


float CKey::GetRightThumbX() const
{
	return m_fRightThumbX;
}

float	CKey::GetRightThumbY() const
{
	return m_fRightThumbY;
}
