/*
*      Copyright (C) 2014 Team XBMC
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

#if !defined(TARGET_WINDOWS) && !defined(TARGET_WIN10)
#error This file is for win32 platforms only
#endif  // !defined(TARGET_WINDOWS) && !defined(TARGET_WIN10)

#include <spdlog/sinks/msvc_sink.h>

#include "Win32InterfaceForCLog.h"
#include "platform/win32/WIN32Util.h"

const spdlog::filename_t CWin32InterfaceForCLog::GetLogFilename(const std::string& filename) const
{
  return CWIN32Util::ConvertPathToWin32Form(CWIN32Util::SmbToUnc(filename));
}

void CWin32InterfaceForCLog::AddSinks(std::shared_ptr<spdlog::sinks::dist_sink_mt> distributionSink) const
{
  distributionSink->add_sink(std::make_shared<spdlog::sinks::msvc_sink_mt>());
}
