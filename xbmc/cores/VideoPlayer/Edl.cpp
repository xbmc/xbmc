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

namespace
{
constexpr const char* ComskipHeader = "FILE PROCESSING COMPLETE";
constexpr const char* VideoredoHeader = "<Version>2";
constexpr const char* VideoredoTagCut = "<Cut>";
constexpr const char* VideoredoTagScene = "<SceneMarker ";
} // namespace

using namespace EDL;
using namespace XFILE;

CEdl::CEdl()
{
  Clear();
}

void CEdl::Clear()
{
  m_edits.clear();
  m_sceneMarkers.clear();
  m_totalCutTime = 0;
  m_lastEditTime = -1;
}

bool CEdl::ReadEditDecisionLists(const CFileItem& fileItem, float fps)
{
  bool found = false;

  // Only check for edit decision lists if the movie is on the local hard drive, or accessed over a
  // network share.
  const std::string& path = fileItem.GetDynPath();
  if ((URIUtils::IsHD(path) || URIUtils::IsOnLAN(path)) && !URIUtils::IsInternetStream(path))
  {
    CLog::LogF(LOGDEBUG,
               "Checking for edit decision lists (EDL) on local drive or remote share for: {}",
               CURL::GetRedacted(path));

    // Read any available file format until a valid EDL related file is found.
    if (!found)
      found = ReadVideoReDo(path);

    if (!found)
      found = ReadEdl(path, fps);

    if (!found)
      found = ReadComskip(path, fps);

    if (!found)
      found = ReadBeyondTV(path);
  }
  else
  {
    found = ReadPvr(fileItem);
  }

  if (found)
  {
    MergeShortCommBreaks();
    AddSceneMarkersAtStartAndEndOfEdits();
  }

  return found;
}

