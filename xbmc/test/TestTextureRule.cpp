/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "TextureDatabase.h"
#include "dbwrappers/test/DatabaseTestUtils.h"
#include "utils/JSONVariantParser.h"
#include "utils/Variant.h"

#include <memory>
#include <optional>
#include <ostream>
#include <string>
#include <string_view>

#include <gtest/gtest.h>

namespace
{
struct FilterCase
{
  //! The filter parameter of Textures.GetTextures
  std::string_view filter;
  std::string_view where;
};

constexpr std::string_view REFUSED{"refused"};

void PrintTo(const FilterCase& c, std::ostream* os)
{
  *os << c.filter;
}

constexpr const char* DB_NAME = "TestTextureRule";

class TestTextureRule : public ::testing::TestWithParam<FilterCase>
{
protected:
  static void SetUpTestSuite()
  {
    s_db = std::make_unique<CConnectedTestDatabase<CTextureDatabase>>(DB_NAME);
  }

  static void TearDownTestSuite() { s_db.reset(); }

  //! The clause as JSONRPC::CTextureOperations::GetTextures builds it
  static std::optional<std::string> WhereOf(std::string_view json)
  {
    CVariant filter;
    if (!CJSONVariantParser::Parse(std::string{json}, filter))
    {
      ADD_FAILURE() << "could not parse " << json;
      return std::nullopt;
    }

    CVariant rules(CVariant::VariantTypeObject);
    if (filter.isMember("field"))
    {
      rules["and"] = CVariant(CVariant::VariantTypeArray);
      rules["and"].push_back(filter);
    }
    else
    {
      rules = filter;
    }

    CTextureRuleGroup group;
    if (!group.Load(rules))
    {
      return std::nullopt;
    }
    return group.GetWhereClause(s_db->Get(), {});
  }

  static std::unique_ptr<CConnectedTestDatabase<CTextureDatabase>> s_db;
};

std::unique_ptr<CConnectedTestDatabase<CTextureDatabase>> TestTextureRule::s_db;

// clang-format off
const FilterCase FILTERS[] = {
    {R"({"field":"url","operator":"is","value":"special://skin/media/a.png"})",
     R"sql(((texture.url LIKE 'special://skin/media/a.png')))sql"},
    {R"({"field":"url","operator":"contains","value":"image://video@"})",
     R"sql(((texture.url LIKE '%image://video@%')))sql"},
    {R"({"field":"url","operator":"startswith","value":"http://example.com/"})",
     R"sql(((texture.url LIKE 'http://example.com/%')))sql"},
    {R"({"field":"url","operator":"isnot","value":["a.png","b.png"]})",
     R"sql(((texture.url NOT  LIKE 'a.png' OR texture.url IS NULL) AND (texture.url NOT  LIKE 'b.png' OR texture.url IS NULL)))sql"},
    {R"({"field":"cachedurl","operator":"endswith","value":".jpg"})",
     R"sql(((texture.cachedurl LIKE '%.jpg')))sql"},
    {R"({"field":"cachedurl","operator":"doesnotcontain","value":"0/"})",
     R"sql(((texture.cachedurl NOT  LIKE '%0/%' OR texture.cachedurl IS NULL)))sql"},
    {R"({"field":"imagehash","operator":"is","value":""})",
     R"sql(((texture.imagehash LIKE '' OR texture.imagehash IS NULL)))sql"},
    {R"({"field":"lasthashcheck","operator":"lessthan","value":"2020-01-01"})",
     R"sql(((texture.lasthashcheck < '2020-01-01')))sql"},
    {R"({"field":"lastlibrarycheck","operator":"greaterthan","value":"2020-01-01"})",
     R"sql(((texture.lastlibrarycheck > '2020-01-01')))sql"},
    {R"({"field":"lastused","operator":"before","value":"2020-01-01"})",
     R"sql(((sizes.lastusetime < '2020-01-01')))sql"},
    {R"({"field":"lastused","operator":"after","value":"2020-01-01"})",
     R"sql(((sizes.lastusetime > '2020-01-01')))sql"},
    {R"({"field":"textureid","operator":"isnot","value":"7"})",
     R"sql(((texture.id != 7)))sql"},
    {R"({"field":"textureid","operator":"between","value":["1","9"]})",
     R"sql((texture.id BETWEEN 1 AND 9))sql"},
    {R"({"field":"textureid","operator":"is","value":["1","2","3"]})",
     R"sql(((texture.id = 1) OR (texture.id = 2) OR (texture.id = 3)))sql"},
    {R"({"field":"width","operator":"lessthan","value":"1920"})",
     R"sql(((sizes.width < 1920)))sql"},
    {R"({"field":"height","operator":"greaterthan","value":""})",
     R"sql(((sizes.height > 0 OR sizes.height IS NULL)))sql"},
    {R"({"field":"usecount","operator":"is","value":"0"})",
     R"sql(((sizes.usecount = 0)))sql"},
    {R"({"field":"usecount","operator":"between","value":["1"]})",
     R"sql(('1'))sql"},
    {R"({"field":"url","operator":"between","value":["a","m"]})",
     R"sql((texture.url BETWEEN 'a' AND 'm'))sql"},
    {R"({"field":"url","operator":"true"})",
     R"sql(('1'))sql"},
    {R"({"field":"width","operator":"false"})",
     R"sql(('1'))sql"},
    {R"({"field":"nosuchfield","operator":"is","value":"x"})",
     R"sql((()))sql"},
    {R"({"field":"url","operator":"nosuchoperator","value":"x"})",
     R"sql(((texture.url LIKE '%x%')))sql"},
    {R"({"and":[{"or":[{"field":"width","operator":"is","value":"1"},{"field":"height","operator":"is","value":"2"}]},{"field":"usecount","operator":"greaterthan","value":"3"}]})",
     R"sql((((sizes.width = 1)) OR ((sizes.height = 2))) AND ((sizes.usecount > 3)))sql"},
    {R"({"or":[{"field":"url","operator":"true"},{"field":"width","operator":"is","value":"1"}]})",
     R"sql(('0') OR ((sizes.width = 1)))sql"},
    {R"({"and":[]})",
     R"sql()sql"},
    {R"({"field":"width","operator":"greaterthan","value":"1 OR 1=1"})",
     REFUSED},
    {R"({"field":"textureid","operator":"between","value":["1","9 OR 1=1"]})",
     REFUSED},
};
// clang-format on
} // namespace

TEST_P(TestTextureRule, AFilterGivesItsClause)
{
  const std::optional<std::string> where{WhereOf(GetParam().filter)};
  if (GetParam().where == REFUSED)
  {
    EXPECT_FALSE(where);
    return;
  }
  ASSERT_TRUE(where);
  EXPECT_EQ(GetParam().where, *where);
}

INSTANTIATE_TEST_SUITE_P(Filters, TestTextureRule, ::testing::ValuesIn(FILTERS));
