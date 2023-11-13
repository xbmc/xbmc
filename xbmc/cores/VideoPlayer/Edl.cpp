/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "Edl.h"

#include "FileItem.h"
#include "ServiceBroker.h"
#include "cores/EdlEdit.h"
#include "filesystem/File.h"
#include "pvr/PVREdl.h"
#include "settings/AdvancedSettings.h"
#include "settings/SettingsComponent.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/XBMCTinyXML.h"
#include "utils/log.h"

#include "PlatformDefs.h"

#define COMSKIP_HEADER "FILE PROCESSING COMPLETE"
#define VIDEOREDO_HEADER "<Version>2"
#define VIDEOREDO_TAG_CUT "<Cut>"
#define VIDEOREDO_TAG_SCENE "<SceneMarker "

using namespace EDL;
using namespace XFILE;

CEdl::CEdl()
{
  Clear();
}

void CEdl::Clear()
{
  m_vecEdits.clear();
  m_vecSceneMarkers.clear();
  m_totalCutTime = 0;
  m_lastEditTime = -1;
}

bool CEdl::ReadEditDecisionLists(const CFileItem& fileItem, const float fFramesPerSecond)
{
  bool bFound = false;

  /*
   * Only check for edit decision lists if the movie is on the local hard drive, or accessed over a
   * network share (even if from a different private network).
   */
  const std::string& strMovie = fileItem.GetDynPath();
  if ((URIUtils::IsHD(strMovie) || URIUtils::IsOnLAN(strMovie, LanCheckMode::ANY_PRIVATE_SUBNET)) &&
      !URIUtils::IsInternetStream(strMovie))
  {
    CLog::Log(LOGDEBUG,
              "{} - Checking for edit decision lists (EDL) on local drive or remote share for: {}",
              __FUNCTION__, CURL::GetRedacted(strMovie));

    /*
     * Read any available file format until a valid EDL related file is found.
     */
    if (!bFound)
      bFound = ReadVideoReDo(strMovie);

    if (!bFound)
      bFound = ReadEdl(strMovie, fFramesPerSecond);

    if (!bFound)
      bFound = ReadComskip(strMovie, fFramesPerSecond);

    if (!bFound)
      bFound = ReadBeyondTV(strMovie);
  }
  else
  {
    bFound = ReadPvr(fileItem);
  }

  if (bFound)
  {
    MergeShortCommBreaks();
    AddSceneMarkersAtStartAndEndOfEdits();
  }

  return bFound;
}

