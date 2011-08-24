///////////////////////////////////////
// main.cpp

#include "Wincore.h"

// Notes:  
//  1) This application doesn't end when the window is closed
//  2) Add the Win32++\include  directory to project's additional include directories



int APIENTRY WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
	// Start Win32++
	CWinApp MyApp;

	// Create a CWnd object
	CWnd MyWindow;

	// Create (and display) the window
	MyWindow.Create();

	// Run the application
	return MyApp.Run();
}


