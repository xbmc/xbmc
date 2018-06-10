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

#include "guilib/guiinfo/GUIInfoBool.h"
#include "guilib/GUIComponent.h"
#include "GUIInfoManager.h"
#include "ServiceBroker.h"

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
    Update();
  }
}

void CGUIInfoBool::Update(const CGUIListItem *item /*= NULL*/)
{
  if (m_info)
    m_value = m_info->Get(item);
}
