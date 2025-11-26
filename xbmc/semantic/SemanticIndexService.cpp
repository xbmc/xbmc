/*
 *  Copyright (C) 2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "SemanticIndexService.h"

#include "FileItem.h"
#include "FileItemList.h"
#include "SemanticDatabase.h"
#include "SemanticTypes.h"
#include "ServiceBroker.h"
#include "application/ApplicationComponents.h"
#include "application/ApplicationPowerHandling.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "ingest/MetadataParser.h"
#include "ingest/SubtitleParser.h"
#include "interfaces/AnnouncementManager.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "settings/lib/Setting.h"
#include "transcription/AudioExtractor.h"
#include "transcription/ITranscriptionProvider.h"
#include "transcription/TranscriptionProviderManager.h"
#include "utils/StringUtils.h"
#include "filesystem/Directory.h"
#include "filesystem/File.h"
#include "filesystem/SpecialProtocol.h"
#include "utils/URIUtils.h"
#include "utils/Variant.h"
#include "utils/log.h"
#include "video/VideoDatabase.h"
#include "video/VideoInfoTag.h"

#include <algorithm>
#include <array>

using namespace KODI::SEMANTIC;

CSemanticIndexService::CSemanticIndexService() : CThread("SemanticIndexService")
{
  CLog::Log(LOGDEBUG, "SemanticIndexService: Created");
}

CSemanticIndexService::~CSemanticIndexService()
{
  Stop();
  if (m_callbacksRegistered)
  {
    if (const auto settingsComponent = CServiceBroker::GetSettingsComponent())
    {
      if (const auto settings = settingsComponent->GetSettings())
        settings->UnregisterCallback(this);
    }
    m_callbacksRegistered = false;
  }
  CLog::Log(LOGDEBUG, "SemanticIndexService: Destroyed");
}

// ========== Service Lifecycle ==========

bool CSemanticIndexService::Start()
{
  if (m_running.load())
  {
    CLog::Log(LOGWARNING, "SemanticIndexService: Already running");
    return true;
  }

  CLog::Log(LOGINFO, "SemanticIndexService: Starting...");

  EnsureSettingsCallback();

  // Initialize database
  m_database = std::make_unique<CSemanticDatabase>();
  if (!m_database->Open())
  {
    CLog::Log(LOGERROR, "SemanticIndexService: Failed to open database");
    m_database.reset();
    return false;
  }

  // Initialize parsers
  m_subtitleParser = std::make_unique<CSubtitleParser>();
  m_metadataParser = std::make_unique<CMetadataParser>();

  // Initialize transcription manager
  m_transcriptionManager = std::make_unique<CTranscriptionProviderManager>();
  if (!m_transcriptionManager->Initialize(m_database.get()))
  {
    CLog::Log(LOGWARNING, "SemanticIndexService: Transcription manager initialization failed");
    // Not fatal - continue without transcription
  }

  // Load settings
  const auto settingsComponent = CServiceBroker::GetSettingsComponent();
  auto settings = settingsComponent->GetSettings();
  m_processMode = settings->GetString(CSettings::SETTING_SEMANTIC_PROCESSMODE);
  m_autoIndex = !StringUtils::EqualsNoCase(m_processMode, "manual");

  // Register for announcements
  CServiceBroker::GetAnnouncementManager()->AddAnnouncer(
      this, ANNOUNCEMENT::VideoLibrary | ANNOUNCEMENT::System);

  // Start background thread
  m_running.store(true);
  Create();

  CLog::Log(LOGINFO, "SemanticIndexService: Started successfully (mode: {})", m_processMode);

  // Queue any pending items if auto-indexing is enabled
  if (m_autoIndex)
  {
    QueueAllUnindexed();
  }

  return true;
}

void CSemanticIndexService::EnsureSettingsCallback()
{
  if (m_callbacksRegistered)
    return;

  const auto settingsComponent = CServiceBroker::GetSettingsComponent();
  if (!settingsComponent)
    return;

  auto settings = settingsComponent->GetSettings();
  if (!settings)
    return;

  settings->RegisterCallback(
      this, {CSettings::SETTING_SEMANTIC_ENABLED, CSettings::SETTING_SEMANTIC_PROCESSMODE,
             CSettings::SETTING_SEMANTIC_AUTOTRANSCRIBE});
  m_callbacksRegistered = true;
}

void CSemanticIndexService::Stop()
{
  if (!m_running.load())
    return;

  CLog::Log(LOGINFO, "SemanticIndexService: Stopping...");

  // Signal thread to stop
  m_running.store(false);
  m_queueCondition.notify_all();

  // Wait for thread to finish
  StopThread(true);

  // Unregister announcements
  CServiceBroker::GetAnnouncementManager()->RemoveAnnouncer(this);

  // Shutdown components
  if (m_transcriptionManager)
  {
    m_transcriptionManager->Shutdown();
    m_transcriptionManager.reset();
  }

  m_metadataParser.reset();
  m_subtitleParser.reset();

  if (m_database)
  {
    m_database->Close();
    m_database.reset();
  }

  CLog::Log(LOGINFO, "SemanticIndexService: Stopped");
}

// ========== Manual Indexing Control ==========

void CSemanticIndexService::QueueMedia(int mediaId, const std::string& mediaType, int priority)
{
  if (!m_running.load())
  {
    CLog::Log(LOGWARNING, "SemanticIndexService: Cannot queue - service not running");
    return;
  }

  std::lock_guard<std::mutex> lock(m_queueMutex);

  // Check if already queued
  if (IsInQueue(mediaId, mediaType))
  {
    CLog::Log(LOGDEBUG, "SemanticIndexService: {} {} already in queue", mediaType, mediaId);
    return;
  }

  // Add to queue
  QueueItem item{mediaId, mediaType, priority, false};
  m_queue.push_back(item);

  // Sort by priority (higher first)
  std::sort(m_queue.begin(), m_queue.end(),
            [](const QueueItem& a, const QueueItem& b) { return a.priority > b.priority; });

  CLog::Log(LOGDEBUG, "SemanticIndexService: Queued {} {} (priority: {}, queue length: {})",
            mediaType, mediaId, priority, m_queue.size());

  // Wake up processing thread
  m_queueCondition.notify_one();
}

void CSemanticIndexService::QueueAllUnindexed()
{
  if (!m_running.load())
  {
    CLog::Log(LOGWARNING, "SemanticIndexService: Cannot queue - service not running");
    return;
  }

  CLog::Log(LOGINFO, "SemanticIndexService: Queueing all unindexed media...");

  SeedMissingIndexStates();

  // Get pending items from database
  std::vector<SemanticIndexState> states;
  if (!m_database->GetPendingIndexStates(1000, states))
  {
    CLog::Log(LOGERROR, "SemanticIndexService: Failed to get pending index states");
    return;
  }

  CLog::Log(LOGINFO, "SemanticIndexService: Found {} unindexed items", states.size());

  // Queue each item
  for (const auto& state : states)
  {
    QueueMedia(state.mediaId, state.mediaType, state.priority);
  }
}

void CSemanticIndexService::QueueTranscription(int mediaId, const std::string& mediaType)
{
  if (!m_running.load())
  {
    CLog::Log(LOGWARNING, "SemanticIndexService: Cannot queue - service not running");
    return;
  }

  std::lock_guard<std::mutex> lock(m_queueMutex);

  // Check if already queued
  if (IsInQueue(mediaId, mediaType))
  {
    CLog::Log(LOGDEBUG, "SemanticIndexService: {} {} already in queue - setting transcribe flag",
              mediaType, mediaId);

    // Find and update the item
    for (auto& item : m_queue)
    {
      if (item.mediaId == mediaId && item.mediaType == mediaType)
      {
        item.transcribe = true;
        break;
      }
    }
    return;
  }

  // Add to queue with transcribe flag
  QueueItem item{mediaId, mediaType, 10 /* higher priority */, true /* transcribe */};
  m_queue.push_back(item);

  // Sort by priority
  std::sort(m_queue.begin(), m_queue.end(),
            [](const QueueItem& a, const QueueItem& b) { return a.priority > b.priority; });

  CLog::Log(LOGDEBUG, "SemanticIndexService: Queued {} {} for transcription (queue length: {})",
            mediaType, mediaId, m_queue.size());

  m_queueCondition.notify_one();
}

