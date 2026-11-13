/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "FileItem.h"
#include "ThumbLoader.h"
#include "interfaces/json-rpc/FileItemHandler.h"
#include "media/MediaType.h"
#include "utils/Variant.h"
#include "video/VideoInfoTag.h"

#include <memory>

#include <gtest/gtest.h>

using namespace JSONRPC;

namespace
{
constexpr const char* DEFAULT_VERSION = "smb://nas/Movies/Easter Parade/Easter Parade - 4K.mkv";

// HandleFileItem builds a CVideoThumbLoader, which opens the video database, unless handed a
// loader; file and filetype never consult it, so a plain one stands in.
class CTestFileItemHandler : public CFileItemHandler
{
public:
  // the overload a handler reaches when the fields it wants are not the ones its caller asked for
  static CVariant HandleWithProperties(const std::shared_ptr<CFileItem>& item,
                                       const CVariant& fields)
  {
    return HandleWith(item, fields);
  }

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
  movie.SetMediaType(KODI::MEDIA::TYPE::MOVIE);
  movie.m_iDbId = 42;
  movie.m_strTitle = "Easter Parade";
  movie.m_strFileNameAndPath = DEFAULT_VERSION;

  auto item{std::make_shared<CFileItem>(movie)};
  item->SetPath("videodb://movies/titles/42");
  item->SetDynPath(DEFAULT_VERSION);

  return item;
}
} // unnamed namespace

TEST(TestFileItemHandler, FieldsComeFromTheListGivenRatherThanFromTheParameters)
{
  CVariant fields{CVariant::VariantTypeArray};
  fields.push_back("file");
  fields.push_back("filetype");

  const CVariant object{CTestFileItemHandler::HandleWithProperties(MakeMovie(), fields)};

  EXPECT_EQ("file", object["filetype"].asString());
  EXPECT_EQ(DEFAULT_VERSION, object["file"].asString());
}
