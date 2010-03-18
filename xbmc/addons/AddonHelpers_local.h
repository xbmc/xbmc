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

#include "cores/dvdplayer/DVDDemuxers/DVDDemuxUtils.h"
#include "addons/include/xbmc_pvr_types.h"
#include "../../addons/org.xbmc.addon.library/libXBMC_addon.h"

typedef void (*AddOnLogCallback)(void *addonData, const addon_log_t loglevel, const char *msg);
typedef void (*AddOnQueueNotification)(void *addonData, const queue_msg_t type, const char *msg);
typedef bool (*AddOnGetSetting)(void *addonData, const char *settingName, void *settingValue);
typedef char* (*AddOnUnknownToUTF8)(const char *sourceDest);

typedef struct CB_AddOn
{
  AddOnLogCallback       Log;
  AddOnQueueNotification QueueNotification;
  AddOnGetSetting        GetSetting;
  AddOnUnknownToUTF8     UnknownToUTF8;
} CB_AddOnLib;


typedef void (*PVRTransferEpgEntry)(void *userData, const PVRHANDLE handle, const PVR_PROGINFO *epgentry);
typedef void (*PVRTransferChannelEntry)(void *userData, const PVRHANDLE handle, const PVR_CHANNEL *chan);
typedef void (*PVRTransferTimerEntry)(void *userData, const PVRHANDLE handle, const PVR_TIMERINFO *timer);
typedef void (*PVRTransferRecordingEntry)(void *userData, const PVRHANDLE handle, const PVR_RECORDINGINFO *recording);
typedef void (*PVRAddMenuHook)(void *addonData, PVR_MENUHOOK *hook);
typedef void (*PVRFreeDemuxPacket)(void *addonData, DemuxPacket* pPacket);
typedef DemuxPacket* (*PVRAllocateDemuxPacket)(void *addonData, int iDataSize);

typedef struct CB_PVRLib
{
  PVRTransferEpgEntry       TransferEpgEntry;
  PVRTransferChannelEntry   TransferChannelEntry;
  PVRTransferTimerEntry     TransferTimerEntry;
  PVRTransferRecordingEntry TransferRecordingEntry;
  PVRAddMenuHook            AddMenuHook;
  PVRFreeDemuxPacket        FreeDemuxPacket;
  PVRAllocateDemuxPacket    AllocateDemuxPacket;

} CB_PVRLib;


typedef CB_AddOnLib* (*XBMCAddOnLib_RegisterMe)(void *addonData);
typedef void (*XBMCAddOnLib_UnRegisterMe)(void *addonData, CB_AddOnLib *cbTable);
typedef CB_PVRLib* (*XBMCPVRLib_RegisterMe)(void *addonData);
typedef void (*XBMCPVRLib_UnRegisterMe)(void *addonData, CB_PVRLib *cbTable);

typedef struct AddonCB
{
  const char                *libBasePath;                  ///> Never, never change this!!!
  void                      *addonData;
  XBMCAddOnLib_RegisterMe    AddOnLib_RegisterMe;
  XBMCAddOnLib_UnRegisterMe  AddOnLib_UnRegisterMe;
  XBMCPVRLib_RegisterMe      PVRLib_RegisterMe;
  XBMCPVRLib_UnRegisterMe    PVRLib_UnRegisterMe;
} AddonCB;


namespace ADDON
{

class CAddon;
class CAddonHelpers_Addon;
class CAddonHelpers_PVR;

class CAddonHelpers
{
public:
  CAddonHelpers(CAddon* addon);
  ~CAddonHelpers();
  AddonCB *GetCallbacks() { return m_callbacks; }

  static CB_AddOnLib* AddOnLib_RegisterMe(void *addonData);
  static void AddOnLib_UnRegisterMe(void *addonData, CB_AddOnLib *cbTable);
  static CB_PVRLib* PVRLib_RegisterMe(void *addonData);
  static void PVRLib_UnRegisterMe(void *addonData, CB_PVRLib *cbTable);

  CAddonHelpers_Addon *GetHelperAddon() { return m_helperAddon; }
  CAddonHelpers_PVR *GetHelperPVR() { return m_helperPVR; }

private:
  AddonCB             *m_callbacks;
  CAddon              *m_addon;
  CAddonHelpers_Addon *m_helperAddon;
  CAddonHelpers_PVR   *m_helperPVR;
};

}; /* namespace ADDON */
