/*
 *      Copyright (C) 2010 Alwin Esch (Team XBMC)
 *      Copyright (C) 2011 Alexander Pipelka
 *      http://xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "client.h"
#include "xbmc_pvr_dll.h"
#include "XVDRDemux.h"
#include "XVDRRecording.h"
#include "XVDRData.h"
#include "XVDRChannelScan.h"
#include "XVDRSettings.h"

#include <string.h>
#include <sstream>
#include <string>
#include <iostream>
#include <stdint.h>

using namespace std;
using namespace ADDON;

ADDON_STATUS m_CurStatus      = ADDON_STATUS_UNKNOWN;

CHelper_libXBMC_addon *XBMC   = NULL;
CHelper_libXBMC_gui   *GUI    = NULL;
CHelper_libXBMC_pvr   *PVR    = NULL;

cXVDRDemux      *XVDRDemuxer       = NULL;
cXVDRData       *XVDRData          = NULL;
cXVDRRecording  *XVDRRecording     = NULL;
cMutex          XVDRMutex;
cMutex          XVDRMutexDemux;
cMutex          XVDRMutexRec;

static int priotable[] = { 0,5,10,15,20,25,30,35,40,45,50,55,60,65,70,75,80,85,90,95,99,100 };

extern "C" {

/***********************************************************
 * Standard AddOn related public library functions
 ***********************************************************/

ADDON_STATUS ADDON_Create(void* hdl, void* props)
{
  if (!hdl || !props)
    return ADDON_STATUS_UNKNOWN;

  XBMC = new CHelper_libXBMC_addon;
  if (!XBMC->RegisterMe(hdl))
  {
    delete XBMC;
    XBMC = NULL;
    return ADDON_STATUS_UNKNOWN;
  }

  GUI = new CHelper_libXBMC_gui;
  if (!GUI->RegisterMe(hdl))
    return ADDON_STATUS_UNKNOWN;

  PVR = new CHelper_libXBMC_pvr;
  if (!PVR->RegisterMe(hdl))
  {
    delete PVR;
    delete XBMC;
    PVR = NULL;
    XBMC = NULL;
    return ADDON_STATUS_UNKNOWN;
  }

  XBMC->Log(LOG_DEBUG, "Creating VDR XVDR PVR-Client");

  m_CurStatus = ADDON_STATUS_UNKNOWN;

  cXVDRSettings& s = cXVDRSettings::GetInstance();
  s.load();

  XVDRData = new cXVDRData;
  XVDRData->SetTimeout(s.ConnectTimeout() * 1000);
  XVDRData->SetCompressionLevel(s.Compression() * 3);
  XVDRData->SetAudioType(s.AudioType());

  cTimeMs RetryTimeout;
  bool bConnected = false;

  while (!(bConnected = XVDRData->Open(s.Hostname())) && RetryTimeout.Elapsed() < (uint32_t)s.ConnectTimeout() * 1000)
    cXVDRSession::SleepMs(100);

  if (!bConnected){
    delete XVDRData;
    delete PVR;
    delete XBMC;
    XVDRData = NULL;
    PVR = NULL;
    XBMC = NULL;
    m_CurStatus = ADDON_STATUS_LOST_CONNECTION;
    return m_CurStatus;
  }


  if (!XVDRData->Login())
  {
    m_CurStatus = ADDON_STATUS_LOST_CONNECTION;
    return m_CurStatus;
  }

  if (!XVDRData->EnableStatusInterface(s.HandleMessages()))
  {
    m_CurStatus = ADDON_STATUS_LOST_CONNECTION;
    return m_CurStatus;
  }

  XVDRData->ChannelFilter(s.FTAChannels(), s.NativeLangOnly(), s.vcaids);
  XVDRData->SetUpdateChannels(s.UpdateChannels());

  m_CurStatus = ADDON_STATUS_OK;
  return m_CurStatus;
}

ADDON_STATUS ADDON_GetStatus()
{
  return m_CurStatus;
}

