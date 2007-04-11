#pragma once
#include "GUIPythonWindowXML.h"

int Py_XBMC_Event_OnClick(void* arg);
int Py_XBMC_Event_OnFocus(void* arg);
int Py_XBMC_Event_OnInit(void* arg);

class CGUIPythonWindowXMLDialog : public CGUIPythonWindowXML
{
  public:
    CGUIPythonWindowXMLDialog(DWORD dwId, CStdString strXML, CStdString strFallBackPath);
    virtual ~CGUIPythonWindowXMLDialog(void);
    virtual bool    OnMessage(CGUIMessage& message);
    void             Activate(DWORD dwParentId);
    virtual void    Close();
    virtual bool    IsDialogRunning() const { return m_bRunning; }
    virtual bool    IsDialog() const { return true;};
    virtual bool    IsModalDialog() const { return true; };
  protected:
    virtual void    UpdateButtons() {};
    virtual void    OnWindowLoaded() {};
    bool             m_bRunning;
};

