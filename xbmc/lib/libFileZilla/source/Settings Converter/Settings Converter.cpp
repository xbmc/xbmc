// ZipDLL.cpp : Definiert den Einsprungpunkt für die DLL-Anwendung.
//

#pragma warning (disable : 4786)

#include <windows.h>

#include "misc\stdstring.h"

#include <map>
#include <list>
#include <vector>



#include "exdll.h"
#include <commctrl.h>
#include "options.h"

BOOL APIENTRY DllMain( HANDLE hModule, 
                       DWORD  ul_reason_for_call, 
                       LPVOID lpReserved
					 )
{
    return TRUE;
}

extern "C" void __declspec(dllexport) convert(HWND hwndParent, int string_size, 
                                      char *variables, stack_t **stacktop)
{
	EXDLL_INIT();
	
	char *buffer = new char[string_size + 1];
	popstring(buffer);

	COptions options(hwndParent);
	options.Convert(TRUE, buffer);
	options.Convert(FALSE, buffer);

	delete [] buffer;
}
