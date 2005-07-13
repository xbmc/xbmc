#include "include.h"
#include "GUIItem.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

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
