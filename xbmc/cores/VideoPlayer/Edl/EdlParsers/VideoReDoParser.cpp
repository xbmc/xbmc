/*
 *  Copyright (C) 2005-2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "VideoReDoParser.h"

#include "FileItem.h"
#include "filesystem/File.h"
#include "utils/URIUtils.h"
#include "utils/log.h"

namespace
{
constexpr const char* VIDEOREDO_HEADER = "<Version>2";
constexpr const char* VIDEOREDO_TAG_CUT = "<Cut>";
constexpr const char* VIDEOREDO_TAG_SCENE = "<SceneMarker ";
} // namespace

using namespace EDL;
using namespace XFILE;

std::string CVideoReDoParser::GetEdlFilePath(const CFileItem& item) const
{
  return URIUtils::ReplaceExtension(item.GetDynPath(), ".Vprj");
}

CEdlParserResult CVideoReDoParser::Parse(const CFileItem& item, float fps)
{
  /*
   * VideoReDo file is strange. Tags are XML like, but it isn't an XML file.
   *
   * http://www.videoredo.com/
   */

  CEdlParserResult result;

  const std::string videoReDoFilename = GetEdlFilePath(item);

  CFile videoReDoFile;
  if (!videoReDoFile.Open(videoReDoFilename))
  {
    CLog::LogF(LOGERROR, "Could not open VideoReDo file: {}", CURL::GetRedacted(videoReDoFilename));
    return result;
  }

  std::string line;
  line.reserve(1024);
  if (!videoReDoFile.ReadLine(line) || !line.starts_with(VIDEOREDO_HEADER))
  {
    CLog::LogF(LOGERROR,
               "Invalid VideoReDo file: {}. Error reading line 1 - expected {}. Only version "
               "2 files are supported.",
               CURL::GetRedacted(videoReDoFilename), VIDEOREDO_HEADER);
    videoReDoFile.Close();
    return result;
  }

  int iLine = 1;
  bool bValid = true;
  while (bValid && videoReDoFile.ReadLine(line))
  {
    iLine++;
    if (line.starts_with(VIDEOREDO_TAG_CUT)) // Found the <Cut> tag
    {
      /*
       * double is used as 32 bit float would overflow.
       */
      double dStart, dEnd;
      if (sscanf(line.c_str() + strlen(VIDEOREDO_TAG_CUT), "%lf:%lf", &dStart, &dEnd) == 2)
      {
        /*
         *  Times need adjusting by 1/10,000 to get ms.
         */
        Edit edit;
        edit.start = std::chrono::milliseconds(std::lround(dStart / 10000));
        edit.end = std::chrono::milliseconds(std::lround(dEnd / 10000));
        edit.action = Action::CUT;
        result.AddEdit(edit, EdlSourceLocation{videoReDoFilename, iLine});
      }
      else
        bValid = false;
    }
    else if (line.starts_with(VIDEOREDO_TAG_SCENE)) // Found the <SceneMarker > tag
    {
      int iScene;
      double dSceneMarker;
      if (sscanf(line.c_str() + strlen(VIDEOREDO_TAG_SCENE), " %i>%lf", &iScene, &dSceneMarker) ==
          2)
        result.AddSceneMarker(std::chrono::milliseconds(std::lround(dSceneMarker / 10000)),
                              EdlSourceLocation{videoReDoFilename, iLine});
      else
        bValid = false;
    }
    /*
     * Ignore any other tags.
     */
  }
  videoReDoFile.Close();

  if (!bValid)
  {
    CLog::LogF(LOGERROR,
               "Invalid VideoReDo file: {}. Error in line {}. Clearing any valid edits or "
               "scenes found.",
               CURL::GetRedacted(videoReDoFilename), iLine);
    return {};
  }
  else if (!result.IsEmpty())
  {
    CLog::LogF(LOGDEBUG, "Read {} edits and {} scene markers in VideoReDo file: {}",
               result.GetEdits().size(), result.GetSceneMarkers().size(),
               CURL::GetRedacted(videoReDoFilename));
  }
  else
  {
    CLog::LogF(LOGDEBUG, "No edits or scene markers found in VideoReDo file: {}",
               CURL::GetRedacted(videoReDoFilename));
  }

  return result;
}