// ========== Cancel Operations ==========

void CSemanticIndexService::CancelMedia(int mediaId, const std::string& mediaType)
{
  std::lock_guard<std::mutex> lock(m_queueMutex);
  RemoveFromQueue(mediaId, mediaType);
  CLog::Log(LOGDEBUG, "SemanticIndexService: Cancelled {} {}", mediaType, mediaId);
}

void CSemanticIndexService::CancelAllPending()
{
  std::lock_guard<std::mutex> lock(m_queueMutex);
  const size_t count = m_queue.size();
  m_queue.clear();
  CLog::Log(LOGINFO, "SemanticIndexService: Cancelled {} pending items", count);
}

// ========== Status Queries ==========

bool CSemanticIndexService::IsMediaIndexed(int mediaId, const std::string& mediaType)
{
  if (!m_database)
    return false;

  SemanticIndexState state;
  if (!m_database->GetIndexState(mediaId, mediaType, state))
    return false;

  return state.subtitleStatus == IndexStatus::COMPLETED ||
         state.metadataStatus == IndexStatus::COMPLETED ||
         state.transcriptionStatus == IndexStatus::COMPLETED;
}

float CSemanticIndexService::GetProgress(int mediaId, const std::string& mediaType)
{
  if (!m_database)
    return -1.0f;

  SemanticIndexState state;
  if (!m_database->GetIndexState(mediaId, mediaType, state))
    return -1.0f;

  // Calculate average progress across all sources
  float total = 0.0f;
  int count = 0;

  if (state.subtitleStatus == IndexStatus::IN_PROGRESS)
  {
    total += 0.5f; // Subtitles are quick, assume 50% if in progress
    count++;
  }
  else if (state.subtitleStatus == IndexStatus::COMPLETED)
  {
    total += 1.0f;
    count++;
  }

  if (state.metadataStatus == IndexStatus::IN_PROGRESS)
  {
    total += 0.5f;
    count++;
  }
  else if (state.metadataStatus == IndexStatus::COMPLETED)
  {
    total += 1.0f;
    count++;
  }

  if (state.transcriptionStatus == IndexStatus::IN_PROGRESS)
  {
    total += state.transcriptionProgress;
    count++;
  }
  else if (state.transcriptionStatus == IndexStatus::COMPLETED)
  {
    total += 1.0f;
    count++;
  }

  return count > 0 ? total / count : 0.0f;
}

