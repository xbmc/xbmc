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
#include "settings/Settings.h"
#include "MediaSource.h"
#include "filesystem/Directory.h"
#include "FileItem.h"
#include "settings/AdvancedSettings.h"
#include "Util.h"
#include "URL.h"

using namespace XFILE;
using namespace Json;
using namespace JSONRPC;

JSON_STATUS CFileOperations::GetRootDirectory(const CStdString &method, ITransportLayer *transport, IClient *client, const Value &parameterObject, Value &result)
{
  const Value param = parameterObject.isObject() ? parameterObject : Value(objectValue);
  CStdString media = param.get("media", "null").asString();
  media = media.ToLower();

  if (media.Equals("video") || media.Equals("music") || media.Equals("pictures") || media.Equals("files") || media.Equals("programs"))
  {
    VECSOURCES *sources = g_settings.GetSourcesFromType(media);
    if (sources)
    {
      CFileItemList items;
      for (unsigned int i = 0; i < (unsigned int)sources->size(); i++)
        items.Add(CFileItemPtr(new CFileItem(sources->at(i))));

      for (unsigned int i = 0; i < (unsigned int)items.Size(); i++)
      {
        if (items[i]->IsSmb())
        {
          CURL url(items[i]->m_strPath);
          items[i]->m_strPath = url.GetWithoutUserDetails();
        }
      }

      HandleFileItemList(NULL, true, "shares", items, parameterObject, result);
    }

    return OK;
  }
  else
    return InvalidParams;
}

JSON_STATUS CFileOperations::GetDirectory(const CStdString &method, ITransportLayer *transport, IClient *client, const Value &parameterObject, Value &result)
{
  if (parameterObject.isObject() && parameterObject.isMember("directory"))
  {
    CStdString media = "files";
    if (parameterObject.isMember("media"))
    {
      if (parameterObject["media"].isString())
        media = parameterObject["media"].asString();
      else
        return InvalidParams;
    }

    media = media.ToLower();

    if (media.Equals("video") || media.Equals("music") || media.Equals("pictures") || media.Equals("files") || media.Equals("programs"))
    {
      CDirectory directory;
      CFileItemList items;
      CStdString strPath = parameterObject["directory"].asString();

      CStdStringArray regexps;
      CStdString extensions = "";
      if (media.Equals("video"))
      {
        regexps = g_advancedSettings.m_videoExcludeFromListingRegExps;
        extensions = g_settings.m_videoExtensions;
      }
      else if (media.Equals("music"))
      {
        regexps = g_advancedSettings.m_audioExcludeFromListingRegExps;
        extensions = g_settings.m_musicExtensions;
      }
      else if (media.Equals("pictures"))
      {
        regexps = g_advancedSettings.m_pictureExcludeFromListingRegExps;
        extensions = g_settings.m_pictureExtensions;
      }

      if (directory.GetDirectory(strPath, items, extensions))
      {
        CFileItemList filteredDirectories, filteredFiles;
        for (unsigned int i = 0; i < (unsigned int)items.Size(); i++)
        {
          if (CUtil::ExcludeFileOrFolder(items[i]->m_strPath, regexps))
            continue;

          if (items[i]->IsSmb())
          {
            CURL url(items[i]->m_strPath);
            items[i]->m_strPath = url.GetWithoutUserDetails();
          }

          if (items[i]->m_bIsFolder)
            filteredDirectories.Add(items[i]);
          else
            filteredFiles.Add(items[i]);
        }

        HandleFileItemList(NULL, true, "directories", filteredDirectories, parameterObject, result);
        HandleFileItemList(NULL, true, "files", filteredFiles, parameterObject, result);

        return OK;
      }
    }
  }

  return InvalidParams;
}

JSON_STATUS CFileOperations::Download(const CStdString &method, ITransportLayer *transport, IClient *client, const Value &parameterObject, Value &result)
{
  if (!parameterObject.isString())
    return InvalidParams;

  return transport->Download(parameterObject.asString().c_str(), &result) ? OK : BadPermission;
}

bool CFileOperations::FillFileItemList(const Value &parameterObject, CFileItemList &list)
{
  if (parameterObject.isObject() && parameterObject.isMember("directory"))
  {
    CStdString media = "files";
    if (parameterObject.isMember("media"))
    {
      if (parameterObject["media"].isString())
        media = parameterObject["media"].asString();
      else
        return false;
    }

    media = media.ToLower();

    if (media.Equals("video") || media.Equals("music") || media.Equals("pictures") || media.Equals("files") || media.Equals("programs"))
    {
      CDirectory directory;
      CFileItemList items;
      CStdString strPath = parameterObject["directory"].asString();

      CStdStringArray regexps;
      CStdString extensions = "";
      if (media.Equals("video"))
      {
        regexps = g_advancedSettings.m_videoExcludeFromListingRegExps;
        extensions = g_settings.m_videoExtensions;
      }
      else if (media.Equals("music"))
      {
        regexps = g_advancedSettings.m_audioExcludeFromListingRegExps;
        extensions = g_settings.m_musicExtensions;
      }
      else if (media.Equals("pictures"))
      {
        regexps = g_advancedSettings.m_pictureExcludeFromListingRegExps;
        extensions = g_settings.m_pictureExtensions;
      }

      if (directory.GetDirectory(strPath, items, extensions))
      {
        CFileItemList filteredDirectories;
        for (unsigned int i = 0; i < (unsigned int)items.Size(); i++)
        {
          if (CUtil::ExcludeFileOrFolder(items[i]->m_strPath, regexps))
            continue;

          if (items[i]->m_bIsFolder)
            filteredDirectories.Add(items[i]);
          else
            list.Add(items[i]);
        }

        if (parameterObject.isMember("recursive") && parameterObject["recursive"].isBool())
        {
          for (int i = 0; i < filteredDirectories.Size(); i++)
          {
            Value val = parameterObject;
            val["directory"] = filteredDirectories[i]->m_strPath;
            FillFileItemList(val, list);
          }
        }

        return true;
      }
    }
  }

  return false;
}
