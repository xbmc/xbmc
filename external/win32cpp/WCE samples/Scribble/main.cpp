///////////////////////////////////////
// main.cpp

#include "wincore.h"
#include "resource.h"
#include "FrameApp.h"


int WINAPI WinMain(	HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow)
{
	TCHAR szBuf[100];
	::LoadString(hInstance, IDW_MAIN, szBuf, 100);
	HWND hWnd = FindWindow(szBuf, NULL);

	// If application is already running, restore it and quit
	if (hWnd)
	{
		SetForegroundWindow ((HWND)(((DWORD)hWnd) | 0x01));
		TRACE(_T("Application Already running"));
		return -1;
	}

	//Start the framework
    CWceFrameApp MyApp;

    //Process the window messages
    return MyApp.Run();
}

