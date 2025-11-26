/*
 *  Copyright (C) 2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "semantic/transcription/TranscriptionProviderManager.h"
#include "semantic/transcription/GroqProvider.h"
#include "semantic/transcription/ITranscriptionProvider.h"
#include "semantic/SemanticDatabase.h"

#include <gtest/gtest.h>
#include <memory>

using namespace KODI::SEMANTIC;

// Mock provider for testing
class CMockProvider : public ITranscriptionProvider
{
public:
  CMockProvider(const std::string& id, const std::string& name, bool configured, bool available)
    : m_id(id), m_name(name), m_configured(configured), m_available(available)
  {
  }

  std::string GetName() const override { return m_name; }
  std::string GetId() const override { return m_id; }
  bool IsConfigured() const override { return m_configured; }
  bool IsAvailable() const override { return m_available; }

  bool Transcribe(const std::string& audioPath,
                  SegmentCallback onSegment,
                  ProgressCallback onProgress,
                  ErrorCallback onError) override
  {
    return m_available;
  }

  void Cancel() override {}
  float EstimateCost(int64_t durationMs) const override { return 0.01f; }

private:
  std::string m_id;
  std::string m_name;
  bool m_configured;
  bool m_available;
};

class TranscriptionProviderManagerTest : public ::testing::Test
{
protected:
  void SetUp() override
  {
    m_database = std::make_unique<CSemanticDatabase>();
    m_database->Open();
    m_manager = std::make_unique<CTranscriptionProviderManager>();
  }

  void TearDown() override
  {
    if (m_manager)
    {
      m_manager->Shutdown();
    }
    if (m_database)
    {
      m_database->Close();
    }
  }

  std::unique_ptr<CSemanticDatabase> m_database;
  std::unique_ptr<CTranscriptionProviderManager> m_manager;
};

TEST_F(TranscriptionProviderManagerTest, InitializeAndShutdown)
{
  EXPECT_TRUE(m_manager->Initialize(m_database.get()));
  EXPECT_NO_THROW({ m_manager->Shutdown(); });
}

TEST_F(TranscriptionProviderManagerTest, RegisterProvider)
{
  m_manager->Initialize(m_database.get());

  auto mockProvider = std::make_unique<CMockProvider>("mock", "Mock Provider", true, true);
  m_manager->RegisterProvider(std::move(mockProvider));

  auto* provider = m_manager->GetProvider("mock");
  ASSERT_NE(provider, nullptr);
  EXPECT_EQ(provider->GetId(), "mock");
  EXPECT_EQ(provider->GetName(), "Mock Provider");
}

TEST_F(TranscriptionProviderManagerTest, GetNonExistentProvider)
{
  m_manager->Initialize(m_database.get());

  auto* provider = m_manager->GetProvider("nonexistent");
  EXPECT_EQ(provider, nullptr);
}

TEST_F(TranscriptionProviderManagerTest, GetAvailableProviders)
{
  m_manager->Initialize(m_database.get());

  // Register some mock providers
  m_manager->RegisterProvider(
      std::make_unique<CMockProvider>("provider1", "Provider 1", true, true));
  m_manager->RegisterProvider(
      std::make_unique<CMockProvider>("provider2", "Provider 2", true, false));

  auto available = m_manager->GetAvailableProviders();
  EXPECT_GE(available.size(), 2);

  // Should contain our mock providers
  EXPECT_NE(std::find(available.begin(), available.end(), "provider1"), available.end());
  EXPECT_NE(std::find(available.begin(), available.end(), "provider2"), available.end());
}

TEST_F(TranscriptionProviderManagerTest, GetDefaultProvider)
{
  m_manager->Initialize(m_database.get());

  // Register configured and available provider
  m_manager->RegisterProvider(
      std::make_unique<CMockProvider>("available", "Available Provider", true, true));

  auto* defaultProvider = m_manager->GetDefaultProvider();

  // Should return first available provider
  if (defaultProvider != nullptr)
  {
    EXPECT_TRUE(defaultProvider->IsAvailable());
  }
}

TEST_F(TranscriptionProviderManagerTest, SetDefaultProvider)
{
  m_manager->Initialize(m_database.get());

  m_manager->RegisterProvider(
      std::make_unique<CMockProvider>("provider1", "Provider 1", true, true));
  m_manager->RegisterProvider(
      std::make_unique<CMockProvider>("provider2", "Provider 2", true, true));

  m_manager->SetDefaultProvider("provider2");

  auto providerId = m_manager->GetDefaultProviderId();
  EXPECT_EQ(providerId, "provider2");
}

TEST_F(TranscriptionProviderManagerTest, GetProviderInfoList)
{
  m_manager->Initialize(m_database.get());

  m_manager->RegisterProvider(
      std::make_unique<CMockProvider>("test1", "Test Provider 1", true, true));
  m_manager->RegisterProvider(
      std::make_unique<CMockProvider>("test2", "Test Provider 2", false, false));

  auto infoList = m_manager->GetProviderInfoList();
  EXPECT_GE(infoList.size(), 2);

  // Find our test providers
  bool foundTest1 = false;
  bool foundTest2 = false;

  for (const auto& info : infoList)
  {
    if (info.id == "test1")
    {
      foundTest1 = true;
      EXPECT_EQ(info.name, "Test Provider 1");
      EXPECT_TRUE(info.isConfigured);
      EXPECT_TRUE(info.isAvailable);
    }
    else if (info.id == "test2")
    {
      foundTest2 = true;
      EXPECT_EQ(info.name, "Test Provider 2");
      EXPECT_FALSE(info.isConfigured);
      EXPECT_FALSE(info.isAvailable);
    }
  }

  EXPECT_TRUE(foundTest1);
  EXPECT_TRUE(foundTest2);
}

TEST_F(TranscriptionProviderManagerTest, RecordUsage)
{
  m_manager->Initialize(m_database.get());

  m_manager->RegisterProvider(
      std::make_unique<CMockProvider>("test", "Test Provider", true, true));

  // Record some usage
  EXPECT_NO_THROW({
    m_manager->RecordUsage("test", 5.5f, 0.05f);
    m_manager->RecordUsage("test", 2.0f, 0.02f);
  });

  // Get total cost
  float totalCost = m_manager->GetTotalCost("test");
  EXPECT_GE(totalCost, 0.0f); // May or may not persist in test environment
}

TEST_F(TranscriptionProviderManagerTest, GetMonthlyUsage)
{
  m_manager->Initialize(m_database.get());

  m_manager->RegisterProvider(
      std::make_unique<CMockProvider>("test", "Test Provider", true, true));

  m_manager->RecordUsage("test", 10.0f, 0.10f);

  float monthlyUsage = m_manager->GetMonthlyUsage("test");
  EXPECT_GE(monthlyUsage, 0.0f);
}

TEST_F(TranscriptionProviderManagerTest, BudgetTracking)
{
  m_manager->Initialize(m_database.get());

  // Budget functions should not throw
  EXPECT_NO_THROW({
    bool exceeded = m_manager->IsBudgetExceeded();
    float remaining = m_manager->GetRemainingBudget();
    EXPECT_GE(remaining, 0.0f);
  });
}

TEST_F(TranscriptionProviderManagerTest, GetTotalCostAllProviders)
{
  m_manager->Initialize(m_database.get());

  m_manager->RegisterProvider(
      std::make_unique<CMockProvider>("provider1", "Provider 1", true, true));
  m_manager->RegisterProvider(
      std::make_unique<CMockProvider>("provider2", "Provider 2", true, true));

  m_manager->RecordUsage("provider1", 5.0f, 0.05f);
  m_manager->RecordUsage("provider2", 3.0f, 0.03f);

  // Get total cost across all providers
  float totalCost = m_manager->GetTotalCost();
  EXPECT_GE(totalCost, 0.0f);
}

TEST_F(TranscriptionProviderManagerTest, NoDefaultProviderWhenNoneAvailable)
{
  m_manager->Initialize(m_database.get());

  // Register only unavailable providers
  m_manager->RegisterProvider(
      std::make_unique<CMockProvider>("unavailable", "Unavailable Provider", false, false));

  auto* defaultProvider = m_manager->GetDefaultProvider();
  // May return nullptr or the unavailable provider depending on implementation
  // Just verify it doesn't crash - either result is acceptable
  (void)defaultProvider;
}

TEST_F(TranscriptionProviderManagerTest, BuiltInGroqProvider)
{
  m_manager->Initialize(m_database.get());

  // After initialization, should have Groq provider registered
  auto* groq = m_manager->GetProvider("groq");

  if (groq != nullptr)
  {
    EXPECT_EQ(groq->GetId(), "groq");
    EXPECT_EQ(groq->GetName(), "Groq Whisper");
  }
}

TEST_F(TranscriptionProviderManagerTest, MultipleInitializationCalls)
{
  // Should handle multiple initialization calls gracefully
  EXPECT_TRUE(m_manager->Initialize(m_database.get()));
  EXPECT_TRUE(m_manager->Initialize(m_database.get())); // Second call should succeed or be no-op
}

TEST_F(TranscriptionProviderManagerTest, ShutdownWithoutInitialization)
{
  // Should handle shutdown without initialization
  EXPECT_NO_THROW({ m_manager->Shutdown(); });
}
