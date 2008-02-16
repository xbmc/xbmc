#pragma once

#include "guiImage.h"

class CButton
{
public:
  CButton()
  {
    id = 0;
    info = 0;
    imageFocus = imageNoFocus = NULL;
  };
  ~CButton()
  {
    if (imageFocus) delete imageFocus;
    if (imageNoFocus) delete imageNoFocus;
  }
  int id;
  int info;
  std::string strLabel;
  std::vector<CStdString> clickActions;
  CGUIImage *imageFocus;
  CGUIImage *imageNoFocus;
};

class CGUIButtonScroller :
      public CGUIControl
{
public:
  CGUIButtonScroller(DWORD dwParentID, DWORD dwControlId, float posX, float posY, float width, float height, float gap, int iSlots, int iDefaultSlot, int iMovementRange, bool bHorizontal, int iAlpha, bool bWrapAround, bool bSmoothScrolling, const CImage& textureFocus, const CImage& textureNoFocus, const CLabelInfo& labelInfo);
  virtual ~CGUIButtonScroller(void);
  virtual bool OnAction(const CAction &action);
  virtual bool OnMessage(CGUIMessage &message);
  virtual void OnUp();
  virtual void OnLeft();
  virtual void OnRight();
  virtual void OnDown();
  virtual bool OnMouseOver(const CPoint &point);
  virtual bool OnMouseClick(DWORD dwButton, const CPoint &point);
  virtual bool OnMouseWheel(char wheel, const CPoint &point);
  virtual void Render();
  virtual void PreAllocResources();
  virtual void AllocResources();
  virtual void FreeResources();
  virtual void DynamicResourceAlloc(bool bOnOff);
  void ClearButtons();
  void AddButton(const std::string &strLabel, const CStdString &strExecute, const int iIcon);
  void SetActiveButton(int iButton);
  int GetActiveButton() const;
  int GetActiveButtonID() const;
  virtual CStdString GetDescription() const;
  virtual void SaveStates(std::vector<CControlState> &states);
  void LoadButtons(TiXmlNode *node);

protected:
  int GetNext(int iCurrent) const;
  int GetPrevious(int iCurrent);
  int GetButton(int iOffset);
  void DoUp();
  void DoDown();
  void RenderItem(float &posX, float &posY, int &iOffset, bool bText);
  void Update();
  void GetScrollZone(float &fStartAlpha, float &fEndAlpha);
private:
  // saved variables from the xml (as this control is user editable...)
  int m_iXMLNumSlots;
  int m_iXMLDefaultSlot;
  float m_xmlPosX;
  float m_xmlPosY;
  float m_xmlWidth;
  float m_xmlHeight;

  float m_buttonGap;     // gap between buttons
  int m_iNumSlots;     // number of button slots available
  int m_iDefaultSlot;    // default highlight position
  int m_iMovementRange;   // amoung that we can move the highlight
  bool m_bHorizontal;    // true if it's a horizontal button bar
  int m_iAlpha;       // amount of alpha (0..100)
  bool m_bWrapAround;    // whether the buttons wrap around or not
  bool m_bSmoothScrolling; // whether there is smooth scrolling or not

  int m_iCurrentSlot;    // currently highlighted slot

  int m_iOffset;      // first item in the list
  float m_scrollOffset;   // offset when scrolling
  bool m_bScrollUp;     // true if we're scrolling up (or left)
  bool m_bScrollDown;    // true if scrolling down (or right)
  bool m_bMoveUp;      // true if we're scrolling up (or left)
  bool m_bMoveDown;     // true if scrolling down (or right)
  float m_fScrollSpeed;   // speed of scrolling
  float m_fAnalogScrollSpeed;  // speed of analog scroll (triggers)
  // stuff we need for the buttons...
  std::vector<CButton*> m_vecButtons;
  typedef std::vector<CButton*>::iterator ivecButtons;
  CGUIImage m_imgFocus;
  CGUIImage m_imgNoFocus;

  CLabelInfo m_label;
  int m_iSlowScrollCount;
};
