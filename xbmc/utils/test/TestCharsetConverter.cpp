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
#include "utils/StdString.h"
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


/* for realistic test results all characters in the next sequences are real characters from Unicode tables */

/* two bytes UTF-8 sequences and corresponding wide strings */
static const unsigned char testCyrUtf8u[] =
{ 0xD0, 0x90, 0xD0, 0x91, 0xD0, 0x92, 0xD0, 0x93, 0xD0, 0x94, 0xD0, 0x95, 0xD0, 0x96, 0xD0, 0x97, 0xD0, 0x98, 0xD0, 0x99, 0xD0, 0x9A, 0xD0,
  0x9B, 0xD0, 0x9C, 0xD0, 0x9D, 0xD0, 0x9E, 0xD0, 0x9F, 0xD0, 0xA0, 0xD0, 0xA1, 0xD0, 0xA2, 0xD0, 0xA3, 0xD0, 0xA4, 0xD0, 0xA5, 0xD0, 0xA6,
  0xD0, 0xA7, 0xD0, 0xA8, 0xD0, 0xA9, 0xD0, 0xAA, 0xD0, 0xAB, 0xD0, 0xAC, 0xD0, 0xAD, 0xD0, 0xAE, 0xD0, 0xAF, 0xD0, 0xB0, 0xD0, 0xB1, 0xD0,
  0xB2, 0xD0, 0xB3, 0xD0, 0xB4, 0xD0, 0xB5, 0xD0, 0xB6, 0xD0, 0xB7, 0xD0, 0xB8, 0xD0, 0xB9, 0xD0, 0xBA, 0xD0, 0xBB, 0xD0, 0xBC, 0xD0, 0xBD,
  0xD0, 0xBE, 0xD0, 0xBF, 0xD1, 0x80, 0xD1, 0x81, 0xD1, 0x82, 0xD1, 0x83, 0xD1, 0x84, 0xD1, 0x85, 0xD1, 0x86, 0xD1, 0x87, 0xD1, 0x88, 0xD1,
  0x89, 0xD1, 0x8A, 0xD1, 0x8B, 0xD1, 0x8C, 0xD1, 0x8D, 0xD1, 0x8E, 0xD1, 0x8F, 0 };
static const char* const testCyrUtf8 = (const char*)testCyrUtf8u;
static const wchar_t testCyrW[] =
{ 0x410, 0x411, 0x412, 0x413, 0x414, 0x415, 0x416, 0x417, 0x418, 0x419, 0x41A, 0x41B, 0x41C, 0x41D, 0x41E, 0x41F, 0x420, 0x421, 0x422, 0x423,
  0x424, 0x425, 0x426, 0x427, 0x428, 0x429, 0x42A, 0x42B, 0x42C, 0x42D, 0x42E, 0x42F, 0x430, 0x431, 0x432, 0x433, 0x434, 0x435, 0x436, 0x437,
  0x438, 0x439, 0x43A, 0x43B, 0x43C, 0x43D, 0x43E, 0x43F, 0x440, 0x441, 0x442, 0x443, 0x444, 0x445, 0x446, 0x447, 0x448, 0x449, 0x44A, 0x44B,
  0x44C, 0x44D, 0x44E, 0x44F, 0 };
static const unsigned char testGreekUtf8u[] =
{ 0xCE, 0x91, 0xCE, 0x92, 0xCE, 0x93, 0xCE, 0x94, 0xCE, 0x95, 0xCE, 0x96, 0xCE, 0x97, 0xCE, 0x98, 0xCE, 0x99, 0xCE, 0x9A, 0xCE, 0x9B, 0xCF,
  0x80, 0xCF, 0x81, 0xCF, 0x82, 0xCF, 0x83, 0xCF, 0x84, 0xCF, 0x85, 0xCF, 0x86, 0xCF, 0x87, 0xCF, 0x88, 0xCF, 0x89, 0 };
