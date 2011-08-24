#ifndef SIMPLEAPP_H
#define SIMPLEAPP_H

#include "wincore.h"
#include "MainFrm.h"


class CWceFrameApp : public CWinApp
{
public:
    CWceFrameApp();
    virtual ~CWceFrameApp() {}
	virtual BOOL InitInstance();
	CMainFrame& GetMainFrame() { return m_Frame; }

private:
    CMainFrame m_Frame;
};


// returns a reference to the CWceFrameApp object
inline CWceFrameApp& GetFrameApp() { return *((CWceFrameApp*)GetApp()); }


#endif //SIMPLEAPP_H

