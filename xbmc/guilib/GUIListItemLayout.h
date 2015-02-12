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

#include "GUIListGroup.h"
#include "GUITexture.h"
#include "GUIInfoTypes.h"

class CGUIListItem;
class CFileItem;
class CLabelInfo;

class CGUIListItemLayout
{
public:
  CGUIListItemLayout();
  CGUIListItemLayout(const CGUIListItemLayout &from);
  virtual ~CGUIListItemLayout();
  void LoadLayout(TiXmlElement *layout, int context, bool focused);
  void Process(CGUIListItem *item, int parentID, unsigned int currentTime, CDirtyRegionList &dirtyregions);
  void Render(CGUIListItem *item, int parentID);
  float Size(ORIENTATION orientation) const;
  unsigned int GetFocusedItem() const;
  void SetFocusedItem(unsigned int focus);
  bool IsAnimating(ANIMATION_TYPE animType);
  void ResetAnimation(ANIMATION_TYPE animType);
  void SetInvalid() { m_invalidated = true; };
  void FreeResources(bool immediately = false);

//#ifdef GUILIB_PYTHON_COMPATIBILITY
  void CreateListControlLayouts(float width, float height, bool focused, const CLabelInfo &labelInfo, const CLabelInfo &labelInfo2, const CTextureInfo &texture, const CTextureInfo &textureFocus, float texHeight, float iconWidth, float iconHeight, const std::string &nofocusCondition, const std::string &focusCondition);
//#endif

  void SetWidth(float width);
  void SetHeight(float height);
  void SelectItemFromPoint(const CPoint &point);
  bool MoveLeft();
  bool MoveRight();

#ifdef _DEBUG
  virtual void DumpTextureUse();
#endif
  bool CheckCondition();
protected:
  void LoadControl(TiXmlElement *child, CGUIControlGroup *group);
  void Update(CFileItem *item);

  CGUIListGroup m_group;

  float m_width;
  float m_height;
  bool m_focused;
  bool m_invalidated;

  INFO::InfoPtr m_condition;
  CGUIInfoBool m_isPlaying;
};

