/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIWindowSystemInfo.h"

#include "GUIInfoManager.h"
#include "ServiceBroker.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIMessage.h"
#include "guilib/LocalizeStrings.h"
#include "guilib/WindowIDs.h"
#include "guilib/guiinfo/GUIInfoLabels.h"
#include "pvr/PVRManager.h"
#include "storage/MediaManager.h"
#include "utils/CPUInfo.h"
#include "utils/StringUtils.h"
#include "utils/SystemInfo.h"

constexpr int CONTROL_TEXT_START = 2;
constexpr int CONTROL_TEXT_END = 13; // 12 lines

#define CONTROL_TB_POLICY   30
#define CONTROL_BT_STORAGE  94
#define CONTROL_BT_DEFAULT  95
#define CONTROL_BT_NETWORK  96
#define CONTROL_BT_VIDEO    97
#define CONTROL_BT_HARDWARE 98
#define CONTROL_BT_PVR      99
#define CONTROL_BT_POLICY   100

constexpr int CONTROL_BT_DONATE = 101;
constexpr int CONTROL_GROUP_DONATE = 102;
constexpr int CONTROL_MULTI_IMAGE_DONATE = 103;

constexpr int CONTROL_GROUP_SYSTEM_BAR = 104;

constexpr int CONTROL_START = CONTROL_BT_STORAGE;
constexpr int CONTROL_END = CONTROL_BT_DONATE;

CGUIWindowSystemInfo::CGUIWindowSystemInfo(void) :
    CGUIWindow(WINDOW_SYSTEM_INFORMATION, "SettingsSystemInfo.xml")
{
  m_section = CONTROL_BT_DEFAULT;
  m_loadType = KEEP_IN_MEMORY;
}

CGUIWindowSystemInfo::~CGUIWindowSystemInfo(void) = default;

bool CGUIWindowSystemInfo::OnMessage(CGUIMessage& message)
{
  switch (message.GetMessage())
  {
    case GUI_MSG_WINDOW_INIT:
    {
      CGUIWindow::OnMessage(message);
      SET_CONTROL_LABEL(52, CSysInfo::GetAppName() + " " + CSysInfo::GetVersion());
      SET_CONTROL_LABEL(53, CSysInfo::GetBuildDate());
      CONTROL_ENABLE_ON_CONDITION(CONTROL_BT_PVR, CServiceBroker::GetPVRManager().IsStarted());
      return true;
    }
    break;

    case GUI_MSG_WINDOW_DEINIT:
    {
      CGUIWindow::OnMessage(message);
      m_diskUsage.clear();
      m_privacyPolicyLoaded = false;
      return true;
    }
    break;

    case GUI_MSG_FOCUSED:
    {
      CGUIWindow::OnMessage(message);
      int focusedControl = GetFocusedControlID();
      if (m_section != focusedControl && focusedControl >= CONTROL_START && focusedControl <= CONTROL_END)
      {
        ResetLabels();
        m_section = focusedControl;
      }
      if (m_section >= CONTROL_BT_STORAGE && m_section <= CONTROL_BT_PVR)
      {
        SET_CONTROL_HIDDEN(CONTROL_TB_POLICY);
        SET_CONTROL_HIDDEN(CONTROL_GROUP_DONATE);
        SET_CONTROL_VISIBLE(CONTROL_GROUP_SYSTEM_BAR);
      }
      else if (m_section == CONTROL_BT_POLICY)
      {
        LoadPrivacyPolicy();
        SET_CONTROL_VISIBLE(CONTROL_TB_POLICY);
        SET_CONTROL_HIDDEN(CONTROL_GROUP_DONATE);
        SET_CONTROL_VISIBLE(CONTROL_GROUP_SYSTEM_BAR);
      }
      else if (m_section == CONTROL_BT_DONATE)
      {
        SET_CONTROL_HIDDEN(CONTROL_TB_POLICY);
        SET_CONTROL_VISIBLE(CONTROL_GROUP_DONATE);
        SET_CONTROL_HIDDEN(CONTROL_GROUP_SYSTEM_BAR);
      }
      return true;
    }
    break;
  }
  return CGUIWindow::OnMessage(message);
}

