/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "Edl.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "filesystem/File.h"
#include "settings/AdvancedSettings.h"
#include "utils/log.h"
#include "utils/XBMCTinyXML.h"
#include "PlatformDefs.h"
#include "pvr/recordings/PVRRecordings.h"
#include "pvr/PVRManager.h"

#define COMSKIP_HEADER "FILE PROCESSING COMPLETE"
#define VIDEOREDO_HEADER "<Version>2"
#define VIDEOREDO_TAG_CUT "<Cut>"
#define VIDEOREDO_TAG_SCENE "<SceneMarker "

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
}

bool CEdl::ReadEditDecisionLists(const std::string& strMovie, const float fFrameRate, const int iHeight)
{
  /*
   * The frame rate hints returned from ffmpeg for the video stream do not appear to take into
   * account whether the content is interlaced. This affects the calculation to time offsets based
   * on frames per second as most commercial detection programs use full frames, which need two
   * interlaced fields to calculate a single frame so the actual frame rate is half.
   *
   * Adjust the frame rate using the detected frame rate or height to determine typical interlaced
   * content (obtained from http://en.wikipedia.org/wiki/Frame_rate)
   *
   * Note that this is a HACK and we should be able to get the frame rate from the source sending
   * back frame markers. However, this doesn't seem possible for MythTV.
   */
  float fFramesPerSecond;
  if (iHeight <= 480 && int(fFrameRate * 100) == 5994) // 59.940 fps = NTSC or 60i content except for 1280x720/60
  {
    fFramesPerSecond = fFrameRate / 2; // ~29.97f - division used to retain accuracy of original.
    CLog::Log(LOGDEBUG, "%s - Assuming NTSC or 60i interlaced content. Adjusted frames per second from %.3f (~59.940 fps) to %.3f",
              __FUNCTION__, fFrameRate, fFramesPerSecond);
  }
  else if (int(fFrameRate * 100) == 4795) // 47.952 fps = 24p -> NTSC conversion
  {
    fFramesPerSecond = fFrameRate / 2; // ~23.976f - division used to retain accuracy of original.
    CLog::Log(LOGDEBUG, "%s - Assuming 24p -> NTSC conversion interlaced content. Adjusted frames per second from %.3f (~47.952 fps) to %.3f",
              __FUNCTION__, fFrameRate, fFramesPerSecond);
  }
  else if (iHeight == 576 && fFrameRate > 30.0) // PAL @ 50.0fps rather than PAL @ 25.0 fps. Can't use direct fps check of 50.0 as this is valid for 720p
  {
    fFramesPerSecond = fFrameRate / 2; // ~25.0f - division used to retain accuracy of original.
    CLog::Log(LOGDEBUG, "%s - Assuming PAL interlaced content. Adjusted frames per second from %.3f (~50.00 fps) to %.3f",
              __FUNCTION__, fFrameRate, fFramesPerSecond);
  }
  else if (iHeight == 1080 && fFrameRate > 30.0) // Don't know of any 1080p content being broadcast at higher than 30.0 fps so assume 1080i
  {
    fFramesPerSecond = fFrameRate / 2;
    CLog::Log(LOGDEBUG, "%s - Assuming 1080i interlaced content. Adjusted frames per second from %.3f to %.3f",
              __FUNCTION__, fFrameRate, fFramesPerSecond);
  }
  else // Assume everything else is not interlaced, e.g. 720p.
    fFramesPerSecond = fFrameRate;

  bool bFound = false;

  /*
   * Only check for edit decision lists if the movie is on the local hard drive, or accessed over a
   * network share.
   */
  if ((URIUtils::IsHD(strMovie)  || 
       URIUtils::IsSmb(strMovie) || 
       URIUtils::IsNfs(strMovie))         &&
      !URIUtils::IsPVRRecording(strMovie) &&
      !URIUtils::IsInternetStream(strMovie))
  {
    CLog::Log(LOGDEBUG, "%s - Checking for edit decision lists (EDL) on local drive or remote share for: %s",
              __FUNCTION__, strMovie.c_str());

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

  /*
   * PVR Recordings
   */
  else if (URIUtils::IsPVRRecording(strMovie))
  {
    CLog::Log(LOGDEBUG, "%s - Checking for edit decision list (EDL) for PVR recording: %s",
      __FUNCTION__, strMovie.c_str());

    bFound = ReadPvr(strMovie);
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
    CLog::Log(LOGERROR, "%s - Could not open EDL file: %s", __FUNCTION__, edlFilename.c_str());
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
      CLog::Log(LOGWARNING, "%s - Error on line %i in EDL file: %s", __FUNCTION__, iLine, edlFilename.c_str());

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

    /*
     * For each of the first two fields read, parse based on whether it is a time string
     * (HH:MM:SS.sss), frame marker (#12345), or normal seconds string (123.45).
     */
    int64_t iCutStartEnd[2];
    for (int i = 0; i < 2; i++)
    {
      if (strFields[i].find(":") != std::string::npos) // HH:MM:SS.sss format
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
        iCutStartEnd[i] = (int64_t)(atol(strFields[i].substr(1).c_str()) / fFramesPerSecond * 1000); // frame number to ms
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
      cut.action = CUT;
      if (!AddCut(cut))
      {
        CLog::Log(LOGWARNING, "%s - Error adding cut from line %i in EDL file: %s", __FUNCTION__,
                  iLine, edlFilename.c_str());
        continue;
      }
      break;
    case 1:
      cut.action = MUTE;
      if (!AddCut(cut))
      {
        CLog::Log(LOGWARNING, "%s - Error adding mute from line %i in EDL file: %s", __FUNCTION__,
                  iLine, edlFilename.c_str());
        continue;
      }
      break;
    case 2:
      if (!AddSceneMarker(cut.end))
      {
        CLog::Log(LOGWARNING, "%s - Error adding scene marker from line %i in EDL file: %s",
                  __FUNCTION__, iLine, edlFilename.c_str());
        continue;
      }
      break;
    case 3:
      cut.action = COMM_BREAK;
      if (!AddCut(cut))
      {
        CLog::Log(LOGWARNING, "%s - Error adding commercial break from line %i in EDL file: %s",
                  __FUNCTION__, iLine, edlFilename.c_str());
        continue;
      }
      break;
    default:
      CLog::Log(LOGWARNING, "%s - Invalid action on line %i in EDL file: %s", __FUNCTION__, iLine,
                edlFilename.c_str());
      continue;
    }
  }

  if (bError) // Log last line warning, if there was one, since while loop will have terminated.
    CLog::Log(LOGWARNING, "%s - Error on line %i in EDL file: %s", __FUNCTION__, iLine, edlFilename.c_str());

  edlFile.Close();

  if (HasCut() || HasSceneMarker())
  {
    CLog::Log(LOGDEBUG, "%s - Read %" PRIuS" cuts and %" PRIuS" scene markers in EDL file: %s", __FUNCTION__,
              m_vecCuts.size(), m_vecSceneMarkers.size(), edlFilename.c_str());
    return true;
  }
  else
  {
    CLog::Log(LOGDEBUG, "%s - No cuts or scene markers found in EDL file: %s", __FUNCTION__,
              edlFilename.c_str());
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
    CLog::Log(LOGERROR, "%s - Could not open Comskip file: %s", __FUNCTION__, comskipFilename.c_str());
    return false;
  }

  char szBuffer[1024];
  if (comskipFile.ReadString(szBuffer, 1023)
  &&  strncmp(szBuffer, COMSKIP_HEADER, strlen(COMSKIP_HEADER)) != 0) // Line 1.
  {
    CLog::Log(LOGERROR, "%s - Invalid Comskip file: %s. Error reading line 1 - expected '%s' at start.",
              __FUNCTION__, comskipFilename.c_str(), COMSKIP_HEADER);
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
    fFrameRate = fFramesPerSecond;
    CLog::Log(LOGWARNING, "%s - Frame rate not in Comskip file. Using detected frames per second: %.3f",
              __FUNCTION__, fFrameRate);
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
      cut.start = (int64_t)(dStartFrame / fFrameRate * 1000);
      cut.end = (int64_t)(dEndFrame / fFrameRate * 1000);
      cut.action = COMM_BREAK;
      bValid = AddCut(cut);
    }
    else
      bValid = false;
  }
  comskipFile.Close();

  if (!bValid)
  {
    CLog::Log(LOGERROR, "%s - Invalid Comskip file: %s. Error on line %i. Clearing any valid commercial breaks found.",
              __FUNCTION__, comskipFilename.c_str(), iLine);
    Clear();
    return false;
  }
  else if (HasCut())
  {
    CLog::Log(LOGDEBUG, "%s - Read %" PRIuS" commercial breaks from Comskip file: %s", __FUNCTION__, m_vecCuts.size(),
              comskipFilename.c_str());
    return true;
  }
  else
  {
    CLog::Log(LOGDEBUG, "%s - No commercial breaks found in Comskip file: %s", __FUNCTION__, comskipFilename.c_str());
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
    CLog::Log(LOGERROR, "%s - Could not open VideoReDo file: %s", __FUNCTION__, videoReDoFilename.c_str());
    return false;
  }

  char szBuffer[1024];
  if (videoReDoFile.ReadString(szBuffer, 1023)
  &&  strncmp(szBuffer, VIDEOREDO_HEADER, strlen(VIDEOREDO_HEADER)) != 0)
  {
    CLog::Log(LOGERROR, "%s - Invalid VideoReDo file: %s. Error reading line 1 - expected %s. Only version 2 files are supported.",
              __FUNCTION__, videoReDoFilename.c_str(), VIDEOREDO_HEADER);
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
        cut.action = CUT;
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
    CLog::Log(LOGERROR, "%s - Invalid VideoReDo file: %s. Error in line %i. Clearing any valid cuts or scenes found.",
              __FUNCTION__, videoReDoFilename.c_str(), iLine);
    Clear();
    return false;
  }
  else if (HasCut() || HasSceneMarker())
  {
    CLog::Log(LOGDEBUG, "%s - Read %" PRIuS" cuts and %" PRIuS" scene markers in VideoReDo file: %s", __FUNCTION__,
              m_vecCuts.size(), m_vecSceneMarkers.size(), videoReDoFilename.c_str());
    return true;
  }
  else
  {
    CLog::Log(LOGDEBUG, "%s - No cuts or scene markers found in VideoReDo file: %s", __FUNCTION__,
              videoReDoFilename.c_str());
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
    CLog::Log(LOGERROR, "%s - Could not load Beyond TV file: %s. %s", __FUNCTION__, beyondTVFilename.c_str(),
              xmlDoc.ErrorDesc());
    return false;
  }

  if (xmlDoc.Error())
  {
    CLog::Log(LOGERROR, "%s - Could not parse Beyond TV file: %s. %s", __FUNCTION__, beyondTVFilename.c_str(),
              xmlDoc.ErrorDesc());
    return false;
  }

  TiXmlElement *pRoot = xmlDoc.RootElement();
  if (!pRoot || strcmp(pRoot->Value(), "cutlist"))
  {
    CLog::Log(LOGERROR, "%s - Invalid Beyond TV file: %s. Expected root node to be <cutlist>", __FUNCTION__,
              beyondTVFilename.c_str());
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
      cut.action = COMM_BREAK;
      bValid = AddCut(cut);
    }
    else
      bValid = false;

    pRegion = pRegion->NextSiblingElement("Region");
  }
  if (!bValid)
  {
    CLog::Log(LOGERROR, "%s - Invalid Beyond TV file: %s. Clearing any valid commercial breaks found.", __FUNCTION__,
              beyondTVFilename.c_str());
    Clear();
    return false;
  }
  else if (HasCut())
  {
    CLog::Log(LOGDEBUG, "%s - Read %" PRIuS" commercial breaks from Beyond TV file: %s", __FUNCTION__, m_vecCuts.size(),
              beyondTVFilename.c_str());
    return true;
  }
  else
  {
    CLog::Log(LOGDEBUG, "%s - No commercial breaks found in Beyond TV file: %s", __FUNCTION__,
              beyondTVFilename.c_str());
    return false;
  }
}