int CSemanticIndexService::GetQueueLength() const
{
  std::lock_guard<std::mutex> lock(m_queueMutex);
  return static_cast<int>(m_queue.size());
}

// ========== Callbacks ==========

void CSemanticIndexService::OnJobComplete(unsigned int jobID, bool success, CJob* job)
{
  CLog::Log(LOGDEBUG, "SemanticIndexService: Job {} completed (success: {})", jobID, success);
  // Transcription jobs will update database directly
}

void CSemanticIndexService::OnJobProgress(unsigned int jobID,
                                          unsigned int progress,
                                          unsigned int total,
                                          const CJob* job)
{
  // Update progress in database if this is current item
  std::lock_guard<std::mutex> lock(m_currentItemMutex);
  if (m_currentMediaId > 0)
  {
    float progressPercent = total > 0 ? static_cast<float>(progress) / total : 0.0f;
    CLog::Log(LOGDEBUG, "SemanticIndexService: Job {} progress: {}/{} ({:.1f}%)", jobID, progress,
              total, progressPercent * 100.0f);
  }
}

void CSemanticIndexService::OnSettingChanged(const std::shared_ptr<const CSetting>& setting)
{
  if (!setting)
    return;

  const std::string& settingId = setting->GetId();
  CLog::Log(LOGDEBUG, "SemanticIndexService: Setting changed: {}", settingId);

  if (settingId == CSettings::SETTING_SEMANTIC_ENABLED)
  {
    bool enabled = std::static_pointer_cast<const CSettingBool>(setting)->GetValue();
    if (enabled && !m_running.load())
    {
      Start();
    }
    else if (!enabled && m_running.load())
    {
      Stop();
    }
  }
  else if (settingId == CSettings::SETTING_SEMANTIC_PROCESSMODE)
  {
    bool previousAutoIndex;
    bool shouldQueueAll = false;
    {
      // Protect m_processMode access - it's read by processing thread in ShouldProcessNow()
      std::lock_guard<std::mutex> lock(m_queueMutex);
      m_processMode = std::static_pointer_cast<const CSettingString>(setting)->GetValue();
      CLog::Log(LOGINFO, "SemanticIndexService: Process mode changed to: {}", m_processMode);
      previousAutoIndex = m_autoIndex;
      m_autoIndex = !StringUtils::EqualsNoCase(m_processMode, "manual");
      shouldQueueAll = m_autoIndex && !previousAutoIndex;
    }

    if (shouldQueueAll)
    {
      CLog::Log(LOGINFO, "SemanticIndexService: Auto-index enabled via process mode");
      QueueAllUnindexed();
    }

    // Wake up thread to re-evaluate processing conditions
    m_queueCondition.notify_one();
  }
  else if (settingId == CSettings::SETTING_SEMANTIC_AUTOTRANSCRIBE)
  {
    bool transcribeEnabled = std::static_pointer_cast<const CSettingBool>(setting)->GetValue();
    CLog::Log(LOGINFO, "SemanticIndexService: Auto-transcription {}", transcribeEnabled ? "enabled"
                                                                                        : "disabled");
  }
}

void CSemanticIndexService::Announce(ANNOUNCEMENT::AnnouncementFlag flag,
                                     const std::string& sender,
                                     const std::string& message,
                                     const CVariant& data)
{
  if (flag == ANNOUNCEMENT::VideoLibrary)
  {
    OnLibraryUpdate(message, data);
  }
  else if (flag == ANNOUNCEMENT::System)
  {
    if (message == "OnQuit")
    {
      Stop();
    }
  }
}

// ========== Thread Processing ==========

void CSemanticIndexService::Process()
{
  CLog::Log(LOGINFO, "SemanticIndexService: Processing thread started");

  while (m_running.load())
  {
    std::unique_lock<std::mutex> lock(m_queueMutex);

    // Wait for work or timeout
    m_queueCondition.wait_for(lock, std::chrono::seconds(30), [this]() {
      return !m_running.load() || (!m_queue.empty() && ShouldProcessNow());
    });

    if (!m_running.load())
      break;

    // Check if we should process now
    if (m_queue.empty() || !ShouldProcessNow())
      continue;

    // Get next item
    QueueItem item = m_queue.front();
    m_queue.pop_front();
    lock.unlock();

    // Process the item
    ProcessItem(item);

    // Small delay between items to avoid overwhelming the system
    if (m_running.load())
    {
      Sleep(std::chrono::milliseconds(100));
    }
  }

  CLog::Log(LOGINFO, "SemanticIndexService: Processing thread stopped");
}

// ========== Processing Methods ==========

bool CSemanticIndexService::ProcessNextItem()
{
  std::lock_guard<std::mutex> lock(m_queueMutex);

  if (m_queue.empty())
    return false;

  QueueItem item = m_queue.front();
  m_queue.pop_front();

  ProcessItem(item);
  return true;
}

