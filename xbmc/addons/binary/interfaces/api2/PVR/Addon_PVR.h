#pragma once
/*
 *      Copyright (C) 2012-2016 Team KODI
 *      http://kodi.tv
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
 *  along with KODI; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

namespace PVR { class CPVRClient; }
namespace ADDON { class CAddon; }

struct PVR_MENUHOOK;
struct EPG_TAG;
struct PVR_CHANNEL;
struct PVR_CHANNEL_GROUP;
struct PVR_CHANNEL_GROUP_MEMBER;
struct PVR_TIMER;
struct PVR_RECORDING;
struct ADDON_HANDLE_STRUCT;

namespace V2
{
namespace KodiAPI
{

struct CB_AddOnLib;

namespace PVR
{
extern "C"
{

  struct EpgEventStateChange;

  /*!
   * Callbacks for a PVR add-on to Kodi.
   *
   * Also translates the addon's C structures to Kodi's C++ structures.
   */
  class CAddonInterfacesPVR
  {
  public:
    static void Init(struct CB_AddOnLib *callbacks);

    /*\___________________________________________________________________________
    \*/
    static void add_menu_hook(void* hdl, PVR_MENUHOOK* hook);
    static void recording(void* hdl, const char* strName, const char* strFileName, bool bOnOff);
    static void connection_state_change(void* hdl, const char* strConnectionString, int newState, const char *strMessage);
    static void epg_event_state_change(void* hdl, EPG_TAG* tag, unsigned int iUniqueChannelId, int newState);
    /*\___________________________________________________________________________
    \*/
    static void transfer_epg_entry(void *hdl, const ADDON_HANDLE_STRUCT* handle, const EPG_TAG* epgentry);
    static void transfer_channel_entry(void *hdl, const ADDON_HANDLE_STRUCT* handle, const PVR_CHANNEL* channel);
    static void transfer_channel_group(void *hdl, const ADDON_HANDLE_STRUCT* handle, const PVR_CHANNEL_GROUP* group);
    static void transfer_channel_group_member(void *hdl, const ADDON_HANDLE_STRUCT* handle, const PVR_CHANNEL_GROUP_MEMBER* member);
    static void transfer_timer_entry(void *hdl, const ADDON_HANDLE_STRUCT* handle, const PVR_TIMER* timer);
    static void transfer_recording_entry(void *hdl, const ADDON_HANDLE_STRUCT* handle, const PVR_RECORDING* recording);
    /*\___________________________________________________________________________
    \*/
    static void trigger_channel_update(void* hdl);
    static void trigger_channel_groups_update(void* hdl);
    static void trigger_timer_update(void* hdl);
    static void trigger_recording_update(void* hdl);
    static void trigger_epg_update(void* hdl, unsigned int iChannelUid);

  private:
    static ::PVR::CPVRClient* GetPVRClient(void* hdl);
    static void UpdateEpgEvent(const EpgEventStateChange &ch, bool bQueued);
  };

} /* extern "C" */
} /* namespace PVR */

} /* namespace KodiAPI */
} /* namespace V2 */
