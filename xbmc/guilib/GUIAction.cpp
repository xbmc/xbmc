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
#include "GUIControl.h"
#include "GUIInfoManager.h"
#include "messaging/helpers/GUIMessageHelper.h"
#include "messaging/helpers/DialogHelper.h"

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
  using namespace KODI::MESSAGING::HELPERS;

  if (m_actions.empty()) return false;
  // take a copy of actions that satisfy our conditions
  std::vector<std::string> actions;
  for (ciActions it = m_actions.begin() ; it != m_actions.end() ; ++it)
  {
    if (it->condition.empty() || g_infoManager.EvaluateBool(it->condition, 0, item))
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
      PostGUIMessage(msg);
    else
      SendGUIMessage(msg);
    retval = true;
  }
  return retval;
}

int CGUIAction::GetNavigation() const
{
  for (ciActions it = m_actions.begin() ; it != m_actions.end() ; ++it)
  {
    if (StringUtils::IsInteger(it->action))
    {
      if (it->condition.empty() || g_infoManager.EvaluateBool(it->condition))
        return atoi(it->action.c_str());
    }
  }
  return 0;
}

void CGUIAction::SetNavigation(int id)
{
  if (id == 0) return;
  std::string strId = StringUtils::Format("%i", id);
  for (iActions it = m_actions.begin() ; it != m_actions.end() ; ++it)
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
  for (ciActions it = m_actions.begin() ; it != m_actions.end() ; ++it)
  {
    if (it->condition.empty() || g_infoManager.EvaluateBool(it->condition))
      return true;
  }
  return false;
}
