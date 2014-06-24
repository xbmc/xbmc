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

#include "MusicInfoTagLoaderShn.h"
#include "MusicInfoTag.h"
#include "utils/log.h"

using namespace MUSIC_INFO;

CMusicInfoTagLoaderSHN::CMusicInfoTagLoaderSHN(void)
{}

CMusicInfoTagLoaderSHN::~CMusicInfoTagLoaderSHN()
{}

bool CMusicInfoTagLoaderSHN::Load(const std::string& strFileName, CMusicInfoTag& tag, EmbeddedArt *art)
{
  try
  {

    tag.SetURL(strFileName);
    tag.SetDuration((long)0); //TODO: Use libavformat to calculate duration.
    tag.SetLoaded(false);

    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "Tag loader shn: exception in file %s", strFileName.c_str());
  }

  tag.SetLoaded(false);
  return false;
}
