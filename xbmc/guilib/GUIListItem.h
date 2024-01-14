/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

/*!
\file GUIListItem.h
\brief
*/

#include <map>
#include <memory>
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
  explicit CGUIListItem(const CGUIListItem& item);
  explicit CGUIListItem(const std::string& strLabel);
  virtual ~CGUIListItem(void);
  virtual CGUIListItem* Clone() const { return new CGUIListItem(*this); }

  CGUIListItem& operator =(const CGUIListItem& item);

  virtual void SetLabel(const std::string& strLabel);
  const std::string& GetLabel() const;

  void SetLabel2(const std::string& strLabel);
  const std::string& GetLabel2() const;

  void SetOverlayImage(GUIIconOverlay icon);
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

  bool HasOverlay() const;
  virtual bool IsFileItem() const { return false; }

  void SetLayout(std::unique_ptr<CGUIListItemLayout> layout);
  CGUIListItemLayout *GetLayout();

  void SetFocusedLayout(std::unique_ptr<CGUIListItemLayout> layout);
  CGUIListItemLayout *GetFocusedLayout();

  void FreeIcons();
  void FreeMemory(bool immediately = false);
  void SetInvalid();

  bool m_bIsFolder;     ///< is item a folder or a file

  void SetProperty(const std::string &strKey, const CVariant &value);

  void IncrementProperty(const std::string &strKey, int nVal);
  void IncrementProperty(const std::string& strKey, int64_t nVal);
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
  bool HasProperties() const { return !m_mapProperties.empty(); }
  void       ClearProperty(const std::string &strKey);

  const CVariant &GetProperty(const std::string &strKey) const;

  /*! \brief Set the current item number within it's container
   Our container classes will set this member with the items position
   in the container starting at 1.
   \param position Position of the item in the container starting at 1.
   */
  void SetCurrentItem(unsigned int position);

  /*! \brief Get the current item number within it's container
   Retrieve the items position in a container, this is useful to show
   for example numbering in front of entities in an arbitrary list of entities,
   like songs of a playlist.
   */
  unsigned int GetCurrentItem() const;

protected:
  std::string m_strLabel2;     // text of column2
  GUIIconOverlay m_overlayIcon; // type of overlay icon

  std::unique_ptr<CGUIListItemLayout> m_layout;
  std::unique_ptr<CGUIListItemLayout> m_focusedLayout;
  bool m_bSelected;     // item is selected or not
  unsigned int m_currentItem; // current item number within container (starting at 1)

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

