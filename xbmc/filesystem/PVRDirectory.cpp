/*
 *      Copyright (C) 2005-2009 Team XBMC
 *      http://www.xbmc.org
 *
 * This Program is free software; you can redistribute it and/or modify
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "PVRDirectory.h"
#include "FileItem.h"
#include "Util.h"
#include "URL.h"
#include "utils/log.h"
#include "guilib/LocalizeStrings.h"

#include "pvr/PVRManager.h"
#include "pvr/PVRChannelGroup.h"
#include "pvr/PVRRecordings.h"
#include "pvr/PVRTimers.h"

using namespace std;
using namespace XFILE;

CPVRDirectory::CPVRDirectory()
{
}

CPVRDirectory::~CPVRDirectory()
{
}

bool CPVRDirectory::GetDirectory(const CStdString& strPath, CFileItemList &items)
{
  CStdString base(strPath);
  URIUtils::RemoveSlashAtEnd(base);

  CURL url(strPath);
  CStdString fileName = url.GetFileName();
  URIUtils::RemoveSlashAtEnd(fileName);
  CLog::Log(LOGDEBUG, "CPVRDirectory::GetDirectory(%s)", base.c_str());

  if (fileName == "")
  {
    CFileItemPtr item;

    item.reset(new CFileItem(base + "/channels/", true));
    item->SetLabel(g_localizeStrings.Get(19019));
    item->SetLabelPreformated(true);
    items.Add(item);

    item.reset(new CFileItem(base + "/recordings/", true));
    item->SetLabel(g_localizeStrings.Get(19017));
    item->SetLabelPreformated(true);
    items.Add(item);

    item.reset(new CFileItem(base + "/timers/", true));
    item->SetLabel(g_localizeStrings.Get(19040));
    item->SetLabelPreformated(true);
    items.Add(item);

    item.reset(new CFileItem(base + "/guide/", true));
    item->SetLabel(g_localizeStrings.Get(19029));
    item->SetLabelPreformated(true);
    items.Add(item);

    // Sort by name only. Labels are preformated.
    items.AddSortMethod(SORT_METHOD_LABEL, 551 /* Name */, LABEL_MASKS("%L", "", "%L", ""));

    return true;
  }
  else if (fileName.Left(10) == "recordings")
  {
    return PVRRecordings.GetDirectory(strPath, items) > 0;
  }
  else if (fileName.Left(8) == "channels")
  {
    return CPVRChannelGroup::GetDirectory(strPath, items) > 0;
  }
  else if (fileName.Left(6) == "timers")
  {
    return g_PVRTimers.GetDirectory(strPath, items) > 0;
  }

  return false;
}

bool CPVRDirectory::SupportsFileOperations(const CStdString& strPath)
{
  CURL url(strPath);
  CStdString filename = url.GetFileName();
  URIUtils::RemoveSlashAtEnd(filename);

  if (filename.Left(11) == "recordings/" && filename.Right(4) == ".pvr")
     return true;

  return false;
}

bool CPVRDirectory::IsLiveTV(const CStdString& strPath)
{
  CURL url(strPath);
  return url.GetFileName().Left(9) == "channelstv/";
}

bool CPVRDirectory::HasRecordings()
{
  return PVRRecordings.GetNumRecordings() > 0;
}
