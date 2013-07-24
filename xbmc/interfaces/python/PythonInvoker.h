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

#include "interfaces/generic/ILanguageInvoker.h"
#include "threads/CriticalSection.h"
#include "threads/Event.h"

class CPythonInvoker : public ILanguageInvoker
{
public:
  CPythonInvoker(ILanguageInvocationHandler *invocationHandler);
  virtual ~CPythonInvoker();

  virtual bool Execute(const std::string &script, const std::vector<std::string> &arguments = std::vector<std::string>());

  virtual bool IsStopping() const { return m_stop || ILanguageInvoker::IsStopping(); }

protected:
  virtual bool execute(const std::string &script, const std::vector<std::string> &arguments);
  virtual bool stop(bool abort);

  virtual void onError();

private:
  void addPath(const std::string path);

  char *m_source;
  unsigned int  m_argc;
  char **m_argv;
  std::string m_pythonPath;
  void *m_threadState;
  bool m_stop;
  CEvent m_stoppedEvent;
  CCriticalSection m_critical;
};
