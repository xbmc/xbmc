/*
 *  Copyright (C) 2005-2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "EdlFileParser.h"

#include "FileItem.h"
#include "filesystem/File.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/log.h"

#include <charconv>

using namespace EDL;
using namespace XFILE;

std::string CEdlFileParser::GetEdlFilePath(const CFileItem& item) const
{
  return URIUtils::ReplaceExtension(item.GetDynPath(), ".edl");
}

CEdlParserResult CEdlFileParser::Parse(const CFileItem& item, float fps)
{
  CEdlParserResult result;

  const std::string edlFilename = GetEdlFilePath(item);

  CFile edlFile;
  if (!edlFile.Open(edlFilename))
  {
    CLog::LogF(LOGERROR, "Could not open EDL file: {}", CURL::GetRedacted(edlFilename));
    return result;
  }

  bool bError = false;
  int iLine = 0;
  std::string strBuffer;
  strBuffer.reserve(1024);
  while (edlFile.ReadLine(strBuffer))
  {
    // Log any errors from previous run in the loop
    if (bError)
      CLog::LogF(LOGWARNING, "Error on line {} in EDL file: {}", iLine,
                 CURL::GetRedacted(edlFilename));

    bError = false;

    iLine++;

    char buffer1[513];
    char buffer2[513];
    int iAction;
    int iFieldsRead = sscanf(strBuffer.c_str(), "%512s %512s %i", buffer1, buffer2, &iAction);
    if (iFieldsRead != 2 && iFieldsRead != 3) // Make sure we read the right number of fields
    {
      bError = true;
      continue;
    }

    std::vector<std::string> strFields(2);
    strFields[0] = buffer1;
    strFields[1] = buffer2;

    if (iFieldsRead == 2) // If only 2 fields read, then assume it's a scene marker.
    {
      iAction = atoi(strFields[1].c_str());
      strFields[1] = strFields[0];
    }

    if (StringUtils::StartsWith(strFields[0], "##"))
    {
      CLog::LogF(LOGDEBUG, "Skipping comment line {} in EDL file: {}", iLine,
                 CURL::GetRedacted(edlFilename));
      continue;
    }
    /*
     * For each of the first two fields read, parse based on whether it is a time string
     * (HH:MM:SS.sss), frame marker (#12345), or normal seconds string (123.45).
     */
    std::chrono::milliseconds editStartEnd[2];
    for (int i = 0; i < 2; i++)
    {
      if (strFields[i].find(':') != std::string::npos) // HH:MM:SS.sss format
      {
        std::vector<std::string> fieldParts = StringUtils::Split(strFields[i], '.');
        if (fieldParts.size() == 1) // No ms
        {
          editStartEnd[i] = std::chrono::duration_cast<std::chrono::milliseconds>(
              std::chrono::seconds(StringUtils::TimeStringToSeconds(fieldParts[0])));
        }
        else if (fieldParts.size() == 2) // Has ms. Everything after the dot (.) is ms
        {
          /*
           * Have to pad or truncate the ms portion to 3 characters before converting to ms.
           */
          if (fieldParts[1].length() == 1)
          {
            fieldParts[1] = fieldParts[1] + "00";
          }
          else if (fieldParts[1].length() == 2)
          {
            fieldParts[1] = fieldParts[1] + "0";
          }
          else if (fieldParts[1].length() > 3)
          {
            fieldParts[1] = fieldParts[1].substr(0, 3);
          }
          int additionalMs{0};
          editStartEnd[i] = std::chrono::duration_cast<std::chrono::milliseconds>(
              std::chrono::seconds(StringUtils::TimeStringToSeconds(fieldParts[0])));
          std::from_chars(fieldParts[1].data(), fieldParts[1].data() + fieldParts[1].size(),
                          additionalMs);
          editStartEnd[i] += std::chrono::milliseconds(additionalMs);
        }
        else
        {
          bError = true;
          continue;
        }
      }
      else if (strFields[i][0] == '#') // #12345 format for frame number
      {
        if (fps > 0.0f)
        {
          std::chrono::duration<double, std::ratio<1>> durationInSeconds{
              std::atol(strFields[i].substr(1).c_str()) / fps};
          editStartEnd[i] =
              std::chrono::duration_cast<std::chrono::milliseconds>(durationInSeconds);
        }
        else
        {
          CLog::LogF(LOGERROR,
                     "Frame number not supported in EDL files when frame rate is unavailable "
                     "(ts) - supplied frame number: {}",
                     strFields[i].substr(1));
          return {};
        }
      }
      else // Plain old seconds in float format, e.g. 123.45
      {
        editStartEnd[i] =
            std::chrono::milliseconds{std::lround(std::atof(strFields[i].c_str()) * 1000)};
      }
    }

    if (bError) // If there was an error in the for loop, ignore and continue with the next line
      continue;

    Edit edit;
    edit.start = editStartEnd[0];
    edit.end = editStartEnd[1];

    switch (iAction)
    {
      case 0:
        edit.action = Action::CUT;
        result.AddEdit(edit, EdlSourceLocation{edlFilename, iLine});
        break;
      case 1:
        edit.action = Action::MUTE;
        result.AddEdit(edit, EdlSourceLocation{edlFilename, iLine});
        break;
      case 2:
        result.AddSceneMarker(edit.end, EdlSourceLocation{edlFilename, iLine});
        break;
      case 3:
        edit.action = Action::COMM_BREAK;
        result.AddEdit(edit, EdlSourceLocation{edlFilename, iLine});
        break;
      default:
        CLog::LogF(LOGWARNING, "Invalid action on line {} in EDL file: {}", iLine,
                   CURL::GetRedacted(edlFilename));
        continue;
    }
  }

  if (bError) // Log last line warning, if there was one, since while loop will have terminated.
    CLog::LogF(LOGWARNING, "Error on line {} in EDL file: {}", iLine,
               CURL::GetRedacted(edlFilename));

  edlFile.Close();

  if (!result.IsEmpty())
  {
    CLog::LogF(LOGDEBUG, "Read {} edits and {} scene markers in EDL file: {}",
               result.GetEdits().size(), result.GetSceneMarkers().size(),
               CURL::GetRedacted(edlFilename));
  }
  else
  {
    CLog::LogF(LOGDEBUG, "No edits or scene markers found in EDL file: {}",
               CURL::GetRedacted(edlFilename));
  }

  return result;
}
