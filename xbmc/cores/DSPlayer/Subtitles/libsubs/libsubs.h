#pragma once
#include "ISubManager.h"

// The following ifdef block is the standard way of creating macros which make exporting 
// from a DLL simpler. All files within this DLL are compiled with the MPCSUBS_EXPORTS
// symbol defined on the command line. this symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see 
// DELME_API functions as being imported from a DLL, whereas this DLL sees symbols
// defined with this macro as being exported.
#ifdef MPCSUBS_EXPORTS
#define MPCSUBS_API __declspec(dllexport)
#else
#define MPCSUBS_API extern
#endif

extern "C" {
  MPCSUBS_API bool CreateSubtitleManager(IDirect3DDevice9* d3DDev, SIZE size, ISubManager ** pManager);
  MPCSUBS_API bool DeleteSubtitleManager(ISubManager * pManager);
}