void CSemanticIndexService::ProcessItem(const QueueItem& item)
{
  CLog::Log(LOGINFO, "SemanticIndexService: Processing {} {} (transcribe: {})", item.mediaType,
            item.mediaId, item.transcribe);

  // Set current item
  {
    std::lock_guard<std::mutex> lock(m_currentItemMutex);
    m_currentMediaId = item.mediaId;
    m_currentMediaType = item.mediaType;
  }

  // Update status to in progress
  SemanticIndexState state;
  state.mediaId = item.mediaId;
  state.mediaType = item.mediaType;
  state.subtitleStatus = IndexStatus::IN_PROGRESS;
  state.metadataStatus = IndexStatus::IN_PROGRESS;
  state.transcriptionStatus =
      item.transcribe ? IndexStatus::IN_PROGRESS : IndexStatus::PENDING;
  m_database->UpdateIndexState(state);

  bool hasContent = false;

  // Index subtitles first (fast, no API cost)
  if (IndexSubtitles(item.mediaId, item.mediaType))
  {
    CLog::Log(LOGINFO, "SemanticIndexService: Subtitle indexing completed for {} {}",
              item.mediaType, item.mediaId);
    hasContent = true;
  }
  else
  {
    CLog::Log(LOGDEBUG, "SemanticIndexService: No subtitles found for {} {}", item.mediaType,
              item.mediaId);
  }

  // Index metadata (fast)
  if (IndexMetadata(item.mediaId, item.mediaType))
  {
    CLog::Log(LOGINFO, "SemanticIndexService: Metadata indexing completed for {} {}",
              item.mediaType, item.mediaId);
    hasContent = true;
  }
  else
  {
    CLog::Log(LOGDEBUG, "SemanticIndexService: Metadata indexing failed for {} {}",
              item.mediaType, item.mediaId);
  }

  // Transcription if requested and settings allow
  auto settings = CServiceBroker::GetSettingsComponent()->GetSettings();
  bool transcribeEnabled = settings->GetBool(CSettings::SETTING_SEMANTIC_AUTOTRANSCRIBE);

  if (item.transcribe && transcribeEnabled)
  {
    if (StartTranscription(item.mediaId, item.mediaType))
    {
      CLog::Log(LOGINFO, "SemanticIndexService: Transcription started for {} {}", item.mediaType,
                item.mediaId);
      hasContent = true;
    }
    else
    {
      CLog::Log(LOGWARNING, "SemanticIndexService: Transcription failed to start for {} {}",
                item.mediaType, item.mediaId);
    }
  }

  // Update final status
  if (m_database->GetIndexState(item.mediaId, item.mediaType, state))
  {
    // Only mark as completed if we got some content and nothing is in progress
    if (hasContent && state.subtitleStatus != IndexStatus::IN_PROGRESS &&
        state.metadataStatus != IndexStatus::IN_PROGRESS &&
        state.transcriptionStatus != IndexStatus::IN_PROGRESS)
    {
      state.chunkCount = 0; // Will be counted from database

      // Count chunks
      std::vector<SemanticChunk> chunks;
      if (m_database->GetChunksForMedia(item.mediaId, item.mediaType, chunks))
      {
        state.chunkCount = static_cast<int>(chunks.size());
      }

      m_database->UpdateIndexState(state);

      CLog::Log(LOGINFO, "SemanticIndexService: Completed indexing {} {} ({} chunks)",
                item.mediaType, item.mediaId, state.chunkCount);
    }
  }

  // Clear current item
  {
    std::lock_guard<std::mutex> lock(m_currentItemMutex);
    m_currentMediaId = -1;
    m_currentMediaType.clear();
  }
}

bool CSemanticIndexService::IndexSubtitles(int mediaId, const std::string& mediaType)
{
  // Get media file path
  std::string mediaPath = GetMediaPath(mediaId, mediaType);
  if (mediaPath.empty())
  {
    CLog::Log(LOGERROR, "SemanticIndexService: Failed to get media path for {} {}", mediaType,
              mediaId);

    SemanticIndexState state;
    if (m_database->GetIndexState(mediaId, mediaType, state))
    {
      state.subtitleStatus = IndexStatus::FAILED;
      m_database->UpdateIndexState(state);
    }
    return false;
  }

  // Find subtitle file
  std::string subtitlePath = CSubtitleParser::FindSubtitleForMedia(mediaPath);
  if (subtitlePath.empty())
  {
    CLog::Log(LOGDEBUG, "SemanticIndexService: No subtitle file found for {}", mediaPath);

    SemanticIndexState state;
    if (m_database->GetIndexState(mediaId, mediaType, state))
    {
      state.subtitleStatus = IndexStatus::COMPLETED; // Not an error - just no subtitles
      m_database->UpdateIndexState(state);
    }
    return false;
  }

  CLog::Log(LOGINFO, "SemanticIndexService: Parsing subtitle file: {}", subtitlePath);

  try
  {
    // Parse subtitle file
    std::vector<ParsedEntry> entries = m_subtitleParser->Parse(subtitlePath);

    if (entries.empty())
    {
      CLog::Log(LOGWARNING, "SemanticIndexService: No entries parsed from {}", subtitlePath);

      SemanticIndexState state;
      if (m_database->GetIndexState(mediaId, mediaType, state))
      {
        state.subtitleStatus = IndexStatus::COMPLETED;
        m_database->UpdateIndexState(state);
      }
      return false;
    }

    // Convert to semantic chunks
    std::vector<SemanticChunk> chunks;
    for (const auto& entry : entries)
    {
      SemanticChunk chunk;
      chunk.mediaId = mediaId;
      chunk.mediaType = mediaType;
      chunk.sourceType = SourceType::SUBTITLE;
      chunk.sourcePath = subtitlePath;
      chunk.startMs = static_cast<int>(entry.startMs);
      chunk.endMs = static_cast<int>(entry.endMs);
      chunk.text = entry.text;
      chunk.confidence = entry.confidence;

      chunks.push_back(chunk);
    }

    // Insert chunks into database
    if (!m_database->InsertChunks(chunks))
    {
      CLog::Log(LOGERROR, "SemanticIndexService: Failed to insert subtitle chunks");

      SemanticIndexState state;
      if (m_database->GetIndexState(mediaId, mediaType, state))
      {
        state.subtitleStatus = IndexStatus::FAILED;
        m_database->UpdateIndexState(state);
      }
      return false;
    }

    // Update status
    SemanticIndexState state;
    if (m_database->GetIndexState(mediaId, mediaType, state))
    {
      state.subtitleStatus = IndexStatus::COMPLETED;
      m_database->UpdateIndexState(state);
    }

    CLog::Log(LOGINFO, "SemanticIndexService: Indexed {} subtitle chunks from {}", chunks.size(),
              subtitlePath);
    return true;
  }
  catch (const std::exception& e)
  {
    CLog::Log(LOGERROR, "SemanticIndexService: Exception parsing subtitles: {}", e.what());

    SemanticIndexState state;
    if (m_database->GetIndexState(mediaId, mediaType, state))
    {
      state.subtitleStatus = IndexStatus::FAILED;
      m_database->UpdateIndexState(state);
    }
    return false;
  }
}

