///////////////////////////////////////
// DlgSubclassApp.cpp

#include "resource.h"
#include "DlgSubclassApp.h"


// Definitions for the CDialogApp class
CDlgSubclassApp::CDlgSubclassApp() : m_MyDialog(IDD_DIALOG1)
{
}

BOOL CDlgSubclassApp::InitInstance()
{
	//Display the Modal Dialog
	m_MyDialog.DoModal();

	//End the program
	::PostQuitMessage(0);

	return TRUE;
}

CDlgSubclassApp::~CDlgSubclassApp()
{
}

