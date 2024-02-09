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

namespace
{
constexpr int DEFAULT_CONTROL_ID = 0;
}

CGUIAction::CExecutableAction::CExecutableAction(const std::string& action) : m_action{action}
{
}

CGUIAction::CExecutableAction::CExecutableAction(const std::string& condition,
                                                 const std::string& action)
  : m_condition{condition}, m_action{action}
{
}

std::string CGUIAction::CExecutableAction::GetCondition() const
{
  return m_condition;
}

std::string CGUIAction::CExecutableAction::GetAction() const
{
  return m_action;
}

bool CGUIAction::CExecutableAction::HasCondition() const
{
  return !m_condition.empty();
}

void CGUIAction::CExecutableAction::SetAction(const std::string& action)
{
  m_action = action;
}

CGUIAction::CGUIAction(int controlID)
{
  SetNavigation(controlID);
}

bool CGUIAction::ExecuteActions() const
{
  return ExecuteActions(DEFAULT_CONTROL_ID, DEFAULT_CONTROL_ID);
}

bool CGUIAction::ExecuteActions(int controlID,
                                int parentID,
                                const std::shared_ptr<CGUIListItem>& item /* = NULL */) const
{
  if (m_actions.empty())
    return false;

  CGUIInfoManager& infoMgr = CServiceBroker::GetGUI()->GetInfoManager();
  // take a copy of actions that satisfy our conditions
  std::vector<std::string> actions;
  for (const auto &i : m_actions)
  {
    if (!i.HasCondition() || infoMgr.EvaluateBool(i.GetCondition(), 0, item))
    {
      if (!StringUtils::IsInteger(i.GetAction()))
        actions.emplace_back(i.GetAction());
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
    if (StringUtils::IsInteger(i.GetAction()))
    {
      if (!i.HasCondition() || infoMgr.EvaluateBool(i.GetCondition(), INFO::DEFAULT_CONTEXT))
        return std::stoi(i.GetAction());
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
    if (StringUtils::IsInteger(i.GetAction()) && !i.HasCondition())
    {
      i.SetAction(strId);
      return;
    }
  }
  m_actions.emplace_back(std::move(strId));
}

bool CGUIAction::HasConditionalActions() const
{
  for (const auto& i : m_actions)
  {
    if (i.HasCondition())
      return true;
  }
  return false;
}

bool CGUIAction::HasActionsMeetingCondition() const
{
  CGUIInfoManager& infoMgr = CServiceBroker::GetGUI()->GetInfoManager();
  for (const auto &i : m_actions)
  {
    if (!i.HasCondition() || infoMgr.EvaluateBool(i.GetCondition(), INFO::DEFAULT_CONTEXT))
      return true;
  }
  return false;
}

bool CGUIAction::HasAnyActions() const
{
  return m_actions.size() > 0;
}

size_t CGUIAction::GetActionCount() const
{
  return m_actions.size();
}

void CGUIAction::Append(const CExecutableAction& action)
{
  m_actions.emplace_back(action);
}

void CGUIAction::EnableSendThreadMessageMode()
{
  m_sendThreadMessages = true;
}

void CGUIAction::Reset()
{
  m_actions.clear();
}
