/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "pvr/channels/PVRChannelsPath.h"

#include <gtest/gtest.h>

TEST(TestPVRChannelsPath, Parse_Protocol)
{
  // pvr protocol is generally fine, but not sufficient for channels pvr paths - component is missing for that.
  PVR::CPVRChannelsPath path("pvr://");

  EXPECT_FALSE(path.IsValid());
}

TEST(TestPVRChannelsPath, Parse_Component_1)
{
  PVR::CPVRChannelsPath path("pvr://channels");

  EXPECT_EQ(static_cast<std::string>(path), "pvr://channels/");
  EXPECT_TRUE(path.IsValid());
  EXPECT_TRUE(path.IsEmpty());
}

TEST(TestPVRChannelsPath, Parse_Component_2)
{
  PVR::CPVRChannelsPath path("pvr://channels/");

  EXPECT_EQ(static_cast<std::string>(path), "pvr://channels/");
  EXPECT_TRUE(path.IsValid());
  EXPECT_TRUE(path.IsEmpty());
}

TEST(TestPVRChannelsPath, Parse_Invalid_Component)
{
  PVR::CPVRChannelsPath path("pvr://foo/");

  EXPECT_FALSE(path.IsValid());
}

TEST(TestPVRChannelsPath, Parse_TV_Root_1)
{
  PVR::CPVRChannelsPath path("pvr://channels/tv");

  EXPECT_EQ(static_cast<std::string>(path), "pvr://channels/tv/");
  EXPECT_TRUE(path.IsValid());
  EXPECT_FALSE(path.IsEmpty());
  EXPECT_FALSE(path.IsRadio());
  EXPECT_TRUE(path.IsChannelsRoot());
  EXPECT_FALSE(path.IsChannelGroup());
  EXPECT_FALSE(path.IsHiddenChannelGroup());
  EXPECT_FALSE(path.IsChannel());
  EXPECT_EQ(path.GetGroupName(), "");
  EXPECT_EQ(path.GetAddonID(), "");
  EXPECT_EQ(path.GetChannelUID(), -1);
}

TEST(TestPVRChannelsPath, Parse_TV_Root_2)
{
  PVR::CPVRChannelsPath path("pvr://channels/tv/");

  EXPECT_EQ(static_cast<std::string>(path), "pvr://channels/tv/");
  EXPECT_TRUE(path.IsValid());
  EXPECT_FALSE(path.IsEmpty());
  EXPECT_FALSE(path.IsRadio());
  EXPECT_TRUE(path.IsChannelsRoot());
  EXPECT_FALSE(path.IsChannelGroup());
  EXPECT_FALSE(path.IsHiddenChannelGroup());
  EXPECT_FALSE(path.IsChannel());
  EXPECT_EQ(path.GetGroupName(), "");
  EXPECT_EQ(path.GetAddonID(), "");
  EXPECT_EQ(path.GetChannelUID(), -1);
}

TEST(TestPVRChannelsPath, Parse_Radio_Root_1)
{
  PVR::CPVRChannelsPath path("pvr://channels/radio");

  EXPECT_EQ(static_cast<std::string>(path), "pvr://channels/radio/");
  EXPECT_TRUE(path.IsValid());
  EXPECT_FALSE(path.IsEmpty());
  EXPECT_TRUE(path.IsRadio());
  EXPECT_TRUE(path.IsChannelsRoot());
  EXPECT_FALSE(path.IsChannelGroup());
  EXPECT_FALSE(path.IsHiddenChannelGroup());
  EXPECT_FALSE(path.IsChannel());
  EXPECT_EQ(path.GetGroupName(), "");
  EXPECT_EQ(path.GetAddonID(), "");
  EXPECT_EQ(path.GetChannelUID(), -1);
}

TEST(TestPVRChannelsPath, Parse_Radio_Root_2)
{
  PVR::CPVRChannelsPath path("pvr://channels/radio/");

  EXPECT_EQ(static_cast<std::string>(path), "pvr://channels/radio/");
  EXPECT_TRUE(path.IsValid());
  EXPECT_FALSE(path.IsEmpty());
  EXPECT_TRUE(path.IsRadio());
  EXPECT_TRUE(path.IsChannelsRoot());
  EXPECT_FALSE(path.IsChannelGroup());
  EXPECT_FALSE(path.IsHiddenChannelGroup());
  EXPECT_FALSE(path.IsChannel());
  EXPECT_EQ(path.GetGroupName(), "");
  EXPECT_EQ(path.GetAddonID(), "");
  EXPECT_EQ(path.GetChannelUID(), -1);
}