bool CEdl::ReadEdl(const std::string& strMovie, const float fFramesPerSecond)
{
  Clear();

  std::string edlFilename(URIUtils::ReplaceExtension(strMovie, ".edl"));
  if (!CFile::Exists(edlFilename))
    return false;

  CFile edlFile;
  if (!edlFile.Open(edlFilename))
  {
    CLog::Log(LOGERROR, "{} - Could not open EDL file: {}", __FUNCTION__,
              CURL::GetRedacted(edlFilename));
    return false;
  }

  bool bError = false;
  int iLine = 0;
  std::string strBuffer;
  strBuffer.resize(1024);
  while (edlFile.ReadString(&strBuffer[0], 1024))
  {
    // Log any errors from previous run in the loop
    if (bError)
      CLog::Log(LOGWARNING, "{} - Error on line {} in EDL file: {}", __FUNCTION__, iLine,
                CURL::GetRedacted(edlFilename));

    bError = false;

    iLine++;

    char buffer1[513];
    char buffer2[513];
    int iAction;
    int iFieldsRead = sscanf(strBuffer.c_str(), "%512s %512s %i", buffer1,
                             buffer2, &iAction);
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
      CLog::Log(LOGDEBUG, "Skipping comment line {} in EDL file: {}", iLine,
                CURL::GetRedacted(edlFilename));
      continue;
    }
    /*
     * For each of the first two fields read, parse based on whether it is a time string
     * (HH:MM:SS.sss), frame marker (#12345), or normal seconds string (123.45).
     */
    int64_t editStartEnd[2];
    for (int i = 0; i < 2; i++)
    {
      if (strFields[i].find(':') != std::string::npos) // HH:MM:SS.sss format
      {
        std::vector<std::string> fieldParts = StringUtils::Split(strFields[i], '.');
        if (fieldParts.size() == 1) // No ms
        {
          editStartEnd[i] = StringUtils::TimeStringToSeconds(fieldParts[0]) *
                            static_cast<int64_t>(1000); // seconds to ms
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
          editStartEnd[i] =
              static_cast<int64_t>(StringUtils::TimeStringToSeconds(fieldParts[0])) * 1000 +
              std::atoi(fieldParts[1].c_str()); // seconds to ms
        }
        else
        {
          bError = true;
          continue;
        }
      }
      else if (strFields[i][0] == '#') // #12345 format for frame number
      {
        if (fFramesPerSecond > 0.0f)
        {
          editStartEnd[i] = static_cast<int64_t>(std::atol(strFields[i].substr(1).c_str()) /
                                                 fFramesPerSecond * 1000); // frame number to ms
        }
        else
        {
          CLog::Log(LOGERROR,
                    "Edl::ReadEdl - Frame number not supported in EDL files when frame rate is "
                    "unavailable (ts) - supplied frame number: {}",
                    strFields[i].substr(1));
          return false;
        }
      }
      else // Plain old seconds in float format, e.g. 123.45
      {
        editStartEnd[i] = std::lround(std::atof(strFields[i].c_str()) * 1000); // seconds to ms
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
      if (!AddEdit(edit))
      {
        CLog::Log(LOGWARNING, "{} - Error adding cut from line {} in EDL file: {}", __FUNCTION__,
                  iLine, CURL::GetRedacted(edlFilename));
        continue;
      }
      break;
    case 1:
      edit.action = Action::MUTE;
      if (!AddEdit(edit))
      {
        CLog::Log(LOGWARNING, "{} - Error adding mute from line {} in EDL file: {}", __FUNCTION__,
                  iLine, CURL::GetRedacted(edlFilename));
        continue;
      }
      break;
    case 2:
      if (!AddSceneMarker(edit.end))
      {
        CLog::Log(LOGWARNING, "{} - Error adding scene marker from line {} in EDL file: {}",
                  __FUNCTION__, iLine, CURL::GetRedacted(edlFilename));
        continue;
      }
      break;
    case 3:
      edit.action = Action::COMM_BREAK;
      if (!AddEdit(edit))
      {
        CLog::Log(LOGWARNING, "{} - Error adding commercial break from line {} in EDL file: {}",
                  __FUNCTION__, iLine, CURL::GetRedacted(edlFilename));
        continue;
      }
      break;
    default:
      CLog::Log(LOGWARNING, "{} - Invalid action on line {} in EDL file: {}", __FUNCTION__, iLine,
                CURL::GetRedacted(edlFilename));
      continue;
    }
  }

  if (bError) // Log last line warning, if there was one, since while loop will have terminated.
    CLog::Log(LOGWARNING, "{} - Error on line {} in EDL file: {}", __FUNCTION__, iLine,
              CURL::GetRedacted(edlFilename));

  edlFile.Close();

  if (HasEdits() || HasSceneMarker())
  {
    CLog::Log(LOGDEBUG, "{0} - Read {1} edits and {2} scene markers in EDL file: {3}", __FUNCTION__,
              m_vecEdits.size(), m_vecSceneMarkers.size(), CURL::GetRedacted(edlFilename));
    return true;
  }
  else
  {
    CLog::Log(LOGDEBUG, "{} - No edits or scene markers found in EDL file: {}", __FUNCTION__,
              CURL::GetRedacted(edlFilename));
    return false;
  }
}