void CGUIWindowSystemInfo::FrameMove()
{
  int i = CONTROL_TEXT_START;
  if (m_section == CONTROL_BT_DEFAULT)
  {
    SET_CONTROL_LABEL(40, g_localizeStrings.Get(20154));
    SetControlLabel(i++, "{}: {}", 158, SYSTEM_FREE_MEMORY);
    SetControlLabel(i++, "{}: {}", 150, NETWORK_IP_ADDRESS);
    SetControlLabel(i++, "{} {}", 13287, SYSTEM_SCREEN_RESOLUTION);
    SetControlLabel(i++, "{} {}", 13283, SYSTEM_OS_VERSION_INFO);
    SetControlLabel(i++, "{}: {}", 12390, SYSTEM_UPTIME);
    SetControlLabel(i++, "{}: {}", 12394, SYSTEM_TOTALUPTIME);
    SetControlLabel(i++, "{}: {}", 12395, SYSTEM_BATTERY_LEVEL);
  }

  else if (m_section == CONTROL_BT_STORAGE)
  {
    SET_CONTROL_LABEL(40, g_localizeStrings.Get(20155));
    if (m_diskUsage.empty())
      m_diskUsage = CServiceBroker::GetMediaManager().GetDiskUsage();

    for (size_t d = 0; d < m_diskUsage.size() && d <= CONTROL_TEXT_END - CONTROL_TEXT_START; ++d)
    {
      SET_CONTROL_LABEL(i++, m_diskUsage[d]);
    }
  }

  else if (m_section == CONTROL_BT_NETWORK)
  {
    SET_CONTROL_LABEL(40,g_localizeStrings.Get(20158));
    SET_CONTROL_LABEL(i++, CServiceBroker::GetGUI()->GetInfoManager().GetLabel(
                               NETWORK_LINK_STATE, INFO::DEFAULT_CONTEXT));
    SetControlLabel(i++, "{}: {}", 149, NETWORK_MAC_ADDRESS);
    SetControlLabel(i++, "{}: {}", 150, NETWORK_IP_ADDRESS);
    SetControlLabel(i++, "{}: {}", 13159, NETWORK_SUBNET_MASK);
    SetControlLabel(i++, "{}: {}", 13160, NETWORK_GATEWAY_ADDRESS);
    SetControlLabel(i++, "{}: {}", 13161, NETWORK_DNS1_ADDRESS);
    SetControlLabel(i++, "{}: {}", 20307, NETWORK_DNS2_ADDRESS);
    SetControlLabel(i++, "{} {}", 13295, SYSTEM_INTERNET_STATE);
  }

  else if (m_section == CONTROL_BT_VIDEO)
  {
    SET_CONTROL_LABEL(40,g_localizeStrings.Get(20159));
    SET_CONTROL_LABEL(i++, CServiceBroker::GetGUI()->GetInfoManager().GetLabel(
                               SYSTEM_VIDEO_ENCODER_INFO, INFO::DEFAULT_CONTEXT));
    SetControlLabel(i++, "{} {}", 13287, SYSTEM_SCREEN_RESOLUTION);

    auto renderingSystem = CServiceBroker::GetRenderSystem();
    if (renderingSystem)
    {
      static std::string vendor = renderingSystem->GetRenderVendor();
      if (!vendor.empty())
        SET_CONTROL_LABEL(i++, StringUtils::Format("{} {}", g_localizeStrings.Get(22007), vendor));

#if defined(HAS_DX)
      int renderVersionLabel = 22024;
#else
      int renderVersionLabel = 22009;
#endif
      static std::string version = renderingSystem->GetRenderVersionString();
      if (!version.empty())
        SET_CONTROL_LABEL(
            i++, StringUtils::Format("{} {}", g_localizeStrings.Get(renderVersionLabel), version));
    }

    auto windowSystem = CServiceBroker::GetWinSystem();
    if (windowSystem)
    {
      static std::string platform = windowSystem->GetName();
      if (platform != "platform default")
        SET_CONTROL_LABEL(i++,
                          StringUtils::Format("{} {}", g_localizeStrings.Get(39153), platform));
    }

    SetControlLabel(i++, "{} {}", 22010, SYSTEM_GPU_TEMPERATURE);

    const std::string hdrTypes = CServiceBroker::GetGUI()->GetInfoManager().GetLabel(
        SYSTEM_SUPPORTED_HDR_TYPES, INFO::DEFAULT_CONTEXT);
    SET_CONTROL_LABEL(
        i++, StringUtils::Format("{}: {}", g_localizeStrings.Get(39174),
                                 hdrTypes.empty() ? g_localizeStrings.Get(231) : hdrTypes));
  }

  else if (m_section == CONTROL_BT_HARDWARE)
  {
    SET_CONTROL_LABEL(40,g_localizeStrings.Get(20160));

    auto cpuInfo = CServiceBroker::GetCPUInfo();
    if (cpuInfo)
    {
      static std::string model = cpuInfo->GetCPUModel();
      if (!model.empty())
        SET_CONTROL_LABEL(i++, "CPU: " + model);

      static std::string mips = cpuInfo->GetCPUBogoMips();
      if (!mips.empty())
        SET_CONTROL_LABEL(i++, "BogoMips: " + mips);

      static std::string soc = cpuInfo->GetCPUSoC();
      if (!soc.empty())
        SET_CONTROL_LABEL(i++, "SoC: " + soc);

      static std::string hardware = cpuInfo->GetCPUHardware();
      if (!hardware.empty())
        SET_CONTROL_LABEL(i++, "Hardware: " + hardware);

      static std::string revision = cpuInfo->GetCPURevision();
      if (!revision.empty())
        SET_CONTROL_LABEL(i++, "Revision: " + revision);

      static std::string serial = cpuInfo->GetCPUSerial();
      if (!serial.empty())
        SET_CONTROL_LABEL(i++, "Serial: " + serial);

      // temperature can't really be conditional because of localization units
      SetControlLabel(i++, "{} {}", 22011, SYSTEM_CPU_TEMPERATURE);

      // we can check if the cpufrequency is not 0 (default if not implemented)
      // but we have to call through CGUIInfoManager -> CSystemGUIInfo -> CSysInfo
      // to limit the frequency of updates
      static float cpuFreq = cpuInfo->GetCPUFrequency();
      if (cpuFreq > 0)
        SetControlLabel(i++, "{} {}", 13284, SYSTEM_CPUFREQUENCY);
    }
  }

  else if (m_section == CONTROL_BT_PVR)
  {
    SET_CONTROL_LABEL(40, g_localizeStrings.Get(19166));
    int i = CONTROL_TEXT_START;

    SetControlLabel(i++, "{}: {}", 19120, PVR_BACKEND_NUMBER);
    i++;  // empty line
    SetControlLabel(i++, "{}: {}", 19012, PVR_BACKEND_NAME);
    SetControlLabel(i++, "{}: {}", 19114, PVR_BACKEND_VERSION);
    SetControlLabel(i++, "{}: {}", 19115, PVR_BACKEND_HOST);
    SetControlLabel(i++, "{}: {}", 19116, PVR_BACKEND_DISKSPACE);
    SetControlLabel(i++, "{}: {}", 19334, PVR_BACKEND_PROVIDERS);
    SetControlLabel(i++, "{}: {}", 19042, PVR_BACKEND_CHANNEL_GROUPS);
    SetControlLabel(i++, "{}: {}", 19019, PVR_BACKEND_CHANNELS);
    SetControlLabel(i++, "{}: {}", 19163, PVR_BACKEND_RECORDINGS);
    SetControlLabel(i++, "{}: {}", 19168,
                    PVR_BACKEND_DELETED_RECORDINGS); // Deleted and recoverable recordings
    SetControlLabel(i++, "{}: {}", 19025, PVR_BACKEND_TIMERS);
  }

  else if (m_section == CONTROL_BT_POLICY)
  {
    SET_CONTROL_LABEL(40, g_localizeStrings.Get(12389));
  }
  CGUIWindow::FrameMove();
}

