//////////////////////////////////////////////////
// SubclassApp.h
//  Declaration of the CSubApp class

#ifndef SUBCLASSAPP_H
#define SUBCLASSAPP_H

#include "MainWin.h"


class CSubclassApp : public CWinApp
{
public:
	CSubclassApp();
	virtual ~CSubclassApp();
	virtual BOOL InitInstance();
	CMainWin& GetWin() { return m_Win; }

private:
	CMainWin m_Win;
};


// returns a reference to the CSubclassApp object
inline CSubclassApp& GetSubApp() { return *((CSubclassApp*)GetApp()); }


#endif // define SUBCLASSAPP_H

