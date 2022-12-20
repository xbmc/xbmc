/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIWindowSettingsScreenCalibration.h"

#include "ServiceBroker.h"
#include "application/ApplicationComponents.h"
#include "application/ApplicationPlayer.h"
#include "dialogs/GUIDialogYesNo.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIMoverControl.h"
#include "guilib/GUIResizeControl.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/LocalizeStrings.h"
#include "input/actions/Action.h"
#include "input/actions/ActionIDs.h"
#include "settings/DisplaySettings.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "settings/SubtitlesSettings.h"
#include "utils/StringUtils.h"
#include "utils/Variant.h"
#include "utils/log.h"
#include "windowing/WinSystem.h"

#include <string>
#include <utility>

using namespace KODI;

namespace
{
constexpr int CONTROL_LABEL_RES = 2;
constexpr int CONTROL_LABEL_DESCRIPTION = 3;
constexpr int CONTROL_LABEL_VALUE = 4;
constexpr int CONTROL_TOP_LEFT = 8;
constexpr int CONTROL_BOTTOM_RIGHT = 9;
constexpr int CONTROL_SUBTITLES = 10;
constexpr int CONTROL_PIXEL_RATIO = 11;
constexpr int CONTROL_RESET = 12;
constexpr int CONTROL_VIDEO = 20;

constexpr int DEFAULT_GUI_HEIGHT = 1080;
constexpr int DEFAULT_GUI_WIDTH = 1920;

// Fixed transparent space of the subtitle bar (on top + below) for touch screen
// must match with the space of the skin bar image
constexpr int CONTROL_SUBTITLES_SPACE = 80;
} // unnamed namespace

CGUIWindowSettingsScreenCalibration::CGUIWindowSettingsScreenCalibration(void)
  : CGUIWindow(WINDOW_SCREEN_CALIBRATION, "SettingsScreenCalibration.xml")
{
  m_iCurRes = 0;
  m_iControl = 0;
  m_fPixelRatioBoxHeight = 0.0f;
  m_needsScaling = false; // we handle all the scaling
}

CGUIWindowSettingsScreenCalibration::~CGUIWindowSettingsScreenCalibration(void) = default;


void CGUIWindowSettingsScreenCalibration::ResetCalibration()
{
  // We ask to reset the calibration
  // Reset will be applied to: windowed mode or per fullscreen resolution
  CGUIDialogYesNo* pDialog =
      CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogYesNo>(WINDOW_DIALOG_YES_NO);
  pDialog->SetHeading(CVariant{20325});
  std::string strText = StringUtils::Format(
      g_localizeStrings.Get(20326),
      CServiceBroker::GetWinSystem()->GetGfxContext().GetResInfo(m_Res[m_iCurRes]).strMode);
  pDialog->SetText(CVariant{std::move(strText)});
  pDialog->SetChoice(0, CVariant{222});
  pDialog->SetChoice(1, CVariant{186});
  pDialog->Open();
  if (pDialog->IsConfirmed())
  {
    CServiceBroker::GetWinSystem()->GetGfxContext().ResetScreenParameters(m_Res[m_iCurRes]);
    ResetControls();
    // Send GUI_MSG_WINDOW_RESIZE to rescale font size/aspect for label controls
    CServiceBroker::GetGUI()->GetWindowManager().SendMessage(
        GUI_MSG_NOTIFY_ALL, WINDOW_SCREEN_CALIBRATION, 0, GUI_MSG_WINDOW_RESIZE);
  }
}

