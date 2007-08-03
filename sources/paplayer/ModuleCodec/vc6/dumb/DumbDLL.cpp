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
  int __declspec(dllexport) DLL_LoadModule(const char* szFileName)
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
    return (int)duh;
  }
  
  void __declspec(dllexport) DLL_FreeModule(int duh)
  {
    unload_duh((DUH*)duh);
  }

  int __declspec(dllexport) DLL_GetModuleLength(int duh)
  {
    return duh_get_length((DUH*)duh);
  }

  int __declspec(dllexport) DLL_GetModulePosition(int sic)
  {
    return duh_sigrenderer_get_position((DUH_SIGRENDERER*)sic);
  }

  int __declspec(dllexport) DLL_StartPlayback(int duh, long pos)
  {
    return (int)duh_start_sigrenderer((DUH*)duh, 0, 2, pos);
  }

  void __declspec(dllexport) DLL_StopPlayback(int sic)
  {
    duh_end_sigrenderer((DUH_SIGRENDERER*)sic);
  }

  long __declspec(dllexport) DLL_FillBuffer(int duh, int sic, char* buffer, int size, float volume)
  {
    return duh_render((DUH_SIGRENDERER*)sic,16,0,volume,65536.f/48000.f,size/4,buffer);
  }
}
