/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "guilib/guiinfo/GUIInfoBool.h"

#include "GUIInfoManager.h"
#include "ServiceBroker.h"
#include "guilib/GUIComponent.h"

using namespace KODI::GUILIB::GUIINFO;

CGUIInfoBool::CGUIInfoBool(bool value)
{
  m_value = value;
}

CGUIInfoBool::~CGUIInfoBool() = default;

void CGUIInfoBool::Parse(const std::string &expression, int context)
{
  if (expression == "true")
    m_value = true;
  else if (expression == "false")
    m_value = false;
  else
  {
    m_info = CServiceBroker::GetGUI()->GetInfoManager().Register(expression, context);
    Update(context);
  }
}

void CGUIInfoBool::Update(int contextWindow, const CGUIListItem* item /*= nullptr*/)
{
  if (m_info)
    m_value = m_info->Get(contextWindow, item);
}
