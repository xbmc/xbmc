/*
 *  Copyright (C) 2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "guilib/GUIDialog.h"
#include "semantic/filters/FilterPreset.h"
#include "semantic/filters/SearchFilters.h"
#include "semantic/search/HybridSearchEngine.h"
#include "semantic/search/ResultEnricher.h"
#include "threads/CriticalSection.h"
#include "threads/Thread.h"

#include <memory>
#include <string>
#include <vector>

class CFileItem;
class CFileItemList;
class CVideoDatabase;

namespace KODI
{
namespace SEMANTIC
{

// Forward declarations
class CSemanticDatabase;
class CEmbeddingEngine;
class CVectorSearcher;
class CContextProvider;
class CSearchHistory;
class CSearchSuggestions;

/*!
 * @brief GUI dialog for semantic search in Kodi
 *
 * This dialog provides a user interface for semantic search, allowing users to:
 * - Enter search queries with real-time results
 * - View search results with media details and thumbnails
 * - Preview context around matched content
 * - Jump to specific timestamps in media playback
 * - Toggle between search modes (hybrid/keyword/semantic)
 * - Filter results by media type
 *
 * The dialog integrates with:
 * - CHybridSearchEngine for search operations
 * - CResultEnricher for media metadata
 * - CContextProvider for surrounding context
 * - CVideoDatabase for artwork and details
 *
 * Control IDs:
 * - 1: Heading label
 * - 2: Search input (edit control)
 * - 3: Results list
 * - 4: Result details panel
 * - 5: Context preview panel
 * - 6: Search mode toggle button
 * - 7: Media type filter button
 * - 8: Progress indicator
 * - 9: Close button
 * - 10: Clear search button
 * - 11: Search status label
 * - 12: Result count label
 * - 13: Filter panel group
 * - 14: Genre filter list
 * - 15: Year range slider min
 * - 16: Year range slider max
 * - 17: Rating filter button
 * - 18: Duration filter button
 * - 19: Source filter group
 * - 20: Clear filters button
 * - 21: Active filter badges container
 * - 22: Filter presets button
 * - 23: Save preset button
 * - 24: Subtitle source toggle
 * - 25: Transcription source toggle
 * - 26: Metadata source toggle
 *
 * Example usage:
 * \code
 * CGUIDialogSemanticSearch* dialog = CServiceBroker::GetGUI()
 *     ->GetWindowManager().GetWindow<CGUIDialogSemanticSearch>(WINDOW_DIALOG_SEMANTIC_SEARCH);
 * if (dialog)
 * {
 *   dialog->Open();
 * }
 * \endcode
 */
class CGUIDialogSemanticSearch : public CGUIDialog, public CThread
{
public:
  CGUIDialogSemanticSearch();
  ~CGUIDialogSemanticSearch() override;

  bool OnMessage(CGUIMessage& message) override;
  bool OnAction(const CAction& action) override;
  bool OnBack(int actionID) override;

  void OnInitWindow() override;
  void OnDeinitWindow(int nextWindowID) override;

  /*!
   * @brief Initialize the dialog with required components
   * @param database Semantic database
   * @param embeddingEngine Embedding engine
   * @param vectorSearcher Vector searcher
   * @param videoDatabase Video database
   * @return true if initialization succeeded
   */
  bool Initialize(CSemanticDatabase* database,
                  CEmbeddingEngine* embeddingEngine,
                  CVectorSearcher* vectorSearcher,
                  CVideoDatabase* videoDatabase);

  /*!
   * @brief Open the dialog with a pre-filled search query
   * @param query Initial search query
   */
  void OpenWithQuery(const std::string& query);

  /*!
   * @brief Set the media type filter
   * @param mediaType Media type to filter by (empty = all)
   */
  void SetMediaTypeFilter(const std::string& mediaType);

  /*!
   * @brief Set the search mode
   * @param mode Search mode (Hybrid, KeywordOnly, SemanticOnly)
   */
  void SetSearchMode(SearchMode mode);

  /*!
   * @brief Get autocomplete suggestions for partial query
   * @param partialQuery Partial search query
   * @return Vector of suggestions
   */
  std::vector<std::string> GetSuggestions(const std::string& partialQuery);

  /*!
   * @brief Clear search history
   */
  void ClearSearchHistory();

  /*!
   * @brief Enable or disable privacy mode
   * @param enabled true to enable privacy mode
   */
  void SetPrivacyMode(bool enabled);

protected:
  // CThread implementation
  void Process() override;

  /*!
   * @brief Update the results list based on current search
   */
  void UpdateResultsList();

  /*!
   * @brief Perform the actual search (called from worker thread)
   */
  void PerformSearch();

