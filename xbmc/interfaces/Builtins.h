#pragma once

/*
 *      Copyright (C) 2005-2012 Team XBMC
 *      http://www.xbmc.org
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

#include "utils/StdString.h"


/* PLEX */
#include "Job.h"
#include "JobManager.h"


class ScriptJob : public CJob
{
  public:
    enum ScriptJobType
    {
      SCRIPT_JOB_APPLE_SCRIPT,
      SCRIPT_JOB_APPLE_SCRIPT_FILE,
    };

    static void DoScriptJob(ScriptJobType type, const CStdString& scriptData);

    ScriptJob(ScriptJobType type, const CStdString& scriptData) :
      CJob(), m_type(type), m_scriptData(scriptData)
    {}

    bool DoWork();

  private:
    CStdString m_scriptData;
    ScriptJobType m_type;
};

/* END PLEX */

class CBuiltins
{
public:
  static bool HasCommand(const CStdString& execString);
  static void GetHelp(CStdString &help);
  static int Execute(const CStdString& execString);
};

