/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "Edl.h"

#include "Edl/EdlParserFactory.h"
#include "FileItem.h"
#include "ServiceBroker.h"
#include "URL.h"
#include "cores/EdlEdit.h"
#include "settings/AdvancedSettings.h"
#include "settings/SettingsComponent.h"
#include "utils/StringUtils.h"
#include "utils/log.h"

using namespace std::chrono_literals;
using namespace EDL;

namespace
{
std::string FormatSourceLocation(const std::optional<EdlSourceLocation>& source)
{
  if (!source)
    return {};
  return source->index
             ? StringUtils::Format(" from {}:{}", CURL::GetRedacted(source->source), *source->index)
             : StringUtils::Format(" from {}", CURL::GetRedacted(source->source));
}
} // namespace

CEdl::CEdl()
{
  Clear();
}

void CEdl::Clear()
{
  m_vecEdits.clear();
  m_vecSceneMarkers.clear();
  m_totalCutTime = 0ms;
  m_lastEditTime = std::nullopt;
}

bool CEdl::ReadEditDecisionLists(const CFileItem& fileItem, float fps)
{
  Clear();

  CLog::LogF(LOGDEBUG, "Checking for edit decision lists (EDL) for: {}",
             CURL::GetRedacted(fileItem.GetDynPath()));

  for (const auto& parser : CEdlParserFactory::GetEdlParsersForItem(fileItem))
  {
    if (!parser->CanParse(fileItem))
      continue;

    CEdlParserResult result = parser->Parse(fileItem, fps);
    if (!result.IsEmpty())
      return ProcessParserResult(result);
  }

  return false;
}

bool CEdl::ProcessParserResult(const CEdlParserResult& result)
{
  for (const auto& entry : result.GetSceneMarkers())
  {
    if (AddSceneMarker(entry.marker))
    {
      CLog::LogF(LOGDEBUG, "Adding scene marker [{}]{}",
                 StringUtils::MillisecondsToTimeString(entry.marker),
                 FormatSourceLocation(entry.source));
    }
    else
    {
      CLog::LogF(LOGERROR, "Error adding scene marker{}", FormatSourceLocation(entry.source));
    }
  }

  for (const auto& entry : result.GetEdits())
  {
    if (AddEdit(entry.edit))
    {
      CLog::LogF(LOGDEBUG, "Added {} edit [{} - {}]{}.", ActionToString(entry.edit.action),
                 StringUtils::MillisecondsToTimeString(entry.edit.start),
                 StringUtils::MillisecondsToTimeString(entry.edit.end),
                 FormatSourceLocation(entry.source));
    }
    else
    {
      CLog::LogF(LOGERROR, "Error adding edit{}", FormatSourceLocation(entry.source));
    }
  }

  MergeShortCommBreaks();
  AddSceneMarkersAtStartAndEndOfEdits();
  return true;
}

