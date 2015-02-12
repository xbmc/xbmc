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

#include <string>
#include "threads/Thread.h"
#include "filesystem/File.h"

class CAsyncFileCopy : public CThread, public XFILE::IFileCallback
{
public:
  CAsyncFileCopy();
  virtual ~CAsyncFileCopy();

  /// \brief  Main routine to copy files from one source to another.
  /// \return true if successful, and false if it failed or was cancelled.
  bool Copy(const std::string &from, const std::string &to, const std::string &heading);

  /// \brief callback from CFile::Copy()
  virtual bool OnFileCallback(void *pContext, int ipercent, float avgSpeed);

protected:
  virtual void Process();

private:
  /// volatile variables as we access these from both threads
  volatile int m_percent;      ///< current percentage (0..100)
  volatile float m_speed;      ///< current speed (in bytes per second)
  volatile bool m_cancelled;   ///< whether or not we cancelled the operation
  volatile bool m_running;     ///< whether or not the copy operation is still in progress
  
  bool m_succeeded;  ///< whether or not the copy operation was successful
  std::string m_from; ///< source URL to copy from
  std::string m_to;   ///< destination URL to copy to
  CEvent m_event;    ///< event to set to force an update
};
