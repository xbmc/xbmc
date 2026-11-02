/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "guilib/FFmpegImage.h"
#include "guilib/TextureFormats.h"
#include "test/TestUtils.h"

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iterator>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include <gtest/gtest.h>
#if defined(HAVE_LCMS2)
#include <lcms2.h>
#endif

extern "C"
{
#include <libavutil/mem.h>
}

namespace
{
// the test images are flat 32x32 JPEGs, RGB (200, 100, 100) or gray 128, made by
// tools/testdata/mkicc.py
constexpr unsigned int SIZE = 32;
constexpr char ICC_NAME[] = "ICC_PROFILE"; // with its NUL, as JPEG's APP2 segments name it

struct Pixels
{
  std::unique_ptr<uint8_t, decltype(&av_free)> data{nullptr, av_free};
  unsigned int width{0};
  unsigned int height{0};
  unsigned int pitch{0};

  // B, G, R, A
  const uint8_t* At(unsigned int x, unsigned int y) const { return data.get() + y * pitch + x * 4; }
  std::vector<uint8_t> Rows() const
  {
    std::vector<uint8_t> rows;
    if (!data)
      return rows;
    for (unsigned int y = 0; y < height; ++y)
      rows.insert(rows.end(), At(0, y), At(0, y) + width * 4);
    return rows;
  }
};

std::vector<unsigned char> ReadFixture(const std::string& name)
{
  std::ifstream file(XBMC_REF_FILE_PATH("xbmc/guilib/test/testdata/" + name), std::ios::binary);
  std::vector<unsigned char> data{std::istreambuf_iterator<char>(file), {}};
  EXPECT_FALSE(data.empty()) << name;
  return data;
}

// no data when the image can't be loaded or decoded
Pixels DecodeData(std::vector<unsigned char> data,
                  unsigned int width = SIZE,
                  unsigned int height = SIZE,
                  ImageColorMode mode = ImageColorMode::CONVERT_TO_SRGB,
                  std::optional<ImageColorMatrix>* matrix = nullptr)
{
  Pixels pixels;
  CFFmpegImage image("image/jpeg");
  if (!image.LoadImageFromMemory(data.data(), static_cast<unsigned int>(data.size()), width,
                                 height))
    return pixels;

  pixels.width = width;
  pixels.height = height;
  pixels.pitch = width * 4;
  pixels.data.reset(static_cast<uint8_t*>(av_malloc(pixels.pitch * height)));
  image.SetColorMode(mode);
  if (!image.Decode(pixels.data.get(), width, height, pixels.pitch, XB_FMT_A8R8G8B8))
    pixels.data.reset();
  if (matrix)
    *matrix = image.GetColorMatrix();
  return pixels;
}

Pixels DecodeAs(const std::string& name,
                ImageColorMode mode,
                std::optional<ImageColorMatrix>* matrix = nullptr)
{
  return DecodeData(ReadFixture(name), SIZE, SIZE, mode, matrix);
}

Pixels Decode(const std::string& name, unsigned int width = SIZE, unsigned int height = SIZE)
{
  return DecodeData(ReadFixture(name), width, height);
}

// the ICC profile in a JPEG's APP2 segments
std::vector<unsigned char> IccOf(const std::vector<unsigned char>& jpeg)
{
  std::vector<unsigned char> icc;
  for (size_t at = 2; at + 4 <= jpeg.size() && jpeg[at] == 0xFF && jpeg[at + 1] != 0xDA;)
  {
    const size_t length{static_cast<size_t>(jpeg[at + 2] << 8 | jpeg[at + 3])};
    if (jpeg[at + 1] == 0xE2 && length > 16 &&
        std::memcmp(&jpeg[at + 4], ICC_NAME, sizeof(ICC_NAME)) == 0)
      icc.insert(icc.end(), jpeg.begin() + at + 18, jpeg.begin() + at + 2 + length);
    at += 2 + length;
  }
  return icc;
}

// an untagged JPEG with the profile added in APP2 segments, right after its start of image
std::vector<unsigned char> WithIcc(const std::vector<unsigned char>& jpeg,
                                   const std::vector<unsigned char>& icc)
{
  constexpr size_t CHUNK = 65519; // 65535 less the length, name, sequence number and count
  const size_t count{(icc.size() + CHUNK - 1) / CHUNK};
  std::vector<unsigned char> out(jpeg.begin(), jpeg.begin() + 2);
  for (size_t n = 0; n < count; ++n)
  {
    const size_t size{std::min(CHUNK, icc.size() - n * CHUNK)};
    const size_t length{2 + sizeof(ICC_NAME) + 2 + size};
    out.insert(out.end(), {0xFF, 0xE2, static_cast<unsigned char>(length >> 8),
                           static_cast<unsigned char>(length & 0xFF)});
    out.insert(out.end(), ICC_NAME, ICC_NAME + sizeof(ICC_NAME));
    out.push_back(static_cast<unsigned char>(n + 1));
    out.push_back(static_cast<unsigned char>(count));
    out.insert(out.end(), icc.begin() + n * CHUNK, icc.begin() + n * CHUNK + size);
  }
  out.insert(out.end(), jpeg.begin() + 2, jpeg.end());
  return out;
}

void ExpectBGRA(const uint8_t* pixel, int b, int g, int r)
{
  EXPECT_NEAR(pixel[0], b, 2);
  EXPECT_NEAR(pixel[1], g, 2);
  EXPECT_NEAR(pixel[2], r, 2);
  EXPECT_EQ(pixel[3], 255);
}
} // namespace

