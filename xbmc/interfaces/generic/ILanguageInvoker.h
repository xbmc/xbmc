#pragma once
/*
 *      Copyright (C) 2013 Team XBMC
 *      http://xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include <memory>
#include <string>
#include <vector>

#include "addons/IAddon.h"

class CLanguageInvokerThread;
class ILanguageInvocationHandler;

typedef enum {
  InvokerStateUninitialized,
  InvokerStateInitialized,
  InvokerStateRunning,
  InvokerStateStopping,
  InvokerStateDone,
  InvokerStateFailed
} InvokerState;

class ILanguageInvoker
{
public:
  ILanguageInvoker(ILanguageInvocationHandler *invocationHandler);
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

protected:
  friend class CLanguageInvokerThread;

  virtual bool execute(const std::string &script, const std::vector<std::string> &arguments) = 0;
  virtual bool stop(bool abort) = 0;

  virtual void pulseGlobalEvent();
  virtual bool onExecutionInitialized();
  virtual void onAbortRequested();
  virtual void onExecutionFailed();
  virtual void onExecutionDone();
  virtual void onExecutionFinalized();

  void setState(InvokerState state);

  ADDON::AddonPtr m_addon;

private:
  int m_id;
  InvokerState m_state;
  ILanguageInvocationHandler *m_invocationHandler;
};

typedef std::shared_ptr<ILanguageInvoker> LanguageInvokerPtr;
