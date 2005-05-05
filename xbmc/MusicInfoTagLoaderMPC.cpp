
#include "stdafx.h"
#include "musicinfotagloadermpc.h"
#include "cores/paplayer/MPCCodec.h"


using namespace MUSIC_INFO;

CMusicInfoTagLoaderMPC::CMusicInfoTagLoaderMPC(void)
{}

CMusicInfoTagLoaderMPC::~CMusicInfoTagLoaderMPC()
{}

int CMusicInfoTagLoaderMPC::ReadDuration(const CStdString &strFileName)
{
  // load the mpc dll if we need it
  DllLoader *pDll = CSectionLoader::LoadDLL(MPC_DLL);
  if (!pDll) return 0;
  // resolve the exports we need
  bool (__cdecl * Open)(const char *, StreamInfo::BasicData *data, double *timeinseconds) = NULL;
  void (__cdecl * Close)() = NULL;
  pDll->ResolveExport("Open", (void **)&Open);
  pDll->ResolveExport("Close", (void **)&Close);
  // Read the duration
  int duration = 0;
  if (Open && Close)
  {
    double timeinseconds;
    Open(strFileName.c_str(), NULL, &timeinseconds);
    Close();
    duration = (int)(timeinseconds + 0.5);
  }
  CSectionLoader::UnloadDLL(MPC_DLL);
  return duration;
}