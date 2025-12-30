/*
 *  Copyright (C) 2005-2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ComskipParser.h"

#include "FileItem.h"
#include "filesystem/File.h"
#include "utils/URIUtils.h"
#include "utils/log.h"

namespace
{
constexpr const char* COMSKIP_HEADER = "FILE PROCESSING COMPLETE";
} // namespace

using namespace EDL;
using namespace XFILE;

std::string CComskipParser::GetEdlFilePath(const CFileItem& item) const
{
  return URIUtils::ReplaceExtension(item.GetDynPath(), ".txt");
}

CEdlParserResult CComskipParser::Parse(const CFileItem& item, float fps)
{
  CEdlParserResult result;

  const std::string comskipFilename = GetEdlFilePath(item);

  CFile comskipFile;
  if (!comskipFile.Open(comskipFilename))
  {
    CLog::LogF(LOGERROR, "Could not open Comskip file: {}", CURL::GetRedacted(comskipFilename));
    return result;
  }

  std::string line;
  line.reserve(1024);
  if (!comskipFile.ReadLine(line) || !line.starts_with(COMSKIP_HEADER)) // Line 1.
  {
    CLog::LogF(LOGERROR, "Invalid Comskip file: {}. Error reading line 1 - expected '{}' at start.",
               CURL::GetRedacted(comskipFilename), COMSKIP_HEADER);
    comskipFile.Close();
    return result;
  }

  int iFrames;
  float fFrameRate;
  if (sscanf(line.c_str(), "FILE PROCESSING COMPLETE %i FRAMES AT %f", &iFrames, &fFrameRate) != 2)
  {
    /*
     * Not all generated Comskip files have the frame rate information.
     */
    if (fps > 0.0f)
    {
      fFrameRate = fps;
      CLog::LogF(LOGWARNING,
                 "Frame rate not in Comskip file. Using detected frames per second: {:.3f}",
                 fFrameRate);
    }
    else
    {
      CLog::LogF(LOGERROR, "Frame rate is unavailable and also not in Comskip file (ts).");
      return result;
    }
  }
  else
    fFrameRate /= 100; // Reduce by factor of 100 to get fps.

  (void)comskipFile.ReadLine(line); // Line 2. Ignore "-------------"

  bool bValid = true;
  int iLine = 2;
  while (bValid && comskipFile.ReadLine(line)) // Line 3 and onwards.
  {
    iLine++;
    double dStartFrame, dEndFrame;
    if (sscanf(line.c_str(), "%lf %lf", &dStartFrame, &dEndFrame) == 2)
    {
      Edit edit;
      edit.start = std::chrono::duration_cast<std::chrono::milliseconds>(
          std::chrono::duration<double, std::ratio<1>>{dStartFrame /
                                                       static_cast<double>(fFrameRate)});
      edit.end = std::chrono::duration_cast<std::chrono::milliseconds>(
          std::chrono::duration_cast<std::chrono::milliseconds>(
              std::chrono::duration<double, std::ratio<1>>{dEndFrame /
                                                           static_cast<double>(fFrameRate)}));
      edit.action = Action::COMM_BREAK;
      result.AddEdit(edit, EdlSourceLocation{comskipFilename, iLine});
    }
    else
      bValid = false;
  }
  comskipFile.Close();

  if (!bValid)
  {
    CLog::LogF(LOGERROR,
               "Invalid Comskip file: {}. Error on line {}. Clearing any valid commercial "
               "breaks found.",
               CURL::GetRedacted(comskipFilename), iLine);
    return {};
  }
  else if (!result.GetEdits().empty())
  {
    CLog::LogF(LOGDEBUG, "Read {} commercial breaks from Comskip file: {}",
               result.GetEdits().size(), CURL::GetRedacted(comskipFilename));
  }
  else
  {
    CLog::LogF(LOGDEBUG, "No commercial breaks found in Comskip file: {}",
               CURL::GetRedacted(comskipFilename));
  }

  return result;
}
