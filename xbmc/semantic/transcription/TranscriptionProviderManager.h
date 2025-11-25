/*
 *  Copyright (C) 2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "ITranscriptionProvider.h"

#include <map>
#include <memory>
#include <string>
#include <vector>

namespace KODI
{
namespace SEMANTIC
{

class CSemanticDatabase;

/*!
 * \brief Manages transcription providers for the semantic search system
 *
 * Handles registration, selection, configuration, and usage tracking
 * of cloud and local transcription providers. Integrates with settings
 * for API keys and budget management.
 */
class CTranscriptionProviderManager
{
public:
  CTranscriptionProviderManager();
  ~CTranscriptionProviderManager();

  /*!
   * \brief Initialize the provider manager
   * \return true if initialization was successful
   *
   * Registers built-in providers (Groq, future: OpenAI, local Whisper)
   * and loads provider configuration from database and settings.
   */
  bool Initialize();

  /*!
   * \brief Shutdown the provider manager
   *
   * Saves provider state and cleans up resources.
   */
  void Shutdown();

  /*!
   * \brief Register a transcription provider
   * \param provider The provider instance to register
   *
   * Providers are registered during Initialize() but this can also be
   * used for dynamic registration of custom providers.
   */
  void RegisterProvider(std::unique_ptr<ITranscriptionProvider> provider);

  /*!
   * \brief Get a provider by its ID
   * \param id The provider identifier (e.g., "groq")
   * \return Pointer to the provider, or nullptr if not found
   */
  ITranscriptionProvider* GetProvider(const std::string& id);

  /*!
   * \brief Get the default/preferred provider
   * \return Pointer to the default provider, or nullptr if none available
   *
   * Returns the first configured and available provider. Preference is given
   * to providers explicitly set as default via settings.
   */
  ITranscriptionProvider* GetDefaultProvider();

  /*!
   * \brief Get list of all registered provider IDs
   * \return Vector of provider identifiers
   */
  std::vector<std::string> GetAvailableProviders() const;

  /*!
   * \brief Information about a provider for UI display
   */
  struct ProviderInfo
  {
    std::string id;           //!< Provider identifier
    std::string name;         //!< Human-readable name
    bool isConfigured;        //!< Has API key/configuration
    bool isAvailable;         //!< Currently available for use
    bool isLocal;             //!< Runs locally (no API cost)
    float costPerMinute;      //!< Estimated cost per minute (USD)
  };

  /*!
   * \brief Get detailed information about all providers
   * \return Vector of provider information structures
   */
  std::vector<ProviderInfo> GetProviderInfoList() const;

  /*!
   * \brief Set the default provider for transcription
   * \param id The provider identifier to set as default
   *
   * Saves the preference to settings. If the provider is not available,
   * the first available provider will be used instead.
   */
  void SetDefaultProvider(const std::string& id);

  /*!
   * \brief Get the ID of the default provider
   * \return The default provider ID, or empty string if none set
   */
  std::string GetDefaultProviderId() const;

  /*!
   * \brief Record usage of a transcription provider
   * \param providerId Provider identifier
   * \param durationMinutes Duration of audio transcribed (in minutes)
   * \param cost Actual cost incurred (USD)
   *
   * Updates database usage statistics for budget tracking and analytics.
   */
  void RecordUsage(const std::string& providerId, float durationMinutes, float cost);

  /*!
   * \brief Get total cost incurred
   * \param providerId Optional provider ID to filter by specific provider
   * \return Total cost in USD (all-time for the provider, or all providers if empty)
   */
  float GetTotalCost(const std::string& providerId = "") const;

  /*!
   * \brief Get usage for the current month
   * \param providerId Optional provider ID to filter by specific provider
   * \return Usage in minutes for current calendar month
   */
  float GetMonthlyUsage(const std::string& providerId = "") const;

  /*!
   * \brief Check if monthly budget has been exceeded
   * \return true if budget limit has been reached
   *
   * Compares current month's cost against SETTING_SEMANTIC_MAXCOST.
   */
  bool IsBudgetExceeded() const;

  /*!
   * \brief Get remaining budget for the current month
   * \return Remaining budget in USD, or 0 if exceeded
   */
  float GetRemainingBudget() const;

private:
  /*!
   * \brief Load provider configuration from database
   *
   * Loads saved preferences, usage stats, and last-used information.
   */
  void LoadProviderSettings();

  /*!
   * \brief Save provider configuration to database
   *
   * Persists current provider state and preferences.
   */
  void SaveProviderSettings();

  /*!
   * \brief Update provider record in database
   * \param provider The provider to update
   *
   * Ensures provider exists in database with current configuration status.
   */
  void UpdateProviderInDatabase(ITranscriptionProvider* provider);

  /*!
   * \brief Calculate cost for current month from database
   * \param providerId Optional provider ID filter
   * \return Total cost for current month
   */
  float CalculateMonthlyCost(const std::string& providerId = "") const;

  std::map<std::string, std::unique_ptr<ITranscriptionProvider>> m_providers;
  std::string m_defaultProviderId;
  CSemanticDatabase* m_database{nullptr};
  bool m_initialized{false};
};

} // namespace SEMANTIC
} // namespace KODI
