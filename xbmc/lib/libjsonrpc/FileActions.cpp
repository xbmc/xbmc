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

#include "FileActions.h"
#include "Settings.h"
#include "MediaSource.h"
#include "../FileSystem/Directory.h"
#include "FileItem.h"
#include "AdvancedSettings.h"
#include "../Util.h"

using namespace DIRECTORY;
using namespace Json;

JSON_STATUS CFileActions::GetRootDirectory(const CStdString &method, const Value& parameterObject, Value &result)
{
  CStdString type = parameterObject.get("type", "null").asString();
  type = type.ToLower();

  if (type.Equals("video") || type.Equals("music") || type.Equals("pictures") || type.Equals("files") || type.Equals("programs"))
  {
    VECSOURCES *sources = g_settings.GetSourcesFromType(type);
    if (sources)
    {
      unsigned int start = parameterObject.get("start", 0).asUInt();
      unsigned int end   = parameterObject.get("end", (unsigned int)sources->size()).asUInt();
      end = end > sources->size() ? sources->size() : end;

      result["start"] = start;
      result["end"]   = end;
      result["total"] = (unsigned int)sources->size();
      for (unsigned int i = start; i < end; i++)
      {
        CMediaSource &testShare = sources->at(i);
        result["shares"].append(testShare.strPath);
      }
    }

    return OK;
  }
  else
    return InvalidParams;
}

JSON_STATUS CFileActions::GetDirectory(const CStdString &method, const Value& parameterObject, Value &result)
{
  if (parameterObject.isMember("type") && parameterObject.isMember("directory"))
  {   
    CStdString type = parameterObject.get("type", "files").asString();
    type = type.ToLower();

    if (type.Equals("video") || type.Equals("music") || type.Equals("pictures") || type.Equals("files") || type.Equals("programs"))
    {
      CDirectory directory;
      CFileItemList items;
      CStdString strPath = parameterObject["directory"].asString();

      if (directory.GetDirectory(strPath, items))
      {
        CStdStringArray regexps;

        if (type.Equals("video"))
          regexps = g_advancedSettings.m_videoExcludeFromListingRegExps;
        else if (type.Equals("music"))
          regexps = g_advancedSettings.m_audioExcludeFromListingRegExps;
        else if (type.Equals("pictures"))
          regexps = g_advancedSettings.m_pictureExcludeFromListingRegExps;

        unsigned int start = parameterObject.get("start", 0).asUInt();
        unsigned int end   = parameterObject.get("end", (unsigned int)items.Size()).asUInt();
        end = end > (unsigned int)items.Size() ? (unsigned int)items.Size() : end;

        result["start"] = start;
        result["end"]   = end;
        result["total"] = (unsigned int)items.Size();
        for (unsigned int i = start; i < end; i++)
        {
          if (regexps.size() == 0 || !CUtil::ExcludeFileOrFolder(items[i]->m_strPath, regexps))
          {
            result["directories"].append(items[i]->m_strPath);
          }
        }
      }

      return OK;
    }
  }

  return InvalidParams;
}

JSON_STATUS CFileActions::Download(const CStdString &method, const Value& parameterObject, Value &result)
{
  if (!parameterObject.isMember("file"))
    return InvalidParams;

  CStdString str = "vfs/" + parameterObject["file"].asString();
  result["url"] = str.c_str();
  return OK;
}
