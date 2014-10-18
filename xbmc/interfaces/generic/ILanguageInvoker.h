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

#include <string>
#include <vector>

#include "ILanguageInvocationHandler.h"
#include "addons/IAddon.h"

class CFileItem;
typedef boost::shared_ptr<CFileItem> CFileItemPtr;

class CLanguageInvokerThread;

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
  ILanguageInvoker(ILanguageInvocationHandler *invocationHandler)
    : m_id(-1), m_state(InvokerStateUninitialized),
      m_invocationHandler(invocationHandler)
  { }
  virtual ~ILanguageInvoker() { }

  virtual bool Execute(const std::string &script, const std::vector<std::string> &arguments = std::vector<std::string>(), const CFileItemPtr item = CFileItemPtr())
  {
    if (m_invocationHandler)
      m_invocationHandler->OnScriptStarted(this);

    return execute(script, arguments, item);
  }
  virtual bool Stop(bool abort = false) { return stop(abort); }

  void SetId(int id) { m_id = id; }
  int GetId() const { return m_id; }
  void SetAddon(const ADDON::AddonPtr &addon) { m_addon = addon; }
  InvokerState GetState() const { return m_state; }
  bool IsActive() const { return GetState() > InvokerStateUninitialized && GetState() < InvokerStateDone; }
  bool IsRunning() const { return GetState() == InvokerStateRunning; }
  virtual bool IsStopping() const { return GetState() == InvokerStateStopping; }

protected:
  friend class CLanguageInvokerThread;

  virtual bool execute(const std::string &script, const std::vector<std::string> &arguments, const CFileItemPtr item) = 0;
  virtual bool stop(bool abort) = 0;

  virtual void onExecutionFailed()
  {
    if (m_invocationHandler)
      m_invocationHandler->OnScriptEnded(this);
  }
  virtual void onExecutionDone()
  {
    if (m_invocationHandler)
      m_invocationHandler->OnScriptEnded(this);
  }

  void setState(InvokerState state)
  {
    if (state <= m_state)
      return;

    m_state = state;
  }

  ADDON::AddonPtr m_addon;

private:
  int m_id;
  InvokerState m_state;
  ILanguageInvocationHandler *m_invocationHandler;
};
