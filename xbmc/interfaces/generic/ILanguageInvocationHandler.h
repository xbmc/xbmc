#pragma once
/*
 *      Copyright (C) 2015 Team Kodi
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

class ILanguageInvoker;

class ILanguageInvocationHandler
{
public:
  ILanguageInvocationHandler() { }
  virtual ~ILanguageInvocationHandler() { }

  virtual bool Initialize() { return true; }
  virtual void Process() { }
  virtual void PulseGlobalEvent() { }
  virtual void Uninitialize() { }

  virtual bool OnScriptInitialized(ILanguageInvoker *invoker) { return true; }
  virtual void OnScriptStarted(ILanguageInvoker *invoker) { }
  virtual void OnScriptAbortRequested(ILanguageInvoker *invoker) { }
  virtual void OnScriptEnded(ILanguageInvoker *invoker) { }
  virtual void OnScriptFinalized(ILanguageInvoker *invoker) { }

  virtual ILanguageInvoker* CreateInvoker() = 0;
};
