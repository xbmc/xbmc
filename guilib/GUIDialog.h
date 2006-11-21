/*!
\file GUIDialog.h
\brief 
*/

#pragma once
#include "GUIWindow.h"

/*!
 \ingroup winmsg
 \brief 
 */
class CGUIDialog :
      public CGUIWindow
{
public:
  CGUIDialog(DWORD dwID, const CStdString &xmlFile);
  virtual ~CGUIDialog(void);

  virtual bool OnAction(const CAction &action);
  virtual bool OnMessage(CGUIMessage& message);
  virtual void Render();

  void DoModal(int iWindowID = WINDOW_INVALID); // modal
  void Show(); // modeless

  virtual void Close(bool forceClose = false);
  virtual bool Load(const CStdString& strFileName, bool bContainsPath = false);
  virtual bool IsRunning() const { return m_bRunning; }
  virtual bool IsDialog() const { return true;};
  virtual bool IsAnimating(ANIMATION_TYPE animType);

protected:
  virtual bool RenderAnimation(DWORD time);
  virtual void Reset();

  bool m_bRunning;
  bool m_bModal;
  bool m_dialogClosing;
};
