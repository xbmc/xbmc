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

#include "settings/Settings.h"
#include "utils/CharsetConverter.h"
#include "utils/Utf8Utils.h"
#include "system.h"

#include "gtest/gtest.h"

static const uint16_t refutf16LE1[] = { 0xff54, 0xff45, 0xff53, 0xff54,
                                        0xff3f, 0xff55, 0xff54, 0xff46,
                                        0xff11, 0xff16, 0xff2c, 0xff25,
                                        0xff54, 0xff4f, 0xff57, 0x0 };

static const uint16_t refutf16LE2[] = { 0xff54, 0xff45, 0xff53, 0xff54,
                                        0xff3f, 0xff55, 0xff54, 0xff46,
                                        0xff18, 0xff34, 0xff4f, 0xff1a,
                                        0xff3f, 0xff43, 0xff48, 0xff41,
                                        0xff52, 0xff53, 0xff45, 0xff54,
                                        0xff3f, 0xff35, 0xff34, 0xff26,
                                        0xff0d, 0xff11, 0xff16, 0xff2c,
                                        0xff25, 0xff0c, 0xff3f, 0xff23,
                                        0xff33, 0xff54, 0xff44, 0xff33,
                                        0xff54, 0xff52, 0xff49, 0xff4e,
                                        0xff47, 0xff11, 0xff16, 0x0 };

static const char refutf16LE3[] = "T\377E\377S\377T\377?\377S\377T\377"
                                  "R\377I\377N\377G\377#\377H\377A\377"
                                  "R\377S\377E\377T\377\064\377O\377\065"
                                  "\377T\377F\377\030\377";

static const uint16_t refutf16LE4[] = { 0xff54, 0xff45, 0xff53, 0xff54,
                                        0xff3f, 0xff55, 0xff54, 0xff46,
                                        0xff11, 0xff16, 0xff2c, 0xff25,
                                        0xff54, 0xff4f, 0xff35, 0xff34,
                                        0xff26, 0xff18, 0x0 };

static const uint32_t refutf32LE1[] = { 0xff54, 0xff45, 0xff53, 0xff54,
                                       0xff3f, 0xff55, 0xff54, 0xff46,
                                       0xff18, 0xff34, 0xff4f, 0xff1a,
                                       0xff3f, 0xff43, 0xff48, 0xff41,
                                       0xff52, 0xff53, 0xff45, 0xff54,
                                       0xff3f, 0xff35, 0xff34, 0xff26,
                                       0xff0d, 0xff13, 0xff12, 0xff2c,
                                       0xff25, 0xff0c, 0xff3f, 0xff23,
                                       0xff33, 0xff54, 0xff44, 0xff33,
                                       0xff54, 0xff52, 0xff49, 0xff4e,
                                       0xff47, 0xff13, 0xff12, 0xff3f,
#ifdef TARGET_DARWIN
                                       0x0 };
#else
                                       0x1f42d, 0x1f42e, 0x0 };
#endif

static const uint16_t refutf16BE[] = { 0x54ff, 0x45ff, 0x53ff, 0x54ff,
                                       0x3fff, 0x55ff, 0x54ff, 0x46ff,
                                       0x11ff, 0x16ff, 0x22ff, 0x25ff,
                                       0x54ff, 0x4fff, 0x35ff, 0x34ff,
                                       0x26ff, 0x18ff, 0x0};

static const uint16_t refucs2[] = { 0xff54, 0xff45, 0xff53, 0xff54,
                                    0xff3f, 0xff55, 0xff43, 0xff53,
                                    0xff12, 0xff54, 0xff4f, 0xff35,
                                    0xff34, 0xff26, 0xff18, 0x0 };

