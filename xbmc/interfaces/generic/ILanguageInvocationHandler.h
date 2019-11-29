/*
 *  Copyright (C) 2013-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

class ILanguageInvoker;

class ILanguageInvocationHandler
{
public:
  ILanguageInvocationHandler() = default;
  virtual ~ILanguageInvocationHandler() = default;

  virtual bool Initialize() { return true; }
  virtual void Process() { }
  virtual void PulseGlobalEvent() { }
  virtual void Uninitialize() { }

  virtual bool OnScriptInitialized(ILanguageInvoker *invoker) { return true; }
  virtual void OnScriptStarted(ILanguageInvoker *invoker) { }
  virtual void OnScriptAbortRequested(ILanguageInvoker *invoker) { }
  virtual void OnExecutionEnded(ILanguageInvoker* invoker) {}
  virtual void OnScriptFinalized(ILanguageInvoker *invoker) { }

  virtual ILanguageInvoker* CreateInvoker() = 0;
};
