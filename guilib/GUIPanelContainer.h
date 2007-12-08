/*!
\file GUIPanelContainer.h
\brief 
*/

#pragma once

#include "GUIBaseContainer.h"

/*!
 \ingroup controls
 \brief 
 */
class CGUIPanelContainer : public CGUIBaseContainer
{
public:
  CGUIPanelContainer(DWORD dwParentID, DWORD dwControlId, float posX, float posY, float width, float height, ORIENTATION orientation, int scrollTime);
//#ifdef PRE_SKIN_VERSION_2_1_COMPATIBILITY
  CGUIPanelContainer(DWORD dwParentID, DWORD dwControlId, float posX, float posY, float width, float height,
                         const CImage& imageNoFocus, const CImage& imageFocus,
                         float itemWidth, float itemHeight,
                         float textureWidth, float textureHeight,
                         float thumbPosX, float thumbPosY, float thumbWidth, float thumbHeight, DWORD thumbAlign, CGUIImage::GUIIMAGE_ASPECT_RATIO thumbAspect,
                         const CLabelInfo& labelInfo, bool hideLabels,
                         CGUIControl *pSpin, CGUIControl *pPanel);
  CGUIControl *m_spinControl;
  CGUIControl *m_largePanel;
//#endif
  virtual ~CGUIPanelContainer(void);

  virtual void Render();
  virtual bool OnAction(const CAction &action);
  virtual bool OnMessage(CGUIMessage& message);
  virtual void OnLeft();
  virtual void OnRight();
  virtual void OnUp();
  virtual void OnDown();
  virtual bool GetCondition(int condition, int data) const;
protected:
  virtual bool MoveUp(DWORD nextControl);
  virtual bool MoveDown(DWORD nextControl);
  virtual bool MoveLeft(DWORD nextControl);
  virtual bool MoveRight(DWORD nextControl);
  virtual void Scroll(int amount);
  float AnalogScrollSpeed() const;
  virtual void ValidateOffset();
  virtual void CalculateLayout();
  unsigned int GetRows() const;
  virtual int  CorrectOffset(int offset, int cursor) const;
  virtual bool SelectItemFromPoint(const CPoint &point);
  void SetCursor(int cursor);
  virtual void SelectItem(int item);

  int m_itemsPerRow;
};