bool CEdl::ReadPvr(const std::string &strMovie)
{
  if (!PVR::g_PVRManager.IsStarted())
  {
    CLog::Log(LOGERROR, "%s - PVR Manager not started, cannot read Edl for %s", __FUNCTION__, strMovie.c_str());
    return false;
  }

  CFileItemPtr tag =  PVR::g_PVRRecordings->GetByPath(strMovie);
  if (tag && tag->HasPVRRecordingInfoTag())
  {
    CLog::Log(LOGDEBUG, "%s - Reading Edl for recording: %s", __FUNCTION__, tag->GetPVRRecordingInfoTag()->m_strTitle.c_str());
  }
  else
  {
    CLog::Log(LOGERROR, "%s - Unable to find PVR recording: %s", __FUNCTION__, strMovie.c_str());
    return false;
  }

  std::vector<PVR_EDL_ENTRY> edl = tag->GetPVRRecordingInfoTag()->GetEdl();
  std::vector<PVR_EDL_ENTRY>::const_iterator it;
  for (it = edl.begin(); it != edl.end(); ++it)
  {
    Cut cut;
    cut.start = it->start;
    cut.end = it->end;

    switch (it->type)
    {
    case PVR_EDL_TYPE_CUT:
      cut.action = CUT;
      break;
    case PVR_EDL_TYPE_MUTE:
      cut.action = MUTE;
      break;
    case PVR_EDL_TYPE_SCENE:
      if (!AddSceneMarker(cut.end))
      {
        CLog::Log(LOGWARNING, "%s - Error adding scene marker for pvr recording", __FUNCTION__);
      }
      continue;
    case PVR_EDL_TYPE_COMBREAK:
      cut.action = COMM_BREAK;
      break;
    default:
      CLog::Log(LOGINFO, "%s - Ignoring entry of unknown type: %d", __FUNCTION__, it->type);
      continue;
    }

    if (AddCut(cut))
    {
      CLog::Log(LOGDEBUG, "%s - Added break [%s - %s] found in PVRRecording for: %s.",
        __FUNCTION__, MillisecondsToTimeString(cut.start).c_str(),
        MillisecondsToTimeString(cut.end).c_str(), strMovie.c_str());
    }
    else
    {
      CLog::Log(LOGERROR, "%s - Invalid break [%s - %s] found in PVRRecording for: %s. Continuing anyway.",
        __FUNCTION__, MillisecondsToTimeString(cut.start).c_str(),
        MillisecondsToTimeString(cut.end).c_str(), strMovie.c_str());
    }
  }

 return !edl.empty();
}

