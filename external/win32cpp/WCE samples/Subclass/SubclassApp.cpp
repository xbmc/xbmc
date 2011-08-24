///////////////////////////////////////////////
//SubclassApp.cpp -  Definitions for the CSubclassApp class


#include "SubclassApp.h"


CSubclassApp::CSubclassApp()
{
}

BOOL CSubclassApp::InitInstance()
{
	TRACE(TEXT("Move the mouse over the button to see messages"));

	//Create the Outer Window
	m_Win.Create();

	return TRUE;
}

CSubclassApp::~CSubclassApp()
{
}
