#pragma once

//#include "guiImage.h"
#include "GUILabelControl.h"  // for CInfoPortion
#include "GUIButtonControl.h"

/*!
 \ingroup controls
 \brief 
 */
class CGUIMultiSelectTextControl : public CGUIControl
{
public:
  CGUIMultiSelectTextControl(DWORD dwParentID, DWORD dwControlId,
                    float posX, float posY, float width, float height,
                    const CImage& textureFocus, const CImage& textureNoFocus, const CLabelInfo &label, const CStdString &labelText);

  virtual ~CGUIMultiSelectTextControl(void);

  virtual void DoRender(DWORD currentTime);
  virtual void Render();

  virtual bool OnAction(const CAction &action);
  virtual void OnLeft();
  virtual void OnRight();
  virtual bool HitTest(const CPoint &point) const;
  virtual bool OnMouseOver(const CPoint &point);
  virtual bool OnMouseClick(DWORD dwButton, const CPoint &point);

  virtual CStdString GetDescription() const;
  virtual bool CanFocus() const;

  void UpdateText(const CStdString &text);
  bool MoveLeft();
  bool MoveRight();
  void SelectItemFromPoint(const CPoint &point);
  unsigned int GetFocusedItem() const;
  void SetFocusedItem(unsigned int item);

  // overrides to allow all focus anims to translate down to the focus image
  virtual void SetAnimations(const vector<CAnimation> &animations);
  virtual void SetFocus(bool focus);
protected:
  void AddString(const CStdString &text, bool selectable, const CStdString &clickAction = "");
  void PositionButtons();
  unsigned int GetNumSelectable() const;
  unsigned int GetItemFromPoint(const CPoint &point) const;
  void ScrollToItem(unsigned int item);

  // the static strings and buttons strings
  class CSelectableString
  {
  public:
    CSelectableString(CGUIFont *font, const CStdString &text, bool selectable, const CStdString &clickAction);
    CGUITextLayout m_text;
    float m_length;
    bool m_selectable;
    CStdString m_clickAction;
  };
  vector<CSelectableString> m_items;

  CLabelInfo m_label;
  vector<CInfoPortion>  m_multiInfo;
  CStdString m_oldText;
  DWORD      m_renderTime;

  // scrolling
  float      m_totalWidth;
  float      m_offset;
  float      m_scrollOffset;
  float      m_scrollSpeed;
  DWORD      m_scrollLastTime;

  // buttons
  CGUIButtonControl m_button;
  unsigned int m_selectedItem;
  vector<CGUIButtonControl> m_buttons;
};

