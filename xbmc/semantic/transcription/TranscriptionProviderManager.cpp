/*
 *  Copyright (C) 2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "TranscriptionProviderManager.h"

#include "GroqProvider.h"
#include "ServiceBroker.h"
#include "semantic/SemanticDatabase.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "utils/StringUtils.h"
#include "utils/log.h"

#include <algorithm>
#include <ctime>

namespace KODI::SEMANTIC
{

CTranscriptionProviderManager::CTranscriptionProviderManager() = default;

CTranscriptionProviderManager::~CTranscriptionProviderManager()
{
  Shutdown();
}

bool CTranscriptionProviderManager::Initialize(CSemanticDatabase* database)
{
  if (m_initialized)
  {
    CLog::LogF(LOGWARNING, "Already initialized");
    return true;
  }

  CLog::LogF(LOGINFO, "Initializing transcription provider manager");

  // Use provided database instance
  if (!database)
  {
    CLog::LogF(LOGERROR, "No database provided");
    return false;
  }
  m_database = database;

  // Register built-in providers
  CLog::LogF(LOGINFO, "Registering built-in providers");
  RegisterProvider(std::make_unique<CGroqProvider>());

  // Future providers:
  // RegisterProvider(std::make_unique<COpenAIProvider>());
  // RegisterProvider(std::make_unique<CLocalWhisperProvider>());

  // Load provider settings from database
  LoadProviderSettings();

  // Set default provider based on availability
  if (m_defaultProviderId.empty())
  {
    // Find first configured provider
    for (const auto& [id, provider] : m_providers)
    {
      if (provider->IsConfigured())
      {
        m_defaultProviderId = id;
        CLog::LogF(LOGINFO, "Auto-selected default provider: {}", id);
        break;
      }
    }
  }

  // Update all providers in database
  for (const auto& [id, provider] : m_providers)
  {
    UpdateProviderInDatabase(provider.get());
  }

  m_initialized = true;
  CLog::LogF(LOGINFO, "Initialized with {} providers ({} configured)", m_providers.size(),
             GetDefaultProvider() ? "default: " + m_defaultProviderId : "none configured");

  return true;
}

void CTranscriptionProviderManager::Shutdown()
{
  if (!m_initialized)
    return;

  CLog::LogF(LOGINFO, "Shutting down transcription provider manager");

  SaveProviderSettings();

  // Clear providers
  m_providers.clear();
  m_defaultProviderId.clear();

  // Database is owned by SemanticIndexService, just clear the pointer
  m_database = nullptr;

  m_initialized = false;
}

void CTranscriptionProviderManager::RegisterProvider(std::unique_ptr<ITranscriptionProvider> provider)
{
  if (!provider)
  {
    CLog::LogF(LOGERROR, "Attempted to register null provider");
    return;
  }

  std::string id = provider->GetId();
  std::string name = provider->GetName();

  if (m_providers.find(id) != m_providers.end())
  {
    CLog::LogF(LOGWARNING, "Provider {} already registered, replacing", id);
  }

  m_providers[id] = std::move(provider);
  CLog::LogF(LOGINFO, "Registered provider: {} ({})", name, id);
}

ITranscriptionProvider* CTranscriptionProviderManager::GetProvider(const std::string& id)
{
  auto it = m_providers.find(id);
  if (it == m_providers.end())
  {
    CLog::LogF(LOGWARNING, "Provider not found: {}", id);
    return nullptr;
  }

  return it->second.get();
}

ITranscriptionProvider* CTranscriptionProviderManager::GetDefaultProvider()
{
  // Try the explicitly set default first
  if (!m_defaultProviderId.empty())
  {
    auto* provider = GetProvider(m_defaultProviderId);
    if (provider && provider->IsConfigured() && provider->IsAvailable())
      return provider;
  }

  // Fall back to first configured and available provider
  for (const auto& [id, provider] : m_providers)
  {
    if (provider->IsConfigured() && provider->IsAvailable())
    {
      CLog::LogF(LOGDEBUG, "Using fallback provider: {}", id);
      return provider.get();
    }
  }

  CLog::LogF(LOGWARNING, "No configured and available providers");
  return nullptr;
}

std::vector<std::string> CTranscriptionProviderManager::GetAvailableProviders() const
{
  std::vector<std::string> providers;
  providers.reserve(m_providers.size());

  for (const auto& [id, provider] : m_providers)
  {
    providers.push_back(id);
  }

  return providers;
}

std::vector<CTranscriptionProviderManager::ProviderInfo>
CTranscriptionProviderManager::GetProviderInfoList() const
{
  std::vector<ProviderInfo> infoList;
  infoList.reserve(m_providers.size());

  for (const auto& [id, provider] : m_providers)
  {
    ProviderInfo info;
    info.id = provider->GetId();
    info.name = provider->GetName();
    info.isConfigured = provider->IsConfigured();
    info.isAvailable = provider->IsAvailable();

    // Determine if local or cloud-based
    // For now, all registered providers are cloud-based
    // Future local providers would be marked as isLocal = true
    info.isLocal = false;

    // Get cost estimate for 1 minute of audio
    info.costPerMinute = provider->EstimateCost(60000); // 60000ms = 1 minute

    infoList.push_back(info);
  }

  return infoList;
}

void CTranscriptionProviderManager::SetDefaultProvider(const std::string& id)
{
  if (m_providers.find(id) == m_providers.end())
  {
    CLog::LogF(LOGWARNING, "Cannot set unknown provider as default: {}", id);
    return;
  }

  m_defaultProviderId = id;
  CLog::LogF(LOGINFO, "Set default provider: {}", id);

  SaveProviderSettings();
}

std::string CTranscriptionProviderManager::GetDefaultProviderId() const
{
  return m_defaultProviderId;
}

void CTranscriptionProviderManager::RecordUsage(const std::string& providerId,
                                                 float durationMinutes,
                                                 float cost)
{
  if (!m_database)
  {
    CLog::LogF(LOGERROR, "Database not available for recording usage");
    return;
  }

  // Update provider usage in database
  if (!m_database->UpdateProviderUsage(providerId, durationMinutes))
  {
    CLog::LogF(LOGERROR, "Failed to update provider usage for {}", providerId);
    return;
  }

  CLog::LogF(LOGINFO, "Recorded usage: {} - {:.2f} minutes, ${:.4f}", providerId, durationMinutes,
             cost);

  // Check if budget exceeded
  if (IsBudgetExceeded())
  {
    CLog::LogF(LOGWARNING, "Monthly budget exceeded! Current cost: ${:.2f}",
               CalculateMonthlyCost());
  }
}

float CTranscriptionProviderManager::GetTotalCost(const std::string& providerId) const
{
  if (!m_database)
    return 0.0f;

  // If no specific provider, sum across all providers
  if (providerId.empty())
  {
    float totalCost = 0.0f;
    for (const auto& [id, provider] : m_providers)
    {
      std::string displayName;
      bool isEnabled;
      float totalMinutes;

      if (m_database->GetProvider(id, displayName, isEnabled, totalMinutes))
      {
        // Calculate cost based on provider's rate
        float costPerMin = provider->EstimateCost(60000); // Cost per minute
        totalCost += totalMinutes * costPerMin;
      }
    }
    return totalCost;
  }

  // Get cost for specific provider
  auto* provider = const_cast<CTranscriptionProviderManager*>(this)->GetProvider(providerId);
  if (!provider)
    return 0.0f;

  std::string displayName;
  bool isEnabled;
  float totalMinutes;

  if (m_database->GetProvider(providerId, displayName, isEnabled, totalMinutes))
  {
    float costPerMin = provider->EstimateCost(60000);
    return totalMinutes * costPerMin;
  }

  return 0.0f;
}

float CTranscriptionProviderManager::GetMonthlyUsage(const std::string& providerId) const
{
  // For now, return total usage
  // TODO: Implement proper monthly tracking in database
  // This would require storing usage with timestamps and filtering by current month
  return GetTotalCost(providerId);
}

bool CTranscriptionProviderManager::IsBudgetExceeded() const
{
  auto settingsComponent = CServiceBroker::GetSettingsComponent();
  if (!settingsComponent)
    return false;

  auto settings = settingsComponent->GetSettings();
  if (!settings)
    return false;

  float maxCost = settings->GetNumber(CSettings::SETTING_SEMANTIC_MAXCOST);
  float currentCost = CalculateMonthlyCost();

  return currentCost >= maxCost;
}

float CTranscriptionProviderManager::GetRemainingBudget() const
{
  auto settingsComponent = CServiceBroker::GetSettingsComponent();
  if (!settingsComponent)
    return 0.0f;

  auto settings = settingsComponent->GetSettings();
  if (!settings)
    return 0.0f;

  float maxCost = settings->GetNumber(CSettings::SETTING_SEMANTIC_MAXCOST);
  float currentCost = CalculateMonthlyCost();

  float remaining = maxCost - currentCost;
  return remaining > 0.0f ? remaining : 0.0f;
}

void CTranscriptionProviderManager::LoadProviderSettings()
{
  CLog::LogF(LOGDEBUG, "Loading provider settings from database");

  // Load default provider preference from settings
  auto settingsComponent = CServiceBroker::GetSettingsComponent();
  if (settingsComponent)
  {
    auto settings = settingsComponent->GetSettings();
    if (settings)
    {
      // Future: Load default provider from settings when setting is added
      // m_defaultProviderId = settings->GetString("semantic.transcription.defaultprovider");
    }
  }
}

void CTranscriptionProviderManager::SaveProviderSettings()
{
  CLog::LogF(LOGDEBUG, "Saving provider settings to database");

  // Update all providers in database
  for (const auto& [id, provider] : m_providers)
  {
    UpdateProviderInDatabase(provider.get());
  }

  // Future: Save default provider to settings when setting is added
}

void CTranscriptionProviderManager::UpdateProviderInDatabase(ITranscriptionProvider* provider)
{
  if (!m_database || !provider)
    return;

  std::string id = provider->GetId();
  std::string name = provider->GetName();
  bool isConfigured = provider->IsConfigured();

  // Update or insert provider record
  if (!m_database->UpdateProvider(id, name, true, isConfigured))
  {
    CLog::LogF(LOGERROR, "Failed to update provider in database: {}", id);
  }
}

float CTranscriptionProviderManager::CalculateMonthlyCost(const std::string& providerId) const
{
  // For now, this returns total cost
  // TODO: Implement proper monthly filtering in database
  // This would require:
  // 1. Adding timestamp tracking to usage records
  // 2. Querying only records from current calendar month
  // 3. Summing costs for that period

  return GetTotalCost(providerId);
}

} // namespace KODI::SEMANTIC
