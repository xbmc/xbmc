/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIAction.h"

#include "GUIComponent.h"
#include "GUIInfoManager.h"
#include "GUIWindowManager.h"
#include "ServiceBroker.h"
#include "utils/StringUtils.h"

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
      if (i.condition.empty() || infoMgr.EvaluateBool(i.condition, INFO::DEFAULT_CONTEXT))
        return atoi(i.action.c_str());
    }
  }
  return 0;
}

void CGUIAction::SetNavigation(int id)
{
  if (id == 0)
    return;

  std::string strId = std::to_string(id);
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
    if (i.condition.empty() || infoMgr.EvaluateBool(i.condition, INFO::DEFAULT_CONTEXT))
      return true;
  }
  return false;
}
