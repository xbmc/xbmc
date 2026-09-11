/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "URL.h"
#include "addons/Scraper.h"
#include "interfaces/json-rpc/AudioLibrary.h"

#include <string>

#include <gtest/gtest.h>

using namespace JSONRPC;

namespace
{

// ResolveInfoProviderView is protected, so a subclass reaches it.
class TestableAudioLibrary : public CAudioLibrary
{
public:
  using CAudioLibrary::ResolveInfoProviderView;
};

// The listing without its options, so a test can say which listing was resolved without
// depending on the order the options come back in.
std::string ListingOf(const std::string& viewPath)
{
  const CURL url{viewPath};
  return url.GetWithoutOptions();
}

bool HasOption(const std::string& viewPath, const std::string& key, const std::string& value)
{
  const CURL url{viewPath};
  const auto& options = url.GetOptions();
  return options.find("?" + key + "=" + value) != std::string::npos ||
         options.find("&" + key + "=" + value) != std::string::npos;
}

bool HasNoOption(const std::string& viewPath, const std::string& key)
{
  const CURL url{viewPath};
  return url.GetOptions().find(key + "=") == std::string::npos;
}

} // namespace

// The id that names one item in the listing is what the view scope drops. On an albums listing
// that is albumid: a path naming a single album is not a view of one album.
TEST(TestAudioLibraryInfoProviderView, AnAlbumsListingDropsAlbumid)
{
  ADDON::ContentType content{ADDON::ContentType::NONE};
  std::string viewPath;

  ASSERT_TRUE(TestableAudioLibrary::ResolveInfoProviderView("musicdb://albums/?albumid=3", content,
                                                            viewPath));

  EXPECT_EQ(ADDON::ContentType::ALBUMS, content);
  EXPECT_EQ("musicdb://albums/", ListingOf(viewPath));
  EXPECT_TRUE(HasNoOption(viewPath, "albumid")) << viewPath;
}

TEST(TestAudioLibraryInfoProviderView, AnArtistsListingDropsArtistid)
{
  ADDON::ContentType content{ADDON::ContentType::NONE};
  std::string viewPath;

  ASSERT_TRUE(TestableAudioLibrary::ResolveInfoProviderView("musicdb://artists/?artistid=5",
                                                            content, viewPath));

  EXPECT_EQ(ADDON::ContentType::ARTISTS, content);
  EXPECT_EQ("musicdb://artists/", ListingOf(viewPath));
  EXPECT_TRUE(HasNoOption(viewPath, "artistid")) << viewPath;
}

// artistid on an albums listing is a filter, not the name of a single album, and
// CMusicDatabase::GetFilter applies it. Dropping it turns "this artist's albums" into every
// album in the library, which is what SetScraperAll would then rewrite.
TEST(TestAudioLibraryInfoProviderView, AnAlbumsListingKeepsArtistid)
{
  ADDON::ContentType content{ADDON::ContentType::NONE};
  std::string viewPath;

  ASSERT_TRUE(TestableAudioLibrary::ResolveInfoProviderView("musicdb://albums/?artistid=5", content,
                                                            viewPath));

  EXPECT_EQ(ADDON::ContentType::ALBUMS, content);
  EXPECT_EQ("musicdb://albums/", ListingOf(viewPath));
  EXPECT_TRUE(HasOption(viewPath, "artistid", "5")) << viewPath;
}

TEST(TestAudioLibraryInfoProviderView, AnArtistsListingKeepsAlbumid)
{
  ADDON::ContentType content{ADDON::ContentType::NONE};
  std::string viewPath;

  ASSERT_TRUE(TestableAudioLibrary::ResolveInfoProviderView("musicdb://artists/?albumid=3", content,
                                                            viewPath));

  EXPECT_EQ(ADDON::ContentType::ARTISTS, content);
  EXPECT_TRUE(HasOption(viewPath, "albumid", "3")) << viewPath;
}

// The same filter spelled as a path segment has to survive too - this is the form the GUI
// navigates with. An artist's node lists that artist's albums, so the listing is albums and the
// artist is the filter on it.
TEST(TestAudioLibraryInfoProviderView, AnAlbumsListingUnderAnArtistKeepsThatArtist)
{
  ADDON::ContentType content{ADDON::ContentType::NONE};
  std::string viewPath;

  ASSERT_TRUE(
      TestableAudioLibrary::ResolveInfoProviderView("musicdb://artists/5/", content, viewPath));

  EXPECT_EQ(ADDON::ContentType::ALBUMS, content);
  EXPECT_EQ("musicdb://albums/", ListingOf(viewPath));
  EXPECT_TRUE(HasOption(viewPath, "artistid", "5")) << viewPath;
}

TEST(TestAudioLibraryInfoProviderView, AnAlbumsListingUnderAGenreKeepsThatGenre)
{
  ADDON::ContentType content{ADDON::ContentType::NONE};
  std::string viewPath;

  ASSERT_TRUE(TestableAudioLibrary::ResolveInfoProviderView("musicdb://genres/7/albums/", content,
                                                            viewPath));

  EXPECT_EQ(ADDON::ContentType::ALBUMS, content);
  EXPECT_EQ("musicdb://albums/", ListingOf(viewPath));
  EXPECT_TRUE(HasOption(viewPath, "genreid", "7")) << viewPath;
}

// Only artists and albums carry an information provider.
TEST(TestAudioLibraryInfoProviderView, ASongsListingIsRefused)
{
  ADDON::ContentType content{ADDON::ContentType::NONE};
  std::string viewPath;

  EXPECT_FALSE(
      TestableAudioLibrary::ResolveInfoProviderView("musicdb://songs/", content, viewPath));
}

TEST(TestAudioLibraryInfoProviderView, APathThatIsNotAMusicListingIsRefused)
{
  ADDON::ContentType content{ADDON::ContentType::NONE};
  std::string viewPath;

  EXPECT_FALSE(
      TestableAudioLibrary::ResolveInfoProviderView("videodb://movies/titles/", content, viewPath));
}
