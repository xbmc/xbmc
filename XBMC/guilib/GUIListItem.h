#ifndef GUILIB_GUILISTITEM_H
#define GUILIB_GUILISTITEM_H

#pragma once

#include "gui3d.h"
#include "guiimage.h"
#include <string>
using namespace std;

class CGUIListItem
{
public:
  CGUIListItem(void);
  CGUIListItem(bool bHasImage, const string& strLabel);
  virtual ~CGUIListItem(void);
  const string& GetLabel() const;
  const string& GetLabel2() const;
  bool          HasImage() const;
  string        m_strLabel;
  string        m_strLabel2;
  string        m_strThumbnailImage;
  bool          m_bImage;
  CGUIImage*    m_pImage;
};
#endif