bool CEdl::AddEdit(const Edit& newEdit)
{
  Edit edit = newEdit;

  if (edit.action != Action::CUT && edit.action != Action::MUTE &&
      edit.action != Action::COMM_BREAK)
  {
    CLog::LogF(LOGERROR, "Not an Action::CUT, Action::MUTE, or Action::COMM_BREAK! [{} - {}], {}",
               StringUtils::MillisecondsToTimeString(edit.start),
               StringUtils::MillisecondsToTimeString(edit.end), static_cast<int>(edit.action));
    return false;
  }

  if (edit.start < 0ms)
  {
    CLog::LogF(LOGERROR, "Before start! [{} - {}], {}",
               StringUtils::MillisecondsToTimeString(edit.start),
               StringUtils::MillisecondsToTimeString(edit.end), static_cast<int>(edit.action));
    return false;
  }

  if (edit.start >= edit.end)
  {
    CLog::LogF(LOGERROR, "Times are around the wrong way or the same! [{} - {}], {}",
               StringUtils::MillisecondsToTimeString(edit.start),
               StringUtils::MillisecondsToTimeString(edit.end), static_cast<int>(edit.action));
    return false;
  }

  if (InEdit(edit.start) || InEdit(edit.end))
  {
    CLog::LogF(LOGERROR, "Start or end is in an existing edit! [{} - {}], {}",
               StringUtils::MillisecondsToTimeString(edit.start),
               StringUtils::MillisecondsToTimeString(edit.end), static_cast<int>(edit.action));
    return false;
  }

  for (size_t i = 0; i < m_vecEdits.size(); ++i)
  {
    if (edit.start < m_vecEdits[i].start && edit.end > m_vecEdits[i].end)
    {
      CLog::LogF(LOGERROR, "Edit surrounds an existing edit! [{} - {}], {}",
                 StringUtils::MillisecondsToTimeString(edit.start),
                 StringUtils::MillisecondsToTimeString(edit.end), static_cast<int>(edit.action));
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
    auto autowait = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::seconds(
        CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_iEdlCommBreakAutowait));
    std::chrono::milliseconds autowind = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::seconds(CServiceBroker::GetSettingsComponent()
                                 ->GetAdvancedSettings()
                                 ->m_iEdlCommBreakAutowind));

    if (edit.start > 0ms) // Only autowait if not at the start.
    {
      /* get the edit length so we don't start skipping after the end */
      std::chrono::milliseconds editLength = edit.end - edit.start;
      /* add the lesser of the edit length or the autowait to the start */
      edit.start += autowait > editLength ? editLength : autowait;
    }
    if (edit.end > edit.start) // Only autowind if there is any edit time remaining.
    {
      /* get the remaining edit length so we don't rewind to before the start */
      std::chrono::milliseconds editLength = edit.end - edit.start;
      /* subtract the lesser of the edit length or the autowind from the end */
      edit.end -= autowind > editLength ? editLength : autowind;
    }
  }

  /*
   * Insert edit in the list in the right position (ALL algorithms assume edits are in ascending order)
   */
  if (m_vecEdits.empty() || edit.start > m_vecEdits.back().start)
  {
    CLog::LogF(LOGDEBUG, "Pushing new edit to back [{} - {}], {}",
               StringUtils::MillisecondsToTimeString(edit.start),
               StringUtils::MillisecondsToTimeString(edit.end), static_cast<int>(edit.action));
    m_vecEdits.emplace_back(edit);
  }
  else
  {
    std::vector<Edit>::iterator pCurrentEdit;
    for (pCurrentEdit = m_vecEdits.begin(); pCurrentEdit != m_vecEdits.end(); ++pCurrentEdit)
    {
      if (edit.start < pCurrentEdit->start)
      {
        CLog::LogF(LOGDEBUG, "Inserting new edit [{} - {}], {}",
                   StringUtils::MillisecondsToTimeString(edit.start),
                   StringUtils::MillisecondsToTimeString(edit.end), static_cast<int>(edit.action));
        m_vecEdits.insert(pCurrentEdit, edit);
        break;
      }
    }
  }

  if (edit.action == Action::CUT)
    m_totalCutTime += edit.end - edit.start;

  return true;
}

bool CEdl::AddSceneMarker(std::chrono::milliseconds iSceneMarker)
{
  const auto edit = InEdit(iSceneMarker);
  if (edit && edit.value()->action == Action::CUT) // Only works for current cuts.
    return false;

  CLog::LogF(LOGDEBUG, "Inserting new scene marker: {}",
             StringUtils::MillisecondsToTimeString(iSceneMarker));
  m_vecSceneMarkers.push_back(iSceneMarker); // Unsorted

  return true;
}

bool CEdl::HasEdits() const
{
  return !m_vecEdits.empty();
}

bool CEdl::HasCuts() const
{
  return m_totalCutTime > 0ms;
}

std::chrono::milliseconds CEdl::GetTotalCutTime() const
{
  return m_totalCutTime; // ms
}

const std::vector<EDL::Edit> CEdl::GetEditList() const
{
  // the sum of cut durations while we iterate over them
  // note: edits are ordered by start time
  std::chrono::milliseconds surpassedSumOfCutDurations{0ms};
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

    // subtract the duration of already surpassed cuts
    edit.start -= surpassedSumOfCutDurations;
    edit.end -= surpassedSumOfCutDurations;
    editList.emplace_back(edit);
  }

  return editList;
}