TEST(TestPVRChannelsPath, Parse_Invalid_Root)
{
  PVR::CPVRChannelsPath path("pvr://channels/foo");

  EXPECT_FALSE(path.IsValid());
}

TEST(TestPVRChannelsPath, Parse_TV_Group_1)
{
  PVR::CPVRChannelsPath path("pvr://channels/tv/Group1@11");

  EXPECT_EQ(static_cast<std::string>(path), "pvr://channels/tv/Group1@11/");
  EXPECT_TRUE(path.IsValid());
  EXPECT_FALSE(path.IsEmpty());
  EXPECT_FALSE(path.IsRadio());
  EXPECT_FALSE(path.IsChannelsRoot());
  EXPECT_TRUE(path.IsChannelGroup());
  EXPECT_FALSE(path.IsHiddenChannelGroup());
  EXPECT_FALSE(path.IsChannel());
  EXPECT_EQ(path.GetGroupName(), "Group1");
  EXPECT_EQ(path.GetGroupClientID(), 11);
  EXPECT_EQ(path.GetAddonID(), "");
  EXPECT_EQ(path.GetChannelUID(), -1);
}

TEST(TestPVRChannelsPath, Parse_TV_Group_2)
{
  PVR::CPVRChannelsPath path("pvr://channels/tv/Group1@11/");

  EXPECT_EQ(static_cast<std::string>(path), "pvr://channels/tv/Group1@11/");
  EXPECT_TRUE(path.IsValid());
  EXPECT_FALSE(path.IsEmpty());
  EXPECT_FALSE(path.IsRadio());
  EXPECT_FALSE(path.IsChannelsRoot());
  EXPECT_TRUE(path.IsChannelGroup());
  EXPECT_FALSE(path.IsHiddenChannelGroup());
  EXPECT_FALSE(path.IsChannel());
  EXPECT_EQ(path.GetGroupName(), "Group1");
  EXPECT_EQ(path.GetGroupClientID(), 11);
  EXPECT_EQ(path.GetAddonID(), "");
  EXPECT_EQ(path.GetChannelUID(), -1);
}

TEST(TestPVRChannelsPath, Parse_Hidden_TV_Group)
{
  PVR::CPVRChannelsPath path("pvr://channels/tv/.hidden@11");

  EXPECT_EQ(static_cast<std::string>(path), "pvr://channels/tv/.hidden@11/");
  EXPECT_TRUE(path.IsValid());
  EXPECT_FALSE(path.IsEmpty());
  EXPECT_FALSE(path.IsRadio());
  EXPECT_FALSE(path.IsChannelsRoot());
  EXPECT_TRUE(path.IsChannelGroup());
  EXPECT_TRUE(path.IsHiddenChannelGroup());
  EXPECT_FALSE(path.IsChannel());
  EXPECT_EQ(path.GetGroupName(), ".hidden");
  EXPECT_EQ(path.GetGroupClientID(), 11);
  EXPECT_EQ(path.GetAddonID(), "");
  EXPECT_EQ(path.GetChannelUID(), -1);
}

TEST(TestPVRChannelsPath, Parse_Special_TV_Group)
{
  PVR::CPVRChannelsPath path("pvr://channels/tv/foo%2Fbar%20baz@11");

  EXPECT_EQ(static_cast<std::string>(path), "pvr://channels/tv/foo%2Fbar%20baz@11/");
  EXPECT_TRUE(path.IsValid());
  EXPECT_FALSE(path.IsEmpty());
  EXPECT_FALSE(path.IsRadio());
  EXPECT_FALSE(path.IsChannelsRoot());
  EXPECT_TRUE(path.IsChannelGroup());
  EXPECT_FALSE(path.IsHiddenChannelGroup());
  EXPECT_FALSE(path.IsChannel());
  EXPECT_EQ(path.GetGroupName(), "foo/bar baz");
  EXPECT_EQ(path.GetGroupClientID(), 11);
  EXPECT_EQ(path.GetAddonID(), "");
  EXPECT_EQ(path.GetChannelUID(), -1);
}

