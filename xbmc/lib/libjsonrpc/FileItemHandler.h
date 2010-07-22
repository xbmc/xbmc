#pragma once
/*
 *      Copyright (C) 2005-2010 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "StdString.h"
#include "JSONRPC.h"
#include "JSONUtils.h"
#include "../FileItem.h"
#include "../VideoInfoTag.h"
#include "../MusicInfoTag.h"

namespace JSONRPC
{
  class CFileItemHandler : public CJSONUtils
  {
  protected:
    static void FillVideoDetails(const CVideoInfoTag *videoInfo, const CStdString &field, Json::Value &result);
    static void FillMusicDetails(const MUSIC_INFO::CMusicInfoTag *musicInfo, const CStdString &field, Json::Value &result);
    static void HandleFileItemList(const char *id, bool allowFile, const char *resultname, CFileItemList &items, const Json::Value &parameterObject, Json::Value &result);

    static bool FillFileItemList(const Json::Value &parameterObject, CFileItemList &list);
  private:
    static bool ParseSortMethods(const CStdString &method, const bool &ignorethe, const CStdString &order, SORT_METHOD &sortmethod, SORT_ORDER &sortorder);
    static void Sort(CFileItemList &items, const Json::Value& parameterObject);
  };
}
