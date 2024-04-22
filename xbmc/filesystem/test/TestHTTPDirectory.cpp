/*
 *  Copyright (C) 2015-2020 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "FileItem.h"
#include "FileItemList.h"
#include "URL.h"
#include "filesystem/CurlFile.h"
#include "filesystem/HTTPDirectory.h"
#include "network/WebServer.h"
#include "network/httprequesthandler/HTTPVfsHandler.h"
#include "settings/MediaSourceSettings.h"
#include "test/TestUtils.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/XTimeUtils.h"

#include <random>
#include <stdlib.h>

#include <gtest/gtest.h>

using namespace XFILE;

#define WEBSERVER_HOST "localhost"

#define SOURCE_PATH "xbmc/filesystem/test/data/httpdirectory/"

#define TEST_FILE_APACHE_DEFAULT "apache-default.html"
#define TEST_FILE_APACHE_FANCY "apache-fancy.html"
#define TEST_FILE_APACHE_HTML "apache-html.html"
#define TEST_FILE_BASIC "basic.html"
#define TEST_FILE_BASIC_MULTILINE "basic-multiline.html"
#define TEST_FILE_LIGHTTP_DEFAULT "lighttp-default.html"
#define TEST_FILE_NGINX_DEFAULT "nginx-default.html"
#define TEST_FILE_NGINX_FANCYINDEX "nginx-fancyindex.html"

#define SAMPLE_ITEM_COUNT 6

#define SAMPLE_ITEM_1_LABEL "folder1"
#define SAMPLE_ITEM_2_LABEL "folder2"
#define SAMPLE_ITEM_3_LABEL "sample3: the sampling.mpg"
#define SAMPLE_ITEM_4_LABEL "sample & samplability 4.mpg"
#define SAMPLE_ITEM_5_LABEL "sample5.mpg"
#define SAMPLE_ITEM_6_LABEL "sample6.mpg"

#define SAMPLE_ITEM_1_SIZE 0
#define SAMPLE_ITEM_2_SIZE 0
#define SAMPLE_ITEM_3_SIZE 123
#define SAMPLE_ITEM_4_SIZE 125952 // 123K
#define SAMPLE_ITEM_5_SIZE 128974848 // 123M
#define SAMPLE_ITEM_6_SIZE 132070244352 // 123G

// HTTPDirectory ignores the seconds component of parsed date/times
#define SAMPLE_ITEM_1_DATETIME "2019-01-01 01:01:00"
#define SAMPLE_ITEM_2_DATETIME "2019-02-02 02:02:00"
#define SAMPLE_ITEM_3_DATETIME "2019-03-03 03:03:00"
#define SAMPLE_ITEM_4_DATETIME "2019-04-04 04:04:00"
#define SAMPLE_ITEM_5_DATETIME "2019-05-05 05:05:00"
#define SAMPLE_ITEM_6_DATETIME "2019-06-06 06:06:00"

class TestHTTPDirectory : public testing::Test
{
protected:
  TestHTTPDirectory() : m_sourcePath(XBMC_REF_FILE_PATH(SOURCE_PATH))
  {
    std::random_device rd;
    std::mt19937 mt(rd());
    std::uniform_int_distribution<uint16_t> dist(49152, 65535);
    m_webServerPort = dist(mt);

    m_baseUrl = StringUtils::Format("http://" WEBSERVER_HOST ":{}", m_webServerPort);
  }

  ~TestHTTPDirectory() override = default;

protected:
  void SetUp() override
  {
    SetupMediaSources();

    m_webServer.Start(m_webServerPort, "", "");
    m_webServer.RegisterRequestHandler(&m_vfsHandler);
  }

  void TearDown() override
  {
    if (m_webServer.IsStarted())
      m_webServer.Stop();

    m_webServer.UnregisterRequestHandler(&m_vfsHandler);

    TearDownMediaSources();
  }

  void SetupMediaSources()
  {
    CMediaSource source;
    source.strName = "WebServer Share";
    source.strPath = m_sourcePath;
    source.vecPaths.push_back(m_sourcePath);
    source.m_allowSharing = true;
    source.m_iDriveType = CMediaSource::SOURCE_TYPE_LOCAL;
    source.m_iLockMode = LOCK_MODE_EVERYONE;
    source.m_ignore = true;

    CMediaSourceSettings::GetInstance().AddShare("videos", source);
  }

  void TearDownMediaSources() { CMediaSourceSettings::GetInstance().Clear(); }

  std::string GetUrl(const std::string& path)
  {
    if (path.empty())
      return m_baseUrl;

    return URIUtils::AddFileToFolder(m_baseUrl, path);
  }

  std::string GetUrlOfTestFile(const std::string& testFile)
  {
    if (testFile.empty())
      return "";

    std::string path = URIUtils::AddFileToFolder(m_sourcePath, testFile);
    path = CURL::Encode(path);
    path = URIUtils::AddFileToFolder("vfs", path);

    return GetUrl(path);
  }

  void CheckFileItemTypes(CFileItemList const& items)
  {
    ASSERT_EQ(items.GetObjectCount(), SAMPLE_ITEM_COUNT);

    // folders
    ASSERT_TRUE(items[0]->m_bIsFolder);
    ASSERT_TRUE(items[1]->m_bIsFolder);

    // files
    ASSERT_FALSE(items[2]->m_bIsFolder);
    ASSERT_FALSE(items[3]->m_bIsFolder);
    ASSERT_FALSE(items[4]->m_bIsFolder);
    ASSERT_FALSE(items[5]->m_bIsFolder);
  }

  void CheckFileItemLabels(CFileItemList const& items)
  {
    ASSERT_EQ(items.GetObjectCount(), SAMPLE_ITEM_COUNT);

    ASSERT_STREQ(items[0]->GetLabel().c_str(), SAMPLE_ITEM_1_LABEL);
    ASSERT_STREQ(items[1]->GetLabel().c_str(), SAMPLE_ITEM_2_LABEL);
    ASSERT_STREQ(items[2]->GetLabel().c_str(), SAMPLE_ITEM_3_LABEL);
    ASSERT_STREQ(items[3]->GetLabel().c_str(), SAMPLE_ITEM_4_LABEL);
    ASSERT_STREQ(items[4]->GetLabel().c_str(), SAMPLE_ITEM_5_LABEL);
    ASSERT_STREQ(items[5]->GetLabel().c_str(), SAMPLE_ITEM_6_LABEL);
  }

  void CheckFileItemDateTimes(CFileItemList const& items)
  {
    ASSERT_EQ(items.GetObjectCount(), SAMPLE_ITEM_COUNT);

    ASSERT_STREQ(items[0]->m_dateTime.GetAsDBDateTime().c_str(), SAMPLE_ITEM_1_DATETIME);
    ASSERT_STREQ(items[1]->m_dateTime.GetAsDBDateTime().c_str(), SAMPLE_ITEM_2_DATETIME);
    ASSERT_STREQ(items[2]->m_dateTime.GetAsDBDateTime().c_str(), SAMPLE_ITEM_3_DATETIME);
    ASSERT_STREQ(items[3]->m_dateTime.GetAsDBDateTime().c_str(), SAMPLE_ITEM_4_DATETIME);
    ASSERT_STREQ(items[4]->m_dateTime.GetAsDBDateTime().c_str(), SAMPLE_ITEM_5_DATETIME);
    ASSERT_STREQ(items[5]->m_dateTime.GetAsDBDateTime().c_str(), SAMPLE_ITEM_6_DATETIME);
  }

  void CheckFileItemSizes(CFileItemList const& items)
  {
    ASSERT_EQ(items.GetObjectCount(), SAMPLE_ITEM_COUNT);

    // folders
    ASSERT_EQ(items[0]->m_dwSize, SAMPLE_ITEM_1_SIZE);
    ASSERT_EQ(items[1]->m_dwSize, SAMPLE_ITEM_2_SIZE);

    // files - due to K/M/G conversions provided by some formats, allow for
    // non-zero values that are less than or equal to the expected file size
    ASSERT_NE(items[2]->m_dwSize, 0);
    ASSERT_LE(items[2]->m_dwSize, SAMPLE_ITEM_3_SIZE);
    ASSERT_NE(items[3]->m_dwSize, 0);
    ASSERT_LE(items[3]->m_dwSize, SAMPLE_ITEM_4_SIZE);
    ASSERT_NE(items[4]->m_dwSize, 0);
    ASSERT_LE(items[4]->m_dwSize, SAMPLE_ITEM_5_SIZE);
    ASSERT_NE(items[5]->m_dwSize, 0);
    ASSERT_LE(items[5]->m_dwSize, SAMPLE_ITEM_6_SIZE);
  }

  void CheckFileItems(CFileItemList const& items)
  {
    CheckFileItemTypes(items);
    CheckFileItemLabels(items);
  }

  void CheckFileItemsAndMetadata(CFileItemList const& items)
  {
    CheckFileItems(items);
    CheckFileItemDateTimes(items);
    CheckFileItemSizes(items);
  }

  CWebServer m_webServer;
  uint16_t m_webServerPort;
  std::string m_baseUrl;
  std::string const m_sourcePath;
  CHTTPVfsHandler m_vfsHandler;
  CHTTPDirectory m_httpDirectory;
};

TEST_F(TestHTTPDirectory, IsStarted)
{
  ASSERT_TRUE(m_webServer.IsStarted());
}

TEST_F(TestHTTPDirectory, ApacheDefaultIndex)
{
  CFileItemList items;

  ASSERT_TRUE(m_httpDirectory.Exists(CURL(GetUrlOfTestFile(TEST_FILE_APACHE_DEFAULT))));
  ASSERT_TRUE(
      m_httpDirectory.GetDirectory(CURL(GetUrlOfTestFile(TEST_FILE_APACHE_DEFAULT)), items));

  CheckFileItems(items);
}

TEST_F(TestHTTPDirectory, ApacheFancyIndex)
{
  CFileItemList items;

  ASSERT_TRUE(m_httpDirectory.Exists(CURL(GetUrlOfTestFile(TEST_FILE_APACHE_FANCY))));
  ASSERT_TRUE(m_httpDirectory.GetDirectory(CURL(GetUrlOfTestFile(TEST_FILE_APACHE_FANCY)), items));

  CheckFileItemsAndMetadata(items);
}

TEST_F(TestHTTPDirectory, ApacheHtmlIndex)
{
  CFileItemList items;

  ASSERT_TRUE(m_httpDirectory.Exists(CURL(GetUrlOfTestFile(TEST_FILE_APACHE_HTML))));
  ASSERT_TRUE(m_httpDirectory.GetDirectory(CURL(GetUrlOfTestFile(TEST_FILE_APACHE_HTML)), items));

  CheckFileItemsAndMetadata(items);
}

TEST_F(TestHTTPDirectory, BasicIndex)
{
  CFileItemList items;

  ASSERT_TRUE(m_httpDirectory.Exists(CURL(GetUrlOfTestFile(TEST_FILE_BASIC))));
  ASSERT_TRUE(m_httpDirectory.GetDirectory(CURL(GetUrlOfTestFile(TEST_FILE_BASIC)), items));

  CheckFileItems(items);
}

TEST_F(TestHTTPDirectory, BasicMultilineIndex)
{
  CFileItemList items;

  ASSERT_TRUE(m_httpDirectory.Exists(CURL(GetUrlOfTestFile(TEST_FILE_BASIC_MULTILINE))));
  ASSERT_TRUE(
      m_httpDirectory.GetDirectory(CURL(GetUrlOfTestFile(TEST_FILE_BASIC_MULTILINE)), items));

  CheckFileItems(items);
}

TEST_F(TestHTTPDirectory, LighttpDefaultIndex)
{
  CFileItemList items;

  ASSERT_TRUE(m_httpDirectory.Exists(CURL(GetUrlOfTestFile(TEST_FILE_LIGHTTP_DEFAULT))));
  ASSERT_TRUE(
      m_httpDirectory.GetDirectory(CURL(GetUrlOfTestFile(TEST_FILE_LIGHTTP_DEFAULT)), items));

  CheckFileItemsAndMetadata(items);
}

TEST_F(TestHTTPDirectory, NginxDefaultIndex)
{
  CFileItemList items;

  ASSERT_TRUE(m_httpDirectory.Exists(CURL(GetUrlOfTestFile(TEST_FILE_NGINX_DEFAULT))));
  ASSERT_TRUE(m_httpDirectory.GetDirectory(CURL(GetUrlOfTestFile(TEST_FILE_NGINX_DEFAULT)), items));

  CheckFileItemsAndMetadata(items);
}

TEST_F(TestHTTPDirectory, NginxFancyIndex)
{
  CFileItemList items;

  ASSERT_TRUE(m_httpDirectory.Exists(CURL(GetUrlOfTestFile(TEST_FILE_NGINX_FANCYINDEX))));
  ASSERT_TRUE(m_httpDirectory.GetDirectory(CURL(GetUrlOfTestFile(TEST_FILE_NGINX_FANCYINDEX)), items));

  CheckFileItemsAndMetadata(items);
}
