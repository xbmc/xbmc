/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <memory>
#include <string>
#include <vector>

class CGUIControl;
class CGUIListItem; typedef std::shared_ptr<CGUIListItem> CGUIListItemPtr;

/**
 * Class containing vector of condition->(action/navigation route) pairs and handling its execution.
 */
class CGUIAction
{
public:
  CGUIAction() = default;
  explicit CGUIAction(int controlID);

  /**
   * Execute actions (no navigation paths), if action is paired with condition - evaluate condition first
   */
  bool ExecuteActions(int controlID, int parentID, const CGUIListItemPtr &item = NULL) const;
  /**
   * Check if there is any action that meet its condition
   */
  bool HasActionsMeetingCondition() const;
  /**
   * Check if there is any action
   */
  bool HasAnyActions() const { return m_actions.size() > 0; }
  /**
   * Get navigation route that meet its conditions first
   */
  int GetNavigation() const;
  /**
   * Set navigation route
   */
  void SetNavigation(int id);
private:
  struct cond_action_pair
  {
    std::string condition;
    std::string action;
  };

  std::vector<cond_action_pair> m_actions;
  bool m_sendThreadMessages = false;

  typedef std::vector<cond_action_pair>::const_iterator ciActions;
  typedef std::vector<cond_action_pair>::iterator iActions;
  friend class CGUIControlFactory; // no need for setters / adders
};