TEST(TestFFmpegImage, UntaggedImageIsUnchanged)
{
  const Pixels pixels{Decode("icc-untagged.jpg")};
  ASSERT_TRUE(pixels.data);
  ExpectBGRA(pixels.At(SIZE / 2, SIZE / 2), 100, 100, 200);
}

TEST(TestFFmpegImage, SRGBProfileIsLeftAlone)
{
  const std::vector<uint8_t> untagged{Decode("icc-untagged.jpg").Rows()};
  ASSERT_FALSE(untagged.empty());
  EXPECT_EQ(Decode("icc-srgb-v4.jpg").Rows(), untagged); // parametric curves
  EXPECT_EQ(Decode("icc-srgb-v2.jpg").Rows(), untagged); // 1024-entry tables
}

TEST(TestFFmpegImage, GrayProfileIsLeftAlone)
{
  const std::vector<uint8_t> untagged{Decode("icc-gray-untagged.jpg").Rows()};
  ASSERT_FALSE(untagged.empty());
  EXPECT_EQ(Decode("icc-gray.jpg").Rows(), untagged);
}

TEST(TestFFmpegImage, BrokenProfileIsLeftAlone)
{
  const std::vector<unsigned char> jpeg{ReadFixture("icc-untagged.jpg")};
  const std::vector<uint8_t> untagged{DecodeData(jpeg).Rows()};
  ASSERT_FALSE(untagged.empty());
  std::vector<unsigned char> truncated{IccOf(ReadFixture("icc-p3.jpg"))};
  ASSERT_GT(truncated.size(), 200u);
  truncated.resize(200);
  EXPECT_EQ(DecodeData(WithIcc(jpeg, truncated)).Rows(), untagged);
  EXPECT_EQ(DecodeData(WithIcc(jpeg, std::vector<unsigned char>(1000, 0x5A))).Rows(), untagged);
}

#if defined(HAVE_LCMS2)
TEST(TestFFmpegImage, DisplayP3IsConvertedToSRGB)
{
  const Pixels pixels{Decode("icc-p3.jpg")};
  ASSERT_TRUE(pixels.data);
  ExpectBGRA(pixels.At(SIZE / 2, SIZE / 2), 97, 93, 215);
}

TEST(TestFFmpegImage, LutIsUsedOverAnSRGBMatrix)
{
  // sRGB's matrix and curves with a LUT that swaps red and blue, which lcms2 converts with
  const Pixels pixels{Decode("icc-srgb-lut.jpg")};
  ASSERT_TRUE(pixels.data);
  ExpectBGRA(pixels.At(SIZE / 2, SIZE / 2), 200, 100, 100);
}

TEST(TestFFmpegImage, CurveIsCheckedAtEveryCodeValue)
{
  // sRGB but for code value 100, which shows as 110 and falls between coarser samples
  const Pixels pixels{Decode("icc-srgb-notch.jpg")};
  ASSERT_TRUE(pixels.data);
  ExpectBGRA(pixels.At(SIZE / 2, SIZE / 2), 110, 110, 200);
}

TEST(TestFFmpegImage, ConversionFollowsTheRowStride)
{
  // 30 pixels wide gives rows of 120 bytes, padded to a wider stride inside the decoder
  const Pixels pixels{Decode("icc-p3.jpg", 30, 30)};
  ASSERT_TRUE(pixels.data);
  for (unsigned int y = 0; y < pixels.height; ++y)
  {
    for (unsigned int x = 0; x < pixels.width; ++x)
    {
      SCOPED_TRACE(testing::Message() << "x " << x << " y " << y);
      ExpectBGRA(pixels.At(x, y), 97, 93, 215);
    }
  }
}

