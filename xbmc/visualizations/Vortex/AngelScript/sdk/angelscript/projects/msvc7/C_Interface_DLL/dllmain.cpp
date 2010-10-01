
//
// dllmain.cpp
//
// This is the entry point for the windows dll
//

#include <windows.h>
#include "../../../include\angelscript.h"

// Don't forget to force inclusion of 
// the exported library functions

BOOL APIENTRY DllMain( HANDLE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
					 )
{
    switch (ul_reason_for_call)
	{
		case DLL_PROCESS_ATTACH:
		case DLL_THREAD_ATTACH:
		case DLL_THREAD_DETACH:
		case DLL_PROCESS_DETACH:
			break;
    }
    return TRUE;
}