bool CGUIWindowSettingsScreenCalibration::OnAction(const CAction& action)
{
  switch (action.GetID())
  {
    case ACTION_CALIBRATE_SWAP_ARROWS:
    {
      NextControl();
      return true;
    }
    break;

    case ACTION_CALIBRATE_RESET:
    {
      ResetCalibration();
      return true;
    }
    break;

    case ACTION_CHANGE_RESOLUTION:
      // choose the next resolution in our list
      {
        m_iCurRes = (m_iCurRes + 1) % m_Res.size();
        CServiceBroker::GetWinSystem()->GetGfxContext().SetVideoResolution(m_Res[m_iCurRes], false);
        ResetControls();
        // Send GUI_MSG_WINDOW_RESIZE to rescale font size/aspect for label controls
        CServiceBroker::GetGUI()->GetWindowManager().SendMessage(
            GUI_MSG_NOTIFY_ALL, WINDOW_SCREEN_CALIBRATION, 0, GUI_MSG_WINDOW_RESIZE);
        return true;
      }
      break;

    // ignore all gesture meta actions
    case ACTION_GESTURE_BEGIN:
    case ACTION_GESTURE_END:
    case ACTION_GESTURE_ABORT:
    case ACTION_GESTURE_NOTIFY:
    case ACTION_GESTURE_PAN:
    case ACTION_GESTURE_ROTATE:
    case ACTION_GESTURE_ZOOM:
      return true;

    case ACTION_MOUSE_LEFT_CLICK:
    case ACTION_TOUCH_TAP:
      if (GetFocusedControlID() == CONTROL_RESET)
      {
        ResetCalibration();
        return true;
      }
      break;
  }

  // if we see a mouse move event without dx and dy (amount2 and amount3) these
  // are the focus actions which are generated on touch events and those should
  // be eaten/ignored here. Else we will switch to the screencalibration controls
  // which are at that x/y value on each touch/tap/swipe which makes the whole window
  // unusable for touch screens
  if (action.GetID() == ACTION_MOUSE_MOVE && action.GetAmount(2) == 0 && action.GetAmount(3) == 0)
    return true;

  return CGUIWindow::OnAction(action); // base class to handle basic movement etc.
}

void CGUIWindowSettingsScreenCalibration::AllocResources(bool forceLoad)
{
  CGUIWindow::AllocResources(forceLoad);
}

void CGUIWindowSettingsScreenCalibration::FreeResources(bool forceUnload)
{
  CGUIWindow::FreeResources(forceUnload);
}


bool CGUIWindowSettingsScreenCalibration::OnMessage(CGUIMessage& message)
{
  switch (message.GetMessage())
  {
    case GUI_MSG_WINDOW_DEINIT:
    {
      CDisplaySettings::GetInstance().UpdateCalibrations();
      CServiceBroker::GetSettingsComponent()->GetSettings()->Save();
      CServiceBroker::GetWinSystem()->GetGfxContext().SetCalibrating(false);
      // reset our screen resolution to what it was initially
      CServiceBroker::GetWinSystem()->GetGfxContext().SetVideoResolution(
          CDisplaySettings::GetInstance().GetCurrentResolution(), false);
      CServiceBroker::GetGUI()->GetWindowManager().SendMessage(
          GUI_MSG_NOTIFY_ALL, WINDOW_SCREEN_CALIBRATION, 0, GUI_MSG_WINDOW_RESIZE);
    }
    break;

    case GUI_MSG_WINDOW_INIT:
    {
      CGUIWindow::OnMessage(message);
      CServiceBroker::GetWinSystem()->GetGfxContext().SetCalibrating(true);

      // Get the default XML size values of controls,
      // we will use these values to scale controls when the resolution change
      for (int id = CONTROL_TOP_LEFT; id <= CONTROL_RESET; id++)
      {
        CGUIControl* control = GetControl(id);
        if (control)
        {
          m_controlsSize.emplace(id, std::make_pair(control->GetHeight(), control->GetWidth()));
        }
      }

      // Get the allowable resolutions that we can calibrate...
      m_Res.clear();

      auto& components = CServiceBroker::GetAppComponents();
      const auto appPlayer = components.GetComponent<CApplicationPlayer>();
      bool isPlayingVideo{appPlayer->IsPlayingVideo()};
      if (isPlayingVideo)
      { // don't allow resolution switching if we are playing a video

        appPlayer->TriggerUpdateResolution();

        m_iCurRes = 0;
        m_Res.push_back(CServiceBroker::GetWinSystem()->GetGfxContext().GetVideoResolution());
        SET_CONTROL_VISIBLE(CONTROL_VIDEO);
      }
      else
      {
        SET_CONTROL_HIDDEN(CONTROL_VIDEO);
        CServiceBroker::GetWinSystem()->GetGfxContext().GetAllowedResolutions(m_Res);
        // find our starting resolution
        m_iCurRes = FindCurrentResolution();
      }

      // Setup the first control
      m_iControl = CONTROL_TOP_LEFT;

      m_isSubtitleBarEnabled =
          !(CServiceBroker::GetSettingsComponent()->GetSubtitlesSettings()->GetAlignment() !=
                SUBTITLES::Align::MANUAL &&
            isPlayingVideo);

      ResetControls();
      return true;
    }
    break;
    case GUI_MSG_CLICKED:
    {
      // On click event select the next control
      NextControl();
    }
    break;
    case GUI_MSG_NOTIFY_ALL:
    {
      if (message.GetParam1() == GUI_MSG_WINDOW_RESIZE &&
          message.GetSenderId() != WINDOW_SCREEN_CALIBRATION && IsActive())
      {
        m_Res.clear();
        CServiceBroker::GetWinSystem()->GetGfxContext().GetAllowedResolutions(m_Res);
        m_iCurRes = FindCurrentResolution();
        ResetControls();
      }
    }
    break;
    // send before touch for requesting gesture features - we don't want this
    // it would result in unfocus in the onmessage below ...
    case GUI_MSG_GESTURE_NOTIFY:
    // send after touch for unfocussing - we don't want this in this window!
    case GUI_MSG_UNFOCUS_ALL:
      return true;
      break;
  }
  return CGUIWindow::OnMessage(message);
}

