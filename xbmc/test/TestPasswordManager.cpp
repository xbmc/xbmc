/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PasswordManager.h"
#include "URL.h"
#include "utils/URIUtils.h"

#include <gtest/gtest.h>

namespace
{
constexpr const char* USERNAME = "kodi";
constexpr const char* PASSWORD = "secret";

CURL AuthenticatedUrl(const std::string& path)
{
  CURL url{path};
  url.SetUserName(USERNAME);
  url.SetPassword(PASSWORD);
  return url;
}
} // namespace

class TestPasswordManager : public testing::Test
{
protected:
  // the password manager is a singleton, so make sure no state leaks between tests
  void SetUp() override { CPasswordManager::GetInstance().Clear(); }
  void TearDown() override { CPasswordManager::GetInstance().Clear(); }

  static void Remember(const std::string& path)
  {
    CPasswordManager::GetInstance().SaveAuthenticatedURL(AuthenticatedUrl(path), false);
  }

  static void ExpectAuthenticated(const CURL& url)
  {
    EXPECT_STREQ(USERNAME, url.GetUserName().c_str());
    EXPECT_STREQ(PASSWORD, url.GetPassWord().c_str());
  }
};

TEST_F(TestPasswordManager, AuthenticatesPathBelowTheRememberedShare)
{
  // this is what a source with a username/password gives us: the credentials are remembered for
  // the source and every item below it has to pick them up again
  Remember("http://192.168.0.1:8910/media/");

  CURL url{"http://192.168.0.1:8910/media/Movies/movie.mkv"};
  ASSERT_TRUE(CPasswordManager::GetInstance().AuthenticateURL(url));
  ExpectAuthenticated(url);
}

TEST_F(TestPasswordManager, LookupIgnoresThePort)
{
  // the lookup key is built from the host name only, so the port must not influence the match.
  // passwords.xml therefore holds port-less <from> keys even for a server on a custom port.
  Remember("http://192.168.0.1:8910/media/");

  CURL url{"http://192.168.0.1:1234/media/Movies/movie.mkv"};
  ASSERT_TRUE(CPasswordManager::GetInstance().AuthenticateURL(url));
  ExpectAuthenticated(url);
}

TEST_F(TestPasswordManager, FallsBackToTheServerForAnotherShare)
{
  // different shares on the same host reuse the credentials
  Remember("http://192.168.0.1:8910/media/");

  CURL url{"http://192.168.0.1:8910/other/file.mkv"};
  ASSERT_TRUE(CPasswordManager::GetInstance().AuthenticateURL(url));
  ExpectAuthenticated(url);
}

TEST_F(TestPasswordManager, DoesNotAuthenticateAnUnknownServer)
{
  Remember("http://192.168.0.1:8910/media/");

  CURL url{"http://192.168.0.2:8910/media/Movies/movie.mkv"};
  EXPECT_FALSE(CPasswordManager::GetInstance().AuthenticateURL(url));
  EXPECT_TRUE(url.GetUserName().empty());
  EXPECT_TRUE(url.GetPassWord().empty());
}

TEST_F(TestPasswordManager, DoesNotAuthenticateWithNothingRemembered)
{
  CURL url{"http://192.168.0.1:8910/media/Movies/movie.mkv"};
  EXPECT_FALSE(CPasswordManager::GetInstance().AuthenticateURL(url));
  EXPECT_TRUE(url.GetUserName().empty());
  EXPECT_TRUE(url.GetPassWord().empty());
}

TEST_F(TestPasswordManager, DoesNotRememberAUrlWithoutAUserName)
{
  CPasswordManager::GetInstance().SaveAuthenticatedURL(CURL{"http://192.168.0.1:8910/media/"},
                                                       false);

  CURL url{"http://192.168.0.1:8910/media/Movies/movie.mkv"};
  EXPECT_FALSE(CPasswordManager::GetInstance().AuthenticateURL(url));
}

TEST_F(TestPasswordManager, SupportsTheProtocolsThatCanCarryCredentials)
{
  CPasswordManager& manager = CPasswordManager::GetInstance();

  EXPECT_TRUE(manager.IsURLSupported(CURL{"http://host/share/"}));
  EXPECT_TRUE(manager.IsURLSupported(CURL{"https://host/share/"}));
  EXPECT_TRUE(manager.IsURLSupported(CURL{"dav://host/share/"}));
  EXPECT_TRUE(manager.IsURLSupported(CURL{"davs://host/share/"}));
  EXPECT_TRUE(manager.IsURLSupported(CURL{"smb://host/share/"}));
  EXPECT_TRUE(manager.IsURLSupported(CURL{"nfs://host/share/"}));
  EXPECT_TRUE(manager.IsURLSupported(CURL{"ftp://host/share/"}));
  EXPECT_TRUE(manager.IsURLSupported(CURL{"ftps://host/share/"}));
  EXPECT_TRUE(manager.IsURLSupported(CURL{"sftp://host/share/"}));

  EXPECT_FALSE(manager.IsURLSupported(CURL{"/local/path/file.mkv"}));
}

TEST_F(TestPasswordManager, AddCredentialsFillsInTheRememberedCredentials)
{
  Remember("http://192.168.0.1:8910/media/");

  // URIUtils::AddCredentials() is what the file system layer uses to authenticate a plain url
  const CURL url{URIUtils::AddCredentials(CURL{"http://192.168.0.1:8910/media/Movies/movie.mkv"})};
  ExpectAuthenticated(url);
}

TEST_F(TestPasswordManager, AddCredentialsKeepsCredentialsAlreadyInTheUrl)
{
  Remember("http://192.168.0.1:8910/media/");

  // credentials that are already part of the url win over the remembered ones
  CURL explicitUrl{"http://192.168.0.1:8910/media/Movies/movie.mkv"};
  explicitUrl.SetUserName("other");
  explicitUrl.SetPassword("password");

  const CURL url{URIUtils::AddCredentials(explicitUrl)};
  EXPECT_STREQ("other", url.GetUserName().c_str());
  EXPECT_STREQ("password", url.GetPassWord().c_str());
}
