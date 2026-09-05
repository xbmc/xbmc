/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "FileItem.h"
#include "JSONRPCTestUtils.h"
#include "ThumbLoader.h"
#include "interfaces/json-rpc/FileItemHandler.h"
#include "media/MediaType.h"
#include "utils/Variant.h"
#include "video/VideoInfoTag.h"

#include <memory>
#include <set>
#include <string>

#include <gtest/gtest.h>

using namespace JSONRPC;

namespace
{
constexpr const char* HYBRID_FOLDER = "videodb://movies/titles/42/-2/";
constexpr const char* SET_FOLDER = "videodb://movies/sets/7/";
constexpr const char* DEFAULT_VERSION = "smb://nas/Movies/Easter Parade/Easter Parade - 4K.mkv";

// HandleFileItem builds a CVideoThumbLoader, which opens the video database, unless handed a
// loader; file and filetype never consult it, so a plain one stands in.
class CTestFileItemHandler : public CFileItemHandler
{
public:
  static CVariant Handle(const std::shared_ptr<CFileItem>& item,
                         const std::set<std::string>& fields)
  {
    return HandleWith(item, fields);
  }

  // the overload a handler reaches when the fields it wants are not the ones its caller asked for
  static CVariant HandleWithProperties(const std::shared_ptr<CFileItem>& item,
                                       const CVariant& fields)
  {
    return HandleWith(item, fields);
  }

  static JSONRPC_STATUS Diagnose(const CVariant& item) { return DiagnoseUnresolvedItem(item); }

private:
  template<typename Fields>
  static CVariant HandleWith(const std::shared_ptr<CFileItem>& item, const Fields& fields)
  {
    CThumbLoader thumbLoader;
    CVariant result;
    HandleFileItem("id", true, "files", item, CVariant{CVariant::VariantTypeObject}, fields, result,
                   true, &thumbLoader);

    return result["files"][0];
  }
};

// A movie as CVideoDatabase::GetMoviesByWhere builds it: the path addresses the library item,
// the video info tag and the dyn path name the file that plays.
std::shared_ptr<CFileItem> MakeMovie()
{
  CVideoInfoTag movie;
  movie.m_type = MediaTypeMovie;
  movie.m_iDbId = 42;
  movie.m_strTitle = "Easter Parade";
  movie.m_strFileNameAndPath = DEFAULT_VERSION;

  auto item{std::make_shared<CFileItem>(movie)};
  item->SetPath("videodb://movies/titles/42");
  item->SetDynPath(DEFAULT_VERSION);

  return item;
}

// The same movie once it has a second version or an extra: it becomes a folder addressing its
// assets, while the video info tag still holds the default version's file.
std::shared_ptr<CFileItem> MakeMovieWithVersions()
{
  auto item{MakeMovie()};
  item->SetPath(HYBRID_FOLDER);
  item->SetFolder(true);
  item->SetProperty("IsHybridFolder", true);

  return item;
}

// A set as GroupUtils::Group builds it: a folder whose video info tag never named a file.
std::shared_ptr<CFileItem> MakeSet()
{
  auto item{std::make_shared<CFileItem>("Easter Parade Collection")};
  item->GetVideoInfoTag()->m_iDbId = 7;
  item->GetVideoInfoTag()->m_type = MediaTypeVideoCollection;
  item->SetPath(SET_FOLDER);
  item->SetFolder(true);

  return item;
}
} // unnamed namespace

TEST(TestFileItemHandler, MovieWithVersionsReportsAPathItsFiletypeAgreesWith)
{
  const CVariant object{
      CTestFileItemHandler::Handle(MakeMovieWithVersions(), {"file", "filetype"})};

  EXPECT_EQ("directory", object["filetype"].asString());
  EXPECT_EQ(HYBRID_FOLDER, object["file"].asString());
}

