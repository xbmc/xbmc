#include "include.h"
#include "GUIItem.h"

CGUIItem::CGUIItem(CStdString& aItemName)
{
  m_strName = aItemName;
  m_dwCookie = 0;
}

CGUIItem::~CGUIItem(void)
{}

CStdString CGUIItem::GetName()
{
  return m_strName;
}

void CGUIItem::SetCookie(DWORD aCookie)
{
  m_dwCookie = aCookie;
}

DWORD CGUIItem::GetCookie()
{
  return m_dwCookie;
}