TEST(TestPVRChannelsPath, Parse_Special_TV_Group1)
{
  PVR::CPVRChannelsPath path("pvr://channels/tv/foo%40bar%20baz@11");

  EXPECT_EQ(static_cast<std::string>(path), "pvr://channels/tv/foo%40bar%20baz@11/");
  EXPECT_TRUE(path.IsValid());
  EXPECT_FALSE(path.IsEmpty());
  EXPECT_FALSE(path.IsRadio());
  EXPECT_FALSE(path.IsChannelsRoot());
  EXPECT_TRUE(path.IsChannelGroup());
  EXPECT_FALSE(path.IsHiddenChannelGroup());
  EXPECT_FALSE(path.IsChannel());
  EXPECT_EQ(path.GetGroupName(), "foo@bar baz");
  EXPECT_EQ(path.GetGroupClientID(), 11);
  EXPECT_EQ(path.GetAddonID(), "");
  EXPECT_EQ(path.GetChannelUID(), -1);
}

TEST(TestPVRChannelsPath, Parse_Invalid_Special_TV_Group)
{
  // special chars in group name not escaped
  PVR::CPVRChannelsPath path("pvr://channels/tv/foo/bar baz");

  EXPECT_FALSE(path.IsValid());
}

TEST(TestPVRChannelsPath, Parse_Radio_Group)
{
  PVR::CPVRChannelsPath path("pvr://channels/radio/Group1@11");

  EXPECT_EQ(static_cast<std::string>(path), "pvr://channels/radio/Group1@11/");
  EXPECT_TRUE(path.IsValid());
  EXPECT_FALSE(path.IsEmpty());
  EXPECT_TRUE(path.IsRadio());
  EXPECT_FALSE(path.IsChannelsRoot());
  EXPECT_TRUE(path.IsChannelGroup());
  EXPECT_FALSE(path.IsHiddenChannelGroup());
  EXPECT_FALSE(path.IsChannel());
  EXPECT_EQ(path.GetGroupName(), "Group1");
  EXPECT_EQ(path.GetGroupClientID(), 11);
  EXPECT_EQ(path.GetAddonID(), "");
  EXPECT_EQ(path.GetChannelUID(), -1);
}

TEST(TestPVRChannelsPath, Parse_TV_Channel)
{
  PVR::CPVRChannelsPath path("pvr://channels/tv/Group1@11/5@pvr.demo_4711.pvr");

  EXPECT_EQ(static_cast<std::string>(path), "pvr://channels/tv/Group1@11/5@pvr.demo_4711.pvr");
  EXPECT_TRUE(path.IsValid());
  EXPECT_FALSE(path.IsEmpty());
  EXPECT_FALSE(path.IsRadio());
  EXPECT_FALSE(path.IsChannelsRoot());
  EXPECT_FALSE(path.IsChannelGroup());
  EXPECT_FALSE(path.IsHiddenChannelGroup());
  EXPECT_TRUE(path.IsChannel());
  EXPECT_EQ(path.GetGroupName(), "Group1");
  EXPECT_EQ(path.GetGroupClientID(), 11);
  EXPECT_EQ(path.GetAddonID(), "pvr.demo");
  EXPECT_EQ(path.GetInstanceID(), 5);
  EXPECT_EQ(path.GetChannelUID(), 4711);
}

TEST(TestPVRChannelsPath, Parse_Invalid_TV_Channel_1)
{
  // trailing ".pvr" missing
  PVR::CPVRChannelsPath path("pvr://channels/radio/Group1@11/1@pvr.demo_4711");

  EXPECT_FALSE(path.IsValid());
}

TEST(TestPVRChannelsPath, Parse_Invalid_TV_Channel_2)
{
  // '-' instead of '_' as clientid / channeluid delimiter
  PVR::CPVRChannelsPath path("pvr://channels/radio/Group1@11/1@pvr.demo-4711.pvr");

  EXPECT_FALSE(path.IsValid());
}

