/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "addons/AddonVersion.h"

#include <gtest/gtest.h>

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

  CAddonVersion v1_0;
  CAddonVersion v1_00;
  CAddonVersion v1_0_0;
  CAddonVersion v1_1;
  CAddonVersion v1_01;
  CAddonVersion v1_0_1;
  CAddonVersion e1_v1_0_0;
  CAddonVersion e1_v1_0_1;
  CAddonVersion e2_v1_0_0;
  CAddonVersion e1_v1_0_0_r1;
  CAddonVersion e1_v1_0_1_r1;
  CAddonVersion e1_v1_0_0_r2;
  CAddonVersion v1_0_0_beta;
  CAddonVersion v1_0_0_alpha;
  CAddonVersion v1_0_0_alpha2;
  CAddonVersion v1_0_0_alpha3;
  CAddonVersion v1_0_0_alpha10;
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
  EXPECT_EQ(v1_0, CAddonVersion("1.0"));
  EXPECT_EQ(v1_00, CAddonVersion("1.00"));
  EXPECT_EQ(v1_0_0, CAddonVersion("1.0.0"));
  EXPECT_EQ(v1_1, CAddonVersion("1.1"));
  EXPECT_EQ(v1_01, CAddonVersion("1.01"));
  EXPECT_EQ(v1_0_1, CAddonVersion("1.0.1"));
  EXPECT_EQ(e1_v1_0_0, CAddonVersion("1:1.0.0"));
  EXPECT_EQ(e1_v1_0_1, CAddonVersion("1:1.0.1"));
  EXPECT_EQ(e2_v1_0_0, CAddonVersion("2:1.0.0"));
  EXPECT_EQ(e1_v1_0_0_r1, CAddonVersion("1:1.0.0-1"));
  EXPECT_EQ(e1_v1_0_1_r1, CAddonVersion("1:1.0.1-1"));
  EXPECT_EQ(e1_v1_0_0_r2, CAddonVersion("1:1.0.0-2"));
  EXPECT_EQ(v1_0_0_beta, CAddonVersion("1.0.0~beta"));
  EXPECT_EQ(v1_0_0_alpha, CAddonVersion("1.0.0~alpha"));
  EXPECT_EQ(v1_0_0_alpha2, CAddonVersion("1.0.0~alpha2"));
  EXPECT_EQ(v1_0_0_alpha3, CAddonVersion("1.0.0~alpha3"));
  EXPECT_EQ(v1_0_0_alpha10, CAddonVersion("1.0.0~alpha10"));
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
  
  // pep-0440/local-version-identifiers
  // ref: https://www.python.org/dev/peps/pep-0440/#local-version-identifiers
  // Python addons use this kind of versioning particularly for script.module
  // addons. The "same" version number may exist in different branches or
  // targeting different kodi versions while keeping consistency with the
  // upstream module version. The addon version available in upper repos
  // (let's say matrix) must have a higher version than the one stored in
  // lower branches (e.g. leia) so that users receive the addon update
  // when upgrading kodi.
  // So, for instance, we use version x.x.x or version x.x.x+kodiversion.r to
  // refer to the same upstream version x.x.x of the module.
  // Eg: script.module.foo-1.0.0 or script.module.foo-1.0.0+leia.1 for upstream
  // module foo (version 1.0.0) available for leia; and
  // script.module.foo-1.0.0+matrix.1 for upstream module foo (1.0.0) for matrix.
  // In summary, 1.0.0 or 1.0.0+leia.1 must be < than 1.0.0+matrix.1
  // tests below assure this won't get broken inadvertently
  EXPECT_LT(CAddonVersion("1.0.0"), CAddonVersion("1.0.0+matrix.1"));
  EXPECT_LT(CAddonVersion("1.0.0+leia.1"), CAddonVersion("1.0.0+matrix.1"));
  EXPECT_LT(CAddonVersion("1.0.0+matrix.1"), CAddonVersion("1.0.0+matrix.2"));
  EXPECT_LT(CAddonVersion("1.0.0+matrix.1"), CAddonVersion("1.0.1+matrix.1"));
  EXPECT_LT(CAddonVersion("1.0.0+matrix.1"), CAddonVersion("1.1.0+matrix.1"));
  EXPECT_LT(CAddonVersion("1.0.0+matrix.1"), CAddonVersion("2.0.0+matrix.1"));
  EXPECT_LT(CAddonVersion("1.0.0+matrix.1"), CAddonVersion("1.0.0.1"));
  EXPECT_LT(CAddonVersion("1.0.0+Leia.1"), CAddonVersion("1.0.0+matrix.1"));
  EXPECT_LT(CAddonVersion("1.0.0+leia.1"), CAddonVersion("1.0.0+Matrix.1"));
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
  
  // pep-0440/local-version-identifiers
  EXPECT_TRUE(CAddonVersion("1.0.0+leia.1") == CAddonVersion("1.0.0+Leia.1"));
}
