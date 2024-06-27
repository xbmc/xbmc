/*
 *  Copyright (C) 2023 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "pictures/metadata/ImageMetadataParser.h"
#include "test/TestUtils.h"

#include <memory>

#include <gtest/gtest.h>

class TestMetadataExtraction : public ::testing::Test
{
protected:
  TestMetadataExtraction() = default;
};

TEST_F(TestMetadataExtraction, TestGPSImage)
{
  auto path = XBMC_REF_FILE_PATH("xbmc/pictures/metadata/test/testdata/exifgps.jpg");
  std::unique_ptr<ImageMetadata> metadata = CImageMetadataParser::ExtractMetadata(path);
  // Extract GPS tags
  EXPECT_EQ(metadata->exifInfo.GpsLat, "49° 5' 47.469\" N");
  EXPECT_EQ(metadata->exifInfo.GpsLong, "9° 23' 14.630\" E");
  EXPECT_EQ(metadata->exifInfo.GpsAlt, "333.45m");
  // Check other exif tags this image also includes
  EXPECT_EQ(metadata->exifInfo.CameraMake, "SONY");
  EXPECT_EQ(metadata->exifInfo.CameraModel, "SLT-A77V");
  EXPECT_EQ(metadata->exifInfo.DateTime, "2020:04:04 13:32:09");
  EXPECT_EQ(metadata->exifInfo.Orientation, 1);
  EXPECT_EQ(metadata->exifInfo.FlashUsed, 16);
  EXPECT_EQ(metadata->exifInfo.FocalLength, 20);
  EXPECT_EQ(static_cast<int>(metadata->exifInfo.ExposureTime * 1000), 2);
  EXPECT_EQ(metadata->exifInfo.ApertureFNumber, 8);
  EXPECT_EQ(metadata->exifInfo.MeteringMode, 5);
  EXPECT_EQ(metadata->exifInfo.ExposureProgram, 2);
  EXPECT_EQ(metadata->exifInfo.Whitebalance, 0);
  EXPECT_EQ(metadata->exifInfo.ISOequivalent, 100);
  // Check the IPTC tags of this image
  EXPECT_EQ(metadata->iptcInfo.Keywords, "blauer Himmel, Ausblick, Parkplatz, Löwenstein, Auto");
  // Generic metadata information
  EXPECT_EQ(metadata->width, 1);
  EXPECT_EQ(metadata->height, 1);
  EXPECT_TRUE(metadata->fileComment.empty());
#if (EXIV2_MAJOR_VERSION >= 0) && (EXIV2_MINOR_VERSION >= 28) && (EXIV2_PATCH_VERSION >= 2)
  // format specific (but common) metadata
  EXPECT_TRUE(metadata->isColor);
  EXPECT_EQ(metadata->encodingProcess, "Baseline DCT, Huffman coding");
#endif
}

TEST_F(TestMetadataExtraction, TestIPTC)
{
  auto path = XBMC_REF_FILE_PATH("xbmc/pictures/metadata/test/testdata/iptc.jpg");
  std::unique_ptr<ImageMetadata> metadata = CImageMetadataParser::ExtractMetadata(path);
  // Check the IPTC tags of this image
  EXPECT_TRUE(metadata->iptcInfo.Keywords.empty());
  EXPECT_EQ(metadata->iptcInfo.RecordVersion, "4");
  EXPECT_EQ(
      metadata->iptcInfo.Caption,
      "The railways of the S45 line are running very close to a small street with parking cars");
  EXPECT_EQ(metadata->iptcInfo.Headline, "The railway and the cars");
  EXPECT_EQ(metadata->iptcInfo.SpecialInstructions,
            "This photo is for metadata testing purposes only");
  EXPECT_EQ(metadata->iptcInfo.Byline, "Jane Photosty");
  EXPECT_EQ(metadata->iptcInfo.Credit, "IPTC/Jane Photosty");
  EXPECT_EQ(metadata->iptcInfo.CopyrightNotice,
            "\xa9 Copyright 2020 IPTC (Test Images) - www.iptc.org");
  EXPECT_EQ(metadata->iptcInfo.Date, "2020-01-08");
  EXPECT_EQ(metadata->iptcInfo.TimeCreated, "13:30:01+01:00");
  // Generic metadata information
  EXPECT_EQ(metadata->width, 1);
  EXPECT_EQ(metadata->height, 1);
  EXPECT_TRUE(metadata->fileComment.empty());
#if (EXIV2_MAJOR_VERSION >= 0) && (EXIV2_MINOR_VERSION >= 28) && (EXIV2_PATCH_VERSION >= 2)
  // format specific (but common) metadata
  EXPECT_TRUE(metadata->isColor);
  EXPECT_EQ(metadata->encodingProcess, "Baseline DCT, Huffman coding");
#endif
}
