/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DatabaseManager.h"
#include "ServiceBroker.h"
#include "TextureCache.h"
#include "TextureCacheJob.h"
#include "filesystem/File.h"
#include "jobs/JobManager.h"
#include "platform/Filesystem.h"
#include "rendering/RenderSystem.h"
#include "utils/URIUtils.h"
#include "windowing/WinSystem.h"

#include <chrono>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include <gtest/gtest.h>

using namespace XFILE;
using namespace std::chrono_literals;

namespace
{
// 2x2 images, one opaque and one with transparency, which the cache stores as .jpg and .png
const std::vector<uint8_t> OPAQUE_PNG{
    0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a, 0x00, 0x00, 0x00, 0x0d, 0x49, 0x48, 0x44,
    0x52, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x02, 0x08, 0x02, 0x00, 0x00, 0x00, 0xfd,
    0xd4, 0x9a, 0x73, 0x00, 0x00, 0x00, 0x0f, 0x49, 0x44, 0x41, 0x54, 0x78, 0x9c, 0x63, 0x60,
    0x60, 0xf8, 0x0f, 0x46, 0x60, 0x0a, 0x00, 0x17, 0xf6, 0x03, 0xfd, 0x7e, 0xeb, 0x37, 0xf3,
    0x00, 0x00, 0x00, 0x00, 0x49, 0x45, 0x4e, 0x44, 0xae, 0x42, 0x60, 0x82};
const std::vector<uint8_t> TRANSPARENT_PNG{
    0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a, 0x00, 0x00, 0x00, 0x0d, 0x49, 0x48, 0x44,
    0x52, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x02, 0x08, 0x06, 0x00, 0x00, 0x00, 0x72,
    0xb6, 0x0d, 0x24, 0x00, 0x00, 0x00, 0x11, 0x49, 0x44, 0x41, 0x54, 0x78, 0x9c, 0x63, 0xf8,
    0xcf, 0xc0, 0xd0, 0x00, 0xc2, 0x0c, 0x30, 0x06, 0x00, 0x38, 0xe8, 0x05, 0xfd, 0x11, 0x33,
    0x30, 0xc9, 0x00, 0x00, 0x00, 0x00, 0x49, 0x45, 0x4e, 0x44, 0xae, 0x42, 0x60, 0x82};

class CTestRenderSystem : public CRenderSystemBase
{
public:
  bool InitRenderSystem() override { return true; }
  bool DestroyRenderSystem() override { return true; }
  bool ResetRenderSystem(int width, int height) override { return true; }
  bool BeginRender() override { return true; }
  bool EndRender() override { return true; }
  void PresentRender(bool rendered, bool videoLayer) override {}
  bool ClearBuffers(KODI::UTILS::COLOR::Color color) override { return true; }
  bool IsExtSupported(const char* extension) const override { return false; }
  void SetViewPort(const CRect& viewPort) override {}
  void GetViewPort(CRect& viewPort) override {}
  void SetScissors(const CRect& rect) override {}
  void ResetScissors() override {}
  void CaptureStateBlock() override {}
  void ApplyStateBlock() override {}
  void SetCameraPosition(const CPoint& camera,
                         int screenWidth,
                         int screenHeight,
                         float stereoFactor) override
  {
  }
};

// Loading an image asks the render system for its maximum texture size
class CTestWinSystem : public CWinSystemBase
{
public:
  CRenderSystemBase* GetRenderSystem() override { return &m_renderSystem; }
  bool CreateNewWindow(const std::string& name, bool fullScreen, RESOLUTION_INFO& res) override
  {
    return true;
  }
  bool ResizeWindow(int newWidth, int newHeight, int newLeft, int newTop) override { return true; }
  bool SetFullScreen(bool fullScreen, RESOLUTION_INFO& res, bool blankOtherDisplays) override
  {
    return true;
  }
  void Register(IDispResource* resource) override {}
  void Unregister(IDispResource* resource) override {}

private:
  CTestRenderSystem m_renderSystem;
};

void WriteFile(const std::string& path, const std::vector<uint8_t>& data)
{
  CFile file;
  ASSERT_TRUE(file.OpenForWrite(path, true));
  ASSERT_EQ(static_cast<ssize_t>(data.size()), file.Write(data.data(), data.size()));
}

std::vector<uint8_t> ReadFile(const std::string& path)
{
  std::vector<uint8_t> data;
  CFile().LoadFile(path, data);
  return data;
}
} // namespace

class TestTextureCache : public testing::Test
{
protected:
  void SetUp() override
  {
    if (!CServiceBroker::GetDatabaseManager().CanOpen("Textures"))
      ASSERT_TRUE(CServiceBroker::GetDatabaseManager().Initialize());

    // Not under special://temp, which the cache takes as already cached
    std::error_code ec;
    m_sourcePath = KODI::PLATFORM::FILESYSTEM::create_temp_directory(ec);
    ASSERT_FALSE(ec);

    CServiceBroker::RegisterWinSystem(&m_winSystem);
    CServiceBroker::RegisterJobManager(std::make_shared<CJobManager>());
    m_cache->Initialize();
    CServiceBroker::RegisterTextureCache(m_cache);
  }

