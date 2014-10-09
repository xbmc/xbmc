/* 
 * $Id: BaseSub.cpp 1785 2010-04-09 14:12:59Z xhmikosr $
 *
 * (C) 2006-2010 see AUTHORS
 *
 * This file is part of mplayerc.
 *
 * Mplayerc is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * Mplayerc is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "stdafx.h"
#include "BaseSub.h"

CStdStringW ReftimeToString(const REFERENCE_TIME& rtVal)
{
  CStdStringW strTemp;
  
  LONGLONG  llTotalMs  = (rtVal / 10000);
  int       lHour      = (int)(llTotalMs  / (1000*60*60));
  int       lMinute    = (llTotalMs / (1000*60)) % 60;
  int       lSecond    = (llTotalMs /  1000) % 60;
  int       lMillisec  = llTotalMs  %  1000;

  strTemp.Format (_T("%02d:%02d:%02d,%03d"), lHour, lMinute, lSecond, lMillisec);
  return strTemp;
}

CBaseSub::CBaseSub(SUBTITLE_TYPE nType)
  : m_nType(nType)
{
}

CBaseSub::~CBaseSub()
{
}