class TestCharsetConverter : public testing::Test
{
protected:
  TestCharsetConverter()
  {
    /* Add default settings for locale.
     * Settings here are taken from CGUISettings::Initialize()
     */
    /* TODO
    CSettingsCategory *loc = CSettings::Get().AddCategory(7, "locale", 14090);
    CSettings::Get().AddString(loc, "locale.language",248,"english",
                            SPIN_CONTROL_TEXT);
    CSettings::Get().AddString(loc, "locale.country", 20026, "USA",
                            SPIN_CONTROL_TEXT);
    CSettings::Get().AddString(loc, "locale.charset", 14091, "DEFAULT",
                            SPIN_CONTROL_TEXT); // charset is set by the
                                                // language file

    // Add default settings for subtitles
    CSettingsCategory *sub = CSettings::Get().AddCategory(5, "subtitles", 287);
    CSettings::Get().AddString(sub, "subtitles.charset", 735, "DEFAULT",
                            SPIN_CONTROL_TEXT);
    */
    CSettings::Get().Initialize();
    g_charsetConverter.reset();
    g_charsetConverter.clear();
  }

  ~TestCharsetConverter()
  {
    CSettings::Get().Unload();
  }

  std::string refstra1, refstra2, varstra1;
  std::wstring refstrw1, varstrw1;
  std::string refstr1;
};

TEST_F(TestCharsetConverter, utf8ToW)
{
  refstra1 = "test utf8ToW";
  refstrw1 = L"test utf8ToW";
  varstrw1.clear();
  g_charsetConverter.utf8ToW(refstra1, varstrw1, true, false, false);
  EXPECT_STREQ(refstrw1.c_str(), varstrw1.c_str());
}


//TEST_F(TestCharsetConverter, utf16LEtoW)
//{
//  refstrw1 = L"ｔｅｓｔ＿ｕｔｆ１６ＬＥｔｏｗ";
//  /* TODO: Should be able to use '=' operator instead of assign() */
//  std::wstring refstr16_1;
//  refstr16_1.assign(refutf16LE1);
//  varstrw1.clear();
//  g_charsetConverter.utf16LEtoW(refstr16_1, varstrw1);
//  EXPECT_STREQ(refstrw1.c_str(), varstrw1.c_str());
//}

TEST_F(TestCharsetConverter, subtitleCharsetToUtf8)
{
  refstra1 = "test subtitleCharsetToW";
  varstra1.clear();
  g_charsetConverter.subtitleCharsetToUtf8(refstra1, varstra1);

  /* Assign refstra1 to refstrw1 so that we can compare */
  EXPECT_STREQ(refstra1.c_str(), varstra1.c_str());
}

TEST_F(TestCharsetConverter, utf8ToStringCharset_1)
{
  refstra1 = "test utf8ToStringCharset";
  varstra1.clear();
  g_charsetConverter.utf8ToStringCharset(refstra1, varstra1);
  EXPECT_STREQ(refstra1.c_str(), varstra1.c_str());
}

TEST_F(TestCharsetConverter, utf8ToStringCharset_2)
{
  refstra1 = "test utf8ToStringCharset";
  varstra1 = "test utf8ToStringCharset";
  g_charsetConverter.utf8ToStringCharset(varstra1);
  EXPECT_STREQ(refstra1.c_str(), varstra1.c_str());
}

TEST_F(TestCharsetConverter, utf8ToSystem)
{
  refstra1 = "test utf8ToSystem";
  varstra1 = "test utf8ToSystem";
  g_charsetConverter.utf8ToSystem(varstra1);
  EXPECT_STREQ(refstra1.c_str(), varstra1.c_str());
}

TEST_F(TestCharsetConverter, utf8To_ASCII)
{
  refstra1 = "test utf8To: charset ASCII, std::string";
  varstra1.clear();
  g_charsetConverter.utf8To("ASCII", refstra1, varstra1);
  EXPECT_STREQ(refstra1.c_str(), varstra1.c_str());
}