static const char* const testGreekUtf8 = (const char*)testGreekUtf8u;
static const wchar_t testGreekW[] =
{ 0x391, 0x392, 0x393, 0x394, 0x395, 0x396, 0x397, 0x398, 0x399, 0x39A, 0x39B, 0x3C0, 0x3C1, 0x3C2, 0x3C3, 0x3C4, 0x3C5, 0x3C6, 0x3C7, 0x3C8,
  0x3C9, 0 };
static const unsigned char testArmUtf8u[] =
{ 0xD4, 0xB1, 0xD4, 0xB2, 0xD4, 0xB3, 0xD4, 0xB4, 0xD4, 0xB5, 0xD4, 0xB6, 0xD4, 0xB7, 0xD4, 0xB8, 0xD4, 0xB9, 0xD4, 0xBA, 0xD5, 0xA7, 0xD5,
  0xA8, 0xD5, 0xA9, 0xD5, 0xAA, 0xD5, 0xAB, 0xD5, 0xAC, 0xD5, 0xAD, 0xD5, 0xAE, 0xD5, 0xAF, 0xD5, 0xB0, 0xD5, 0xB1, 0xD5, 0xB2, 0 };
static const char* const testArmUtf8 = (const char*)testArmUtf8u;
static const wchar_t testArmW[] =
{ 0x531, 0x532, 0x533, 0x534, 0x535, 0x536, 0x537, 0x538, 0x539, 0x53A, 0x567, 0x568, 0x569, 0x56A, 0x56B, 0x56C, 0x56D, 0x56E, 0x56F, 0x570,
  0x571, 0x572, 0 };

/* three bytes UTF-8 sequences and corresponding wide strings */
static const unsigned char testDevnUtf8u[] =
{ 0xE0, 0xA4, 0x84, 0xE0, 0xA4, 0x85, 0xE0, 0xA4, 0x86, 0xE0, 0xA4, 0x87, 0xE0, 0xA4, 0x88, 0xE0, 0xA4, 0x89, 0xE0, 0xA4, 0x8A, 0xE0, 0xA4,
  0x8B, 0xE0, 0xA4, 0x8C, 0xE0, 0xA4, 0x8D, 0xE0, 0xA4, 0x8E, 0xE0, 0xA5, 0xBB, 0xE0, 0xA5, 0xBC, 0xE0, 0xA5, 0xBD, 0xE0, 0xA5, 0xBE, 0xE0,
  0xA5, 0xBF, 0 };
static const char* const testDevnUtf8 = (const char*)testDevnUtf8u;
static const wchar_t testDevnW[] =
{ 0x904, 0x905, 0x906, 0x907, 0x908, 0x909, 0x90A, 0x90B, 0x90C, 0x90D, 0x90E, 0x97B, 0x97C, 0x97D, 0x97E, 0x97F, 0 };
static const unsigned char testGeorgUtf8u[] =
{ 0xE1, 0x82, 0xA0, 0xE1, 0x82, 0xA1, 0xE1, 0x82, 0xA2, 0xE1, 0x82, 0xA3, 0xE1, 0x82, 0xA4, 0xE1, 0x82, 0xA5, 0xE1, 0x82, 0xA6, 0xE1, 0x83,
  0xA8, 0xE1, 0x83, 0xA9, 0xE1, 0x83, 0xAA, 0xE1, 0x83, 0xAB, 0xE1, 0x83, 0xAC, 0xE1, 0x83, 0xAD, 0xE1, 0x83, 0xAE, 0xE1, 0x83, 0xAF, 0 };
