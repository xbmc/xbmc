#include "gtest/gtest.h"
#include "Client/PlexServerVersion.h"


TEST(PlexServerVersion, parseBasic)
{
  CPlexServerVersion version("0.9.9.8.234-abc123");
  EXPECT_EQ(version.major, 0);
  EXPECT_EQ(version.minor, 9);
  EXPECT_EQ(version.micro, 9);
  EXPECT_EQ(version.patch, 8);
  EXPECT_EQ(version.build, 234);
  EXPECT_EQ(version.gitrev, "abc123");
}

TEST(PlexServerVersion, parseDevBuild)
{
  CPlexServerVersion version("0.9.9.8.dev-abc123");
  EXPECT_TRUE(version.isValid);
  EXPECT_TRUE(version.isDev);
  EXPECT_EQ(version.build, 0);
}

TEST(PlexServerVersion, parseBroken)
{
  CPlexServerVersion version("abc123");
  EXPECT_FALSE(version.isValid);
}

TEST(PlexServerVersion, shortStringNonDev)
{
  CPlexServerVersion version("0.9.9.8.123-abc123");
  EXPECT_STREQ(version.shortString(), "0.9.9.8.123");
}

TEST(PlexServerVersion, shortStringDev)
{
  CPlexServerVersion version("0.9.9.8.dev-abc123");
  EXPECT_STREQ(version.shortString(), "0.9.9.8");
}

TEST(PlexServerVersion, testEqual)
{
  CPlexServerVersion version("0.9.9.8.234-abc123");
  CPlexServerVersion version2("0.9.9.8.234-abc123");
  EXPECT_TRUE(version == version2);
}

TEST(PlexServerVersion, testNEqual)
{
  CPlexServerVersion version("0.9.9.8.235-abc123");
  CPlexServerVersion version2("0.9.9.8.234-abc123");
  EXPECT_FALSE(version == version2);
}

TEST(PlexServerVersion, testGT)
{
  CPlexServerVersion version("0.9.9.7.235-abc123");
  CPlexServerVersion version2("0.9.9.8.235-abc123");

  EXPECT_TRUE(version2 > version);
}

TEST(PlexServerVersion, testLT)
{
  CPlexServerVersion version("0.9.9.7.235-abc123");
  CPlexServerVersion version2("0.9.9.8.235-abc123");

  EXPECT_TRUE(version < version2);
}

TEST(PlexServerVersion, testLTsmallerBuild)
{
  CPlexServerVersion version1("0.9.9.7.435-abc123");
  CPlexServerVersion version2("0.9.9.8.34-624687d");
  EXPECT_TRUE(version1 < version2);
  EXPECT_FALSE(version2 < version1);
}

TEST(PlexServerVersion, testGTsmallerBuild)
{
  CPlexServerVersion version1("0.9.9.7.435-abc123");
  CPlexServerVersion version2("0.9.9.8.34-624687d");
  EXPECT_TRUE(version2 > version1);
  EXPECT_FALSE(version1 > version2);
}
