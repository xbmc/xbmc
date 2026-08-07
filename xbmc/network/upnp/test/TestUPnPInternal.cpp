/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "FileItem.h"
#include "XBDateTime.h"
#include "network/upnp/UPnPInternal.h"

#include <memory>
#include <string>

#include <Platinum/Source/Devices/MediaServer/PltDidl.h>
#include <Platinum/Source/Devices/MediaServer/PltMediaItem.h>
#include <gtest/gtest.h>

using namespace UPNP;

namespace
{

constexpr const char* DIDL_HEADER =
    "<DIDL-Lite xmlns=\"urn:schemas-upnp-org:metadata-1-0/DIDL-Lite/\" "
    "xmlns:dc=\"http://purl.org/dc/elements/1.1/\" "
    "xmlns:upnp=\"urn:schemas-upnp-org:metadata-1-0/upnp/\">";

std::string BuildDidl(const std::string& id, const std::string& title, const std::string& date)
{
  std::string didl = DIDL_HEADER;
  didl += "<item id=\"" + id + "\" parentID=\"0\" restricted=\"1\">";
  didl += "<dc:title>" + title + "</dc:title>";
  didl += "<upnp:class>object.item.videoItem</upnp:class>";
  if (!date.empty())
    didl += "<dc:date>" + date + "</dc:date>";
  didl += "</item></DIDL-Lite>";
  return didl;
}

//! \brief Parse a DIDL-Lite document and return its first object, as CUPnPDirectory does.
NPT_Reference<PLT_MediaObject> ParseFirstObject(const std::string& didl)
{
  PLT_MediaObjectListReference objects;
  if (NPT_FAILED(PLT_Didl::FromDidl(didl.c_str(), objects)) || objects.IsNull())
    return NPT_Reference<PLT_MediaObject>();

  NPT_List<PLT_MediaObject*>::Iterator entry = objects->GetFirstItem();
  if (!entry)
    return NPT_Reference<PLT_MediaObject>();

  // detach the object from the list so it outlives it
  PLT_MediaObject* object = *entry;
  objects->Remove(object, true);
  return NPT_Reference<PLT_MediaObject>(object);
}

//! \brief Set up a minimal non-container media object, avoiding any info tag population.
void InitItem(PLT_MediaItem& item, const char* date)
{
  item.m_ObjectClass.type = "object.item";
  item.m_ObjectID = "1";
  item.m_ParentID = "0";
  item.m_Title = "test item";
  item.m_Date = date;
}

} // namespace

TEST(TestUPnPInternal, BuildObjectSetsDateFromDateOnlyValue)
{
  PLT_MediaItem entry;
  InitItem(entry, "2021-03-04");

  const std::shared_ptr<CFileItem> item = BuildObject(&entry);

  ASSERT_NE(nullptr, item);
  const CDateTime& date = item->GetDateTime();
  ASSERT_TRUE(date.IsValid());
  EXPECT_EQ(2021, date.GetYear());
  EXPECT_EQ(3, date.GetMonth());
  EXPECT_EQ(4, date.GetDay());
  EXPECT_EQ(0, date.GetHour());
  EXPECT_EQ(0, date.GetMinute());
  EXPECT_EQ(0, date.GetSecond());
}

TEST(TestUPnPInternal, BuildObjectSetsDateFromDateTimeValue)
{
  PLT_MediaItem entry;
  InitItem(entry, "2021-03-04T05:06:07");

  const std::shared_ptr<CFileItem> item = BuildObject(&entry);

  ASSERT_NE(nullptr, item);
  const CDateTime& date = item->GetDateTime();
  ASSERT_TRUE(date.IsValid());
  EXPECT_EQ(2021, date.GetYear());
  EXPECT_EQ(3, date.GetMonth());
  EXPECT_EQ(4, date.GetDay());
  EXPECT_EQ(5, date.GetHour());
  EXPECT_EQ(6, date.GetMinute());
  EXPECT_EQ(7, date.GetSecond());
}

TEST(TestUPnPInternal, BuildObjectLeavesDateUnsetWhenNoDatePresent)
{
  PLT_MediaItem entry;
  InitItem(entry, "");

  const std::shared_ptr<CFileItem> item = BuildObject(&entry);

  ASSERT_NE(nullptr, item);
  EXPECT_FALSE(item->GetDateTime().IsValid());
}

TEST(TestUPnPInternal, BuildObjectLeavesDateUnsetOnUnparsableValue)
{
  PLT_MediaItem entry;
  InitItem(entry, "not a date");

  const std::shared_ptr<CFileItem> item = BuildObject(&entry);

  ASSERT_NE(nullptr, item);
  EXPECT_FALSE(item->GetDateTime().IsValid());
}

//! \brief PLT_Description::date is never filled by Platinum, so it must not be the source of truth.
TEST(TestUPnPInternal, DidlParsingFillsMediaObjectDateAndNotDescriptionDate)
{
  const NPT_Reference<PLT_MediaObject> entry =
      ParseFirstObject(BuildDidl("1", "test item", "2021-03-04"));

  ASSERT_FALSE(entry.IsNull());
  EXPECT_STREQ("2021-03-04T00:00:00Z", entry->m_Date.GetChars());
  EXPECT_EQ(0u, entry->m_Description.date.GetLength());
}

TEST(TestUPnPInternal, BuildObjectSetsDateForDidlSourcedItem)
{
  const NPT_Reference<PLT_MediaObject> entry =
      ParseFirstObject(BuildDidl("1", "test item", "2021-03-04"));
  ASSERT_FALSE(entry.IsNull());

  const std::shared_ptr<CFileItem> item = BuildObject(entry.AsPointer());

  ASSERT_NE(nullptr, item);
  // the DIDL value is UTC, so only compare against the same conversion
  EXPECT_EQ(CDateTime::FromW3CDateTime("2021-03-04T00:00:00Z"), item->GetDateTime());
}

//! \brief The point of the fix: dates from a server are comparable, so SortBy::DATE works.
TEST(TestUPnPInternal, BuildObjectDatesFromDidlAreOrdered)
{
  const NPT_Reference<PLT_MediaObject> older =
      ParseFirstObject(BuildDidl("1", "older", "2019-01-02"));
  const NPT_Reference<PLT_MediaObject> newer =
      ParseFirstObject(BuildDidl("2", "newer", "2021-03-04T05:06:07Z"));
  ASSERT_FALSE(older.IsNull());
  ASSERT_FALSE(newer.IsNull());

  const std::shared_ptr<CFileItem> olderItem = BuildObject(older.AsPointer());
  const std::shared_ptr<CFileItem> newerItem = BuildObject(newer.AsPointer());

  ASSERT_NE(nullptr, olderItem);
  ASSERT_NE(nullptr, newerItem);
  ASSERT_TRUE(olderItem->GetDateTime().IsValid());
  ASSERT_TRUE(newerItem->GetDateTime().IsValid());
  EXPECT_LT(olderItem->GetDateTime(), newerItem->GetDateTime());
}