static const char* const testGeorgUtf8 = (const char*)testGeorgUtf8u;
static const wchar_t testGeorgW[] =
{ 0x10A0, 0x10A1, 0x10A2, 0x10A3, 0x10A4, 0x10A5, 0x10A6, 0x10E8, 0x10E9, 0x10EA, 0x10EB, 0x10EC, 0x10ED, 0x10EE, 0x10EF, 0 };
static const unsigned char testCJKUtf8u[] =
{ 0xE7, 0x81, 0xB5, 0xE7, 0x81, 0xB6, 0xE7, 0x81, 0xB7, 0xE7, 0x81, 0xB8, 0xE7, 0x81, 0xB9, 0xE7, 0x81, 0xBA, 0xE7, 0x81, 0xBB, 0xE7, 0x83,
  0xA9, 0xE7, 0x83, 0xAA, 0xE7, 0x83, 0xAB, 0xE7, 0x83, 0xAC, 0xE7, 0x83, 0xAD, 0 };
static const char* const testCJKUtf8 = (const char*)testCJKUtf8u;
static const wchar_t testCJKW[] =
{ 0x7075, 0x7076, 0x7077, 0x7078, 0x7079, 0x707A, 0x707B, 0x70E9, 0x70EA, 0x70EB, 0x70EC, 0x70ED, 0 };
static const unsigned char testArLiUtf8u[] =
{ 0xEF, 0xB5, 0x94, 0xEF, 0xB5, 0x95, 0xEF, 0xB5, 0x96, 0xEF, 0xB5, 0x97, 0xEF, 0xB5, 0x98, 0xEF, 0xB6, 0xA7, 0xEF, 0xB6, 0xA8, 0 };
static const char* const testArLiUtf8 = (const char*)testArLiUtf8u;
static const wchar_t testArLiW[] =
{ 0xFD54, 0xFD55, 0xFD56, 0xFD57, 0xFD58, 0xFDA7, 0xFDA8, 0 };

/* four bytes UTF-8 sequences */
static const unsigned char testLinBIdeoUtf8u[] =
{ 0xF0, 0x90, 0x82, 0x82, 0xF0, 0x90, 0x82, 0x83, 0xF0, 0x90, 0x82, 0x84, 0xF0, 0x90, 0x82, 0x85, 0xF0, 0x90, 0x83, 0xA8, 0xF0, 0x90, 0x83,
  0xA9, 0xF0, 0x90, 0x83, 0xAA, 0xF0, 0x90, 0x83, 0xAB, 0xF0, 0x90, 0x83, 0xAC, 0xF0, 0x90, 0x83, 0xAD, 0 };
static const char* const testLinBIdeoUtf8 = (const char*)testLinBIdeoUtf8u;
static const unsigned char testCJKComIdUtf8u[] =
{ 0xF0, 0xAF, 0xA1, 0xA3, 0xF0, 0xAF, 0xA1, 0xA4, 0xF0, 0xAF, 0xA1, 0xA5, 0xF0, 0xAF, 0xA1, 0xA6, 0xF0, 0xAF, 0xA1, 0xA7, 0xF0, 0xAF, 0xA1,
  0xA8, 0xF0, 0xAF, 0xA3, 0x96, 0xF0, 0xAF, 0xA3, 0x97, 0xF0, 0xAF, 0xA3, 0x98, 0xF0, 0xAF, 0xA3, 0x99, 0xF0, 0xAF, 0xA3, 0x9A, 0 };
static const char* const testCJKComIdUtf8 = (const char*)testCJKComIdUtf8u;
static const unsigned char testTagsUtf8u[] =
{ 0xF3, 0xA0, 0x80, 0xA1, 0xF3, 0xA0, 0x80, 0xA2, 0xF3, 0xA0, 0x80, 0xA3, 0xF3, 0xA0, 0x80, 0xA4, 0xF3, 0xA0, 0x80, 0xA5, 0xF3, 0xA0, 0x80,
  0xA6, 0xF3, 0xA0, 0x80, 0xA7, 0xF3, 0xA0, 0x80, 0xA8, 0xF3, 0xA0, 0x80, 0xA9, 0xF3, 0xA0, 0x80, 0xAA, 0xF3, 0xA0, 0x81, 0xAA, 0xF3, 0xA0,
  0x81, 0xAB, 0xF3, 0xA0, 0x81, 0xAC, 0xF3, 0xA0, 0x81, 0xAD, 0xF3, 0xA0, 0x81, 0xAE, 0xF3, 0xA0, 0x81, 0xAF, 0 };