/*
TEST_F(TestCharsetConverter, utf8To_UTF16LE)
{
  refstra1 = "ｔｅｓｔ＿ｕｔｆ８Ｔｏ：＿ｃｈａｒｓｅｔ＿ＵＴＦ－１６ＬＥ，＿"
             "ＣＳｔｄＳｔｒｉｎｇ１６";
  refstr16_1.assign(refutf16LE2);
  varstr16_1.clear();
  g_charsetConverter.utf8To("UTF-16LE", refstra1, varstr16_1);
  EXPECT_TRUE(!memcmp(refstr16_1.c_str(), varstr16_1.c_str(),
                      refstr16_1.length() * sizeof(uint16_t)));
}
*/

//TEST_F(TestCharsetConverter, utf8To_UTF32LE)
//{
//  refstra1 = "ｔｅｓｔ＿ｕｔｆ８Ｔｏ：＿ｃｈａｒｓｅｔ＿ＵＴＦ－３２ＬＥ，＿"
//#ifdef TARGET_DARWIN
///* OSX has it's own 'special' utf-8 charset which we use (see UTF8_SOURCE in CharsetConverter.cpp)
//   which is basically NFD (decomposed) utf-8.  The trouble is, it fails on the COW FACE and MOUSE FACE
//   characters for some reason (possibly anything over 0x100000, or maybe there's a decomposed form of these
//   that I couldn't find???)  If UTF8_SOURCE is switched to UTF-8 then this test would pass as-is, but then
//   some filenames stored in utf8-mac wouldn't display correctly in the UI. */
//             "ＣＳｔｄＳｔｒｉｎｇ３２＿";
//#else
//             "ＣＳｔｄＳｔｒｉｎｇ３２＿🐭🐮";
//#endif
//  refstr32_1.assign(refutf32LE1);
//  varstr32_1.clear();
//  g_charsetConverter.utf8To("UTF-32LE", refstra1, varstr32_1);
//  EXPECT_TRUE(!memcmp(refstr32_1.c_str(), varstr32_1.c_str(),
//                      sizeof(refutf32LE1)));
//}

TEST_F(TestCharsetConverter, stringCharsetToUtf8)
{
  refstra1 = "ｔｅｓｔ＿ｓｔｒｉｎｇＣｈａｒｓｅｔＴｏＵｔｆ８";
  varstra1.clear();
  g_charsetConverter.ToUtf8("UTF-16LE", refutf16LE3, varstra1);
  EXPECT_STREQ(refstra1.c_str(), varstra1.c_str());
}

TEST_F(TestCharsetConverter, isValidUtf8_1)
{
  varstra1.clear();
  g_charsetConverter.ToUtf8("UTF-16LE", refutf16LE3, varstra1);
  EXPECT_TRUE(CUtf8Utils::isValidUtf8(varstra1.c_str()));
}

TEST_F(TestCharsetConverter, isValidUtf8_2)
{
  refstr1 = refutf16LE3;
  EXPECT_FALSE(CUtf8Utils::isValidUtf8(refstr1));
}

TEST_F(TestCharsetConverter, isValidUtf8_3)
{
  varstra1.clear();
  g_charsetConverter.ToUtf8("UTF-16LE", refutf16LE3, varstra1);
  EXPECT_TRUE(CUtf8Utils::isValidUtf8(varstra1.c_str()));
}

TEST_F(TestCharsetConverter, isValidUtf8_4)
{
  EXPECT_FALSE(CUtf8Utils::isValidUtf8(refutf16LE3));
}

/* TODO: Resolve correct input/output for this function */
// TEST_F(TestCharsetConverter, ucs2CharsetToStringCharset)
// {
//   void ucs2CharsetToStringCharset(const std::wstring& strSource,
//                                   std::string& strDest, bool swap = false);
// }

TEST_F(TestCharsetConverter, wToUTF8)
{
  refstrw1 = L"ｔｅｓｔ＿ｗＴｏＵＴＦ８";
  refstra1 = "ｔｅｓｔ＿ｗＴｏＵＴＦ８";
  varstra1.clear();
  g_charsetConverter.wToUTF8(refstrw1, varstra1);
  EXPECT_STREQ(refstra1.c_str(), varstra1.c_str());
}

