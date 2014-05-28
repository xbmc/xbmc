
//
// dllmain.cpp
//
// This is the entry point for the windows dll
//

#include <windows.h>
#ifndef CLIB_ANGELSCRIPT_C_H_INCLUDED
#define CLIB_ANGELSCRIPT_C_H_INCLUDED
#include "angelscript_c.h"
#endif


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
