#include "fileitem.h"

CFileItem::CFileItem(void)
{
  m_dwSize=0;
  m_bIsFolder=false;
  m_bIsShareOrDrive=false;
	memset(&m_stTime,0,sizeof(m_stTime));
  
}


CFileItem::CFileItem(const CStdString& strLabel)
:CGUIListItem(strLabel)
{
  m_dwSize=0;
	memset(&m_stTime,0,sizeof(m_stTime));
}

CFileItem::~CFileItem(void)
{
}