bool CEdl::ReadEdl(const std::string& path, float fps)
{
  Clear();

  std::string edlFilename{URIUtils::ReplaceExtension(path, ".edl")};
  if (!CFile::Exists(edlFilename))
    return false;

  CFile edlFile;
  if (!edlFile.Open(edlFilename))
  {
    CLog::LogF(LOGERROR, "Could not open EDL file: {}", CURL::GetRedacted(edlFilename));
    return false;
  }

  bool error{false};
  int line{0};
  std::array<char, 1024> buffer{};
  while (edlFile.ReadString(&buffer[0], 1024))
  {
    // Log any errors from previous run in the loop
    if (error)
      CLog::LogF(LOGWARNING, "Error on line {} in EDL file: {}", line,
                 CURL::GetRedacted(edlFilename));

    error = false;
    line++;

    std::array<char, 513> buffer1{};
    std::array<char, 513> buffer2{};
    int action{EDL::EDL_ACTION_NONE};

    int fieldsRead =
        sscanf(buffer.data(), "%512s %512s %i", buffer1.data(), buffer2.data(), &action);
    // Make sure we read the right number of fields
    if (fieldsRead != 2 && fieldsRead != 3)
    {
      error = true;
      continue;
    }

    std::array<std::string, 2> fields = {buffer1.data(), buffer2.data()};

    // If only 2 fields read, then assume it's a scene marker.
    if (fieldsRead == 2)
    {
      action = static_cast<int>(Action::SCENE);
      fields[1] = fields[0];
    }

    if (StringUtils::StartsWith(fields[0], "##"))
    {
      CLog::LogF(LOGDEBUG, "Skipping comment line {} in EDL file: {}", line,
                 CURL::GetRedacted(edlFilename));
      continue;
    }

    // For each of the first two fields read, parse based on whether it is a time string
    // (HH:MM:SS.sss), frame marker (#12345), or normal seconds string (123.45).
    std::array<int64_t, 2> editStartEnd{};
    for (size_t i = 0; i < fields.size(); i++)
    {
      // HH:MM:SS.sss format
      if (fields[i].find(':') != std::string::npos)
      {
        std::vector<std::string> fieldParts = StringUtils::Split(fields[i], '.');
        if (fieldParts.size() == 1) // No ms
        {
          editStartEnd[i] = StringUtils::TimeStringToSeconds(fieldParts[0]) *
                            static_cast<int64_t>(1000); // seconds to ms
        }
        // Has ms. Everything after the dot (.) is ms
        else if (fieldParts.size() == 2)
        {
          // Have to pad or truncate the ms portion to 3 characters before converting to ms.
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
              std::stoi(fieldParts[1]); // seconds to ms
        }
        else
        {
          error = true;
          continue;
        }
      }
      // #12345 format for frame number
      else if (fields[i][0] == '#')
      {
        if (fps > 0.0f)
        {
          editStartEnd[i] = static_cast<int64_t>(std::stol(fields[i].substr(1)) / fps *
                                                 1000); // frame number to ms
        }
        else
        {
          CLog::LogF(LOGERROR,
                     "Frame number not supported in EDL files when frame rate is "
                     "unavailable (ts) - supplied frame number: {}",
                     fields[i].substr(1));
          return false;
        }
      }
      // Plain old seconds in float format, e.g. 123.45
      else
      {
        editStartEnd[i] = static_cast<int64_t>(std::stod(fields[i]) * 1000); // seconds to ms
      }
    }

    // If there was an error in the for loop, ignore and continue with the next line
    if (error)
      continue;

    Edit edit;
    edit.start = editStartEnd[0];
    edit.end = editStartEnd[1];

    if (action < 0 || action >= static_cast<uint16_t>(EDL::edlActionDescription.size()))
    {
      CLog::LogF(LOGWARNING, "Invalid action on line {} in EDL file: {}", line,
                 CURL::GetRedacted(edlFilename));
      continue;
    }

    edit.action = static_cast<EDL::Action>(action);

    bool added{false};

    if (edit.action == Action::SCENE)
      added = AddSceneMarker(edit.end);
    else
      added = AddEdit(edit);

    if (!added)
    {
      CLog::LogF(LOGWARNING, "Error adding {} from line {} in EDL file: {}",
                 EDL::edlActionDescription.at(action), line, CURL::GetRedacted(edlFilename));
      continue;
    }
  }

  // Log last line warning, if there was one, since while loop will have terminated.
  if (error)
    CLog::Log(LOGWARNING, "Error on line {} in EDL file: {}", line, CURL::GetRedacted(edlFilename));

  edlFile.Close();

  if (HasEdits() || HasSceneMarker())
  {
    CLog::LogF(LOGDEBUG, "Read {} edits and {} scene markers in EDL file: {}", m_edits.size(),
               m_sceneMarkers.size(), CURL::GetRedacted(edlFilename));
    return true;
  }
  else
  {
    CLog::LogF(LOGDEBUG, "No edits or scene markers found in EDL file: {}",
               CURL::GetRedacted(edlFilename));
    return false;
  }
}

