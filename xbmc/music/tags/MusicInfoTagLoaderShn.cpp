/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "MusicInfoTagLoaderShn.h"
#include "MusicInfoTag.h"
#include "utils/log.h"

using namespace MUSIC_INFO;

CMusicInfoTagLoaderSHN::CMusicInfoTagLoaderSHN(void) = default;

CMusicInfoTagLoaderSHN::~CMusicInfoTagLoaderSHN() = default;

bool CMusicInfoTagLoaderSHN::Load(const std::string& strFileName, CMusicInfoTag& tag, EmbeddedArt *art)
{
  try
  {

    tag.SetURL(strFileName);
    tag.SetDuration((long)0); //! @todo Use libavformat to calculate duration.
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
