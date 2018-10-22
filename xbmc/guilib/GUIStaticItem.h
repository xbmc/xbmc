/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

/*!
 \file GUIStaticItem.h
 \brief
 */

#include <utility>
#include <vector>

#include "GUIAction.h"
#include "guilib/guiinfo/GUIInfoLabel.h"
#include "interfaces/info/InfoBool.h"
#include "FileItem.h"

class TiXmlElement;

/*!
 \ingroup lists,items
 \brief wrapper class for a static item in a list container

 A wrapper class for the items in a container specified via the <content>
 flag.  Handles constructing items from XML and updating item labels, icons
 and properties.

 \sa CFileItem, CGUIBaseContainer
 */
class CGUIStaticItem : public CFileItem
{
public:
  /*! \brief constructor
   Construct an item based on an XML description:
     <item>
       <label>$INFO[MusicPlayer.Artist]</label>
       <label2>$INFO[MusicPlayer.Album]</label2>
       <thumb>bar.png</thumb>
       <icon>foo.jpg</icon>
       <onclick>ActivateWindow(Home)</onclick>
     </item>

   \param element XML element to construct from
   \param contextWindow window context to use for any info labels
   */
  CGUIStaticItem(const TiXmlElement *element, int contextWindow);
  explicit CGUIStaticItem(const CFileItem &item); // for python
  ~CGUIStaticItem() override = default;
  CGUIListItem *Clone() const override { return new CGUIStaticItem(*this); };

  /*! \brief update any infolabels in the items properties
   Runs through all the items properties, updating any that should be
   periodically recomputed
   \param contextWindow window context to use for any info labels
   */
  void UpdateProperties(int contextWindow);

  /*! \brief update visibility of this item
   \param contextWindow window context to use for any info labels
   \return true if visible state has changed, false otherwise
   */
  bool UpdateVisibility(int contextWindow);

  /*! \brief whether this item is visible or not
   */
  bool IsVisible() const;

  /*! \brief set a visible condition for this item.
   \param condition the condition to use.
   \param context the context for the condition (typically a window id).
   */
  void SetVisibleCondition(const std::string &condition, int context);

  const CGUIAction &GetClickActions() const { return m_clickActions; };
private:
  typedef std::vector< std::pair<KODI::GUILIB::GUIINFO::CGUIInfoLabel, std::string> > InfoVector;
  InfoVector m_info;
  INFO::InfoPtr m_visCondition;
  bool m_visState;
  CGUIAction m_clickActions;
};

typedef std::shared_ptr<CGUIStaticItem> CGUIStaticItemPtr;
