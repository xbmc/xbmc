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
class CGUIListContainer : public CGUIBaseContainer
{
public:
  CGUIListContainer(DWORD dwParentID, DWORD dwControlId, float posX, float posY, float width, float height, ORIENTATION orientation, int scrollTime);
//#ifdef PRE_SKIN_VERSION_2_1_COMPATIBILITY
  CGUIListContainer(DWORD dwParentID, DWORD dwControlId, float posX, float posY, float width, float height,
                         const CLabelInfo& labelInfo, const CLabelInfo& labelInfo2,
                         const CImage& textureButton, const CImage& textureButtonFocus,
                         float textureHeight, float itemWidth, float itemHeight, float spaceBetweenItems, CGUIControl *pSpin);
  CGUIControl *m_spinControl;
//#endif
  virtual ~CGUIListContainer(void);

  virtual void Render();
  virtual bool OnAction(const CAction &action);
  virtual bool OnMessage(CGUIMessage& message);
  
  virtual bool HasNextPage() const;
  virtual bool HasPreviousPage() const;
  
protected:
  virtual void Scroll(int amount);
  void SetCursor(int cursor);
  virtual bool MoveDown(DWORD nextControl);
  virtual bool MoveUp(DWORD nextControl);
  virtual void ValidateOffset();
  virtual void SelectItem(int item);
};