bool CEdl::ReadComskip(const std::string& strMovie, const float fFramesPerSecond)
{
  Clear();

  std::string comskipFilename(URIUtils::ReplaceExtension(strMovie, ".txt"));
  if (!CFile::Exists(comskipFilename))
    return false;

  CFile comskipFile;
  if (!comskipFile.Open(comskipFilename))
  {
    CLog::Log(LOGERROR, "{} - Could not open Comskip file: {}", __FUNCTION__,
              CURL::GetRedacted(comskipFilename));
    return false;
  }

  char szBuffer[1024];
  if (comskipFile.ReadString(szBuffer, 1023)
  &&  strncmp(szBuffer, COMSKIP_HEADER, strlen(COMSKIP_HEADER)) != 0) // Line 1.
  {
    CLog::Log(LOGERROR,
              "{} - Invalid Comskip file: {}. Error reading line 1 - expected '{}' at start.",
              __FUNCTION__, CURL::GetRedacted(comskipFilename), COMSKIP_HEADER);
    comskipFile.Close();
    return false;
  }

  int iFrames;
  float fFrameRate;
  if (sscanf(szBuffer, "FILE PROCESSING COMPLETE %i FRAMES AT %f", &iFrames, &fFrameRate) != 2)
  {
    /*
     * Not all generated Comskip files have the frame rate information.
     */
    if (fFramesPerSecond > 0.0f)
    {
      fFrameRate = fFramesPerSecond;
      CLog::Log(LOGWARNING,
                "Edl::ReadComskip - Frame rate not in Comskip file. Using detected frames per "
                "second: {:.3f}",
                fFrameRate);
    }
    else
    {
      CLog::Log(LOGERROR, "Edl::ReadComskip - Frame rate is unavailable and also not in Comskip file (ts).");
      return false;
    }
  }
  else
    fFrameRate /= 100; // Reduce by factor of 100 to get fps.

  (void)comskipFile.ReadString(szBuffer, 1023); // Line 2. Ignore "-------------"

  bool bValid = true;
  int iLine = 2;
  while (bValid && comskipFile.ReadString(szBuffer, 1023)) // Line 3 and onwards.
  {
    iLine++;
    double dStartFrame, dEndFrame;
    if (sscanf(szBuffer, "%lf %lf", &dStartFrame, &dEndFrame) == 2)
    {
      Edit edit;
      edit.start = std::lround(dStartFrame / static_cast<double>(fFrameRate) * 1000.0);
      edit.end = std::lround(dEndFrame / static_cast<double>(fFrameRate) * 1000.0);
      edit.action = Action::COMM_BREAK;
      bValid = AddEdit(edit);
    }
    else
      bValid = false;
  }
  comskipFile.Close();

  if (!bValid)
  {
    CLog::Log(LOGERROR,
              "{} - Invalid Comskip file: {}. Error on line {}. Clearing any valid commercial "
              "breaks found.",
              __FUNCTION__, CURL::GetRedacted(comskipFilename), iLine);
    Clear();
    return false;
  }
  else if (HasEdits())
  {
    CLog::Log(LOGDEBUG, "{0} - Read {1} commercial breaks from Comskip file: {2}", __FUNCTION__,
              m_vecEdits.size(), CURL::GetRedacted(comskipFilename));
    return true;
  }
  else
  {
    CLog::Log(LOGDEBUG, "{} - No commercial breaks found in Comskip file: {}", __FUNCTION__,
              CURL::GetRedacted(comskipFilename));
    return false;
  }
}