bool CEdl::AddCut(Cut& cut)
{
  if (cut.action != CUT && cut.action != MUTE && cut.action != COMM_BREAK)
  {
    CLog::Log(LOGERROR, "%s - Not a CUT, MUTE, or COMM_BREAK! [%s - %s], %d", __FUNCTION__,
              MillisecondsToTimeString(cut.start).c_str(), MillisecondsToTimeString(cut.end).c_str(),
              cut.action);
    return false;
  }

  if (cut.start < 0)
  {
    CLog::Log(LOGERROR, "%s - Before start! [%s - %s], %d", __FUNCTION__,
              MillisecondsToTimeString(cut.start).c_str(), MillisecondsToTimeString(cut.end).c_str(),
              cut.action);
    return false;
  }

  if (cut.start >= cut.end)
  {
    CLog::Log(LOGERROR, "%s - Times are around the wrong way or the same! [%s - %s], %d", __FUNCTION__,
              MillisecondsToTimeString(cut.start).c_str(), MillisecondsToTimeString(cut.end).c_str(),
              cut.action);
    return false;
  }

  if (InCut(cut.start) || InCut(cut.end))
  {
    CLog::Log(LOGERROR, "%s - Start or end is in an existing cut! [%s - %s], %d", __FUNCTION__,
              MillisecondsToTimeString(cut.start).c_str(), MillisecondsToTimeString(cut.end).c_str(),
              cut.action);
    return false;
  }

  for (int i = 0; i < (int)m_vecCuts.size(); i++)
  {
    if (cut.start < m_vecCuts[i].start && cut.end > m_vecCuts[i].end)
    {
      CLog::Log(LOGERROR, "%s - Cut surrounds an existing cut! [%s - %s], %d", __FUNCTION__,
                MillisecondsToTimeString(cut.start).c_str(), MillisecondsToTimeString(cut.end).c_str(),
                cut.action);
      return false;
    }
  }

  if (cut.action == COMM_BREAK)
  {
    /*
     * Detection isn't perfect near the edges of commercial breaks so automatically wait for a bit at
     * the start (autowait) and automatically rewind by a bit (autowind) at the end of the commercial
     * break.
     */
    int autowait = g_advancedSettings.m_iEdlCommBreakAutowait * 1000; // seconds -> ms
    int autowind = g_advancedSettings.m_iEdlCommBreakAutowind * 1000; // seconds -> ms

    if (cut.start > 0) // Only autowait if not at the start.
     cut.start += autowait;
    if (cut.end > cut.start + autowind) // Only autowind if it won't go back past the start (should never happen).
     cut.end -= autowind;
  }

  /*
   * Insert cut in the list in the right position (ALL algorithms assume cuts are in ascending order)
   */
  if (m_vecCuts.empty() || cut.start > m_vecCuts.back().start)
  {
    CLog::Log(LOGDEBUG, "%s - Pushing new cut to back [%s - %s], %d", __FUNCTION__,
              MillisecondsToTimeString(cut.start).c_str(), MillisecondsToTimeString(cut.end).c_str(),
              cut.action);
    m_vecCuts.push_back(cut);
  }
  else
  {
    std::vector<Cut>::iterator pCurrentCut;
    for (pCurrentCut = m_vecCuts.begin(); pCurrentCut != m_vecCuts.end(); ++pCurrentCut)
    {
      if (cut.start < pCurrentCut->start)
      {
        CLog::Log(LOGDEBUG, "%s - Inserting new cut [%s - %s], %d", __FUNCTION__,
                  MillisecondsToTimeString(cut.start).c_str(), MillisecondsToTimeString(cut.end).c_str(),
                  cut.action);
        m_vecCuts.insert(pCurrentCut, cut);
        break;
      }
    }
  }

  if (cut.action == CUT)
    m_iTotalCutTime += cut.end - cut.start;

  return true;
}

