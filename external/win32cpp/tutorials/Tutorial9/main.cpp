////////////////////////////////
// Main.cpp

// Add the Win32++\include  directory to project's additional include directories
// Add the Win32++\include  directory to project's additional resource directories
#include "ScribbleApp.h"


int APIENTRY WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
    // Start Win32++
    CScribbleApp MyApp;

	// Run the application
    return MyApp.Run();
}
