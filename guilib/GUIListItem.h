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
  CGUIListItem(const CGUIListItem& item);
  CGUIListItem(const CStdString& strLabel);
  virtual ~CGUIListItem(void);

	
  const CGUIListItem& operator =(const CGUIListItem& item);

  void							SetLabel(const CStdString& strLabel);
  const CStdString& GetLabel() const;
  
  void							SetLabel2(const CStdString& strLabel);
  const CStdString& GetLabel2() const;
  
  void							SetIconImage(const CStdString& strIcon);
  const CStdString&	GetIconImage() const;

  void							SetThumbnailImage(const CStdString& strThumbnail);
  const CStdString&	GetThumbnailImage() const;

  void							Select(bool bOnOff);
  bool							IsSelected() const;

  bool							HasIcon() const;
  bool							HasThumbnail() const;
  
  void							SetThumbnail(CGUIImage*	pImage);
  CGUIImage*				GetThumbnail();

  void							SetIcon(CGUIImage*	pImage);
  CGUIImage*				GetIcon();
	void							FreeIcons();
protected:  
  CStdString        m_strLabel;						// text of column1
  CStdString        m_strLabel2;					// text of column2
  CStdString        m_strThumbnailImage;	// filename of thumbnail 
  CStdString        m_strIcon;						// filename of icon
  CGUIImage*        m_pThumbnailImage;		// pointer to CImage containing the thumbnail
  CGUIImage*        m_pIconImage;					// pointer to CImage containing the icon
  bool              m_bSelected;					// item is selected or not
};
#endif