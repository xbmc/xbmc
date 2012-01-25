#pragma once
#include "ISubManager.h"
#include "../ILog.h"

#define MPCSUBS_API __declspec(dllexport)

extern "C" {
  MPCSUBS_API bool CreateSubtitleManager(IDirect3DDevice9* d3DDev, SIZE size, ILog* logger, SSubSettings settings, ISubManager** pManager);
  MPCSUBS_API bool DeleteSubtitleManager(ISubManager * pManager);
}
