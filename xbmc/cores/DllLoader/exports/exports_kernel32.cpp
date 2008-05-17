/*
 *      Copyright (C) 2005-2008 Team XBMC
 *      http://www.xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */
 
#include "stdafx.h"
#include "../DllLoader.h"

#include "emu_kernel32.h"
#include "../dll_tracker_library.h"
#include "../dll_tracker_memory.h"
#include "../dll_tracker_critical_section.h"

Export export_kernel32[] =
{
  { "AddAtomA",                                     -1, (void*)dllAddAtomA,                                  NULL },
  { "FindAtomA",                                    -1, (void*)dllFindAtomA,                                 NULL },
  { "GetAtomNameA",                                 -1, (void*)dllGetAtomNameA,                              NULL },
  { "CreateThread",                                 -1, (void*)dllCreateThread,                              NULL },
  { "FindClose",                                    -1, (void*)dllFindClose,                                 NULL },
#ifndef _LINUX
  { "FindFirstFileA",                               -1, (void*)FindFirstFileA,                               NULL },
  { "FindNextFileA",                                -1, (void*)FindNextFileA,                                NULL },
#endif
  { "GetFileAttributesA",                           -1, (void*)dllGetFileAttributesA,                        NULL },
  { "GetLastError",                                 -1, (void*)GetLastError,                                 NULL },
  { "SetLastError",                                 -1, (void*)SetLastError,                                 NULL },
  { "RestoreLastError",                             -1, (void*)SetLastError,                                 NULL },
  { "SetUnhandledExceptionFilter",                  -1, (void*)dllSetUnhandledExceptionFilter,               NULL },
  { "Sleep",                                        -1, (void*)dllSleep,                                     NULL },
  { "SleepEx",                                      -1, (void*)SleepEx,                                      NULL },
  { "TerminateThread",                              -1, (void*)dllTerminateThread,                           NULL },
  { "GetCurrentThread",                             -1, (void*)dllGetCurrentThread,                          NULL },
  { "QueryPerformanceCounter",                      -1, (void*)QueryPerformanceCounter,                      NULL },
#ifdef _XBOX
  { "QueryPerformanceFrequency",                    -1, (void*)QueryPerformanceFrequencyXbox,                NULL },
#else
  { "QueryPerformanceFrequency",                    -1, (void*)QueryPerformanceFrequency,                    NULL },
#endif
  { "SetThreadPriority",                            -1, (void*)SetThreadPriority,                            NULL },
  { "GetTickCount",                                 -1, (void*)GetTickCount,                                 NULL },
  { "GetCurrentThreadId",                           -1, (void*)GetCurrentThreadId,                           NULL },
  { "GetCurrentProcessId",                          -1, (void*)dllGetCurrentProcessId,                       NULL },
  { "GetSystemTimeAsFileTime",                      -1, (void*)GetSystemTimeAsFileTime,                      NULL },
  { "OutputDebugStringA",                           -1, (void*)OutputDebugString,                            NULL },
  { "DisableThreadLibraryCalls",                    -1, (void*)dllDisableThreadLibraryCalls,                 NULL },
  { "GlobalMemoryStatus",                           -1, (void*)GlobalMemoryStatus,                           NULL },
#ifndef _LINUX
  { "CreateEventA",                                 -1, (void*)CreateEventA,                                 NULL },
#else
  { "CreateEventA",                                 -1, (void*)CreateEvent,                                  NULL },
#endif
  { "ResetEvent",                                   -1, (void*)ResetEvent,                                   NULL },
  { "PulseEvent",                                   -1, (void*)PulseEvent,                                   NULL },
  { "WaitForSingleObject",                          -1, (void*)dllWaitForSingleObject,                       NULL },
  { "LoadLibraryA",                                 -1, (void*)dllLoadLibraryA,                              (void*)track_LoadLibraryA },
  { "FreeLibrary",                                  -1, (void*)dllFreeLibrary,                               (void*)track_FreeLibrary },
  { "GetProcAddress",                               -1, (void*)dllGetProcAddress,                            NULL },
  { "LeaveCriticalSection",                         -1, (void*)dllLeaveCriticalSection,                      NULL },
  { "EnterCriticalSection",                         -1, (void*)dllEnterCriticalSection,                      NULL },
  { "DeleteCriticalSection",                        -1, (void*)dllDeleteCriticalSection,                     (void*)track_DeleteCriticalSection },
  { "InitializeCriticalSection",                    -1, (void*)dllInitializeCriticalSection,                 (void*)track_InitializeCriticalSection },
  { "GetSystemInfo",                                -1, (void*)dllGetSystemInfo,                             NULL },
  { "CloseHandle",                                  -1, (void*)CloseHandle,                                  NULL },
  { "GetPrivateProfileIntA",                        -1, (void*)dllGetPrivateProfileIntA,                     NULL },
  { "WaitForMultipleObjects",                       -1, (void*)dllWaitForMultipleObjects,                    NULL },
  { "SetEvent",                                     -1, (void*)SetEvent,                                     NULL },
  { "TlsAlloc",                                     -1, (void*)dllTlsAlloc,                                  NULL },
  { "TlsFree",                                      -1, (void*)dllTlsFree,                                   NULL },
  { "TlsGetValue",                                  -1, (void*)dllTlsGetValue,                               NULL },
  { "TlsSetValue",                                  -1, (void*)dllTlsSetValue,                               NULL },
#ifndef _LINUX
  { "HeapFree",                                     -1, (void*)HeapFree,                                     NULL },
  { "HeapAlloc",                                    -1, (void*)HeapAlloc,                                    NULL },
  { "HeapReAlloc",                                  -1, (void*)HeapReAlloc,                                  NULL },
  { "HeapSize",                                     -1, (void*)HeapSize,                                     NULL },
  { "HeapDestroy",                                  -1, (void*)HeapDestroy,                                  (void*)track_HeapDestroy },
  { "HeapCreate",                                   -1, (void*)HeapCreate,                                   (void*)track_HeapCreate },
  { "LocalFree",                                    -1, (void*)LocalFree,                                    NULL },
  { "LocalAlloc",                                   -1, (void*)LocalAlloc,                                   NULL },
  { "LocalReAlloc",                                 -1, (void*)LocalReAlloc,                                 NULL },
  { "LocalLock",                                    -1, (void*)LocalLock,                                    NULL },
  { "LocalUnlock",                                  -1, (void*)LocalUnlock,                                  NULL },
  { "LocalHandle",                                  -1, (void*)LocalHandle,                                  NULL },
  { "GlobalAlloc",                                  -1, (void*)GlobalAlloc,                                  NULL },
  { "GlobalLock",                                   -1, (void*)GlobalLock,                                   NULL },
  { "GlobalUnlock",                                 -1, (void*)GlobalUnlock,                                 NULL },
  { "GlobalHandle",                                 -1, (void*)GlobalHandle,                                 NULL },
  { "GlobalFree",                                   -1, (void*)GlobalFree,                                   NULL },
  { "GetProcessHeap",                               -1, (void*)GetProcessHeap,                               NULL },
  { "DeviceIoControl",                              -1, (void*)DeviceIoControl,                              NULL },
#endif
  { "InterlockedIncrement",                         -1, (void*)InterlockedIncrement,                         NULL },
  { "InterlockedDecrement",                         -1, (void*)InterlockedDecrement,                         NULL },
  { "InterlockedExchange",                          -1, (void*)InterlockedExchange,                          NULL },
  { "GetModuleHandleA",                             -1, (void*)dllGetModuleHandleA,                          NULL },
  { "InterlockedCompareExchange",                   -1, (void*)InterlockedCompareExchange,                   NULL },
  { "GetVersionExA",                                -1, (void*)dllGetVersionExA,                             NULL },
  { "GetVersionExW",                                -1, (void*)dllGetVersionExW,                             NULL },
  { "GetProfileIntA",                               -1, (void*)dllGetProfileIntA,                            NULL },
  { "CreateFileA",                                  -1, (void*)dllCreateFileA,                               NULL },
  { "ReadFile",                                     -1, (void*)ReadFile,                                     NULL },
  { "dllDVDReadFile",                               -1, (void*)dllDVDReadFileLayerChangeHack,                NULL },
  { "SetFilePointer",                               -1, (void*)SetFilePointer,                               NULL },
#ifdef _XBOX
  { "xboxopendvdrom",                               -1, (void*)xboxopendvdrom,                               NULL },
#endif
  { "GetVersion",                                   -1, (void*)dllGetVersion,                                NULL },
#ifndef _LINUX
  { "MulDiv",                                       -1, (void*)MulDiv,                                       NULL },
  { "lstrlenA",                                     -1, (void*)lstrlenA,                                     NULL },
  { "lstrlenW",                                     -1, (void*)lstrlenW,                                     NULL },
#endif
  { "LoadLibraryExA",                               -1, (void*)dllLoadLibraryExA,                            (void*)track_LoadLibraryExA }, 

#ifdef _LINUX
  { "DeleteFileA",                                 -1,  (void*)DeleteFile,                                   NULL },
#else
  { "DeleteFileA",                                 -1,  (void*)DeleteFileA,                                  NULL },
#endif
  { "GetModuleFileNameA",                           -1, (void*)dllGetModuleFileNameA,                        NULL },
  { "FreeEnvironmentStringsW",                      -1, (void*)dllFreeEnvironmentStringsW,                   NULL },
  { "GetOEMCP",                                     -1, (void*)dllGetOEMCP,                                  NULL },
  { "SetEndOfFile",                                 -1, (void*)SetEndOfFile,                                 NULL },
  { "RtlUnwind",                                    -1, (void*)dllRtlUnwind,                                 NULL },
  { "GetCommandLineA",                              -1, (void*)dllGetCommandLineA,                           NULL },
  { "ExitProcess",                                  -1, (void*)dllExitProcess,                               NULL },
  { "TerminateProcess",                             -1, (void*)dllTerminateProcess,                          NULL },
  { "GetCurrentProcess",                            -1, (void*)dllGetCurrentProcess,                         NULL },
  { "WriteFile",                                    -1, (void*)WriteFile,                                    NULL },
  { "GetACP",                                       -1, (void*)dllGetACP,                                    NULL },
  { "SetHandleCount",                               -1, (void*)dllSetHandleCount,                            NULL },
  { "GetStdHandle",                                 -1, (void*)dllGetStdHandle,                              NULL },
  { "GetFileType",                                  -1, (void*)dllGetFileType,                               NULL },
  { "GetStartupInfoA",                              -1, (void*)dllGetStartupInfoA,                           NULL },
  { "FreeEnvironmentStringsA",                      -1, (void*)dllFreeEnvironmentStringsA,                   NULL },
  { "WideCharToMultiByte",                          -1, (void*)dllWideCharToMultiByte,                       NULL },
  { "GetEnvironmentStrings",                        -1, (void*)dllGetEnvironmentStrings,                     NULL },
  { "GetEnvironmentStringsW",                       -1, (void*)dllGetEnvironmentStringsW,                    NULL },
  { "GetEnvironmentVariableA",                      -1, (void*)dllGetEnvironmentVariableA,                   NULL },
#ifndef _LINUX
  { "VirtualFree",                                  -1, (void*)VirtualFree,                                  (void*)track_VirtualFree },
  { "VirtualFreeEx",                                -1, (void*)VirtualFreeEx,                                (void*)track_VirtualFreeEx },
  { "VirtualAlloc",                                 -1, (void*)VirtualAlloc,                                 (void*)track_VirtualAlloc },
  { "VirtualAllocEx",                               -1, (void*)VirtualAllocEx,                               (void*)track_VirtualAllocEx },
  { "VirtualQuery",                                 -1, (void*)VirtualQuery,                                 NULL },
  { "VirtualQueryEx",                               -1, (void*)VirtualQueryEx,                               NULL },
  { "VirtualProtect",                               -1, (void*)VirtualProtect,                               NULL },
  { "VirtualProtectEx",                             -1, (void*)VirtualProtectEx,                             NULL },
#endif
  { "MultiByteToWideChar",                          -1, (void*)dllMultiByteToWideChar,                       NULL },
  { "LCMapStringA",                                 -1, (void*)dllLCMapStringA,                              NULL },
  { "LCMapStringW",                                 -1, (void*)dllLCMapStringW,                              NULL },
  { "SetStdHandle",                                 -1, (void*)dllSetStdHandle,                              NULL },
  { "FlushFileBuffers",                             -1, (void*)FlushFileBuffers,                             NULL },
  { "GetStringTypeA",                               -1, (void*)dllGetStringTypeA,                            NULL },
  { "GetStringTypeW",                               -1, (void*)dllGetStringTypeW,                            NULL },
#ifndef _LINUX
  { "IsBadWritePtr",                                -1, (void*)IsBadWritePtr,                                NULL },
  { "IsBadReadPtr",                                 -1, (void*)IsBadReadPtr,                                 NULL },
  { "IsBadCodePtr",                                 -1, (void*)IsBadCodePtr,                                 NULL },
#endif
  { "GetCPInfo",                                    -1, (void*)dllGetCPInfo,                                 NULL },

#ifdef _LINUX
  { "CreateMutexA",                                 -1, (void*)CreateMutex,                                  NULL },
#else
  { "CreateMutexA",                                 -1, (void*)CreateMutexA,                                 NULL },
#endif
  { "ReleaseMutex",                                 -1, (void*)ReleaseMutex,                                 NULL },

#ifndef _LINUX
  { "CreateSemaphoreA",                             -1, (void*)CreateSemaphoreA,                             NULL },
  { "ReleaseSemaphore",                             -1, (void*)ReleaseSemaphore,                             NULL },
#endif

  { "GetThreadLocale",                              -1, (void*)dllGetThreadLocale,                           NULL },
  { "SetPriorityClass",                             -1, (void*)dllSetPriorityClass,                          NULL },
  { "FormatMessageA",                               -1, (void*)dllFormatMessageA,                            NULL },
  { "GetFullPathNameA",                             -1, (void*)dllGetFullPathNameA,                          NULL },
#ifdef _XBOX
  { "SignalObjectAndWait",                          -1, (void*)SignalObjectAndWait,                          NULL },
#endif
  { "ExpandEnvironmentStringsA",                    -1, (void*)dllExpandEnvironmentStringsA,                 NULL },
#ifndef _LINUX
  { "GetVolumeInformationA",                        -1, (void*)GetVolumeInformationA,                        NULL },
#endif
  { "GetWindowsDirectoryA",                         -1, (void*)dllGetWindowsDirectoryA,                      NULL },
  { "GetSystemDirectoryA",                          -1, (void*)dllGetSystemDirectoryA,                       NULL },
  { "DuplicateHandle",                              -1, (void*)dllDuplicateHandle,                           NULL },
  { "GetShortPathNameA",                            -1, (void*)dllGetShortPathName,                          NULL },
  { "GetTempPathA",                                 -1, (void*)dllGetTempPathA,                              NULL },
  { "SetErrorMode",                                 -1, (void*)dllSetErrorMode,                              NULL },
  { "IsProcessorFeaturePresent",                    -1, (void*)dllIsProcessorFeaturePresent,                 NULL },
  { "FileTimeToLocalFileTime",                      -1, (void*)FileTimeToLocalFileTime,                      NULL },
  { "FileTimeToSystemTime",                         -1, (void*)FileTimeToSystemTime,                         NULL },
  { "GetTimeZoneInformation",                       -1, (void*)GetTimeZoneInformation,                       NULL },

  { "GetCurrentDirectoryA",                         -1, (void*)dllGetCurrentDirectoryA,                      NULL },
  { "SetCurrentDirectoryA",                         -1, (void*)dllSetCurrentDirectoryA,                      NULL },

  { "SetEnvironmentVariableA",                      -1, (void*)dllSetEnvironmentVariableA,                   NULL },
  { "CreateDirectoryA",                             -1, (void*)dllCreateDirectoryA,                          NULL },

  { "GetProcessAffinityMask",                       -1, (void*)dllGetProcessAffinityMask,                    NULL },

#ifndef _LINUX
  { "lstrcpyA",                                     -1, (void*)lstrcpyA,                                     NULL },
#endif
  { "GetProcessTimes",                              -1, (void*)dllGetProcessTimes,                           NULL },

  { "GetLocaleInfoA",                               -1, (void*)dllGetLocaleInfoA,                            NULL },
  { "GetConsoleCP",                                 -1, (void*)dllGetConsoleCP,                              NULL },
  { "GetConsoleOutputCP",                           -1, (void*)dllGetConsoleOutputCP,                        NULL },
  { "SetConsoleCtrlHandler",                        -1, (void*)dllSetConsoleCtrlHandler,                     NULL },
#ifndef _LINUX
  { "GetExitCodeThread",                            -1, (void*)GetExitCodeThread,                            NULL },
  { "ResumeThread",                                 -1, (void*)ResumeThread,                                 NULL },
  { "ExitThread",                                   -1, (void*)ExitThread,                                   NULL },
  { "UnhandledExceptionFilter",                     -1, (void*)UnhandledExceptionFilter,                     NULL },
  { "RaiseException",                               -1, (void*)RaiseException,                               NULL },
  { "DebugBreak",                                   -1, (void*)DebugBreak,                                   NULL },
#endif
  { "FlsAlloc",                                     -1, (void*)dllFlsAlloc,                                  NULL },
  { "FlsGetValue",                                  -1, (void*)dllFlsGetValue,                               NULL },
  { "FlsSetValue",                                  -1, (void*)dllFlsSetValue,                               NULL },
  { "FlsFree",                                      -1, (void*)dllFlsFree,                                   NULL },
  { "GetThreadTimes",                               -1, (void*)GetThreadTimes,                               NULL },
  { "EncodePointer",                                -1, (void*)dllEncodePointer,                             NULL },
  { "DecodePointer",                                -1, (void*)dllDecodePointer,                             NULL },

  { "LockFile",                                     -1, (void*)dllLockFile,                                  NULL },
  { "LockFileEx",                                   -1, (void*)dllLockFileEx,                                NULL },
  { "UnlockFile",                                   -1, (void*)dllUnlockFile,                                NULL },

#ifndef _LINUX
  { "GetSystemTime",                                -1, (void*)GetSystemTime,                                NULL },
#endif
  { "GetFileSize",                                  -1, (void*)GetFileSize,                                  NULL },
  { "FindResourceA",                                -1, (void*)dllFindResourceA,                             NULL },
  { "LoadResource",                                 -1, (void*)dllLoadResource,                              NULL },
  { "LockResource",                                 -1, (void*)dllLockResource,                              NULL },
  { "GlobalSize",                                   -1, (void*)dllGlobalSize,                                NULL },
  { "SizeofResource",                               -1, (void*)dllSizeofResource,                            NULL },
  { NULL,                                           -1, (void*)NULL,                                         NULL }
};
