#ifndef GUILIB_GUILISTITEM_H
#define GUILIB_GUILISTITEM_H

#pragma once

#include "gui3d.h"
#include "guiimage.h"
#include "stdstring.h"
using namespace std;

class CGUIListItem
{
public:
  CGUIListItem(void);
  CGUIListItem(bool bHasImage, const CStdString& strLabel);
  virtual ~CGUIListItem(void);
  const CStdString& GetLabel() const;
  const CStdString& GetLabel2() const;
  bool          HasImage() const;
  CStdString        m_strLabel;
  CStdString        m_strLabel2;
  CStdString        m_strThumbnailImage;
  bool          m_bImage;
  CGUIImage*    m_pImage;
};
#endif