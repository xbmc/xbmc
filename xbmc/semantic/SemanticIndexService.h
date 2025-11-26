/*
 *  Copyright (C) 2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "dbwrappers/dataset.h"
#include "interfaces/IAnnouncer.h"
#include "jobs/IJobCallback.h"
#include "settings/lib/ISettingCallback.h"
#include "threads/Thread.h"
#include "video/VideoDatabase.h"

#include <atomic>
#include <condition_variable>
#include <deque>
#include <memory>
#include <mutex>
#include <string>

namespace KODI
{
namespace SEMANTIC
{

// Forward declarations
class CSemanticDatabase;
class CSubtitleParser;
class CMetadataParser;
class CTranscriptionProviderManager;

/*!
 * @brief Main orchestrator for semantic indexing operations
 *
 * This service runs in the background and manages all semantic indexing:
 * - Watches video library for new/updated content
 * - Queues media items for indexing
 * - Coordinates subtitle parsing, metadata extraction, and transcription
 * - Respects idle mode and processing preferences
 * - Provides status queries and manual control
 */
class CSemanticIndexService : public CThread,
                              public ANNOUNCEMENT::IAnnouncer,
                              public IJobCallback,
                              public ISettingCallback
{
public:
  CSemanticIndexService();
  ~CSemanticIndexService() override;

  // ========== Service Lifecycle ==========

  /*!
   * @brief Start the semantic indexing service
   * @return true if started successfully, false otherwise
   *
   * Initializes database, parsers, and background thread.
   * Registers for library announcements and settings changes.
   */
  bool Start();

  /*!
   * @brief Stop the semantic indexing service
   *
   * Stops the background thread, saves state, and unregisters callbacks.
   */
  void Stop();

  /*!
   * @brief Ensure settings callbacks are registered even if the service is stopped
   */
  void EnsureSettingsCallback();

  /*!
   * @brief Check if the service is running
   * @return true if service is active, false otherwise
   */
  bool IsRunning() const { return m_running.load(); }

  // ========== Manual Indexing Control ==========

  /*!
   * @brief Queue a specific media item for indexing
   * @param mediaId The media item database ID
   * @param mediaType The media type (movie, episode, musicvideo)
   * @param priority Processing priority (higher = sooner, default 0)
   */
  void QueueMedia(int mediaId, const std::string& mediaType, int priority = 0);

  /*!
   * @brief Queue all unindexed media in the library
   *
   * Scans the video database for items that haven't been indexed yet
   * and adds them to the processing queue.
   */
  void QueueAllUnindexed();

  /*!
   * @brief Queue a media item for transcription
   * @param mediaId The media item database ID
   * @param mediaType The media type
   *
   * Explicitly requests transcription for this item (even if subtitles exist).
   */
  void QueueTranscription(int mediaId, const std::string& mediaType);

  // ========== Cancel Operations ==========

  /*!
   * @brief Cancel indexing for a specific media item
   * @param mediaId The media item database ID
   * @param mediaType The media type
   */
  void CancelMedia(int mediaId, const std::string& mediaType);

  /*!
   * @brief Cancel all pending indexing operations
   *
   * Clears the queue but does not stop in-progress operations.
   */
  void CancelAllPending();

  // ========== Status Queries ==========

  /*!
   * @brief Check if a media item has been indexed
   * @param mediaId The media item database ID
   * @param mediaType The media type
   * @return true if indexing is complete, false otherwise
   */
  bool IsMediaIndexed(int mediaId, const std::string& mediaType);

  /*!
   * @brief Get indexing progress for a media item
   * @param mediaId The media item database ID
   * @param mediaType The media type
   * @return Progress value (0.0-1.0), or -1.0 if not queued
   */
  float GetProgress(int mediaId, const std::string& mediaType);

  /*!
   * @brief Get the number of items in the processing queue
   * @return Queue length
   */
  int GetQueueLength() const;

  /*!
   * @brief Get the semantic database instance
   * @return Pointer to the database, or nullptr if not initialized
   */
  CSemanticDatabase* GetDatabase() const { return m_database.get(); }

  /*!
   * @brief Get the transcription provider manager
   * @return Pointer to the provider manager, or nullptr if not initialized
   */
  CTranscriptionProviderManager* GetTranscriptionManager() const { return m_transcriptionManager.get(); }

  // ========== Callbacks ==========

  /*!
   * @brief Callback when a job completes
   * @param jobID The job identifier
   * @param success Whether the job succeeded
   * @param job The job instance (will be destroyed after callback)
   */
  void OnJobComplete(unsigned int jobID, bool success, CJob* job) override;

  /*!
   * @brief Callback for job progress updates
   * @param jobID The job identifier
   * @param progress Current progress value
   * @param total Total progress value
   * @param job The job instance
   */
  void OnJobProgress(unsigned int jobID,
                     unsigned int progress,
                     unsigned int total,
                     const CJob* job) override;

  /*!
   * @brief Callback when settings change
   * @param setting The setting that changed
   */
  void OnSettingChanged(const std::shared_ptr<const CSetting>& setting) override;

  /*!
   * @brief Callback for library announcements
   * @param flag The announcement type
   * @param sender The sender identifier
   * @param message The announcement message
   * @param data Additional announcement data
   */
  void Announce(ANNOUNCEMENT::AnnouncementFlag flag,
                const std::string& sender,
                const std::string& message,
                const CVariant& data) override;

protected:
  // ========== Thread Processing ==========

  /*!
   * @brief Main processing thread loop
   *
   * Waits for queue items and processes them according to idle/background mode.
   */
  void Process() override;

private:
  // ========== Queue Item Structure ==========

  struct QueueItem
  {
    int mediaId;
    std::string mediaType;
    int priority;
    bool transcribe; // Force transcription even if subtitles exist

    // For priority queue sorting (higher priority first)
    bool operator<(const QueueItem& other) const { return priority < other.priority; }
  };

  // ========== Processing Methods ==========

  /*!
   * @brief Process the next item in the queue
   * @return true if an item was processed, false if queue is empty
   */
  bool ProcessNextItem();

  /*!
   * @brief Process a single queue item
   * @param item The item to process
   */
  void ProcessItem(const QueueItem& item);

  /*!
   * @brief Index subtitles for a media item
   * @param mediaId The media item ID
   * @param mediaType The media type
   * @return true if successful or no subtitles found, false on error
   */
  bool IndexSubtitles(int mediaId, const std::string& mediaType);

  /*!
   * @brief Index metadata for a media item
   * @param mediaId The media item ID
   * @param mediaType The media type
   * @return true if successful, false on error
   */
  bool IndexMetadata(int mediaId, const std::string& mediaType);

  /*!
   * @brief Start transcription for a media item
   * @param mediaId The media item ID
   * @param mediaType The media type
   * @return true if transcription was started, false otherwise
   */
  bool StartTranscription(int mediaId, const std::string& mediaType);

  // ========== Helper Methods ==========

  /*!
   * @brief Handle library update announcement
   * @param type The update type (OnScanFinished, OnUpdate, OnRemove)
   * @param data The announcement data
   */
  void OnLibraryUpdate(const std::string& type, const CVariant& data);

  /*!
   * @brief Check if processing should happen now
   * @return true if we should process, false if we should wait
   *
   * Respects manual/idle/background mode settings.
   */
  bool ShouldProcessNow() const;

  /*!
   * @brief Get media file path from database
   * @param mediaId The media item ID
   * @param mediaType The media type
   * @return File path, or empty string on error
   */
  std::string GetMediaPath(int mediaId, const std::string& mediaType);
  std::string GetMediaPathFromDatabase(CVideoDatabase& videoDb,
                                       int mediaId,
                                       const std::string& mediaType);

  /*!
   * @brief Check if item is already in queue
   * @param mediaId The media item ID
   * @param mediaType The media type
   * @return true if already queued, false otherwise
   */
  bool IsInQueue(int mediaId, const std::string& mediaType) const;

  /*!
   * @brief Remove item from queue
   * @param mediaId The media item ID
   * @param mediaType The media type
   */
  void RemoveFromQueue(int mediaId, const std::string& mediaType);

  /*!
   * @brief Ensure semantic_index_state rows exist for all library items
   */
  void SeedMissingIndexStates();
  std::vector<int> GetMissingMediaIds(const std::string& mediaType,
                                      const std::string& viewName,
                                      const std::string& idColumn);
  void EnsureIndexState(int mediaId, const std::string& mediaType, const std::string& mediaPath);

  // ========== Member Variables ==========

  // Core components
  std::unique_ptr<CSemanticDatabase> m_database;
  std::unique_ptr<CSubtitleParser> m_subtitleParser;
  std::unique_ptr<CMetadataParser> m_metadataParser;
  std::unique_ptr<CTranscriptionProviderManager> m_transcriptionManager;

  // Thread control
  std::atomic<bool> m_running{false};
  mutable std::mutex m_queueMutex;
  std::condition_variable m_queueCondition;

  // Processing queue (priority queue - higher priority items first)
  std::deque<QueueItem> m_queue;

  // Current processing state
  int m_currentMediaId{-1};
  std::string m_currentMediaType;
  std::mutex m_currentItemMutex;

  // Settings cache
  std::string m_processMode; // "manual", "idle", "background"
  bool m_autoIndex{true};
  bool m_callbacksRegistered{false};
};

} // namespace SEMANTIC
} // namespace KODI