unsigned int CGUIWindowSettingsScreenCalibration::FindCurrentResolution()
{
  RESOLUTION curRes = CServiceBroker::GetWinSystem()->GetGfxContext().GetVideoResolution();
  for (size_t i = 0; i < m_Res.size(); i++)
  {
    // If it's a CUSTOM (monitor) resolution, then CServiceBroker::GetWinSystem()->GetGfxContext().GetAllowedResolutions()
    // returns just one entry with CUSTOM in it. Update that entry to point to the current
    // CUSTOM resolution.
    if (curRes >= RES_CUSTOM)
    {
      if (m_Res[i] == RES_CUSTOM)
      {
        m_Res[i] = curRes;
        return i;
      }
    }
    else if (m_Res[i] == CServiceBroker::GetWinSystem()->GetGfxContext().GetVideoResolution())
      return i;
  }
  CLog::Log(LOGERROR, "CALIBRATION: Reported current resolution: {}",
            CServiceBroker::GetWinSystem()->GetGfxContext().GetVideoResolution());
  CLog::Log(LOGERROR,
            "CALIBRATION: Could not determine current resolution, falling back to default");
  return 0;
}

void CGUIWindowSettingsScreenCalibration::NextControl()
{ // set the old control invisible and not focused, and choose the next control
  CGUIControl* pControl = GetControl(m_iControl);
  if (pControl)
  {
    pControl->SetVisible(false);
    pControl->SetFocus(false);
  }
  // If the current control is the reset button
  // ask to reset the calibration settings
  if (m_iControl == CONTROL_RESET)
  {
    ResetCalibration();
  }
  // switch to the next control
  m_iControl++;
  if (m_iControl > CONTROL_RESET)
    m_iControl = CONTROL_TOP_LEFT;
  // enable the new control
  EnableControl(m_iControl);
}

void CGUIWindowSettingsScreenCalibration::EnableControl(int iControl)
{
  SET_CONTROL_VISIBLE(CONTROL_TOP_LEFT);
  SET_CONTROL_VISIBLE(CONTROL_BOTTOM_RIGHT);
  SET_CONTROL_VISIBLE(CONTROL_SUBTITLES);
  SET_CONTROL_VISIBLE(CONTROL_PIXEL_RATIO);
  SET_CONTROL_VISIBLE(CONTROL_RESET);
  SET_CONTROL_FOCUS(iControl, 0);
}