void CGUIWindowSystemInfo::ResetLabels()
{
  for (int i = CONTROL_TEXT_START; i <= CONTROL_TEXT_END; ++i)
  {
    SET_CONTROL_LABEL(i, "");
  }

  // Reset the multiimage to the beginning
  CGUIMessage msg{GUI_MSG_RESET_MULTI_IMAGE, GetID(), CONTROL_MULTI_IMAGE_DONATE};
  OnMessage(msg);
}

void CGUIWindowSystemInfo::SetControlLabel(int id, const char *format, int label, int info)
{
  std::string tmpStr = StringUtils::Format(
      format, g_localizeStrings.Get(label),
      CServiceBroker::GetGUI()->GetInfoManager().GetLabel(info, INFO::DEFAULT_CONTEXT));
  SET_CONTROL_LABEL(id, tmpStr);
}

void CGUIWindowSystemInfo::LoadPrivacyPolicy()
{
  if (!m_privacyPolicyLoaded)
  {
    m_privacyPolicyLoaded = true;
    SET_CONTROL_LABEL(CONTROL_TB_POLICY, CServiceBroker::GetGUI()->GetInfoManager().GetLabel(
                                             SYSTEM_PRIVACY_POLICY, INFO::DEFAULT_CONTEXT));
  }
}
