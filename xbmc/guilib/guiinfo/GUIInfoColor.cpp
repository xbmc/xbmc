/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "guilib/guiinfo/GUIInfoColor.h"

#include "FileItem.h"
#include "GUIInfoManager.h"
#include "ServiceBroker.h"
#include "addons/Skin.h"
#include "guilib/GUIColorManager.h"
#include "guilib/GUIComponent.h"
#include "utils/StringUtils.h"

using namespace KODI::GUILIB::GUIINFO;

bool CGUIInfoColor::Update(const CGUIListItem* item /* = nullptr */)
{
  if (!m_info)
    return false; // no infolabel

  // Expand the infolabel, and then convert it to a color
  std::string infoLabel;
  if (item && item->IsFileItem())
    infoLabel = CServiceBroker::GetGUI()->GetInfoManager().GetItemLabel(
        static_cast<const CFileItem*>(item), 0, m_info);
  else
    infoLabel = CServiceBroker::GetGUI()->GetInfoManager().GetLabel(m_info, INFO::DEFAULT_CONTEXT);

  UTILS::COLOR::Color color =
      !infoLabel.empty() ? CServiceBroker::GetGUI()->GetColorManager().GetColor(infoLabel) : 0;
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
    m_color = CServiceBroker::GetGUI()->GetColorManager().GetColor(label);
}
