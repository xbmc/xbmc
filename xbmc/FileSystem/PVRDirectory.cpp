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
#include "LocalizeStrings.h"

using namespace std;
using namespace DIRECTORY;

CPVRDirectory::CPVRDirectory()
{
}

CPVRDirectory::~CPVRDirectory()
{
}

bool CPVRDirectory::GetDirectory(const CStdString& strPath, CFileItemList &items)
{
  CURL url(strPath);
  CStdString base(strPath);
  CUtil::RemoveSlashAtEnd(base);
  CLog::Log(LOGDEBUG, "CPVRDirectory::GetDirectory(%s)", base.c_str());

  if(url.GetFileName().IsEmpty())
  {
    CFileItemPtr item;

    item.reset(new CFileItem(base + "/channelstv/", true));
    item->SetLabel(g_localizeStrings.Get(18051));
    item->SetLabelPreformated(true);
    items.Add(item);

    item.reset(new CFileItem(base + "/channelsradio/", true));
    item->SetLabel(g_localizeStrings.Get(18052));
    item->SetLabelPreformated(true);
    items.Add(item);

    item.reset(new CFileItem(base + "/recordings/", true));
    item->SetLabel(g_localizeStrings.Get(18066));
    item->SetLabelPreformated(true);
    items.Add(item);

    item.reset(new CFileItem(base + "/timers/", true));
    item->SetLabel(g_localizeStrings.Get(18054));
    item->SetLabelPreformated(true);
    items.Add(item);

    item.reset(new CFileItem(base + "/guide/", true));
    item->SetLabel(g_localizeStrings.Get(18050));
    item->SetLabelPreformated(true);
    items.Add(item);

    return true;
  }

  return false;
}