  void TearDown() override
  {
    m_cache->Deinitialize();
    CServiceBroker::GetJobManager()->CancelJobs();
    CServiceBroker::GetJobManager()->Restart();
    CServiceBroker::UnregisterTextureCache();
    CServiceBroker::UnregisterJobManager();
    CServiceBroker::UnregisterWinSystem();

    std::error_code ec;
    std::filesystem::remove_all(
        std::u8string{reinterpret_cast<const char8_t*>(m_sourcePath.data()), m_sourcePath.size()},
        ec);
  }

  //! \brief Write an image as a local file
  std::string AddSource(const std::string& name, const std::vector<uint8_t>& image) const
  {
    const std::string source{URIUtils::AddFileToFolder(m_sourcePath, name)};
    WriteFile(source, image);
    return source;
  }

  //! \brief Add a database row for the image, without a hash, naming a cached file that isn't there
  std::string AddStaleRow(const std::string& source, const std::string& extension)
  {
    CTextureDetails details;
    details.file = CTextureCache::GetCacheFile(source) + extension;
    EXPECT_TRUE(m_cache->AddCachedTexture(source, details));
    return CTextureCache::GetCachedPath(details.file);
  }

  //! \brief The cached file of an image, once it is there
  std::string WaitForCachedFile(const std::string& source)
  {
    for (auto waited = 0ms; waited < 10s; waited += 50ms)
    {
      bool needsRecaching{false};
      const std::string cached{m_cache->CheckCachedImage(source, needsRecaching)};
      if (!cached.empty() && CFile::Exists(cached))
        return cached;
      std::this_thread::sleep_for(50ms);
    }
    return {};
  }

  std::string m_sourcePath;
  CTestWinSystem m_winSystem;
  std::shared_ptr<CTextureCache> m_cache{std::make_shared<CTextureCache>()};
};

TEST_F(TestTextureCache, BackgroundCacheImageCachesAgainWhereTheFileHasGone)
{
  const std::string source{AddSource("background.png", OPAQUE_PNG)};
  const std::string cached{AddStaleRow(source, ".jpg")};

  m_cache->BackgroundCacheImage(source);

  EXPECT_EQ(cached, WaitForCachedFile(source));
}

TEST_F(TestTextureCache, ExportCachesAgainWhereTheFileHasGone)
{
  const std::string source{AddSource("export.png", OPAQUE_PNG)};
  const std::string cached{AddStaleRow(source, ".jpg")};
  const std::string destination{"special://temp/export-poster"};

  EXPECT_TRUE(m_cache->Export(source, destination, false));

  EXPECT_TRUE(CFile::Exists(destination + ".jpg"));
  EXPECT_TRUE(CFile::Exists(cached));
}

TEST_F(TestTextureCache, ExportFetchesNothingForADestinationItWontWrite)
{
  const std::string source{AddSource("existing.png", OPAQUE_PNG)};
  const std::string cached{AddStaleRow(source, ".jpg")};
  const std::string destination{"special://temp/existing-poster"};
  const std::vector<uint8_t> existing{'o', 'l', 'd'};
  WriteFile(destination + ".jpg", existing);

  EXPECT_FALSE(m_cache->Export(source, destination, false));

  EXPECT_EQ(existing, ReadFile(destination + ".jpg"));
  EXPECT_FALSE(CFile::Exists(cached));
}

TEST_F(TestTextureCache, ExportNamesAnImageCachedAgainByItsNewType)
{
  const std::string source{AddSource("newtype.png", TRANSPARENT_PNG)};
  AddStaleRow(source, ".jpg");
  const std::string destination{"special://temp/newtype-clearlogo"};

  EXPECT_TRUE(m_cache->Export(source, destination, false));

  EXPECT_TRUE(CFile::Exists(destination + ".png"));
  EXPECT_FALSE(CFile::Exists(destination + ".jpg"));
}

TEST_F(TestTextureCache, ExportKeepsAnExistingDestinationOfTheNewType)
{
  const std::string source{AddSource("newtypeexisting.png", TRANSPARENT_PNG)};
  AddStaleRow(source, ".jpg");
  const std::string destination{"special://temp/newtypeexisting-clearlogo"};
  const std::vector<uint8_t> existing{'o', 'l', 'd'};
  WriteFile(destination + ".png", existing);

  EXPECT_FALSE(m_cache->Export(source, destination, false));

  EXPECT_EQ(existing, ReadFile(destination + ".png"));
  EXPECT_FALSE(CFile::Exists(destination + ".jpg"));
}
