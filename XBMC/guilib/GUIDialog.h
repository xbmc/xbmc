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
  virtual bool IsDialogRunning() const { return m_bRunning; };
  virtual bool IsDialog() const { return true;};
  virtual bool IsModalDialog() const { return m_bModal; };

  virtual bool IsAnimating(ANIMATION_TYPE animType);

  void SetAutoClose(unsigned int timeoutMs);
protected:
  virtual bool RenderAnimation(DWORD time);
  virtual void SetDefaults();

  friend class CApplicationMessenger;
  void DoModal_Internal(int iWindowID = WINDOW_INVALID); // modal
  void Show_Internal(); // modeless

  bool m_bRunning;
  bool m_bModal;
  bool m_dialogClosing;
  bool m_autoClosing;
  DWORD m_showStartTime;
  DWORD m_showDuration;
};
