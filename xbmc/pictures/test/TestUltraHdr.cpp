/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "filesystem/File.h"
#include "pictures/UltraHdr.h"
#include "test/TestUtils.h"

#include <cstdint>
#include <string>

#include <gtest/gtest.h>

using namespace KODI;

namespace
{
std::string MakeJpeg(const std::string& xmp)
{
  return "\xFF\xD8<x:xmpmeta xmlns:x=\"adobe:ns:meta/\">" + xmp + "</x:xmpmeta>\xFF\xD9";
}

std::string MakeUltraHdr(const std::string& gainMapXmp, const std::string& length = "")
{
  const std::string gainMap = MakeJpeg(gainMapXmp);
  const std::string primary = MakeJpeg(
      "<rdf:Description hdrgm:Version=\"1.0\"><Container:Directory><rdf:Seq>"
      "<rdf:li><Container:Item Item:Semantic=\"Primary\" Item:Mime=\"image/jpeg\"/></rdf:li>"
      "<rdf:li><Container:Item Item:Semantic=\"GainMap\" Item:Mime=\"image/jpeg\" Item:Length=\"" +
      (length.empty() ? std::to_string(gainMap.size()) : length) +
      "\"/></rdf:li></rdf:Seq></Container:Directory></rdf:Description>");
  return primary + gainMap;
}

std::string LittleEndian(uint32_t value, size_t bytes)
{
  std::string out;
  for (size_t i = 0; i < bytes; ++i)
    out += static_cast<char>((value >> (8 * i)) & 0xFF);
  return out;
}

// An APP2 MPF segment whose second MP entry is the gain map, at an offset counted from the TIFF
// header that follows "MPF\0"
std::string MakeMpf(uint32_t gainMapSize, uint32_t gainMapOffset)
{
  const std::string tiff = std::string("II\x2A\0", 4) + LittleEndian(8, 4) + LittleEndian(1, 2) +
                           LittleEndian(0xB002, 2) + LittleEndian(7, 2) + LittleEndian(32, 4) +
                           LittleEndian(26, 4) + LittleEndian(0, 4) + LittleEndian(0x030000, 4) +
                           LittleEndian(0, 4) + LittleEndian(0, 4) + LittleEndian(0, 4) +
                           LittleEndian(0, 4) + LittleEndian(gainMapSize, 4) +
                           LittleEndian(gainMapOffset, 4) + LittleEndian(0, 4);
  const std::string content = std::string("MPF\0", 4) + tiff;
  const size_t length = content.size() + 2;
  return std::string("\xFF\xE2") + static_cast<char>(length >> 8) +
         static_cast<char>(length & 0xFF) + content;
}

PICTURE::GainMap Read(const std::string& contents)
{
  XFILE::CFile* file = XBMC_CREATETEMPFILE(".jpg");
  EXPECT_NE(file, nullptr);
  if (!file)
    return {};
  file->Write(contents.data(), contents.size());
  file->Close();

  PICTURE::GainMap gainMap = PICTURE::ReadGainMap(XBMC_TEMPFILEPATH(file));
  XBMC_DELETETEMPFILE(file);
  return gainMap;
}
} // namespace

TEST(TestUltraHdr, GainMap)
{
  const std::string gainMapXmp =
      "<rdf:Description hdrgm:Version=\"1.0\" hdrgm:GainMapMin=\"0.5\" hdrgm:GainMapMax=\"2.5\" "
      "hdrgm:Gamma=\"1.2\" hdrgm:OffsetSDR=\"0\" hdrgm:OffsetHDR=\"0\" "
      "hdrgm:HDRCapacityMin=\"0\" hdrgm:HDRCapacityMax=\"2.5\"/>";
  const PICTURE::GainMap gainMap = Read(MakeUltraHdr(gainMapXmp));

  EXPECT_EQ(gainMap.image, MakeJpeg(gainMapXmp));
  EXPECT_FLOAT_EQ(gainMap.params.min, 0.5f);
  EXPECT_FLOAT_EQ(gainMap.params.max, 2.5f);
  EXPECT_FLOAT_EQ(gainMap.params.gamma, 1.2f);
  EXPECT_FLOAT_EQ(gainMap.params.offsetSdr, 0.0f);
  EXPECT_FLOAT_EQ(gainMap.params.offsetHdr, 0.0f);
  EXPECT_FLOAT_EQ(gainMap.params.capacityMin, 0.0f);
  EXPECT_FLOAT_EQ(gainMap.params.capacityMax, 2.5f);
}

