#pragma once
/*
 *      Copyright (C) 2005-2013 Team XBMC
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
#ifndef STORAGE_SYSTEM_H_INCLUDED
#define STORAGE_SYSTEM_H_INCLUDED
#include "system.h"
#endif

#ifndef STORAGE_UTILS_STDSTRING_H_INCLUDED
#define STORAGE_UTILS_STDSTRING_H_INCLUDED
#include "utils/StdString.h"
#endif

#ifndef STORAGE_UTILS_JOB_H_INCLUDED
#define STORAGE_UTILS_JOB_H_INCLUDED
#include "utils/Job.h"
#endif


class CAutorunMediaJob : public CJob
{
public:
  CAutorunMediaJob(const CStdString &label, const CStdString &path);

  virtual bool DoWork();
private:
  const char *GetWindowString(int selection);

  CStdString m_path, m_label;
};
