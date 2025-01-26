/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PVRDirectory.h"

#include "pvr/filesystem/PVRGUIDirectory.h"

using namespace XFILE;
using namespace PVR;

CPVRDirectory::CPVRDirectory() = default;

CPVRDirectory::~CPVRDirectory() = default;

bool CPVRDirectory::Exists(const CURL& url)
{
  const CPVRGUIDirectory dir(url);
  return dir.Exists();
}

bool CPVRDirectory::Resolve(CFileItem& item) const
{
  return CPVRGUIDirectory::Resolve(item);
}

bool CPVRDirectory::GetDirectory(const CURL& url, CFileItemList &items)
{
  const CPVRGUIDirectory dir(url);
  return dir.GetDirectory(items);
}

bool CPVRDirectory::SupportsWriteFileOperations(const std::string& strPath)
{
  const CPVRGUIDirectory dir(strPath);
  return dir.SupportsWriteFileOperations();
}

bool CPVRDirectory::HasTVRecordings()
{
  return CPVRGUIDirectory::HasTVRecordings();
}

bool CPVRDirectory::HasDeletedTVRecordings()
{
  return CPVRGUIDirectory::HasDeletedTVRecordings();
}

bool CPVRDirectory::HasRadioRecordings()
{
  return CPVRGUIDirectory::HasRadioRecordings();
}

bool CPVRDirectory::HasDeletedRadioRecordings()
{
  return CPVRGUIDirectory::HasDeletedRadioRecordings();
}