bool CSemanticIndexService::IndexMetadata(int mediaId, const std::string& mediaType)
{
  CLog::Log(LOGDEBUG, "SemanticIndexService: Indexing metadata for {} {}", mediaType, mediaId);

  try
  {
    // Get video info from database
    CVideoDatabase videoDB;
    if (!videoDB.Open())
    {
      CLog::Log(LOGERROR, "SemanticIndexService: Failed to open video database");

      SemanticIndexState state;
      if (m_database->GetIndexState(mediaId, mediaType, state))
      {
        state.metadataStatus = IndexStatus::FAILED;
        m_database->UpdateIndexState(state);
      }
      return false;
    }

    CVideoInfoTag tag;
    bool loaded = false;

    if (mediaType == "movie")
    {
      loaded = videoDB.GetMovieInfo("", tag, mediaId);
    }
    else if (mediaType == "episode")
    {
      loaded = videoDB.GetEpisodeInfo("", tag, mediaId);
    }
    else if (mediaType == "musicvideo")
    {
      loaded = videoDB.GetMusicVideoInfo("", tag, mediaId);
    }

    videoDB.Close();

    if (!loaded)
    {
      CLog::Log(LOGWARNING, "SemanticIndexService: Failed to load info for {} {}", mediaType,
                mediaId);

      SemanticIndexState state;
      if (m_database->GetIndexState(mediaId, mediaType, state))
      {
        state.metadataStatus = IndexStatus::FAILED;
        m_database->UpdateIndexState(state);
      }
      return false;
    }

    // Parse metadata from VideoInfoTag
    std::vector<ParsedEntry> entries = m_metadataParser->ParseFromVideoInfo(tag);

    if (entries.empty())
    {
      CLog::Log(LOGDEBUG, "SemanticIndexService: No metadata entries for {} {}", mediaType,
                mediaId);

      SemanticIndexState state;
      if (m_database->GetIndexState(mediaId, mediaType, state))
      {
        state.metadataStatus = IndexStatus::COMPLETED; // Not an error
        m_database->UpdateIndexState(state);
      }
      return false;
    }

    // Convert to semantic chunks
    std::vector<SemanticChunk> chunks;
    for (const auto& entry : entries)
    {
      SemanticChunk chunk;
      chunk.mediaId = mediaId;
      chunk.mediaType = mediaType;
      chunk.sourceType = SourceType::METADATA;
      chunk.sourcePath = "metadata";
      chunk.startMs = 0; // Metadata has no timestamp
      chunk.endMs = 0;
      chunk.text = entry.text;
      chunk.confidence = entry.confidence;

      chunks.push_back(chunk);
    }

    // Insert chunks into database
    if (!m_database->InsertChunks(chunks))
    {
      CLog::Log(LOGERROR, "SemanticIndexService: Failed to insert metadata chunks");

      SemanticIndexState state;
      if (m_database->GetIndexState(mediaId, mediaType, state))
      {
        state.metadataStatus = IndexStatus::FAILED;
        m_database->UpdateIndexState(state);
      }
      return false;
    }

    // Update status
    SemanticIndexState state;
    if (m_database->GetIndexState(mediaId, mediaType, state))
    {
      state.metadataStatus = IndexStatus::COMPLETED;
      m_database->UpdateIndexState(state);
    }

    CLog::Log(LOGINFO, "SemanticIndexService: Indexed {} metadata chunks", chunks.size());
    return true;
  }
  catch (const std::exception& e)
  {
    CLog::Log(LOGERROR, "SemanticIndexService: Exception indexing metadata: {}", e.what());

    SemanticIndexState state;
    if (m_database->GetIndexState(mediaId, mediaType, state))
    {
      state.metadataStatus = IndexStatus::FAILED;
      m_database->UpdateIndexState(state);
    }
    return false;
  }
}

