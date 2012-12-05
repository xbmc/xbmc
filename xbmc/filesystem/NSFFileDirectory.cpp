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

#include "NSFFileDirectory.h"
#include "music/tags/MusicInfoTagLoaderNSF.h"
#include "music/tags/MusicInfoTag.h"

using namespace MUSIC_INFO;
using namespace XFILE;

CNSFFileDirectory::CNSFFileDirectory(void)
{
  m_strExt = "nsfstream";
}

CNSFFileDirectory::~CNSFFileDirectory(void)
{
}

int CNSFFileDirectory::GetTrackCount(const CStdString& strPath)
{
  CMusicInfoTagLoaderNSF nsf;
  nsf.Load(strPath,m_tag);
  m_tag.SetDuration(4*60); // 4 mins

  return nsf.GetStreamCount(strPath);
}
