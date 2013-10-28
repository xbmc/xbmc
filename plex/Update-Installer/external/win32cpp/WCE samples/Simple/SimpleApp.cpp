#include "SimpleApp.h"

CSimpleApp::CSimpleApp()
{
}

BOOL CSimpleApp::InitInstance()
{
	//Create the Window
    m_View.Create();

	return TRUE;
}