bool CSemanticIndexService::StartTranscription(int mediaId, const std::string& mediaType)
{
  if (!m_transcriptionManager)
  {
    CLog::Log(LOGWARNING,
              "SemanticIndexService: Transcription manager not initialized - skipping");

    SemanticIndexState state;
    if (m_database->GetIndexState(mediaId, mediaType, state))
    {
      state.transcriptionStatus = IndexStatus::FAILED;
      m_database->UpdateIndexState(state);
    }
    return false;
  }

  // Check budget
  if (m_transcriptionManager->IsBudgetExceeded())
  {
    CLog::Log(LOGWARNING,
              "SemanticIndexService: Transcription budget exceeded - skipping {} {}", mediaType,
              mediaId);

    SemanticIndexState state;
    if (m_database->GetIndexState(mediaId, mediaType, state))
    {
      state.transcriptionStatus = IndexStatus::FAILED;
      m_database->UpdateIndexState(state);
    }
    return false;
  }

  // Get default provider
  auto* provider = m_transcriptionManager->GetDefaultProvider();
  if (!provider)
  {
    CLog::Log(LOGWARNING, "SemanticIndexService: No transcription provider available");

    SemanticIndexState state;
    if (m_database->GetIndexState(mediaId, mediaType, state))
    {
      state.transcriptionStatus = IndexStatus::FAILED;
      m_database->UpdateIndexState(state);
    }
    return false;
  }

  // Get media path
  std::string mediaPath = GetMediaPath(mediaId, mediaType);
  if (mediaPath.empty())
  {
    CLog::Log(LOGERROR, "SemanticIndexService: Failed to get media path for transcription");

    SemanticIndexState state;
    if (m_database->GetIndexState(mediaId, mediaType, state))
    {
      state.transcriptionStatus = IndexStatus::FAILED;
      m_database->UpdateIndexState(state);
    }
    return false;
  }

  CLog::Log(LOGINFO, "SemanticIndexService: Starting transcription for {} {} using provider {}",
            mediaType, mediaId, provider->GetId());

  // Update status to in_progress
  SemanticIndexState state;
  if (m_database->GetIndexState(mediaId, mediaType, state))
  {
    state.transcriptionStatus = IndexStatus::IN_PROGRESS;
    state.transcriptionProvider = provider->GetId();
    state.transcriptionProgress = 0.0f;
    m_database->UpdateIndexState(state);
  }

  // Check FFmpeg availability
  if (!CAudioExtractor::IsFFmpegAvailable())
  {
    CLog::Log(LOGERROR, "SemanticIndexService: FFmpeg not available for audio extraction");
    if (m_database->GetIndexState(mediaId, mediaType, state))
    {
      state.transcriptionStatus = IndexStatus::FAILED;
      m_database->UpdateIndexState(state);
    }
    return false;
  }

  // Create audio extractor with optimal settings for Whisper
  CAudioExtractor extractor;

  // Get media duration for cost estimation
  int64_t durationMs = extractor.GetMediaDuration(mediaPath);
  if (durationMs < 0)
  {
    CLog::Log(LOGWARNING, "SemanticIndexService: Could not determine media duration, estimating");
    durationMs = 60 * 60 * 1000; // Assume 1 hour for cost check
  }

  // Check estimated cost against budget
  float estimatedCost = provider->EstimateCost(durationMs);
  float remainingBudget = m_transcriptionManager->GetRemainingBudget();
  CLog::Log(LOGINFO, "SemanticIndexService: Estimated cost ${:.4f}, remaining budget ${:.2f}",
            estimatedCost, remainingBudget);

  if (estimatedCost > remainingBudget)
  {
    CLog::Log(LOGWARNING, "SemanticIndexService: Estimated cost ${:.4f} exceeds budget ${:.2f}",
              estimatedCost, remainingBudget);
    if (m_database->GetIndexState(mediaId, mediaType, state))
    {
      state.transcriptionStatus = IndexStatus::FAILED;
      m_database->UpdateIndexState(state);
    }
    return false;
  }

  // Create temp directory for audio
  std::string tempDir = CSpecialProtocol::TranslatePath("special://temp/semantic_audio/");
  XFILE::CDirectory::Create(tempDir);

  // Generate output path
  std::string audioPath = tempDir + StringUtils::Format("audio_{}_{}.mp3", mediaType, mediaId);

  CLog::Log(LOGINFO, "SemanticIndexService: Extracting audio from {} to {}", mediaPath, audioPath);

  // Extract audio
  if (!extractor.ExtractAudio(mediaPath, audioPath))
  {
    CLog::Log(LOGERROR, "SemanticIndexService: Failed to extract audio from {}", mediaPath);
    if (m_database->GetIndexState(mediaId, mediaType, state))
    {
      state.transcriptionStatus = IndexStatus::FAILED;
      m_database->UpdateIndexState(state);
    }
    return false;
  }

  CLog::Log(LOGINFO, "SemanticIndexService: Audio extracted, starting transcription");

  // Transcribe with callbacks
  std::vector<SemanticChunk> chunks;
  bool transcriptionSuccess = false;
  std::string transcriptionError;

  // Capture context for callbacks
  int capturedMediaId = mediaId;
  std::string capturedMediaType = mediaType;

  transcriptionSuccess = provider->Transcribe(
      audioPath,
      // Segment callback - called for each transcribed segment
      [&chunks, capturedMediaId, &capturedMediaType, &audioPath](const TranscriptSegment& segment) {
        SemanticChunk chunk;
        chunk.mediaId = capturedMediaId;
        chunk.mediaType = capturedMediaType;
        chunk.sourceType = SourceType::TRANSCRIPTION;
        chunk.sourcePath = audioPath;
        chunk.startMs = static_cast<int>(segment.startMs);
        chunk.endMs = static_cast<int>(segment.endMs);
        chunk.text = segment.text;
        chunk.language = segment.language;
        chunk.confidence = segment.confidence;
        chunks.push_back(chunk);

        CLog::Log(LOGDEBUG, "SemanticIndexService: Transcribed segment [{}-{}ms]: {}",
                  segment.startMs, segment.endMs,
                  segment.text.substr(0, 50) + (segment.text.length() > 50 ? "..." : ""));
      },
      // Progress callback
      [this, capturedMediaId, &capturedMediaType](float progress) {
        SemanticIndexState progressState;
        if (m_database->GetIndexState(capturedMediaId, capturedMediaType, progressState))
        {
          progressState.transcriptionProgress = progress;
          m_database->UpdateIndexState(progressState);
        }
        CLog::Log(LOGDEBUG, "SemanticIndexService: Transcription progress: {:.1f}%", progress * 100);
      },
      // Error callback
      [&transcriptionError](const std::string& error) {
        transcriptionError = error;
        CLog::Log(LOGERROR, "SemanticIndexService: Transcription error: {}", error);
      });

  // Clean up temp audio file
  XFILE::CFile::Delete(audioPath);

  if (!transcriptionSuccess || chunks.empty())
  {
    CLog::Log(LOGERROR, "SemanticIndexService: Transcription failed: {}",
              transcriptionError.empty() ? "no segments returned" : transcriptionError);
    if (m_database->GetIndexState(mediaId, mediaType, state))
    {
      state.transcriptionStatus = IndexStatus::FAILED;
      m_database->UpdateIndexState(state);
    }
    return false;
  }

  // Store chunks in database
  CLog::Log(LOGINFO, "SemanticIndexService: Storing {} transcription chunks", chunks.size());
  if (!m_database->InsertChunks(chunks))
  {
    CLog::Log(LOGERROR, "SemanticIndexService: Failed to store transcription chunks");
    if (m_database->GetIndexState(mediaId, mediaType, state))
    {
      state.transcriptionStatus = IndexStatus::FAILED;
      m_database->UpdateIndexState(state);
    }
    return false;
  }

  // Record usage for budget tracking
  float actualDurationMin = durationMs / 60000.0f;
  float actualCost = provider->EstimateCost(durationMs);
  m_transcriptionManager->RecordUsage(provider->GetId(), actualDurationMin, actualCost);

  CLog::Log(LOGINFO,
            "SemanticIndexService: Transcription complete - {} chunks, {:.1f} min, ${:.4f}",
            chunks.size(), actualDurationMin, actualCost);

  // Update status to completed
  if (m_database->GetIndexState(mediaId, mediaType, state))
  {
    state.transcriptionStatus = IndexStatus::COMPLETED;
    state.transcriptionProgress = 1.0f;
    m_database->UpdateIndexState(state);
  }

  return true;
}

