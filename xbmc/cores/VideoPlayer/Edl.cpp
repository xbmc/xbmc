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
#include "cores/Cut.h"
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
  m_vecCuts.clear();
  m_vecSceneMarkers.clear();
  m_iTotalCutTime = 0;
  m_lastCutTime = -1;
}

bool CEdl::ReadEditDecisionLists(const CFileItem& fileItem, const float fFramesPerSecond)
{
  bool bFound = false;

  /*
   * Only check for edit decision lists if the movie is on the local hard drive, or accessed over a
   * network share.
   */
  const std::string& strMovie = fileItem.GetDynPath();
  if ((URIUtils::IsHD(strMovie) || URIUtils::IsOnLAN(strMovie)) &&
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
    MergeShortCommBreaks();

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
    int64_t iCutStartEnd[2];
    for (int i = 0; i < 2; i++)
    {
      if (strFields[i].find(':') != std::string::npos) // HH:MM:SS.sss format
      {
        std::vector<std::string> fieldParts = StringUtils::Split(strFields[i], '.');
        if (fieldParts.size() == 1) // No ms
        {
          iCutStartEnd[i] = StringUtils::TimeStringToSeconds(fieldParts[0]) * (int64_t)1000; // seconds to ms
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
          iCutStartEnd[i] = (int64_t)StringUtils::TimeStringToSeconds(fieldParts[0]) * 1000 + atoi(fieldParts[1].c_str()); // seconds to ms
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
          iCutStartEnd[i] = static_cast<int64_t>(std::atol(strFields[i].substr(1).c_str()) / fFramesPerSecond * 1000); // frame number to ms
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
        iCutStartEnd[i] = (int64_t)(atof(strFields[i].c_str()) * 1000); // seconds to ms
      }
    }

    if (bError) // If there was an error in the for loop, ignore and continue with the next line
      continue;

    Cut cut;
    cut.start = iCutStartEnd[0];
    cut.end = iCutStartEnd[1];

    switch (iAction)
    {
    case 0:
      cut.action = Action::CUT;
      if (!AddCut(cut))
      {
        CLog::Log(LOGWARNING, "{} - Error adding cut from line {} in EDL file: {}", __FUNCTION__,
                  iLine, CURL::GetRedacted(edlFilename));
        continue;
      }
      break;
    case 1:
      cut.action = Action::MUTE;
      if (!AddCut(cut))
      {
        CLog::Log(LOGWARNING, "{} - Error adding mute from line {} in EDL file: {}", __FUNCTION__,
                  iLine, CURL::GetRedacted(edlFilename));
        continue;
      }
      break;
    case 2:
      if (!AddSceneMarker(cut.end))
      {
        CLog::Log(LOGWARNING, "{} - Error adding scene marker from line {} in EDL file: {}",
                  __FUNCTION__, iLine, CURL::GetRedacted(edlFilename));
        continue;
      }
      break;
    case 3:
      cut.action = Action::COMM_BREAK;
      if (!AddCut(cut))
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

  if (HasCut() || HasSceneMarker())
  {
    CLog::Log(LOGDEBUG, "{0} - Read {1} cuts and {2} scene markers in EDL file: {3}", __FUNCTION__,
              m_vecCuts.size(), m_vecSceneMarkers.size(), CURL::GetRedacted(edlFilename));
    return true;
  }
  else
  {
    CLog::Log(LOGDEBUG, "{} - No cuts or scene markers found in EDL file: {}", __FUNCTION__,
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
      Cut cut;
      cut.start = static_cast<int64_t>(dStartFrame / static_cast<double>(fFrameRate) * 1000.0);
      cut.end = static_cast<int64_t>(dEndFrame / static_cast<double>(fFrameRate) * 1000.0);
      cut.action = Action::COMM_BREAK;
      bValid = AddCut(cut);
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
  else if (HasCut())
  {
    CLog::Log(LOGDEBUG, "{0} - Read {1} commercial breaks from Comskip file: {2}", __FUNCTION__,
              m_vecCuts.size(), CURL::GetRedacted(comskipFilename));
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
        Cut cut;
        cut.start = (int64_t)(dStart / 10000);
        cut.end = (int64_t)(dEnd / 10000);
        cut.action = Action::CUT;
        bValid = AddCut(cut);
      }
      else
        bValid = false;
    }
    else if (strncmp(szBuffer, VIDEOREDO_TAG_SCENE, strlen(VIDEOREDO_TAG_SCENE)) == 0) // Found the <SceneMarker > tag
    {
      int iScene;
      double dSceneMarker;
      if (sscanf(szBuffer + strlen(VIDEOREDO_TAG_SCENE), " %i>%lf", &iScene, &dSceneMarker) == 2)
        bValid = AddSceneMarker((int64_t)(dSceneMarker / 10000)); // Times need adjusting by 1/10,000 to get ms.
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
              "{} - Invalid VideoReDo file: {}. Error in line {}. Clearing any valid cuts or "
              "scenes found.",
              __FUNCTION__, CURL::GetRedacted(videoReDoFilename), iLine);
    Clear();
    return false;
  }
  else if (HasCut() || HasSceneMarker())
  {
    CLog::Log(LOGDEBUG, "{0} - Read {1} cuts and {2} scene markers in VideoReDo file: {3}",
              __FUNCTION__, m_vecCuts.size(), m_vecSceneMarkers.size(),
              CURL::GetRedacted(videoReDoFilename));
    return true;
  }
  else
  {
    CLog::Log(LOGDEBUG, "{} - No cuts or scene markers found in VideoReDo file: {}", __FUNCTION__,
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
       * atof() returns 0 if there were any problems and will subsequently be rejected in AddCut().
       */
      Cut cut;
      cut.start = (int64_t)(atof(pStart->FirstChild()->Value()) / 10000);
      cut.end = (int64_t)(atof(pEnd->FirstChild()->Value()) / 10000);
      cut.action = Action::COMM_BREAK;
      bValid = AddCut(cut);
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
  else if (HasCut())
  {
    CLog::Log(LOGDEBUG, "{0} - Read {1} commercial breaks from Beyond TV file: {2}", __FUNCTION__,
              m_vecCuts.size(), CURL::GetRedacted(beyondTVFilename));
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
  const std::vector<Cut> cutlist = PVR::CPVREdl::GetCuts(fileItem);
  for (const auto& cut : cutlist)
  {
    switch (cut.action)
    {
      case Action::CUT:
      case Action::MUTE:
      case Action::COMM_BREAK:
        if (AddCut(cut))
        {
          CLog::Log(LOGDEBUG, "{} - Added break [{} - {}] found in PVR item for: {}.", __FUNCTION__,
                    MillisecondsToTimeString(cut.start), MillisecondsToTimeString(cut.end),
                    CURL::GetRedacted(fileItem.GetDynPath()));
        }
        else
        {
          CLog::Log(LOGERROR,
                    "{} - Invalid break [{} - {}] found in PVR item for: {}. Continuing anyway.",
                    __FUNCTION__, MillisecondsToTimeString(cut.start),
                    MillisecondsToTimeString(cut.end), CURL::GetRedacted(fileItem.GetDynPath()));
        }
        break;

      case Action::SCENE:
        if (!AddSceneMarker(cut.end))
        {
          CLog::Log(LOGWARNING, "{} - Error adding scene marker for PVR item", __FUNCTION__);
        }
        break;

      default:
        CLog::Log(LOGINFO, "{} - Ignoring entry of unknown cut action: {}", __FUNCTION__,
                  static_cast<int>(cut.action));
        break;
    }
  }

  return !cutlist.empty();
}

bool CEdl::AddCut(const Cut& newCut)
{
  Cut cut = newCut;

  if (cut.action != Action::CUT && cut.action != Action::MUTE && cut.action != Action::COMM_BREAK)
  {
    CLog::Log(LOGERROR,
              "{} - Not an Action::CUT, Action::MUTE, or Action::COMM_BREAK! [{} - {}], {}",
              __FUNCTION__, MillisecondsToTimeString(cut.start), MillisecondsToTimeString(cut.end),
              static_cast<int>(cut.action));
    return false;
  }

  if (cut.start < 0)
  {
    CLog::Log(LOGERROR, "{} - Before start! [{} - {}], {}", __FUNCTION__,
              MillisecondsToTimeString(cut.start), MillisecondsToTimeString(cut.end),
              static_cast<int>(cut.action));
    return false;
  }

  if (cut.start >= cut.end)
  {
    CLog::Log(LOGERROR, "{} - Times are around the wrong way or the same! [{} - {}], {}",
              __FUNCTION__, MillisecondsToTimeString(cut.start), MillisecondsToTimeString(cut.end),
              static_cast<int>(cut.action));
    return false;
  }

  if (InCut(cut.start) || InCut(cut.end))
  {
    CLog::Log(LOGERROR, "{} - Start or end is in an existing cut! [{} - {}], {}", __FUNCTION__,
              MillisecondsToTimeString(cut.start), MillisecondsToTimeString(cut.end),
              static_cast<int>(cut.action));
    return false;
  }

  for (int i = 0; i < (int)m_vecCuts.size(); i++)
  {
    if (cut.start < m_vecCuts[i].start && cut.end > m_vecCuts[i].end)
    {
      CLog::Log(LOGERROR, "{} - Cut surrounds an existing cut! [{} - {}], {}", __FUNCTION__,
                MillisecondsToTimeString(cut.start), MillisecondsToTimeString(cut.end),
                static_cast<int>(cut.action));
      return false;
    }
  }

  if (cut.action == Action::COMM_BREAK)
  {
    /*
     * Detection isn't perfect near the edges of commercial breaks so automatically wait for a bit at
     * the start (autowait) and automatically rewind by a bit (autowind) at the end of the commercial
     * break.
     */
    int autowait = CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_iEdlCommBreakAutowait * 1000; // seconds -> ms
    int autowind = CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_iEdlCommBreakAutowind * 1000; // seconds -> ms

    if (cut.start > 0) // Only autowait if not at the start.
    {
      /* get the cut length so we don't start skipping after the end */
      int cutLength = cut.end - cut.start;
      /* add the lesser of the cut length or the autowait to the start */
      cut.start += autowait > cutLength ? cutLength : autowait;
    }
    if (cut.end > cut.start) // Only autowind if there is any cut time remaining.
    {
      /* get the remaining cut length so we don't rewind to before the start */
      int cutLength = cut.end - cut.start;
      /* subtract the lesser of the cut length or the autowind from the end */
      cut.end -= autowind > cutLength ? cutLength : autowind;
    }
  }

  /*
   * Insert cut in the list in the right position (ALL algorithms assume cuts are in ascending order)
   */
  if (m_vecCuts.empty() || cut.start > m_vecCuts.back().start)
  {
    CLog::Log(LOGDEBUG, "{} - Pushing new cut to back [{} - {}], {}", __FUNCTION__,
              MillisecondsToTimeString(cut.start), MillisecondsToTimeString(cut.end),
              static_cast<int>(cut.action));
    m_vecCuts.push_back(cut);
  }
  else
  {
    std::vector<Cut>::iterator pCurrentCut;
    for (pCurrentCut = m_vecCuts.begin(); pCurrentCut != m_vecCuts.end(); ++pCurrentCut)
    {
      if (cut.start < pCurrentCut->start)
      {
        CLog::Log(LOGDEBUG, "{} - Inserting new cut [{} - {}], {}", __FUNCTION__,
                  MillisecondsToTimeString(cut.start), MillisecondsToTimeString(cut.end),
                  static_cast<int>(cut.action));
        m_vecCuts.insert(pCurrentCut, cut);
        break;
      }
    }
  }

  if (cut.action == Action::CUT)
    m_iTotalCutTime += cut.end - cut.start;

  return true;
}

bool CEdl::AddSceneMarker(const int iSceneMarker)
{
  Cut cut;

  if (InCut(iSceneMarker, &cut) && cut.action == Action::CUT) // Only works for current cuts.
    return false;

  CLog::Log(LOGDEBUG, "{} - Inserting new scene marker: {}", __FUNCTION__,
            MillisecondsToTimeString(iSceneMarker));
  m_vecSceneMarkers.push_back(iSceneMarker); // Unsorted

  return true;
}

bool CEdl::HasCut() const
{
  return !m_vecCuts.empty();
}

int CEdl::GetTotalCutTime() const
{
  return m_iTotalCutTime; // ms
}

int CEdl::RemoveCutTime(int iSeek) const
{
  if (!HasCut())
    return iSeek;

  /**
   * @todo Consider an optimization of using the (now unused) total cut time if the seek time
   * requested is later than the end of the last recorded cut. For example, when calculating the
   * total duration for display.
   */
  int iCutTime = 0;
  for (int i = 0; i < (int)m_vecCuts.size(); i++)
  {
    if (m_vecCuts[i].action == Action::CUT)
    {
      if (iSeek >= m_vecCuts[i].start && iSeek <= m_vecCuts[i].end) // Inside cut
        iCutTime += iSeek - m_vecCuts[i].start - 1; // Decrease cut length by 1ms to jump over end boundary.
      else if (iSeek >= m_vecCuts[i].start) // Cut has already been passed over.
        iCutTime += m_vecCuts[i].end - m_vecCuts[i].start;
    }
  }
  return iSeek - iCutTime;
}

double CEdl::RestoreCutTime(double dClock) const
{
  if (!HasCut())
    return dClock;

  double dSeek = dClock;
  for (int i = 0; i < (int)m_vecCuts.size(); i++)
  {
    if (m_vecCuts[i].action == Action::CUT && dSeek >= m_vecCuts[i].start)
      dSeek += static_cast<double>(m_vecCuts[i].end - m_vecCuts[i].start);
  }

  return dSeek;
}

bool CEdl::HasSceneMarker() const
{
  return !m_vecSceneMarkers.empty();
}

std::string CEdl::GetInfo() const
{
  std::string strInfo;
  if (HasCut())
  {
    int cutCount = 0, muteCount = 0, commBreakCount = 0;
    for (int i = 0; i < (int)m_vecCuts.size(); i++)
    {
      switch (m_vecCuts[i].action)
      {
      case Action::CUT:
        cutCount++;
        break;
      case Action::MUTE:
        muteCount++;
        break;
      case Action::COMM_BREAK:
        commBreakCount++;
        break;
      default:
        break;
      }
    }
    if (cutCount > 0)
      strInfo += StringUtils::Format("c{}", cutCount);
    if (muteCount > 0)
      strInfo += StringUtils::Format("m{}", muteCount);
    if (commBreakCount > 0)
      strInfo += StringUtils::Format("b{}", commBreakCount);
  }
  if (HasSceneMarker())
    strInfo += StringUtils::Format("s{0}", m_vecSceneMarkers.size());

  return strInfo;
}

bool CEdl::InCut(const int iSeek, Cut *pCut)
{
  for (int i = 0; i < (int)m_vecCuts.size(); i++)
  {
    if (iSeek < m_vecCuts[i].start) // Early exit if not even up to the cut start time.
      return false;

    if (iSeek >= m_vecCuts[i].start && iSeek <= m_vecCuts[i].end) // Inside cut.
    {
      if (pCut)
        *pCut = m_vecCuts[i];
      return true;
    }
  }

  return false;
}

int CEdl::GetLastCutTime() const
{
  return m_lastCutTime;
}

void CEdl::SetLastCutTime(const int iCutTime)
{
  m_lastCutTime = iCutTime;
}

bool CEdl::GetNearestCut(bool bPlus, const int iSeek, Cut *pCut) const
{
  if (bPlus)
  {
    // Searching forwards
    for (auto &cut : m_vecCuts)
    {
      if (iSeek >= cut.start && iSeek <= cut.end) // Inside cut.
      {
        if (pCut)
          *pCut = cut;
        return true;
      }
      else if (iSeek < cut.start) // before this cut
      {
        if (pCut)
          *pCut = cut;
        return true;
      }
    }
    return false;
  }
  else
  {
    // Searching backwards
    for (int i = (int)m_vecCuts.size() - 1; i >= 0; i--)
    {
      if (iSeek - 20000 >= m_vecCuts[i].start && iSeek <= m_vecCuts[i].end)
        // Inside cut. We ignore if we're closer to 20 seconds inside
      {
        if (pCut)
          *pCut = m_vecCuts[i];
        return true;
      }
      else if (iSeek > m_vecCuts[i].end) // after this cut
      {
        if (pCut)
          *pCut = m_vecCuts[i];
        return true;
      }
    }
    return false;
  }
}

bool CEdl::GetNextSceneMarker(bool bPlus, const int iClock, int *iSceneMarker)
{
  if (!HasSceneMarker())
    return false;

  int iSeek = RestoreCutTime(iClock);

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
  Cut cut;
  if (bFound && InCut(*iSceneMarker, &cut) && cut.action == Action::CUT)
    *iSceneMarker = cut.end;

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
  if (!m_vecCuts.empty()
  &&  m_vecCuts[0].action == Action::COMM_BREAK
  && (m_vecCuts[0].end - m_vecCuts[0].start) < 5 * 1000) // 5 seconds
  {
    CLog::Log(LOGDEBUG, "{} - Removing short commercial break at start [{} - {}]. <5 seconds",
              __FUNCTION__, MillisecondsToTimeString(m_vecCuts[0].start),
              MillisecondsToTimeString(m_vecCuts[0].end));
    m_vecCuts.erase(m_vecCuts.begin());
  }

  const std::shared_ptr<CAdvancedSettings> advancedSettings = CServiceBroker::GetSettingsComponent()->GetAdvancedSettings();
  if (advancedSettings->m_bEdlMergeShortCommBreaks)
  {
    for (int i = 0; i < static_cast<int>(m_vecCuts.size()) - 1; i++)
    {
      if ((m_vecCuts[i].action == Action::COMM_BREAK && m_vecCuts[i + 1].action == Action::COMM_BREAK)
          &&  (m_vecCuts[i + 1].end - m_vecCuts[i].start < advancedSettings->m_iEdlMaxCommBreakLength * 1000) // s to ms
          &&  (m_vecCuts[i + 1].start - m_vecCuts[i].end < advancedSettings->m_iEdlMaxCommBreakGap * 1000)) // s to ms
      {
        Cut commBreak;
        commBreak.action = Action::COMM_BREAK;
        commBreak.start = m_vecCuts[i].start;
        commBreak.end = m_vecCuts[i + 1].end;

        CLog::Log(
            LOGDEBUG, "{} - Consolidating commercial break [{} - {}] and [{} - {}] to: [{} - {}]",
            __FUNCTION__, MillisecondsToTimeString(m_vecCuts[i].start),
            MillisecondsToTimeString(m_vecCuts[i].end),
            MillisecondsToTimeString(m_vecCuts[i + 1].start),
            MillisecondsToTimeString(m_vecCuts[i + 1].end),
            MillisecondsToTimeString(commBreak.start), MillisecondsToTimeString(commBreak.end));

        /*
         * Erase old cuts and insert the new merged one.
         */
        m_vecCuts.erase(m_vecCuts.begin() + i, m_vecCuts.begin() + i + 2);
        m_vecCuts.insert(m_vecCuts.begin() + i, commBreak);

        i--; // Reduce i to see if the next break is also within the max commercial break length.
      }
    }

    /*
     * To cater for recordings that are started early and then have a commercial break identified
     * before the TV show starts, expand the first commercial break to the very beginning if it
     * starts within the maximum start gap. This is done outside of the consolidation to prevent
     * the maximum commercial break length being triggered.
     */
    if (!m_vecCuts.empty()
        &&  m_vecCuts[0].action == Action::COMM_BREAK
        &&  m_vecCuts[0].start < advancedSettings->m_iEdlMaxStartGap * 1000)
    {
      CLog::Log(LOGDEBUG, "{} - Expanding first commercial break back to start [{} - {}].",
                __FUNCTION__, MillisecondsToTimeString(m_vecCuts[0].start),
                MillisecondsToTimeString(m_vecCuts[0].end));
      m_vecCuts[0].start = 0;
    }

    /*
     * Remove any commercial breaks shorter than the minimum (unless at the start)
     */
    for (int i = 0; i < static_cast<int>(m_vecCuts.size()); i++)
    {
      if (m_vecCuts[i].action == Action::COMM_BREAK
          &&  m_vecCuts[i].start > 0
          && (m_vecCuts[i].end - m_vecCuts[i].start) < advancedSettings->m_iEdlMinCommBreakLength * 1000)
      {
        CLog::Log(
            LOGDEBUG, "{} - Removing short commercial break [{} - {}]. Minimum length: {} seconds",
            __FUNCTION__, MillisecondsToTimeString(m_vecCuts[i].start),
            MillisecondsToTimeString(m_vecCuts[i].end), advancedSettings->m_iEdlMinCommBreakLength);
        m_vecCuts.erase(m_vecCuts.begin() + i);

        i--;
      }
    }
  }

  /*
   * Add in scene markers at the start and end of the commercial breaks.
   */
  for (int i = 0; i < (int)m_vecCuts.size(); i++)
  {
    if (m_vecCuts[i].action == Action::COMM_BREAK)
    {
      if (m_vecCuts[i].start > 0) // Don't add a scene marker at the start.
        AddSceneMarker(m_vecCuts[i].start);
      AddSceneMarker(m_vecCuts[i].end);
    }
  }
}
