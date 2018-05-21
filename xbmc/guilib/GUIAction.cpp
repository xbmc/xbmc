/*
 *      Copyright (C) 2005-present Team Kodi
 *      http://kodi.tv
 *
 *  Kodi is free software: you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Kodi is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Kodi. If not, see <https://www.gnu.org/licenses/>.
 *
 */

#include "GUIAction.h"
#include "utils/StringUtils.h"
#include "GUIWindowManager.h"
#include "GUIControl.h"
#include "GUIInfoManager.h"
#include "GUIComponent.h"
#include "ServiceBroker.h"

CGUIAction::CGUIAction()
{
  m_sendThreadMessages = false;
}

CGUIAction::CGUIAction(int controlID)
{
  m_sendThreadMessages = false;
  SetNavigation(controlID);
}

bool CGUIAction::ExecuteActions(int controlID, int parentID, const CGUIListItemPtr &item /* = NULL */) const
{
  if (m_actions.empty())
    return false;

  CGUIInfoManager& infoMgr = CServiceBroker::GetGUI()->GetInfoManager();
  // take a copy of actions that satisfy our conditions
  std::vector<std::string> actions;
  for (ciActions it = m_actions.begin(); it != m_actions.end(); ++it)
  {
    if (it->condition.empty() || infoMgr.EvaluateBool(it->condition, 0, item))
    {
      if (!StringUtils::IsInteger(it->action))
        actions.push_back(it->action);
    }
  }
  // execute them
  bool retval = false;
  for (std::vector<std::string>::iterator i = actions.begin(); i != actions.end(); ++i)
  {
    CGUIMessage msg(GUI_MSG_EXECUTE, controlID, parentID, 0, 0, item);
    msg.SetStringParam(*i);
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
  for (ciActions it = m_actions.begin(); it != m_actions.end(); ++it)
  {
    if (StringUtils::IsInteger(it->action))
    {
      if (it->condition.empty() || infoMgr.EvaluateBool(it->condition))
        return atoi(it->action.c_str());
    }
  }
  return 0;
}

void CGUIAction::SetNavigation(int id)
{
  if (id == 0)
    return;

  std::string strId = StringUtils::Format("%i", id);
  for (iActions it = m_actions.begin(); it != m_actions.end(); ++it)
  {
    if (StringUtils::IsInteger(it->action) && it->condition.empty())
    {
      it->action = strId;
      return;
    }
  }
  cond_action_pair pair;
  pair.action = strId;
  m_actions.push_back(pair);
}

bool CGUIAction::HasActionsMeetingCondition() const
{
  CGUIInfoManager& infoMgr = CServiceBroker::GetGUI()->GetInfoManager();
  for (ciActions it = m_actions.begin(); it != m_actions.end(); ++it)
  {
    if (it->condition.empty() || infoMgr.EvaluateBool(it->condition))
      return true;
  }
  return false;
}
