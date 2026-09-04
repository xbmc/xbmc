/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "interfaces/json-rpc/FileItemHandler.h"
#include "interfaces/legacy/InfoTagVideo.h"
#include "utils/Variant.h"
#include "video/VideoInfoTag.h"

#include <gtest/gtest.h>
#include <set>
#include <string>

namespace
{
class TestFileItemHandler : public JSONRPC::CFileItemHandler
{
public:
  using CFileItemHandler::FillDetails;
};
} // unnamed namespace

TEST(TestInfoTagVideo, GetFileId)
{
  CVideoInfoTag videoInfoTag;
  videoInfoTag.m_iDbId = 42;
  videoInfoTag.m_iFileId = 84;

  XBMCAddon::xbmc::InfoTagVideo infoTag{&videoInfoTag};

  EXPECT_EQ(42, infoTag.getDbId());
  EXPECT_EQ(84, infoTag.getFileId());
}

TEST(TestInfoTagVideo, GetFileIdUnavailable)
{
  XBMCAddon::xbmc::InfoTagVideo infoTag;

  EXPECT_EQ(-1, infoTag.getFileId());
}

TEST(TestInfoTagVideo, GetFileIdJsonRpc)
{
  CVideoInfoTag videoInfoTag;
  videoInfoTag.m_iFileId = 84;

  std::set<std::string> fields{"fileid"};
  CVariant result;
  TestFileItemHandler::FillDetails(&videoInfoTag, {}, fields, result);

  EXPECT_EQ(84, result["fileid"]);
}
