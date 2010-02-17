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

#include "../include/libXBMC_addon.h"
#include "../include/libXBMC_gui.h"
#include "../include/libXBMC_pvr.h"
#include "../include/libXBMC_vis.h"
#include "../include/xbmc_pvr_types.h"

typedef void (*AddOnLogCallback)(void *addonData, const addon_log_t loglevel, const char *msg);
typedef void (*AddOnQueueNotification)(void *addonData, const queue_msg_t type, const char *msg);
typedef bool (*AddOnGetSetting)(void *addonData, const char *settingName, void *settingValue);

typedef struct CB_AddOn
{
  AddOnLogCallback       Log;
  AddOnQueueNotification QueueNotification;
  AddOnGetSetting        GetSetting;
} CB_AddOn;

typedef char* (*UtilsUnknownToUTF8)(const char *sourceDest);

typedef struct CB_Utils
{
  UtilsUnknownToUTF8        UnknownToUTF8;
} CB_Utils;

typedef void (*PVRTransferEpgEntry)(void *userData, const PVRHANDLE handle, const PVR_PROGINFO *epgentry);
typedef void (*PVRTransferChannelEntry)(void *userData, const PVRHANDLE handle, const PVR_CHANNEL *chan);
typedef void (*PVRTransferTimerEntry)(void *userData, const PVRHANDLE handle, const PVR_TIMERINFO *timer);
typedef void (*PVRTransferRecordingEntry)(void *userData, const PVRHANDLE handle, const PVR_RECORDINGINFO *recording);

typedef struct CB_PVR
{
  PVRTransferEpgEntry       TransferEpgEntry;
  PVRTransferChannelEntry   TransferChannelEntry;
  PVRTransferTimerEntry     TransferTimerEntry;
  PVRTransferRecordingEntry TransferRecordingEntry;

} CB_PVR;

typedef struct AddonCB
{
  CB_AddOn          AddOn;
  CB_PVR            PVR;
  CB_Utils          Utils;

  void             *addonData;
} AddonCB;

namespace ADDON
{

class CAddon;
class CAddonHelpers_Addon;
class CAddonHelpers_GUI;
class CAddonHelpers_PVR;
class CAddonHelpers_Vis;

class CAddonHelpers
{
public:
  CAddonHelpers(CAddon* addon);
  ~CAddonHelpers();
  AddonCB *GetCallbacks() { return m_callbacks; }
  CAddonHelpers_Addon *GetHelperAddon() { return m_helperAddon; }
  CAddonHelpers_GUI *GetHelperGUI() { return m_helperGUI; }
  CAddonHelpers_PVR *GetHelperPVR() { return m_helperPVR; }
  CAddonHelpers_Vis *GetHelperVis() { return m_helperVis; }


private:
  AddonCB             *m_callbacks;
  CAddonHelpers_Addon *m_helperAddon;
  CAddonHelpers_GUI   *m_helperGUI;
  CAddonHelpers_PVR   *m_helperPVR;
  CAddonHelpers_Vis   *m_helperVis;
};

}; /* namespace ADDON */
