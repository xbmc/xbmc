/*
 *  Copyright (C) 2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "CGUIDialogSemanticSearch.h"

#include "FileItem.h"
#include "FileItemList.h"
#include "ServiceBroker.h"
#include "application/Application.h"
#include "application/ApplicationComponents.h"
#include "application/ApplicationPlayer.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIEditControl.h"
#include "guilib/GUIKeyboardFactory.h"
#include "guilib/GUIMessage.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/LocalizeStrings.h"
#include "input/actions/ActionIDs.h"
#include "semantic/SemanticDatabase.h"
#include "semantic/embedding/EmbeddingEngine.h"
#include "semantic/history/SearchHistory.h"
#include "semantic/history/SearchSuggestions.h"
#include "semantic/search/ContextProvider.h"
#include "semantic/search/VectorSearcher.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/log.h"
#include "video/VideoDatabase.h"
#include "video/VideoInfoTag.h"

#include <chrono>

// Control IDs
#define CONTROL_HEADING 1
#define CONTROL_SEARCH_INPUT 2
#define CONTROL_RESULTS_LIST 3
#define CONTROL_DETAILS_PANEL 4
#define CONTROL_CONTEXT_PREVIEW 5
#define CONTROL_SEARCH_MODE_BUTTON 6
#define CONTROL_MEDIA_FILTER_BUTTON 7
#define CONTROL_PROGRESS 8
#define CONTROL_CLOSE_BUTTON 9
#define CONTROL_CLEAR_BUTTON 10
#define CONTROL_STATUS_LABEL 11
#define CONTROL_RESULT_COUNT 12
#define CONTROL_FILTER_PANEL 13
#define CONTROL_GENRE_LIST 14
#define CONTROL_YEAR_MIN_SLIDER 15
#define CONTROL_YEAR_MAX_SLIDER 16
#define CONTROL_RATING_BUTTON 17
#define CONTROL_DURATION_BUTTON 18
#define CONTROL_SOURCE_GROUP 19
#define CONTROL_CLEAR_FILTERS_BUTTON 20
#define CONTROL_FILTER_BADGES 21
#define CONTROL_PRESET_BUTTON 22
#define CONTROL_SAVE_PRESET_BUTTON 23
#define CONTROL_SUBTITLE_TOGGLE 24
#define CONTROL_TRANSCRIPTION_TOGGLE 25
#define CONTROL_METADATA_TOGGLE 26

// Window ID (needs to be added to WindowIDs.h)
#define WINDOW_DIALOG_SEMANTIC_SEARCH 10161

namespace KODI
{
namespace SEMANTIC
{

CGUIDialogSemanticSearch::CGUIDialogSemanticSearch()
  : CGUIDialog(WINDOW_DIALOG_SEMANTIC_SEARCH, "DialogSemanticSearch.xml"),
    CThread("SemanticSearch"),
    m_results(std::make_unique<CFileItemList>())
{
  m_loadType = KEEP_IN_MEMORY;

  // Initialize search options with defaults
  m_searchOptions.mode = SearchMode::Hybrid;
  m_searchOptions.maxResults = 50;
  m_searchOptions.keywordWeight = 0.4f;
  m_searchOptions.vectorWeight = 0.6f;

  // Initialize filter preset manager
  m_presetManager = std::make_unique<CFilterPresetManager>();
}

CGUIDialogSemanticSearch::~CGUIDialogSemanticSearch()
{
  StopThread();
}

bool CGUIDialogSemanticSearch::Initialize(CSemanticDatabase* database,
                                          CEmbeddingEngine* embeddingEngine,
                                          CVectorSearcher* vectorSearcher,
                                          CVideoDatabase* videoDatabase)
{
  if (!database || !embeddingEngine || !vectorSearcher || !videoDatabase)
  {
    CLog::Log(LOGERROR, "CGUIDialogSemanticSearch: Invalid parameters for initialization");
    return false;
  }

  m_database = database;
  m_embeddingEngine = embeddingEngine;
  m_vectorSearcher = vectorSearcher;
  m_videoDatabase = videoDatabase;

  // Initialize search engine
  m_searchEngine = std::make_unique<CHybridSearchEngine>();
  if (!m_searchEngine->Initialize(database, embeddingEngine, vectorSearcher))
  {
    CLog::Log(LOGERROR, "CGUIDialogSemanticSearch: Failed to initialize search engine");
    return false;
  }

  // Initialize result enricher
  m_enricher = std::make_unique<CResultEnricher>();
  if (!m_enricher->Initialize(videoDatabase))
  {
    CLog::Log(LOGERROR, "CGUIDialogSemanticSearch: Failed to initialize result enricher");
    return false;
  }

  // Initialize context provider
  m_contextProvider = std::make_unique<CContextProvider>();
  if (!m_contextProvider->Initialize(database))
  {
    CLog::Log(LOGERROR, "CGUIDialogSemanticSearch: Failed to initialize context provider");
    return false;
  }

  // Load filter presets
  if (m_presetManager)
  {
    m_presetManager->Load();
  }

  // Load available genres from database
  m_availableGenres = LoadGenresFromDatabase();

  // Initialize search history
  m_searchHistory = std::make_unique<CSearchHistory>();
  if (!m_searchHistory->Initialize(database))
  {
    CLog::Log(LOGERROR, "CGUIDialogSemanticSearch: Failed to initialize search history");
    return false;
  }

  // Initialize search suggestions
  m_searchSuggestions = std::make_unique<CSearchSuggestions>();
  if (!m_searchSuggestions->Initialize(database, m_searchHistory.get()))
  {
    CLog::Log(LOGERROR, "CGUIDialogSemanticSearch: Failed to initialize search suggestions");
    return false;
  }

  CLog::Log(LOGINFO, "CGUIDialogSemanticSearch: Successfully initialized");
  return true;
}

void CGUIDialogSemanticSearch::OnInitWindow()
{
  CGUIDialog::OnInitWindow();

  // Set heading
  SET_CONTROL_LABEL(CONTROL_HEADING, "Semantic Search");

  // Initialize UI state
  UpdateSearchStatus("Enter a search query to begin");
  UpdateResultCount();

  // Update button labels
  SET_CONTROL_LABEL(CONTROL_SEARCH_MODE_BUTTON, FormatSearchMode());
  SET_CONTROL_LABEL(CONTROL_MEDIA_FILTER_BUTTON, FormatMediaTypeFilter());
  SET_CONTROL_LABEL(CONTROL_CLEAR_BUTTON, "Clear");
  SET_CONTROL_LABEL(CONTROL_CLOSE_BUTTON, "Close");

  // Hide progress indicator initially
  SET_CONTROL_HIDDEN(CONTROL_PROGRESS);

  // Clear any previous results
  m_results->Clear();
  m_enrichedResults.clear();

  // Initialize filter controls
  UpdateFilterControls();
  UpdateFilterBadges();

  // Focus search input
  SET_CONTROL_FOCUS(CONTROL_SEARCH_INPUT, 0);
}

void CGUIDialogSemanticSearch::OnDeinitWindow(int nextWindowID)
{
  // Stop any ongoing search
  StopThread();

  CGUIDialog::OnDeinitWindow(nextWindowID);
}

bool CGUIDialogSemanticSearch::OnMessage(CGUIMessage& message)
{
  switch (message.GetMessage())
  {
    case GUI_MSG_CLICKED:
    {
      int controlId = message.GetSenderId();

      if (controlId == CONTROL_RESULTS_LIST)
      {
        // Result item selected
        int selectedIndex = message.GetParam1();
        CSingleLock lock(m_criticalSection);

        if (selectedIndex >= 0 && selectedIndex < static_cast<int>(m_enrichedResults.size()))
        {
          m_selectedItemIndex = selectedIndex;
          auto item = CreateFileItem(m_enrichedResults[selectedIndex]);

          if (message.GetParam2() == ACTION_SELECT_ITEM ||
              message.GetParam2() == ACTION_MOUSE_LEFT_CLICK)
          {
            OnResultSelected(item);
          }
          else if (message.GetParam2() == ACTION_SHOW_INFO ||
                   message.GetParam2() == ACTION_CONTEXT_MENU)
          {
            ShowContextPreview(m_enrichedResults[selectedIndex]);
          }
        }
        return true;
      }
      else if (controlId == CONTROL_SEARCH_MODE_BUTTON)
      {
        ToggleSearchMode();
        return true;
      }
      else if (controlId == CONTROL_MEDIA_FILTER_BUTTON)
      {
        ToggleMediaTypeFilter();
        return true;
      }
      else if (controlId == CONTROL_CLEAR_BUTTON)
      {
        ClearSearch();
        return true;
      }
      else if (controlId == CONTROL_CLOSE_BUTTON)
      {
        Close();
        return true;
      }
      else if (controlId == CONTROL_RATING_BUTTON)
      {
        ToggleRatingFilter();
        return true;
      }
      else if (controlId == CONTROL_DURATION_BUTTON)
      {
        ToggleDurationFilter();
        return true;
      }
      else if (controlId == CONTROL_GENRE_LIST)
      {
        ShowGenreSelector();
        return true;
      }
      else if (controlId == CONTROL_CLEAR_FILTERS_BUTTON)
      {
        ClearFilters();
        return true;
      }
      else if (controlId == CONTROL_PRESET_BUTTON)
      {
        ShowFilterPresetSelector();
        return true;
      }
      else if (controlId == CONTROL_SAVE_PRESET_BUTTON)
      {
        ShowSavePresetDialog();
        return true;
      }
      else if (controlId == CONTROL_SUBTITLE_TOGGLE)
      {
        ToggleSourceFilter(SourceType::SUBTITLE);
        return true;
      }
      else if (controlId == CONTROL_TRANSCRIPTION_TOGGLE)
      {
        ToggleSourceFilter(SourceType::TRANSCRIPTION);
        return true;
      }
      else if (controlId == CONTROL_METADATA_TOGGLE)
      {
        ToggleSourceFilter(SourceType::METADATA);
        return true;
      }
      break;
    }

    case GUI_MSG_UPDATE:
    {
      // Search results ready - update UI
      UpdateResultsList();
      return true;
    }

    case GUI_MSG_NOTIFY_ALL:
    {
      if (message.GetParam1() == GUI_MSG_UPDATE_ITEM && message.GetItem())
      {
        // Update specific item if needed
        return true;
      }
      break;
    }

    case GUI_MSG_WINDOW_DEINIT:
    {
      StopThread();
      break;
    }
  }

  return CGUIDialog::OnMessage(message);
}

bool CGUIDialogSemanticSearch::OnAction(const CAction& action)
{
  if (action.GetID() == ACTION_NAV_BACK || action.GetID() == ACTION_PREVIOUS_MENU)
  {
    return OnBack(action.GetID());
  }

  // Handle keyboard input in search field
  if (GetFocusedControlID() == CONTROL_SEARCH_INPUT)
  {
    if (action.GetID() == ACTION_ENTER || action.GetID() == ACTION_SELECT_ITEM)
    {
      // Trigger immediate search
      CGUIEditControl* editControl =
          dynamic_cast<CGUIEditControl*>(GetControl(CONTROL_SEARCH_INPUT));
      if (editControl)
      {
        m_currentQuery = editControl->GetLabel2();
        if (!m_currentQuery.empty())
        {
          // Trigger search immediately
          m_needsUpdate = true;
          if (!IsRunning())
          {
            Create();
          }
        }
      }
      return true;
    }
    else if (action.GetID() >= KEY_VKEY && action.GetID() <= KEY_ASCII)
    {
      // Character input - start debounced search
      auto now = std::chrono::steady_clock::now();
      auto currentTime =
          std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
      m_lastSearchTime = static_cast<unsigned int>(currentTime);

      CGUIEditControl* editControl =
          dynamic_cast<CGUIEditControl*>(GetControl(CONTROL_SEARCH_INPUT));
      if (editControl)
      {
        m_currentQuery = editControl->GetLabel2();
        m_needsUpdate = true;

        if (!IsRunning())
        {
          Create();
        }
      }
    }
  }

  return CGUIDialog::OnAction(action);
}

bool CGUIDialogSemanticSearch::OnBack(int actionID)
{
  StopThread();
  return CGUIDialog::OnBack(actionID);
}

void CGUIDialogSemanticSearch::OpenWithQuery(const std::string& query)
{
  m_currentQuery = query;
  Open();

  // Set the query in the search input
  CGUIEditControl* editControl =
      dynamic_cast<CGUIEditControl*>(GetControl(CONTROL_SEARCH_INPUT));
  if (editControl)
  {
    editControl->SetLabel2(query);
  }

  // Start search
  m_needsUpdate = true;
  if (!IsRunning())
  {
    Create();
  }
}

void CGUIDialogSemanticSearch::SetMediaTypeFilter(const std::string& mediaType)
{
  if (m_mediaTypeFilter != mediaType)
  {
    m_mediaTypeFilter = mediaType;
    m_searchOptions.mediaType = mediaType;

    SET_CONTROL_LABEL(CONTROL_MEDIA_FILTER_BUTTON, FormatMediaTypeFilter());

    // Re-run search with new filter
    if (!m_currentQuery.empty())
    {
      m_needsUpdate = true;
      if (!IsRunning())
      {
        Create();
      }
    }
  }
}

void CGUIDialogSemanticSearch::SetSearchMode(SearchMode mode)
{
  if (m_searchMode != mode)
  {
    m_searchMode = mode;
    m_searchOptions.mode = mode;

    SET_CONTROL_LABEL(CONTROL_SEARCH_MODE_BUTTON, FormatSearchMode());

    // Re-run search with new mode
    if (!m_currentQuery.empty())
    {
      m_needsUpdate = true;
      if (!IsRunning())
      {
        Create();
      }
    }
  }
}

void CGUIDialogSemanticSearch::Process()
{
  while (!m_bStop)
  {
    if (m_needsUpdate)
    {
      // Wait for debounce period
      auto now = std::chrono::steady_clock::now();
      auto currentTime =
          std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();

      if (currentTime - m_lastSearchTime >= SEARCH_DEBOUNCE_MS)
      {
        m_needsUpdate = false;
        PerformSearch();
      }
    }

    CThread::Sleep(100ms);
  }
}

void CGUIDialogSemanticSearch::PerformSearch()
{
  CSingleLock lock(m_criticalSection);

  if (m_currentQuery.empty())
  {
    m_enrichedResults.clear();

    CGUIMessage msg(GUI_MSG_UPDATE, GetID(), 0);
    CServiceBroker::GetGUI()->GetWindowManager().SendThreadMessage(msg, GetID());
    return;
  }

  m_isSearching = true;

  // Show progress indicator
  CGUIMessage showProgress(GUI_MSG_VISIBLE, GetID(), CONTROL_PROGRESS);
  CServiceBroker::GetGUI()->GetWindowManager().SendThreadMessage(showProgress, GetID());

  UpdateSearchStatus("Searching...");

  try
  {
    // Apply current filters to search options
    ApplyFiltersToOptions();

    // Perform search
    auto results = m_searchEngine->Search(m_currentQuery, m_searchOptions);

    CLog::Log(LOGINFO, "CGUIDialogSemanticSearch: Found {} raw results", results.size());

    // Enrich results with media metadata
    m_enrichedResults = m_enricher->EnrichBatch(results);

    CLog::Log(LOGINFO, "CGUIDialogSemanticSearch: Enriched {} results",
              m_enrichedResults.size());

    // Record search in history
    RecordSearchInHistory(m_currentQuery, static_cast<int>(m_enrichedResults.size()));

    // Update UI on main thread
    CGUIMessage msg(GUI_MSG_UPDATE, GetID(), 0);
    CServiceBroker::GetGUI()->GetWindowManager().SendThreadMessage(msg, GetID());

    UpdateSearchStatus("Search complete");
  }
  catch (const std::exception& ex)
  {
    CLog::Log(LOGERROR, "CGUIDialogSemanticSearch: Search failed: {}", ex.what());
    UpdateSearchStatus("Search failed");
    m_enrichedResults.clear();
  }

  // Hide progress indicator
  CGUIMessage hideProgress(GUI_MSG_HIDDEN, GetID(), CONTROL_PROGRESS);
  CServiceBroker::GetGUI()->GetWindowManager().SendThreadMessage(hideProgress, GetID());

  m_isSearching = false;
}

void CGUIDialogSemanticSearch::UpdateResultsList()
{
  CSingleLock lock(m_criticalSection);

  m_results->Clear();

  for (const auto& result : m_enrichedResults)
  {
    auto item = CreateFileItem(result);
    m_results->Add(item);
  }

  // Update list control
  CGUIMessage msg(GUI_MSG_LABEL_BIND, GetID(), CONTROL_RESULTS_LIST, 0, 0, m_results.get());
  OnMessage(msg);

  UpdateResultCount();

  // Focus first result if available
  if (m_results->Size() > 0)
  {
    SET_CONTROL_FOCUS(CONTROL_RESULTS_LIST, 0);
  }
}

void CGUIDialogSemanticSearch::OnResultSelected(const std::shared_ptr<CFileItem>& item)
{
  if (!item || item->GetProperty("chunk_id").asInteger64() < 0)
    return;

  int64_t chunkId = item->GetProperty("chunk_id").asInteger64();

  CSingleLock lock(m_criticalSection);

  // Find the enriched result
  for (const auto& result : m_enrichedResults)
  {
    if (result.chunkId == chunkId)
    {
      // Record clicked result in history
      if (m_searchHistory && !m_currentQuery.empty())
      {
        m_searchHistory->UpdateClickedResult(m_currentQuery, chunkId);
      }

      JumpToTimestamp(result);
      break;
    }
  }
}

void CGUIDialogSemanticSearch::ShowContextPreview(const EnrichedSearchResult& result)
{
  if (!m_contextProvider)
    return;

  try
  {
    // Get context window (±30 seconds)
    HybridSearchResult hybridResult;
    hybridResult.chunkId = result.chunkId;
    hybridResult.chunk = result.chunk;

    auto context = m_contextProvider->GetContext(hybridResult, 60000);

    // Build preview text
    std::string previewText;
    for (const auto& contextChunk : context.chunks)
    {
      if (contextChunk.isMatch)
      {
        previewText += StringUtils::Format("[B]>>> {} <<<[/B]\n{}\n\n",
                                          contextChunk.formattedTime,
                                          contextChunk.chunk.text);
      }
      else
      {
        previewText += StringUtils::Format("{}\n{}\n\n",
                                          contextChunk.formattedTime,
                                          contextChunk.chunk.text);
      }
    }

    // Update context preview panel
    SET_CONTROL_LABEL(CONTROL_CONTEXT_PREVIEW, previewText);
    SET_CONTROL_VISIBLE(CONTROL_CONTEXT_PREVIEW);
  }
  catch (const std::exception& ex)
  {
    CLog::Log(LOGERROR, "CGUIDialogSemanticSearch: Failed to get context: {}", ex.what());
  }
}

void CGUIDialogSemanticSearch::JumpToTimestamp(const EnrichedSearchResult& result)
{
  // Load and play the media file at the specified timestamp
  CFileItem mediaItem;
  mediaItem.SetPath(result.filePath);

  // Set video info tag
  CVideoInfoTag* tag = mediaItem.GetVideoInfoTag();
  tag->m_iDbId = result.mediaId;
  tag->SetTitle(result.title);
  tag->m_type = result.mediaType;

  // Calculate seek time in seconds
  int64_t seekTimeMs = result.chunk.startMs;
  double seekTimeSec = static_cast<double>(seekTimeMs) / 1000.0;

  // Play the file
  auto& components = CServiceBroker::GetAppComponents();
  const auto appPlayer = components.GetComponent<CApplicationPlayer>();

  if (appPlayer)
  {
    CLog::Log(LOGINFO, "CGUIDialogSemanticSearch: Playing {} at {}s",
              result.filePath, seekTimeSec);

    // Start playback
    g_application.PlayFile(mediaItem, "", false);

    // Seek to timestamp after a short delay to ensure playback has started
    CThread::Sleep(500ms);
    appPlayer->SeekTime(seekTimeSec);
  }

  // Close the dialog
  Close();
}

void CGUIDialogSemanticSearch::ToggleSearchMode()
{
  switch (m_searchMode)
  {
    case SearchMode::Hybrid:
      SetSearchMode(SearchMode::KeywordOnly);
      break;
    case SearchMode::KeywordOnly:
      SetSearchMode(SearchMode::SemanticOnly);
      break;
    case SearchMode::SemanticOnly:
      SetSearchMode(SearchMode::Hybrid);
      break;
  }
}

void CGUIDialogSemanticSearch::ToggleMediaTypeFilter()
{
  // Cycle through: All -> Movies -> TV Shows -> Music Videos -> All
  if (m_mediaTypeFilter.empty())
  {
    SetMediaTypeFilter("movie");
  }
  else if (m_mediaTypeFilter == "movie")
  {
    SetMediaTypeFilter("episode");
  }
  else if (m_mediaTypeFilter == "episode")
  {
    SetMediaTypeFilter("musicvideo");
  }
  else
  {
    SetMediaTypeFilter("");
  }
}

void CGUIDialogSemanticSearch::ClearSearch()
{
  m_currentQuery.clear();
  m_enrichedResults.clear();
  m_results->Clear();

  // Clear search input
  CGUIEditControl* editControl =
      dynamic_cast<CGUIEditControl*>(GetControl(CONTROL_SEARCH_INPUT));
  if (editControl)
  {
    editControl->SetLabel2("");
  }

  UpdateResultsList();
  UpdateSearchStatus("Enter a search query to begin");

  SET_CONTROL_FOCUS(CONTROL_SEARCH_INPUT, 0);
}

void CGUIDialogSemanticSearch::UpdateSearchStatus(const std::string& status)
{
  SET_CONTROL_LABEL(CONTROL_STATUS_LABEL, status);
}

void CGUIDialogSemanticSearch::UpdateResultCount()
{
  std::string countText = StringUtils::Format("{} results", m_enrichedResults.size());
  SET_CONTROL_LABEL(CONTROL_RESULT_COUNT, countText);
}

std::shared_ptr<CFileItem> CGUIDialogSemanticSearch::CreateFileItem(
    const EnrichedSearchResult& result)
{
  auto item = std::make_shared<CFileItem>(result.GetDisplayTitle());

  // Set basic properties
  item->SetPath(result.filePath);
  item->SetProperty("chunk_id", result.chunkId);
  item->SetProperty("media_id", result.mediaId);
  item->SetProperty("media_type", result.mediaType);
  item->SetProperty("timestamp_ms", static_cast<int64_t>(result.chunk.startMs));
  item->SetProperty("timestamp_formatted", result.formattedTimestamp);
  item->SetProperty("relevance_score", result.combinedScore);
  item->SetProperty("keyword_score", result.keywordScore);
  item->SetProperty("vector_score", result.vectorScore);

  // Set subtitle with context info
  std::string subtitle = result.GetSubtitle();
  if (!result.formattedTimestamp.empty())
  {
    subtitle += " • " + result.formattedTimestamp;
  }
  item->SetProperty("subtitle", subtitle);

  // Set snippet as label2
  item->SetLabel2(result.snippet);

  // Set artwork
  if (!result.thumbnailPath.empty())
  {
    item->SetArt("thumb", result.thumbnailPath);
  }
  if (!result.fanartPath.empty())
  {
    item->SetArt("fanart", result.fanartPath);
  }

  // Set video info tag
  CVideoInfoTag* tag = item->GetVideoInfoTag();
  tag->m_iDbId = result.mediaId;
  tag->SetTitle(result.title);
  tag->m_type = result.mediaType;
  tag->SetPlot(result.plot);
  tag->SetOriginalTitle(result.originalTitle);
  tag->SetYear(result.year);
  tag->SetRating(result.rating);
  tag->SetGenre(result.genre);
  tag->SetPlayCount(result.runtime);

  // Episode-specific fields
  if (result.mediaType == "episode")
  {
    tag->m_strShowTitle = result.showTitle;
    tag->m_iSeason = result.seasonNumber;
    tag->m_iEpisode = result.episodeNumber;
    tag->SetTitle(result.episodeTitle);
  }

  return item;
}

std::string CGUIDialogSemanticSearch::FormatSearchMode() const
{
  switch (m_searchMode)
  {
    case SearchMode::Hybrid:
      return "Mode: Hybrid";
    case SearchMode::KeywordOnly:
      return "Mode: Keyword";
    case SearchMode::SemanticOnly:
      return "Mode: Semantic";
    default:
      return "Mode: Unknown";
  }
}

std::string CGUIDialogSemanticSearch::FormatMediaTypeFilter() const
{
  if (m_mediaTypeFilter.empty())
  {
    return "Filter: All";
  }
  else if (m_mediaTypeFilter == "movie")
  {
    return "Filter: Movies";
  }
  else if (m_mediaTypeFilter == "episode")
  {
    return "Filter: TV Shows";
  }
  else if (m_mediaTypeFilter == "musicvideo")
  {
    return "Filter: Music Videos";
  }
  else
  {
    return "Filter: " + m_mediaTypeFilter;
  }
}

void CGUIDialogSemanticSearch::ApplyFiltersToOptions()
{
  // Apply media type filter
  m_searchOptions.mediaType = m_filters.GetMediaTypeString();

  // Apply genre filter
  const auto& genres = m_filters.GetGenres();
  m_searchOptions.genres.clear();
  m_searchOptions.genres.assign(genres.begin(), genres.end());

  // Apply year range filter
  const auto& yearRange = m_filters.GetYearRange();
  m_searchOptions.minYear = yearRange.minYear;
  m_searchOptions.maxYear = yearRange.maxYear;

  // Apply rating filter
  m_searchOptions.mpaaRating = m_filters.GetRatingString();

  // Apply duration filter
  auto durationRange = m_filters.GetDurationMinutesRange();
  m_searchOptions.minDurationMinutes = durationRange.first;
  m_searchOptions.maxDurationMinutes = durationRange.second;

  // Apply source filters
  const auto& sourceFilter = m_filters.GetSourceFilter();
  m_searchOptions.includeSubtitles = sourceFilter.includeSubtitles;
  m_searchOptions.includeTranscription = sourceFilter.includeTranscription;
  m_searchOptions.includeMetadata = sourceFilter.includeMetadata;
}

void CGUIDialogSemanticSearch::UpdateFilterControls()
{
  // Update rating button
  SET_CONTROL_LABEL(CONTROL_RATING_BUTTON, m_filters.GetRatingString());

  // Update duration button
  SET_CONTROL_LABEL(CONTROL_DURATION_BUTTON, m_filters.GetDurationString());

  // Update year range labels (if controls exist)
  const auto& yearRange = m_filters.GetYearRange();
  if (yearRange.IsActive())
  {
    // Update slider controls if they exist
    // This would require actual slider control implementation
  }

  // Update source toggles
  const auto& sourceFilter = m_filters.GetSourceFilter();
  if (sourceFilter.includeSubtitles)
    SET_CONTROL_SELECTED(GetID(), CONTROL_SUBTITLE_TOGGLE, true);
  else
    SET_CONTROL_DESELECTED(GetID(), CONTROL_SUBTITLE_TOGGLE);

  if (sourceFilter.includeTranscription)
    SET_CONTROL_SELECTED(GetID(), CONTROL_TRANSCRIPTION_TOGGLE, true);
  else
    SET_CONTROL_DESELECTED(GetID(), CONTROL_TRANSCRIPTION_TOGGLE);

  if (sourceFilter.includeMetadata)
    SET_CONTROL_SELECTED(GetID(), CONTROL_METADATA_TOGGLE, true);
  else
    SET_CONTROL_DESELECTED(GetID(), CONTROL_METADATA_TOGGLE);
}

void CGUIDialogSemanticSearch::UpdateFilterBadges()
{
  // Get active filter badges
  auto badges = m_filters.GetActiveFilterBadges();

  // Build badge display string
  std::string badgeText;
  if (!badges.empty())
  {
    badgeText = StringUtils::Format("{} active filters: {}",
                                    m_filters.GetActiveFilterCount(),
                                    StringUtils::Join(badges, " • "));
  }
  else
  {
    badgeText = "No filters active";
  }

  // Update badge control label
  SET_CONTROL_LABEL(CONTROL_FILTER_BADGES, badgeText);
}

void CGUIDialogSemanticSearch::ToggleRatingFilter()
{
  RatingFilter current = m_filters.GetRating();
  RatingFilter next;

  switch (current)
  {
    case RatingFilter::All:
      next = RatingFilter::G;
      break;
    case RatingFilter::G:
      next = RatingFilter::PG;
      break;
    case RatingFilter::PG:
      next = RatingFilter::PG13;
      break;
    case RatingFilter::PG13:
      next = RatingFilter::R;
      break;
    case RatingFilter::R:
      next = RatingFilter::NC17;
      break;
    case RatingFilter::NC17:
      next = RatingFilter::Unrated;
      break;
    case RatingFilter::Unrated:
      next = RatingFilter::All;
      break;
    default:
      next = RatingFilter::All;
      break;
  }

  m_filters.SetRating(next);
  UpdateFilterControls();
  UpdateFilterBadges();

  // Re-run search with new filter
  if (!m_currentQuery.empty())
  {
    m_needsUpdate = true;
    if (!IsRunning())
      Create();
  }
}

void CGUIDialogSemanticSearch::ToggleDurationFilter()
{
  DurationFilter current = m_filters.GetDuration();
  DurationFilter next;

  switch (current)
  {
    case DurationFilter::All:
      next = DurationFilter::Short;
      break;
    case DurationFilter::Short:
      next = DurationFilter::Medium;
      break;
    case DurationFilter::Medium:
      next = DurationFilter::Long;
      break;
    case DurationFilter::Long:
      next = DurationFilter::All;
      break;
    default:
      next = DurationFilter::All;
      break;
  }

  m_filters.SetDuration(next);
  UpdateFilterControls();
  UpdateFilterBadges();

  // Re-run search with new filter
  if (!m_currentQuery.empty())
  {
    m_needsUpdate = true;
    if (!IsRunning())
      Create();
  }
}

void CGUIDialogSemanticSearch::ShowGenreSelector()
{
  // This would show a multi-select dialog for genres
  // Implementation would use CGUIDialogSelect or similar
  // For now, just log that it was called
  CLog::Log(LOGINFO, "CGUIDialogSemanticSearch: Genre selector requested");

  // TODO: Implement genre selection dialog using available genres from m_availableGenres
  // This would allow users to select multiple genres from the list
}

void CGUIDialogSemanticSearch::UpdateYearRangeSliders()
{
  // Update year range slider controls
  // This would be implemented when actual slider controls are added to the XML
  const auto& yearRange = m_filters.GetYearRange();

  if (yearRange.IsActive())
  {
    CLog::Log(LOGDEBUG, "CGUIDialogSemanticSearch: Year range filter: {} - {}",
              yearRange.minYear, yearRange.maxYear);
  }
}

void CGUIDialogSemanticSearch::ToggleSourceFilter(SourceType sourceType)
{
  SourceFilter sourceFilter = m_filters.GetSourceFilter();

  switch (sourceType)
  {
    case SourceType::SUBTITLE:
      sourceFilter.includeSubtitles = !sourceFilter.includeSubtitles;
      break;
    case SourceType::TRANSCRIPTION:
      sourceFilter.includeTranscription = !sourceFilter.includeTranscription;
      break;
    case SourceType::METADATA:
      sourceFilter.includeMetadata = !sourceFilter.includeMetadata;
      break;
  }

  m_filters.SetSourceFilter(sourceFilter);
  UpdateFilterControls();
  UpdateFilterBadges();

  // Re-run search with new filter
  if (!m_currentQuery.empty())
  {
    m_needsUpdate = true;
    if (!IsRunning())
      Create();
  }
}

void CGUIDialogSemanticSearch::ClearFilters()
{
  m_filters.Clear();
  UpdateFilterControls();
  UpdateFilterBadges();

  // Re-run search without filters
  if (!m_currentQuery.empty())
  {
    m_needsUpdate = true;
    if (!IsRunning())
      Create();
  }

  CLog::Log(LOGINFO, "CGUIDialogSemanticSearch: All filters cleared");
}

void CGUIDialogSemanticSearch::ShowFilterPresetSelector()
{
  if (!m_presetManager)
    return;

  // Get available presets
  auto presetNames = m_presetManager->GetPresetNames();

  if (presetNames.empty())
  {
    CLog::Log(LOGINFO, "CGUIDialogSemanticSearch: No filter presets available");
    return;
  }

  // TODO: Show selection dialog with preset names
  // For now, just log available presets
  CLog::Log(LOGINFO, "CGUIDialogSemanticSearch: Available presets: {}",
            StringUtils::Join(presetNames, ", "));

  // Example: Apply first preset for demonstration
  if (const FilterPreset* preset = m_presetManager->GetPreset(presetNames[0]))
  {
    ApplyFilterPreset(*preset);
  }
}

void CGUIDialogSemanticSearch::ShowSavePresetDialog()
{
  // TODO: Show dialog to enter preset name and description
  // For now, save with a default name
  std::string presetName = StringUtils::Format("Custom Preset {}",
                                               m_presetManager->GetPresetCount() + 1);

  if (m_presetManager->SavePreset(presetName, "User-created preset", m_filters))
  {
    CLog::Log(LOGINFO, "CGUIDialogSemanticSearch: Saved preset '{}'", presetName);
  }
  else
  {
    CLog::Log(LOGERROR, "CGUIDialogSemanticSearch: Failed to save preset");
  }
}

void CGUIDialogSemanticSearch::ApplyFilterPreset(const FilterPreset& preset)
{
  m_filters = preset.filters;
  UpdateFilterControls();
  UpdateFilterBadges();

  // Re-run search with preset filters
  if (!m_currentQuery.empty())
  {
    m_needsUpdate = true;
    if (!IsRunning())
      Create();
  }

  CLog::Log(LOGINFO, "CGUIDialogSemanticSearch: Applied preset '{}'", preset.name);
}

std::vector<std::string> CGUIDialogSemanticSearch::LoadGenresFromDatabase()
{
  std::vector<std::string> genres;

  if (!m_videoDatabase)
    return genres;

  // TODO: Query video database for available genres
  // This would use CVideoDatabase::GetGenresNav or similar
  // For now, return common genres as placeholders

  genres = {
      "Action",       "Adventure", "Animation", "Comedy",   "Crime",  "Documentary",
      "Drama",        "Family",    "Fantasy",   "History",  "Horror", "Music",
      "Mystery",      "Romance",   "Sci-Fi",    "Thriller", "War",    "Western"
  };

  CLog::Log(LOGINFO, "CGUIDialogSemanticSearch: Loaded {} genres", genres.size());
  return genres;
}

void CGUIDialogSemanticSearch::RecordSearchInHistory(const std::string& query, int resultCount)
{
  if (m_searchHistory && !query.empty())
  {
    m_searchHistory->AddSearch(query, resultCount);
  }
}

std::vector<std::string> CGUIDialogSemanticSearch::GetSuggestions(const std::string& partialQuery)
{
  std::vector<std::string> suggestionTexts;

  if (m_searchSuggestions && !partialQuery.empty())
  {
    auto suggestions = m_searchSuggestions->GetSuggestions(partialQuery, 10);
    for (const auto& suggestion : suggestions)
    {
      suggestionTexts.push_back(suggestion.GetDisplayLabel());
    }
  }

  return suggestionTexts;
}

void CGUIDialogSemanticSearch::ClearSearchHistory()
{
  if (m_searchHistory)
  {
    m_searchHistory->ClearHistory();
    CLog::Log(LOGINFO, "CGUIDialogSemanticSearch: Cleared search history");
  }
}

void CGUIDialogSemanticSearch::SetPrivacyMode(bool enabled)
{
  if (m_searchHistory)
  {
    m_searchHistory->SetPrivacyMode(enabled);
    CLog::Log(LOGINFO, "CGUIDialogSemanticSearch: Privacy mode {}", enabled ? "enabled" : "disabled");
  }
}

} // namespace SEMANTIC
} // namespace KODI
