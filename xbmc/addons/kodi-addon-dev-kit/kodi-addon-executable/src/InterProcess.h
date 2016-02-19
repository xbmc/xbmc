#pragma once
/*
 *      Copyright (C) 2016 Team KODI
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

#include "addon/InterProcess_Addon_General.h"
#include "addon/InterProcess_Addon_Codec.h"
#include "addon/InterProcess_Addon_Network.h"
#include "addon/InterProcess_Addon_SoundPlay.h"
#include "addon/InterProcess_Addon_VFSUtils.h"
#include "audioengine/InterProcess_AudioEngine_General.h"
#include "audioengine/InterProcess_AudioEngine_Stream.h"
#include "gui/InterProcess_GUI_General.h"
#include "gui/InterProcess_GUI_ControlButton.h"
#include "gui/InterProcess_GUI_ControlEdit.h"
#include "gui/InterProcess_GUI_ControlFadeLabel.h"
#include "gui/InterProcess_GUI_ControlImage.h"
#include "gui/InterProcess_GUI_ControlLabel.h"
#include "gui/InterProcess_GUI_ControlProgress.h"
#include "gui/InterProcess_GUI_ControlRadioButton.h"
#include "gui/InterProcess_GUI_ControlRendering.h"
#include "gui/InterProcess_GUI_ControlSettingsSlider.h"
#include "gui/InterProcess_GUI_ControlSlider.h"
#include "gui/InterProcess_GUI_ControlSpin.h"
#include "gui/InterProcess_GUI_ControlTextBox.h"
#include "gui/InterProcess_GUI_DialogExtendedProgress.h"
#include "gui/InterProcess_GUI_DialogFileBrowser.h"
#include "gui/InterProcess_GUI_DialogKeyboard.h"
#include "gui/InterProcess_GUI_DialogNumeric.h"
#include "gui/InterProcess_GUI_DialogOK.h"
#include "gui/InterProcess_GUI_DialogProgress.h"
#include "gui/InterProcess_GUI_DialogSelect.h"
#include "gui/InterProcess_GUI_DialogTextViewer.h"
#include "gui/InterProcess_GUI_DialogYesNo.h"
#include "gui/InterProcess_GUI_ListItem.h"
#include "gui/InterProcess_GUI_Window.h"
#include "pvr/InterProcess_PVR_General.h"
#include "pvr/InterProcess_PVR_Transfer.h"
#include "pvr/InterProcess_PVR_Trigger.h"
#include "player/InterProcess_Player_InfoTagMusic.h"
#include "player/InterProcess_Player_InfoTagVideo.h"
#include "player/InterProcess_Player_Player.h"
#include "player/InterProcess_Player_PlayList.h"

#include "kodi/api2/AddonLib.hpp"

#include <thread>
#include <unordered_map>
#include <memory>
#include <string>
#include <p8-platform/sockets/tcp.h>
#include <p8-platform/threads/threads.h>
#if (defined TARGET_WINDOWS)
#  include "SharedMemoryWindows.h"
#elif (defined TARGET_POSIX)
#  include "SharedMemoryPosix.h"
#endif

#ifdef TARGET_WINDOWS
#include <windows.h>
#else
#ifndef __cdecl
#define __cdecl
#endif
#ifndef __declspec
#define __declspec(X)
#endif
#endif

class CResponsePacket;
class CRequestPacket;

extern "C"
{

class CKODIAddon_InterProcess
#if (defined TARGET_WINDOWS)
  : public CBinaryAddonSharedMemoryWindows,
#elif (defined TARGET_POSIX)
  : public CBinaryAddonSharedMemoryPosix,
#endif
    public P8PLATFORM::CThread,
    public CKODIAddon_InterProcess_Addon_General,
    public CKODIAddon_InterProcess_Addon_Codec,
    public CKODIAddon_InterProcess_Addon_Network,
    public CKODIAddon_InterProcess_Addon_SoundPlay,
    public CKODIAddon_InterProcess_Addon_VFSUtils,
    public CKODIAddon_InterProcess_AudioEngine_General,
    public CKODIAddon_InterProcess_AudioEngine_Stream,
    public CKODIAddon_InterProcess_GUI_General,
    public CKODIAddon_InterProcess_GUI_ControlButton,
    public CKODIAddon_InterProcess_GUI_ControlEdit,
    public CKODIAddon_InterProcess_GUI_ControlFadeLabel,
    public CKODIAddon_InterProcess_GUI_ControlImage,
    public CKODIAddon_InterProcess_GUI_ControlLabel,
    public CKODIAddon_InterProcess_GUI_ControlProgress,
    public CKODIAddon_InterProcess_GUI_ControlRadioButton,
    public CKODIAddon_InterProcess_GUI_ControlRendering,
    public CKODIAddon_InterProcess_GUI_ControlSettingsSlider,
    public CKODIAddon_InterProcess_GUI_ControlSlider,
    public CKODIAddon_InterProcess_GUI_ControlSpin,
    public CKODIAddon_InterProcess_GUI_ControlTextBox,
    public CKODIAddon_InterProcess_GUI_DialogExtendedProgress,
    public CKODIAddon_InterProcess_GUI_DialogFileBrowser,
    public CKODIAddon_InterProcess_GUI_DialogKeyboard,
    public CKODIAddon_InterProcess_GUI_DialogNumeric,
    public CKODIAddon_InterProcess_GUI_DialogOK,
    public CKODIAddon_InterProcess_GUI_DialogProgress,
    public CKODIAddon_InterProcess_GUI_DialogSelect,
    public CKODIAddon_InterProcess_GUI_DialogTextViewer,
    public CKODIAddon_InterProcess_GUI_DialogYesNo,
    public CKODIAddon_InterProcess_GUI_ListItem,
    public CKODIAddon_InterProcess_GUI_Window,
    public CKODIAddon_InterProcess_PVR_General,
    public CKODIAddon_InterProcess_PVR_Transfer,
    public CKODIAddon_InterProcess_PVR_Trigger,
    public CKODIAddon_InterProcess_Player_InfoTagMusic,
    public CKODIAddon_InterProcess_Player_InfoTagVideo,
    public CKODIAddon_InterProcess_Player_Player,
    public CKODIAddon_InterProcess_Player_PlayList
{
public:
  CKODIAddon_InterProcess();
  CKODIAddon_InterProcess(CKODIAddon_InterProcess *parent);
  virtual ~CKODIAddon_InterProcess();

  static uint64_t GetThreadId();

  bool Connect(const std::string &strHostname);
  void Disconnect();

  int Init(int argc, char *argv[], addon_properties* props, const std::string &strHostname);
  int InitLibAddon(void* hdl);
  int Finalize();

  /*
   * Function pointers to set with for connection possible types
   */
  static bool Ping();
  static void Log(const addon_log loglevel, const char* text);
  int
        (__cdecl* InitThread)
              ();

  int
        (__cdecl* FinalizeThread)
              (CKODIAddon_InterProcess* thread);

  inline const bool SharedMemUsed() const { return m_sharedMemUsed; }

