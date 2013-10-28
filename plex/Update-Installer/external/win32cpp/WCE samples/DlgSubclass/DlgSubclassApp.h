///////////////////////////////////////
// DlgSubclassApp.h

#ifndef DLGSUBCLASSAPP_H
#define DLGSUBCLASSAPP_H

#include "MyDialog.h"


// Declaration of the CDialogApp class
class CDlgSubclassApp : public CWinApp
{
public:
	CDlgSubclassApp();
	virtual ~CDlgSubclassApp();
	virtual BOOL InitInstance();
	CMyDialog& GetDialog() {return m_MyDialog;}

private:
	CMyDialog m_MyDialog;
};


// returns a reference to the CDlgSubclassApp object
inline CDlgSubclassApp& GetSubApp() { return *((CDlgSubclassApp*)GetApp()); }


#endif // define DLGSUBCLASSAPP_H

