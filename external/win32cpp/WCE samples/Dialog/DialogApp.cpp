/////////////////////////////////////////////////
//App.cpp -  Definitions for the CDialogApp class

#include "dialog.h"
#include "resource.h"
#include "DialogApp.h"


CDialogApp::CDialogApp() : MyDialog(IDD_DIALOG)
{
}

BOOL CDialogApp::InitInstance()
{
	//Display the Modal Dialog
	MyDialog.DoModal();

	//End the program
	::PostQuitMessage(0);

	return TRUE;
}

CDialogApp::~CDialogApp()
{
}

