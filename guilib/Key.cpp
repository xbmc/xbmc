#include "stdafx.h"
#include "key.h"

CKey::CKey(void)
{
	m_dwButtonCode = KEY_INVALID;
	m_bLeftTrigger = 0;
	m_bRightTrigger = 0;
	m_fLeftThumbX	 = 0.0f;
	m_fLeftThumbY	 = 0.0f;
	m_fRightThumbX = 0.0f;
	m_fRightThumbY = 0.0f;
}

CKey::~CKey(void)
{
}

CKey::CKey(DWORD dwButtonCode, BYTE bLeftTrigger, BYTE bRightTrigger, float fLeftThumbX, float fLeftThumbY, float fRightThumbX, float fRightThumbY)
{
	m_bLeftTrigger = bLeftTrigger;
	m_bRightTrigger = bRightTrigger;
	m_fLeftThumbX	 = fLeftThumbX;
	m_fLeftThumbY	 = fLeftThumbY;
	m_fRightThumbX = fRightThumbX;
	m_fRightThumbY = fRightThumbY;
  m_dwButtonCode = dwButtonCode;
}

CKey::CKey(const CKey& key)
{
  *this=key;
}

DWORD CKey::GetButtonCode() const
{
  return m_dwButtonCode;
}
const CKey& CKey::operator=(const CKey& key)
{
	if (&key==this) return *this;
	m_bLeftTrigger = key.m_bLeftTrigger;
	m_bRightTrigger = key.m_bRightTrigger;
	m_dwButtonCode	= key.m_dwButtonCode;
	m_fLeftThumbX	  = key.m_fLeftThumbX;
	m_fLeftThumbY		= key.m_fLeftThumbY;
	m_fRightThumbX	= key.m_fRightThumbX;
	m_fRightThumbY	= key.m_fRightThumbY;
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

bool CKey::FromKeyboard() const
{
	return (m_dwButtonCode>=KEY_VKEY && m_dwButtonCode != KEY_INVALID);
}