TEST(TestUltraHdr, ValuePerChannel)
{
  const PICTURE::GainMap gainMap =
      Read(MakeUltraHdr("<rdf:Description hdrgm:Version=\"1.0\" hdrgm:HDRCapacityMax=\"3\">"
                        "<hdrgm:GainMapMax><rdf:Seq><rdf:li>3</rdf:li><rdf:li>2.9</rdf:li>"
                        "<rdf:li>2.8</rdf:li></rdf:Seq></hdrgm:GainMapMax></rdf:Description>"));

  EXPECT_FALSE(gainMap.image.empty());
  EXPECT_FLOAT_EQ(gainMap.params.max, 3.0f);
  EXPECT_FLOAT_EQ(gainMap.params.offsetSdr, 1.0f / 64);
}

TEST(TestUltraHdr, NoGainMap)
{
  EXPECT_TRUE(Read(MakeJpeg("<rdf:Description/>")).image.empty());
  EXPECT_TRUE(Read(MakeUltraHdr("<rdf:Description hdrgm:Version=\"1.0\" "
                                "hdrgm:BaseRenditionIsHDR=\"True\"/>"))
                  .image.empty());
  EXPECT_TRUE(Read(MakeUltraHdr("<rdf:Description hdrgm:Version=\"1.0\"/>", "18446744073709551615"))
                  .image.empty());
}

TEST(TestUltraHdr, MotionPhotoWithoutItsClip)
{
  // An editor dropped the clip but left its length in the XMP container; the MPF index is still right
  const std::string gainMapXmp = "<rdf:Description hdrgm:Version=\"1.0\" hdrgm:GainMapMax=\"2\" "
                                 "hdrgm:HDRCapacityMax=\"2\"/>";
  const std::string gainMap = MakeJpeg(gainMapXmp);
  const std::string xmp =
      "<x:xmpmeta xmlns:x=\"adobe:ns:meta/\"><rdf:Description hdrgm:Version=\"1.0\">"
      "<Container:Directory><rdf:Seq>"
      "<rdf:li><Container:Item Item:Semantic=\"Primary\" Item:Mime=\"image/jpeg\"/></rdf:li>"
      "<rdf:li><Container:Item Item:Semantic=\"GainMap\" Item:Mime=\"image/jpeg\" Item:Length=\"" +
      std::to_string(gainMap.size()) +
      "\"/></rdf:li><rdf:li><Container:Item Item:Semantic=\"MotionPhoto\" Item:Mime=\"video/mp4\" "
      "Item:Length=\"99999\"/></rdf:li></rdf:Seq></Container:Directory></rdf:Description>"
      "</x:xmpmeta>\xFF\xD9";
  const size_t primarySize = 2 + MakeMpf(0, 0).size() + xmp.size();
  // Offsets count from the TIFF header: SOI, the APP2 marker and length, then "MPF\0"
  const std::string primary =
      "\xFF\xD8" +
      MakeMpf(static_cast<uint32_t>(gainMap.size()), static_cast<uint32_t>(primarySize - 10)) + xmp;

  const PICTURE::GainMap read = Read(primary + gainMap);
  EXPECT_EQ(read.image, gainMap);
  EXPECT_FLOAT_EQ(read.params.max, 2.0f);
}
