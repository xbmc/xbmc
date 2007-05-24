
#include "stdafx.h"
#include "../DllLoader.h"
#include "emu_registry.h"

Export export_advapi32[] =
{
  { "RegCloseKey",                -1, dllRegCloseKey,                NULL },
  { "RegOpenKeyExA",              -1, dllRegOpenKeyExA,              NULL },
  { "RegOpenKeyA",                -1, dllRegOpenKeyA,                NULL },
  { "RegSetValueA",               -1, dllRegSetValueA,               NULL },
  { "RegEnumKeyExA",              -1, dllRegEnumKeyExA,              NULL },
  { "RegDeleteKeyA",              -1, dllRegDeleteKeyA,              NULL },
  { "RegQueryValueExA",           -1, dllRegQueryValueExA,           NULL },
  { "RegQueryValueExW",           -1, dllRegQueryValueExW,           NULL },
  { "RegCreateKeyA",              -1, dllRegCreateKeyA,              NULL },
  { "RegSetValueExA",             -1, dllRegSetValueExA,             NULL },
  { "RegCreateKeyExA",            -1, dllRegCreateKeyExA,            NULL },
  { "RegEnumValueA",              -1, dllRegEnumValueA,              NULL },
  { "RegQueryInfoKeyA",           -1, dllRegQueryInfoKeyA,           NULL },
  { "CryptAcquireContextA",       -1, dllCryptAcquireContextA,       NULL },
  { "CryptGenRandom",             -1, dllCryptGenRandom,             NULL },
  { "CryptReleaseContext",        -1, dllCryptReleaseContext,        NULL },
  { "RegQueryValueA",             -1, dllRegQueryValueA,             NULL },
  { NULL,                         -1, NULL,                          NULL }
};
