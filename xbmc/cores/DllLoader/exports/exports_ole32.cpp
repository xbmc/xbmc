
#include "..\..\..\stdafx.h"
#include "..\DllLoaderContainer.h"
#include "emu_ole32.h"

void export_ole32()
{
  g_dlls.ole32.AddExport("CoInitialize", (unsigned long)dllCoInitialize);
  g_dlls.ole32.AddExport("CoUninitialize", (unsigned long)dllCoUninitialize);
  g_dlls.ole32.AddExport("CoCreateInstance", (unsigned long)dllCoCreateInstance);
  g_dlls.ole32.AddExport("CoFreeUnusedLibraries", (unsigned long)dllCoFreeUnusedLibraries);
  g_dlls.ole32.AddExport("StringFromGUID2", (unsigned long)dllStringFromGUID2);
  g_dlls.ole32.AddExport("CoTaskMemFree", (unsigned long)dllCoTaskMemFree);
  g_dlls.ole32.AddExport("CoTaskMemAlloc", (unsigned long)dllCoTaskMemAlloc);
  //Exp2Dll* ole32_exp3 = new Exp2Dll("ole32.dll", "CoCreateInstance", (unsigned long)CoCreateInstance);
}

