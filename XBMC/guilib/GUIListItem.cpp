#include "guilistitem.h"

CGUIListItem::CGUIListItem(void)
{
  m_strLabel2="";
  m_strLabel="";
  m_bImage=false;
  m_pImage=NULL;
}

CGUIListItem::CGUIListItem(bool bHasImage, const CStdString& strLabel)
{
  m_strLabel2="";
  m_bImage=bHasImage;
  m_strLabel=strLabel;
  m_pImage=NULL;
}

CGUIListItem::~CGUIListItem(void)
{
  if (m_pImage) 
  {
    m_pImage->FreeResources();
    delete m_pImage;
    m_pImage=NULL;
  }
}


const CStdString& CGUIListItem::GetLabel() const
{
  return m_strLabel;
}


const CStdString& CGUIListItem::GetLabel2() const
{
  return m_strLabel2;
}

bool CGUIListItem::HasImage() const
{
  return m_bImage;
}