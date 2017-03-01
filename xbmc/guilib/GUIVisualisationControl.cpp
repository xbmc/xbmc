/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "GUIVisualisationControl.h"
#include "GUIWindowManager.h"
#include "GUIUserMessages.h"
#include "Application.h"
#include "addons/AddonManager.h"
#include "addons/AddonSystemSettings.h"
#include "addons/Visualisation.h"
#include "utils/log.h"
#include "input/Key.h"
#include "windowing/WindowingFactory.h"

using namespace ADDON;

#define LABEL_ROW1 10
#define LABEL_ROW2 11
#define LABEL_ROW3 12

CGUIVisualisationControl::CGUIVisualisationControl(int parentID, int controlID, float posX, float posY, float width, float height)
    : CGUIRenderingControl(parentID, controlID, posX, posY, width, height), m_bAttemptedLoad(false)
{
  ControlType = GUICONTROL_VISUALISATION;
}

CGUIVisualisationControl::CGUIVisualisationControl(const CGUIVisualisationControl &from)
  : CGUIRenderingControl(from), m_bAttemptedLoad(false), m_addon()
{
  ControlType = GUICONTROL_VISUALISATION;
}

bool CGUIVisualisationControl::OnMessage(CGUIMessage &message)
{
  switch (message.GetMessage())
  {
  case GUI_MSG_GET_VISUALISATION:
    message.SetPointer(m_addon.get());
    return m_addon != NULL;
  case GUI_MSG_VISUALISATION_RELOAD:
    FreeResources(true);
    return true;
  case GUI_MSG_PLAYBACK_STARTED:
    if (m_addon)
    {
      m_addon->UpdateTrack();
      return true;
    }
    break;
  }
  return CGUIRenderingControl::OnMessage(message);
}

bool CGUIVisualisationControl::OnAction(const CAction &action)
{
  if (!m_addon)
    return false;

  switch (action.GetID())
  {
  case ACTION_VIS_PRESET_NEXT:
    return m_addon->OnAction(VIS_ACTION_NEXT_PRESET);
  case ACTION_VIS_PRESET_PREV:
    return m_addon->OnAction(VIS_ACTION_PREV_PRESET);
  case ACTION_VIS_PRESET_RANDOM:
    return m_addon->OnAction(VIS_ACTION_RANDOM_PRESET);
  case ACTION_VIS_RATE_PRESET_PLUS:
    return m_addon->OnAction(VIS_ACTION_RATE_PRESET_PLUS);
  case ACTION_VIS_RATE_PRESET_MINUS:
    return m_addon->OnAction(VIS_ACTION_RATE_PRESET_MINUS);
  case ACTION_VIS_PRESET_LOCK:
    return m_addon->OnAction(VIS_ACTION_LOCK_PRESET);
  default:
    return CGUIRenderingControl::OnAction(action);
  }
}

void CGUIVisualisationControl::Process(unsigned int currentTime, CDirtyRegionList &dirtyregions)
{
  if (g_application.m_pPlayer->IsPlayingAudio())
  {
    if (m_bInvalidated)
      FreeResources(true);

    if (!m_addon && !m_bAttemptedLoad)
    {
      AddonPtr addon;
      if (ADDON::CAddonSystemSettings::GetInstance().GetActive(ADDON_VIZ, addon))
      {
        m_addon = std::dynamic_pointer_cast<CVisualisation>(addon);
        if (m_addon)
          if (!InitCallback(m_addon.get()))
            m_addon.reset();
      }

      m_bAttemptedLoad = true;
    }
  }
  CGUIRenderingControl::Process(currentTime, dirtyregions);
}

void CGUIVisualisationControl::FreeResources(bool immediately)
{
  m_bAttemptedLoad = false;
  // tell our app that we're going
  if (!m_addon)
    return;

  CGUIMessage msg(GUI_MSG_VISUALISATION_UNLOADING, m_controlID, 0);
  g_windowManager.SendMessage(msg);
  CLog::Log(LOGDEBUG, "FreeVisualisation() started");
  CGUIRenderingControl::FreeResources(immediately);
  m_addon.reset();
  CLog::Log(LOGDEBUG, "FreeVisualisation() done");
}

