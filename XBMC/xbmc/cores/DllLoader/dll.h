
#ifndef __DLL_H_
#define __DLL_H_ 

extern "C" HMODULE __stdcall dllLoadLibraryExtended(LPCSTR file, LPCSTR sourcedll);
extern "C" HMODULE __stdcall dllLoadLibraryA(LPCSTR file);
extern "C" HMODULE __stdcall dllLoadLibraryExExtended(LPCSTR lpLibFileName, HANDLE hFile, DWORD dwFlags, LPCSTR sourcedll);
extern "C" HMODULE __stdcall dllLoadLibraryExA(LPCSTR lpLibFileName, HANDLE hFile, DWORD dwFlags);
extern "C" BOOL __stdcall dllFreeLibrary(HINSTANCE hLibModule);
extern "C" FARPROC __stdcall dllGetProcAddress(HMODULE hModule, LPCSTR function);
extern "C" HMODULE WINAPI dllGetModuleHandleA(LPCSTR lpModuleName);
extern "C" DWORD WINAPI dllGetModuleFileNameA(HMODULE hModule, LPSTR lpFilename, DWORD nSize);

#endif