void CGUIWindowSettingsScreenCalibration::ResetControls()
{
  // disable the video control, so that our other controls take mouse clicks etc.
  CONTROL_DISABLE(CONTROL_VIDEO);
  // disable the UI calibration for our controls
  // and set their limits
  // also, set them to invisible if they don't have focus
  RESOLUTION_INFO info =
      CServiceBroker::GetWinSystem()->GetGfxContext().GetResInfo(m_Res[m_iCurRes]);

  m_subtitleVerticalMargin = static_cast<int>(
      static_cast<float>(info.iHeight) / 100 *
      CServiceBroker::GetSettingsComponent()->GetSubtitlesSettings()->GetVerticalMarginPerc());

  CGUIMoverControl* pControl = dynamic_cast<CGUIMoverControl*>(GetControl(CONTROL_TOP_LEFT));
  if (pControl)
  {
    pControl->SetLimits(-info.iWidth / 4, -info.iHeight / 4, info.iWidth / 4, info.iHeight / 4);
    auto& size = m_controlsSize[CONTROL_TOP_LEFT];
    pControl->SetHeight(size.first / DEFAULT_GUI_HEIGHT * info.iHeight);
    pControl->SetWidth(size.second / DEFAULT_GUI_WIDTH * info.iWidth);
    pControl->SetPosition(static_cast<float>(info.Overscan.left) + info.guiInsets.left,
                          static_cast<float>(info.Overscan.top) + info.guiInsets.top);
    pControl->SetLocation(info.Overscan.left, info.Overscan.top, false);
  }
  pControl = dynamic_cast<CGUIMoverControl*>(GetControl(CONTROL_BOTTOM_RIGHT));
  if (pControl)
  {
    pControl->SetLimits(info.iWidth * 3 / 4, info.iHeight * 3 / 4, info.iWidth * 5 / 4,
                        info.iHeight * 5 / 4);
    auto& size = m_controlsSize[CONTROL_BOTTOM_RIGHT];
    pControl->SetHeight(size.first / DEFAULT_GUI_HEIGHT * info.iHeight);
    pControl->SetWidth(size.second / DEFAULT_GUI_WIDTH * info.iWidth);
    pControl->SetPosition(
        static_cast<float>(info.Overscan.right) - info.guiInsets.right - pControl->GetWidth(),
        static_cast<float>(info.Overscan.bottom) - info.guiInsets.bottom - pControl->GetHeight());
    pControl->SetLocation(info.Overscan.right, info.Overscan.bottom, false);
  }
  // Subtitles and OSD controls can only move up and down
  pControl = dynamic_cast<CGUIMoverControl*>(GetControl(CONTROL_SUBTITLES));
  if (pControl)
  {
    auto& size = m_controlsSize[CONTROL_SUBTITLES];
    float scaledHeight = size.first / DEFAULT_GUI_HEIGHT * info.iHeight;
    float scaledSpace =
        static_cast<float>(CONTROL_SUBTITLES_SPACE) / DEFAULT_GUI_HEIGHT * info.iHeight;
    m_subtitlesHalfSpace = static_cast<int>(scaledSpace / 2);
    int barHeight = static_cast<int>(scaledHeight - scaledSpace);
    pControl->SetLimits(0,
                        m_subtitlesHalfSpace + barHeight + info.Overscan.top + info.guiInsets.top,
                        0, info.Overscan.bottom + m_subtitlesHalfSpace - info.guiInsets.bottom);
    pControl->SetHeight(scaledHeight);
    pControl->SetWidth(size.second / DEFAULT_GUI_WIDTH * info.iWidth);
    // If the vertical margin has been changed from the previous calibration,
    // the text bar could appear offscreen, then force move to visible area
    if (info.iSubtitles - m_subtitleVerticalMargin >
        info.iHeight + info.guiInsets.top - info.guiInsets.bottom)
      info.iSubtitles = info.Overscan.bottom - info.guiInsets.bottom;
    // We want the text to be at the base of the bar,
    // then we shift the position to include the vertical margin
    pControl->SetPosition((info.iWidth - pControl->GetWidth()) * 0.5f,
                          info.iSubtitles - pControl->GetHeight() + m_subtitlesHalfSpace -
                              m_subtitleVerticalMargin);
    pControl->SetLocation(0, info.iSubtitles + m_subtitlesHalfSpace - m_subtitleVerticalMargin,
                          false);
    pControl->SetEnabled(m_isSubtitleBarEnabled);
  }
  // The pixel ratio control
  CGUIResizeControl* pResize = dynamic_cast<CGUIResizeControl*>(GetControl(CONTROL_PIXEL_RATIO));
  if (pResize)
  {
    pResize->SetLimits(info.iWidth * 0.25f, info.iHeight * 0.5f, info.iWidth * 0.75f,
                       info.iHeight * 0.5f);
    pResize->SetHeight(info.iHeight * 0.5f);
    pResize->SetWidth(pResize->GetHeight() / info.fPixelRatio);
    pResize->SetPosition((info.iWidth - pResize->GetWidth()) / 2,
                         (info.iHeight - pResize->GetHeight()) / 2);
  }
  // The calibration reset
  pControl = dynamic_cast<CGUIMoverControl*>(GetControl(CONTROL_RESET));
  if (pControl)
  {
    auto& size = m_controlsSize[CONTROL_RESET];
    pControl->SetHeight(size.first / DEFAULT_GUI_HEIGHT * info.iHeight);
    pControl->SetWidth(size.second / DEFAULT_GUI_WIDTH * info.iWidth);
    float posX = 0 + info.guiInsets.right;
    float posY =
        static_cast<float>(info.Overscan.bottom) - info.guiInsets.bottom - pControl->GetHeight();
    pControl->SetLimits(posX, posY, posX, posY);
    pControl->SetPosition(posX, posY);
    pControl->SetLocation(posX, posY, false);
  }
  // Enable the default control
  EnableControl(m_iControl);
}

