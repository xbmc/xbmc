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
  CStdString type = parameterObject.get("type", "files").asString();
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
  else
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