TEST(TestPVRChannelsPath, Parse_Invalid_TV_Channel_3)
{
  // channeluid not numerical
  PVR::CPVRChannelsPath path("pvr://channels/radio/Group1@11/1@pvr.demo_abc4711.pvr");

  EXPECT_FALSE(path.IsValid());
}

TEST(TestPVRChannelsPath, Parse_Invalid_TV_Channel_4)
{
  // channeluid not positive or zero
  PVR::CPVRChannelsPath path("pvr://channels/radio/Group1@11/1@pvr.demo_-4711.pvr");

  EXPECT_FALSE(path.IsValid());
}

TEST(TestPVRChannelsPath, Parse_Invalid_TV_Channel_5)
{
  // empty clientid
  PVR::CPVRChannelsPath path("pvr://channels/radio/Group1@11/1@_4711.pvr");

  EXPECT_FALSE(path.IsValid());
}

TEST(TestPVRChannelsPath, Parse_Invalid_TV_Channel_6)
{
  // empty channeluid
  PVR::CPVRChannelsPath path("pvr://channels/radio/Group1@11/1@pvr.demo_.pvr");

  EXPECT_FALSE(path.IsValid());
}

TEST(TestPVRChannelsPath, Parse_Invalid_TV_Channel_7)
{
  // empty group client id, empty channel clientid and empty channeluid
  PVR::CPVRChannelsPath path("pvr://channels/radio/Group1/1@_.pvr");

  EXPECT_FALSE(path.IsValid());
}

TEST(TestPVRChannelsPath, Parse_Invalid_TV_Channel_8)
{
  // empty group client id, empty channel clientid and empty channeluid, only extension ".pvr" given
  PVR::CPVRChannelsPath path("pvr://channels/radio/Group1/.pvr");

  EXPECT_FALSE(path.IsValid());
}

TEST(TestPVRChannelsPath, Parse_Invalid_TV_Channel_9)
{
  // empty group client id
  PVR::CPVRChannelsPath path("pvr://channels/radio/Group1/0@pvr.demo_4711.pvr");

  EXPECT_FALSE(path.IsValid());
}

TEST(TestPVRChannelsPath, TV_Channelgroup)
{
  PVR::CPVRChannelsPath path(false, "Group1", 11);

  EXPECT_EQ(static_cast<std::string>(path), "pvr://channels/tv/Group1@11/");
  EXPECT_TRUE(path.IsValid());
  EXPECT_FALSE(path.IsEmpty());
  EXPECT_FALSE(path.IsRadio());
  EXPECT_FALSE(path.IsChannelsRoot());
  EXPECT_TRUE(path.IsChannelGroup());
  EXPECT_FALSE(path.IsHiddenChannelGroup());
  EXPECT_FALSE(path.IsChannel());
  EXPECT_EQ(path.GetGroupName(), "Group1");
  EXPECT_EQ(path.GetGroupClientID(), 11);
  EXPECT_EQ(path.GetAddonID(), "");
  EXPECT_EQ(path.GetChannelUID(), -1);
}

TEST(TestPVRChannelsPath, Radio_Channelgroup)
{
  PVR::CPVRChannelsPath path(true, "Group1", 11);

  EXPECT_EQ(static_cast<std::string>(path), "pvr://channels/radio/Group1@11/");
  EXPECT_TRUE(path.IsValid());
  EXPECT_FALSE(path.IsEmpty());
  EXPECT_TRUE(path.IsRadio());
  EXPECT_FALSE(path.IsChannelsRoot());
  EXPECT_TRUE(path.IsChannelGroup());
  EXPECT_FALSE(path.IsHiddenChannelGroup());
  EXPECT_FALSE(path.IsChannel());
  EXPECT_EQ(path.GetGroupName(), "Group1");
  EXPECT_EQ(path.GetGroupClientID(), 11);
  EXPECT_EQ(path.GetAddonID(), "");
  EXPECT_EQ(path.GetChannelUID(), -1);
}

