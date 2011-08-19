#pragma once

#define PLATFORM_LINUX
#define PLATFORM_MAC
#define PLATFORM_WINDOWS

#if defined(PLATFORM_LINUX) || defined(PLATFORM_MAC)
 #define PLATFORM_UNIX
#endif