//TEST_F(TestCharsetConverter, utf16BEtoUTF8)
//{
//  refstr16_1.assign(refutf16BE);
//  refstra1 = "ｔｅｓｔ＿ｕｔｆ１６ＢＥｔｏＵＴＦ８";
//  varstra1.clear();
//  g_charsetConverter.utf16BEtoUTF8(refstr16_1, varstra1);
//  EXPECT_STREQ(refstra1.c_str(), varstra1.c_str());
//}

//TEST_F(TestCharsetConverter, utf16LEtoUTF8)
//{
//  refstr16_1.assign(refutf16LE4);
//  refstra1 = "ｔｅｓｔ＿ｕｔｆ１６ＬＥｔｏＵＴＦ８";
//  varstra1.clear();
//  g_charsetConverter.utf16LEtoUTF8(refstr16_1, varstra1);
//  EXPECT_STREQ(refstra1.c_str(), varstra1.c_str());
//}

//TEST_F(TestCharsetConverter, ucs2ToUTF8)
//{
//  refstr16_1.assign(refucs2);
//  refstra1 = "ｔｅｓｔ＿ｕｃｓ２ｔｏＵＴＦ８";
//  varstra1.clear();
//  g_charsetConverter.ucs2ToUTF8(refstr16_1, varstra1);
//  EXPECT_STREQ(refstra1.c_str(), varstra1.c_str());
//}

TEST_F(TestCharsetConverter, utf8logicalToVisualBiDi)
{
  refstra1 = "ｔｅｓｔ＿ｕｔｆ８ｌｏｇｉｃａｌＴｏＶｉｓｕａｌＢｉＤｉ";
  refstra2 = "ｔｅｓｔ＿ｕｔｆ８ｌｏｇｉｃａｌＴｏＶｉｓｕａｌＢｉＤｉ";
  varstra1.clear();
  g_charsetConverter.utf8logicalToVisualBiDi(refstra1, varstra1);
  EXPECT_STREQ(refstra2.c_str(), varstra1.c_str());
}

/* TODO: Resolve correct input/output for this function */
// TEST_F(TestCharsetConverter, utf32ToStringCharset)
// {
//   void utf32ToStringCharset(const unsigned long* strSource, std::string& strDest);
// }

TEST_F(TestCharsetConverter, getCharsetLabels)
{
  std::vector<std::string> reflabels;
  reflabels.push_back("Western Europe (ISO)");
  reflabels.push_back("Central Europe (ISO)");
  reflabels.push_back("South Europe (ISO)");
  reflabels.push_back("Baltic (ISO)");
  reflabels.push_back("Cyrillic (ISO)");
  reflabels.push_back("Arabic (ISO)");
  reflabels.push_back("Greek (ISO)");
  reflabels.push_back("Hebrew (ISO)");
  reflabels.push_back("Turkish (ISO)");
  reflabels.push_back("Central Europe (Windows)");
  reflabels.push_back("Cyrillic (Windows)");
  reflabels.push_back("Western Europe (Windows)");
  reflabels.push_back("Greek (Windows)");
  reflabels.push_back("Turkish (Windows)");
  reflabels.push_back("Hebrew (Windows)");
  reflabels.push_back("Arabic (Windows)");
  reflabels.push_back("Baltic (Windows)");
  reflabels.push_back("Vietnamesse (Windows)");
  reflabels.push_back("Thai (Windows)");
  reflabels.push_back("Chinese Traditional (Big5)");
  reflabels.push_back("Chinese Simplified (GBK)");
  reflabels.push_back("Japanese (Shift-JIS)");
  reflabels.push_back("Korean");
  reflabels.push_back("Hong Kong (Big5-HKSCS)");

  std::vector<std::string> varlabels = g_charsetConverter.getCharsetLabels();
  ASSERT_EQ(reflabels.size(), varlabels.size());

  std::vector<std::string>::iterator it;
  for (it = varlabels.begin(); it < varlabels.end(); ++it)
  {
    EXPECT_STREQ((reflabels.at(it - varlabels.begin())).c_str(), (*it).c_str());
  }
}

