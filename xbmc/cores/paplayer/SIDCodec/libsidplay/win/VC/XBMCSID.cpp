#include "../../src/player.h"
#include "../../../builders/resid-builder/include/sidplay/builders/resid.h"

#ifdef _LINUX
#include "XSyncUtils.h"
#else
#include <windows.h>
#endif

struct SSid
{
  __sidplay2__::Player player;
  sid2_config_t config;
  SidTune tune;
};

HANDLE hMutex = NULL;

extern "C"
{
    int __declspec(dllexport) DLL_Init()
    {
      if (!hMutex)
        hMutex = CreateMutex(NULL,false,NULL);
      return 0;
    }

    long __declspec(dllexport) DLL_LoadSID(const char* szFileName)
    {
      WaitForSingleObject(hMutex,INFINITE);
      SSid* result = new SSid;
      result->tune.load(szFileName,true);

      result->config.sidEmulation = NULL;
      
      ReleaseMutex(hMutex);
      return (long)result;
    }

    void __declspec(dllexport) DLL_StartPlayback(int sid, int track)
    {
      WaitForSingleObject(hMutex,INFINITE);
      SSid* result = (SSid*)sid;

      result->tune.selectSong(track);
      result->player.load(&result->tune);
      result->config.clockDefault = SID2_CLOCK_PAL;
      result->config.clockForced = false;
      result->config.clockSpeed = SID2_CLOCK_CORRECT;
      result->config.emulateStereo = true;
      result->config.environment = sid2_envR;
      result->config.forceDualSids = false;
      result->config.frequency = 48000;
      result->config.leftVolume = 255;
      result->config.optimisation = SID2_DEFAULT_OPTIMISATION;
      result->config.playback = sid2_stereo;
      result->config.powerOnDelay = SID2_DEFAULT_POWER_ON_DELAY;
      result->config.precision = 16;
      result->config.rightVolume = 255;
      result->config.sampleFormat = SID2_LITTLE_SIGNED;
      if (!result->config.sidEmulation)
      {
        ReSIDBuilder* rs = new ReSIDBuilder("Resid Builder");
        rs->create (result->player.info().maxsids);
        rs->filter(false);
        rs->sampling(48000);
        result->config.sidEmulation = rs;
      }

      result->player.config(result->config);
      result->player.fastForward(100*32);
      ReleaseMutex(hMutex);
    }

    int __declspec(dllexport) DLL_FillBuffer(int sid, void* szBuffer, int length)
    {
      WaitForSingleObject(hMutex,INFINITE);
      SSid* player = (SSid*)sid;
      int iResult = player->player.play(szBuffer,length);
      ReleaseMutex(hMutex);
      return iResult;
    }

    void __declspec(dllexport) DLL_FreeSID(int sid)
    {
      WaitForSingleObject(hMutex,INFINITE);
      SSid* player = (SSid*)sid;
      delete player;
      ReleaseMutex(hMutex);
    }

    int __declspec(dllexport) DLL_GetNumberOfSongs(const char* szFileName)
    {
      SidTune tune;
      tune.load(szFileName,true);
      return tune.getInfo().songs;
    }

    void __declspec(dllexport) DLL_SetSpeed(int sid, int speed)
    {
      SSid* player = (SSid*)sid;
      player->player.fastForward(speed);
    }
}

