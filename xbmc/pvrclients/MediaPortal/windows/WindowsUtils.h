#pragma once

#include <string>

namespace OS
{
  typedef enum _WindowsVersion
  {
    Unknown = 0,
    Windows2000 = 10,
    WindowsXPHome = 20,
    WindowsXPPro = 21,
    WindowsXPx64 = 22,
    WindowsVista = 30,
    WindowsServer2003 = 31,
    WindowsStorageServer2003 = 32,
    WindowsHomeServer = 33,
    WindowsServer2003R2 = 34,
    Windows7 = 40,
    WindowsServer2008 = 41,
    WindowsServer2008R2 = 42
  } WindowsVersion;

  WindowsVersion Version();

  bool GetEnvironmentVariable(const char* strVarName, std::string& strResult);
}