const std::vector<std::chrono::milliseconds> CEdl::GetCutMarkers() const
{
  std::chrono::milliseconds surpassedSumOfCutDurations{0};
  std::vector<std::chrono::milliseconds> cutList;
  for (const EDL::Edit& edit : m_vecEdits)
  {
    if (edit.action != Action::CUT)
      continue;

    cutList.emplace_back(edit.start - surpassedSumOfCutDurations);
    surpassedSumOfCutDurations += edit.end - edit.start;
  }
  return cutList;
}

const std::vector<std::chrono::milliseconds> CEdl::GetSceneMarkers() const
{
  std::vector<std::chrono::milliseconds> sceneMarkers;
  sceneMarkers.reserve(m_vecSceneMarkers.size());
  for (const std::chrono::milliseconds& scene : m_vecSceneMarkers)
  {
    sceneMarkers.emplace_back(GetTimeWithoutCuts(scene));
  }
  return sceneMarkers;
}

std::chrono::milliseconds CEdl::GetTimeWithoutCuts(std::chrono::milliseconds seek) const
{
  if (!HasCuts())
    return seek;

  std::chrono::milliseconds cutTime = 0ms;
  for (const EDL::Edit& edit : m_vecEdits)
  {
    if (edit.action != Action::CUT)
      continue;

    // inside cut
    if (seek >= edit.start && seek <= edit.end)
    {
      // decrease cut length by 1 ms to jump over the end boundary.
      cutTime += seek - edit.start - 1ms;
    }
    // cut has already been passed over
    else if (seek >= edit.start)
    {
      cutTime += edit.end - edit.start;
    }
  }
  return seek - cutTime;
}

