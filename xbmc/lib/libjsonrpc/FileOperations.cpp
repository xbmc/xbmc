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

#include "FileOperations.h"
#include "Settings.h"
#include "MediaSource.h"
#include "../FileSystem/Directory.h"
#include "FileItem.h"
#include "AdvancedSettings.h"
#include "../Util.h"

using namespace XFILE;
using namespace Json;
using namespace JSONRPC;

JSON_STATUS CFileOperations::GetRootDirectory(const CStdString &method, ITransportLayer *transport, IClient *client, const Json::Value& parameterObject, Json::Value &result)
{
  const Value param = parameterObject.isObject() ? parameterObject : Value(objectValue);
  CStdString type = param.get("type", "null").asString();
  type = type.ToLower();

  if (type.Equals("video") || type.Equals("music") || type.Equals("pictures") || type.Equals("files") || type.Equals("programs"))
  {
    VECSOURCES *sources = g_settings.GetSourcesFromType(type);
    if (sources)
    {
      unsigned int start = param.get("start", 0).asUInt();
      unsigned int end   = param.get("end", (unsigned int)sources->size()).asUInt();
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

JSON_STATUS CFileOperations::GetDirectory(const CStdString &method, ITransportLayer *transport, IClient *client, const Json::Value& parameterObject, Json::Value &result)
{
  if (parameterObject.isObject() && parameterObject.isMember("type") && parameterObject.isMember("directory"))
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

        CFileItemList filtereditems;
        for (unsigned int i = 0; i < (unsigned int)items.Size(); i++)
        {
          if (regexps.size() == 0 || !CUtil::ExcludeFileOrFolder(items[i]->m_strPath, regexps))
            filtereditems.Add(items[i]);
        }  

        HandleFileItemList(NULL, "directories", filtereditems, parameterObject, result);

        return OK;
      }
    }
  }

  return InvalidParams;
}

JSON_STATUS CFileOperations::Download(const CStdString &method, ITransportLayer *transport, IClient *client, const Json::Value& parameterObject, Json::Value &result)
{
  if (!parameterObject.isString())
    return InvalidParams;

  return transport->Download(parameterObject.asString().c_str(), &result) ? OK : BadPermission;
}
