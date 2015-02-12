/*
 *      Copyright (C) 2015 Team XBMC
 *      http://xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include <gtest/gtest.h>

#include "utils/HttpRangeUtils.h"

#define RANGES_START  "bytes="

static const uint64_t DefaultFirstPosition = 1;
static const uint64_t DefaultLastPosition = 0;
static const uint64_t DefaultLength = 0;
static const void* DefaultData = NULL;

TEST(TestHttpRange, FirstPosition)
{
  const uint64_t expectedFirstPosition = 25;

  CHttpRange range;
  EXPECT_EQ(DefaultFirstPosition, range.GetFirstPosition());

  range.SetFirstPosition(expectedFirstPosition);
  EXPECT_EQ(expectedFirstPosition, range.GetFirstPosition());
}

TEST(TestHttpRange, LastPosition)
{
  const uint64_t expectedLastPosition = 25;

  CHttpRange range;
  EXPECT_EQ(DefaultLastPosition, range.GetLastPosition());

  range.SetLastPosition(expectedLastPosition);
  EXPECT_EQ(expectedLastPosition, range.GetLastPosition());
}

TEST(TestHttpRange, Length)
{
  const uint64_t expectedFirstPosition = 10;
  const uint64_t expectedLastPosition = 25;
  const uint64_t expectedLength = expectedLastPosition - expectedFirstPosition + 1;

  CHttpRange range;
  EXPECT_EQ(DefaultLength, range.GetLength());

  range.SetFirstPosition(expectedFirstPosition);
  range.SetLastPosition(expectedLastPosition);
  EXPECT_EQ(expectedLength, range.GetLength());

  CHttpRange range_length;
  range.SetFirstPosition(expectedFirstPosition);
  range.SetLength(expectedLength);
  EXPECT_EQ(expectedLastPosition, range.GetLastPosition());
  EXPECT_EQ(expectedLength, range.GetLength());
}

TEST(TestHttpRange, IsValid)
{
  const uint64_t validFirstPosition = 10;
  const uint64_t validLastPosition = 25;
  const uint64_t invalidLastPosition = 5;

  CHttpRange range;
  EXPECT_FALSE(range.IsValid());

  range.SetFirstPosition(validFirstPosition);
  EXPECT_FALSE(range.IsValid());

  range.SetLastPosition(invalidLastPosition);
  EXPECT_FALSE(range.IsValid());

  range.SetLastPosition(validLastPosition);
  EXPECT_TRUE(range.IsValid());
}

TEST(TestHttpRange, Ctor)
{
  const uint64_t validFirstPosition = 10;
  const uint64_t validLastPosition = 25;
  const uint64_t invalidLastPosition = 5;
  const uint64_t validLength = validLastPosition - validFirstPosition + 1;

  CHttpRange range_invalid(validFirstPosition, invalidLastPosition);
  EXPECT_EQ(validFirstPosition, range_invalid.GetFirstPosition());
  EXPECT_EQ(invalidLastPosition, range_invalid.GetLastPosition());
  EXPECT_EQ(DefaultLength, range_invalid.GetLength());
  EXPECT_FALSE(range_invalid.IsValid());

  CHttpRange range_valid(validFirstPosition, validLastPosition);
  EXPECT_EQ(validFirstPosition, range_valid.GetFirstPosition());
  EXPECT_EQ(validLastPosition, range_valid.GetLastPosition());
  EXPECT_EQ(validLength, range_valid.GetLength());
  EXPECT_TRUE(range_valid.IsValid());
}

TEST(TestHttpResponseRange, SetData)
{
  const uint64_t validFirstPosition = 1;
  const uint64_t validLastPosition = 2;
  const uint64_t validLength = validLastPosition - validFirstPosition + 1;
  const char* validData = "test";
  const void* invalidData = DefaultData;
  const size_t validDataLength = strlen(validData);
  const size_t invalidDataLength = 1;

  CHttpResponseRange range;
  EXPECT_EQ(DefaultData, range.GetData());
  EXPECT_FALSE(range.IsValid());

  range.SetData(invalidData);
  EXPECT_EQ(invalidData, range.GetData());
  EXPECT_FALSE(range.IsValid());

  range.SetData(validData);
  EXPECT_EQ(validData, range.GetData());
  EXPECT_FALSE(range.IsValid());

  range.SetData(invalidData, 0);
  EXPECT_EQ(validData, range.GetData());
  EXPECT_FALSE(range.IsValid());

  range.SetData(invalidData, invalidDataLength);
  EXPECT_EQ(invalidData, range.GetData());
  EXPECT_FALSE(range.IsValid());

  range.SetData(validData, validDataLength);
  EXPECT_EQ(validData, range.GetData());
  EXPECT_EQ(0, range.GetFirstPosition());
  EXPECT_EQ(validDataLength - 1, range.GetLastPosition());
  EXPECT_EQ(validDataLength, range.GetLength());
  EXPECT_TRUE(range.IsValid());

  range.SetData(invalidData, 0, 0);
  EXPECT_EQ(invalidData, range.GetData());
  EXPECT_FALSE(range.IsValid());

  range.SetData(validData, validFirstPosition, validLastPosition);
  EXPECT_EQ(validData, range.GetData());
  EXPECT_EQ(validFirstPosition, range.GetFirstPosition());
  EXPECT_EQ(validLastPosition, range.GetLastPosition());
  EXPECT_EQ(validLength, range.GetLength());
  EXPECT_TRUE(range.IsValid());
}

TEST(TestHttpRanges, Ctor)
{
  CHttpRange range;
  uint64_t position;

  CHttpRanges ranges_empty;

  EXPECT_EQ(0, ranges_empty.Size());
  EXPECT_TRUE(ranges_empty.Get().empty());

  EXPECT_FALSE(ranges_empty.Get(0, range));
  EXPECT_FALSE(ranges_empty.GetFirst(range));
  EXPECT_FALSE(ranges_empty.GetLast(range));

  EXPECT_FALSE(ranges_empty.GetFirstPosition(position));
  EXPECT_FALSE(ranges_empty.GetLastPosition(position));
  EXPECT_EQ(0, ranges_empty.GetLength());
  EXPECT_FALSE(ranges_empty.GetTotalRange(range));
}

TEST(TestHttpRanges, GetAll)
{
  CHttpRange range_0(0, 2);
  CHttpRange range_1(4, 6);
  CHttpRange range_2(8, 10);

  HttpRanges ranges_raw;
  ranges_raw.push_back(range_0);
  ranges_raw.push_back(range_1);
  ranges_raw.push_back(range_2);
  
  CHttpRanges ranges(ranges_raw);

  const HttpRanges& ranges_raw_get = ranges.Get();
  ASSERT_EQ(ranges_raw.size(), ranges_raw_get.size());

  for (size_t i = 0; i < ranges_raw.size(); ++i)
    EXPECT_EQ(ranges_raw.at(i), ranges_raw_get.at(i));
}

TEST(TestHttpRanges, GetIndex)
{
  CHttpRange range_0(0, 2);
  CHttpRange range_1(4, 6);
  CHttpRange range_2(8, 10);

  HttpRanges ranges_raw;
  ranges_raw.push_back(range_0);
  ranges_raw.push_back(range_1);
  ranges_raw.push_back(range_2);

  CHttpRanges ranges(ranges_raw);

  CHttpRange range;
  EXPECT_TRUE(ranges.Get(0, range));
  EXPECT_EQ(range_0, range);

  EXPECT_TRUE(ranges.Get(1, range));
  EXPECT_EQ(range_1, range);

  EXPECT_TRUE(ranges.Get(2, range));
  EXPECT_EQ(range_2, range);

  EXPECT_FALSE(ranges.Get(3, range));
}

TEST(TestHttpRanges, GetFirst)
{
  CHttpRange range_0(0, 2);
  CHttpRange range_1(4, 6);
  CHttpRange range_2(8, 10);

  HttpRanges ranges_raw;
  ranges_raw.push_back(range_0);
  ranges_raw.push_back(range_1);
  ranges_raw.push_back(range_2);

  CHttpRanges ranges(ranges_raw);

  CHttpRange range;
  EXPECT_TRUE(ranges.GetFirst(range));
  EXPECT_EQ(range_0, range);
}

TEST(TestHttpRanges, GetLast)
{
  CHttpRange range_0(0, 2);
  CHttpRange range_1(4, 6);
  CHttpRange range_2(8, 10);

  HttpRanges ranges_raw;
  ranges_raw.push_back(range_0);
  ranges_raw.push_back(range_1);
  ranges_raw.push_back(range_2);

  CHttpRanges ranges(ranges_raw);

  CHttpRange range;
  EXPECT_TRUE(ranges.GetLast(range));
  EXPECT_EQ(range_2, range);
}

TEST(TestHttpRanges, Size)
{
  CHttpRange range_0(0, 2);
  CHttpRange range_1(4, 6);
  CHttpRange range_2(8, 10);

  HttpRanges ranges_raw;
  ranges_raw.push_back(range_0);
  ranges_raw.push_back(range_1);
  ranges_raw.push_back(range_2);

  CHttpRanges ranges_empty;
  EXPECT_EQ(0, ranges_empty.Size());

  CHttpRanges ranges(ranges_raw);
  EXPECT_EQ(ranges_raw.size(), ranges.Size());
}

TEST(TestHttpRanges, GetFirstPosition)
{
  CHttpRange range_0(0, 2);
  CHttpRange range_1(4, 6);
  CHttpRange range_2(8, 10);

  HttpRanges ranges_raw;
  ranges_raw.push_back(range_0);
  ranges_raw.push_back(range_1);
  ranges_raw.push_back(range_2);

  CHttpRanges ranges(ranges_raw);

  uint64_t position;
  EXPECT_TRUE(ranges.GetFirstPosition(position));
  EXPECT_EQ(range_0.GetFirstPosition(), position);
}

TEST(TestHttpRanges, GetLastPosition)
{
  CHttpRange range_0(0, 2);
  CHttpRange range_1(4, 6);
  CHttpRange range_2(8, 10);

  HttpRanges ranges_raw;
  ranges_raw.push_back(range_0);
  ranges_raw.push_back(range_1);
  ranges_raw.push_back(range_2);

  CHttpRanges ranges(ranges_raw);

  uint64_t position;
  EXPECT_TRUE(ranges.GetLastPosition(position));
  EXPECT_EQ(range_2.GetLastPosition(), position);
}

TEST(TestHttpRanges, GetLength)
{
  CHttpRange range_0(0, 2);
  CHttpRange range_1(4, 6);
  CHttpRange range_2(8, 10);
  const uint64_t expectedLength = range_0.GetLength() + range_1.GetLength() + range_2.GetLength();

  HttpRanges ranges_raw;
  ranges_raw.push_back(range_0);
  ranges_raw.push_back(range_1);
  ranges_raw.push_back(range_2);

  CHttpRanges ranges(ranges_raw);

  EXPECT_EQ(expectedLength, ranges.GetLength());
}

TEST(TestHttpRanges, GetTotalRange)
{
  CHttpRange range_0(0, 2);
  CHttpRange range_1(4, 6);
  CHttpRange range_2(8, 10);
  CHttpRange range_total_expected(range_0.GetFirstPosition(), range_2.GetLastPosition());

  HttpRanges ranges_raw;
  ranges_raw.push_back(range_0);
  ranges_raw.push_back(range_1);
  ranges_raw.push_back(range_2);

  CHttpRanges ranges(ranges_raw);

  CHttpRange range_total;
  EXPECT_TRUE(ranges.GetTotalRange(range_total));
  EXPECT_EQ(range_total_expected, range_total);
}

TEST(TestHttpRanges, Add)
{
  CHttpRange range_0(0, 2);
  CHttpRange range_1(4, 6);
  CHttpRange range_2(8, 10);

  CHttpRanges ranges;
  CHttpRange range;

  ranges.Add(range_0);
  EXPECT_EQ(1, ranges.Size());
  EXPECT_TRUE(ranges.GetFirst(range));
  EXPECT_EQ(range_0, range);
  EXPECT_TRUE(ranges.GetLast(range));
  EXPECT_EQ(range_0, range);

  ranges.Add(range_1);
  EXPECT_EQ(2, ranges.Size());
  EXPECT_TRUE(ranges.GetFirst(range));
  EXPECT_EQ(range_0, range);
  EXPECT_TRUE(ranges.GetLast(range));
  EXPECT_EQ(range_1, range);

  ranges.Add(range_2);
  EXPECT_EQ(3, ranges.Size());
  EXPECT_TRUE(ranges.GetFirst(range));
  EXPECT_EQ(range_0, range);
  EXPECT_TRUE(ranges.GetLast(range));
  EXPECT_EQ(range_2, range);
}

TEST(TestHttpRanges, Remove)
{
  CHttpRange range_0(0, 2);
  CHttpRange range_1(4, 6);
  CHttpRange range_2(8, 10);

  HttpRanges ranges_raw;
  ranges_raw.push_back(range_0);
  ranges_raw.push_back(range_1);
  ranges_raw.push_back(range_2);

  CHttpRanges ranges(ranges_raw);

  CHttpRange range;
  EXPECT_EQ(3, ranges.Size());
  EXPECT_TRUE(ranges.Get(0, range));
  EXPECT_EQ(range_0, range);
  EXPECT_TRUE(ranges.Get(1, range));
  EXPECT_EQ(range_1, range);
  EXPECT_TRUE(ranges.Get(2, range));
  EXPECT_EQ(range_2, range);

  // remove non-existing range
  ranges.Remove(ranges.Size());
  EXPECT_EQ(3, ranges.Size());
  EXPECT_TRUE(ranges.Get(0, range));
  EXPECT_EQ(range_0, range);
  EXPECT_TRUE(ranges.Get(1, range));
  EXPECT_EQ(range_1, range);
  EXPECT_TRUE(ranges.Get(2, range));
  EXPECT_EQ(range_2, range);

  // remove first range
  ranges.Remove(0);
  EXPECT_EQ(2, ranges.Size());
  EXPECT_TRUE(ranges.Get(0, range));
  EXPECT_EQ(range_1, range);
  EXPECT_TRUE(ranges.Get(1, range));
  EXPECT_EQ(range_2, range);

  // remove last range
  ranges.Remove(1);
  EXPECT_EQ(1, ranges.Size());
  EXPECT_TRUE(ranges.Get(0, range));
  EXPECT_EQ(range_1, range);

  // remove remaining range
  ranges.Remove(0);
  EXPECT_EQ(0, ranges.Size());
}

TEST(TestHttpRanges, Clear)
{
  CHttpRange range_0(0, 2);
  CHttpRange range_1(4, 6);
  CHttpRange range_2(8, 10);

  HttpRanges ranges_raw;
  ranges_raw.push_back(range_0);
  ranges_raw.push_back(range_1);
  ranges_raw.push_back(range_2);

  CHttpRanges ranges(ranges_raw);

  CHttpRange range;
  EXPECT_EQ(3, ranges.Size());
  EXPECT_TRUE(ranges.Get(0, range));
  EXPECT_EQ(range_0, range);
  EXPECT_TRUE(ranges.Get(1, range));
  EXPECT_EQ(range_1, range);
  EXPECT_TRUE(ranges.Get(2, range));
  EXPECT_EQ(range_2, range);

  ranges.Clear();
  EXPECT_EQ(0, ranges.Size());
}

TEST(TestHttpRanges, ParseInvalid)
{
  CHttpRanges ranges;

  // combinations of invalid string and invalid total length
  EXPECT_FALSE(ranges.Parse(""));
  EXPECT_FALSE(ranges.Parse("", 0));
  EXPECT_FALSE(ranges.Parse("", 1));
  EXPECT_FALSE(ranges.Parse("test", 0));
  EXPECT_FALSE(ranges.Parse(RANGES_START, 0));

  // empty range definition
  EXPECT_FALSE(ranges.Parse(RANGES_START));
  EXPECT_FALSE(ranges.Parse(RANGES_START "-"));

  // bad characters in range definition
  EXPECT_FALSE(ranges.Parse(RANGES_START "a"));
  EXPECT_FALSE(ranges.Parse(RANGES_START "1a"));
  EXPECT_FALSE(ranges.Parse(RANGES_START "1-a"));
  EXPECT_FALSE(ranges.Parse(RANGES_START "a-a"));
  EXPECT_FALSE(ranges.Parse(RANGES_START "a-1"));
  EXPECT_FALSE(ranges.Parse(RANGES_START "--"));
  EXPECT_FALSE(ranges.Parse(RANGES_START "1--"));
  EXPECT_FALSE(ranges.Parse(RANGES_START "1--2"));
  EXPECT_FALSE(ranges.Parse(RANGES_START "--2"));

  // combination of valid and empty range definitions
  EXPECT_FALSE(ranges.Parse(RANGES_START "0-1,"));
  EXPECT_FALSE(ranges.Parse(RANGES_START ",0-1"));

  // too big start position
  EXPECT_FALSE(ranges.Parse(RANGES_START "10-11", 5));

  // end position smaller than start position
  EXPECT_FALSE(ranges.Parse(RANGES_START "1-0"));
}

TEST(TestHttpRanges, ParseStartOnly)
{
  const uint64_t totalLength = 5;
  const CHttpRange range0_(0, totalLength - 1);
  const CHttpRange range2_(2, totalLength - 1);

  CHttpRange range;

  CHttpRanges ranges_all;
  EXPECT_TRUE(ranges_all.Parse(RANGES_START "0-", totalLength));
  EXPECT_EQ(1, ranges_all.Size());
  EXPECT_TRUE(ranges_all.Get(0, range));
  EXPECT_EQ(range0_, range);

  CHttpRanges ranges_some;
  EXPECT_TRUE(ranges_some.Parse(RANGES_START "2-", totalLength));
  EXPECT_EQ(1, ranges_some.Size());
  EXPECT_TRUE(ranges_some.Get(0, range));
  EXPECT_EQ(range2_, range);
}

TEST(TestHttpRanges, ParseFromEnd)
{
  const uint64_t totalLength = 5;
  const CHttpRange range_1(totalLength - 1, totalLength - 1);
  const CHttpRange range_3(totalLength - 3, totalLength - 1);

  CHttpRange range;

  CHttpRanges ranges_1;
  EXPECT_TRUE(ranges_1.Parse(RANGES_START "-1", totalLength));
  EXPECT_EQ(1, ranges_1.Size());
  EXPECT_TRUE(ranges_1.Get(0, range));
  EXPECT_EQ(range_1, range);

  CHttpRanges ranges_3;
  EXPECT_TRUE(ranges_3.Parse(RANGES_START "-3", totalLength));
  EXPECT_EQ(1, ranges_3.Size());
  EXPECT_TRUE(ranges_3.Get(0, range));
  EXPECT_EQ(range_3, range);
}

TEST(TestHttpRanges, ParseSingle)
{
  const uint64_t totalLength = 5;
  const CHttpRange range0_0(0, 0);
  const CHttpRange range0_1(0, 1);
  const CHttpRange range0_5(0, totalLength - 1);
  const CHttpRange range1_1(1, 1);
  const CHttpRange range1_3(1, 3);
  const CHttpRange range3_4(3, 4);
  const CHttpRange range4_4(4, 4);

  CHttpRange range;

  CHttpRanges ranges;
  EXPECT_TRUE(ranges.Parse(RANGES_START "0-0", totalLength));
  EXPECT_EQ(1, ranges.Size());
  EXPECT_TRUE(ranges.Get(0, range));
  EXPECT_EQ(range0_0, range);

  EXPECT_TRUE(ranges.Parse(RANGES_START "0-1", totalLength));
  EXPECT_EQ(1, ranges.Size());
  EXPECT_TRUE(ranges.Get(0, range));
  EXPECT_EQ(range0_1, range);

  EXPECT_TRUE(ranges.Parse(RANGES_START "0-5", totalLength));
  EXPECT_EQ(1, ranges.Size());
  EXPECT_TRUE(ranges.Get(0, range));
  EXPECT_EQ(range0_5, range);

  EXPECT_TRUE(ranges.Parse(RANGES_START "1-1", totalLength));
  EXPECT_EQ(1, ranges.Size());
  EXPECT_TRUE(ranges.Get(0, range));
  EXPECT_EQ(range1_1, range);

  EXPECT_TRUE(ranges.Parse(RANGES_START "1-3", totalLength));
  EXPECT_EQ(1, ranges.Size());
  EXPECT_TRUE(ranges.Get(0, range));
  EXPECT_EQ(range1_3, range);

  EXPECT_TRUE(ranges.Parse(RANGES_START "3-4", totalLength));
  EXPECT_EQ(1, ranges.Size());
  EXPECT_TRUE(ranges.Get(0, range));
  EXPECT_EQ(range3_4, range);

  EXPECT_TRUE(ranges.Parse(RANGES_START "4-4", totalLength));
  EXPECT_EQ(1, ranges.Size());
  EXPECT_TRUE(ranges.Get(0, range));
  EXPECT_EQ(range4_4, range);
}

TEST(TestHttpRanges, ParseMulti)
{
  const uint64_t totalLength = 6;
  const CHttpRange range0_0(0, 0);
  const CHttpRange range0_1(0, 1);
  const CHttpRange range1_3(1, 3);
  const CHttpRange range2_2(2, 2);
  const CHttpRange range4_5(4, 5);
  const CHttpRange range5_5(5, 5);

  CHttpRange range;

  CHttpRanges ranges;
  EXPECT_TRUE(ranges.Parse(RANGES_START "0-0,2-2", totalLength));
  EXPECT_EQ(2, ranges.Size());
  EXPECT_TRUE(ranges.Get(0, range));
  EXPECT_EQ(range0_0, range);
  EXPECT_TRUE(ranges.Get(1, range));
  EXPECT_EQ(range2_2, range);

  EXPECT_TRUE(ranges.Parse(RANGES_START "0-0,2-2,4-5", totalLength));
  EXPECT_EQ(3, ranges.Size());
  EXPECT_TRUE(ranges.Get(0, range));
  EXPECT_EQ(range0_0, range);
  EXPECT_TRUE(ranges.Get(1, range));
  EXPECT_EQ(range2_2, range);
  EXPECT_TRUE(ranges.Get(2, range));
  EXPECT_EQ(range4_5, range);

  EXPECT_TRUE(ranges.Parse(RANGES_START "0-1,5-5", totalLength));
  EXPECT_EQ(2, ranges.Size());
  EXPECT_TRUE(ranges.Get(0, range));
  EXPECT_EQ(range0_1, range);
  EXPECT_TRUE(ranges.Get(1, range));
  EXPECT_EQ(range5_5, range);

  EXPECT_TRUE(ranges.Parse(RANGES_START "1-3,5-5", totalLength));
  EXPECT_EQ(2, ranges.Size());
  EXPECT_TRUE(ranges.Get(0, range));
  EXPECT_EQ(range1_3, range);
  EXPECT_TRUE(ranges.Get(1, range));
  EXPECT_EQ(range5_5, range);
}

TEST(TestHttpRanges, ParseOrderedNotOverlapping)
{
  const uint64_t totalLength = 5;
  const CHttpRange range0_0(0, 0);
  const CHttpRange range0_1(0, 1);
  const CHttpRange range2_2(2, 2);
  const CHttpRange range2_(2, totalLength - 1);
  const CHttpRange range_1(totalLength - 1, totalLength - 1);

  CHttpRange range;

  CHttpRanges ranges;
  EXPECT_TRUE(ranges.Parse(RANGES_START "0-0,-1", totalLength));
  EXPECT_EQ(2, ranges.Size());
  EXPECT_TRUE(ranges.Get(0, range));
  EXPECT_EQ(range0_0, range);
  EXPECT_TRUE(ranges.Get(1, range));
  EXPECT_EQ(range_1, range);

  EXPECT_TRUE(ranges.Parse(RANGES_START "0-0,2-2,-1", totalLength));
  EXPECT_EQ(3, ranges.Size());
  EXPECT_TRUE(ranges.Get(0, range));
  EXPECT_EQ(range0_0, range);
  EXPECT_TRUE(ranges.Get(1, range));
  EXPECT_EQ(range2_2, range);
  EXPECT_TRUE(ranges.Get(2, range));
  EXPECT_EQ(range_1, range);

  EXPECT_TRUE(ranges.Parse(RANGES_START "0-0,2-", totalLength));
  EXPECT_EQ(2, ranges.Size());
  EXPECT_TRUE(ranges.Get(0, range));
  EXPECT_EQ(range0_0, range);
  EXPECT_TRUE(ranges.Get(1, range));
  EXPECT_EQ(range2_, range);
}

TEST(TestHttpRanges, ParseOrderedBackToBack)
{
  const uint64_t totalLength = 5;
  const CHttpRange range0_1(0, 1);
  const CHttpRange range0_2(0, 2);
  const CHttpRange range1_2(1, 2);
  const CHttpRange range0_3(0, 3);
  const CHttpRange range4_4(4, 4);
  const CHttpRange range0_4(0, 4);
  const CHttpRange range3_4(3, 4);

  CHttpRange range;

  CHttpRanges ranges;
  EXPECT_TRUE(ranges.Parse(RANGES_START "0-0,1-1", totalLength));
  EXPECT_EQ(1, ranges.Size());
  EXPECT_TRUE(ranges.Get(0, range));
  EXPECT_EQ(range0_1, range);

  EXPECT_TRUE(ranges.Parse(RANGES_START "0-0,1-1,2-2", totalLength));
  EXPECT_EQ(1, ranges.Size());
  EXPECT_TRUE(ranges.Get(0, range));
  EXPECT_EQ(range0_2, range);

  EXPECT_TRUE(ranges.Parse(RANGES_START "0-0,1-1,2-2,3-3", totalLength));
  EXPECT_EQ(1, ranges.Size());
  EXPECT_TRUE(ranges.Get(0, range));
  EXPECT_EQ(range0_3, range);

  EXPECT_TRUE(ranges.Parse(RANGES_START "0-0,1-1,2-2,3-3,4-4", totalLength));
  EXPECT_EQ(1, ranges.Size());
  EXPECT_TRUE(ranges.Get(0, range));
  EXPECT_EQ(range0_4, range);

  EXPECT_TRUE(ranges.Parse(RANGES_START "0-0,1-1,3-3,4-4", totalLength));
  EXPECT_EQ(2, ranges.Size());
  EXPECT_TRUE(ranges.Get(0, range));
  EXPECT_EQ(range0_1, range);
  EXPECT_TRUE(ranges.Get(1, range));
  EXPECT_EQ(range3_4, range);

  EXPECT_TRUE(ranges.Parse(RANGES_START "1-1,2-2,4-4", totalLength));
  EXPECT_EQ(2, ranges.Size());
  EXPECT_TRUE(ranges.Get(0, range));
  EXPECT_EQ(range1_2, range);
  EXPECT_TRUE(ranges.Get(1, range));
  EXPECT_EQ(range4_4, range);
}

TEST(TestHttpRanges, ParseOrderedOverlapping)
{
  const uint64_t totalLength = 5;
  const CHttpRange range0_0(0, 0);
  const CHttpRange range0_1(0, 1);
  const CHttpRange range0_2(0, 2);
  const CHttpRange range0_3(0, 3);
  const CHttpRange range0_4(0, 4);
  const CHttpRange range2_4(2, 4);

  CHttpRange range;

  CHttpRanges ranges;
  EXPECT_TRUE(ranges.Parse(RANGES_START "0-0,0-1", totalLength));
  EXPECT_EQ(1, ranges.Size());
  EXPECT_TRUE(ranges.Get(0, range));
  EXPECT_EQ(range0_1, range);

  EXPECT_TRUE(ranges.Parse(RANGES_START "0-0,0-1,0-2", totalLength));
  EXPECT_EQ(1, ranges.Size());
  EXPECT_TRUE(ranges.Get(0, range));
  EXPECT_EQ(range0_2, range);

  EXPECT_TRUE(ranges.Parse(RANGES_START "0-0,0-1,1-2", totalLength));
  EXPECT_EQ(1, ranges.Size());
  EXPECT_TRUE(ranges.Get(0, range));
  EXPECT_EQ(range0_2, range);

  EXPECT_TRUE(ranges.Parse(RANGES_START "0-0,0-2,1-3", totalLength));
  EXPECT_EQ(1, ranges.Size());
  EXPECT_TRUE(ranges.Get(0, range));
  EXPECT_EQ(range0_3, range);

  EXPECT_TRUE(ranges.Parse(RANGES_START "0-1,1-2,2-3,3-4", totalLength));
  EXPECT_EQ(1, ranges.Size());
  EXPECT_TRUE(ranges.Get(0, range));
  EXPECT_EQ(range0_4, range);

  EXPECT_TRUE(ranges.Parse(RANGES_START "0-0,2-3,2-4,4-4", totalLength));
  EXPECT_EQ(2, ranges.Size());
  EXPECT_TRUE(ranges.Get(0, range));
  EXPECT_EQ(range0_0, range);
  EXPECT_TRUE(ranges.Get(1, range));
  EXPECT_EQ(range2_4, range);
}

TEST(TestHttpRanges, ParseUnorderedNotOverlapping)
{
  const uint64_t totalLength = 5;
  const CHttpRange range0_0(0, 0);
  const CHttpRange range0_1(0, 1);
  const CHttpRange range2_2(2, 2);
  const CHttpRange range2_(2, totalLength - 1);
  const CHttpRange range_1(totalLength - 1, totalLength - 1);

  CHttpRange range;

  CHttpRanges ranges;
  EXPECT_TRUE(ranges.Parse(RANGES_START "-1,0-0", totalLength));
  EXPECT_EQ(2, ranges.Size());
  EXPECT_TRUE(ranges.Get(0, range));
  EXPECT_EQ(range0_0, range);
  EXPECT_TRUE(ranges.Get(1, range));
  EXPECT_EQ(range_1, range);

  EXPECT_TRUE(ranges.Parse(RANGES_START "2-2,-1,0-0", totalLength));
  EXPECT_EQ(3, ranges.Size());
  EXPECT_TRUE(ranges.Get(0, range));
  EXPECT_EQ(range0_0, range);
  EXPECT_TRUE(ranges.Get(1, range));
  EXPECT_EQ(range2_2, range);
  EXPECT_TRUE(ranges.Get(2, range));
  EXPECT_EQ(range_1, range);

  EXPECT_TRUE(ranges.Parse(RANGES_START "2-,0-0", totalLength));
  EXPECT_EQ(2, ranges.Size());
  EXPECT_TRUE(ranges.Get(0, range));
  EXPECT_EQ(range0_0, range);
  EXPECT_TRUE(ranges.Get(1, range));
  EXPECT_EQ(range2_, range);
}

TEST(TestHttpRanges, ParseUnorderedBackToBack)
{
  const uint64_t totalLength = 5;
  const CHttpRange range0_0(0, 0);
  const CHttpRange range1_1(1, 1);
  const CHttpRange range0_1(0, 1);
  const CHttpRange range2_2(2, 2);
  const CHttpRange range0_2(0, 2);
  const CHttpRange range1_2(1, 2);
  const CHttpRange range3_3(3, 3);
  const CHttpRange range0_3(0, 3);
  const CHttpRange range4_4(4, 4);
  const CHttpRange range0_4(0, 4);
  const CHttpRange range3_4(3, 4);

  CHttpRange range;

  CHttpRanges ranges;
  EXPECT_TRUE(ranges.Parse(RANGES_START "1-1,0-0", totalLength));
  EXPECT_EQ(1, ranges.Size());
  EXPECT_TRUE(ranges.Get(0, range));
  EXPECT_EQ(range0_1, range);

  EXPECT_TRUE(ranges.Parse(RANGES_START "1-1,0-0,2-2", totalLength));
  EXPECT_EQ(1, ranges.Size());
  EXPECT_TRUE(ranges.Get(0, range));
  EXPECT_EQ(range0_2, range);

  EXPECT_TRUE(ranges.Parse(RANGES_START "2-2,1-1,3-3,0-0", totalLength));
  EXPECT_EQ(1, ranges.Size());
  EXPECT_TRUE(ranges.Get(0, range));
  EXPECT_EQ(range0_3, range);

  EXPECT_TRUE(ranges.Parse(RANGES_START "4-4,1-1,0-0,2-2,3-3", totalLength));
  EXPECT_EQ(1, ranges.Size());
  EXPECT_TRUE(ranges.Get(0, range));
  EXPECT_EQ(range0_4, range);

  EXPECT_TRUE(ranges.Parse(RANGES_START "3-3,0-0,4-4,1-1", totalLength));
  EXPECT_EQ(2, ranges.Size());
  EXPECT_TRUE(ranges.Get(0, range));
  EXPECT_EQ(range0_1, range);
  EXPECT_TRUE(ranges.Get(1, range));
  EXPECT_EQ(range3_4, range);

  EXPECT_TRUE(ranges.Parse(RANGES_START "4-4,1-1,2-2", totalLength));
  EXPECT_EQ(2, ranges.Size());
  EXPECT_TRUE(ranges.Get(0, range));
  EXPECT_EQ(range1_2, range);
  EXPECT_TRUE(ranges.Get(1, range));
  EXPECT_EQ(range4_4, range);
}

TEST(TestHttpRanges, ParseUnorderedOverlapping)
{
  const uint64_t totalLength = 5;
  const CHttpRange range0_0(0, 0);
  const CHttpRange range0_1(0, 1);
  const CHttpRange range0_2(0, 2);
  const CHttpRange range0_3(0, 3);
  const CHttpRange range0_4(0, 4);
  const CHttpRange range2_4(2, 4);

  CHttpRange range;

  CHttpRanges ranges;
  EXPECT_TRUE(ranges.Parse(RANGES_START "0-1,0-0", totalLength));
  EXPECT_EQ(1, ranges.Size());
  EXPECT_TRUE(ranges.Get(0, range));
  EXPECT_EQ(range0_1, range);

  EXPECT_TRUE(ranges.Parse(RANGES_START "0-2,0-0,0-1", totalLength));
  EXPECT_EQ(1, ranges.Size());
  EXPECT_TRUE(ranges.Get(0, range));
  EXPECT_EQ(range0_2, range);

  EXPECT_TRUE(ranges.Parse(RANGES_START "0-1,1-2,0-0", totalLength));
  EXPECT_EQ(1, ranges.Size());
  EXPECT_TRUE(ranges.Get(0, range));
  EXPECT_EQ(range0_2, range);

  EXPECT_TRUE(ranges.Parse(RANGES_START "0-2,0-0,1-3", totalLength));
  EXPECT_EQ(1, ranges.Size());
  EXPECT_TRUE(ranges.Get(0, range));
  EXPECT_EQ(range0_3, range);

  EXPECT_TRUE(ranges.Parse(RANGES_START "2-3,1-2,0-1,3-4", totalLength));
  EXPECT_EQ(1, ranges.Size());
  EXPECT_TRUE(ranges.Get(0, range));
  EXPECT_EQ(range0_4, range);

  EXPECT_TRUE(ranges.Parse(RANGES_START "4-4,0-0,2-4,2-3", totalLength));
  EXPECT_EQ(2, ranges.Size());
  EXPECT_TRUE(ranges.Get(0, range));
  EXPECT_EQ(range0_0, range);
  EXPECT_TRUE(ranges.Get(1, range));
  EXPECT_EQ(range2_4, range);
}