std::chrono::milliseconds CEdl::GetTimeAfterRestoringCuts(std::chrono::milliseconds seek) const
{
  if (!HasCuts())
    return seek;

  for (const EDL::Edit& edit : m_vecEdits)
  {
    std::chrono::milliseconds cutDuration = edit.end - edit.start;
    // add 1 ms to jump over the start boundary
    if (edit.action == Action::CUT && seek > edit.start + 1ms)
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

std::optional<std::unique_ptr<EDL::Edit>> CEdl::InEdit(std::chrono::milliseconds seekTime)
{
  for (size_t i = 0; i < m_vecEdits.size(); ++i)
  {
    if (seekTime < m_vecEdits[i].start) // Early exit if not even up to the edit start time.
      return std::nullopt;

    if (seekTime >= m_vecEdits[i].start && seekTime <= m_vecEdits[i].end) // Inside edit.
      return std::make_unique<EDL::Edit>(m_vecEdits[i]);
  }

  return std::nullopt;
}

std::optional<std::chrono::milliseconds> CEdl::GetLastEditTime() const
{
  return m_lastEditTime;
}

void CEdl::SetLastEditTime(std::chrono::milliseconds editTime)
{
  m_lastEditTime = editTime;
}

void CEdl::ResetLastEditTime()
{
  m_lastEditTime = std::nullopt;
}

void CEdl::SetLastEditActionType(EDL::Action action)
{
  m_lastEditActionType = action;
}

EDL::Action CEdl::GetLastEditActionType() const
{
  return m_lastEditActionType;
}

std::optional<std::chrono::milliseconds> CEdl::GetNextSceneMarker(Direction direction,
                                                                  std::chrono::milliseconds clock)
{
  if (!HasSceneMarker())
    return std::nullopt;

  std::optional<std::chrono::milliseconds> sceneMarker;
  const std::chrono::milliseconds seekTime = GetTimeAfterRestoringCuts(clock);

  std::chrono::milliseconds diff =
      std::chrono::milliseconds(10 * 60 * 60 * 1000); // 10 hours to ms.

  if (direction == Direction::FORWARD) // Find closest scene forwards
  {
    for (int i = 0; i < (int)m_vecSceneMarkers.size(); i++)
    {
      if ((m_vecSceneMarkers[i] > seekTime) && ((m_vecSceneMarkers[i] - seekTime) < diff))
      {
        diff = m_vecSceneMarkers[i] - seekTime;
        sceneMarker = m_vecSceneMarkers[i];
      }
    }
  }
  else if (direction == Direction::BACKWARD) // Find closest scene backwards
  {
    for (int i = 0; i < (int)m_vecSceneMarkers.size(); i++)
    {
      if ((m_vecSceneMarkers[i] < seekTime) && ((seekTime - m_vecSceneMarkers[i]) < diff))
      {
        diff = seekTime - m_vecSceneMarkers[i];
        sceneMarker = m_vecSceneMarkers[i];
      }
    }
  }

  /*
   * If the scene marker is in a cut then return the end of the cut. Can't guarantee that this is
   * picked up when scene markers are added.
   */
  if (sceneMarker)
  {
    auto edit = InEdit(sceneMarker.value());
    if (edit && edit.value()->action == Action::CUT)
    {
      sceneMarker = edit.value()->end;
    }
  }

  return sceneMarker;
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
      (m_vecEdits[0].end - m_vecEdits[0].start) < 5s)
  {
    CLog::LogF(LOGDEBUG, "Removing short commercial break at start [{} - {}]. <5 seconds",
               StringUtils::MillisecondsToTimeString(m_vecEdits[0].start),
               StringUtils::MillisecondsToTimeString(m_vecEdits[0].end));
    m_vecEdits.erase(m_vecEdits.begin());
  }

  const std::shared_ptr<CAdvancedSettings> advancedSettings =
      CServiceBroker::GetSettingsComponent()->GetAdvancedSettings();
  if (advancedSettings->m_bEdlMergeShortCommBreaks)
  {
    for (size_t i = 0; i < m_vecEdits.size() - 1; ++i)
    {
      if ((m_vecEdits[i].action == Action::COMM_BREAK &&
           m_vecEdits[i + 1].action == Action::COMM_BREAK) &&
          (m_vecEdits[i + 1].end - m_vecEdits[i].start <
           std::chrono::duration_cast<std::chrono::milliseconds>(
               std::chrono::seconds(advancedSettings->m_iEdlMaxCommBreakLength))) &&
          (m_vecEdits[i + 1].start - m_vecEdits[i].end <
           std::chrono::duration_cast<std::chrono::milliseconds>(
               std::chrono::seconds(advancedSettings->m_iEdlMaxCommBreakGap))))
      {
        Edit commBreak;
        commBreak.action = Action::COMM_BREAK;
        commBreak.start = m_vecEdits[i].start;
        commBreak.end = m_vecEdits[i + 1].end;

        CLog::LogF(LOGDEBUG, "Consolidating commercial break [{} - {}] and [{} - {}] to: [{} - {}]",
                   StringUtils::MillisecondsToTimeString(m_vecEdits[i].start),
                   StringUtils::MillisecondsToTimeString(m_vecEdits[i].end),
                   StringUtils::MillisecondsToTimeString(m_vecEdits[i + 1].start),
                   StringUtils::MillisecondsToTimeString(m_vecEdits[i + 1].end),
                   StringUtils::MillisecondsToTimeString(commBreak.start),
                   StringUtils::MillisecondsToTimeString(commBreak.end));

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
        m_vecEdits[0].start < std::chrono::duration_cast<std::chrono::milliseconds>(
                                  std::chrono::seconds(advancedSettings->m_iEdlMaxStartGap)))
    {
      CLog::LogF(LOGDEBUG, "Expanding first commercial break back to start [{} - {}].",
                 StringUtils::MillisecondsToTimeString(m_vecEdits[0].start),
                 StringUtils::MillisecondsToTimeString(m_vecEdits[0].end));
      m_vecEdits[0].start = 0ms;
    }

    /*
     * Remove any commercial breaks shorter than the minimum (unless at the start)
     */
    for (size_t i = 0; i < m_vecEdits.size(); ++i)
    {
      if (m_vecEdits[i].action == Action::COMM_BREAK && m_vecEdits[i].start > 0ms &&
          (m_vecEdits[i].end - m_vecEdits[i].start) <
              std::chrono::duration_cast<std::chrono::milliseconds>(
                  std::chrono::seconds(advancedSettings->m_iEdlMinCommBreakLength)))
      {
        CLog::LogF(LOGDEBUG,
                   "Removing short commercial break [{} - {}]. Minimum length: {} seconds",
                   StringUtils::MillisecondsToTimeString(m_vecEdits[i].start),
                   StringUtils::MillisecondsToTimeString(m_vecEdits[i].end),
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
      if (edit.start > 0ms)
        AddSceneMarker(edit.start);
      AddSceneMarker(edit.end);
    }
  }
}
