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
class CGUIListItem;

/**
 * Class containing vector of condition->(action/navigation route) and handling its execution.
 */
class CGUIAction
{
public:
  /**
   * Class which defines an executable action
   */
  class CExecutableAction
  {
  public:
    /**
    * Executable action constructor (without conditional execution)
    * @param action - The action to be executed
    */
    explicit CExecutableAction(const std::string& action);
    /**
    * Executable action constructor (providing the condition and the action)
    * @param condition - The condition that dictates the action execution
    * @param action - The actual action
    */
    CExecutableAction(const std::string& condition, const std::string& action);

    /**
    * Get the condition of this executable action (may be empty)
    * @return condition - The condition that dictates the action execution (or an empty string)
    */
    std::string GetCondition() const;

    /**
    * Checks if the executable action has any condition
    * @return true if the executable action has any condition, else false
    */
    bool HasCondition() const;

    /**
    * Get the action string of this executable action
    * @return action - The action string
    */
    std::string GetAction() const;

    /**
    * Sets/Replaces the action string of this executable action
    * @param action - The action string
    */
    void SetAction(const std::string& action);

  private:
    /**
    * Executable action default constructor
    */
    CExecutableAction() = delete;
    /* The condition that dictates the action execution */
    std::string m_condition;
    /* The actual action */
    std::string m_action;
  };

  CGUIAction() = default;
  explicit CGUIAction(int controlID);
  /**
   * Execute actions without specifying any target control or parent control. Action will use the default focused control.
   */
  bool ExecuteActions() const;
  /**
   * Execute actions (no navigation paths); if action is paired with condition - evaluate condition first
   */
  bool ExecuteActions(int controlID,
                      int parentID,
                      const std::shared_ptr<CGUIListItem>& item = nullptr) const;
  /**
   * Check if there are any conditional actions
  */
  bool HasConditionalActions() const;
  /**
   * Check if there is any action that meet its condition
   */
  bool HasActionsMeetingCondition() const;
  /**
   * Check if there is any action
   */
  bool HasAnyActions() const;
  /**
   * Get the total number of actions
  */
  size_t GetActionCount() const;
  /**
   * Get navigation route that meet its conditions first
   */
  int GetNavigation() const;
  /**
   * Set navigation route
   */
  void SetNavigation(int id);
  /**
   * Configure CGUIAction to send threaded messages
   */
  void EnableSendThreadMessageMode();
  /**
   * Add an executable action to the CGUIAction container
   */
  void Append(const CExecutableAction& action);
  /**
   * Prune any executable actions stored in the CGUIAction
   */
  void Reset();

private:
  std::vector<CExecutableAction> m_actions;
  bool m_sendThreadMessages = false;
};
