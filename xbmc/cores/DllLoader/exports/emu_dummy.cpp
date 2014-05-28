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

#ifndef EXPORTS_EMU_DUMMY_H_INCLUDED
#define EXPORTS_EMU_DUMMY_H_INCLUDED
#include "emu_dummy.h"
#endif

#ifndef EXPORTS_UTILS_LOG_H_INCLUDED
#define EXPORTS_UTILS_LOG_H_INCLUDED
#include "utils/log.h"
#endif


extern "C" void not_implement( const char* debuginfo)
{
  if (debuginfo)
  {
    CLog::Log(LOGDEBUG, "%s", (char*)debuginfo);
  }
}

