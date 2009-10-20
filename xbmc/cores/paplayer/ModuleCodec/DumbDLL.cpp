/*
THIS FILE IS NOT A PART OF THE ORIGINAL DUMB PACKAGE.
The addition of this file (and only this file) is the only change done to
the dumb source code. The project was modified to have a dll as target.
*/

#include "dumb.h"

#ifdef _LINUX
#define __declspec(x)
#endif

extern "C"
{
  __declspec(dllexport) DUH *DLL_LoadModule(const char *szFileName)
  {
    dumb_register_stdfiles();
    DUH* duh;
    duh = load_duh(szFileName);
    if (!duh)
    {
      duh = dumb_load_it(szFileName);
      if (!duh)
      {
        duh = dumb_load_xm(szFileName);
        if (!duh)
        {
          duh = dumb_load_s3m(szFileName);
          if (!duh)
          {
            duh = dumb_load_mod(szFileName);
            if (!duh)
            {
              return 0;
            }
          }
        }
      }
    }
    return duh;
  }

  __declspec(dllexport) void DLL_FreeModule(DUH *duh)
  {
    unload_duh(duh);
  }

  __declspec(dllexport) long DLL_GetModuleLength(DUH *duh)
  {
    return duh_get_length(duh);
  }

  __declspec(dllexport) long DLL_GetModulePosition(DUH_SIGRENDERER *sig)
  {
    return duh_sigrenderer_get_position(sig);
  }

  __declspec(dllexport) DUH_SIGRENDERER *DLL_StartPlayback(DUH *duh, long pos)
  {
    return duh_start_sigrenderer(duh, 0, 2, pos);
  }

  __declspec(dllexport) void DLL_StopPlayback(DUH_SIGRENDERER *sig)
  {
    duh_end_sigrenderer(sig);
  }

  __declspec(dllexport) long DLL_FillBuffer(DUH_SIGRENDERER *sig, char *buffer, int size, float volume)
  {
    return duh_render(sig, 16, 0, volume, 65536.0f/48000.0f, size/4, buffer);
  }
}
