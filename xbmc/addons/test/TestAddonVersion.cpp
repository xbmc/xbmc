/*
 *      Copyright (C) 2005-2014 Team XBMC
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

#include "addons/AddonVersion.h"

#include "gtest/gtest.h"

using namespace ADDON;

class TestAddonVersion : public testing::Test
{
public:
  TestAddonVersion()
  : v1_0("1.0"),
    v1_00("1.00"),
    v1_0_0("1.0.0"),
    v1_1("1.1"),
    v1_01("1.01"),
    v1_0_1("1.0.1"),
    e1_v1_0_0("1:1.0.0"),
    e1_v1_0_1("1:1.0.1"),
    e2_v1_0_0("2:1.0.0"),
    e1_v1_0_0_r1("1:1.0.0-1"),
    e1_v1_0_1_r1("1:1.0.1-1"),
    e1_v1_0_0_r2("1:1.0.0-2"),
    v1_0_0_beta("1.0.0~beta"),
    v1_0_0_alpha("1.0.0~alpha"),
    v1_0_0_alpha2("1.0.0~alpha2"),
    v1_0_0_alpha3("1.0.0~alpha3"),
    v1_0_0_alpha10("1.0.0~alpha10")
  {
  }

  AddonVersion v1_0;
  AddonVersion v1_00;
  AddonVersion v1_0_0;
  AddonVersion v1_1;
  AddonVersion v1_01;
  AddonVersion v1_0_1;
  AddonVersion e1_v1_0_0;
  AddonVersion e1_v1_0_1;
  AddonVersion e2_v1_0_0;
  AddonVersion e1_v1_0_0_r1;
  AddonVersion e1_v1_0_1_r1;
  AddonVersion e1_v1_0_0_r2;
  AddonVersion v1_0_0_beta;
  AddonVersion v1_0_0_alpha;
  AddonVersion v1_0_0_alpha2;
  AddonVersion v1_0_0_alpha3;
  AddonVersion v1_0_0_alpha10;
};

TEST_F(TestAddonVersion, Constructor)
{
  EXPECT_EQ(v1_0.Upstream(), "1.0");
  EXPECT_EQ(v1_0.Epoch(), 0);
  EXPECT_TRUE(v1_0.Revision().empty());

  EXPECT_EQ(v1_00.Upstream(), "1.00");
  EXPECT_EQ(v1_00.Epoch(), 0);
  EXPECT_TRUE(v1_00.Revision().empty());

  EXPECT_EQ(v1_0_0.Upstream(), "1.0.0");
  EXPECT_EQ(v1_0_0.Epoch(), 0);
  EXPECT_TRUE(v1_0_0.Revision().empty());

  EXPECT_EQ(v1_1.Upstream(), "1.1");
  EXPECT_EQ(v1_1.Epoch(), 0);
  EXPECT_TRUE(v1_1.Revision().empty());

  EXPECT_EQ(v1_01.Upstream(), "1.01");
  EXPECT_EQ(v1_01.Epoch(), 0);
  EXPECT_TRUE(v1_01.Revision().empty());

  EXPECT_EQ(v1_0_1.Upstream(), "1.0.1");
  EXPECT_EQ(v1_0_1.Epoch(), 0);
  EXPECT_TRUE(v1_0_1.Revision().empty());

  EXPECT_EQ(e1_v1_0_0.Upstream(), "1.0.0");
  EXPECT_EQ(e1_v1_0_0.Epoch(), 1);
  EXPECT_TRUE(e1_v1_0_0.Revision().empty());

  EXPECT_EQ(e1_v1_0_1.Upstream(), "1.0.1");
  EXPECT_EQ(e1_v1_0_1.Epoch(), 1);
  EXPECT_TRUE(e1_v1_0_1.Revision().empty());

  EXPECT_EQ(e2_v1_0_0.Upstream(), "1.0.0");
  EXPECT_EQ(e2_v1_0_0.Epoch(), 2);
  EXPECT_TRUE(e2_v1_0_0.Revision().empty());

  EXPECT_EQ(e1_v1_0_0_r1.Upstream(), "1.0.0");
  EXPECT_EQ(e1_v1_0_0_r1.Epoch(), 1);
  EXPECT_EQ(e1_v1_0_0_r1.Revision(), "1");

  EXPECT_EQ(e1_v1_0_1_r1.Upstream(), "1.0.1");
  EXPECT_EQ(e1_v1_0_1_r1.Epoch(), 1);
  EXPECT_EQ(e1_v1_0_1_r1.Revision(), "1");

  EXPECT_EQ(e1_v1_0_0_r2.Upstream(), "1.0.0");
  EXPECT_EQ(e1_v1_0_0_r2.Epoch(), 1);
  EXPECT_EQ(e1_v1_0_0_r2.Revision(), "2");

  EXPECT_EQ(v1_0_0_beta.Upstream(), "1.0.0~beta");
  EXPECT_EQ(v1_0_0_beta.Epoch(), 0);
  EXPECT_TRUE(v1_0_0_beta.Revision().empty());

  EXPECT_EQ(v1_0_0_alpha.Upstream(), "1.0.0~alpha");
  EXPECT_EQ(v1_0_0_alpha.Epoch(), 0);
  EXPECT_TRUE(v1_0_0_alpha.Revision().empty());

  EXPECT_EQ(v1_0_0_alpha2.Upstream(), "1.0.0~alpha2");
  EXPECT_EQ(v1_0_0_alpha2.Epoch(), 0);
  EXPECT_TRUE(v1_0_0_alpha2.Revision().empty());

  EXPECT_EQ(v1_0_0_alpha3.Upstream(), "1.0.0~alpha3");
  EXPECT_EQ(v1_0_0_alpha3.Epoch(), 0);
  EXPECT_TRUE(v1_0_0_alpha3.Revision().empty());

  EXPECT_EQ(v1_0_0_alpha10.Upstream(), "1.0.0~alpha10");
  EXPECT_EQ(v1_0_0_alpha10.Epoch(), 0);
  EXPECT_TRUE(v1_0_0_alpha10.Revision().empty());
}

TEST_F(TestAddonVersion, asString)
{
  EXPECT_EQ(v1_0.asString(), "1.0");
  EXPECT_EQ(v1_00.asString(), "1.00");
  EXPECT_EQ(v1_0_0.asString(), "1.0.0");
  EXPECT_EQ(v1_1.asString(), "1.1");
  EXPECT_EQ(v1_01.asString(), "1.01");
  EXPECT_EQ(v1_0_1.asString(), "1.0.1");
  EXPECT_EQ(e1_v1_0_0.asString(), "1:1.0.0");
  EXPECT_EQ(e1_v1_0_1.asString(), "1:1.0.1");
  EXPECT_EQ(e2_v1_0_0.asString(), "2:1.0.0");
  EXPECT_EQ(e1_v1_0_0_r1.asString(), "1:1.0.0-1");
  EXPECT_EQ(e1_v1_0_1_r1.asString(), "1:1.0.1-1");
  EXPECT_EQ(e1_v1_0_0_r2.asString(), "1:1.0.0-2");
  EXPECT_EQ(v1_0_0_beta.asString(), "1.0.0~beta");
  EXPECT_EQ(v1_0_0_alpha.asString(), "1.0.0~alpha");
  EXPECT_EQ(v1_0_0_alpha2.asString(), "1.0.0~alpha2");
  EXPECT_EQ(v1_0_0_alpha3.asString(), "1.0.0~alpha3");
  EXPECT_EQ(v1_0_0_alpha10.asString(), "1.0.0~alpha10");
}

TEST_F(TestAddonVersion, Equals)
{
  EXPECT_EQ(v1_0, AddonVersion("1.0"));
  EXPECT_EQ(v1_00, AddonVersion("1.00"));
  EXPECT_EQ(v1_0_0, AddonVersion("1.0.0"));
  EXPECT_EQ(v1_1, AddonVersion("1.1"));
  EXPECT_EQ(v1_01, AddonVersion("1.01"));
  EXPECT_EQ(v1_0_1, AddonVersion("1.0.1"));
  EXPECT_EQ(e1_v1_0_0, AddonVersion("1:1.0.0"));
  EXPECT_EQ(e1_v1_0_1, AddonVersion("1:1.0.1"));
  EXPECT_EQ(e2_v1_0_0, AddonVersion("2:1.0.0"));
  EXPECT_EQ(e1_v1_0_0_r1, AddonVersion("1:1.0.0-1"));
  EXPECT_EQ(e1_v1_0_1_r1, AddonVersion("1:1.0.1-1"));
  EXPECT_EQ(e1_v1_0_0_r2, AddonVersion("1:1.0.0-2"));
  EXPECT_EQ(v1_0_0_beta, AddonVersion("1.0.0~beta"));
  EXPECT_EQ(v1_0_0_alpha, AddonVersion("1.0.0~alpha"));
  EXPECT_EQ(v1_0_0_alpha2, AddonVersion("1.0.0~alpha2"));
  EXPECT_EQ(v1_0_0_alpha3, AddonVersion("1.0.0~alpha3"));
  EXPECT_EQ(v1_0_0_alpha10, AddonVersion("1.0.0~alpha10"));
}

TEST_F(TestAddonVersion, Equivalent)
{
  EXPECT_FALSE(v1_0 != v1_00);
  EXPECT_FALSE(v1_0 < v1_00);
  EXPECT_FALSE(v1_0 > v1_00);
  EXPECT_TRUE(v1_0 == v1_00);

  EXPECT_FALSE(v1_01 != v1_1);
  EXPECT_FALSE(v1_01 < v1_1);
  EXPECT_FALSE(v1_01 > v1_1);
  EXPECT_TRUE(v1_01 == v1_1);
}

TEST_F(TestAddonVersion, LessThan)
{
  EXPECT_LT(v1_0, v1_0_0);
  EXPECT_LT(v1_0, v1_1);
  EXPECT_LT(v1_0, v1_01);
  EXPECT_LT(v1_0, v1_0_1);

  EXPECT_LT(v1_00, v1_0_0);
  EXPECT_LT(v1_00, v1_1);
  EXPECT_LT(v1_00, v1_01);
  EXPECT_LT(v1_00, v1_0_1);

  EXPECT_LT(v1_0_0, v1_1);
  EXPECT_LT(v1_0_0, v1_01);
  EXPECT_LT(v1_0_0, v1_0_1);

  EXPECT_LT(v1_0_1, v1_01);
  EXPECT_LT(v1_0_1, v1_1);

  // epochs
  EXPECT_LT(v1_0_0, e1_v1_0_0);
  EXPECT_LT(v1_0_0, e1_v1_0_1);
  EXPECT_LT(v1_0_0, e2_v1_0_0);
  EXPECT_LT(v1_0_1, e1_v1_0_1);
  EXPECT_LT(v1_0_1, e2_v1_0_0);

  EXPECT_LT(e1_v1_0_0, e1_v1_0_1);
  EXPECT_LT(e1_v1_0_0, e2_v1_0_0);
  EXPECT_LT(e1_v1_0_1, e2_v1_0_0);

  // revisions
  EXPECT_LT(e1_v1_0_0, e1_v1_0_0_r1);
  EXPECT_LT(e1_v1_0_0, e1_v1_0_1_r1);
  EXPECT_LT(e1_v1_0_0, e1_v1_0_0_r2);
  EXPECT_LT(e1_v1_0_1, e1_v1_0_1_r1);
  EXPECT_LT(e1_v1_0_0_r1, e1_v1_0_1);
  EXPECT_LT(e1_v1_0_0_r1, e1_v1_0_1_r1);
  EXPECT_LT(e1_v1_0_0_r1, e1_v1_0_0_r2);
  EXPECT_LT(e1_v1_0_0_r2, e1_v1_0_1);
  EXPECT_LT(e1_v1_0_0_r2, e1_v1_0_1_r1);
  EXPECT_LT(e1_v1_0_1_r1, e2_v1_0_0);

  // alpha, beta
  EXPECT_LT(v1_0_0_beta, v1_0_0);
  EXPECT_LT(v1_0_0_alpha, v1_0_0);
  EXPECT_LT(v1_0_0_alpha, v1_0_0_beta);
  EXPECT_LT(v1_0_0_alpha, v1_0_0_alpha2);
  EXPECT_LT(v1_0_0_alpha, v1_0_0_alpha3);
  EXPECT_LT(v1_0_0_alpha, v1_0_0_alpha10);
  EXPECT_LT(v1_0_0_alpha2, v1_0_0);
  EXPECT_LT(v1_0_0_alpha2, v1_0_0_beta);
  EXPECT_LT(v1_0_0_alpha2, v1_0_0_alpha3);
  EXPECT_LT(v1_0_0_alpha2, v1_0_0_alpha10);
  EXPECT_LT(v1_0_0_alpha3, v1_0_0);
  EXPECT_LT(v1_0_0_alpha3, v1_0_0_beta);
  EXPECT_LT(v1_0_0_alpha3, v1_0_0_alpha10);
  EXPECT_LT(v1_0_0_alpha10, v1_0_0);
  EXPECT_LT(v1_0_0_alpha10, v1_0_0_beta);
}