bool CGUIWindowSettingsScreenCalibration::UpdateFromControl(int iControl)
{
  RESOLUTION_INFO info =
      CServiceBroker::GetWinSystem()->GetGfxContext().GetResInfo(m_Res[m_iCurRes]);
  RESOLUTION_INFO infoPrev = info;
  std::string labelDescription;
  std::string labelValue;

  if (iControl == CONTROL_PIXEL_RATIO)
  {
    CGUIControl* pControl = GetControl(CONTROL_PIXEL_RATIO);
    if (pControl)
    {
      float fWidth = pControl->GetWidth();
      float fHeight = pControl->GetHeight();
      info.fPixelRatio = fHeight / fWidth;
      // recenter our control...
      pControl->SetPosition((static_cast<float>(info.iWidth) - pControl->GetWidth()) / 2,
                            (static_cast<float>(info.iHeight) - pControl->GetHeight()) / 2);
      labelDescription = StringUtils::Format("[B]{}[/B][CR]{}", g_localizeStrings.Get(272),
                                             g_localizeStrings.Get(273));
      labelValue = StringUtils::Format("{:5.3f}", info.fPixelRatio);
      labelValue = StringUtils::Format(g_localizeStrings.Get(20327), labelValue);
    }
  }
  else
  {
    CGUIMoverControl* pControl = dynamic_cast<CGUIMoverControl*>(GetControl(iControl));
    if (pControl)
    {
      switch (iControl)
      {
        case CONTROL_TOP_LEFT:
        {
          info.Overscan.left = pControl->GetXLocation();
          info.Overscan.top = pControl->GetYLocation();
          labelDescription = StringUtils::Format("[B]{}[/B][CR]{}", g_localizeStrings.Get(274),
                                                 g_localizeStrings.Get(276));
          labelValue =
              StringUtils::Format("{}, {}", pControl->GetXLocation(), pControl->GetYLocation());
          labelValue = StringUtils::Format(g_localizeStrings.Get(20327), labelValue);
          // Update reset control position
          CGUIMoverControl* pControl = dynamic_cast<CGUIMoverControl*>(GetControl(CONTROL_RESET));
          if (pControl)
          {
            float posX = info.Overscan.left + info.guiInsets.left;
            float posY = info.Overscan.bottom - pControl->GetHeight() - info.guiInsets.bottom;
            pControl->SetLimits(posX, posY, posX, posY);
            pControl->SetPosition(posX, posY);
            pControl->SetLocation(posX, posY, false);
          }
        }
        break;

        case CONTROL_BOTTOM_RIGHT:
        {
          info.Overscan.right = pControl->GetXLocation();
          info.Overscan.bottom = pControl->GetYLocation();
          int iXOff1 = info.iWidth - pControl->GetXLocation();
          int iYOff1 = info.iHeight - pControl->GetYLocation();
          labelDescription = StringUtils::Format("[B]{}[/B][CR]{}", g_localizeStrings.Get(275),
                                                 g_localizeStrings.Get(276));
          labelValue = StringUtils::Format("{}, {}", iXOff1, iYOff1);
          labelValue = StringUtils::Format(g_localizeStrings.Get(20327), labelValue);
          // Update reset control position
          pControl = dynamic_cast<CGUIMoverControl*>(GetControl(CONTROL_RESET));
          if (pControl)
          {
            float posX = info.Overscan.left + info.guiInsets.left;
            float posY = info.Overscan.bottom - pControl->GetHeight() - info.guiInsets.bottom;
            pControl->SetLimits(posX, posY, posX, posY);
            pControl->SetPosition(posX, posY);
            pControl->SetLocation(posX, posY, false);
          }
        }
        break;

        case CONTROL_SUBTITLES:
        {
          if (m_isSubtitleBarEnabled)
          {
            info.iSubtitles =
                pControl->GetYLocation() - m_subtitlesHalfSpace + m_subtitleVerticalMargin;

            labelDescription = StringUtils::Format("[B]{}[/B][CR]{}", g_localizeStrings.Get(277),
                                                   g_localizeStrings.Get(278));
            labelValue = StringUtils::Format(g_localizeStrings.Get(39184), info.iSubtitles,
                                             info.iSubtitles - m_subtitleVerticalMargin);
          }
          else
          {
            labelDescription = StringUtils::Format("[B]{}[/B][CR]{}", g_localizeStrings.Get(277),
                                                   g_localizeStrings.Get(39189));
          }
        }
        break;

        case CONTROL_RESET:
        {
          labelDescription = g_localizeStrings.Get(20325);
        }
        break;
      }
    }
  }

  SET_CONTROL_LABEL(CONTROL_LABEL_DESCRIPTION, labelDescription);
  SET_CONTROL_LABEL(CONTROL_LABEL_VALUE, labelValue);

  // Set resolution info text
  std::string resInfo;
  if (CServiceBroker::GetWinSystem()->IsFullScreen())
  {
    resInfo =
        StringUtils::Format("{} {}x{}@{:.2f} - {}", g_localizeStrings.Get(13287), info.iScreenWidth,
                            info.iScreenHeight, info.fRefreshRate, g_localizeStrings.Get(244));
  }
  else
  {
    resInfo = StringUtils::Format("{} {}x{} - {}", g_localizeStrings.Get(13287), info.iScreenWidth,
                                  info.iScreenHeight, g_localizeStrings.Get(242));
  }
  SET_CONTROL_LABEL(CONTROL_LABEL_RES, resInfo);

  // Detect overscan changes
  bool isOverscanChanged = info.Overscan != infoPrev.Overscan;

  // Adjust subtitle bar position due to overscan changes
  if (isOverscanChanged)
  {
    CGUIMoverControl* pControl = dynamic_cast<CGUIMoverControl*>(GetControl(CONTROL_SUBTITLES));
    if (pControl)
    {
      // Keep the subtitle bar within the overscan boundary
      if (info.Overscan.bottom + m_subtitleVerticalMargin < info.iSubtitles)
      {
        info.iSubtitles = info.Overscan.bottom - info.guiInsets.bottom + m_subtitleVerticalMargin;

        // We want the text to be at the base of the bar,
        // then we shift the position to include the vertical margin
        pControl->SetPosition((info.iWidth - pControl->GetWidth()) * 0.5f,
                              info.iSubtitles - pControl->GetHeight() + m_subtitlesHalfSpace -
                                  m_subtitleVerticalMargin);
        pControl->SetLocation(0, info.iSubtitles + m_subtitlesHalfSpace - m_subtitleVerticalMargin,
                              false);
      }

      // Recalculate limits based on overscan values
      const auto& size = m_controlsSize[CONTROL_SUBTITLES];
      const float scaledHeight = size.first / DEFAULT_GUI_HEIGHT * info.iHeight;
      const float scaledSpace =
          static_cast<float>(CONTROL_SUBTITLES_SPACE) / DEFAULT_GUI_HEIGHT * info.iHeight;

      m_subtitlesHalfSpace = static_cast<int>(scaledSpace / 2);
      const int barHeight = static_cast<int>(scaledHeight - scaledSpace);

      pControl->SetLimits(0,
                          m_subtitlesHalfSpace + barHeight + info.Overscan.top + info.guiInsets.top,
                          0, info.Overscan.bottom + m_subtitlesHalfSpace);
    }
  }

  CServiceBroker::GetWinSystem()->GetGfxContext().SetResInfo(m_Res[m_iCurRes], info);

  return isOverscanChanged;
}

