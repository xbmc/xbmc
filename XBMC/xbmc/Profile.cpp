/*
 *      Copyright (C) 2005-2007 Team XboxMediaCenter
 *      http://www.xboxmediacenter.com
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
 *  along with GNU Make; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "stdafx.h"
#include "Profile.h"
#include "utils/GUIInfoManager.h"

CProfile::CProfile(void)
{
  _bDatabases = true;
  _bCanWrite = true;
  _bSources = true;
  _bCanWriteSources = true;
  _bUseAvpackSettings = false;
}

CProfile::~CProfile(void)
{}

void CProfile::setDate()
{
  CStdString strDate = g_infoManager.GetDate(true);
  CStdString strTime = g_infoManager.GetTime();
  if (strDate.IsEmpty() || strTime.IsEmpty())
    g_settings.m_vecProfiles[g_settings.m_iLastLoadedProfileIndex].setDate("-");
  else
    g_settings.m_vecProfiles[g_settings.m_iLastLoadedProfileIndex].setDate(strDate+" - "+strTime);
}