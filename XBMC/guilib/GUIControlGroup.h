/*!
\file GUIControlGroup.h
\brief 
*/

#pragma once

#include "GUIControl.h"

/*!
 \ingroup controls
 \brief group of controls, useful for remembering last control + animating/hiding together
 */
class CGUIControlGroup : public CGUIControl
{
public:
  CGUIControlGroup();
  CGUIControlGroup(DWORD dwParentID, DWORD dwControlId, float posX, float posY, float width, float height);
  virtual ~CGUIControlGroup(void);
  virtual void Render();
  virtual bool OnAction(const CAction &action);
  virtual bool OnMessage(CGUIMessage& message);
  virtual bool HasFocus() const;
  virtual void PreAllocResources();
  virtual void AllocResources();
  virtual void FreeResources();
  virtual void DynamicResourceAlloc(bool bOnOff);
  virtual bool CanFocus() const;

  virtual bool HitTest(const CPoint &point) const;
  virtual bool CanFocusFromPoint(const CPoint &point, CGUIControl **control, CPoint &controlPoint) const;
  virtual void UnfocusFromPoint(const CPoint &point);

  virtual void SetInitialVisibility();

  virtual void DoRender(DWORD currentTime);
  virtual bool IsAnimating(ANIMATION_TYPE anim);
  virtual void QueueAnimation(ANIMATION_TYPE anim);
  virtual void ResetAnimation(ANIMATION_TYPE anim);

  virtual bool HasID(DWORD dwID) const;
  virtual bool HasVisibleID(DWORD dwID) const;

  int GetFocusedControlID() const;
  CGUIControl *GetFocusedControl() const;
  const CGUIControl *GetControl(int id) const;
  CGUIControl *GetFirstFocusableControl(int id);
  void GetContainers(vector<CGUIControl *> &containers) const;

  virtual void AddControl(CGUIControl *control);
  virtual bool RemoveControl(int id);
  virtual void ClearAll();
  void SetDefaultControl(DWORD id) { m_defaultControl = id; };

  virtual void SaveStates(vector<CControlState> &states);

  virtual bool IsGroup() const { return true; };

#ifdef _DEBUG
  virtual void DumpTextureUse();
#endif
protected:
  // sub controls
  vector<CGUIControl *> m_children;
  typedef vector<CGUIControl *>::iterator iControls;
  typedef vector<CGUIControl *>::const_iterator ciControls;
  typedef vector<CGUIControl *>::const_reverse_iterator crControls;

  int m_defaultControl;
  int m_focusedControl;

  // render time
  DWORD m_renderTime;
};

