#ifndef __XBMC_VIS_H__
#define __XBMC_VIS_H__

#include <vector>
#ifdef HAS_XBOX_HARDWARE
#include <xtl.h>
#else
#ifdef _LINUX
#include "../xbmc/linux/PlatformInclude.h"
#ifndef __APPLE__
#include <sys/sysinfo.h>
#endif
#else
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif
#include "../xbmc/utils/log.h"
#include "../xbmc/visualizations/VisualisationTypes.h"
#include <sys/stat.h>
#include <errno.h>
#endif

using namespace std;

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

//#define NEW_STRING(str, ch) { str = new char[strlen(ch) + 1]; strcpy(str, ch); };
#ifdef HAS_XBOX_HARDWARE
#pragma comment (lib, "lib/xbox_dx8.lib" )
#endif

// the settings vector
vector<VisSetting> m_vecSettings;


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

  // the action commands ( see Visualisation.h )
  #define VIS_ACTION_NEXT_PRESET       1
  #define VIS_ACTION_PREV_PRESET       2
  #define VIS_ACTION_LOAD_PRESET       3
  #define VIS_ACTION_RANDOM_PRESET     4
  #define VIS_ACTION_LOCK_PRESET       5
  #define VIS_ACTION_RATE_PRESET_PLUS  6
  #define VIS_ACTION_RATE_PRESET_MINUS 7
  #define VIS_ACTION_UPDATE_ALBUMART   8
  #define VIS_ACTION_UPDATE_TRACK      9

  #define VIS_ACTION_USER 100

  // Functions that your visualisation must implement
  void Create(void* unused, int iPosX, int iPosY, int iWidth, int iHeight, const char* szVisualisationName,
              float fPixelRatio, const char *szSubModuleName);
  void Start(int iChannels, int iSamplesPerSec, int iBitsPerSample, const char* szSongName);
  void AudioData(short* pAudioData, int iAudioDataLength, float *pFreqData, int iFreqDataLength);
  void Render();
  void Stop();
  bool OnAction(long action, void *param);
  void GetInfo(VIS_INFO* pInfo);
  void GetSettings(vector<VisSetting> **vecSettings);
  void UpdateSetting(int num);
  void GetPresets(char ***pPresets, int *currentPreset, int *numPresets, bool *locked);
  int  GetSubModules(char ***names, char ***paths);

  // function to export the above structure to XBMC
  void __declspec(dllexport) get_module(struct Visualisation* pVisz)
  {
    pVisz->Create = Create;
    pVisz->Start = Start;
    pVisz->AudioData = AudioData;
    pVisz->Render = Render;
    pVisz->Stop = Stop;
    pVisz->GetInfo = GetInfo;
    pVisz->OnAction = OnAction;
    pVisz->GetSettings = GetSettings;
    pVisz->UpdateSetting = UpdateSetting;
    pVisz->GetPresets = GetPresets;
    pVisz->GetSubModules = GetSubModules;
  };
};

#endif
