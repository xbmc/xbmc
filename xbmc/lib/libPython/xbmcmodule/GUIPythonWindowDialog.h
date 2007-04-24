#pragma once
#include "GUIPythonWindow.h"

class CGUIPythonWindowDialog : public CGUIPythonWindow
{
public:
  CGUIPythonWindowDialog(DWORD dwId);
  virtual ~CGUIPythonWindowDialog(void);
  virtual bool    OnMessage(CGUIMessage& message);
  void             Activate(DWORD dwParentId);
  virtual void    Close();
  virtual bool    IsDialogRunning() const { return m_bRunning; }
  virtual bool    IsDialog() const { return true;};
  virtual bool    IsModalDialog() const { return true; };

protected:
  bool             m_bRunning;
};
