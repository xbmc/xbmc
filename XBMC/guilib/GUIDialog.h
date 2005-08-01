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

  void DoModal(DWORD dwParentId); // modal
  void Show(DWORD dwParentId); // modeless

  virtual void Close(bool forceClose = false);
  virtual bool Load(const CStdString& strFileName, bool bContainsPath = false);
  virtual bool IsRunning() const { return m_bRunning; }
  virtual bool IsDialog() { return true;};

protected:
  DWORD m_dwParentWindowID;
  CGUIWindow* m_pParentWindow;
  bool m_bRunning;
  bool m_bModal;
  unsigned int m_dialogClosing;
};