protected:
  virtual void *Process(void);

  bool                        m_sharedMemUsed;

private:
  void SetFunctions_LocalNet();
  static int LocalNet_InitThread();
  static int LocalNet_FinalizeThread(CKODIAddon_InterProcess* thread);

  //--- - --- - --- - --- - --- - --- - --- - --- - --- - --- - --- - --- - ---

  void SetFunctions_SharedMem();
  static int SharedMem_InitThread();
  static int SharedMem_FinalizeThread(CKODIAddon_InterProcess* thread);

  //--- - --- - --- - --- - --- - --- - --- - --- - --- - --- - --- - --- - ---

  static void Log_Console(const addon_log loglevel, const char* text);

  friend CKODIAddon_InterProcess_Addon_General;
  friend CKODIAddon_InterProcess_Addon_Codec;
  friend CKODIAddon_InterProcess_AudioEngine_General;
  friend CKODIAddon_InterProcess_AudioEngine_Stream;
  friend CKODIAddon_InterProcess_PVR_General;
  friend CKODIAddon_InterProcess_PVR_Transfer;
  friend CKODIAddon_InterProcess_PVR_Trigger;
  friend CKODIAddon_InterProcess_GUI_DialogExtendedProgress;
  friend CKODIAddon_InterProcess_GUI_DialogFileBrowser;
  friend CKODIAddon_InterProcess_GUI_DialogKeyboard;
  friend CKODIAddon_InterProcess_GUI_DialogNumeric;
  friend CKODIAddon_InterProcess_GUI_DialogOK;
  friend CKODIAddon_InterProcess_GUI_DialogSelect;
  friend CKODIAddon_InterProcess_GUI_DialogTextViewer;
  friend CKODIAddon_InterProcess_GUI_DialogYesNo;
  friend CKODIAddon_InterProcess_GUI_DialogProgress;

  std::unique_ptr<CResponsePacket> ReadResult(CRequestPacket* vrp);
  std::unique_ptr<CResponsePacket> ReadMessage(int iInitialTimeout = 10000, int iDatapacketTimeout = 10000);
  bool TransmitMessage(CRequestPacket* vrp);
  bool readData(uint8_t* buffer, int totalBytes, int timeout);

  bool IsConnected();
  bool TryReconnect();
  bool ReadSuccess(CRequestPacket* vrp);

  std::string                 m_name;
  uint32_t                    m_KodiAPILevel;
  std::string                 m_kodiName;
  std::string                 m_kodiVersion;
  bool                        m_LoggedIn;
  std::string                 m_hostname;

  P8PLATFORM::CMutex          m_callMutex;

  P8PLATFORM::CTcpConnection *m_socket;
  P8PLATFORM::CMutex          m_readMutex;
  bool                        m_connectionLost;
  bool                        m_sharedMemAvailable;
  CKODIAddon_InterProcess    *m_parent;
  addon_properties            m_props;
};

extern CKODIAddon_InterProcess g_interProcess;
extern std::unordered_map<std::thread::id, CKODIAddon_InterProcess*> g_threadMap;

}; /* extern "C" */
