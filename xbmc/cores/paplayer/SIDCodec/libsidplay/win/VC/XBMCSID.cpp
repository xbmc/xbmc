#include "../../src/player.h"
#include "../../../builders/resid-builder/include/sidplay/builders/resid.h"

#ifdef _LINUX
#define __declspec(x)
#else
#include <windows.h>
#endif

struct SSid
{
  __sidplay2__::Player player;
  sid2_config_t config;
  SidTune tune;
};

extern "C"
{
    __declspec(dllexport) int DLL_Init()
    {
      return 0;
    }

    __declspec(dllexport) void* DLL_LoadSID(const char* szFileName)
    {
      SSid* result = new SSid;
      result->tune.load(szFileName,true);

      result->config.sidEmulation = NULL;
      
      return result;
    }

    __declspec(dllexport) void DLL_StartPlayback(void* sid, int track)
    {
      SSid* result = (SSid*)sid;

      result->tune.selectSong(track);
      result->player.load(&result->tune);
      result->config.clockDefault = SID2_CLOCK_PAL;
      result->config.clockForced = false;
      result->config.clockSpeed = SID2_CLOCK_CORRECT;
      result->config.emulateStereo = false;
      result->config.environment = sid2_envR;
      result->config.forceDualSids = false;
      result->config.frequency = 48000;
      result->config.leftVolume = 255;
      result->config.optimisation = SID2_DEFAULT_OPTIMISATION;
      result->config.playback = sid2_mono;
      result->config.powerOnDelay = SID2_DEFAULT_POWER_ON_DELAY;
      result->config.precision = 16;
      result->config.rightVolume = 255;
      result->config.sampleFormat = SID2_LITTLE_SIGNED;
      if (!result->config.sidEmulation)
      {
        ReSIDBuilder* rs = new ReSIDBuilder("Resid Builder");
        rs->create (result->player.info().maxsids);
        rs->filter(true);
        rs->sampling(48000);
        result->config.sidEmulation = rs;
      }

      result->player.config(result->config);
      result->player.fastForward(100*32);
    }

    __declspec(dllexport) int DLL_FillBuffer(void* sid, void* szBuffer, int length)
    {
      SSid* player = (SSid*)sid;
      int iResult = player->player.play(szBuffer,length);
      return iResult;
    }

    __declspec(dllexport) void DLL_FreeSID(void* sid)
    {
      SSid* player = (SSid*)sid;
      delete player;
    }

    __declspec(dllexport) int DLL_GetNumberOfSongs(const char* szFileName)
    {
      SidTune tune;
      tune.load(szFileName,true);
      return tune.getInfo().songs;
    }

    __declspec(dllexport) void DLL_SetSpeed(void* sid, int speed)
    {
      SSid* player = (SSid*)sid;
      player->player.fastForward(speed);
    }
}