void ADDON_Destroy()
{
  delete XVDRData;
  delete PVR;
  delete XBMC;

  XVDRData = NULL;
  PVR = NULL;
  XBMC = NULL;
  m_CurStatus = ADDON_STATUS_UNKNOWN;
}

bool ADDON_HasSettings()
{
  return true;
}

unsigned int ADDON_GetSettings(ADDON_StructSetting ***sSet)
{
  return 0;
}

ADDON_STATUS ADDON_SetSetting(const char *settingName, const void *settingValue)
{
  bool bChanged = false;
  cXVDRSettings& s = cXVDRSettings::GetInstance();
  bChanged = s.set(settingName, settingValue);

  if(!bChanged)
    return ADDON_STATUS_OK;

  if(strcmp(settingName, "host") == 0)
    return ADDON_STATUS_NEED_RESTART;

  s.checkValues();

  XVDRData->SetTimeout(s.ConnectTimeout() * 1000);
  XVDRData->SetCompressionLevel(s.Compression() * 3);
  XVDRData->SetAudioType(s.AudioType());

  XVDRData->EnableStatusInterface(s.HandleMessages());
  XVDRData->SetUpdateChannels(s.UpdateChannels());
  XVDRData->ChannelFilter(s.FTAChannels(), s.NativeLangOnly(), s.vcaids);

  return ADDON_STATUS_OK;
}

void ADDON_Stop()
{
}

void ADDON_FreeSettings()
{
}

/***********************************************************
 * PVR Client AddOn specific public library functions
 ***********************************************************/

PVR_ERROR GetAddonCapabilities(PVR_ADDON_CAPABILITIES* pCapabilities)
{
  cMutexLock lock(&XVDRMutex);

  pCapabilities->bSupportsTimeshift          = false;
  pCapabilities->bSupportsEPG                = true;
  pCapabilities->bSupportsRecordings         = true;
  pCapabilities->bSupportsTimers             = true;
  pCapabilities->bSupportsTV                 = true;
  pCapabilities->bSupportsRadio              = true;
  pCapabilities->bSupportsChannelSettings    = false;
  pCapabilities->bSupportsChannelGroups      = true;
  pCapabilities->bHandlesInputStream         = true;
  pCapabilities->bHandlesDemuxing            = true;
  pCapabilities->bSupportsChannelScan        = (XVDRData && XVDRData->SupportChannelScan());

  // <tricky_mode>
  // we don't know if XBMC has the new ABI, so
  // check if bSupportsRecordingFolders is 0
  // there is no guarantee that this doesn't cause any
  // segfault or memory corruption
  if (pCapabilities->bSupportsRecordingFolders == 0)
    pCapabilities->bSupportsRecordingFolders = true;
  // </tricky_mode>

  return PVR_ERROR_NO_ERROR;
}

const char * GetBackendName(void)
{
  cMutexLock lock(&XVDRMutex);

  static std::string BackendName = XVDRData ? XVDRData->GetServerName() : "unknown";
  return BackendName.c_str();
}

const char * GetBackendVersion(void)
{
  cMutexLock lock(&XVDRMutex);

  static std::string BackendVersion;
  if (XVDRData) {
    std::stringstream format;
    format << XVDRData->GetVersion() << "(Protocol: " << XVDRData->GetProtocol() << ")";
    BackendVersion = format.str();
  }
  return BackendVersion.c_str();
}

const char * GetConnectionString(void)
{
  cMutexLock lock(&XVDRMutex);

  static std::string ConnectionString;
  std::stringstream format;

  if (XVDRData) {
    format << cXVDRSettings::GetInstance().Hostname();
  }
  else {
    format << cXVDRSettings::GetInstance().Hostname() << " (addon error!)";
  }
  ConnectionString = format.str();
  return ConnectionString.c_str();
}

PVR_ERROR GetDriveSpace(long long *iTotal, long long *iUsed)
{
  cMutexLock lock(&XVDRMutex);

  if (!XVDRData)
    return PVR_ERROR_SERVER_ERROR;

  return (XVDRData->GetDriveSpace(iTotal, iUsed) ? PVR_ERROR_NO_ERROR : PVR_ERROR_SERVER_ERROR);
}

