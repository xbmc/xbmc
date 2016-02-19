#pragma once
/*
 *      Copyright (C) 2015 Team KODI
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

#include "addons/binary/callbacks/AddonCallbacksAddonBase.h"
#include "addons/kodi-addon-dev-kit/include/kodi/api2/definitions.hpp"
#include "tools.h"

#include "api2/addon/AddonExe_Addon_Codec.h"
#include "api2/addon/AddonExe_Addon_General.h"
#include "api2/addon/AddonExe_Addon_Network.h"
#include "api2/addon/AddonExe_Addon_SoundPlay.h"
#include "api2/addon/AddonExe_Addon_VFSUtils.h"
#include "api2/audioengine/AddonExe_AudioEngine_General.h"
#include "api2/audioengine/AddonExe_AudioEngine_Stream.h"
#include "api2/gui/AddonExe_GUI_ControlButton.h"
#include "api2/gui/AddonExe_GUI_ControlEdit.h"
#include "api2/gui/AddonExe_GUI_ControlFadeLabel.h"
#include "api2/gui/AddonExe_GUI_ControlImage.h"
#include "api2/gui/AddonExe_GUI_ControlLabel.h"
#include "api2/gui/AddonExe_GUI_ControlProgress.h"
#include "api2/gui/AddonExe_GUI_ControlRadioButton.h"
#include "api2/gui/AddonExe_GUI_ControlRendering.h"
#include "api2/gui/AddonExe_GUI_ControlSettingsSlider.h"
#include "api2/gui/AddonExe_GUI_ControlSlider.h"
#include "api2/gui/AddonExe_GUI_ControlSpin.h"
#include "api2/gui/AddonExe_GUI_ControlTextBox.h"
#include "api2/gui/AddonExe_GUI_DialogExtendedProgress.h"
#include "api2/gui/AddonExe_GUI_DialogFileBrowser.h"
#include "api2/gui/AddonExe_GUI_DialogKeyboard.h"
#include "api2/gui/AddonExe_GUI_DialogNumeric.h"
#include "api2/gui/AddonExe_GUI_DialogOK.h"
#include "api2/gui/AddonExe_GUI_DialogProgress.h"
#include "api2/gui/AddonExe_GUI_DialogSelect.h"
#include "api2/gui/AddonExe_GUI_DialogTextViewer.h"
#include "api2/gui/AddonExe_GUI_DialogYesNo.h"
#include "api2/gui/AddonExe_GUI_General.h"
#include "api2/gui/AddonExe_GUI_ListItem.h"
#include "api2/gui/AddonExe_GUI_Window.h"
#include "api2/player/AddonExe_Player_InfoTagMusic.h"
#include "api2/player/AddonExe_Player_InfoTagVideo.h"
#include "api2/player/AddonExe_Player_PlayList.h"
#include "api2/player/AddonExe_Player_Player.h"
#include "api2/pvr/AddonExe_PVR_General.h"
#include "api2/pvr/AddonExe_PVR_Transfer.h"
#include "api2/pvr/AddonExe_PVR_Trigger.h"

#include "addons/Addon.h"
#include "threads/CriticalSection.h"
#include "threads/Thread.h"

#include "requestpacket.h"
#include "responsepacket.h"
#if (defined TARGET_WINDOWS)
#  include "SharedMemoryWindows.h"
#elif (defined TARGET_POSIX)
#  include "SharedMemoryPosix.h"
#endif

#include <memory>
#include <vector>
#include <poll.h>

#include <list>


namespace ADDON
{

  #define ADDON_NET_SEND_RETURN(requestID, returnCode) \
  do { \
    uint32_t value = returnCode; \
    resp.init(requestID); \
    resp.push(API_UINT32_T, &value); \
    resp.finalise(); \
  } while(0)

  class CAddon;
  class CAddonCallbacks;

  class CBinaryAddon
   : public CThread,
     private V2::KodiAPI::CAddonExeCB_Addon_Codec,
     private V2::KodiAPI::CAddonExeCB_Addon_General,
     private V2::KodiAPI::CAddonExeCB_Addon_Network,
     private V2::KodiAPI::CAddonExeCB_Addon_SoundPlay,
     private V2::KodiAPI::CAddonExeCB_Addon_VFSUtils,
     private V2::KodiAPI::CAddonExeCB_AudioEngine_General,
     private V2::KodiAPI::CAddonExeCB_AudioEngine_Stream,
     private V2::KodiAPI::CAddonExeCB_GUI_ControlButton,
     private V2::KodiAPI::CAddonExeCB_GUI_ControlEdit,
     private V2::KodiAPI::CAddonExeCB_GUI_ControlFadeLabel,
     private V2::KodiAPI::CAddonExeCB_GUI_ControlImage,
     private V2::KodiAPI::CAddonExeCB_GUI_ControlLabel,
     private V2::KodiAPI::CAddonExeCB_GUI_ControlProgress,
     private V2::KodiAPI::CAddonExeCB_GUI_ControlRadioButton,
     private V2::KodiAPI::CAddonExeCB_GUI_ControlRendering,
     private V2::KodiAPI::CAddonExeCB_GUI_ControlSettingsSlider,
     private V2::KodiAPI::CAddonExeCB_GUI_ControlSlider,
     private V2::KodiAPI::CAddonExeCB_GUI_ControlSpin,
     private V2::KodiAPI::CAddonExeCB_GUI_ControlTextBox,
     private V2::KodiAPI::CAddonExeCB_GUI_DialogExtendedProgress,
     private V2::KodiAPI::CAddonExeCB_GUI_DialogFileBrowser,
     private V2::KodiAPI::CAddonExeCB_GUI_DialogKeyboard,
     private V2::KodiAPI::CAddonExeCB_GUI_DialogNumeric,
     private V2::KodiAPI::CAddonExeCB_GUI_DialogOK,
     private V2::KodiAPI::CAddonExeCB_GUI_DialogProgress,
     private V2::KodiAPI::CAddonExeCB_GUI_DialogSelect,
     private V2::KodiAPI::CAddonExeCB_GUI_DialogTextViewer,
     private V2::KodiAPI::CAddonExeCB_GUI_DialogYesNo,
     private V2::KodiAPI::CAddonExeCB_GUI_General,
     private V2::KodiAPI::CAddonExeCB_GUI_ListItem,
     private V2::KodiAPI::CAddonExeCB_GUI_Window,
     private V2::KodiAPI::CAddonExeCB_Player_InfoTagMusic,
     private V2::KodiAPI::CAddonExeCB_Player_InfoTagVideo,
     private V2::KodiAPI::CAddonExeCB_Player_PlayList,
     private V2::KodiAPI::CAddonExeCB_Player_Player,
     private V2::KodiAPI::CAddonExeCB_PVR_General,
     private V2::KodiAPI::CAddonExeCB_PVR_Transfer,
     private V2::KodiAPI::CAddonExeCB_PVR_Trigger
  {
  public:
    CBinaryAddon(bool isLocal, int fd, unsigned int id, bool isFather = false, int rand = -1);
    CBinaryAddon(const CBinaryAddon *right, int rand);
    virtual ~CBinaryAddon();

    const bool         IsActive()      const { return m_active; }
    const unsigned int GetID()         const { return m_Id; }
    const std::string  GetAddonName()  const { return m_addonName; }
    const uint64_t     GetAddonPID()   const { return m_addonAPIpid; }
    const bool         IsIndependent() const { return m_isIndependent; }
    const bool         IsSubThread()   const { return m_isSubThread; }

    bool addon_Ping();
    bool processRequest(CRequestPacket& req, CResponsePacket& resp);

  protected:
    void SetLoggedIn(bool yesNo) { m_loggedIn = yesNo; }

    virtual void Process(void);

  private:
    bool StartNetStreamThread();
    bool StartSharedMemoryThread(int rand);

    /*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
    |
    | Group to handle function calls from add-on over the TCP interface
    |
    ****************************************************************************
    */

    //==========================================================================
    /**
    * @brief Network Stream Function: KODICall_LoginVerify
    *
    * Used  from  add-on  for  login to Kodi, all  basic parts  needed  for work
    * between  Kodi and Add-on becomes created, also if possible is with  better
    * performce a shared memory interface created.
    *
    * @param[in] r             Stream Tx traffic class (CRequestPacket)
    * - Stream parameter received from add-on:
    *   |   Input Parameter        |  Store name       | Description                                    |
    *   |--------------------------|-------------------|------------------------------------------------|
    *   | @param[in] API_UINT32_T  | m_addonAPILevel   | API level add-on where add-on was created      |
    *   | @param[in] API_UINT64_T  | m_addonAPIpid     | PID used from add-on process                   |
    *   | @param[in] API_BOOLEAN   | m_isIndependent   | To become informed that add-on is independent  |
    *   | @param[in] API_STRING    | m_addonName       | The readable name of the add-on                |
    *   | @param[in] API_STRING    | m_addonAPIVersion | The readable API version name on add-on side   |
    *
    * - Stream parameter send to add-on:
    *   |   Output Parameter       |  Load name        | Description                                    |
    *   |--------------------------|-------------------|------------------------------------------------|
    *   | @param[out] API_UINT32_T | netStreamOK       | To inform add-on login was verified and OK     |
    *   | @param[out] API_INT      | usedRand          | Used random number on all the places           |
    *   | @param[out] API_UINT32_T | KODI_API_Level    | Kodi's API level number                        |
    *   | @param[out] API_STRING   | GetAppName()      | The name of Kodi itself                        |
    *   | @param[out] API_STRING   | Versions          | Kodi's own version                             |
    *   | @param[out] API_BOOLEAN  | sharedMemOK       | To inform add-on about available shared memory |
    *   | @param[out] API_INT      | m_sharedMemSize   | Size of shared memory between add-on and Kodi  |
    */
    bool process_Login(CRequestPacket& req, CResponsePacket& resp);
    //==========================================================================

    //==========================================================================
    /**
    * @brief Network Stream Function: KODICall_Logout
    *
    * Used    to   disconnect   interface   between    add-on   and   Kodi.   If
    * addon_properties.is_independent  is false becomes the  add-on killed after
    * a  timeout  from  Kodi  to   prevent  Zombies  on  the  back.  The  add-on
    * can change on  'addon_properties.is_independent'  the value to true,  this
    * prevent  them from kill of Kodi,  usable if add-on stays on background  as
    * independent application.
    *
    * @param[in] r             Stream Tx traffic class (CRequestPacket)
    * - Stream parameter received from add-on:
    *   |   Input Parameter        |  Store name       | Description                                    |
    *   |--------------------------|-------------------|------------------------------------------------|
    *   |           -              |         -         |                        -                       |
    *
    * - Stream parameter send to add-on:
    *   |   Output Parameter       |  Load name        | Description                                    |
    *   |--------------------------|-------------------|------------------------------------------------|
    *   | @param[out] API_UINT32_T | value=API_SUCCESS | To inform add-on logout was OK                 |
    */
    bool process_Logout(CRequestPacket& req, CResponsePacket& resp);
    //==========================================================================

    //==========================================================================
    /**
    * @brief Network Stream Function: KODICall_Ping
    *
    * Used  to  confirm  functional  data  transfer interface  over  network TCP
    * connection.
    *
    * @param[in] r             Stream Tx traffic class (CRequestPacket)
    * - Stream parameter received from add-on:
    *   |   Input Parameter        |  Store name       | Description                                    |
    *   |--------------------------|-------------------|------------------------------------------------|
    *   |           -              |         -         |                        -                       |
    *
    * - Stream parameter send to add-on:
    *   |   Output Parameter       |  Load name        | Description                                    |
    *   |--------------------------|-------------------|------------------------------------------------|
    *   | @param[out] API_UINT32_T | value=API_SUCCESS | To inform add-on ping was OK                   |
    */
    bool process_Ping(CRequestPacket& req, CResponsePacket& resp);
    //==========================================================================

    //==========================================================================
    /**
    * @brief Network Stream Function: KODICall_Log
    *
    * Used from add-on to send Log message over TCP stream connection in Kodi to
    * store there in Log file.
    *
    * @param[in] r             Stream Tx traffic class (CRequestPacket)
    * - Stream parameter received from add-on:
    *   |   Input Parameter        |  Store name       | Description                                    |
    *   |--------------------------|-------------------|------------------------------------------------|
    *   | @param[in] API_UINT32_T  | loglevel          | Log level format to use, see enum addon_log    |
    *   | @param[in] API_STRING    | string            | String to store in log file from Kodi          |
    *
    * - Stream parameter send to add-on:
    *   |   Output Parameter       |  Load name        | Description                                    |
    *   |--------------------------|-------------------|------------------------------------------------|
    *   | @param[out] API_UINT32_T | value=API_SUCCESS | To inform add-on ping was OK                   |
    */
    bool process_Log(CRequestPacket& req, CResponsePacket& resp);
    //==========================================================================


    bool process_CreateSubThread(CRequestPacket& req, CResponsePacket& resp);
    bool process_DeleteSubThread(CRequestPacket& req, CResponsePacket& resp);

    /*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

    std::vector<CBinaryAddon*>      m_childThreads;

    bool              m_isLocal;
    bool              m_active;
    int               m_fd;
    unsigned int      m_Id;
    CAddonAPI_Socket  m_socket;
    bool              m_loggedIn;
    bool              m_StatusInterfaceEnabled;

    int               m_addonAPILevel;
    std::string       m_addonAPIVersion;
    std::string       m_addonName;
    uint64_t          m_addonAPIpid;
    bool              m_isIndependent;
    bool              m_isFather;
    bool              m_isSubThread;
    int               m_randomConnectionNumber;

    std::string       m_addonId;
    std::string       m_addonType;
    std::string       m_addonVersion;

    CCriticalSection m_msgLock;

    AddonCB           m_addonData;
    CAddon*           m_addon;
    V2::KodiAPI::CB_AddOnLib*      m_addonCB;
    CAddonCallbacks*  m_pHelpers;

#if (defined TARGET_WINDOWS)
    CBinaryAddonSharedMemoryWindows *m_sharedMem;
#elif (defined TARGET_POSIX)
    CBinaryAddonSharedMemoryPosix   *m_sharedMem;
#endif
  };

} /* namespace ADDON */
