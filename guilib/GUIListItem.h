/*!
\file GUIListItem.h
\brief 
*/

#ifndef GUILIB_GUILISTITEM_H
#define GUILIB_GUILISTITEM_H

#pragma once

#include "StdString.h"

#include <map>
#include <string>

//  Forward
class CGUIListItemLayout;

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
                        ICON_OVERLAY_WATCHED,
                        ICON_OVERLAY_HD};

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
  virtual bool IsFileItem() const { return false; };

  void SetLayout(CGUIListItemLayout *layout);
  CGUIListItemLayout *GetLayout();

  void SetFocusedLayout(CGUIListItemLayout *layout);
  CGUIListItemLayout *GetFocusedLayout();

  void FreeIcons();
  void FreeMemory();
  void SetInvalid();

  bool m_bIsFolder;     ///< is item a folder or a file

  void SetProperty(const CStdString &strKey, const char *strValue);
  void SetProperty(const CStdString &strKey, int nVal);
  void SetProperty(const CStdString &strKey, bool bVal);
  void SetProperty(const CStdString &strKey, double dVal);

  bool       HasProperty(const CStdString &strKey) const;

  CStdString GetProperty(const CStdString &strKey) const;
  bool       GetPropertyBOOL(const CStdString &strKey) const;
  int        GetPropertyInt(const CStdString &strKey) const;
  double     GetPropertyDouble(const CStdString &strKey) const;

protected:
  CStdString m_strLabel;      // text of column1
  CStdString m_strLabel2;     // text of column2
  CStdString m_strThumbnailImage; // filename of thumbnail
  CStdString m_strIcon;      // filename of icon
  GUIIconOverlay m_overlayIcon; // type of overlay icon

  CGUIListItemLayout *m_layout;
  CGUIListItemLayout *m_focusedLayout;
  bool m_bSelected;     // item is selected or not

  struct icompare
  {
    bool operator()(const CStdString &s1, const CStdString &s2) const
    {
      return s1.CompareNoCase(s2) < 0;
    }
  };

  std::map<CStdString, CStdString, icompare> m_mapProperties;
};
#endif
