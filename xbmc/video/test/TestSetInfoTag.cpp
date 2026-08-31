/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "utils/XBMCTinyXML.h"
#include "utils/XMLUtils.h"
#include "video/SetInfoTag.h"

#include <string>

#include <gtest/gtest.h>

namespace
{
CSetInfoTag LoadFromXml(const std::string& document)
{
  CXBMCTinyXML doc;
  doc.Parse(document, TIXML_ENCODING_UNKNOWN);

  CSetInfoTag tag;
  EXPECT_TRUE(tag.Load(doc.RootElement()));
  return tag;
}

std::string SaveToXml(const CSetInfoTag& tag)
{
  CXBMCTinyXML doc;
  EXPECT_TRUE(tag.Save(&doc, "set"));

  return XMLUtils::NodeStringSerialization(doc.RootElement(),
                                           XMLUtils::SerializationFormat::COMPACT);
}
} // namespace

TEST(TestSetInfoTag, ReadSortTitle)
{
  const CSetInfoTag tag{LoadFromXml(
      R"(<?xml version="1.0" encoding="UTF-8" standalone="yes" ?>
         <set>
         <title>The Lord of the Rings Collection</title>
         <originaltitle>The Lord of the Rings Collection</originaltitle>
         <sorttitle>Lord of the Rings, The</sorttitle>
         <overview>Frodo goes for a walk.</overview>
         </set>)")};

  EXPECT_TRUE(tag.HasSortTitle());
  EXPECT_EQ(tag.GetSortTitle(), "Lord of the Rings, The");
  // the other fields must be unaffected
  EXPECT_EQ(tag.GetTitle(), "The Lord of the Rings Collection");
  EXPECT_EQ(tag.GetOriginalTitle(), "The Lord of the Rings Collection");
  EXPECT_EQ(tag.GetOverview(), "Frodo goes for a walk.");
}

TEST(TestSetInfoTag, ReadSortTitleIsTrimmed)
{
  const CSetInfoTag tag{
      LoadFromXml(R"(<set><title>A Set</title><sorttitle>   Set, A   </sorttitle></set>)")};

  EXPECT_EQ(tag.GetSortTitle(), "Set, A");
}

TEST(TestSetInfoTag, ReadWithoutSortTitle)
{
  const CSetInfoTag tag{LoadFromXml(R"(<set><title>A Set</title></set>)")};

  EXPECT_FALSE(tag.HasSortTitle());
  EXPECT_TRUE(tag.GetSortTitle().empty());
}

TEST(TestSetInfoTag, ReadEmptySortTitle)
{
  const CSetInfoTag tag{LoadFromXml(R"(<set><title>A Set</title><sorttitle></sorttitle></set>)")};

  EXPECT_FALSE(tag.HasSortTitle());
}

TEST(TestSetInfoTag, SaveSortTitle)
{
  CSetInfoTag tag;
  tag.SetTitle("The Lord of the Rings Collection");
  tag.SetSortTitle("Lord of the Rings, The");

  EXPECT_EQ(SaveToXml(tag), "<set>"
                            "<title>The Lord of the Rings Collection</title>"
                            "<sorttitle>Lord of the Rings, The</sorttitle>"
                            "</set>");
}

TEST(TestSetInfoTag, SaveOmitsEmptySortTitle)
{
  CSetInfoTag tag;
  tag.SetTitle("A Set");

  EXPECT_EQ(SaveToXml(tag), "<set><title>A Set</title></set>");
}

TEST(TestSetInfoTag, SortTitleRoundTrip)
{
  const std::string document{
      R"(<set><title>A Set</title><originaltitle>A Set</originaltitle>)"
      R"(<sorttitle>Set, A</sorttitle><overview>An overview.</overview></set>)"};

  EXPECT_EQ(SaveToXml(LoadFromXml(document)), document);
}

TEST(TestSetInfoTag, ResetClearsSortTitle)
{
  CSetInfoTag tag;
  tag.SetTitle("A Set");
  tag.SetOriginalTitle("A Set");
  tag.SetSortTitle("Set, A");

  tag.Reset();

  EXPECT_FALSE(tag.HasSortTitle());
  EXPECT_FALSE(tag.HasOriginalTitle());
  EXPECT_FALSE(tag.HasTitle());
}

TEST(TestSetInfoTag, LoadWithoutAppendResetsSortTitle)
{
  CSetInfoTag tag;
  tag.SetSortTitle("Set, A");

  CXBMCTinyXML doc;
  doc.Parse(R"(<set><title>Another Set</title></set>)", TIXML_ENCODING_UNKNOWN);
  EXPECT_TRUE(tag.Load(doc.RootElement()));

  EXPECT_FALSE(tag.HasSortTitle());
}

TEST(TestSetInfoTag, SortTitleAloneDoesNotMakeTagNonEmpty)
{
  CSetInfoTag tag;
  tag.SetSortTitle("Set, A");

  EXPECT_TRUE(tag.IsEmpty());
}

TEST(TestSetInfoTag, MergeSortTitle)
{
  CSetInfoTag tag;
  tag.SetTitle("A Set");
  tag.SetSortTitle("Set, A");

  CSetInfoTag other;
  other.SetTitle("A Set");
  other.SetSortTitle("Set, The");
  tag.Merge(other);

  EXPECT_EQ(tag.GetSortTitle(), "Set, The");

  // an empty sort title must not overwrite an existing one
  CSetInfoTag empty;
  empty.SetTitle("A Set");
  tag.Merge(empty);

  EXPECT_EQ(tag.GetSortTitle(), "Set, The");
}

TEST(TestSetInfoTag, CopySortTitle)
{
  CSetInfoTag other;
  other.SetTitle("A Set");
  other.SetSortTitle("Set, A");

  CSetInfoTag tag;
  tag.SetSortTitle("Something Else");
  tag.Copy(other);

  EXPECT_EQ(tag.GetSortTitle(), "Set, A");

  // unlike Merge, Copy must propagate an empty sort title
  tag.Copy({});

  EXPECT_FALSE(tag.HasSortTitle());
}