void CGUIWindowSettingsScreenCalibration::FrameMove()
{
  m_iControl = GetFocusedControlID();
  if (m_iControl >= 0)
  {
    if (UpdateFromControl(m_iControl))
    {
      // Send GUI_MSG_WINDOW_RESIZE to rescale font size/aspect for label controls
      CServiceBroker::GetGUI()->GetWindowManager().SendMessage(
          GUI_MSG_NOTIFY_ALL, WINDOW_SCREEN_CALIBRATION, 0, GUI_MSG_WINDOW_RESIZE);
    }
  }
  else
  {
    SET_CONTROL_LABEL(CONTROL_LABEL_DESCRIPTION, "");
    SET_CONTROL_LABEL(CONTROL_LABEL_VALUE, "");
    SET_CONTROL_LABEL(CONTROL_LABEL_RES, "");
  }
  CGUIWindow::FrameMove();
}

void CGUIWindowSettingsScreenCalibration::DoProcess(unsigned int currentTime,
                                                    CDirtyRegionList& dirtyregions)
{
  MarkDirtyRegion();

  for (int i = CONTROL_TOP_LEFT; i <= CONTROL_RESET; i++)
    SET_CONTROL_HIDDEN(i);
  m_needsScaling = true;
  CGUIWindow::DoProcess(currentTime, dirtyregions);
  m_needsScaling = false;

  CServiceBroker::GetWinSystem()->GetGfxContext().SetRenderingResolution(m_Res[m_iCurRes], false);
  CServiceBroker::GetWinSystem()->GetGfxContext().AddGUITransform();

  // process the movers etc.
  for (int i = CONTROL_TOP_LEFT; i <= CONTROL_RESET; i++)
  {
    SET_CONTROL_VISIBLE(i);
    CGUIControl* control = GetControl(i);
    if (control)
      control->DoProcess(currentTime, dirtyregions);
  }
  CServiceBroker::GetWinSystem()->GetGfxContext().RemoveTransform();
}

void CGUIWindowSettingsScreenCalibration::DoRender()
{
  // we set that we need scaling here to render so that anything else on screen scales correctly
  m_needsScaling = true;
  CGUIWindow::DoRender();
  m_needsScaling = false;
}