// ========== Helper Methods ==========

void CSemanticIndexService::OnLibraryUpdate(const std::string& type, const CVariant& data)
{
  if (!m_autoIndex)
  {
    CLog::Log(LOGDEBUG, "SemanticIndexService: Auto-index disabled - ignoring library update");
    return;
  }

  if (type == "OnScanFinished" || type == "OnUpdate")
  {
    CLog::Log(LOGINFO, "SemanticIndexService: Library updated - queueing new items");
    QueueAllUnindexed();
  }
  else if (type == "OnRemove")
  {
    // Clean up removed items
    if (data.isMember("id") && data.isMember("type"))
    {
      int mediaId = data["id"].asInteger();
      std::string mediaType = data["type"].asString();

      CLog::Log(LOGINFO, "SemanticIndexService: Removing chunks for deleted {} {}", mediaType,
                mediaId);

      m_database->DeleteChunksForMedia(mediaId, mediaType);

      // Remove from queue if present
      CancelMedia(mediaId, mediaType);
    }
  }
}

bool CSemanticIndexService::ShouldProcessNow() const
{
  if (m_processMode == "manual")
  {
    // Only process manually queued items with high priority
    return false;
  }

  if (m_processMode == "idle")
  {
    // Check if user is idle (5 minutes)
    auto& components = CServiceBroker::GetAppComponents();
    const auto appPower = components.GetComponent<CApplicationPowerHandling>();
    if (!appPower)
      return false;

    int idleTime = appPower->GlobalIdleTime();
    return idleTime >= 300; // 5 minutes in seconds
  }

  // "background" mode - always process
  return true;
}