TEST_F(TestCharsetConverter, getCharsetLabelByName)
{
  std::string varstr =
    g_charsetConverter.getCharsetLabelByName("ISO-8859-1");
  EXPECT_STREQ("Western Europe (ISO)", varstr.c_str());
  varstr.clear();
  varstr = g_charsetConverter.getCharsetLabelByName("Bogus");
  EXPECT_STREQ("", varstr.c_str());
}

TEST_F(TestCharsetConverter, getCharsetNameByLabel)
{
  std::string varstr =
    g_charsetConverter.getCharsetNameByLabel("Western Europe (ISO)");
  EXPECT_STREQ("ISO-8859-1", varstr.c_str());
  varstr.clear();
  varstr = g_charsetConverter.getCharsetNameByLabel("Bogus");
  EXPECT_STREQ("", varstr.c_str());
}

TEST_F(TestCharsetConverter, unknownToUTF8_1)
{
  refstra1 = "ｔｅｓｔ＿ｕｎｋｎｏｗｎＴｏＵＴＦ８";
  varstra1 = "ｔｅｓｔ＿ｕｎｋｎｏｗｎＴｏＵＴＦ８";
  g_charsetConverter.unknownToUTF8(varstra1);
  EXPECT_STREQ(refstra1.c_str(), varstra1.c_str());
}

TEST_F(TestCharsetConverter, unknownToUTF8_2)
{
  refstra1 = "ｔｅｓｔ＿ｕｎｋｎｏｗｎＴｏＵＴＦ８";
  varstra1.clear();
  g_charsetConverter.unknownToUTF8(refstra1, varstra1);
  EXPECT_STREQ(refstra1.c_str(), varstra1.c_str());
}

TEST_F(TestCharsetConverter, toW)
{
  refstra1 = "ｔｅｓｔ＿ｔｏＷ：＿ｃｈａｒｓｅｔ＿ＵＴＦ－１６ＬＥ";
  refstrw1 = L"\xBDEF\xEF94\x85BD\xBDEF\xEF93\x94BD\xBCEF\xEFBF"
             L"\x94BD\xBDEF\xEF8F\xB7BC\xBCEF\xEF9A\xBFBC\xBDEF"
             L"\xEF83\x88BD\xBDEF\xEF81\x92BD\xBDEF\xEF93\x85BD"
             L"\xBDEF\xEF94\xBFBC\xBCEF\xEFB5\xB4BC\xBCEF\xEFA6"
             L"\x8DBC\xBCEF\xEF91\x96BC\xBCEF\xEFAC\xA5BC";
  varstrw1.clear();
  g_charsetConverter.toW(refstra1, varstrw1, "UTF-16LE");
  EXPECT_STREQ(refstrw1.c_str(), varstrw1.c_str());
}

TEST_F(TestCharsetConverter, fromW)
{
  refstrw1 = L"\xBDEF\xEF94\x85BD\xBDEF\xEF93\x94BD\xBCEF\xEFBF"
             L"\x86BD\xBDEF\xEF92\x8FBD\xBDEF\xEF8D\xB7BC\xBCEF"
             L"\xEF9A\xBFBC\xBDEF\xEF83\x88BD\xBDEF\xEF81\x92BD"
             L"\xBDEF\xEF93\x85BD\xBDEF\xEF94\xBFBC\xBCEF\xEFB5"
             L"\xB4BC\xBCEF\xEFA6\x8DBC\xBCEF\xEF91\x96BC\xBCEF"
             L"\xEFAC\xA5BC";
  refstra1 = "ｔｅｓｔ＿ｆｒｏｍＷ：＿ｃｈａｒｓｅｔ＿ＵＴＦ－１６ＬＥ";
  varstra1.clear();
  g_charsetConverter.fromW(refstrw1, varstra1, "UTF-16LE");
  EXPECT_STREQ(refstra1.c_str(), varstra1.c_str());
}
