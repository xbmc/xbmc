#pragma once
/*
 *      Copyright (C) 2005-2010 Team XBMC
 *      http://www.xbmc.org
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

#ifndef __XBMC_PVR_H__
#define __XBMC_PVR_H__

#include "xbmc_addon_dll.h"               /* Dll related functions available to all AddOn's */
#include "xbmc_pvr_types.h"

extern "C"
{
  // Functions that your PVR client must implement, also you must implement the functions from
  // xbmc_addon_dll.h
  PVR_ERROR GetProperties(PVR_SERVERPROPS* pProps);
  PVR_ERROR GetStreamProperties(PVR_STREAMPROPS* pProps);
  const char* GetBackendName();
  const char* GetBackendVersion();
  const char* GetConnectionString();
  PVR_ERROR GetDriveSpace(long long *total, long long *used);
  PVR_ERROR GetBackendTime(time_t *localTime, int *gmtOffset);

  PVR_ERROR RequestEPGForChannel(PVRHANDLE handle, const PVR_CHANNEL &channel, time_t start, time_t end);
  int GetNumBouquets();
  int GetNumChannels();
  int GetNumRecordings();
  int GetNumTimers();
  PVR_ERROR RequestChannelList(PVRHANDLE handle, int radio);
//  PVR_ERROR GetChannelSettings(cPVRChannelInfoTag *result);
//  PVR_ERROR UpdateChannelSettings(const cPVRChannelInfoTag &chaninfo);
//  PVR_ERROR AddChannel(const cPVRChannelInfoTag &info);
//  PVR_ERROR DeleteChannel(unsigned int number);
//  PVR_ERROR RenameChannel(unsigned int number, CStdString &newname);
//  PVR_ERROR MoveChannel(unsigned int number, unsigned int newnumber);

  PVR_ERROR RequestRecordingsList(PVRHANDLE handle);
  PVR_ERROR DeleteRecording(const PVR_RECORDINGINFO &recinfo);
  PVR_ERROR RenameRecording(const PVR_RECORDINGINFO &recinfo, const char *newname);

  PVR_ERROR RequestTimerList(PVRHANDLE handle);
  PVR_ERROR AddTimer(const PVR_TIMERINFO &timerinfo);
  PVR_ERROR DeleteTimer(const PVR_TIMERINFO &timerinfo, bool force);
  PVR_ERROR RenameTimer(const PVR_TIMERINFO &timerinfo, const char *newname);
  PVR_ERROR UpdateTimer(const PVR_TIMERINFO &timerinfo);

  bool OpenLiveStream(const PVR_CHANNEL &channelinfo);
  void CloseLiveStream();
  int ReadLiveStream(unsigned char* buf, int buf_size);
  long long SeekLiveStream(long long pos, int whence=SEEK_SET);
  long long LengthLiveStream(void);
  int GetCurrentClientChannel();
  bool SwitchChannel(const PVR_CHANNEL &channelinfo);
  PVR_ERROR SignalQuality(PVR_SIGNALQUALITY &qualityinfo);

  bool OpenRecordedStream(const PVR_RECORDINGINFO &recinfo);
  void CloseRecordedStream(void);
  int ReadRecordedStream(unsigned char* buf, int buf_size);
  long long SeekRecordedStream(long long pos, int whence=SEEK_SET);
  long long LengthRecordedStream(void);
  const char* GetLiveStreamURL(const PVR_CHANNEL &channelinfo);

  // function to export the above structure to XBMC
  void __declspec(dllexport) get_addon(struct PVRClient* pClient)
  {
    pClient->GetProperties          = GetProperties;
    pClient->GetStreamProperties    = GetStreamProperties;
    pClient->GetConnectionString    = GetConnectionString;
    pClient->GetBackendName         = GetBackendName;
    pClient->GetBackendVersion      = GetBackendVersion;
    pClient->GetDriveSpace          = GetDriveSpace;
    pClient->GetBackendTime         = GetBackendTime;
    pClient->GetNumBouquets         = GetNumBouquets;
    pClient->GetNumChannels         = GetNumChannels;
    pClient->GetNumRecordings       = GetNumRecordings;
    pClient->GetNumTimers           = GetNumTimers;
    pClient->RequestEPGForChannel   = RequestEPGForChannel;
    pClient->RequestChannelList     = RequestChannelList;
//    pClient->GetChannelSettings     = GetChannelSettings;
//    pClient->UpdateChannelSettings  = UpdateChannelSettings;
//    pClient->AddChannel             = AddChannel;
//    pClient->DeleteChannel          = DeleteChannel;
//    pClient->RenameChannel          = RenameChannel;
//    pClient->MoveChannel            = MoveChannel;
    pClient->RequestRecordingsList  = RequestRecordingsList;
    pClient->DeleteRecording        = DeleteRecording;
    pClient->RenameRecording        = RenameRecording;
    pClient->RequestTimerList       = RequestTimerList;
    pClient->AddTimer               = AddTimer;
    pClient->DeleteTimer            = DeleteTimer;
    pClient->RenameTimer            = RenameTimer;
    pClient->UpdateTimer            = UpdateTimer;
    pClient->OpenLiveStream         = OpenLiveStream;
    pClient->CloseLiveStream        = CloseLiveStream;
    pClient->ReadLiveStream         = ReadLiveStream;
    pClient->SeekLiveStream         = SeekLiveStream;
    pClient->LengthLiveStream       = LengthLiveStream;
    pClient->GetCurrentClientChannel= GetCurrentClientChannel;
    pClient->SwitchChannel          = SwitchChannel;
    pClient->SignalQuality          = SignalQuality;
    pClient->OpenRecordedStream     = OpenRecordedStream;
    pClient->CloseRecordedStream    = CloseRecordedStream;
    pClient->ReadRecordedStream     = ReadRecordedStream;
    pClient->SeekRecordedStream     = SeekRecordedStream;
    pClient->LengthRecordedStream   = LengthRecordedStream;
    pClient->GetLiveStreamURL       = GetLiveStreamURL;
  };
};

#endif
