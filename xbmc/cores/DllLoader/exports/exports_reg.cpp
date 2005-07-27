
#include "..\..\..\stdafx.h"
#include "..\DllLoaderContainer.h"
#include "emu_registry.h"

void export_reg()
{
  g_dlls.advapi32.AddExport("RegCloseKey", (unsigned long)dllRegCloseKey);
  g_dlls.advapi32.AddExport("RegOpenKeyExA", (unsigned long)dllRegOpenKeyExA);
  g_dlls.advapi32.AddExport("RegOpenKeyA", (unsigned long)dllRegOpenKeyA);
  g_dlls.advapi32.AddExport("RegSetValueA", (unsigned long)dllRegSetValueA);
  g_dlls.advapi32.AddExport("RegEnumKeyExA", (unsigned long)dllRegEnumKeyExA);
  g_dlls.advapi32.AddExport("RegDeleteKeyA", (unsigned long)dllRegDeleteKeyA);
  g_dlls.advapi32.AddExport("RegQueryValueExA", (unsigned long)dllRegQueryValueExA);
  g_dlls.advapi32.AddExport("RegCreateKeyA", (unsigned long)dllRegCreateKeyA);
  g_dlls.advapi32.AddExport("RegSetValueExA", (unsigned long)dllRegSetValueExA);
  g_dlls.advapi32.AddExport("RegCreateKeyExA", (unsigned long)dllRegCreateKeyExA);
  g_dlls.advapi32.AddExport("RegEnumValueA", (unsigned long)dllRegEnumValueA);
  g_dlls.advapi32.AddExport("RegQueryInfoKeyA", (unsigned long)dllRegQueryInfoKeyA);
  g_dlls.advapi32.AddExport("CryptAcquireContextA", (unsigned long)dllCryptAcquireContextA);
  g_dlls.advapi32.AddExport("CryptGenRandom", (unsigned long)dllCryptGenRandom);
  g_dlls.advapi32.AddExport("CryptReleaseContext", (unsigned long)dllCryptReleaseContext);
  g_dlls.advapi32.AddExport("RegQueryValueA", (unsigned long)dllRegQueryValueA);
}
