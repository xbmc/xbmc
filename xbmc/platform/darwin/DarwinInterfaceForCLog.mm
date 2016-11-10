/*
 *      Copyright (C) 2016 Team XBMC
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
 
#import <Foundation/Foundation.h>

#include "DarwinInterfaceForCLog.h"

void CDarwinInterfaceForCLog::AddSinks(std::shared_ptr<spdlog::sinks::dist_sink_mt> distributionSink) const
{
  distributionSink->add_sink(std::make_shared<CDarwinInterfaceForCLog>());
}

void CDarwinInterfaceForCLog::log(const spdlog::details::log_msg& msg)
{
  NSLog(@"%s", msg.formatted.str().c_str());
}

void CDarwinInterfaceForCLog::flush()
{
  fflush(stderr);
}