std::string CSemanticIndexService::GetMediaPath(int mediaId, const std::string& mediaType)
{
  CVideoDatabase videoDB;
  if (!videoDB.Open())
  {
    CLog::Log(LOGERROR, "SemanticIndexService: Failed to open video database");
    return "";
  }

  std::string path = GetMediaPathFromDatabase(videoDB, mediaId, mediaType);
  videoDB.Close();
  return path;
}

std::string CSemanticIndexService::GetMediaPathFromDatabase(CVideoDatabase& videoDb,
                                                            int mediaId,
                                                            const std::string& mediaType)
{
  std::string path;

  if (mediaType == "movie")
  {
    CVideoInfoTag tag;
    if (videoDb.GetMovieInfo("", tag, mediaId))
      path = tag.m_strFileNameAndPath;
  }
  else if (mediaType == "episode")
  {
    CVideoInfoTag tag;
    if (videoDb.GetEpisodeInfo("", tag, mediaId))
      path = tag.m_strFileNameAndPath;
  }
  else if (mediaType == "musicvideo")
  {
    CVideoInfoTag tag;
    if (videoDb.GetMusicVideoInfo("", tag, mediaId))
      path = tag.m_strFileNameAndPath;
  }

  return path;
}

bool CSemanticIndexService::IsInQueue(int mediaId, const std::string& mediaType) const
{
  // Note: Caller must hold m_queueMutex
  return std::any_of(m_queue.begin(), m_queue.end(), [&](const QueueItem& item) {
    return item.mediaId == mediaId && item.mediaType == mediaType;
  });
}

void CSemanticIndexService::RemoveFromQueue(int mediaId, const std::string& mediaType)
{
  // Note: Caller must hold m_queueMutex
  m_queue.erase(std::remove_if(m_queue.begin(), m_queue.end(),
                                [&](const QueueItem& item) {
                                  return item.mediaId == mediaId && item.mediaType == mediaType;
                                }),
                m_queue.end());
}

std::vector<int> CSemanticIndexService::GetMissingMediaIds(const std::string& mediaType,
                                                           const std::string& viewName,
                                                           const std::string& idColumn)
{
  std::vector<int> ids;
  if (!m_database)
    return ids;

  // Get all media items from video database
  CVideoDatabase videoDb;
  if (!videoDb.Open())
    return ids;

  CFileItemList items;
  std::string baseDir;

  if (mediaType == "movie")
  {
    baseDir = "videodb://movies/titles/";
    videoDb.GetMoviesNav(baseDir, items);
  }
  else if (mediaType == "episode")
  {
    baseDir = "videodb://tvshows/titles/";
    videoDb.GetEpisodesNav(baseDir, items);
  }
  else if (mediaType == "musicvideo")
  {
    baseDir = "videodb://musicvideos/titles/";
    videoDb.GetMusicVideosNav(baseDir, items);
  }

  videoDb.Close();

  // Filter out items already in semantic database
  for (int i = 0; i < items.Size(); i++)
  {
    int dbId = items[i]->GetVideoInfoTag()->m_iDbId;
    if (dbId <= 0)
      continue;

    std::string checkSql = StringUtils::Format(
        "SELECT 1 FROM semantic_index_state WHERE media_id = %d AND media_type = '%s' LIMIT 1",
        dbId, mediaType.c_str());
    auto dataset = m_database->Query(checkSql);
    if (!dataset || dataset->eof())
    {
      ids.push_back(dbId);
    }
    if (dataset)
      dataset->close();
  }

  return ids;
}

void CSemanticIndexService::EnsureIndexState(int mediaId,
                                             const std::string& mediaType,
                                             const std::string& mediaPath)
{
  if (!m_database || mediaPath.empty())
    return;

  SemanticIndexState state;
  state.mediaId = mediaId;
  state.mediaType = mediaType;
  state.mediaPath = mediaPath;
  state.subtitleStatus = IndexStatus::PENDING;
  state.transcriptionStatus = IndexStatus::PENDING;
  state.metadataStatus = IndexStatus::PENDING;
  state.embeddingStatus = IndexStatus::PENDING;
  state.priority = 0;

  if (m_database->UpdateIndexState(state))
  {
    CLog::Log(LOGDEBUG, "SemanticIndexService: Seeded index state for {} {}", mediaType, mediaId);
  }
}

void CSemanticIndexService::SeedMissingIndexStates()
{
  if (!m_database)
    return;

  CVideoDatabase videoDb;
  if (!videoDb.Open())
  {
    CLog::Log(LOGERROR, "SemanticIndexService: Failed to open video database for seeding");
    return;
  }

  struct SeedInfo
  {
    const char* mediaType;
    const char* viewName;
    const char* idColumn;
  };

  const std::array<SeedInfo, 3> tables = {{
      {"movie", "movie_view", "idMovie"},
      {"episode", "episode_view", "idEpisode"},
      {"musicvideo", "musicvideo_view", "idMVideo"},
  }};

  for (const auto& table : tables)
  {
    auto ids = GetMissingMediaIds(table.mediaType, table.viewName, table.idColumn);
    for (int id : ids)
    {
      std::string path = GetMediaPathFromDatabase(videoDb, id, table.mediaType);
      if (path.empty())
        continue;
      EnsureIndexState(id, table.mediaType, path);
    }
  }

  videoDb.Close();
}
