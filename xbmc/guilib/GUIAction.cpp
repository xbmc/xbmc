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

#include "GUIAction.h"
#include "utils/StringUtils.h"
#include "GUIWindowManager.h"
#include "GUIInfoManager.h"
#include "GUIComponent.h"
#include "ServiceBroker.h"

CGUIAction::CGUIAction(int controlID)
{
  SetNavigation(controlID);
}

bool CGUIAction::ExecuteActions(int controlID, int parentID, const CGUIListItemPtr &item /* = NULL */) const
{
  if (m_actions.empty())
    return false;

  CGUIInfoManager& infoMgr = CServiceBroker::GetGUI()->GetInfoManager();
  // take a copy of actions that satisfy our conditions
  std::vector<std::string> actions;
  for (const auto &i : m_actions)
  {
    if (i.condition.empty() || infoMgr.EvaluateBool(i.condition, 0, item))
    {
      if (!StringUtils::IsInteger(i.action))
        actions.emplace_back(i.action);
    }
  }
  // execute them
  bool retval = false;
  for (const auto &i : actions)
  {
    CGUIMessage msg(GUI_MSG_EXECUTE, controlID, parentID, 0, 0, item);
    msg.SetStringParam(i);
    if (m_sendThreadMessages)
      CServiceBroker::GetGUI()->GetWindowManager().SendThreadMessage(msg);
    else
      CServiceBroker::GetGUI()->GetWindowManager().SendMessage(msg);
    retval = true;
  }
  return retval;
}

int CGUIAction::GetNavigation() const
{
  CGUIInfoManager& infoMgr = CServiceBroker::GetGUI()->GetInfoManager();
  for (const auto &i : m_actions)
  {
    if (StringUtils::IsInteger(i.action))
    {
      if (i.condition.empty() || infoMgr.EvaluateBool(i.condition))
        return atoi(i.action.c_str());
    }
  }
  return 0;
}

void CGUIAction::SetNavigation(int id)
{
  if (id == 0)
    return;

  std::string strId = StringUtils::Format("%i", id);
  for (auto &i : m_actions)
  {
    if (StringUtils::IsInteger(i.action) && i.condition.empty())
    {
      i.action = std::move(strId);
      return;
    }
  }
  m_actions.emplace_back();
  m_actions.back().action = std::move(strId);
}

bool CGUIAction::HasActionsMeetingCondition() const
{
  CGUIInfoManager& infoMgr = CServiceBroker::GetGUI()->GetInfoManager();
  for (const auto &i : m_actions)
  {
    if (i.condition.empty() || infoMgr.EvaluateBool(i.condition))
      return true;
  }
  return false;
}
