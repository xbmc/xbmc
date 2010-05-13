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

#include "GUIVisualisationControl.h"
#include "GUIWindowManager.h"
#include "GUIUserMessages.h"
#include "Application.h"
#include "addons/AddonManager.h"
#include "utils/log.h"

using namespace std;
using namespace ADDON;

#define LABEL_ROW1 10
#define LABEL_ROW2 11
#define LABEL_ROW3 12

CGUIVisualisationControl::CGUIVisualisationControl(int parentID, int controlID, float posX, float posY, float width, float height)
    : CGUIRenderingControl(parentID, controlID, posX, posY, width, height)
{
  ControlType = GUICONTROL_VISUALISATION;
}

CGUIVisualisationControl::CGUIVisualisationControl(const CGUIVisualisationControl &from)
: CGUIRenderingControl(from)
{
  ControlType = GUICONTROL_VISUALISATION;
}

void CGUIVisualisationControl::Render()
{
  if (!m_addon && g_application.IsPlayingAudio())
  {
    AddonPtr viz;
    if (ADDON::CAddonMgr::Get().GetDefault(ADDON_VIZ, viz))
      LoadAddon(viz);
  }
  else
    CGUIRenderingControl::Render();
}

void CGUIVisualisationControl::FreeResources(bool immediately)
{
  // tell our app that we're going
  if (!m_addon)
    return;

  CGUIMessage msg(GUI_MSG_VISUALISATION_UNLOADING, m_controlID, 0);
  g_windowManager.SendMessage(msg);
  CLog::Log(LOGDEBUG, "FreeVisualisation() started");
  CGUIRenderingControl::FreeResources(immediately);
  CLog::Log(LOGDEBUG, "FreeVisualisation() done");
}

