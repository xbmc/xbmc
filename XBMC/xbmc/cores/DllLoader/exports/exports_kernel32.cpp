
#include "..\..\..\stdafx.h"
#include "..\DllLoaderContainer.h"

#include "emu_kernel32.h"
#include "../dll_tracker_library.h"
#include "../dll_tracker_memory.h"
#include "../dll_tracker_critical_section.h"

void export_kernel32()
{
  g_dlls.kernel32.AddExport("AddAtomA", (unsigned long)dllAddAtomA);
  g_dlls.kernel32.AddExport("FindAtomA", (unsigned long)dllFindAtomA);
  g_dlls.kernel32.AddExport("GetAtomNameA", (unsigned long)dllGetAtomNameA);
  g_dlls.kernel32.AddExport("CreateThread", (unsigned long)dllCreateThread);
  g_dlls.kernel32.AddExport("FindClose", (unsigned long)dllFindClose);
  g_dlls.kernel32.AddExport("FindFirstFileA", (unsigned long)FindFirstFileA);
  g_dlls.kernel32.AddExport("FindNextFileA", (unsigned long)FindNextFileA);
  g_dlls.kernel32.AddExport("GetFileAttributesA", (unsigned long)dllGetFileAttributesA);
  g_dlls.kernel32.AddExport("GetLastError", (unsigned long)GetLastError);
  g_dlls.kernel32.AddExport("SetUnhandledExceptionFilter", (unsigned long)dllSetUnhandledExceptionFilter);
  g_dlls.kernel32.AddExport("Sleep", (unsigned long)dllSleep);
  g_dlls.kernel32.AddExport("SleepEx", (unsigned long)SleepEx);
  g_dlls.kernel32.AddExport("TerminateThread", (unsigned long)dllTerminateThread);
  g_dlls.kernel32.AddExport("GetCurrentThread", (unsigned long)dllGetCurrentThread);
  g_dlls.kernel32.AddExport("QueryPerformanceCounter", (unsigned long)QueryPerformanceCounter);
#ifdef _XBOX
  g_dlls.kernel32.AddExport("QueryPerformanceFrequency", (unsigned long)QueryPerformanceFrequencyXbox);
#else
  g_dlls.kernel32.AddExport("QueryPerformanceFrequency", (unsigned long)QueryPerformanceFrequency);
#endif
  g_dlls.kernel32.AddExport("SetThreadPriority", (unsigned long)SetThreadPriority);
  g_dlls.kernel32.AddExport("GetTickCount", (unsigned long)GetTickCount);
  g_dlls.kernel32.AddExport("GetCurrentThreadId", (unsigned long)GetCurrentThreadId); //test
  g_dlls.kernel32.AddExport("GetCurrentProcessId", (unsigned long)dllGetCurrentProcessId);
  g_dlls.kernel32.AddExport("GetSystemTimeAsFileTime", (unsigned long)GetSystemTimeAsFileTime);
  g_dlls.kernel32.AddExport("OutputDebugStringA", (unsigned long)OutputDebugString);
  g_dlls.kernel32.AddExport("DisableThreadLibraryCalls", (unsigned long)dllDisableThreadLibraryCalls);
  g_dlls.kernel32.AddExport("GlobalMemoryStatus", (unsigned long)GlobalMemoryStatus);
  g_dlls.kernel32.AddExport("CreateEventA", (unsigned long)CreateEventA); //test
  g_dlls.kernel32.AddExport("ResetEvent", (unsigned long)ResetEvent);
  g_dlls.kernel32.AddExport("WaitForSingleObject", (unsigned long)dllWaitForSingleObject);
  g_dlls.kernel32.AddExport("LoadLibraryA", (unsigned long)dllLoadLibraryA, (void*)track_LoadLibraryA);
  g_dlls.kernel32.AddExport("FreeLibrary", (unsigned long)dllFreeLibrary, (void*)track_FreeLibrary);
  g_dlls.kernel32.AddExport("GetProcAddress", (unsigned long)dllGetProcAddress);
  g_dlls.kernel32.AddExport("LeaveCriticalSection", (unsigned long)dllLeaveCriticalSection);
  g_dlls.kernel32.AddExport("EnterCriticalSection", (unsigned long)dllEnterCriticalSection);
  g_dlls.kernel32.AddExport("DeleteCriticalSection", (unsigned long)dllDeleteCriticalSection, (void*)track_DeleteCriticalSection);
  g_dlls.kernel32.AddExport("InitializeCriticalSection", (unsigned long)dllInitializeCriticalSection, (void*)track_InitializeCriticalSection);
  g_dlls.kernel32.AddExport("GetSystemInfo", (unsigned long) dllGetSystemInfo);
  g_dlls.kernel32.AddExport("CloseHandle", (unsigned long) CloseHandle);
  g_dlls.kernel32.AddExport("GetPrivateProfileIntA", (unsigned long) dllGetPrivateProfileIntA);
  g_dlls.kernel32.AddExport("WaitForMultipleObjects", (unsigned long) dllWaitForMultipleObjects);
  g_dlls.kernel32.AddExport("SetEvent", (unsigned long) SetEvent);
  g_dlls.kernel32.AddExport("TlsAlloc", (unsigned long) dllTlsAlloc);
  g_dlls.kernel32.AddExport("TlsFree", (unsigned long) dllTlsFree);
  g_dlls.kernel32.AddExport("TlsGetValue", (unsigned long) dllTlsGetValue);
  g_dlls.kernel32.AddExport("TlsSetValue", (unsigned long) dllTlsSetValue);
  g_dlls.kernel32.AddExport("HeapFree", (unsigned long) HeapFree); //test
  g_dlls.kernel32.AddExport("HeapAlloc", (unsigned long) HeapAlloc); //test
  g_dlls.kernel32.AddExport("LocalFree", (unsigned long) LocalFree); //test
  g_dlls.kernel32.AddExport("LocalAlloc", (unsigned long) LocalAlloc); //test
  g_dlls.kernel32.AddExport("InterlockedIncrement", (unsigned long) InterlockedIncrement);
  g_dlls.kernel32.AddExport("InterlockedDecrement", (unsigned long) InterlockedDecrement);
  g_dlls.kernel32.AddExport("InterlockedExchange", (unsigned long) InterlockedExchange);
  g_dlls.kernel32.AddExport("GetProcessHeap", (unsigned long) GetProcessHeap); //test
  g_dlls.kernel32.AddExport("GetModuleHandleA", (unsigned long) dllGetModuleHandleA);
  g_dlls.kernel32.AddExport("InterlockedCompareExchange", (unsigned long) InterlockedCompareExchange);
  g_dlls.kernel32.AddExport("GetVersionExA", (unsigned long) dllGetVersionExA);
  g_dlls.kernel32.AddExport("GetVersionExW", (unsigned long) dllGetVersionExW);
  g_dlls.kernel32.AddExport("GetProfileIntA", (unsigned long) dllGetProfileIntA);
  g_dlls.kernel32.AddExport("CreateFileA", (unsigned long) dllCreateFileA);
  g_dlls.kernel32.AddExport("DeviceIoControl", (unsigned long) DeviceIoControl);
  g_dlls.kernel32.AddExport("ReadFile", (unsigned long) ReadFile);
  g_dlls.kernel32.AddExport("dllDVDReadFile", (unsigned long) dllDVDReadFileLayerChangeHack);
  g_dlls.kernel32.AddExport("SetFilePointer", (unsigned long) SetFilePointer);
#ifdef _XBOX
  g_dlls.kernel32.AddExport("xboxopendvdrom", (unsigned long) xboxopendvdrom);
#endif
  g_dlls.kernel32.AddExport("GetVersion", (unsigned long) dllGetVersion);
  g_dlls.kernel32.AddExport("MulDiv", (unsigned long) MulDiv);
  g_dlls.kernel32.AddExport("lstrlenA", (unsigned long) lstrlenA);
  g_dlls.kernel32.AddExport("lstrlenW", (unsigned long) lstrlenW);
  g_dlls.kernel32.AddExport("LoadLibraryExA", (unsigned long)dllLoadLibraryExA, (void*)track_LoadLibraryExA);

  g_dlls.kernel32.AddExport("DeleteFileA", (unsigned long) DeleteFileA);
  g_dlls.kernel32.AddExport("GetModuleFileNameA", (unsigned long) dllGetModuleFileNameA);
  g_dlls.kernel32.AddExport("GlobalAlloc", (unsigned long) GlobalAlloc);
  g_dlls.kernel32.AddExport("GlobalLock", (unsigned long) GlobalLock);
  g_dlls.kernel32.AddExport("GlobalUnlock", (unsigned long) GlobalUnlock);
  g_dlls.kernel32.AddExport("FreeEnvironmentStringsW", (unsigned long) dllFreeEnvironmentStringsW);
  g_dlls.kernel32.AddExport("SetLastError", (unsigned long) SetLastError);
  g_dlls.kernel32.AddExport("RestoreLastError", (unsigned long) SetLastError);
  g_dlls.kernel32.AddExport("GetOEMCP", (unsigned long) dllGetOEMCP);
  g_dlls.kernel32.AddExport("SetEndOfFile", (unsigned long) SetEndOfFile);
  g_dlls.kernel32.AddExport("RtlUnwind", (unsigned long) dllRtlUnwind);
  g_dlls.kernel32.AddExport("GetCommandLineA", (unsigned long) dllGetCommandLineA);
  g_dlls.kernel32.AddExport("HeapReAlloc", (unsigned long) HeapReAlloc); //test
  g_dlls.kernel32.AddExport("ExitProcess", (unsigned long) dllExitProcess);
  g_dlls.kernel32.AddExport("TerminateProcess", (unsigned long) dllTerminateProcess);
  g_dlls.kernel32.AddExport("GetCurrentProcess", (unsigned long) dllGetCurrentProcess);
  g_dlls.kernel32.AddExport("HeapSize", (unsigned long) HeapSize);
  g_dlls.kernel32.AddExport("WriteFile", (unsigned long) WriteFile);
  g_dlls.kernel32.AddExport("GlobalFree", (unsigned long) GlobalFree);
  g_dlls.kernel32.AddExport("GetACP", (unsigned long) dllGetACP);
  g_dlls.kernel32.AddExport("SetHandleCount", (unsigned long) dllSetHandleCount);
  g_dlls.kernel32.AddExport("GetStdHandle", (unsigned long) dllGetStdHandle);
  g_dlls.kernel32.AddExport("GetFileType", (unsigned long) dllGetFileType);
  g_dlls.kernel32.AddExport("GetStartupInfoA", (unsigned long) dllGetStartupInfoA);
  g_dlls.kernel32.AddExport("FreeEnvironmentStringsA", (unsigned long) dllFreeEnvironmentStringsA);
  g_dlls.kernel32.AddExport("WideCharToMultiByte", (unsigned long) dllWideCharToMultiByte);
  g_dlls.kernel32.AddExport("GetEnvironmentStrings", (unsigned long) dllGetEnvironmentStrings);
  g_dlls.kernel32.AddExport("GetEnvironmentStringsW", (unsigned long) dllGetEnvironmentStringsW);
  g_dlls.kernel32.AddExport("GetEnvironmentVariableA", (unsigned long) dllGetEnvironmentVariableA);
  g_dlls.kernel32.AddExport("HeapDestroy", (unsigned long) HeapDestroy, (void*)track_HeapDestroy );
  g_dlls.kernel32.AddExport("HeapCreate", (unsigned long) HeapCreate, (void*)track_HeapCreate );
  g_dlls.kernel32.AddExport("VirtualFree", (unsigned long) VirtualFree, (unsigned long)track_VirtualFree);
  g_dlls.kernel32.AddExport("VirtualFreeEx", (unsigned long) VirtualFreeEx, (unsigned long)track_VirtualFreeEx);
  g_dlls.kernel32.AddExport("VirtualAlloc", (unsigned long) VirtualAlloc, (unsigned long)track_VirtualAlloc);
  g_dlls.kernel32.AddExport("VirtualAllocEx", (unsigned long) VirtualAllocEx, (unsigned long)track_VirtualAllocEx);
  g_dlls.kernel32.AddExport("MultiByteToWideChar", (unsigned long) dllMultiByteToWideChar);
  g_dlls.kernel32.AddExport("LCMapStringA", (unsigned long) dllLCMapStringA);
  g_dlls.kernel32.AddExport("LCMapStringW", (unsigned long) dllLCMapStringW);
  g_dlls.kernel32.AddExport("IsBadWritePtr", (unsigned long) IsBadWritePtr);
  g_dlls.kernel32.AddExport("SetStdHandle", (unsigned long) dllSetStdHandle);
  g_dlls.kernel32.AddExport("FlushFileBuffers", (unsigned long) FlushFileBuffers);
  g_dlls.kernel32.AddExport("GetStringTypeA", (unsigned long) dllGetStringTypeA);
  g_dlls.kernel32.AddExport("GetStringTypeW", (unsigned long) dllGetStringTypeW);
  g_dlls.kernel32.AddExport("IsBadReadPtr", (unsigned long) IsBadReadPtr);
  g_dlls.kernel32.AddExport("IsBadCodePtr", (unsigned long) IsBadCodePtr);
  g_dlls.kernel32.AddExport("GetCPInfo", (unsigned long) dllGetCPInfo);

  g_dlls.kernel32.AddExport("CreateMutexA", (unsigned long) CreateMutexA); //test
  g_dlls.kernel32.AddExport("CreateSemaphoreA", (unsigned long) CreateSemaphoreA);
  g_dlls.kernel32.AddExport("PulseEvent", (unsigned long) PulseEvent);
  g_dlls.kernel32.AddExport("ReleaseMutex", (unsigned long) ReleaseMutex);
  g_dlls.kernel32.AddExport("ReleaseSemaphore", (unsigned long) ReleaseSemaphore);

  g_dlls.kernel32.AddExport("GetThreadLocale", (unsigned long) dllGetThreadLocale);
  g_dlls.kernel32.AddExport("SetPriorityClass", (unsigned long) dllSetPriorityClass);
  g_dlls.kernel32.AddExport("FormatMessageA", (unsigned long) dllFormatMessageA);
  g_dlls.kernel32.AddExport("GetFullPathNameA", (unsigned long) dllGetFullPathNameA);
#ifdef _XBOX
  g_dlls.kernel32.AddExport("SignalObjectAndWait", (unsigned long) SignalObjectAndWait);
#endif
  g_dlls.kernel32.AddExport("ExpandEnvironmentStringsA", (unsigned long) dllExpandEnvironmentStringsA);
  g_dlls.kernel32.AddExport("GetVolumeInformationA", (unsigned long) GetVolumeInformationA);
  g_dlls.kernel32.AddExport("GetWindowsDirectoryA", (unsigned long) dllGetWindowsDirectoryA);
  g_dlls.kernel32.AddExport("GetSystemDirectoryA", (unsigned long) dllGetSystemDirectoryA);
  g_dlls.kernel32.AddExport("DuplicateHandle", (unsigned long) dllDuplicateHandle);
  g_dlls.kernel32.AddExport("GetShortPathNameA", (unsigned long) dllGetShortPathName);
  g_dlls.kernel32.AddExport("GetTempPathA", (unsigned long) dllGetTempPathA);
  g_dlls.kernel32.AddExport("SetErrorMode", (unsigned long) dllSetErrorMode);
  g_dlls.kernel32.AddExport("IsProcessorFeaturePresent", (unsigned long) dllIsProcessorFeaturePresent);
  g_dlls.kernel32.AddExport("FileTimeToLocalFileTime", (unsigned long) FileTimeToLocalFileTime);
  g_dlls.kernel32.AddExport("FileTimeToSystemTime", (unsigned long) FileTimeToSystemTime);
  g_dlls.kernel32.AddExport("GetTimeZoneInformation", (unsigned long) GetTimeZoneInformation);

  g_dlls.kernel32.AddExport("GetCurrentDirectoryA", (unsigned long) dllGetCurrentDirectoryA);
  g_dlls.kernel32.AddExport("SetCurrentDirectoryA", (unsigned long) dllSetCurrentDirectoryA);

  g_dlls.kernel32.AddExport("SetEnvironmentVariableA", (unsigned long) dllSetEnvironmentVariableA);
  g_dlls.kernel32.AddExport("CreateDirectoryA", (unsigned long) dllCreateDirectoryA);

  g_dlls.kernel32.AddExport("GetProcessAffinityMask", (unsigned long) dllGetProcessAffinityMask);

  g_dlls.kernel32.AddExport("lstrcpyA", (unsigned long)lstrcpyA);
  g_dlls.kernel32.AddExport("GetProcessTimes", (unsigned long)dllGetProcessTimes);

  g_dlls.kernel32.AddExport("GetLocaleInfoA", (unsigned long)dllGetLocaleInfoA);
  g_dlls.kernel32.AddExport("GetConsoleCP", (unsigned long)dllGetConsoleCP);
  g_dlls.kernel32.AddExport("GetConsoleOutputCP", (unsigned long)dllGetConsoleOutputCP);
  g_dlls.kernel32.AddExport("SetConsoleCtrlHandler", (unsigned long)dllSetConsoleCtrlHandler);
  g_dlls.kernel32.AddExport("GetExitCodeThread", (unsigned long)GetExitCodeThread);
  g_dlls.kernel32.AddExport("ResumeThread", (unsigned long)ResumeThread);
  g_dlls.kernel32.AddExport("ExitThread", (unsigned long)ExitThread);
  g_dlls.kernel32.AddExport("VirtualQuery", (unsigned long)VirtualQuery);
  g_dlls.kernel32.AddExport("VirtualQueryEx", (unsigned long)VirtualQueryEx);
  g_dlls.kernel32.AddExport("VirtualProtect", (unsigned long)VirtualProtect);
  g_dlls.kernel32.AddExport("VirtualProtectEx", (unsigned long)VirtualProtectEx);
  g_dlls.kernel32.AddExport("UnhandledExceptionFilter", (unsigned long)UnhandledExceptionFilter);
  g_dlls.kernel32.AddExport("RaiseException", (unsigned long)RaiseException);
  g_dlls.kernel32.AddExport("FlsAlloc", (unsigned long)dllFlsAlloc);
  g_dlls.kernel32.AddExport("FlsGetValue", (unsigned long)dllFlsGetValue);
  g_dlls.kernel32.AddExport("FlsSetValue", (unsigned long)dllFlsSetValue);
  g_dlls.kernel32.AddExport("FlsFree", (unsigned long)dllFlsFree);
  g_dlls.kernel32.AddExport("DebugBreak", (unsigned long)DebugBreak);
  g_dlls.kernel32.AddExport("GetThreadTimes", (unsigned long)GetThreadTimes);
  g_dlls.kernel32.AddExport("EncodePointer", (unsigned long)dllEncodePointer);
  g_dlls.kernel32.AddExport("DecodePointer", (unsigned long)dllDecodePointer);

  g_dlls.kernel32.AddExport("LockFile", (unsigned long)dllLockFile);
  g_dlls.kernel32.AddExport("LockFileEx", (unsigned long)dllLockFileEx);
  g_dlls.kernel32.AddExport("UnlockFile", (unsigned long)dllUnlockFile);

  g_dlls.kernel32.AddExport("GetSystemTime", (unsigned long)GetSystemTime);
  g_dlls.kernel32.AddExport("GetFileSize", (unsigned long)GetFileSize);

}