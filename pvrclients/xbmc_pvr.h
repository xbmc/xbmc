#ifndef __XBMC_PVR_H__
#define __XBMC_PVR_H__

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
#include "../xbmc/pvrclients/PVRClientTypes.h"
#include <sys/stat.h>
#include <errno.h>
#endif

using namespace std;

extern "C"
{
  // Functions that your client must implement
  PVR_ERROR Create(PVRCallbacks*);
  PVR_ERROR GetProperties(PVR_SERVERPROPS* pProps);
  PVR_ERROR SetUserSetting(const char *settingName, const void *settingValue);
  PVR_ERROR Connect();
  void Disconnect();
  bool IsUp();
  const char* GetBackendName();
  const char* GetBackendVersion();
  PVR_ERROR GetDriveSpace(long long *total, long long *used);
  PVR_ERROR GetEPGForChannel(unsigned int number, EPG_DATA &epg, time_t start, time_t end);
  PVR_ERROR GetEPGNowInfo(unsigned int number, CTVEPGInfoTag *result);
  PVR_ERROR GetEPGNextInfo(unsigned int number, CTVEPGInfoTag *result);
  int GetNumChannels();
  PVR_ERROR GetChannelList(VECCHANNELS *channels, bool radio);
  PVR_ERROR GetChannelSettings(CTVChannelInfoTag *result);
  PVR_ERROR UpdateChannelSettings(const CTVChannelInfoTag &chaninfo);
  PVR_ERROR AddChannel(const CTVChannelInfoTag &info);
  PVR_ERROR DeleteChannel(unsigned int number);
  PVR_ERROR RenameChannel(unsigned int number, CStdString &newname);
  PVR_ERROR MoveChannel(unsigned int number, unsigned int newnumber);
  int GetNumRecordings(void);
  PVR_ERROR GetAllRecordings(VECRECORDINGS *results);
  PVR_ERROR DeleteRecording(const CTVRecordingInfoTag &recinfo);
  PVR_ERROR RenameRecording(const CTVRecordingInfoTag &recinfo, CStdString &newname);
  int GetNumTimers(void);
  PVR_ERROR GetAllTimers(VECTVTIMERS *results);
  PVR_ERROR AddTimer(const CTVTimerInfoTag &timerinfo);
  PVR_ERROR DeleteTimer(const CTVTimerInfoTag &timerinfo, bool force);
  PVR_ERROR RenameTimer(const CTVTimerInfoTag &timerinfo, CStdString &newname);
  PVR_ERROR UpdateTimer(const CTVTimerInfoTag &timerinfo);
  bool OpenLiveStream(unsigned int channel);
  void CloseLiveStream();
  int ReadLiveStream(BYTE* buf, int buf_size);
  int GetCurrentClientChannel();
  bool SwitchChannel(unsigned int channel);
  bool OpenRecordedStream(const CTVRecordingInfoTag &recinfo);
  void CloseRecordedStream(void);
  int ReadRecordedStream(BYTE* buf, int buf_size);
  __int64 SeekRecordedStream(__int64 pos, int whence=SEEK_SET);
  __int64 LengthRecordedStream(void);

  // function to export the above structure to XBMC
  void __declspec(dllexport) get_module(struct PVRClient* pClient)
  {
    pClient->Create = Create;
    pClient->GetProperties = GetProperties;
    pClient->SetUserSetting = SetUserSetting;
    pClient->Connect = Connect;
    pClient->Disconnect = Disconnect;
    pClient->IsUp = IsUp;
    pClient->GetBackendName = GetBackendName;
    pClient->GetBackendVersion = GetBackendVersion;
    pClient->GetDriveSpace = GetDriveSpace;
    pClient->GetEPGForChannel = GetEPGForChannel;
    pClient->GetEPGNowInfo = GetEPGNowInfo;
    pClient->GetEPGNextInfo = GetEPGNextInfo;
    pClient->GetNumChannels = GetNumChannels;
    pClient->GetChannelList = GetChannelList;
    pClient->GetChannelSettings = GetChannelSettings;
    pClient->UpdateChannelSettings = UpdateChannelSettings;
    pClient->AddChannel = AddChannel;
    pClient->DeleteChannel = DeleteChannel;
    pClient->RenameChannel = RenameChannel;
    pClient->MoveChannel = MoveChannel;
    pClient->GetNumRecordings = GetNumRecordings;
    pClient->GetAllRecordings = GetAllRecordings;
    pClient->DeleteRecording = DeleteRecording;
    pClient->RenameRecording = RenameRecording;
    pClient->GetNumTimers = GetNumTimers;
    pClient->GetAllTimers = GetAllTimers;
    pClient->AddTimer = AddTimer;
    pClient->DeleteTimer = DeleteTimer;
    pClient->RenameTimer = RenameTimer;
    pClient->UpdateTimer = UpdateTimer;
    pClient->OpenLiveStream = OpenLiveStream;
    pClient->CloseLiveStream = CloseLiveStream;
    pClient->ReadLiveStream = ReadLiveStream;
    pClient->GetCurrentClientChannel = GetCurrentClientChannel;
    pClient->SwitchChannel = SwitchChannel;
    pClient->OpenRecordedStream = OpenRecordedStream;
    pClient->CloseRecordedStream = CloseRecordedStream;
    pClient->ReadRecordedStream = ReadRecordedStream;
    pClient->SeekRecordedStream = SeekRecordedStream;
    pClient->LengthRecordedStream = LengthRecordedStream;
  };
};

#endif
