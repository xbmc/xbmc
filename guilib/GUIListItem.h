/*!
\file GUIListItem.h
\brief 
*/

#ifndef GUILIB_GUILISTITEM_H
#define GUILIB_GUILISTITEM_H

#pragma once

#include "stdstring.h"

//  Forward
class CGUIImage;

/*!
 \ingroup controls
 \brief 
 */
class CGUIListItem
{
public:
  enum GUIIconOverlay { ICON_OVERLAY_NONE = 0,
                        ICON_OVERLAY_RAR,
                        ICON_OVERLAY_ZIP,
                        ICON_OVERLAY_LOCKED,
                        ICON_OVERLAY_HAS_TRAINER,
                        ICON_OVERLAY_TRAINED,
                        ICON_OVERLAY_UNWATCHED,
                        ICON_OVERLAY_WATCHED};

  CGUIListItem(void);
  CGUIListItem(const CGUIListItem& item);
  CGUIListItem(const CStdString& strLabel);
  virtual ~CGUIListItem(void);


  const CGUIListItem& operator =(const CGUIListItem& item);

  virtual void SetLabel(const CStdString& strLabel);
  const CStdString& GetLabel() const;

  void SetLabel2(const CStdString& strLabel);
  const CStdString& GetLabel2() const;

  void SetIconImage(const CStdString& strIcon);
  const CStdString& GetIconImage() const;

  void SetThumbnailImage(const CStdString& strThumbnail);
  const CStdString& GetThumbnailImage() const;

  void SetOverlayImage(GUIIconOverlay icon, bool bOnOff=false);
  CStdString GetOverlayImage() const;

  void Select(bool bOnOff);
  bool IsSelected() const;

  bool HasIcon() const;
  bool HasThumbnail() const;
  bool HasOverlay() const;

  void SetThumbnail(CGUIImage* pImage);
  CGUIImage* GetThumbnail();

  void SetIcon(CGUIImage* pImage);
  CGUIImage* GetIcon();

  void SetOverlay(CGUIImage* pImage);
  CGUIImage* GetOverlay();

  void FreeIcons();
  void FreeMemory();

  bool m_bIsFolder;     ///< is item a folder or a file
protected:
  CStdString m_strLabel;      // text of column1
  CStdString m_strLabel2;     // text of column2
  CStdString m_strThumbnailImage; // filename of thumbnail
  CStdString m_strIcon;      // filename of icon
  GUIIconOverlay m_overlayIcon; // type of overlay icon
  CGUIImage* m_pThumbnailImage;  // pointer to CImage containing the thumbnail
  CGUIImage* m_pIconImage;     // pointer to CImage containing the icon
  CGUIImage* m_overlayImage;    // CGUIImage containing the transparent overlay icon
  bool m_bSelected;     // item is selected or not
};
#endif
