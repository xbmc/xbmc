#include "key.h"

CKey::CKey(void)
{
}

CKey::~CKey(void)
{
}

CKey::CKey(bool bButton, DWORD dwButtonCode)
{
  m_bIsButton = bButton;
  m_dwButtonCode=dwButtonCode;
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
  m_bIsButton=key.m_bIsButton;
  m_dwButtonCode=key.m_dwButtonCode;
  return *this;
}
