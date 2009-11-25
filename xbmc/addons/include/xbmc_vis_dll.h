#ifndef __XBMC_VIS_H__
#define __XBMC_VIS_H__

#include "xbmc_addon_dll.h"
#include "xbmc_vis_types.h"
#include "libvisualisation.h"

#include <ctype.h>
#ifdef HAS_XBOX_HARDWARE
#include <xtl.h>
#pragma comment (lib, "lib/xbox_dx8.lib" )
#else
#ifndef __APPLE__
#include <sys/sysinfo.h>
#endif
#ifdef WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif
#include <sys/stat.h>
#include <errno.h>
#endif

int htoi(const char *str) /* Convert hex string to integer */
{
  unsigned int digit, number = 0;
  while (*str)
  {
    if (isdigit(*str))
      digit = *str - '0';
    else
      digit = tolower(*str)-'a'+10;
    number<<=4;
    number+=digit;
    str++;
  }
  return number;
}

extern "C"
{
  // exports for d3d hacks
#ifndef HAS_SDL_OPENGL
  void d3dSetTextureStageState( int x, DWORD dwY, DWORD dwZ);
  void d3dSetRenderState(DWORD dwY, DWORD dwZ);
#endif

#ifdef HAS_SDL_OPENGL
#ifndef D3DCOLOR_RGBA
#define D3DCOLOR_RGBA(r,g,b,a) (r||(g<<8)||(b<<16)||(a<<24))
#endif
#endif

  // Functions that your visualisation must implement
  void Start(int iChannels, int iSamplesPerSec, int iBitsPerSample, const char* szSongName);
  void AudioData(const short* pAudioData, int iAudioDataLength, float *pFreqData, int iFreqDataLength);
  void Render();
  void Stop();
  bool OnAction(long action, const void *param);
  void GetInfo(VIS_INFO* pInfo);
  viz_preset_list_t GetPresets();
  viz_preset_t GetCurrentPreset();

  // function to export the above structure to XBMC
  void __declspec(dllexport) get_addon(struct Visualisation* pVisz)
  {
    pVisz->Start = Start;
    pVisz->AudioData = AudioData;
    pVisz->Render = Render;
    pVisz->Stop = Stop;
    pVisz->OnAction = OnAction;
    pVisz->GetInfo = GetInfo;
    pVisz->GetPresets = GetPresets;
  };
};

#endif
