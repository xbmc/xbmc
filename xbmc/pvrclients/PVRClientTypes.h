#pragma once
/*
 *      Copyright (C) 2005-2009 Team XBMC
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

/*
  Common data structures shared between XBMC and XBMC's PVR Clients
 */

#ifndef __PVRCLIENT_TYPES_H__
#define __PVRCLIENT_TYPES_H__

#include <vector>
#include "utils/TVEPGInfoTag.h"
#include "utils/TVChannelInfoTag.h"
#include "utils/TVRecordInfoTag.h"
#include "utils/TVTimerInfoTag.h"

extern "C"
{

  /**
  * XBMC logging levels
  */
  enum PVR_LOG {
    LOG_DEBUG,
    LOG_INFO,
    LOG_ERROR
  };

  /**
  * PVR Client Error Codes
  */
  typedef enum {
    PVR_ERROR_NO_ERROR             = 0,
    PVR_ERROR_UNKOWN               = -1,
    PVR_ERROR_NOT_IMPLEMENTED      = -2,
    PVR_ERROR_SERVER_WRONG_VERSION = -3,
    PVR_ERROR_SERVER_ERROR         = -4,
    PVR_ERROR_SERVER_TIMEOUT       = -5,
    PVR_ERROR_NOT_SYNC             = -7,
    PVR_ERROR_NOT_DELETED          = -8,
    PVR_ERROR_NOT_SAVED            = -9,
    PVR_ERROR_RECORDING_RUNNING    = -10,
    PVR_ERROR_ALREADY_PRESENT      = -11,
  } PVR_ERROR;

  /**
  * PVR Client Event Codes
  * Sent via PVRManager callback
  */
  typedef enum {
    PVR_EVENT_UNKNOWN              = 0,
    PVR_EVENT_CLOSE                = 1,
    PVR_EVENT_RECORDINGS_CHANGE    = 2,
    PVR_EVENT_CHANNELS_CHANGE      = 3,
    PVR_EVENT_TIMERS_CHANGE        = 4
  } PVR_EVENT;

  /**
  * PVR Client Properties
  * Returned on client initialization
  */
  typedef struct PVR_SERVERPROPS {
    const char* Name;
    const char* Hostname;
    int   Port;
    const char* DefaultUser;
    const char* DefaultPassword;
    bool SupportChannelLogo;
    bool SupportChannelSettings;
    bool SupportTimeShift;
    bool SupportEPG;
    bool SupportRadio;
    bool SupportRecordings;
    bool SupportTimers;
    bool SupportTeletext;
    bool SupportDirector;
    bool SupportBouquets;
  } PVR_SERVERPROPS;

  /**
  * XBMC callbacks
  */
  typedef void (*PVREventCallback)(void *userData, const PVR_EVENT, const char*);
  typedef void (*PVRLogCallback)(void *userData, const PVR_LOG loglevel, const char *format, ... );
  typedef void (*PVRCharConv)(CStdStringA &sourceDest);
  typedef const char* (*PVRLocStrings)(DWORD dwCode);

  typedef struct PVRCallbacks
  {
    PVREventCallback Event;
    PVRLogCallback   Log;
    PVRCharConv      CharConv;
    PVRLocStrings    LocStrings;
    void            *userData;
  } PVRCallbacks;

  // Structure to transfer the above functions to XBMC
  struct PVRClient
  {
    PVR_ERROR (__cdecl* Create)(PVRCallbacks *callbacks);
    PVR_ERROR (__cdecl* GetProperties)(PVR_SERVERPROPS *props);
    PVR_ERROR (__cdecl* SetProperties)(PVR_SERVERPROPS *props);
    PVR_ERROR (__cdecl* SetUserSetting)(const char *settingName, const void *settingValue);
    PVR_ERROR (__cdecl* Connect)();
    void (__cdecl* Disconnect)();
    bool (__cdecl* IsUp)();
    const char* (__cdecl* GetBackendName)();
    const char* (__cdecl* GetBackendVersion)();
    PVR_ERROR (__cdecl* GetDriveSpace)(long long *total, long long *used);
    PVR_ERROR (__cdecl* GetEPGForChannel)(unsigned int number, EPG_DATA &epg, time_t start, time_t end);
    PVR_ERROR (__cdecl* GetEPGNowInfo)(unsigned int number, CTVEPGInfoTag *result);
    PVR_ERROR (__cdecl* GetEPGNextInfo)(unsigned int number, CTVEPGInfoTag *result);
    int (__cdecl* GetNumChannels)();
    PVR_ERROR (__cdecl* GetChannelList)(VECCHANNELS *channels, bool radio);
    PVR_ERROR (__cdecl* GetChannelSettings)(CTVChannelInfoTag *result);
    PVR_ERROR (__cdecl* UpdateChannelSettings)(const CTVChannelInfoTag &chaninfo);
    PVR_ERROR (__cdecl* AddChannel)(const CTVChannelInfoTag &info);
    PVR_ERROR (__cdecl* DeleteChannel)(unsigned int number);
    PVR_ERROR (__cdecl* RenameChannel)(unsigned int number, CStdString &newname);
    PVR_ERROR (__cdecl* MoveChannel)(unsigned int number, unsigned int newnumber);
    int (__cdecl* GetNumRecordings)(void);
    PVR_ERROR (__cdecl* GetAllRecordings)(VECRECORDINGS *results);
    PVR_ERROR (__cdecl* DeleteRecording)(const CTVRecordingInfoTag &recinfo);
    PVR_ERROR (__cdecl* RenameRecording)(const CTVRecordingInfoTag &recinfo, CStdString &newname);
    int (__cdecl* GetNumTimers)(void);
    PVR_ERROR (__cdecl* GetAllTimers)(VECTVTIMERS *results);
    PVR_ERROR (__cdecl* AddTimer)(const CTVTimerInfoTag &timerinfo);
    PVR_ERROR (__cdecl* DeleteTimer)(const CTVTimerInfoTag &timerinfo, bool force);
    PVR_ERROR (__cdecl* RenameTimer)(const CTVTimerInfoTag &timerinfo, CStdString &newname);
    PVR_ERROR (__cdecl* UpdateTimer)(const CTVTimerInfoTag &timerinfo);
    bool (__cdecl* OpenLiveStream)(unsigned int channel);
    void (__cdecl* CloseLiveStream)();
    int (__cdecl* ReadLiveStream)(BYTE* buf, int buf_size);
    int (__cdecl* GetCurrentClientChannel)();
    bool (__cdecl* SwitchChannel)(unsigned int channel);
    bool (__cdecl* OpenRecordedStream)(const CTVRecordingInfoTag &recinfo);
    void (__cdecl* CloseRecordedStream)(void);
    int (__cdecl* ReadRecordedStream)(BYTE* buf, int buf_size);
    __int64 (__cdecl* SeekRecordedStream)(__int64 pos, int whence);
    __int64 (__cdecl* LengthRecordedStream)(void);
  };



}

#endif //__PVRCLIENT_TYPES_H__
