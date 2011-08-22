#pragma once

// basic platform defines
#ifdef __linux__
 #define PLATFORM_LINUX
#endif

#ifdef WIN32
 #define PLATFORM_WINDOWS
 #include <windows.h>
#endif

#ifdef __APPLE__
 #define PLATFORM_MAC
#endif

#if defined(PLATFORM_LINUX) || defined(PLATFORM_MAC)
 #define PLATFORM_UNIX
#endif

// platform-specific type aliases
#if defined(PLATFORM_UNIX)
 #define PLATFORM_PID pid_t
#else
 #define PLATFORM_PID DWORD
#endif