// Display P3 to BT.2020 in linear light, by rows; GetColorMatrix() is column-major
constexpr float P3_TO_BT2020[3][3]{{0.75383f, 0.19860f, 0.04757f},
                                   {0.04574f, 0.94178f, 0.01248f},
                                   {-0.00121f, 0.01760f, 0.98361f}};

void ExpectP3ToBT2020(const std::optional<ImageColorMatrix>& matrix)
{
  ASSERT_TRUE(matrix);
  for (int r = 0; r < 3; ++r)
  {
    for (int c = 0; c < 3; ++c)
      EXPECT_NEAR((*matrix)[c * 3 + r], P3_TO_BT2020[r][c], 1e-3) << "row " << r << " col " << c;
  }
}

TEST(TestFFmpegImage, DisplayP3KeepsItsGamut)
{
  std::optional<ImageColorMatrix> matrix;
  const Pixels pixels{DecodeAs("icc-p3.jpg", ImageColorMode::KEEP_GAMUT, &matrix)};
  ASSERT_TRUE(pixels.data);
  ExpectBGRA(pixels.At(SIZE / 2, SIZE / 2), 100, 100, 200);
  ExpectP3ToBT2020(matrix);
}

TEST(TestFFmpegImage, SRGBHasNoColorMatrix)
{
  std::optional<ImageColorMatrix> matrix;
  const Pixels pixels{DecodeAs("icc-srgb-v4.jpg", ImageColorMode::KEEP_GAMUT, &matrix)};
  ASSERT_TRUE(pixels.data);
  ExpectBGRA(pixels.At(SIZE / 2, SIZE / 2), 100, 100, 200);
  EXPECT_FALSE(matrix);
}

TEST(TestFFmpegImage, LutProfileIsConvertedWhenKeepingGamut)
{
  std::optional<ImageColorMatrix> matrix;
  const Pixels pixels{DecodeAs("icc-srgb-lut.jpg", ImageColorMode::KEEP_GAMUT, &matrix)};
  ASSERT_TRUE(pixels.data);
  ExpectBGRA(pixels.At(SIZE / 2, SIZE / 2), 200, 100, 100);
  EXPECT_FALSE(matrix);
}

TEST(TestFFmpegImage, AsIsLeavesDisplayP3Alone)
{
  std::optional<ImageColorMatrix> matrix;
  const Pixels pixels{DecodeAs("icc-p3.jpg", ImageColorMode::AS_IS, &matrix)};
  ASSERT_TRUE(pixels.data);
  ExpectBGRA(pixels.At(SIZE / 2, SIZE / 2), 100, 100, 200);
  EXPECT_FALSE(matrix);
}

TEST(TestFFmpegImage, KeptGamutGetsSRGBCurves)
{
  // Display P3's primaries with a gamma 1.8 curve: 200 and 100 come out as 210 and 119 in sRGB's
  const cmsCIExyY d65{0.3127, 0.3290, 1.0};
  const cmsCIExyYTRIPLE p3{{0.680, 0.320, 1.0}, {0.265, 0.690, 1.0}, {0.150, 0.060, 1.0}};
  cmsToneCurve* gamma{cmsBuildGamma(nullptr, 1.8)};
  ASSERT_TRUE(gamma);
  cmsToneCurve* const curves[3]{gamma, gamma, gamma};
  cmsHPROFILE profile{cmsCreateRGBProfile(&d65, &p3, curves)};
  cmsFreeToneCurve(gamma);
  ASSERT_TRUE(profile);
  cmsUInt32Number size{0};
  ASSERT_TRUE(cmsSaveProfileToMem(profile, nullptr, &size));
  std::vector<unsigned char> icc(size);
  ASSERT_TRUE(cmsSaveProfileToMem(profile, icc.data(), &size));
  cmsCloseProfile(profile);

  std::optional<ImageColorMatrix> matrix;
  const Pixels pixels{DecodeData(WithIcc(ReadFixture("icc-untagged.jpg"), icc), SIZE, SIZE,
                                 ImageColorMode::KEEP_GAMUT, &matrix)};
  ASSERT_TRUE(pixels.data);
  ExpectBGRA(pixels.At(SIZE / 2, SIZE / 2), 119, 119, 210);
  ExpectP3ToBT2020(matrix);
}

#else
TEST(TestFFmpegImage, DisplayP3IsLeftAloneWithoutLcms2)
{
  const Pixels pixels{Decode("icc-p3.jpg")};
  ASSERT_TRUE(pixels.data);
  ExpectBGRA(pixels.At(SIZE / 2, SIZE / 2), 100, 100, 200);
}
#endif
