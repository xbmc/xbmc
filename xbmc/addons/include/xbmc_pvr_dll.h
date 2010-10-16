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

  /** PVR General Functions **/
  PVR_ERROR GetProperties(PVR_SERVERPROPS* pProps);
  PVR_ERROR GetStreamProperties(PVR_STREAMPROPS* pProps);
  const char* GetBackendName();
  const char* GetBackendVersion();
  const char* GetConnectionString();
  PVR_ERROR GetDriveSpace(long long *total, long long *used);
  PVR_ERROR GetBackendTime(time_t *localTime, int *gmtOffset);
  PVR_ERROR DialogChannelScan();
  PVR_ERROR MenuHook(const PVR_MENUHOOK &menuhook);

  /** PVR EPG Functions **/
  PVR_ERROR RequestEPGForChannel(PVRHANDLE handle, const PVR_CHANNEL &channel, time_t start, time_t end);

  /** PVR Bouquets Functions **/
  int GetNumBouquets();
  PVR_ERROR RequestBouquetsList(PVRHANDLE handle, int radio);

  /** PVR Channel Functions **/
  int GetNumChannels();
  PVR_ERROR RequestChannelList(PVRHANDLE handle, int radio);
  PVR_ERROR DeleteChannel(unsigned int number);
  PVR_ERROR RenameChannel(unsigned int number, const char *newname);
  PVR_ERROR MoveChannel(unsigned int number, unsigned int newnumber);
  PVR_ERROR DialogChannelSettings(const PVR_CHANNEL &channelinfo);
  PVR_ERROR DialogAddChannel(const PVR_CHANNEL &channelinfo);

  /** PVR Recording Functions **/
  int GetNumRecordings();
  PVR_ERROR RequestRecordingsList(PVRHANDLE handle);
  PVR_ERROR DeleteRecording(const PVR_RECORDINGINFO &recinfo);
  PVR_ERROR RenameRecording(const PVR_RECORDINGINFO &recinfo, const char *newname);

  /** PVR Recording cut marks Functions **/
  bool HaveCutmarks();
  PVR_ERROR RequestCutMarksList(PVRHANDLE handle);
  PVR_ERROR AddCutMark(const PVR_CUT_MARK &cutmark);
  PVR_ERROR DeleteCutMark(const PVR_CUT_MARK &cutmark);
  PVR_ERROR StartCut();

  /** PVR Timer Functions **/
  int GetNumTimers();
  PVR_ERROR RequestTimerList(PVRHANDLE handle);
  PVR_ERROR AddTimer(const PVR_TIMERINFO &timerinfo);
  PVR_ERROR DeleteTimer(const PVR_TIMERINFO &timerinfo, bool force);
  PVR_ERROR RenameTimer(const PVR_TIMERINFO &timerinfo, const char *newname);
  PVR_ERROR UpdateTimer(const PVR_TIMERINFO &timerinfo);

  /** PVR Live Stream Functions **/
  bool OpenLiveStream(const PVR_CHANNEL &channelinfo);
  void CloseLiveStream();
  int ReadLiveStream(unsigned char* buf, int buf_size);
  long long SeekLiveStream(long long pos, int whence=SEEK_SET);
  long long PositionLiveStream(void);
  long long LengthLiveStream(void);
  int GetCurrentClientChannel();
  bool SwitchChannel(const PVR_CHANNEL &channelinfo);
  PVR_ERROR SignalQuality(PVR_SIGNALQUALITY &qualityinfo);

  /** PVR Secondary Stream Functions **/
  bool SwapLiveTVSecondaryStream();
  bool OpenSecondaryStream(const PVR_CHANNEL &channelinfo);
  void CloseSecondaryStream();
  int ReadSecondaryStream(unsigned char* buf, int buf_size);

  /** PVR Recording Stream Functions **/
  bool OpenRecordedStream(const PVR_RECORDINGINFO &recinfo);
  void CloseRecordedStream(void);
  int ReadRecordedStream(unsigned char* buf, int buf_size);
  long long SeekRecordedStream(long long pos, int whence=SEEK_SET);
  long long PositionRecordedStream(void);
  long long LengthRecordedStream(void);
  const char* GetLiveStreamURL(const PVR_CHANNEL &channelinfo);

  /** \name Demuxer Interface */
  void DemuxReset();
  void DemuxAbort();
  void DemuxFlush();
  DemuxPacket* DemuxRead();

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
    pClient->DialogChannelScan      = DialogChannelScan;
    pClient->MenuHook               = MenuHook;
    pClient->GetNumBouquets         = GetNumBouquets;
    pClient->RequestBouquetsList    = RequestBouquetsList;
    pClient->GetNumChannels         = GetNumChannels;
    pClient->GetNumRecordings       = GetNumRecordings;
    pClient->GetNumTimers           = GetNumTimers;
    pClient->RequestEPGForChannel   = RequestEPGForChannel;
    pClient->RequestChannelList     = RequestChannelList;
    pClient->DeleteChannel          = DeleteChannel;
    pClient->RenameChannel          = RenameChannel;
    pClient->MoveChannel            = MoveChannel;
    pClient->DialogChannelSettings  = DialogChannelSettings;
    pClient->DialogAddChannel       = DialogAddChannel;
    pClient->RequestRecordingsList  = RequestRecordingsList;
    pClient->DeleteRecording        = DeleteRecording;
    pClient->RenameRecording        = RenameRecording;
    pClient->HaveCutmarks           = HaveCutmarks;
    pClient->RequestCutMarksList    = RequestCutMarksList;
    pClient->AddCutMark             = AddCutMark;
    pClient->DeleteCutMark          = DeleteCutMark;
    pClient->StartCut               = StartCut;
    pClient->RequestTimerList       = RequestTimerList;
    pClient->AddTimer               = AddTimer;
    pClient->DeleteTimer            = DeleteTimer;
    pClient->RenameTimer            = RenameTimer;
    pClient->UpdateTimer            = UpdateTimer;
    pClient->OpenLiveStream         = OpenLiveStream;
    pClient->CloseLiveStream        = CloseLiveStream;
    pClient->ReadLiveStream         = ReadLiveStream;
    pClient->SeekLiveStream         = SeekLiveStream;
    pClient->PositionLiveStream     = PositionLiveStream;
    pClient->LengthLiveStream       = LengthLiveStream;
    pClient->GetCurrentClientChannel= GetCurrentClientChannel;
    pClient->SwitchChannel          = SwitchChannel;
    pClient->SignalQuality          = SignalQuality;
    pClient->SwapLiveTVSecondaryStream  = SwapLiveTVSecondaryStream;
    pClient->OpenSecondaryStream    = OpenSecondaryStream;
    pClient->CloseSecondaryStream   = CloseSecondaryStream;
    pClient->ReadSecondaryStream    = ReadSecondaryStream;
    pClient->OpenRecordedStream     = OpenRecordedStream;
    pClient->CloseRecordedStream    = CloseRecordedStream;
    pClient->ReadRecordedStream     = ReadRecordedStream;
    pClient->SeekRecordedStream     = SeekRecordedStream;
    pClient->PositionRecordedStream = PositionRecordedStream;
    pClient->LengthRecordedStream   = LengthRecordedStream;
    pClient->GetLiveStreamURL       = GetLiveStreamURL;
    pClient->DemuxReset             = DemuxReset;
    pClient->DemuxAbort             = DemuxAbort;
    pClient->DemuxFlush             = DemuxFlush;
    pClient->DemuxRead              = DemuxRead;
  };
};

#endif
