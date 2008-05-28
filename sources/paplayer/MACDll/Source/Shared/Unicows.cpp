#include <windows.h>

HMODULE LoadUnicowsProc(void)
{
	HMODULE hModule = LoadLibraryA("unicows.dll");
	if (hModule == NULL)
	{
		MessageBoxA(NULL, "The Microsoft Layer for Unicode failed to initialize.  Please re-install.", "Unicode Failure", MB_OK | MB_ICONERROR);
		_exit(-1);
	}

	return hModule;
}

extern "C" 
{
	extern FARPROC _PfnLoadUnicows = (FARPROC) &LoadUnicowsProc;
}