static const char* const testTagsUtf8 = (const char*)testTagsUtf8u;


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

    g_charsetConverter.reset();
    g_charsetConverter.clear();
  }

  ~TestCharsetConverter()
  {
    CSettings::Get().Unload();
  }

  CStdStringA refstra1, refstra2, varstra1;
  CStdStringW refstrw1, varstrw1;
  CStdString16 refstr16_1, varstr16_1;
  CStdString32 refstr32_1, varstr32_1;
  CStdString refstr1;
};

TEST_F(TestCharsetConverter, utf8ToW)
{
  refstra1 = "test utf8ToW";
  refstrw1 = L"test utf8ToW";
  varstrw1.clear();
  g_charsetConverter.utf8ToW(refstra1, varstrw1, true, false, NULL);
  EXPECT_STREQ(refstrw1.c_str(), varstrw1.c_str());
}

TEST_F(TestCharsetConverter, utf16LEtoW)
{
  refstrw1 = L"ｔｅｓｔ＿ｕｔｆ１６ＬＥｔｏｗ";
  /* TODO: Should be able to use '=' operator instead of assign() */
  refstr16_1.assign(refutf16LE1);
  varstrw1.clear();
  g_charsetConverter.utf16LEtoW(refstr16_1, varstrw1);
  EXPECT_STREQ(refstrw1.c_str(), varstrw1.c_str());
}

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
  refstra1 = "test utf8To: charset ASCII, CStdStringA";
  varstra1.clear();
  g_charsetConverter.utf8To("ASCII", refstra1, varstra1);
  EXPECT_STREQ(refstra1.c_str(), varstra1.c_str());
}

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

TEST_F(TestCharsetConverter, utf8To_UTF32LE)
{
  refstra1 = "ｔｅｓｔ＿ｕｔｆ８Ｔｏ：＿ｃｈａｒｓｅｔ＿ＵＴＦ－３２ＬＥ，＿"
#ifdef TARGET_DARWIN
/* OSX has it's own 'special' utf-8 charset which we use (see UTF8_SOURCE in CharsetConverter.cpp)
   which is basically NFD (decomposed) utf-8.  The trouble is, it fails on the COW FACE and MOUSE FACE
   characters for some reason (possibly anything over 0x100000, or maybe there's a decomposed form of these
   that I couldn't find???)  If UTF8_SOURCE is switched to UTF-8 then this test would pass as-is, but then
   some filenames stored in utf8-mac wouldn't display correctly in the UI. */
             "ＣＳｔｄＳｔｒｉｎｇ３２＿";
#else
             "ＣＳｔｄＳｔｒｉｎｇ３２＿🐭🐮";
#endif
  refstr32_1.assign(refutf32LE1);
  varstr32_1.clear();
  g_charsetConverter.utf8To("UTF-32LE", refstra1, varstr32_1);
  EXPECT_TRUE(!memcmp(refstr32_1.c_str(), varstr32_1.c_str(),
                      sizeof(refutf32LE1)));
}

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
//   void ucs2CharsetToStringCharset(const CStdStringW& strSource,
//                                   CStdStringA& strDest, bool swap = false);
// }

TEST_F(TestCharsetConverter, wToUTF8)
{
  refstrw1 = L"ｔｅｓｔ＿ｗＴｏＵＴＦ８";
  refstra1 = "ｔｅｓｔ＿ｗＴｏＵＴＦ８";
  varstra1.clear();
  g_charsetConverter.wToUTF8(refstrw1, varstra1);
  EXPECT_STREQ(refstra1.c_str(), varstra1.c_str());
}

