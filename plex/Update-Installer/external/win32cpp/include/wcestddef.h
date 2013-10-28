
#pragma once

#pragma comment(linker, "/nodefaultlib:libc.lib")
#pragma comment(linker, "/nodefaultlib:libcd.lib")


#include <ceconfig.h>
#if defined(WIN32_PLATFORM_PSPC) || defined(WIN32_PLATFORM_WFSP)
  #define SHELL_AYGSHELL
#endif

#ifdef _CE_DCOM
  #define _ATL_APARTMENT_THREADED
#endif

#if defined(WIN32_PLATFORM_PSPC) || defined(WIN32_PLATFORM_WFSP)
  #ifndef _DEVICE_RESOLUTION_AWARE
    #define _DEVICE_RESOLUTION_AWARE
  #endif
#endif


#if _WIN32_WCE == 420 || _WIN32_WCE == 0x420
  // For Pocket PC 2003
  #pragma comment(lib, "ccrtrtti.lib")
#endif

#if _MSC_VER >= 1300

  // NOTE - this value is not strongly correlated to the Windows CE OS version being targeted
  #undef  WINVER
  #define WINVER _WIN32_WCE

  #ifdef _DEVICE_RESOLUTION_AWARE
    #include "DeviceResolutionAware.h"
  #endif

  #if _WIN32_WCE < 0x500 && ( defined(WIN32_PLATFORM_PSPC) || defined(WIN32_PLATFORM_WFSP) )
    #ifdef _X86_
      #if defined(_DEBUG)
        #pragma comment(lib, "libcmtx86d.lib")
      #else
        #pragma comment(lib, "libcmtx86.lib")
      #endif
    #endif
  #endif

  #include <altcecrt.h>

#endif// _MSC_VER >= 1300

#ifdef SHELL_AYGSHELL
  #include <aygshell.h>
  #pragma comment(lib, "aygshell.lib")
#endif // SHELL_AYGSHELL

// TODO: reference additional headers your program requires here