bool CEdl::ReadComskip(const std::string& path, float fps)
{
  Clear();

  std::string comskipFilename{URIUtils::ReplaceExtension(path, ".txt")};
  if (!CFile::Exists(comskipFilename))
    return false;

  CFile comskipFile;
  if (!comskipFile.Open(comskipFilename))
  {
    CLog::LogF(LOGERROR, "Could not open Comskip file: {}", CURL::GetRedacted(comskipFilename));
    return false;
  }

  std::array<char, 1024> buffer{};
  if (comskipFile.ReadString(buffer.data(), 1023) &&
      strncmp(buffer.data(), ComskipHeader, strlen(ComskipHeader)) != 0) // Line 1.
  {
    CLog::LogF(LOGERROR, "Invalid Comskip file: {}. Error reading line 1 - expected '{}' at start.",
               CURL::GetRedacted(comskipFilename), ComskipHeader);
    comskipFile.Close();
    return false;
  }

  int frames{0};
  float frameRate{0};
  if (sscanf(buffer.data(), "FILE PROCESSING COMPLETE %i FRAMES AT %f", &frames, &frameRate) != 2)
  {
    // Not all generated Comskip files have the frame rate information.
    if (fps > 0.0f)
    {
      frameRate = fps;
      CLog::LogF(LOGWARNING,
                 "Frame rate not in Comskip file. Using detected frames per "
                 "second: {:.3f}",
                 frameRate);
    }
    else
    {
      CLog::LogF(LOGERROR, "Frame rate is unavailable and also not in Comskip file (ts).");
      return false;
    }
  }
  else
  {
    // Reduce by factor of 100 to get fps.
    frameRate /= 100;
  }

  // Line 2. Ignore "-------------"
  if (!comskipFile.ReadString(buffer.data(), 1023))
    CLog::LogF(LOGERROR, "Failed to read comskip buffer");

  bool valid{true};
  int line{2};
  while (valid && comskipFile.ReadString(buffer.data(), 1023)) // Line 3 and onwards.
  {
    line++;
    double startFrame{0};
    double endFrame{0};
    if (sscanf(buffer.data(), "%lf %lf", &startFrame, &endFrame) == 2)
    {
      Edit edit;
      edit.start = static_cast<int64_t>(startFrame / static_cast<double>(frameRate) * 1000.0);
      edit.end = static_cast<int64_t>(endFrame / static_cast<double>(frameRate) * 1000.0);
      edit.action = Action::COMM_BREAK;
      valid = AddEdit(edit);
    }
    else
      valid = false;
  }
  comskipFile.Close();

  if (!valid)
  {
    CLog::LogF(LOGERROR,
               "Invalid Comskip file: {}. Error on line {}. Clearing any valid commercial "
               "breaks found.",
               CURL::GetRedacted(comskipFilename), line);
    Clear();
    return false;
  }
  else if (HasEdits())
  {
    CLog::LogF(LOGDEBUG, "Read {} commercial breaks from Comskip file: {}", m_edits.size(),
               CURL::GetRedacted(comskipFilename));
    return true;
  }
  else
  {
    CLog::LogF(LOGDEBUG, "No commercial breaks found in Comskip file: {}",
               CURL::GetRedacted(comskipFilename));
    return false;
  }
}

bool CEdl::ReadVideoReDo(const std::string& path)
{
  // VideoReDo file is strange. Tags are XML like, but it isn't an XML file.
  // http://www.videoredo.com/

  Clear();
  std::string videoReDoFilename{URIUtils::ReplaceExtension(path, ".Vprj")};
  if (!CFile::Exists(videoReDoFilename))
    return false;

  CFile videoReDoFile;
  if (!videoReDoFile.Open(videoReDoFilename))
  {
    CLog::LogF(LOGERROR, "Could not open VideoReDo file: {}", CURL::GetRedacted(videoReDoFilename));
    return false;
  }

  std::array<char, 1024> buffer{};
  if (videoReDoFile.ReadString(buffer.data(), 1023) &&
      strncmp(buffer.data(), VideoredoHeader, strlen(VideoredoHeader)) != 0)
  {
    CLog::LogF(LOGERROR,
               "Invalid VideoReDo file: {}. Error reading line 1 - expected {}. Only version 2 "
               "files are supported.",
               CURL::GetRedacted(videoReDoFilename), VideoredoHeader);
    videoReDoFile.Close();
    return false;
  }

  int line{1};
  bool valid{true};
  while (valid && videoReDoFile.ReadString(buffer.data(), 1023))
  {
    line++;
    if (strncmp(buffer.data(), VideoredoTagCut, strlen(VideoredoTagCut)) == 0)
    {
      // Found the <Cut> tag
      // double is used as 32 bit float would overflow.
      double start{0};
      double end{0};
      if (sscanf(buffer.data() + strlen(VideoredoTagCut), "%lf:%lf", &start, &end) == 2)
      {
        // Times need adjusting by 1/10,000 to get ms.
        Edit edit;
        edit.start = static_cast<int64_t>(start / 10000);
        edit.end = static_cast<int64_t>(end / 10000);
        edit.action = Action::CUT;
        valid = AddEdit(edit);
      }
      else
        valid = false;
    }
    else if (strncmp(buffer.data(), VideoredoTagScene, strlen(VideoredoTagScene)) == 0)
    {
      // Found the <SceneMarker > tag
      int scene{0};
      double sceneMarker{0};
      if (sscanf(buffer.data() + strlen(VideoredoTagScene), " %i>%lf", &scene, &sceneMarker) == 2)
      {
        // Times need adjusting by 1/10,000 to get ms.
        valid = AddSceneMarker(static_cast<int64_t>(sceneMarker / 10000));
      }
      else
      {
        valid = false;
      }
    }
    // Ignore any other tags.
  }
  videoReDoFile.Close();

  if (!valid)
  {
    CLog::LogF(LOGERROR,
               "Invalid VideoReDo file: {}. Error in line {}. Clearing any valid edits or "
               "scenes found.",
               CURL::GetRedacted(videoReDoFilename), line);
    Clear();
    return false;
  }
  else if (HasEdits() || HasSceneMarker())
  {
    CLog::Log(LOGDEBUG, "Read {} edits and {} scene markers in VideoReDo file: {}", m_edits.size(),
              m_sceneMarkers.size(), CURL::GetRedacted(videoReDoFilename));
    return true;
  }
  else
  {
    CLog::LogF(LOGDEBUG, "No edits or scene markers found in VideoReDo file: {}",
               CURL::GetRedacted(videoReDoFilename));
    return false;
  }
}

