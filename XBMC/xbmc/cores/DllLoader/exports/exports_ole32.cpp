
#include "stdafx.h"
#include "../DllLoader.h"
#include "emu_ole32.h"

Export export_ole32[] =
{
  { "CoInitialize",               -1, dllCoInitialize,               NULL },
  { "CoUninitialize",             -1, dllCoUninitialize,             NULL },
  { "CoCreateInstance",           -1, dllCoCreateInstance,           NULL },
  { "CoFreeUnusedLibraries",      -1, dllCoFreeUnusedLibraries,      NULL },
  { "StringFromGUID2",            -1, dllStringFromGUID2,            NULL },
  { "CoTaskMemFree",              -1, dllCoTaskMemFree,              NULL },
  { "CoTaskMemAlloc",             -1, dllCoTaskMemAlloc,             NULL },
  { NULL,                         -1, NULL,                          NULL }
};

