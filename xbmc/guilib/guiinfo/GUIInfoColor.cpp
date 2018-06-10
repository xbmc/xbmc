/*
 *      Copyright (C) 2005-2015 Team Kodi
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
 *  along with Kodi; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "guilib/guiinfo/GUIInfoColor.h"
#include "GUIInfoManager.h"
#include "addons/Skin.h"
#include "guilib/GUIColorManager.h"
#include "guilib/GUIComponent.h"
#include "utils/StringUtils.h"
#include "ServiceBroker.h"

using namespace KODI::GUILIB::GUIINFO;

bool CGUIInfoColor::Update()
{
  if (!m_info)
    return false; // no infolabel

  // Expand the infolabel, and then convert it to a color
  std::string infoLabel(CServiceBroker::GetGUI()->GetInfoManager().GetLabel(m_info));
  UTILS::Color color = !infoLabel.empty() ? g_colorManager.GetColor(infoLabel.c_str()) : 0;
  if (m_color != color)
  {
    m_color = color;
    return true;
  }
  else
    return false;
}

void CGUIInfoColor::Parse(const std::string &label, int context)
{
  if (label.empty())
    return;

  CGUIInfoManager& infoMgr = CServiceBroker::GetGUI()->GetInfoManager();

  // Check for the standard $INFO[] block layout, and strip it if present
  std::string label2 = label;
  if (StringUtils::StartsWithNoCase(label, "$var["))
  {
    label2 = label.substr(5, label.length() - 6);
    m_info = infoMgr.TranslateSkinVariableString(label2, context);
    if (!m_info)
      m_info = infoMgr.RegisterSkinVariableString(g_SkinInfo->CreateSkinVariable(label2, context));
    return;
  }

  if (StringUtils::StartsWithNoCase(label, "$info["))
    label2 = label.substr(6, label.length()-7);

  m_info = infoMgr.TranslateString(label2);
  if (!m_info)
    m_color = g_colorManager.GetColor(label);
}
