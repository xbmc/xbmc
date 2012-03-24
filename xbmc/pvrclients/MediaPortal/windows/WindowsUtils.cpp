#include "WindowsUtils.h"
#include <windows.h>
#include <stdio.h>
#include <string>

namespace OS
{
  typedef void (WINAPI *PGNSI)(LPSYSTEM_INFO);

  WindowsVersion Version()
  {
    OSVERSIONINFOEX osvi;
    SYSTEM_INFO si;
    PGNSI GetNativeSystemInfo;
    BOOL bOsVersionInfoEx;

    ZeroMemory(&si, sizeof(SYSTEM_INFO));
    ZeroMemory(&osvi, sizeof(OSVERSIONINFOEX));

    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
    bOsVersionInfoEx = GetVersionEx((OSVERSIONINFO*) &osvi);

    if(bOsVersionInfoEx == NULL ) return Unknown;

    // Call GetNativeSystemInfo if supported or GetSystemInfo otherwise.
    GetNativeSystemInfo = (PGNSI) GetProcAddress( GetModuleHandle(TEXT("kernel32.dll")), "GetNativeSystemInfo");
    if (NULL != GetNativeSystemInfo)
      GetNativeSystemInfo(&si);
    else
      GetSystemInfo(&si);

    if ( VER_PLATFORM_WIN32_NT==osvi.dwPlatformId && osvi.dwMajorVersion > 4 )
    {
      // Test for the specific product.
      if ( osvi.dwMajorVersion == 6 )
      {
        if( osvi.dwMinorVersion == 0 )
        {
          if( osvi.wProductType == VER_NT_WORKSTATION )
            return WindowsVista;
          else
            return WindowsServer2008;
        }

        if ( osvi.dwMinorVersion == 1 )
        {
          if( osvi.wProductType == VER_NT_WORKSTATION )
            return Windows7;
          else
            return WindowsServer2008R2;
        }
      }

      if ( osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 2 )
      {
        if( GetSystemMetrics(SM_SERVERR2) )
          return WindowsServer2003R2;
        else if ( osvi.wSuiteMask & VER_SUITE_STORAGE_SERVER )
          return WindowsStorageServer2003;
        else if ( osvi.wSuiteMask & VER_SUITE_WH_SERVER )
          return WindowsHomeServer;
        else if( osvi.wProductType == VER_NT_WORKSTATION && si.wProcessorArchitecture==PROCESSOR_ARCHITECTURE_AMD64)
          return WindowsXPx64;
        else
          return WindowsServer2003;
      }

      if ( osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 1 )
      {
        if( osvi.wSuiteMask & VER_SUITE_PERSONAL )
          return WindowsXPHome;
        else
          return WindowsXPPro;
      }

      if ( osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 0 )
      {
        return Windows2000;
      }
    }

    return Unknown;
  }

  bool GetEnvironmentVariable(const char* strVarName, std::string& strResult)
  {
    char strBuffer[4096];
    DWORD dwRet;

    dwRet = ::GetEnvironmentVariable(strVarName, strBuffer, 4096);

    if(0 == dwRet)
    {
      dwRet = GetLastError();
      if( ERROR_ENVVAR_NOT_FOUND == dwRet )
      {
        strResult.clear();
        return false;
      }
    }
    strResult = strBuffer;
    return true;
  }
}
