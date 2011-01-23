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

#include "AddonHelpers_local.h"
#include "include/xbmc_pvr_types.h"

namespace ADDON
{

class CAddonHelpers_PVR
{
public:
  CAddonHelpers_PVR(CAddon* addon);
  ~CAddonHelpers_PVR();

  /**! \name General Functions */
  CB_PVRLib *GetCallbacks() { return m_callbacks; }

  /**! \name Callback functions */
  static void PVRTransferEpgEntry(void *addonData, const PVRHANDLE handle, const PVR_PROGINFO *epgentry);
  static void PVRTransferChannelEntry(void *addonData, const PVRHANDLE handle, const PVR_CHANNEL *channel);
  static void PVRTransferTimerEntry(void *addonData, const PVRHANDLE handle, const PVR_TIMERINFO *timer);
  static void PVRTransferRecordingEntry(void *addonData, const PVRHANDLE handle, const PVR_RECORDINGINFO *recording);
  static void PVRAddMenuHook(void *addonData, PVR_MENUHOOK *hook);
  static void PVRRecording(void *addonData, const char *Name, const char *FileName, bool On);
  static void PVRTriggerTimerUpdate(void *addonData);
  static void PVRTriggerRecordingUpdate(void *addonData);
  static void PVRFreeDemuxPacket(void *addonData, DemuxPacket* pPacket);
  static DemuxPacket* PVRAllocateDemuxPacket(void *addonData, int iDataSize = 0);

private:
  CB_PVRLib    *m_callbacks;
  CAddon       *m_addon;
};

}; /* namespace ADDON */