PVR_ERROR DialogChannelScan(void)
{
  cMutexLock lock(&XVDRMutex);

  cXVDRChannelScan scanner;
  scanner.Open(cXVDRSettings::GetInstance().Hostname());
  return PVR_ERROR_NO_ERROR;
}

/*******************************************/
/** PVR EPG Functions                     **/

PVR_ERROR GetEPGForChannel(PVR_HANDLE handle, const PVR_CHANNEL &channel, time_t iStart, time_t iEnd)
{
  cMutexLock lock(&XVDRMutex);

  if (!XVDRData)
    return PVR_ERROR_SERVER_ERROR;

  return (XVDRData->GetEPGForChannel(handle, channel, iStart, iEnd) ? PVR_ERROR_NO_ERROR: PVR_ERROR_SERVER_ERROR);
}


/*******************************************/
/** PVR Channel Functions                 **/

int GetChannelsAmount(void)
{
  cMutexLock lock(&XVDRMutex);

  if (!XVDRData)
    return 0;

  return XVDRData->GetChannelsCount();
}

PVR_ERROR GetChannels(PVR_HANDLE handle, bool bRadio)
{
  cMutexLock lock(&XVDRMutex);

  if (!XVDRData)
    return PVR_ERROR_SERVER_ERROR;

  return (XVDRData->GetChannelsList(handle, bRadio) ? PVR_ERROR_NO_ERROR : PVR_ERROR_SERVER_ERROR);
}


/*******************************************/
/** PVR Channelgroups Functions           **/

int GetChannelGroupsAmount()
{
  cMutexLock lock(&XVDRMutex);

  if (!XVDRData)
    return PVR_ERROR_SERVER_ERROR;

  return XVDRData->GetChannelGroupCount(cXVDRSettings::GetInstance().AutoChannelGroups());
}

