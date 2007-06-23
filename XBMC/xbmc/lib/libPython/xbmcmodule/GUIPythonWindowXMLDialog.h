#pragma once
#include "GUIPythonWindowXML.h"

class CGUIPythonWindowXMLDialog : public CGUIPythonWindowXML
{
  public:
    CGUIPythonWindowXMLDialog(DWORD dwId, CStdString strXML, CStdString strFallBackPath);
    virtual ~CGUIPythonWindowXMLDialog(void);
    void            Activate(DWORD dwParentId);
    virtual bool    OnMessage(CGUIMessage &message);
    virtual void    Close();
    virtual bool    IsDialogRunning() const { return m_bRunning; }
    virtual bool    IsDialog() const { return true;};
    virtual bool    IsModalDialog() const { return true; };
  protected:
    bool             m_bRunning;
};

