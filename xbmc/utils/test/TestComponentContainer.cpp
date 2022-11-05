/*
 *  Copyright (C) 2015-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "utils/ComponentContainer.h"

#include <utility>

#include <gtest/gtest.h>

class BaseTestType
{
public:
  virtual ~BaseTestType() = default;
};

struct DerivedType1 : public BaseTestType
{
  int a = 1;
};

struct DerivedType2 : public BaseTestType
{
  int a = 2;
};

struct DerivedType3 : public BaseTestType
{
  int a = 3;
};

class TestContainer : public CComponentContainer<BaseTestType>
{
  FRIEND_TEST(TestComponentContainer, Generic);
};

TEST(TestComponentContainer, Generic)
{
  TestContainer container;

  // check that we can register types
  container.RegisterComponent(std::make_shared<DerivedType1>());
  EXPECT_EQ(container.size(), 1u);
  container.RegisterComponent(std::make_shared<DerivedType2>());
  EXPECT_EQ(container.size(), 2u);

  // check that trying to register a component twice does nothing
  container.RegisterComponent(std::make_shared<DerivedType2>());
  EXPECT_EQ(container.size(), 2u);

  // check that first component is valid
  const auto t1 = container.GetComponent<DerivedType1>();
  EXPECT_TRUE(t1 != nullptr);
  EXPECT_EQ(t1->a, 1);

  // check that second component is valid
  const auto t2 = container.GetComponent<DerivedType2>();
  EXPECT_TRUE(t2 != nullptr);
  EXPECT_EQ(t2->a, 2);

  // check that third component is not there
  EXPECT_THROW(container.GetComponent<DerivedType3>(), std::logic_error);

  // check that component instance is constant
  const auto t4 = container.GetComponent<DerivedType1>();
  EXPECT_EQ(t1.get(), t4.get());

  // check we can call the const overload for GetComponent
  // and that the returned type is const
  const auto t5 = const_cast<const TestContainer&>(container).GetComponent<DerivedType1>();
  EXPECT_TRUE(std::is_const_v<typename decltype(t5)::element_type>);
}