  /*!
   * @brief Handle selection of a search result
   * @param item The selected result item
   */
  void OnResultSelected(const std::shared_ptr<CFileItem>& item);

  /*!
   * @brief Show context preview for a result
   * @param result The result to show context for
   */
  void ShowContextPreview(const EnrichedSearchResult& result);

  /*!
   * @brief Jump to the timestamp in the media player
   * @param result The result containing the timestamp
   */
  void JumpToTimestamp(const EnrichedSearchResult& result);

  /*!
   * @brief Toggle search mode between hybrid/keyword/semantic
   */
  void ToggleSearchMode();

  /*!
   * @brief Toggle media type filter
   */
  void ToggleMediaTypeFilter();

  /*!
   * @brief Toggle rating filter
   */
  void ToggleRatingFilter();

  /*!
   * @brief Toggle duration filter
   */
  void ToggleDurationFilter();

  /*!
   * @brief Show genre selection dialog
   */
  void ShowGenreSelector();

  /*!
   * @brief Update year range sliders
   */
  void UpdateYearRangeSliders();

  /*!
   * @brief Toggle source filter (subtitle/transcription/metadata)
   */
  void ToggleSourceFilter(SourceType sourceType);

  /*!
   * @brief Clear all filters
   */
  void ClearFilters();

  /*!
   * @brief Clear the search input and results
   */
  void ClearSearch();

  /*!
   * @brief Update the search status label
   * @param status Status message to display
   */
  void UpdateSearchStatus(const std::string& status);

  /*!
   * @brief Update the result count label
   */
  void UpdateResultCount();

  /*!
   * @brief Create a CFileItem from an enriched search result
   * @param result The enriched search result
   * @return Shared pointer to the created file item
   */
  std::shared_ptr<CFileItem> CreateFileItem(const EnrichedSearchResult& result);

  /*!
   * @brief Format the search mode as a display string
   * @return Formatted mode string
   */
  std::string FormatSearchMode() const;

  /*!
   * @brief Format the media type filter as a display string
   * @return Formatted filter string
   */
  std::string FormatMediaTypeFilter() const;

  /*!
   * @brief Apply search filters to search options
   */
  void ApplyFiltersToOptions();

  /*!
   * @brief Update filter UI controls based on current filter state
   */
  void UpdateFilterControls();

  /*!
   * @brief Update active filter badges display
   */
  void UpdateFilterBadges();

  /*!
   * @brief Show filter preset selector dialog
   */
  void ShowFilterPresetSelector();

  /*!
   * @brief Show save filter preset dialog
   */
  void ShowSavePresetDialog();

  /*!
   * @brief Apply a filter preset
   * @param preset The preset to apply
   */
  void ApplyFilterPreset(const FilterPreset& preset);

  /*!
   * @brief Load available genres from video database
   * @return Vector of genre names
   */
  std::vector<std::string> LoadGenresFromDatabase();

  /*!
   * @brief Record the search in history
   * @param query Search query
   * @param resultCount Number of results
   */
  void RecordSearchInHistory(const std::string& query, int resultCount);

private:
  // Search components
  std::unique_ptr<CHybridSearchEngine> m_searchEngine;
  std::unique_ptr<CResultEnricher> m_enricher;
  std::unique_ptr<CContextProvider> m_contextProvider;
  std::unique_ptr<CSearchHistory> m_searchHistory;
  std::unique_ptr<CSearchSuggestions> m_searchSuggestions;

  // Database references
  CSemanticDatabase* m_database{nullptr};
  CEmbeddingEngine* m_embeddingEngine{nullptr};
  CVectorSearcher* m_vectorSearcher{nullptr};
  CVideoDatabase* m_videoDatabase{nullptr};

  // Current search state
  std::string m_currentQuery;
  SearchMode m_searchMode{SearchMode::Hybrid};
  std::string m_mediaTypeFilter; // empty = all
  HybridSearchOptions m_searchOptions;

  // Filter management
  CSearchFilters m_filters;
  std::unique_ptr<CFilterPresetManager> m_presetManager;
  std::vector<std::string> m_availableGenres;

  // Results
  std::unique_ptr<CFileItemList> m_results;
  std::vector<EnrichedSearchResult> m_enrichedResults;
  mutable CCriticalSection m_criticalSection;

  // UI state
  bool m_isSearching{false};
  bool m_needsUpdate{false};
  int m_selectedItemIndex{-1};

  // Debounce for real-time search
  unsigned int m_lastSearchTime{0};
  static constexpr unsigned int SEARCH_DEBOUNCE_MS = 500;
};

} // namespace SEMANTIC
} // namespace KODI
