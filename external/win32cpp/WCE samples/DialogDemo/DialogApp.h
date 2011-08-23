//////////////////////////////////////////////////
// DialogApp.h
//  Declaration of the CDialogApp class

#ifndef DIALOGAPP_H
#define DIALOGAPP_H


#include "dialog.h"
#include "MyDialog.h"


class CDialogApp : public CWinApp
{
public:
	CDialogApp();
	virtual ~CDialogApp();
	virtual BOOL InitInstance();
	CMyDialog& GetDlg() { return MyDialog; }

private:
	CMyDialog MyDialog;
};


// returns a reference to the CDialogApp object
inline CDialogApp& GetDlgApp() { return *((CDialogApp*)GetApp()); }


#endif // define DIALOGAPP_H