bool CEdl::AddSceneMarker(const int iSceneMarker)
{
  Cut cut;

  if (InCut(iSceneMarker, &cut) && cut.action == CUT) // Only works for current cuts.
    return false;

  CLog::Log(LOGDEBUG, "%s - Inserting new scene marker: %s", __FUNCTION__,
            MillisecondsToTimeString(iSceneMarker).c_str());
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
   * @todo Consider an optimisation of using the (now unused) total cut time if the seek time
   * requested is later than the end of the last recorded cut. For example, when calculating the
   * total duration for display.
   */
  int iCutTime = 0;
  for (int i = 0; i < (int)m_vecCuts.size(); i++)
  {
    if (m_vecCuts[i].action == CUT)
    {
      if (iSeek >= m_vecCuts[i].start && iSeek <= m_vecCuts[i].end) // Inside cut
        iCutTime += iSeek - m_vecCuts[i].start - 1; // Decrease cut length by 1ms to jump over end boundary.
      else if (iSeek >= m_vecCuts[i].start) // Cut has already been passed over.
        iCutTime += m_vecCuts[i].end - m_vecCuts[i].start;
    }
  }
  return iSeek - iCutTime;
}

int CEdl::RestoreCutTime(int iClock) const
{
  if (!HasCut())
    return iClock;

  int iSeek = iClock;
  for (int i = 0; i < (int)m_vecCuts.size(); i++)
  {
    if (m_vecCuts[i].action == CUT && iSeek >= m_vecCuts[i].start)
      iSeek += m_vecCuts[i].end - m_vecCuts[i].start;
  }

  return iSeek;
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
      case CUT:
        cutCount++;
        break;
      case MUTE:
        muteCount++;
        break;
      case COMM_BREAK:
        commBreakCount++;
        break;
      }
    }
    if (cutCount > 0)
      strInfo += StringUtils::Format("c%i", cutCount);
    if (muteCount > 0)
      strInfo += StringUtils::Format("m%i", muteCount);
    if (commBreakCount > 0)
      strInfo += StringUtils::Format("b%i", commBreakCount);
  }
  if (HasSceneMarker())
    strInfo += StringUtils::Format("s%" PRIuS, m_vecSceneMarkers.size());

  return strInfo.empty() ? "-" : strInfo;
}

