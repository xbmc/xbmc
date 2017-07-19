/*!
\file GUIControlGroup.h
\brief
*/

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

#include <vector>

#include "GUIControlLookup.h"

/*!
 \ingroup controls
 \brief group of controls, useful for remembering last control + animating/hiding together
 */
class CGUIControlGroup : public CGUIControlLookup
{
public:
  CGUIControlGroup();
  CGUIControlGroup(int parentID, int controlID, float posX, float posY, float width, float height);
  CGUIControlGroup(const CGUIControlGroup &from);
  ~CGUIControlGroup(void) override;
  CGUIControlGroup *Clone() const override { return new CGUIControlGroup(*this); };

  void Process(unsigned int currentTime, CDirtyRegionList &dirtyregions) override;
  void Render() override;
  void RenderEx() override;
  bool OnAction(const CAction &action) override;
  bool OnMessage(CGUIMessage& message) override;
  virtual bool SendControlMessage(CGUIMessage& message);
  bool HasFocus() const override;
  void AllocResources() override;
  void FreeResources(bool immediately = false) override;
  void DynamicResourceAlloc(bool bOnOff) override;
  bool CanFocus() const override;

  EVENT_RESULT SendMouseEvent(const CPoint &point, const CMouseEvent &event) override;
  void UnfocusFromPoint(const CPoint &point) override;

  void SetInitialVisibility() override;

  bool IsAnimating(ANIMATION_TYPE anim) override;
  bool HasAnimation(ANIMATION_TYPE anim) override;
  void QueueAnimation(ANIMATION_TYPE anim) override;
  void ResetAnimation(ANIMATION_TYPE anim) override;
  void ResetAnimations() override;

  int GetFocusedControlID() const;
  CGUIControl *GetFocusedControl() const;
  virtual CGUIControl *GetFirstFocusableControl(int id);

  virtual void AddControl(CGUIControl *control, int position = -1);
  bool InsertControl(CGUIControl *control, const CGUIControl *insertPoint);
  virtual bool RemoveControl(const CGUIControl *control);
  virtual void ClearAll();
  void SetDefaultControl(int id, bool always) { m_defaultControl = id; m_defaultAlways = always; };
  void SetRenderFocusedLast(bool renderLast) { m_renderFocusedLast = renderLast; };

  void SaveStates(std::vector<CControlState> &states) override;

  bool IsGroup() const override { return true; };

#ifdef _DEBUG
  void DumpTextureUse() override;
#endif
protected:
  // sub controls
  std::vector<CGUIControl *> m_children;

  typedef std::vector<CGUIControl *>::iterator iControls;
  typedef std::vector<CGUIControl *>::const_iterator ciControls;
  typedef std::vector<CGUIControl *>::reverse_iterator rControls;
  typedef std::vector<CGUIControl *>::const_reverse_iterator crControls;

  int  m_defaultControl;
  bool m_defaultAlways;
  int m_focusedControl;
  bool m_renderFocusedLast;
private:
  typedef std::vector< std::vector<CGUIControl *> * > COLLECTORTYPE;

  struct IDCollectorList
  {
    ~IDCollectorList() { for (auto item : m_items) delete item; };

    std::vector<CGUIControl *> *Get() {
      if (++m_stackDepth > m_items.size())
        m_items.push_back(new std::vector<CGUIControl *>());
      return m_items[m_stackDepth - 1];
    }

    void Release() { --m_stackDepth; };

    COLLECTORTYPE m_items;
    size_t m_stackDepth = 0;
  }m_idCollector;

  struct IDCollector
  {
    IDCollector(IDCollectorList &list)
      : m_list(list)
      , m_collector(list.Get()) {};

    ~IDCollector() { m_list.Release(); };

    IDCollectorList &m_list;
    std::vector<CGUIControl *> *m_collector;
  };
};

