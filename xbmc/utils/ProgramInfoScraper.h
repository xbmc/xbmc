#pragma once

/*
 *      Copyright (C) 2005-2008 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "ProgramInfo.h"
#include "ScraperSettings.h"
#include "Thread.h"
#include "FileSystem/FileCurl.h"

namespace PROGRAM_GRABBER
{
class CProgramInfoScraper : public CThread
{
public:
  CProgramInfoScraper(const SScraperInfo& info);
  virtual ~CProgramInfoScraper(void);
  void FindProgramInfo(const CStdString& strProgram);
  void LoadProgramInfo(int iProgram);
  bool Completed();
  bool Successfull();
  void Cancel();
  bool IsCanceled();
  std::vector<CProgramInfo> GetPrograms();

protected:
  virtual void OnStartup();
  virtual void Process();
  void FindProgramInfo();
  void LoadProgramInfo();

  CStdString m_strProgram;
  std::vector<CProgramInfo> m_vecPrograms;
  int m_iProgram;
  bool m_bSuccessfull;
  bool m_bCanceled;
  SScraperInfo m_info;
  XFILE::CFileCurl m_http;
};
}