bool CEdl::ReadVideoReDo(const std::string& strMovie)
{
  /*
   * VideoReDo file is strange. Tags are XML like, but it isn't an XML file.
   *
   * http://www.videoredo.com/
   */

  Clear();
  std::string videoReDoFilename(URIUtils::ReplaceExtension(strMovie, ".Vprj"));
  if (!CFile::Exists(videoReDoFilename))
    return false;

  CFile videoReDoFile;
  if (!videoReDoFile.Open(videoReDoFilename))
  {
    CLog::Log(LOGERROR, "{} - Could not open VideoReDo file: {}", __FUNCTION__,
              CURL::GetRedacted(videoReDoFilename));
    return false;
  }

  char szBuffer[1024];
  if (videoReDoFile.ReadString(szBuffer, 1023)
  &&  strncmp(szBuffer, VIDEOREDO_HEADER, strlen(VIDEOREDO_HEADER)) != 0)
  {
    CLog::Log(LOGERROR,
              "{} - Invalid VideoReDo file: {}. Error reading line 1 - expected {}. Only version 2 "
              "files are supported.",
              __FUNCTION__, CURL::GetRedacted(videoReDoFilename), VIDEOREDO_HEADER);
    videoReDoFile.Close();
    return false;
  }

  int iLine = 1;
  bool bValid = true;
  while (bValid && videoReDoFile.ReadString(szBuffer, 1023))
  {
    iLine++;
    if (strncmp(szBuffer, VIDEOREDO_TAG_CUT, strlen(VIDEOREDO_TAG_CUT)) == 0) // Found the <Cut> tag
    {
      /*
       * double is used as 32 bit float would overflow.
       */
      double dStart, dEnd;
      if (sscanf(szBuffer + strlen(VIDEOREDO_TAG_CUT), "%lf:%lf", &dStart, &dEnd) == 2)
      {
        /*
         *  Times need adjusting by 1/10,000 to get ms.
         */
        Edit edit;
        edit.start = std::lround(dStart / 10000);
        edit.end = std::lround(dEnd / 10000);
        edit.action = Action::CUT;
        bValid = AddEdit(edit);
      }
      else
        bValid = false;
    }
    else if (strncmp(szBuffer, VIDEOREDO_TAG_SCENE, strlen(VIDEOREDO_TAG_SCENE)) == 0) // Found the <SceneMarker > tag
    {
      int iScene;
      double dSceneMarker;
      if (sscanf(szBuffer + strlen(VIDEOREDO_TAG_SCENE), " %i>%lf", &iScene, &dSceneMarker) == 2)
        bValid = AddSceneMarker(
            std::lround(dSceneMarker / 10000)); // Times need adjusting by 1/10,000 to get ms.
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
    CLog::Log(LOGERROR,
              "{} - Invalid VideoReDo file: {}. Error in line {}. Clearing any valid edits or "
              "scenes found.",
              __FUNCTION__, CURL::GetRedacted(videoReDoFilename), iLine);
    Clear();
    return false;
  }
  else if (HasEdits() || HasSceneMarker())
  {
    CLog::Log(LOGDEBUG, "{0} - Read {1} edits and {2} scene markers in VideoReDo file: {3}",
              __FUNCTION__, m_vecEdits.size(), m_vecSceneMarkers.size(),
              CURL::GetRedacted(videoReDoFilename));
    return true;
  }
  else
  {
    CLog::Log(LOGDEBUG, "{} - No edits or scene markers found in VideoReDo file: {}", __FUNCTION__,
              CURL::GetRedacted(videoReDoFilename));
    return false;
  }
}

bool CEdl::ReadBeyondTV(const std::string& strMovie)
{
  Clear();

  std::string beyondTVFilename(URIUtils::ReplaceExtension(strMovie, URIUtils::GetExtension(strMovie) + ".chapters.xml"));
  if (!CFile::Exists(beyondTVFilename))
    return false;

  CXBMCTinyXML xmlDoc;
  if (!xmlDoc.LoadFile(beyondTVFilename))
  {
    CLog::Log(LOGERROR, "{} - Could not load Beyond TV file: {}. {}", __FUNCTION__,
              CURL::GetRedacted(beyondTVFilename), xmlDoc.ErrorDesc());
    return false;
  }

  if (xmlDoc.Error())
  {
    CLog::Log(LOGERROR, "{} - Could not parse Beyond TV file: {}. {}", __FUNCTION__,
              CURL::GetRedacted(beyondTVFilename), xmlDoc.ErrorDesc());
    return false;
  }

  TiXmlElement *pRoot = xmlDoc.RootElement();
  if (!pRoot || strcmp(pRoot->Value(), "cutlist"))
  {
    CLog::Log(LOGERROR, "{} - Invalid Beyond TV file: {}. Expected root node to be <cutlist>",
              __FUNCTION__, CURL::GetRedacted(beyondTVFilename));
    return false;
  }

  bool bValid = true;
  TiXmlElement *pRegion = pRoot->FirstChildElement("Region");
  while (bValid && pRegion)
  {
    TiXmlElement *pStart = pRegion->FirstChildElement("start");
    TiXmlElement *pEnd = pRegion->FirstChildElement("end");
    if (pStart && pEnd && pStart->FirstChild() && pEnd->FirstChild())
    {
      /*
       * Need to divide the start and end times by a factor of 10,000 to get msec.
       * E.g. <start comment="00:02:44.9980867">1649980867</start>
       *
       * Use atof so doesn't overflow 32 bit float or integer / long.
       * E.g. <end comment="0:26:49.0000009">16090090000</end>
       *
       * Don't use atoll even though it is more correct as it isn't natively supported by
       * Visual Studio.
       *
       * atof() returns 0 if there were any problems and will subsequently be rejected in AddEdit().
       */
      Edit edit;
      edit.start = std::lround((std::atof(pStart->FirstChild()->Value()) / 10000));
      edit.end = std::lround((std::atof(pEnd->FirstChild()->Value()) / 10000));
      edit.action = Action::COMM_BREAK;
      bValid = AddEdit(edit);
    }
    else
      bValid = false;

    pRegion = pRegion->NextSiblingElement("Region");
  }
  if (!bValid)
  {
    CLog::Log(LOGERROR,
              "{} - Invalid Beyond TV file: {}. Clearing any valid commercial breaks found.",
              __FUNCTION__, CURL::GetRedacted(beyondTVFilename));
    Clear();
    return false;
  }
  else if (HasEdits())
  {
    CLog::Log(LOGDEBUG, "{0} - Read {1} commercial breaks from Beyond TV file: {2}", __FUNCTION__,
              m_vecEdits.size(), CURL::GetRedacted(beyondTVFilename));
    return true;
  }
  else
  {
    CLog::Log(LOGDEBUG, "{} - No commercial breaks found in Beyond TV file: {}", __FUNCTION__,
              CURL::GetRedacted(beyondTVFilename));
    return false;
  }
}

bool CEdl::ReadPvr(const CFileItem &fileItem)
{
  const std::vector<Edit> editlist = PVR::CPVREdl::GetEdits(fileItem);
  for (const auto& edit : editlist)
  {
    switch (edit.action)
    {
      case Action::CUT:
      case Action::MUTE:
      case Action::COMM_BREAK:
        if (AddEdit(edit))
        {
          CLog::Log(LOGDEBUG, "{} - Added break [{} - {}] found in PVR item for: {}.", __FUNCTION__,
                    MillisecondsToTimeString(edit.start), MillisecondsToTimeString(edit.end),
                    CURL::GetRedacted(fileItem.GetDynPath()));
        }
        else
        {
          CLog::Log(LOGERROR,
                    "{} - Invalid break [{} - {}] found in PVR item for: {}. Continuing anyway.",
                    __FUNCTION__, MillisecondsToTimeString(edit.start),
                    MillisecondsToTimeString(edit.end), CURL::GetRedacted(fileItem.GetDynPath()));
        }
        break;

      case Action::SCENE:
        if (!AddSceneMarker(edit.end))
        {
          CLog::Log(LOGWARNING, "{} - Error adding scene marker for PVR item", __FUNCTION__);
        }
        break;

      default:
        CLog::Log(LOGINFO, "{} - Ignoring entry of unknown edit action: {}", __FUNCTION__,
                  static_cast<int>(edit.action));
        break;
    }
  }

  return !editlist.empty();
}

bool CEdl::AddEdit(const Edit& newEdit)
{
  Edit edit = newEdit;

  if (edit.action != Action::CUT && edit.action != Action::MUTE &&
      edit.action != Action::COMM_BREAK)
  {
    CLog::Log(LOGERROR,
              "{} - Not an Action::CUT, Action::MUTE, or Action::COMM_BREAK! [{} - {}], {}",
              __FUNCTION__, MillisecondsToTimeString(edit.start),
              MillisecondsToTimeString(edit.end), static_cast<int>(edit.action));
    return false;
  }

  if (edit.start < 0)
  {
    CLog::Log(LOGERROR, "{} - Before start! [{} - {}], {}", __FUNCTION__,
              MillisecondsToTimeString(edit.start), MillisecondsToTimeString(edit.end),
              static_cast<int>(edit.action));
    return false;
  }

  if (edit.start >= edit.end)
  {
    CLog::Log(LOGERROR, "{} - Times are around the wrong way or the same! [{} - {}], {}",
              __FUNCTION__, MillisecondsToTimeString(edit.start),
              MillisecondsToTimeString(edit.end), static_cast<int>(edit.action));
    return false;
  }

  if (InEdit(edit.start) || InEdit(edit.end))
  {
    CLog::Log(LOGERROR, "{} - Start or end is in an existing edit! [{} - {}], {}", __FUNCTION__,
              MillisecondsToTimeString(edit.start), MillisecondsToTimeString(edit.end),
              static_cast<int>(edit.action));
    return false;
  }

  for (size_t i = 0; i < m_vecEdits.size(); ++i)
  {
    if (edit.start < m_vecEdits[i].start && edit.end > m_vecEdits[i].end)
    {
      CLog::Log(LOGERROR, "{} - Edit surrounds an existing edit! [{} - {}], {}", __FUNCTION__,
                MillisecondsToTimeString(edit.start), MillisecondsToTimeString(edit.end),
                static_cast<int>(edit.action));
      return false;
    }
  }

  if (edit.action == Action::COMM_BREAK)
  {
    /*
     * Detection isn't perfect near the edges of commercial breaks so automatically wait for a bit at
     * the start (autowait) and automatically rewind by a bit (autowind) at the end of the commercial
     * break.
     */
    int autowait = CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_iEdlCommBreakAutowait * 1000; // seconds -> ms
    int autowind = CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_iEdlCommBreakAutowind * 1000; // seconds -> ms

    if (edit.start > 0) // Only autowait if not at the start.
    {
      /* get the edit length so we don't start skipping after the end */
      int editLength = edit.end - edit.start;
      /* add the lesser of the edit length or the autowait to the start */
      edit.start += autowait > editLength ? editLength : autowait;
    }
    if (edit.end > edit.start) // Only autowind if there is any edit time remaining.
    {
      /* get the remaining edit length so we don't rewind to before the start */
      int editLength = edit.end - edit.start;
      /* subtract the lesser of the edit length or the autowind from the end */
      edit.end -= autowind > editLength ? editLength : autowind;
    }
  }

  /*
   * Insert edit in the list in the right position (ALL algorithms assume edits are in ascending order)
   */
  if (m_vecEdits.empty() || edit.start > m_vecEdits.back().start)
  {
    CLog::Log(LOGDEBUG, "{} - Pushing new edit to back [{} - {}], {}", __FUNCTION__,
              MillisecondsToTimeString(edit.start), MillisecondsToTimeString(edit.end),
              static_cast<int>(edit.action));
    m_vecEdits.emplace_back(edit);
  }
  else
  {
    std::vector<Edit>::iterator pCurrentEdit;
    for (pCurrentEdit = m_vecEdits.begin(); pCurrentEdit != m_vecEdits.end(); ++pCurrentEdit)
    {
      if (edit.start < pCurrentEdit->start)
      {
        CLog::Log(LOGDEBUG, "{} - Inserting new edit [{} - {}], {}", __FUNCTION__,
                  MillisecondsToTimeString(edit.start), MillisecondsToTimeString(edit.end),
                  static_cast<int>(edit.action));
        m_vecEdits.insert(pCurrentEdit, edit);
        break;
      }
    }
  }

  if (edit.action == Action::CUT)
    m_totalCutTime += edit.end - edit.start;

  return true;
}

bool CEdl::AddSceneMarker(const int iSceneMarker)
{
  Edit edit;

  if (InEdit(iSceneMarker, &edit) && edit.action == Action::CUT) // Only works for current cuts.
    return false;

  CLog::Log(LOGDEBUG, "{} - Inserting new scene marker: {}", __FUNCTION__,
            MillisecondsToTimeString(iSceneMarker));
  m_vecSceneMarkers.push_back(iSceneMarker); // Unsorted

  return true;
}

bool CEdl::HasEdits() const
{
  return !m_vecEdits.empty();
}

bool CEdl::HasCuts() const
{
  return m_totalCutTime > 0;
}

int CEdl::GetTotalCutTime() const
{
  return m_totalCutTime; // ms
}

const std::vector<EDL::Edit> CEdl::GetEditList() const
{
  // the sum of cut durations while we iterate over them
  // note: edits are ordered by start time
  int surpassedSumOfCutDurations{0};
  std::vector<EDL::Edit> editList;

  // @note we should not modify the original edits since
  // they are used during playback. However we need to correct
  // the start and end times to present on the GUI by removing
  // the already surpassed cut time. The copy here is intentional
  // \sa Player_Editlist
  for (EDL::Edit edit : m_vecEdits)
  {
    if (edit.action == Action::CUT)
    {
      surpassedSumOfCutDurations += edit.end - edit.start;
      continue;
    }

    // substract the duration of already surpassed cuts
    edit.start -= surpassedSumOfCutDurations;
    edit.end -= surpassedSumOfCutDurations;
    editList.emplace_back(edit);
  }

  return editList;
}

const std::vector<int64_t> CEdl::GetCutMarkers() const
{
  int surpassedSumOfCutDurations{0};
  std::vector<int64_t> cutList;
  for (const EDL::Edit& edit : m_vecEdits)
  {
    if (edit.action != Action::CUT)
      continue;

    cutList.emplace_back(edit.start - surpassedSumOfCutDurations);
    surpassedSumOfCutDurations += edit.end - edit.start;
  }
  return cutList;
}

const std::vector<int64_t> CEdl::GetSceneMarkers() const
{
  std::vector<int64_t> sceneMarkers;
  sceneMarkers.reserve(m_vecSceneMarkers.size());
  for (const int& scene : m_vecSceneMarkers)
  {
    sceneMarkers.emplace_back(GetTimeWithoutCuts(scene));
  }
  return sceneMarkers;
}

int CEdl::GetTimeWithoutCuts(int seek) const
{
  if (!HasCuts())
    return seek;

  int cutTime = 0;
  for (const EDL::Edit& edit : m_vecEdits)
  {
    if (edit.action != Action::CUT)
      continue;

    // inside cut
    if (seek >= edit.start && seek <= edit.end)
    {
      // decrease cut lenght by 1 ms to jump over the end boundary.
      cutTime += seek - edit.start - 1;
    }
    // cut has already been passed over
    else if (seek >= edit.start)
    {
      cutTime += edit.end - edit.start;
    }
  }
  return seek - cutTime;
}

double CEdl::GetTimeAfterRestoringCuts(double seek) const
{
  if (!HasCuts())
    return seek;

  for (const EDL::Edit& edit : m_vecEdits)
  {
    double cutDuration = static_cast<double>(edit.end - edit.start);
    // add 1 ms to jump over the start boundary
    if (edit.action == Action::CUT && seek > edit.start + 1)
    {
      seek += cutDuration;
    }
  }
  return seek;
}

bool CEdl::HasSceneMarker() const
{
  return !m_vecSceneMarkers.empty();
}

bool CEdl::InEdit(const int iSeek, Edit* pEdit)
{
  for (size_t i = 0; i < m_vecEdits.size(); ++i)
  {
    if (iSeek < m_vecEdits[i].start) // Early exit if not even up to the edit start time.
      return false;

    if (iSeek >= m_vecEdits[i].start && iSeek <= m_vecEdits[i].end) // Inside edit.
    {
      if (pEdit)
        *pEdit = m_vecEdits[i];
      return true;
    }
  }

  return false;
}

int CEdl::GetLastEditTime() const
{
  return m_lastEditTime;
}

void CEdl::SetLastEditTime(int editTime)
{
  m_lastEditTime = editTime;
}

void CEdl::ResetLastEditTime()
{
  m_lastEditTime = -1;
}

void CEdl::SetLastEditActionType(EDL::Action action)
{
  m_lastEditActionType = action;
}

EDL::Action CEdl::GetLastEditActionType() const
{
  return m_lastEditActionType;
}

bool CEdl::GetNextSceneMarker(bool bPlus, const int iClock, int *iSceneMarker)
{
  if (!HasSceneMarker())
    return false;

  int iSeek = GetTimeAfterRestoringCuts(iClock);

  int iDiff = 10 * 60 * 60 * 1000; // 10 hours to ms.
  bool bFound = false;

  if (bPlus) // Find closest scene forwards
  {
    for (int i = 0; i < (int)m_vecSceneMarkers.size(); i++)
    {
      if ((m_vecSceneMarkers[i] > iSeek) && ((m_vecSceneMarkers[i] - iSeek) < iDiff))
      {
        iDiff = m_vecSceneMarkers[i] - iSeek;
        *iSceneMarker = m_vecSceneMarkers[i];
        bFound = true;
      }
    }
  }
  else // Find closest scene backwards
  {
    for (int i = 0; i < (int)m_vecSceneMarkers.size(); i++)
    {
      if ((m_vecSceneMarkers[i] < iSeek) && ((iSeek - m_vecSceneMarkers[i]) < iDiff))
      {
        iDiff = iSeek - m_vecSceneMarkers[i];
        *iSceneMarker = m_vecSceneMarkers[i];
        bFound = true;
      }
    }
  }

  /*
   * If the scene marker is in a cut then return the end of the cut. Can't guarantee that this is
   * picked up when scene markers are added.
   */
  Edit edit;
  if (bFound && InEdit(*iSceneMarker, &edit) && edit.action == Action::CUT)
    *iSceneMarker = edit.end;

  return bFound;
}

std::string CEdl::MillisecondsToTimeString(const int iMilliseconds)
{
  std::string strTimeString = StringUtils::SecondsToTimeString((long)(iMilliseconds / 1000), TIME_FORMAT_HH_MM_SS); // milliseconds to seconds
  strTimeString += StringUtils::Format(".{:03}", iMilliseconds % 1000);
  return strTimeString;
}

void CEdl::MergeShortCommBreaks()
{
  /*
   * mythcommflag routinely seems to put a 20-40ms commercial break at the start of the recording.
   *
   * Remove any spurious short commercial breaks at the very start so they don't interfere with
   * the algorithms below.
   */
  if (!m_vecEdits.empty() && m_vecEdits[0].action == Action::COMM_BREAK &&
      (m_vecEdits[0].end - m_vecEdits[0].start) < 5 * 1000) // 5 seconds
  {
    CLog::Log(LOGDEBUG, "{} - Removing short commercial break at start [{} - {}]. <5 seconds",
              __FUNCTION__, MillisecondsToTimeString(m_vecEdits[0].start),
              MillisecondsToTimeString(m_vecEdits[0].end));
    m_vecEdits.erase(m_vecEdits.begin());
  }

  const std::shared_ptr<CAdvancedSettings> advancedSettings = CServiceBroker::GetSettingsComponent()->GetAdvancedSettings();
  if (advancedSettings->m_bEdlMergeShortCommBreaks)
  {
    for (size_t i = 0; i < m_vecEdits.size() - 1; ++i)
    {
      if ((m_vecEdits[i].action == Action::COMM_BREAK &&
           m_vecEdits[i + 1].action == Action::COMM_BREAK) &&
          (m_vecEdits[i + 1].end - m_vecEdits[i].start <
           advancedSettings->m_iEdlMaxCommBreakLength * 1000) // s to ms
          && (m_vecEdits[i + 1].start - m_vecEdits[i].end <
              advancedSettings->m_iEdlMaxCommBreakGap * 1000)) // s to ms
      {
        Edit commBreak;
        commBreak.action = Action::COMM_BREAK;
        commBreak.start = m_vecEdits[i].start;
        commBreak.end = m_vecEdits[i + 1].end;

        CLog::Log(
            LOGDEBUG, "{} - Consolidating commercial break [{} - {}] and [{} - {}] to: [{} - {}]",
            __FUNCTION__, MillisecondsToTimeString(m_vecEdits[i].start),
            MillisecondsToTimeString(m_vecEdits[i].end),
            MillisecondsToTimeString(m_vecEdits[i + 1].start),
            MillisecondsToTimeString(m_vecEdits[i + 1].end),
            MillisecondsToTimeString(commBreak.start), MillisecondsToTimeString(commBreak.end));

        /*
         * Erase old edits and insert the new merged one.
         */
        m_vecEdits.erase(m_vecEdits.begin() + i, m_vecEdits.begin() + i + 2);
        m_vecEdits.insert(m_vecEdits.begin() + i, commBreak);

        i--; // Reduce i to see if the next break is also within the max commercial break length.
      }
    }

    /*
     * To cater for recordings that are started early and then have a commercial break identified
     * before the TV show starts, expand the first commercial break to the very beginning if it
     * starts within the maximum start gap. This is done outside of the consolidation to prevent
     * the maximum commercial break length being triggered.
     */
    if (!m_vecEdits.empty() && m_vecEdits[0].action == Action::COMM_BREAK &&
        m_vecEdits[0].start < advancedSettings->m_iEdlMaxStartGap * 1000)
    {
      CLog::Log(LOGDEBUG, "{} - Expanding first commercial break back to start [{} - {}].",
                __FUNCTION__, MillisecondsToTimeString(m_vecEdits[0].start),
                MillisecondsToTimeString(m_vecEdits[0].end));
      m_vecEdits[0].start = 0;
    }

    /*
     * Remove any commercial breaks shorter than the minimum (unless at the start)
     */
    for (size_t i = 0; i < m_vecEdits.size(); ++i)
    {
      if (m_vecEdits[i].action == Action::COMM_BREAK && m_vecEdits[i].start > 0 &&
          (m_vecEdits[i].end - m_vecEdits[i].start) <
              advancedSettings->m_iEdlMinCommBreakLength * 1000)
      {
        CLog::Log(LOGDEBUG,
                  "{} - Removing short commercial break [{} - {}]. Minimum length: {} seconds",
                  __FUNCTION__, MillisecondsToTimeString(m_vecEdits[i].start),
                  MillisecondsToTimeString(m_vecEdits[i].end),
                  advancedSettings->m_iEdlMinCommBreakLength);
        m_vecEdits.erase(m_vecEdits.begin() + i);

        i--;
      }
    }
  }
}

void CEdl::AddSceneMarkersAtStartAndEndOfEdits()
{
  for (const EDL::Edit& edit : m_vecEdits)
  {
    // Add scene markers at the start and end of commercial breaks
    if (edit.action == Action::COMM_BREAK)
    {
      // Don't add a scene marker at the start.
      if (edit.start > 0)
        AddSceneMarker(edit.start);
      AddSceneMarker(edit.end);
    }
  }
}