TEST_F(TestCharsetConverter, utf16BEtoUTF8)
{
  refstr16_1.assign(refutf16BE);
  refstra1 = "ｔｅｓｔ＿ｕｔｆ１６ＢＥｔｏＵＴＦ８";
  varstra1.clear();
  g_charsetConverter.utf16BEtoUTF8(refstr16_1, varstra1);
  EXPECT_STREQ(refstra1.c_str(), varstra1.c_str());
}

TEST_F(TestCharsetConverter, utf16LEtoUTF8)
{
  refstr16_1.assign(refutf16LE4);
  refstra1 = "ｔｅｓｔ＿ｕｔｆ１６ＬＥｔｏＵＴＦ８";
  varstra1.clear();
  g_charsetConverter.utf16LEtoUTF8(refstr16_1, varstra1);
  EXPECT_STREQ(refstra1.c_str(), varstra1.c_str());
}

TEST_F(TestCharsetConverter, ucs2ToUTF8)
{
  refstr16_1.assign(refucs2);
  refstra1 = "ｔｅｓｔ＿ｕｃｓ２ｔｏＵＴＦ８";
  varstra1.clear();
  g_charsetConverter.ucs2ToUTF8(refstr16_1, varstra1);
  EXPECT_STREQ(refstra1.c_str(), varstra1.c_str());
}

TEST_F(TestCharsetConverter, utf8logicalToVisualBiDi)
{
  refstra1 = "ｔｅｓｔ＿ｕｔｆ８ｌｏｇｉｃａｌＴｏＶｉｓｕａｌＢｉＤｉ";
  refstra2 = "ｔｅｓｔ＿ｕｔｆ８ｌｏｇｉｃａｌＴｏＶｉｓｕａｌＢｉＤｉ";
  varstra1.clear();
  g_charsetConverter.utf8logicalToVisualBiDi(refstra1, varstra1);
  EXPECT_STREQ(refstra2.c_str(), varstra1.c_str());
}

TEST_F(TestCharsetConverter, simpleWToUtf8)
{
  /* test one byte UTF-8 sequences */
  EXPECT_STREQ("Simple US-ASCII string 1234567890,?./!", CCharsetConverter::simpleWToUtf8(L"Simple US-ASCII string 1234567890,?./!").c_str());

  /* test two bytes UTF-8 sequences */
  EXPECT_STREQ(testCyrUtf8, CCharsetConverter::simpleWToUtf8(testCyrW).c_str());
  EXPECT_STREQ(testGreekUtf8, CCharsetConverter::simpleWToUtf8(testGreekW).c_str());
  EXPECT_STREQ(testArmUtf8, CCharsetConverter::simpleWToUtf8(testArmW).c_str());

  /* test three bytes UTF-8 sequences */
  EXPECT_STREQ(testDevnUtf8, CCharsetConverter::simpleWToUtf8(testDevnW).c_str());
  EXPECT_STREQ(testGeorgUtf8, CCharsetConverter::simpleWToUtf8(testGeorgW).c_str());
  EXPECT_STREQ(testCJKUtf8, CCharsetConverter::simpleWToUtf8(testCJKW).c_str());
  EXPECT_STREQ(testArLiUtf8, CCharsetConverter::simpleWToUtf8(testArLiW).c_str());

  /* test four bytes UTF-8 sequences */
  /* those tests can fail on some platform with limited wchar_t range and lack of surrogates support */
  std::wstring helperStringW;
  EXPECT_TRUE(g_charsetConverter.utf8ToW(testLinBIdeoUtf8, helperStringW, false, false, true)) << "Can't prepare source wide string for test";
  EXPECT_STREQ(testLinBIdeoUtf8, CCharsetConverter::simpleWToUtf8(helperStringW).c_str());
  helperStringW.clear();
  EXPECT_TRUE(g_charsetConverter.utf8ToW(testCJKComIdUtf8, helperStringW, false, false, true)) << "Can't prepare source wide string for test";
  EXPECT_STREQ(testCJKComIdUtf8, CCharsetConverter::simpleWToUtf8(helperStringW).c_str());
  helperStringW.clear();
  EXPECT_TRUE(g_charsetConverter.utf8ToW(testTagsUtf8, helperStringW, false, false, true)) << "Can't prepare source wide string for test";
  EXPECT_STREQ(testTagsUtf8, CCharsetConverter::simpleWToUtf8(helperStringW).c_str());

  /* test invalid chars */
  static const wchar_t testInvalid1W[] =
  { L'a', L'b', L'c', 0xD800, 0 }; // invalid char at the end
  static const wchar_t testInvalid2W[] =
  { L'1', L'2', 0xDE00, 0xDF00, L'3', L'4', 0 }; // invalid chars at the middle
  static const wchar_t testInvalid3W[] =
  { 0xD901, L'x', L'y', L'z', 0 }; // invalid char at the beginning
  static const wchar_t testInvalid4W[] =
  { 0xDA11, 0xDBCC, 0 }; // only invalid char
  EXPECT_TRUE(CCharsetConverter::simpleWToUtf8(testInvalid1W, true).empty());
  EXPECT_TRUE(CCharsetConverter::simpleWToUtf8(testInvalid2W, true).empty());
  EXPECT_TRUE(CCharsetConverter::simpleWToUtf8(testInvalid3W, true).empty());
  EXPECT_TRUE(CCharsetConverter::simpleWToUtf8(testInvalid4W, true).empty());
  EXPECT_STREQ("abc", CCharsetConverter::simpleWToUtf8(testInvalid1W, false).c_str());
  EXPECT_STREQ("1234", CCharsetConverter::simpleWToUtf8(testInvalid2W, false).c_str());
  EXPECT_STREQ("xyz", CCharsetConverter::simpleWToUtf8(testInvalid3W, false).c_str());
  EXPECT_STREQ("", CCharsetConverter::simpleWToUtf8(testInvalid4W, false).c_str());
}

