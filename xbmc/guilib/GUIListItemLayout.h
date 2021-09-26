/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "GUIListGroup.h"
#include "GUITexture.h"
#include "guilib/guiinfo/GUIInfoLabel.h"

class CGUIListItem;
class CFileItem;
class CLabelInfo;

class CGUIListItemLayout final
{
public:
  CGUIListItemLayout();
  CGUIListItemLayout(const CGUIListItemLayout &from, CGUIControl *control);
  void LoadLayout(TiXmlElement *layout, int context, bool focused, float maxWidth, float maxHeight);
  void Process(CGUIListItem *item, int parentID, unsigned int currentTime, CDirtyRegionList &dirtyregions);
  void Render(CGUIListItem *item, int parentID);
  float Size(ORIENTATION orientation) const;
  unsigned int GetFocusedItem() const;
  void SetFocusedItem(unsigned int focus);
  bool IsAnimating(ANIMATION_TYPE animType);
  void ResetAnimation(ANIMATION_TYPE animType);
  void SetInvalid() { m_invalidated = true; }
  void FreeResources(bool immediately = false);
  void SetParentControl(CGUIControl* control) { m_group.SetParentControl(control); }

  //#ifdef GUILIB_PYTHON_COMPATIBILITY
  void CreateListControlLayouts(float width, float height, bool focused, const CLabelInfo &labelInfo, const CLabelInfo &labelInfo2, const CTextureInfo &texture, const CTextureInfo &textureFocus, float texHeight, float iconWidth, float iconHeight, const std::string &nofocusCondition, const std::string &focusCondition);
//#endif

  void SetWidth(float width);
  void SetHeight(float height);
  void SelectItemFromPoint(const CPoint &point);
  bool MoveLeft();
  bool MoveRight();

#ifdef _DEBUG
  void DumpTextureUse();
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
  KODI::GUILIB::GUIINFO::CGUIInfoBool m_isPlaying;
};

