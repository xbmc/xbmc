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

protected:
  virtual void Scroll(int amount);
  virtual bool MoveDown(DWORD nextControl);
  virtual bool MoveUp(DWORD nextControl);
  virtual void ValidateOffset();
  virtual int  CorrectOffset(int offset, int cursor) const;
  virtual bool SelectItemFromPoint(const CPoint &point);
  virtual void SelectItem(int item);
};