TEST(TestFileItemHandler, SetReportsAPathItsFiletypeAgreesWith)
{
  const CVariant object{CTestFileItemHandler::Handle(MakeSet(), {"file", "filetype"})};

  EXPECT_EQ("directory", object["filetype"].asString());
  EXPECT_EQ(SET_FOLDER, object["file"].asString());
}

TEST(TestFileItemHandler, MovieWithoutVersionsReportsItsFile)
{
  const CVariant object{CTestFileItemHandler::Handle(MakeMovie(), {"file", "filetype"})};

  EXPECT_EQ("file", object["filetype"].asString());
  EXPECT_EQ(DEFAULT_VERSION, object["file"].asString());
}

TEST(TestFileItemHandler, MovieWithVersionsNamesItsDefaultVersionWhereFiletypeIsAbsent)
{
  const CVariant object{CTestFileItemHandler::Handle(MakeMovieWithVersions(), {"file"})};

  EXPECT_EQ(DEFAULT_VERSION, object["file"].asString());
}

TEST(TestFileItemHandler, FieldsComeFromTheListGivenRatherThanFromTheParameters)
{
  CVariant fields{CVariant::VariantTypeArray};
  fields.push_back("file");
  fields.push_back("filetype");

  const CVariant object{CTestFileItemHandler::HandleWithProperties(MakeMovie(), fields)};

  EXPECT_EQ("file", object["filetype"].asString());
  EXPECT_EQ(DEFAULT_VERSION, object["file"].asString());
}

namespace
{

// Playlist.Item is the type both callers of DiagnoseUnresolvedItem take, so the identifiers
// it declares are exactly the ones a diagnosis has to account for.
std::set<std::string> DeclaredIdentifiers()
{
  std::set<std::string> identifiers;

  CVariant parsed;
  CJSONVariantParser::Parse(ShippedDefinition("Playlist.Item"), parsed);

  const CVariant& alternatives{parsed["Playlist.Item"]["anyOf"]};
  for (auto alternative = alternatives.begin_array(); alternative != alternatives.end_array();
       ++alternative)
  {
    const CVariant& properties{(*alternative)["properties"]};
    for (auto property = properties.begin_map(); property != properties.end_map(); ++property)
    {
      if (property->second["$ref"].asString() == "#/$defs/Library.Id")
        identifiers.insert(property->first);
    }
  }

  return identifiers;
}

class TestUnresolvedItemDiagnosis : public JSONServiceDescriptionTestBase
{
protected:
  void SetUp() override
  {
    JSONServiceDescriptionTestBase::SetUp();

    // Playlist.Item references both, so they have to be registered first
    ASSERT_TRUE(CJSONServiceDescription::AddType(ShippedDefinition("Library.Id")));
    ASSERT_TRUE(CJSONServiceDescription::AddType(ShippedDefinition("Files.Media")));
    ASSERT_TRUE(CJSONServiceDescription::AddType(ShippedDefinition("Playlist.Item")));
  }
};

} // unnamed namespace

/*!
 Every Library.Id property Playlist.Item declares must diagnose as NotFound; reading them
 from the schema keeps this test growing with the type.
 */
TEST_F(TestUnresolvedItemDiagnosis, EveryDeclaredIdentifierIsDiagnosedAsAMissingItem)
{
  const std::set<std::string> identifiers{DeclaredIdentifiers()};

  ASSERT_FALSE(identifiers.empty()) << "Playlist.Item declares no library identifiers";

  for (const std::string& identifier : identifiers)
  {
    CVariant item{CVariant::VariantTypeObject};
    item[identifier] = 1;

    EXPECT_EQ(NotFound, CTestFileItemHandler::Diagnose(item))
        << "an item naming " << identifier << " is not diagnosed as a reference that has gone";
  }
}

TEST_F(TestUnresolvedItemDiagnosis, AnItemNamingNothingResolvableIsMalformed)
{
  CVariant item{CVariant::VariantTypeObject};
  item["nosuchid"] = 1;

  EXPECT_EQ(InvalidParams, CTestFileItemHandler::Diagnose(item));
}
