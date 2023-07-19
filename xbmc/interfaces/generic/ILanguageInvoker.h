/*
 *  Copyright (C) 2013-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "addons/IAddon.h"

#include <memory>
#include <string>
#include <vector>

class CLanguageInvokerThread;
class ILanguageInvocationHandler;

typedef enum
{
  InvokerStateUninitialized,
  InvokerStateInitialized,
  InvokerStateRunning,
  InvokerStateStopping,
  InvokerStateScriptDone,
  InvokerStateExecutionDone,
  InvokerStateFailed
} InvokerState;

class ILanguageInvoker
{
public:
  explicit ILanguageInvoker(ILanguageInvocationHandler *invocationHandler);
  virtual ~ILanguageInvoker();

  virtual bool Execute(const std::string &script, const std::vector<std::string> &arguments = std::vector<std::string>());
  virtual bool Stop(bool abort = false);
  virtual bool IsStopping() const;

  void SetId(int id) { m_id = id; }
  int GetId() const { return m_id; }
  const ADDON::AddonPtr& GetAddon() const { return m_addon; }
  void SetAddon(const ADDON::AddonPtr &addon) { m_addon = addon; }
  InvokerState GetState() const { return m_state; }
  bool IsActive() const;
  bool IsRunning() const;
  void Reset() { m_state = InvokerStateUninitialized; }

protected:
  friend class CLanguageInvokerThread;

  /**
   * Called to notify the script is aborting.
   */
  virtual void AbortNotification();

  virtual bool execute(const std::string &script, const std::vector<std::string> &arguments) = 0;
  virtual bool stop(bool abort) = 0;

  virtual void pulseGlobalEvent();
  virtual bool onExecutionInitialized();
  virtual void onExecutionFailed();
  virtual void onExecutionDone();
  virtual void onExecutionFinalized();

  void setState(InvokerState state);

  ADDON::AddonPtr m_addon;

private:
  int m_id = -1;
  InvokerState m_state = InvokerStateUninitialized;
  ILanguageInvocationHandler *m_invocationHandler;
};

typedef std::shared_ptr<ILanguageInvoker> LanguageInvokerPtr;
