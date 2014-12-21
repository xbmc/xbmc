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

#pragma once

/*!
 \file GUIStaticItem.h
 \brief
 */

#include "GUIInfoTypes.h"
#include "xbmc/FileItem.h"
#include "GUIAction.h"

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
  CGUIStaticItem(const CFileItem &item); // for python
  virtual ~CGUIStaticItem() {};
  virtual CGUIListItem *Clone() const { return new CGUIStaticItem(*this); };
  
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
  typedef std::vector< std::pair<CGUIInfoLabel, std::string> > InfoVector;
  InfoVector m_info;
  INFO::InfoPtr m_visCondition;
  bool m_visState;
  CGUIAction m_clickActions;
};

typedef std::shared_ptr<CGUIStaticItem> CGUIStaticItemPtr;