PVR_ERROR GetChannelGroups(PVR_HANDLE handle, bool bRadio)
{
  cMutexLock lock(&XVDRMutex);

  if (!XVDRData)
    return PVR_ERROR_SERVER_ERROR;

  if(XVDRData->GetChannelGroupCount(cXVDRSettings::GetInstance().AutoChannelGroups()) > 0)
    return XVDRData->GetChannelGroupList(handle, bRadio) ? PVR_ERROR_NO_ERROR : PVR_ERROR_SERVER_ERROR;

  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR GetChannelGroupMembers(PVR_HANDLE handle, const PVR_CHANNEL_GROUP &group)
{
  cMutexLock lock(&XVDRMutex);

  if (!XVDRData)
    return PVR_ERROR_SERVER_ERROR;

  XVDRData->GetChannelGroupMembers(handle, group);
  return PVR_ERROR_NO_ERROR;
}


/*******************************************/
/** PVR Timer Functions                   **/

int GetTimersAmount(void)
{
  cMutexLock lock(&XVDRMutex);

  if (!XVDRData)
    return 0;

  return XVDRData->GetTimersCount();
}

PVR_ERROR GetTimers(PVR_HANDLE handle)
{
  cMutexLock lock(&XVDRMutex);

  if (!XVDRData)
    return PVR_ERROR_SERVER_ERROR;

  return (XVDRData->GetTimersList(handle) ? PVR_ERROR_NO_ERROR : PVR_ERROR_SERVER_ERROR);
}

PVR_ERROR AddTimer(const PVR_TIMER &timer)
{
  cMutexLock lock(&XVDRMutex);

  if (!XVDRData)
    return PVR_ERROR_SERVER_ERROR;

  return XVDRData->AddTimer(timer);
}

PVR_ERROR DeleteTimer(const PVR_TIMER &timer, bool bForce)
{
  cMutexLock lock(&XVDRMutex);

  if (!XVDRData)
    return PVR_ERROR_SERVER_ERROR;

  return XVDRData->DeleteTimer(timer, bForce);
}

PVR_ERROR UpdateTimer(const PVR_TIMER &timer)
{
  cMutexLock lock(&XVDRMutex);

  if (!XVDRData)
    return PVR_ERROR_SERVER_ERROR;

  return XVDRData->UpdateTimer(timer);
}


/*******************************************/
/** PVR Recording Functions               **/

int GetRecordingsAmount(void)
{
  cMutexLock lock(&XVDRMutex);

  if (!XVDRData)
    return 0;

  return XVDRData->GetRecordingsCount();
}

PVR_ERROR GetRecordings(PVR_HANDLE handle)
{
  cMutexLock lock(&XVDRMutex);

  if (!XVDRData)
    return PVR_ERROR_SERVER_ERROR;

  return XVDRData->GetRecordingsList(handle);
}

PVR_ERROR RenameRecording(const PVR_RECORDING &recording)
{
  cMutexLock lock(&XVDRMutex);

  if (!XVDRData)
    return PVR_ERROR_SERVER_ERROR;

  return XVDRData->RenameRecording(recording, recording.strTitle);
}

PVR_ERROR DeleteRecording(const PVR_RECORDING &recording)
{
  cMutexLock lock(&XVDRMutex);

  if (!XVDRData)
    return PVR_ERROR_SERVER_ERROR;

  return XVDRData->DeleteRecording(recording);
}

/*******************************************/
/** PVR Live Stream Functions             **/

bool OpenLiveStream(const PVR_CHANNEL &channel)
{
  cMutexLock lock(&XVDRMutexDemux);

  if (XVDRDemuxer)
  {
    XVDRDemuxer->Close();
    delete XVDRDemuxer;
  }

  XVDRDemuxer = new cXVDRDemux;
  XVDRDemuxer->SetTimeout(cXVDRSettings::GetInstance().ConnectTimeout() * 1000);
  XVDRDemuxer->SetAudioType(cXVDRSettings::GetInstance().AudioType());
  XVDRDemuxer->SetPriority(priotable[cXVDRSettings::GetInstance().Priority()]);

  return XVDRDemuxer->OpenChannel(cXVDRSettings::GetInstance().Hostname(), channel);
}

void CloseLiveStream(void)
{
  cMutexLock lock(&XVDRMutexDemux);

  if (XVDRDemuxer)
  {
    XVDRDemuxer->Close();
    delete XVDRDemuxer;
    XVDRDemuxer = NULL;
  }
}

PVR_ERROR GetStreamProperties(PVR_STREAM_PROPERTIES* pProperties)
{
  cMutexLock lock(&XVDRMutexDemux);

  if (!XVDRDemuxer)
    return PVR_ERROR_SERVER_ERROR;

  return (XVDRDemuxer->GetStreamProperties(pProperties) ? PVR_ERROR_NO_ERROR : PVR_ERROR_SERVER_ERROR);
}

void DemuxAbort(void)
{
  cMutexLock lock(&XVDRMutexDemux);
  XBMC->Log(LOG_DEBUG, "DemuxAbort");
}

void DemuxReset(void)
{
  cMutexLock lock(&XVDRMutexDemux);
  XBMC->Log(LOG_DEBUG, "DemuxReset");
}

void DemuxFlush(void)
{
  cMutexLock lock(&XVDRMutexDemux);
  XBMC->Log(LOG_DEBUG, "DemuxFlush");
}

DemuxPacket* DemuxRead(void)
{
  cMutexLock lock(&XVDRMutexDemux);

  if (!XVDRDemuxer)
    return NULL;

  return XVDRDemuxer->Read();
}

int GetCurrentClientChannel(void)
{
  cMutexLock lock(&XVDRMutexDemux);

  if (XVDRDemuxer)
    return XVDRDemuxer->CurrentChannel();

  return -1;
}

bool SwitchChannel(const PVR_CHANNEL &channel)
{
  cMutexLock lock(&XVDRMutexDemux);

  if (XVDRDemuxer == NULL)
    return false;

  XVDRDemuxer->SetTimeout(cXVDRSettings::GetInstance().ConnectTimeout() * 1000);
  XVDRDemuxer->SetAudioType(cXVDRSettings::GetInstance().AudioType());
  XVDRDemuxer->SetPriority(priotable[cXVDRSettings::GetInstance().Priority()]);

  return XVDRDemuxer->SwitchChannel(channel);
}

PVR_ERROR SignalStatus(PVR_SIGNAL_STATUS &signalStatus)
{
  cMutexLock lock(&XVDRMutexDemux);

  if (!XVDRDemuxer)
    return PVR_ERROR_SERVER_ERROR;

  return (XVDRDemuxer->GetSignalStatus(signalStatus) ? PVR_ERROR_NO_ERROR : PVR_ERROR_SERVER_ERROR);
}


/*******************************************/
/** PVR Recording Stream Functions        **/

bool OpenRecordedStream(const PVR_RECORDING &recording)
{
  cMutexLock lock(&XVDRMutexRec);

  if(!XVDRData)
    return false;

  CloseRecordedStream();

  XVDRRecording = new cXVDRRecording;
  XVDRRecording->SetTimeout(cXVDRSettings::GetInstance().ConnectTimeout() * 1000);

  return XVDRRecording->OpenRecording(cXVDRSettings::GetInstance().Hostname(), recording);
}

void CloseRecordedStream(void)
{
  cMutexLock lock(&XVDRMutexRec);

  if (XVDRRecording)
  {
    XVDRRecording->Close();
    delete XVDRRecording;
    XVDRRecording = NULL;
  }
}

int ReadRecordedStream(unsigned char *pBuffer, unsigned int iBufferSize)
{
  cMutexLock lock(&XVDRMutexRec);

  if (!XVDRRecording)
    return -1;

  return XVDRRecording->Read(pBuffer, iBufferSize);
}

long long SeekRecordedStream(long long iPosition, int iWhence /* = SEEK_SET */)
{
  cMutexLock lock(&XVDRMutexRec);

  if (XVDRRecording)
    return XVDRRecording->Seek(iPosition, iWhence);

  return -1;
}

long long PositionRecordedStream(void)
{
  cMutexLock lock(&XVDRMutexRec);

  if (XVDRRecording)
    return XVDRRecording->Position();

  return 0;
}

long long LengthRecordedStream(void)
{
  cMutexLock lock(&XVDRMutexRec);

  if (XVDRRecording)
    return XVDRRecording->Length();

  return 0;
}

/** UNUSED API FUNCTIONS */
PVR_ERROR CallMenuHook(const PVR_MENUHOOK &menuhook) { return PVR_ERROR_NOT_IMPLEMENTED; }
PVR_ERROR DeleteChannel(const PVR_CHANNEL &channel) { return PVR_ERROR_NOT_IMPLEMENTED; }
PVR_ERROR RenameChannel(const PVR_CHANNEL &channel) { return PVR_ERROR_NOT_IMPLEMENTED; }
PVR_ERROR MoveChannel(const PVR_CHANNEL &channel) { return PVR_ERROR_NOT_IMPLEMENTED; }
PVR_ERROR DialogChannelSettings(const PVR_CHANNEL &channel) { return PVR_ERROR_NOT_IMPLEMENTED; }
PVR_ERROR DialogAddChannel(const PVR_CHANNEL &channel) { return PVR_ERROR_NOT_IMPLEMENTED; }
int ReadLiveStream(unsigned char *pBuffer, unsigned int iBufferSize) { return 0; }
long long SeekLiveStream(long long iPosition, int iWhence /* = SEEK_SET */) { return -1; }
long long PositionLiveStream(void) { return -1; }
long long LengthLiveStream(void) { return -1; }
const char * GetLiveStreamURL(const PVR_CHANNEL &channel) { return ""; }
PVR_ERROR SetRecordingPlayCount(const PVR_RECORDING &recording, int count) { return PVR_ERROR_NOT_IMPLEMENTED; }
PVR_ERROR SetRecordingLastPlayedPosition(const PVR_RECORDING &recording, int lastplayedposition) { return PVR_ERROR_NOT_IMPLEMENTED; }
int GetRecordingLastPlayedPosition(const PVR_RECORDING &recording) { return -1; }
}
