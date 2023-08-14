/*
 *  Copyright (C) 2023 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DVDDirectory.h"

#include "FileItem.h"
#include "ServiceBroker.h"
#include "URL.h"
#include "storage/MediaManager.h"

using namespace XFILE;

bool CDVDDirectory::Resolve(CFileItem& item) const
{
  const CURL url{item.GetDynPath()};
  if (url.GetProtocol() != "dvd")
  {
    return false;
  }

  item.SetDynPath(CServiceBroker::GetMediaManager().TranslateDevicePath(""));
  return true;
}
