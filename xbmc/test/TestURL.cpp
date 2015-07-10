/*
 *      Copyright (C) 2005-2013 Team XBMC
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

#include "URL.h"

#include "gtest/gtest.h"

using ::testing::Test;
using ::testing::WithParamInterface;
using ::testing::ValuesIn;

struct TestURLGetWithoutUserDetailsData
{
  std::string input;
  std::string expected;
  bool redact;
};

std::ostream& operator<<(std::ostream& os,
                       const TestURLGetWithoutUserDetailsData& rhs)
{
  return os << "(Input: " << rhs.input <<
    "; Redact: " << (rhs.redact?"true":"false") <<
    "; Expected: " << rhs.expected << ")";
}

class TestURLGetWithoutUserDetails : public Test,
                                     public WithParamInterface<TestURLGetWithoutUserDetailsData>
{
};

TEST_P(TestURLGetWithoutUserDetails, GetWithoutUserDetails)
{
  CURL input(GetParam().input);
  std::string result = input.GetWithoutUserDetails(GetParam().redact);
  EXPECT_EQ(result, GetParam().expected);
}

const TestURLGetWithoutUserDetailsData values[] = {
  { std::string("smb://example.com/example"), std::string("smb://example.com/example"), false },
  { std::string("smb://example.com/example"), std::string("smb://example.com/example"), true },
  { std::string("smb://god:universe@example.com/example"), std::string("smb://example.com/example"), false },
  { std::string("smb://god@example.com/example"), std::string("smb://USERNAME@example.com/example"), true },
  { std::string("smb://god:universe@example.com/example"), std::string("smb://USERNAME:PASSWORD@example.com/example"), true },
  { std::string("http://god:universe@example.com:8448/example|auth=digest"), std::string("http://USERNAME:PASSWORD@example.com:8448/example|auth=digest"), true },
  { std::string("smb://fd00::1/example"), std::string("smb://fd00::1/example"), false },
  { std::string("smb://fd00::1/example"), std::string("smb://fd00::1/example"), true },
  { std::string("smb://[fd00::1]:8080/example"), std::string("smb://[fd00::1]:8080/example"), false },
  { std::string("smb://[fd00::1]:8080/example"), std::string("smb://[fd00::1]:8080/example"), true },
  { std::string("smb://god:universe@[fd00::1]:8080/example"), std::string("smb://[fd00::1]:8080/example"), false },
  { std::string("smb://god@[fd00::1]:8080/example"), std::string("smb://USERNAME@[fd00::1]:8080/example"), true },
  { std::string("smb://god:universe@fd00::1/example"), std::string("smb://USERNAME:PASSWORD@fd00::1/example"), true },
  { std::string("http://god:universe@[fd00::1]:8448/example|auth=digest"), std::string("http://USERNAME:PASSWORD@[fd00::1]:8448/example|auth=digest"), true },
  { std::string("smb://00ff:1:0000:abde::/example"), std::string("smb://ff:1:0:abde::/example"), true },
  { std::string("smb://god:universe@[00ff:1:0000:abde::]:8080/example"), std::string("smb://[ff:1:0:abde::]:8080/example"), false },
  { std::string("smb://god@[00ff:1:0000:abde::]:8080/example"), std::string("smb://USERNAME@[ff:1:0:abde::]:8080/example"), true },
  { std::string("smb://god:universe@00ff:1:0000:abde::/example"), std::string("smb://USERNAME:PASSWORD@ff:1:0:abde::/example"), true },
  { std::string("http://god:universe@[00ff:1:0000:abde::]:8448/example|auth=digest"), std::string("http://USERNAME:PASSWORD@[ff:1:0:abde::]:8448/example|auth=digest"), true }
  };

INSTANTIATE_TEST_CASE_P(URL, TestURLGetWithoutUserDetails, ValuesIn(values));