TEST(TestPVRChannelsPath, Hidden_TV_Channelgroup)
{
  PVR::CPVRChannelsPath path(false, true, "Group1", 11);

  EXPECT_EQ(static_cast<std::string>(path), "pvr://channels/tv/.hidden@11/");
  EXPECT_TRUE(path.IsValid());
  EXPECT_FALSE(path.IsEmpty());
  EXPECT_FALSE(path.IsRadio());
  EXPECT_FALSE(path.IsChannelsRoot());
  EXPECT_TRUE(path.IsChannelGroup());
  EXPECT_TRUE(path.IsHiddenChannelGroup());
  EXPECT_FALSE(path.IsChannel());
  EXPECT_EQ(path.GetGroupName(), ".hidden");
  EXPECT_EQ(path.GetGroupClientID(), 11);
  EXPECT_EQ(path.GetAddonID(), "");
  EXPECT_EQ(path.GetChannelUID(), -1);
}

TEST(TestPVRChannelsPath, Hidden_Radio_Channelgroup)
{
  PVR::CPVRChannelsPath path(true, true, "Group1", 11);

  EXPECT_EQ(static_cast<std::string>(path), "pvr://channels/radio/.hidden@11/");
  EXPECT_TRUE(path.IsValid());
  EXPECT_FALSE(path.IsEmpty());
  EXPECT_TRUE(path.IsRadio());
  EXPECT_FALSE(path.IsChannelsRoot());
  EXPECT_TRUE(path.IsChannelGroup());
  EXPECT_TRUE(path.IsHiddenChannelGroup());
  EXPECT_FALSE(path.IsChannel());
  EXPECT_EQ(path.GetGroupName(), ".hidden");
  EXPECT_EQ(path.GetGroupClientID(), 11);
  EXPECT_EQ(path.GetAddonID(), "");
  EXPECT_EQ(path.GetChannelUID(), -1);
}

TEST(TestPVRChannelsPath, TV_Channel)
{
  PVR::CPVRChannelsPath path(false, "Group1", 11, "pvr.demo", ADDON::ADDON_SINGLETON_INSTANCE_ID,
                             4711);

  EXPECT_EQ(static_cast<std::string>(path), "pvr://channels/tv/Group1@11/0@pvr.demo_4711.pvr");
  EXPECT_TRUE(path.IsValid());
  EXPECT_FALSE(path.IsEmpty());
  EXPECT_FALSE(path.IsRadio());
  EXPECT_FALSE(path.IsChannelsRoot());
  EXPECT_FALSE(path.IsChannelGroup());
  EXPECT_FALSE(path.IsHiddenChannelGroup());
  EXPECT_TRUE(path.IsChannel());
  EXPECT_EQ(path.GetGroupName(), "Group1");
  EXPECT_EQ(path.GetGroupClientID(), 11);
  EXPECT_EQ(path.GetAddonID(), "pvr.demo");
  EXPECT_EQ(path.GetChannelUID(), 4711);
}

TEST(TestPVRChannelsPath, Radio_Channel)
{
  PVR::CPVRChannelsPath path(true, "Group1", 11, "pvr.demo", ADDON::ADDON_SINGLETON_INSTANCE_ID,
                             4711);

  EXPECT_EQ(static_cast<std::string>(path), "pvr://channels/radio/Group1@11/0@pvr.demo_4711.pvr");
  EXPECT_TRUE(path.IsValid());
  EXPECT_FALSE(path.IsEmpty());
  EXPECT_TRUE(path.IsRadio());
  EXPECT_FALSE(path.IsChannelsRoot());
  EXPECT_FALSE(path.IsChannelGroup());
  EXPECT_FALSE(path.IsHiddenChannelGroup());
  EXPECT_TRUE(path.IsChannel());
  EXPECT_EQ(path.GetGroupName(), "Group1");
  EXPECT_EQ(path.GetGroupClientID(), 11);
  EXPECT_EQ(path.GetAddonID(), "pvr.demo");
  EXPECT_EQ(path.GetChannelUID(), 4711);
}

TEST(TestPVRChannelsPath, Operator_Equals)
{
  PVR::CPVRChannelsPath path2(true, "Group1", 11);
  PVR::CPVRChannelsPath path(static_cast<std::string>(path2));

  EXPECT_EQ(path, path2);
}