bool CEdl::InCut(const int iSeek, Cut *pCut) const
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

bool CEdl::GetNextSceneMarker(bool bPlus, const int iClock, int *iSceneMarker) const
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
  if (bFound && InCut(*iSceneMarker, &cut) && cut.action == CUT)
    *iSceneMarker = cut.end;

  return bFound;
}

std::string CEdl::MillisecondsToTimeString(const int iMilliseconds)
{
  std::string strTimeString = StringUtils::SecondsToTimeString((long)(iMilliseconds / 1000), TIME_FORMAT_HH_MM_SS); // milliseconds to seconds
  strTimeString += StringUtils::Format(".%03i", iMilliseconds % 1000);
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
  &&  m_vecCuts[0].action == COMM_BREAK
  && (m_vecCuts[0].end - m_vecCuts[0].start) < 5 * 1000) // 5 seconds
  {
    CLog::Log(LOGDEBUG, "%s - Removing short commercial break at start [%s - %s]. <5 seconds", __FUNCTION__,
              MillisecondsToTimeString(m_vecCuts[0].start).c_str(), MillisecondsToTimeString(m_vecCuts[0].end).c_str());
    m_vecCuts.erase(m_vecCuts.begin());
  }

  if (g_advancedSettings.m_bEdlMergeShortCommBreaks)
  {
    for (int i = 0; i < (int)m_vecCuts.size() - 1; i++)
    {
      if ((m_vecCuts[i].action == COMM_BREAK && m_vecCuts[i + 1].action == COMM_BREAK)
      &&  (m_vecCuts[i + 1].end - m_vecCuts[i].start < g_advancedSettings.m_iEdlMaxCommBreakLength * 1000) // s to ms
      &&  (m_vecCuts[i + 1].start - m_vecCuts[i].end < g_advancedSettings.m_iEdlMaxCommBreakGap * 1000)) // s to ms
      {
        Cut commBreak;
        commBreak.action = COMM_BREAK;
        commBreak.start = m_vecCuts[i].start;
        commBreak.end = m_vecCuts[i + 1].end;

        CLog::Log(LOGDEBUG, "%s - Consolidating commercial break [%s - %s] and [%s - %s] to: [%s - %s]", __FUNCTION__,
                  MillisecondsToTimeString(m_vecCuts[i].start).c_str(), MillisecondsToTimeString(m_vecCuts[i].end).c_str(),
                  MillisecondsToTimeString(m_vecCuts[i + 1].start).c_str(), MillisecondsToTimeString(m_vecCuts[i + 1].end).c_str(),
                  MillisecondsToTimeString(commBreak.start).c_str(), MillisecondsToTimeString(commBreak.end).c_str());

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
    &&  m_vecCuts[0].action == COMM_BREAK
    &&  m_vecCuts[0].start < g_advancedSettings.m_iEdlMaxStartGap * 1000)
    {
      CLog::Log(LOGDEBUG, "%s - Expanding first commercial break back to start [%s - %s].", __FUNCTION__,
                MillisecondsToTimeString(m_vecCuts[0].start).c_str(), MillisecondsToTimeString(m_vecCuts[0].end).c_str());
      m_vecCuts[0].start = 0;
    }

    /*
     * Remove any commercial breaks shorter than the minimum (unless at the start)
     */
    for (int i = 0; i < (int)m_vecCuts.size(); i++)
    {
      if (m_vecCuts[i].action == COMM_BREAK
      &&  m_vecCuts[i].start > 0
      && (m_vecCuts[i].end - m_vecCuts[i].start) < g_advancedSettings.m_iEdlMinCommBreakLength * 1000)
      {
        CLog::Log(LOGDEBUG, "%s - Removing short commercial break [%s - %s]. Minimum length: %i seconds", __FUNCTION__,
                  MillisecondsToTimeString(m_vecCuts[i].start).c_str(), MillisecondsToTimeString(m_vecCuts[i].end).c_str(),
                  g_advancedSettings.m_iEdlMinCommBreakLength);
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
    if (m_vecCuts[i].action == COMM_BREAK)
    {
      if (m_vecCuts[i].start > 0) // Don't add a scene marker at the start.
        AddSceneMarker(m_vecCuts[i].start);
      AddSceneMarker(m_vecCuts[i].end);
    }
  }
  return;
}
