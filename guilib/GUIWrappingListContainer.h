/*!
\file GUIListContainer.h
\brief 
*/

#pragma once

#include "GUIBaseContainer.h"
/*!
 \ingroup controls
 \brief 
 */
class CGUIWrappingListContainer : public CGUIBaseContainer
{
public:
  CGUIWrappingListContainer(DWORD dwParentID, DWORD dwControlId, float posX, float posY, float width, float height, ORIENTATION orientation, int scrollTime, int fixedPosition);
  virtual ~CGUIWrappingListContainer(void);

  virtual void Render();
  virtual bool OnAction(const CAction &action);
  virtual bool OnMessage(CGUIMessage& message);
  virtual int GetSelectedItem() const;

protected:
  virtual void Scroll(int amount);
  virtual bool MoveDown(bool wrapAround);
  virtual bool MoveUp(bool wrapAround);
  virtual void ValidateOffset();
  virtual int  CorrectOffset(int offset, int cursor) const;
  virtual bool SelectItemFromPoint(const CPoint &point);
  virtual void SelectItem(int item);
  virtual void Reset();

  void ResetExtraItems();
  unsigned int m_extraItems;
};

