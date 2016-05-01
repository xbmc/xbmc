/*!
\file GUIListItem.h
\brief
*/

#ifndef GUILIB_GUILISTITEM_H
#define GUILIB_GUILISTITEM_H

#pragma once

/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include <map>
#include <string>

//  Forward
class CGUIListItemLayout;
class CArchive;
class CVariant;

/*!
 \ingroup controls
 \brief
 */
class CGUIListItem
{
public:
  typedef std::map<std::string, std::string> ArtMap;

  ///
  /// @ingroup controls python_xbmcgui_listitem
  /// @defgroup kodi_guilib_listitem_iconoverlay Overlay icon types
  /// @brief Overlay icon types used on list item.
  /// @{
  enum GUIIconOverlay { ICON_OVERLAY_NONE = 0,   //!< Value **0** - No overlay icon
                        ICON_OVERLAY_RAR,        //!< Value **1** - Compressed *.rar files
                        ICON_OVERLAY_ZIP,        //!< Value **2** - Compressed *.zip files
                        ICON_OVERLAY_LOCKED,     //!< Value **3** - Locked files
                        ICON_OVERLAY_UNWATCHED,  //!< Value **4** - For not watched files
                        ICON_OVERLAY_WATCHED,    //!< Value **5** - For seen files
                        ICON_OVERLAY_HD          //!< Value **6** - Is on hard disk stored
                      };
  /// @}

  CGUIListItem(void);
  CGUIListItem(const CGUIListItem& item);
  CGUIListItem(const std::string& strLabel);
  virtual ~CGUIListItem(void);
  virtual CGUIListItem *Clone() const { return new CGUIListItem(*this); };

  CGUIListItem& operator =(const CGUIListItem& item);

  virtual void SetLabel(const std::string& strLabel);
  const std::string& GetLabel() const;

  void SetLabel2(const std::string& strLabel);
  const std::string& GetLabel2() const;

  void SetIconImage(const std::string& strIcon);
  const std::string& GetIconImage() const;

  void SetOverlayImage(GUIIconOverlay icon, bool bOnOff=false);
  std::string GetOverlayImage() const;

  /*! \brief Set a particular art type for an item
   \param type type of art to set.
   \param url the url of the art.
   */
  void SetArt(const std::string &type, const std::string &url);

  /*! \brief set artwork for an item
   \param art a type:url map for artwork
   \sa GetArt
   */
  void SetArt(const ArtMap &art);

  /*! \brief append artwork to an item
   \param art a type:url map for artwork
   \param prefix a prefix for the art, if applicable.
   \sa GetArt
   */
  void AppendArt(const ArtMap &art, const std::string &prefix = "");

  /*! \brief set a fallback image for art
   \param from the type to fallback from
   \param to the type to fallback to
   \sa SetArt
   */
  void SetArtFallback(const std::string &from, const std::string &to);

  /*! \brief clear art on an item
   \sa SetArt
   */
  void ClearArt();

  /*! \brief Get a particular art type for an item
   \param type type of art to fetch.
   \return the art URL, if available, else empty.
   */
  std::string GetArt(const std::string &type) const;

  /*! \brief get artwork for an item
   Retrieves artwork in a type:url map
   \return a type:url map for artwork
   \sa SetArt
   */
  const ArtMap &GetArt() const;

  /*! \brief Check whether an item has a particular piece of art
   Equivalent to !GetArt(type).empty()
   \param type type of art to set.
   \return true if the item has that art set, false otherwise.
   */
  bool HasArt(const std::string &type) const;

  void SetSortLabel(const std::string &label);
  void SetSortLabel(const std::wstring &label);
  const std::wstring &GetSortLabel() const;

  void Select(bool bOnOff);
  bool IsSelected() const;

  bool HasIcon() const;
  bool HasOverlay() const;
  virtual bool IsFileItem() const { return false; };

  void SetLayout(CGUIListItemLayout *layout);
  CGUIListItemLayout *GetLayout();

  void SetFocusedLayout(CGUIListItemLayout *layout);
  CGUIListItemLayout *GetFocusedLayout();

  void FreeIcons();
  void FreeMemory(bool immediately = false);
  void SetInvalid();

  bool m_bIsFolder;     ///< is item a folder or a file

  void SetProperty(const std::string &strKey, const CVariant &value);

  void IncrementProperty(const std::string &strKey, int nVal);
  void IncrementProperty(const std::string &strKey, double dVal);

  void ClearProperties();

  /*! \brief Append the properties of one CGUIListItem to another.
   Any existing properties in the current item will be overridden if they
   are set in the passed in item.
   \param item the item containing the properties to append.
   */
  void AppendProperties(const CGUIListItem &item);

  void Archive(CArchive& ar);
  void Serialize(CVariant& value);

  bool       HasProperty(const std::string &strKey) const;
  bool       HasProperties() const { return !m_mapProperties.empty(); };
  void       ClearProperty(const std::string &strKey);

  const CVariant &GetProperty(const std::string &strKey) const;

protected:
  std::string m_strLabel2;     // text of column2
  std::string m_strIcon;      // filename of icon
  GUIIconOverlay m_overlayIcon; // type of overlay icon

  CGUIListItemLayout *m_layout;
  CGUIListItemLayout *m_focusedLayout;
  bool m_bSelected;     // item is selected or not

  struct icompare
  {
    bool operator()(const std::string &s1, const std::string &s2) const;
  };

  typedef std::map<std::string, CVariant, icompare> PropertyMap;
  PropertyMap m_mapProperties;
private:
  std::wstring m_sortLabel;    // text for sorting. Need to be UTF16 for proper sorting
  std::string m_strLabel;      // text of column1

  ArtMap m_art;
  ArtMap m_artFallbacks;
};
#endif