bool CEdl::ReadBeyondTV(const std::string& path)
{
  Clear();

  std::string beyondTVFilename{
      URIUtils::ReplaceExtension(path, URIUtils::GetExtension(path) + ".chapters.xml")};
  if (!CFile::Exists(beyondTVFilename))
    return false;

  CXBMCTinyXML xmlDoc;
  if (!xmlDoc.LoadFile(beyondTVFilename))
  {
    CLog::LogF(LOGERROR, "Could not load Beyond TV file: {}. {}",
               CURL::GetRedacted(beyondTVFilename), xmlDoc.ErrorDesc());
    return false;
  }

  if (xmlDoc.Error())
  {
    CLog::LogF(LOGERROR, "Could not parse Beyond TV file: {}. {}",
               CURL::GetRedacted(beyondTVFilename), xmlDoc.ErrorDesc());
    return false;
  }

  TiXmlElement* rootElement = xmlDoc.RootElement();
  if (!rootElement || strcmp(rootElement->Value(), "cutlist"))
  {
    CLog::LogF(LOGERROR, "Invalid Beyond TV file: {}. Expected root node to be <cutlist>",
               CURL::GetRedacted(beyondTVFilename));
    return false;
  }

  bool valid{true};
  TiXmlElement* regionElement = rootElement->FirstChildElement("Region");
  while (valid && regionElement)
  {
    TiXmlElement* startElement = regionElement->FirstChildElement("start");
    TiXmlElement* endElement = regionElement->FirstChildElement("end");
    if (startElement && endElement && startElement->FirstChild() && endElement->FirstChild())
    {
      // Need to divide the start and end times by a factor of 10,000 to get msec.
      // E.g. <start comment="00:02:44.9980867">1649980867</start>
      // Use atof so doesn't overflow 32 bit float or integer / long.
      // E.g. <end comment="0:26:49.0000009">16090090000</end>
      // Don't use atoll even though it is more correct as it isn't natively supported by
      // Visual Studio.
      // atof() returns 0 if there were any problems and will subsequently be rejected in AddEdit().
      Edit edit;
      edit.start = static_cast<int64_t>((std::atof(startElement->FirstChild()->Value()) / 10000));
      edit.end = static_cast<int64_t>((std::atof(endElement->FirstChild()->Value()) / 10000));
      edit.action = Action::COMM_BREAK;
      valid = AddEdit(edit);
    }
    else
      valid = false;

    regionElement = regionElement->NextSiblingElement("Region");
  }
  if (!valid)
  {
    CLog::LogF(LOGERROR, "Invalid Beyond TV file: {}. Clearing any valid commercial breaks found.",
               CURL::GetRedacted(beyondTVFilename));
    Clear();
    return false;
  }
  else if (HasEdits())
  {
    CLog::LogF(LOGDEBUG, "Read {} commercial breaks from Beyond TV file: {}", m_edits.size(),
               CURL::GetRedacted(beyondTVFilename));
    return true;
  }
  else
  {
    CLog::LogF(LOGDEBUG, "No commercial breaks found in Beyond TV file: {}",
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
          CLog::LogF(LOGDEBUG, "Added break [{} - {}] found in PVR item for: {}.",
                     MillisecondsToTimeString(edit.start), MillisecondsToTimeString(edit.end),
                     CURL::GetRedacted(fileItem.GetDynPath()));
        }
        else
        {
          CLog::LogF(LOGERROR,
                     "Invalid break [{} - {}] found in PVR item for: {}. Continuing anyway.",
                     MillisecondsToTimeString(edit.start), MillisecondsToTimeString(edit.end),
                     CURL::GetRedacted(fileItem.GetDynPath()));
        }
        break;

      case Action::SCENE:
        if (!AddSceneMarker(edit.end))
        {
          CLog::LogF(LOGWARNING, "Error adding scene marker for PVR item");
        }
        break;

      default:
        CLog::LogF(LOGINFO, "Ignoring entry of unknown edit action: {}",
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
    CLog::LogF(LOGERROR, "Not an Action::CUT, Action::MUTE, or Action::COMM_BREAK! [{} - {}], {}",
               MillisecondsToTimeString(edit.start), MillisecondsToTimeString(edit.end),
               static_cast<int>(edit.action));
    return false;
  }

  if (edit.start < 0)
  {
    CLog::LogF(LOGERROR, "Before start! [{} - {}], {}", MillisecondsToTimeString(edit.start),
               MillisecondsToTimeString(edit.end), static_cast<int>(edit.action));
    return false;
  }

  if (edit.start >= edit.end)
  {
    CLog::LogF(LOGERROR, "Times are around the wrong way or the same! [{} - {}], {}",
               MillisecondsToTimeString(edit.start), MillisecondsToTimeString(edit.end),
               static_cast<int>(edit.action));
    return false;
  }

  if (InEdit(edit.start) || InEdit(edit.end))
  {
    CLog::LogF(LOGERROR, "Start or end is in an existing edit! [{} - {}], {}",
               MillisecondsToTimeString(edit.start), MillisecondsToTimeString(edit.end),
               static_cast<int>(edit.action));
    return false;
  }

  for (const EDL::Edit& editItem : m_edits)
  {
    if (edit.start < editItem.start && edit.end > editItem.end)
    {
      CLog::LogF(LOGERROR, "Edit surrounds an existing edit! [{} - {}], {}",
                 MillisecondsToTimeString(edit.start), MillisecondsToTimeString(edit.end),
                 static_cast<int>(edit.action));
      return false;
    }
  }

  if (edit.action == Action::COMM_BREAK)
  {
    // Detection isn't perfect near the edges of commercial breaks so automatically wait for a bit at
    // the start (autowait) and automatically rewind by a bit (autowind) at the end of the commercial
    // break.
    int autowait = CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_iEdlCommBreakAutowait * 1000; // seconds -> ms
    int autowind = CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_iEdlCommBreakAutowind * 1000; // seconds -> ms

    if (edit.start > 0) // Only autowait if not at the start.
    {
      // Get the edit length so we don't start skipping after the end
      int editLength = edit.end - edit.start;
      // Add the lesser of the edit length or the autowait to the start
      edit.start += autowait > editLength ? editLength : autowait;
    }
    if (edit.end > edit.start) // Only autowind if there is any edit time remaining.
    {
      // Get the remaining edit length so we don't rewind to before the start
      int editLength = edit.end - edit.start;
      // Subtract the lesser of the edit length or the autowind from the end
      edit.end -= autowind > editLength ? editLength : autowind;
    }
  }

  // Insert edit in the list in the right position (ALL algorithms assume edits are in ascending order)
  if (m_edits.empty() || edit.start > m_edits.back().start)
  {
    CLog::LogF(LOGDEBUG, "Pushing new edit to back [{} - {}], {}",
               MillisecondsToTimeString(edit.start), MillisecondsToTimeString(edit.end),
               static_cast<int>(edit.action));
    m_edits.emplace_back(edit);
  }
  else
  {
    std::vector<Edit>::iterator currentEdit;
    for (currentEdit = m_edits.begin(); currentEdit != m_edits.end(); ++currentEdit)
    {
      if (edit.start < currentEdit->start)
      {
        CLog::LogF(LOGDEBUG, "Inserting new edit [{} - {}], {}",
                   MillisecondsToTimeString(edit.start), MillisecondsToTimeString(edit.end),
                   static_cast<int>(edit.action));
        m_edits.insert(currentEdit, edit);
        break;
      }
    }
  }

  if (edit.action == Action::CUT)
    m_totalCutTime += edit.end - edit.start;

  return true;
}

bool CEdl::AddSceneMarker(int sceneMarker)
{
  Edit edit;

  if (InEdit(sceneMarker, &edit) && edit.action == Action::CUT) // Only works for current cuts.
    return false;

  CLog::LogF(LOGDEBUG, "Inserting new scene marker: {}", MillisecondsToTimeString(sceneMarker));
  m_sceneMarkers.push_back(sceneMarker); // Unsorted

  return true;
}

bool CEdl::HasEdits() const
{
  return !m_edits.empty();
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
  for (EDL::Edit edit : m_edits)
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
  for (const EDL::Edit& edit : m_edits)
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
  for (const int& scene : m_sceneMarkers)
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
  for (const EDL::Edit& edit : m_edits)
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

  for (const EDL::Edit& edit : m_edits)
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
  return !m_sceneMarkers.empty();
}

bool CEdl::InEdit(int seekTime, Edit* edit)
{
  for (const EDL::Edit& editItem : m_edits)
  {
    // Early exit if not even up to the edit start time.
    if (seekTime < editItem.start)
      return false;

    // Inside edit.
    if (seekTime >= editItem.start && seekTime <= editItem.end)
    {
      if (edit)
        *edit = editItem;
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

bool CEdl::GetNextSceneMarker(bool forward, int clock, int* sceneMarker)
{
  if (!HasSceneMarker())
    return false;

  int seekTime = GetTimeAfterRestoringCuts(clock);

  int diff = 10 * 60 * 60 * 1000; // 10 hours to ms.
  bool found = false;

  // Find closest scene forwards
  if (forward)
  {
    for (const int& scene : m_sceneMarkers)
    {
      if ((scene > seekTime) && ((scene - seekTime) < diff))
      {
        diff = scene - seekTime;
        *sceneMarker = scene;
        found = true;
      }
    }
  }
  // Find closest scene backwards
  else
  {
    for (const int& scene : m_sceneMarkers)
    {
      if ((scene < seekTime) && ((seekTime - scene) < diff))
      {
        diff = seekTime - scene;
        *sceneMarker = scene;
        found = true;
      }
    }
  }

  // If the scene marker is in a cut then return the end of the cut. Can't guarantee that this is
  // picked up when scene markers are added.
  Edit edit;
  if (found && InEdit(*sceneMarker, &edit) && edit.action == Action::CUT)
    *sceneMarker = edit.end;

  return found;
}

std::string CEdl::MillisecondsToTimeString(int msec)
{
  // milliseconds to seconds
  std::string timeString =
      StringUtils::SecondsToTimeString(static_cast<long>(msec / 1000), TIME_FORMAT_HH_MM_SS);
  timeString += StringUtils::Format(".{:03}", msec % 1000);
  return timeString;
}

void CEdl::MergeShortCommBreaks()
{
  // mythcommflag routinely seems to put a 20-40ms commercial break at the start of the recording.
  // Remove any spurious short commercial breaks at the very start so they don't interfere with
  // the algorithms below.
  if (!m_edits.empty() && m_edits.front().action == Action::COMM_BREAK &&
      (m_edits.front().end - m_edits.front().start) < 5 * 1000) // 5 seconds
  {
    CLog::LogF(LOGDEBUG, "Removing short commercial break at start [{} - {}]. <5 seconds",
               MillisecondsToTimeString(m_edits.front().start),
               MillisecondsToTimeString(m_edits.front().end));
    m_edits.erase(m_edits.begin());
  }

  const std::shared_ptr<CAdvancedSettings> advancedSettings = CServiceBroker::GetSettingsComponent()->GetAdvancedSettings();
  if (advancedSettings->m_bEdlMergeShortCommBreaks)
  {
    for (size_t i = 0; i < m_edits.size() - 1; ++i)
    {
      if ((m_edits[i].action == Action::COMM_BREAK &&
           m_edits[i + 1].action == Action::COMM_BREAK) &&
          (m_edits[i + 1].end - m_edits[i].start <
           advancedSettings->m_iEdlMaxCommBreakLength * 1000) // s to ms
          && (m_edits[i + 1].start - m_edits[i].end <
              advancedSettings->m_iEdlMaxCommBreakGap * 1000)) // s to ms
      {
        Edit commBreak;
        commBreak.action = Action::COMM_BREAK;
        commBreak.start = m_edits[i].start;
        commBreak.end = m_edits[i + 1].end;

        CLog::LogF(
            LOGDEBUG, "Consolidating commercial break [{} - {}] and [{} - {}] to: [{} - {}]",
            MillisecondsToTimeString(m_edits[i].start), MillisecondsToTimeString(m_edits[i].end),
            MillisecondsToTimeString(m_edits[i + 1].start),
            MillisecondsToTimeString(m_edits[i + 1].end), MillisecondsToTimeString(commBreak.start),
            MillisecondsToTimeString(commBreak.end));

        // Erase old edits and insert the new merged one.
        m_edits.erase(m_edits.begin() + i, m_edits.begin() + i + 2);
        m_edits.insert(m_edits.begin() + i, commBreak);

        // Reduce i to see if the next break is also within the max commercial break length.
        i--;
      }
    }

    // To cater for recordings that are started early and then have a commercial break identified
    // before the TV show starts, expand the first commercial break to the very beginning if it
    // starts within the maximum start gap. This is done outside of the consolidation to prevent
    // the maximum commercial break length being triggered.
    if (!m_edits.empty() && m_edits.front().action == Action::COMM_BREAK &&
        m_edits.front().start < advancedSettings->m_iEdlMaxStartGap * 1000)
    {
      CLog::Log(LOGDEBUG, "Expanding first commercial break back to start [{} - {}].",
                MillisecondsToTimeString(m_edits.front().start),
                MillisecondsToTimeString(m_edits.front().end));
      m_edits.front().start = 0;
    }

    // Remove any commercial breaks shorter than the minimum (unless at the start)
    for (size_t i = 0; i < m_edits.size(); ++i)
    {
      if (m_edits[i].action == Action::COMM_BREAK && m_edits[i].start > 0 &&
          (m_edits[i].end - m_edits[i].start) < advancedSettings->m_iEdlMinCommBreakLength * 1000)
      {
        CLog::LogF(
            LOGDEBUG, "Removing short commercial break [{} - {}]. Minimum length: {} seconds",
            MillisecondsToTimeString(m_edits[i].start), MillisecondsToTimeString(m_edits[i].end),
            advancedSettings->m_iEdlMinCommBreakLength);
        m_edits.erase(m_edits.begin() + i);

        i--;
      }
    }
  }
}

void CEdl::AddSceneMarkersAtStartAndEndOfEdits()
{
  for (const EDL::Edit& edit : m_edits)
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