TEST_F(TestCharsetConverter, simpleUtf8ToW)
{
  /* test one byte UTF-8 sequences */
  EXPECT_STREQ(L"Simple US-ASCII string 1234567890,?./!", CCharsetConverter::simpleUtf8ToW("Simple US-ASCII string 1234567890,?./!").c_str());

  /* test two bytes UTF-8 sequences */
  EXPECT_STREQ(testCyrW, CCharsetConverter::simpleUtf8ToW(testCyrUtf8).c_str());
  EXPECT_STREQ(testGreekW, CCharsetConverter::simpleUtf8ToW(testGreekUtf8).c_str());
  EXPECT_STREQ(testArmW, CCharsetConverter::simpleUtf8ToW(testArmUtf8).c_str());

  /* test three bytes UTF-8 sequences */
  EXPECT_STREQ(testDevnW, CCharsetConverter::simpleUtf8ToW(testDevnUtf8).c_str());
  EXPECT_STREQ(testGeorgW, CCharsetConverter::simpleUtf8ToW(testGeorgUtf8).c_str());
  EXPECT_STREQ(testCJKW, CCharsetConverter::simpleUtf8ToW(testCJKUtf8).c_str());
  EXPECT_STREQ(testArLiW, CCharsetConverter::simpleUtf8ToW(testArLiUtf8).c_str());

  /* test four bytes UTF-8 sequences */
  /* those tests can fail on some platform with limited wchar_t range and lack of surrogates support */
  std::string resultStringUtf8;
  EXPECT_TRUE(g_charsetConverter.wToUTF8(CCharsetConverter::simpleUtf8ToW(testLinBIdeoUtf8), resultStringUtf8, true)) << "Can't convert result back to UTF-8 from wide string";
  EXPECT_EQ(testLinBIdeoUtf8, resultStringUtf8);
  resultStringUtf8.clear();
  EXPECT_TRUE(g_charsetConverter.wToUTF8(CCharsetConverter::simpleUtf8ToW(testCJKComIdUtf8), resultStringUtf8, true)) << "Can't convert result back to UTF-8 from wide string";
  EXPECT_EQ(testCJKComIdUtf8, resultStringUtf8);
  resultStringUtf8.clear();
  EXPECT_TRUE(g_charsetConverter.wToUTF8(CCharsetConverter::simpleUtf8ToW(testTagsUtf8), resultStringUtf8, true)) << "Can't convert result back to UTF-8 from wide string";
  EXPECT_EQ(testTagsUtf8, resultStringUtf8);

  /* test invalid sequences */
  static const char testInvalid1Utf8[] =
  { 'a', 'b', 'c', (char)0xC0, 0 }; // invalid sequences at the end
  static const char testInvalid2Utf8[] =
  { '1', '2', (char)0xC1, (char)0xAB, '3', '4', 0 }; // invalid sequences at the middle
  static const char testInvalid3Utf8[] =
  { 'F', 'J', (char)0xC2, 'Q', 'W', 0 }; // incomplete sequences at the middle, 'Q' must be decoded
  static const char testInvalid4Utf8[] =
  { (char)0xF7, 'x', 'y', 'z', 0 }; // invalid sequences at the beginning
  static const char testInvalid5Utf8[] =
  { ' ', (char)0xED, (char)0xA0, (char)0x80, (char)0xED, (char)0xB0, (char)0x82, '?', '!', '*', 0 }; // surrogates in the middle
  static const char testInvalid6Utf8[] =
  { (char)0xF4, (char)0x90, (char)0x80, (char)0x80, 0 }; // only invalid sequences
  EXPECT_TRUE(CCharsetConverter::simpleUtf8ToW(testInvalid1Utf8, true).empty());
  EXPECT_TRUE(CCharsetConverter::simpleUtf8ToW(testInvalid2Utf8, true).empty());
  EXPECT_TRUE(CCharsetConverter::simpleUtf8ToW(testInvalid3Utf8, true).empty());
  EXPECT_TRUE(CCharsetConverter::simpleUtf8ToW(testInvalid4Utf8, true).empty());
  EXPECT_TRUE(CCharsetConverter::simpleUtf8ToW(testInvalid5Utf8, true).empty());
  EXPECT_TRUE(CCharsetConverter::simpleUtf8ToW(testInvalid6Utf8, true).empty());
  EXPECT_STREQ(L"abc", CCharsetConverter::simpleUtf8ToW(testInvalid1Utf8, false).c_str());
  EXPECT_STREQ(L"1234", CCharsetConverter::simpleUtf8ToW(testInvalid2Utf8, false).c_str());
  EXPECT_STREQ(L"FJQW", CCharsetConverter::simpleUtf8ToW(testInvalid3Utf8, false).c_str());
  EXPECT_STREQ(L"xyz", CCharsetConverter::simpleUtf8ToW(testInvalid4Utf8, false).c_str());
  EXPECT_STREQ(L" ?!*", CCharsetConverter::simpleUtf8ToW(testInvalid5Utf8, false).c_str());
  EXPECT_STREQ(L"", CCharsetConverter::simpleUtf8ToW(testInvalid6Utf8, false).c_str());
}

/* TODO: Resolve correct input/output for this function */
// TEST_F(TestCharsetConverter, utf32ToStringCharset)
// {
//   void utf32ToStringCharset(const unsigned long* strSource, CStdStringA& strDest);
// }

TEST_F(TestCharsetConverter, getCharsetLabels)
{
  std::vector<CStdString> reflabels;
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
  CStdString varstr =
    g_charsetConverter.getCharsetLabelByName("ISO-8859-1");
  EXPECT_STREQ("Western Europe (ISO)", varstr.c_str());
  varstr.clear();
  varstr = g_charsetConverter.getCharsetLabelByName("Bogus");
  EXPECT_STREQ("", varstr.c_str());
}

TEST_F(TestCharsetConverter, getCharsetNameByLabel)
{
  CStdString varstr =
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
