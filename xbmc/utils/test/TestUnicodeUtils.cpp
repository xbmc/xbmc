/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include <sstream>
#include "unicode/uloc.h"
#include "unicode/locid.h"
#include "utils/UnicodeUtils.h"

#include <algorithm>
#include <fstream>
#include <functional>
#include <locale>
#include <mcheck.h>
#include <tuple>

#include <gtest/gtest.h>
enum class ECG
{
  A,
  B
};

enum EG
{
  C,
  D
};

namespace test_enum
{
enum class ECN
{
  A = 1,
      B
};
enum EN
{
  C = 1,
  D
};
} // namespace test_enum

namespace TestUnicodeUtils
{
// MULTICODEPOINT_CHARS

// There are multiple ways to compose some graphemes. These are the same grapheme:

// Each of these five series of unicode codepoints represent the
// SAME grapheme (character)! To compare them correctly they should be
// normalized first. Normalization should reduce each sequence to the
// single codepoint (although some graphemes require more than one
// codepoint after normalization).
//
// TODO: It may be a good idea to normalize everything on
// input and renormalize when something requires it.
// A: U+006f (o) + U+0302 (◌̂) + U+0323 (◌̣): o◌̣◌̂
const char32_t MULTI_CODEPOINT_CHAR_1_VARIENT_1[] = {0x006f, 0x0302, 0x0323};
// B: U+006f (o) + U+0323 (◌̣) + U+0302 (◌̂)
const char32_t MULTI_CODEPOINT_CHAR_1_VARIENT_2[] = {0x006f, 0x0323, 0x302};
// C: U+00f4 (ô) + U+0323 (◌̣)
const char32_t MULTI_CODEPOINT_CHAR_1_VARIENT_3[] = {0x00f4, 0x0323};
// D: U+1ecd (ọ) + U+0302 (◌̂)
const char32_t MULTI_CODEPOINT_CHAR_1_VARIENT_4[] = {0x1ecd, 0x0302};
// E: U+1ed9 (ộ)
const char32_t MULTI_CODEPOINT_CHAR_1_VARIENT_5[] = {0x1ed9};

// UTF8 versions of the above.

const char UTF8_MULTI_CODEPOINT_CHAR1_VARIENT_1[] = {"\x6f\xcc\x82\xcc\xa3\x00"};
const char UTF8_MULTI_CODEPOINT_CHAR1_VARIENT_2[] = {"\x6f\xcc\xa3\xcc\x82\x00"};
const char UTF8_MULTI_CODEPOINT_CHAR1_VARIENT_3[] = {"\xc3\xb4\xcc\xa3\x00"};
const char UTF8_MULTI_CODEPOINT_CHAR1_VARIENT_4[] = {"\xe1\xbb\x8d\xcc\x82\x00"};
const char UTF8_MULTI_CODEPOINT_CHAR1_VARIENT_5[] = {"\xe1\xbb\x99\x00"};

const char UTF8_SHORT_CHINESE[] =
{"戰後，國民政府接收臺灣"};

const char UTF8_LONG_CHINESE[] =
{"戰後，國民政府接收臺灣，負責接收臺北帝國大學的羅宗洛亦因此成為戰後臺大的首任代理校長，並邀請克洛爾留任臺"
    "大，並由講師升任為副教授，調往物理學系教授物理學與德語[3]:177-178。然而，在羅宗洛離任之後，由於克洛爾身"
    "為敗戰國國民，因此有了將被降職的傳聞。直到傅斯年就任臺灣大學校長之後，再度鼓勵其留任，同時為他加薪[3]:"
    "178。傅斯年並邀請克洛爾參加1951年5月24日大一課程設定計畫的討論，而克洛爾則是該次會議中唯一的外籍教師[3]"
    ":1781948年，日本的《學藝雜誌》登出了克洛爾有關電子半徑研究的論文，成為臺大物理系教師第一篇在國際科學"
    "刊物上發表的論文[3]:179。1950年，克洛爾獨力完成了固體比熱方面的研究，其研究成果論文則在1952年刊登"
    "於日本京都出版的《理論物理的發展》，並於1954年以此升任教授[3]:179[11]:58。在這段期間，克洛爾也指導"
    "他當時的助教黃振麟，共同從事固體比熱與頻譜方面的研究，而黃振麟則在1955年完成休斯頓法的修正，利用插值"
    "法確定等頻譜線圖，避免了原本休斯頓法在高頻端的奇點問題。同時他也將該次研究成果寫成論文，發表於該年度的"
    "《物理評論》上，成為了臺灣本土學者在該刊物的第一篇論文[11]:58,66[12]:103。此後，臺大物理系理論物理課"
    "程幾乎是由克洛爾負責教學，其講授課程包含了理論物理學、量子力學、相對論、統計力學、物理數學、電子學"
    "等理論課程[3]:178[4]。他的理論物理課程主要是參考德國物理學家阿諾·索末菲所著之教科書，並將其內容整理"
    "成筆記作為教材使用[11]:58。克洛爾上課時會印講義給學生"};
const char UTF8_LONG_THAI[] =
{"ประเทศไทย มีชื่ออย่างเป็นทางการว่า ราชอาณาจักรไทย เป็นรัฐชาติอันตั้งอยู่ในภูมิภาคเอเชียตะวันออกเฉียงใต้ เดิมมีชื่อว่า"
    " \"สยาม\" รัฐบาลประกาศเปลี่ยนชื่อเป็นประเทศไทยอย่างเป็นทางการตั้งแต่ปี 2482 ประเทศไทยมีขนาดใหญ่เป็นอันดับที่"
    " 50 ของโลก มีเนื้อที่ 513,120 ตารางกิโลเมตร[10] และมีประชากรมากเป็นอันดับที่ 20 ของโลก คือ ประมาณ 70 "
    "ล้านคน มีอาณาเขตติดต่อกับประเทศพม่าทางทิศเหนือและตะวันตก ประเทศลาวทางทิศเหนือและตะวันออก ประเทศกัมพูชาทา"
    "งทิศตะวันออก และประเทศมาเลเซียทางทิศใต้ กรุงเทพมหานครเป็นศูนย์กลางการบริหารราชการแผ่นดินและนครใหญ่สุดของป"
    "ระเทศ และการปกครองส่วนภูมิภาค จัดระเบียบเป็น 76 จังหวัด[11] แม้จะมีการสถาปนาระบอบราชาธิปไตยภายใต้รัฐธรรม"
    "นูญและประชาธิปไตยระบบรัฐสภา ในปี 2475 แต่กองทัพยังมีบทบาทในการเมืองไทยสูง โดยมีรัฐประหารครั้งล่าสุดในปี 2557 "};
const char UTF8_LONG_RUSSIAN[] =
{" 4 июня, после 100 дней войны, Россия заняла более 80 тыс. км² территории Украины, в том числе большую часть территории Херсонской и Запорожской областей. Таким образом, контролируя вместе с Донбассом и Крымом около 20 % территории Украины[⇨]. Но по состоянию на июнь 2022 года российским войскам не удалось выполнить как предполагаемые изначальные задачи (захват Киева и смена власти на Украине), так и цели, заявленные позднее — выход на административные границы Донецкой и Луганской областей, а также создание сухопутного коридора до Приднестровья[нужна атрибуция мнения][25]. На оккупированных территориях Россией создаются «военно-гражданские администрации», сняты украинские флаги, начата замена украинской гривны на российский рубль и организована выдача российских паспортов[26][27]. "};

const char UTF8_LONG_RUSSIAN_LOWER_CASE[] =
{" 4 июня, после 100 дней войны, россия заняла более 80 тыс. км² территории украины, в том числе большую часть территории херсонской и запорожской областей. таким образом, контролируя вместе с донбассом и крымом около 20 % территории украины[⇨]. но по состоянию на июнь 2022 года российским войскам не удалось выполнить как предполагаемые изначальные задачи (захват киева и смена власти на украине), так и цели, заявленные позднее — выход на административные границы донецкой и луганской областей, а также создание сухопутного коридора до приднестровья[нужна атрибуция мнения][25]. на оккупированных территориях россией создаются «военно-гражданские администрации», сняты украинские флаги, начата замена украинской гривны на российский рубль и организована выдача российских паспортов[26][27]. "};

const char UTF8_LONG_RUSSIAN_UPPER_CASE[] =
{" 4 ИЮНЯ, ПОСЛЕ 100 ДНЕЙ ВОЙНЫ, РОССИЯ ЗАНЯЛА БОЛЕЕ 80 ТЫС. КМ² ТЕРРИТОРИИ УКРАИНЫ, В ТОМ ЧИСЛЕ БОЛЬШУЮ ЧАСТЬ ТЕРРИТОРИИ ХЕРСОНСКОЙ И ЗАПОРОЖСКОЙ ОБЛАСТЕЙ. ТАКИМ ОБРАЗОМ, КОНТРОЛИРУЯ ВМЕСТЕ С ДОНБАССОМ И КРЫМОМ ОКОЛО 20 % ТЕРРИТОРИИ УКРАИНЫ[⇨]. НО ПО СОСТОЯНИЮ НА ИЮНЬ 2022 ГОДА РОССИЙСКИМ ВОЙСКАМ НЕ УДАЛОСЬ ВЫПОЛНИТЬ КАК ПРЕДПОЛАГАЕМЫЕ ИЗНАЧАЛЬНЫЕ ЗАДАЧИ (ЗАХВАТ КИЕВА И СМЕНА ВЛАСТИ НА УКРАИНЕ), ТАК И ЦЕЛИ, ЗАЯВЛЕННЫЕ ПОЗДНЕЕ — ВЫХОД НА АДМИНИСТРАТИВНЫЕ ГРАНИЦЫ ДОНЕЦКОЙ И ЛУГАНСКОЙ ОБЛАСТЕЙ, А ТАКЖЕ СОЗДАНИЕ СУХОПУТНОГО КОРИДОРА ДО ПРИДНЕСТРОВЬЯ[НУЖНА АТРИБУЦИЯ МНЕНИЯ][25]. НА ОККУПИРОВАННЫХ ТЕРРИТОРИЯХ РОССИЕЙ СОЗДАЮТСЯ «ВОЕННО-ГРАЖДАНСКИЕ АДМИНИСТРАЦИИ», СНЯТЫ УКРАИНСКИЕ ФЛАГИ, НАЧАТА ЗАМЕНА УКРАИНСКОЙ ГРИВНЫ НА РОССИЙСКИЙ РУБЛЬ И ОРГАНИЗОВАНА ВЫДАЧА РОССИЙСКИХ ПАСПОРТОВ[26][27]. "};


// 			u"óóßChloë" // German "Sharp-S" ß is (mostly) equivalent to ss (lower case).
//                     Lower case of two SS characters can either be ss or ß,
//                     depending upon context.
// óóßChloë
const char UTF8_GERMAN_SAMPLE[] = {"\xc3\xb3\xc3\xb3\xc3\x9f\x43\x68\x6c\x6f\xc3\xab"};
// u"ÓÓSSCHLOË";
const char* UTF8_GERMAN_UPPER = {"\xc3\x93\xc3\x93\x53\x53\x43\x48\x4c\x4f\xc3\x8b"};
// u"óósschloë"
const char UTF8_GERMAN_LOWER[] = {"\xc3\xb3\xc3\xb3\xc3\x9f\x63\x68\x6c\x6f\xc3\xab"};

// óóßchloë
const char* UTF8_GERMAN_LOWER_SS = {"\xc3\xb3\xc3\xb3\x73\x73\x63\x68\x6c\x6f\xc3\xab"};
// u"óósschloë";
} // namespace TestUnicodeUtils

/**
 * Sample long, multicodepoint emojiis
 *  the transgender flag emoji (🏳️‍⚧️), consists of the five-codepoint sequence"
 *   U+1F3F3 U+FE0F U+200D U+26A7 U+FE0F, requires sixteen bytes to encode,
 *   the flag of Scotland (🏴󠁧󠁢󠁳󠁣󠁴󠁿) requires a total of twenty-eight bytes for the
 *   seven-codepoint sequence U+1F3F4 U+E0067 U+E0062 U+E0073 U+E0063 U+E0074 U+E007F.
 */

static icu::Locale getCLocale()
{
  icu::Locale c_locale = Unicode::GetICULocale(std::locale::classic().name().c_str());
  return c_locale;
}
static icu::Locale getTurkicLocale()
{
  icu::Locale turkic_locale = Unicode::GetICULocale("tr", "TR");
  return turkic_locale;
}
static icu::Locale getUSEnglishLocale()
{
  icu::Locale us_english_locale = icu::Locale::getUS(); // Unicode::GetICULocale("en", "US");
  return us_english_locale;
}

static icu::Locale getUkranianLocale()
{
  icu::Locale ukranian_locale = Unicode::GetICULocale("uk", "UA");
  return ukranian_locale;
}


TEST(TestUnicodeUtils, ToUpper)
{
  std::string refstr = "TEST";

  std::string varstr = "TeSt";
  UnicodeUtils::ToUpper(varstr);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = TestUnicodeUtils::UTF8_GERMAN_UPPER; // ÓÓSSCHLOË
  varstr = TestUnicodeUtils::UTF8_GERMAN_SAMPLE; // óóßChloë
  UnicodeUtils::ToUpper(varstr);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());
  // Lower: óósschloë
}

//static void ToUpper(std::string& str, const std::locale& locale);
TEST(TestUnicodeUtils, ToUpper_Boundary)
{
  // Chinese appear caselsss

  std::string longChinese{TestUnicodeUtils::UTF8_LONG_CHINESE};
  UnicodeUtils::ToUpper(longChinese, icu::Locale::getChinese());
  EXPECT_EQ(UnicodeUtils::Compare(longChinese, TestUnicodeUtils::UTF8_LONG_CHINESE), 0);
  UnicodeUtils::ToLower(longChinese, icu::Locale::getChinese());
  EXPECT_EQ(UnicodeUtils::Compare(longChinese, TestUnicodeUtils::UTF8_LONG_CHINESE), 0);
  UnicodeUtils::FoldCase(longChinese);
  EXPECT_EQ(UnicodeUtils::Compare(longChinese, TestUnicodeUtils::UTF8_LONG_CHINESE), 0);

  std::string longThai{TestUnicodeUtils::UTF8_LONG_THAI};
  UnicodeUtils::ToUpper(longThai, Unicode::GetICULocale("th_TH"));

  const icu::Locale RUSSIAN_LOCALE = Unicode::GetICULocale("ru_RU");
  const std::string longRussian{TestUnicodeUtils::UTF8_LONG_RUSSIAN};
  const std::string lowerRussian{TestUnicodeUtils::UTF8_LONG_RUSSIAN_LOWER_CASE};
  const std::string upperRussian{TestUnicodeUtils::UTF8_LONG_RUSSIAN_UPPER_CASE};

  std::string readWriteString(longRussian);
  UnicodeUtils::ToUpper(readWriteString, RUSSIAN_LOCALE);
  EXPECT_EQ(UnicodeUtils::Compare(readWriteString, upperRussian), 0);
  EXPECT_NE(UnicodeUtils::Compare(readWriteString, longRussian), 0);

  UnicodeUtils::ToLower(readWriteString, RUSSIAN_LOCALE);
  EXPECT_EQ(UnicodeUtils::Compare(readWriteString, lowerRussian), 0);

  readWriteString = {upperRussian};
  UnicodeUtils::ToLower(readWriteString, RUSSIAN_LOCALE);
  EXPECT_EQ(UnicodeUtils::Compare(readWriteString, lowerRussian), 0);
}

//static void ToUpper(std::string& str, const icu::Locale& locale);

TEST(TestUnicodeUtils, ToUpper_Locale)
{
  std::string refstr = "TWITCH";
  std::string varstr = "Twitch";
  UnicodeUtils::ToUpper(varstr, getCLocale());
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "ABCÇDEFGĞH IİI JKLMNOÖPRSŞTUÜVYZ";
  varstr = "abcçdefgğh ıİi jklmnoöprsştuüvyz";
  UnicodeUtils::ToUpper(varstr, getUSEnglishLocale());
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  // refstr = "ABCÇDEFGĞH IİI JKLMNOÖPRSŞTUÜVYZ";
  refstr = "ABCÇDEFGĞH Iİİ JKLMNOÖPRSŞTUÜVYZ";
  varstr = "abcçdefgğh ıİi jklmnoöprsştuüvyz";
  UnicodeUtils::ToUpper(varstr, getTurkicLocale());
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "ABCÇDEFGĞH IİI JKLMNOÖPRSŞTUÜVYZ";
  varstr = "abcçdefgğh ıİi jklmnoöprsştuüvyz";
  UnicodeUtils::ToUpper(varstr, getUkranianLocale());
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = TestUnicodeUtils::UTF8_GERMAN_UPPER; // ÓÓSSCHLOË
  varstr = TestUnicodeUtils::UTF8_GERMAN_SAMPLE; // óóßChloë
  UnicodeUtils::ToUpper(varstr, getCLocale());
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());
  // Lower: óósschloë
}

//static void ToUpper(std::wstring& str);

TEST(TestUnicodeUtils, ToUpper_w)
{
  std::wstring refstr = L"TEST";
  std::wstring varstr = L"TeSt";

  UnicodeUtils::ToUpper(varstr);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = Unicode::UTF8ToWString(std::string(TestUnicodeUtils::UTF8_GERMAN_UPPER)); // ÓÓSSCHLOË
  varstr = Unicode::UTF8ToWString(std::string(TestUnicodeUtils::UTF8_GERMAN_SAMPLE)); // óóßChloë
  UnicodeUtils::ToUpper(varstr);
  int32_t cmp = UnicodeUtils::Compare(refstr, varstr);
  EXPECT_EQ(cmp, 0);
}

//static void ToUpper(std::wstring& str, const std::locale& locale);

//static void ToUpper(std::wstring& str, const icu::Locale& locale);

// static void ToLower(std::string& str);

TEST(TestUnicodeUtils, ToLower)
{
  std::string refstr = "test";
  std::string varstr = "TeSt";

  UnicodeUtils::ToLower(varstr);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  varstr = TestUnicodeUtils::UTF8_GERMAN_UPPER; // ÓÓSSCHLOË
  refstr = TestUnicodeUtils::UTF8_GERMAN_LOWER_SS; // óósschloë // Does not convert SS to ß
  UnicodeUtils::ToLower(varstr);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());
  // Lower: óósschloë

  // ToLower of string with (with sharp-s) should not change it.

  varstr = TestUnicodeUtils::UTF8_GERMAN_SAMPLE; // óóßChloë
  refstr = TestUnicodeUtils::UTF8_GERMAN_LOWER; // óóßChloë
  UnicodeUtils::ToLower(varstr);
  int32_t cmp = UnicodeUtils::Compare(refstr, varstr);
  EXPECT_EQ(cmp, 0);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());
}

//static void ToLower(std::string& str, const std::locale& locale);

//static void ToLower(std::string& str, const icu::Locale& locale);

/*!
 * \brief Converts a wstring to Lower case using LangInfo::GetSystemLocale
 */
// static void ToLower(std::wstring& str);

TEST(TestUnicodeUtils, ToLower_w)
{
  std::wstring refstr = L"test";
  std::wstring varstr = L"TeSt";

  UnicodeUtils::ToLower(varstr);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str()); // Binary compare should work

  varstr = Unicode::UTF8ToWString(std::string(TestUnicodeUtils::UTF8_GERMAN_UPPER)); // ÓÓSSCHLOË
  refstr = Unicode::UTF8ToWString(std::string(TestUnicodeUtils::UTF8_GERMAN_LOWER_SS)); // óóßChloë
  UnicodeUtils::ToLower(varstr);
  int32_t cmp = UnicodeUtils::Compare(refstr, varstr);
  EXPECT_EQ(cmp, 0);

  EXPECT_STREQ(refstr.c_str(), varstr.c_str());
  // Lower: óósschloë

  // ToLower of string with (with sharp-s) should not change it.

  varstr = Unicode::UTF8ToWString(std::string(TestUnicodeUtils::UTF8_GERMAN_SAMPLE)); // óóßChloë
  refstr = Unicode::UTF8ToWString(std::string(TestUnicodeUtils::UTF8_GERMAN_LOWER)); // óóßchloë
  UnicodeUtils::ToLower(varstr);
  cmp = UnicodeUtils::Compare(refstr, varstr);
  EXPECT_EQ(cmp, 0);

  EXPECT_STREQ(refstr.c_str(), varstr.c_str());
}

// static void ToLower(std::wstring& str, const std::locale& locale);
// static void ToLower(std::wstring& str, const icu::Locale& locale);

TEST(TestUnicodeUtils, Turkic_I)
{
  // Specifically test behavior of Turkic I
  // US English
  // First, ToLower

  std::string originalInput = "I i İ ı";

  /*
   * For US English locale behavior of the 4 versions of "I" used in Turkey
   * ToLower yields:
   *
   * I (Dotless I \u0049)        -> i (Dotted small I)                 \u0069
   * i (Dotted small I \u0069)   -> i (Dotted small I)                 \u0069
   * İ (Dotted I) \u0130         -> i̇ (Dotted small I + Combining dot) \u0069 \u0307
   * ı (Dotless small I) \u0131  -> ı (Dotless small I)                \u0131
   */
  std::string varstr = std::string(originalInput); // hex: 0049 0069 0130 0131
  std::string refstr = "iii̇ı"; // hex: 0069 0069 0069 0307 0131
  // Convert to native Unicode, UChar32
  std::string orig = std::string(varstr);
  std::wstring w_varstr_in = Unicode::UTF8ToWString(varstr);
  UnicodeUtils::ToLower(varstr, getUSEnglishLocale());
  std::wstring w_varstr_out = Unicode::UTF8ToWString(varstr);
  std::stringstream ss;
  std::string prefix = "\\u";
  for (const auto& item : w_varstr_in)
  {
    ss << prefix << std::hex << int(item);
    prefix = " \\u";
  }
  std::string hexInput = ss.str();
  ss.clear();
  ss.str(std::string());
  prefix = "\\u";
  for (const auto& item : w_varstr_out)
  {
    ss << prefix << std::hex << int(item);
    prefix = " \\u";
  }
  std::string hexOutput = ss.str();
  ss.clear();
  ss.str(std::string());
  // std::cout << "US English ToLower input: " << orig << " output: " << varstr << " input hex: "
  //    << hexInput << " output hex: " << hexOutput << std::endl;

  /*
   * For US English locale behavior of the 4 versions of "I" used in Turkey
   * ToUpper yields:
   *
   * I (Dotless I)       \u0049 -> I (Dotless I) \u0049
   * i (Dotted small I)  \u0069 -> I (Dotless I) \u0049
   * İ (Dotted I)        \u0130 -> İ (Dotted I)  \u0130
   * ı (Dotless small I) \u0131 -> I (Dotless I) \u0049
   */

  varstr = std::string(originalInput);
  refstr = "IIİI";
  orig = std::string(varstr);
  w_varstr_in = Unicode::UTF8ToWString(varstr);
  UnicodeUtils::ToUpper(varstr, getUSEnglishLocale());
  w_varstr_out = Unicode::UTF8ToWString(varstr);
  ss.clear();
  ss.str(std::string());
  prefix = "\\u";
  for (const auto& item : w_varstr_in)
  {
    ss << prefix << std::hex << int(item);
    prefix = " \\u";
  }
  hexInput = ss.str();
  ss.clear();
  ss.str(std::string());
  prefix = "\\u";
  for (const auto& item : w_varstr_out)
  {
    ss << prefix << std::hex << int(item);
    prefix = " \\u";
  }
  hexOutput = ss.str();
  ss.clear();
  ss.str(std::string());
  // std::cout << "US English ToUpper input: " << orig << " output: " << varstr << " input hex: "
  //    << hexInput << " output hex: " << hexOutput << std::endl;

  /*
   * For Turkish locale behavior of the 4 versions of "I" used in Turkey
   * ToLower yields:
   *
   * I (Dotless I)       \u0049 -> ı (Dotless small I) \u0131
   * i (Dotted small I)  \u0069 -> i (Dotted small I)  \u0069
   * İ (Dotted I)        \u0130 -> i (Dotted small I)  \u0069
   * ı (Dotless small I) \u0131 -> ı (Dotless small I) \u0131
   */

  varstr = std::string(originalInput);
  refstr = "ıiiı";
  // Convert to native Unicode, UChar32
  orig = std::string(varstr);
  w_varstr_in = Unicode::UTF8ToWString(varstr);
  UnicodeUtils::ToLower(varstr, getTurkicLocale());
  w_varstr_out = Unicode::UTF8ToWString(varstr);
  ss.clear();
  ss.str(std::string());
  prefix = "\\u";
  for (const auto& item : w_varstr_in)
  {
    ss << prefix << std::hex << int(item);
    prefix = " \\u";
  }
  hexInput = ss.str();
  ss.clear();
  ss.str(std::string());
  prefix = "\\u";
  for (const auto& item : w_varstr_out)
  {
    ss << prefix << std::hex << int(item);
    prefix = " \\u";
  }
  hexOutput = ss.str();
  ss.clear();
  ss.str(std::string());
  // std::cout << "Turkic ToLower input: " << orig << " output: " << varstr << " input hex: "
  //    << hexInput << " output hex: " << hexOutput << std::endl;
  /*
   * For Turkish locale behavior of the 4 versions of "I" used in Turkey
   * ToUpper yields:
   *
   * I (Dotless I)       \u0049 -> I (Dotless I) \u0049
   * i (Dotted small I)  \u0069 -> İ (Dotted I)  \u0130
   * İ (Dotted I)        \u0130 -> İ (Dotted I)  \u0130
   * ı (Dotless small I) \u0131 -> I (Dotless I) \u0049
   */

  varstr = std::string(originalInput);
  refstr = "IİİI";
  orig = std::string(varstr);
  w_varstr_in = Unicode::UTF8ToWString(varstr);
  UnicodeUtils::ToUpper(varstr, getTurkicLocale());
  w_varstr_out = Unicode::UTF8ToWString(varstr);
  ss.clear();
  ss.str(std::string());
  prefix = "\\u";
  for (const auto& item : w_varstr_in)
  {
    ss << prefix << std::hex << int(item);
    prefix = " \\u";
  }
  hexInput = ss.str();
  ss.clear();
  ss.str(std::string());
  prefix = "";
  for (const auto& item : w_varstr_out)
  {
    ss << prefix << std::hex << int(item);
    prefix = " \\u";
  }
  hexOutput = ss.str();
  // std::cout << "Turkic ToUpper input: " << orig << " output: " << varstr << " input hex: "
  //    << hexInput << " output hex: " << hexOutput << std::endl;
}

/*!
 * Similar to ToLower except in addition, insignificant accents are stripped
 * and other transformations are made (such as German sharp-S is converted to ss).
 * The transformation is independent of locale.
 *
 * In particular, when FOLD_CASE_DEFAULT is used, the Turkic Dotted I and Dotless
 * i follow the "en" locale rules for ToLower.
 *
 * DEVELOPERS who use non-ASCII keywords that will use FoldCase
 * should be aware that it may not always work as expected. Testing is important.
 * Changes will have to be made to keywords that don't work as expected. One solution is
 * to try to always use lower-case in the first place.
 *
 * \param str string to fold in place
 * \param opt StringOptions to fine-tune behavior. For most purposes, leave at
 *            default value, FOLD_CASE_DEFAULT
 *
 * Note: This function serves a similar purpose that "ToLower/ToUpper" is
 *       frequently used for ASCII maps indexed by keyword. ToLower/ToUpper does
 *       NOT work to 'Normalize' Unicode to a consistent value regardless of
 *       the case variations of the input string. A particular problem is the behavior
 *       of "The Turkic I". FOLD_CASE_DEFAULT is effective at
 *       eliminating this problem. Below are the four "I" characters in
 *       Turkic and the result of FoldCase for each:
 *
 * Locale                    Unicode                                      Unicode
 *                           codepoint                                    (hex 32-bit codepoint(s))
 * en_US I (Dotless I)       \u0049 -> i (Dotted small I)                 \u0069
 * tr_TR I (Dotless I)       \u0049 -> ı (Dotless small I)                \u0131
 * FOLD1 I (Dotless I)       \u0049 -> i (Dotted small I)                 \u0069
 * FOLD2 I (Dotless I)       \u0049 -> ı (Dotless small I)                \u0131
 *
 * en_US i (Dotted small I)  \u0069 -> i (Dotted small I)                 \u0069
 * tr_TR i (Dotted small I)  \u0069 -> i (Dotted small I)                 \u0069
 * FOLD1 i (Dotted small I)  \u0069 -> i (Dotted small I)                 \u0069
 * FOLD2 i (Dotted small I)  \u0069 -> i (Dotted small I)                 \u0069
 *
 * en_US İ (Dotted I)        \u0130 -> i̇ (Dotted small I + Combining dot) \u0069 \u0307
 * tr_TR İ (Dotted I)        \u0130 -> i (Dotted small I)                 \u0069
 * FOLD1 İ (Dotted I)        \u0130 -> i̇ (Dotted small I + Combining dot) \u0069 \u0307
 * FOLD2 İ (Dotted I)        \u0130 -> i (Dotted small I)                 \u0069
 *
 * en_US ı (Dotless small I) \u0131 -> ı (Dotless small I)                \u0131
 * tr_TR ı (Dotless small I) \u0131 -> ı (Dotless small I)                \u0131
 * FOLD1 ı (Dotless small I) \u0131 -> ı (Dotless small I)                \u0131
 * FOLD2 ı (Dotless small I) \u0131 -> ı (Dotless small I)                \u0131
 *
 * FOLD_CASE_DEFAULT causes FoldCase to behave similar to ToLower for the "en" locale
 * FOLD_CASE_SPECIAL_I causes FoldCase to behave similar to ToLower for the "tr_TR" locale.
 *
 * Case folding also ignores insignificant differences in strings (some accent marks,
 * etc.).
 */

TEST(TestUnicodeUtils, FoldCase)
{
  /*
   *  FOLD_CASE_DEFAULT
   * I (Dotless I)       \u0049 -> i (Dotted small I)                 \u0069
   * İ (Dotted I)        \u0130 -> i̇ (Dotted small I + Combining dot) \u0069 \u0307
   * i (Dotted small I)  \u0069 -> i (Dotted small I)                 \u0069
   * ı (Dotless small I) \u0131 -> ı (Dotless small I)                \u0131
   *
   * FOLD_CASE_EXCLUDE_SPECIAL_I
   * I (Dotless I)       \u0049 -> ı (Dotless small I) \u0131
   * İ (Dotted I)        \u0130 -> i (Dotted small I)  \u0069
   * i (Dotted small I)  \u0069 -> i (Dotted small I)  \u0069
   * ı (Dotless small I) \u0131 -> ı (Dotless small I) \u0131
   */
  std::string s1 = std::string(TestUnicodeUtils::UTF8_MULTI_CODEPOINT_CHAR1_VARIENT_5);
  std::string s2 = std::string(TestUnicodeUtils::UTF8_MULTI_CODEPOINT_CHAR1_VARIENT_1);

  UnicodeUtils::FoldCase(s1);
  UnicodeUtils::FoldCase(s2);
  int32_t result = UnicodeUtils::Compare(s1, s2);
  EXPECT_NE(result, 0);

  s1 = std::string(TestUnicodeUtils::UTF8_MULTI_CODEPOINT_CHAR1_VARIENT_5);
  s2 = std::string(TestUnicodeUtils::UTF8_MULTI_CODEPOINT_CHAR1_VARIENT_1);
  UnicodeUtils::FoldCase(s1, StringOptions::FOLD_CASE_DEFAULT);
  UnicodeUtils::FoldCase(s2, StringOptions::FOLD_CASE_DEFAULT);
  // td::cout << "Turkic folded s1: " << s1 << std::endl;
  // std::cout << "Turkic folded s2: " << s2 << std::endl;
  result = UnicodeUtils::Compare(s1, s2);
  EXPECT_NE(result, 0);

  s1 = "I İ İ i ı";
  s2 = "i i̇ i̇ i ı";

  UnicodeUtils::FoldCase(s1, StringOptions::FOLD_CASE_DEFAULT);
  UnicodeUtils::FoldCase(s2, StringOptions::FOLD_CASE_DEFAULT);
  // std::cout << "Turkic folded s1: " << s1 << std::endl;
  // std::cout << "Turkic folded s2: " << s2 << std::endl;
  result = UnicodeUtils::Compare(s1, s2);
  EXPECT_EQ(result, 0);

  std::string I = "I";
  std::string I_DOT = "İ";
  std::string I_DOT_I_DOT = "İİ";
  std::string i = "i";
  std::string ii = "ii";
  std::string i_DOTLESS = "ı";
  std::string i_DOTLESS_i_DOTTLESS = "ıı";
  std::string i_COMBINING_DOUBLE_DOT = "i̇";

  s1 = I + I_DOT + i + i_DOTLESS + i_COMBINING_DOUBLE_DOT;
  s2 = i + i_COMBINING_DOUBLE_DOT + i + i_DOTLESS + i_COMBINING_DOUBLE_DOT;

  UnicodeUtils::FoldCase(s1, StringOptions::FOLD_CASE_DEFAULT);
  UnicodeUtils::FoldCase(s2, StringOptions::FOLD_CASE_DEFAULT);
  // std::cout << "Turkic folded s1: " << s1 << std::endl;
  // std::cout << "Turkic folded s2: " << s2 << std::endl;
  result = UnicodeUtils::Compare(s1, s2);
  EXPECT_EQ(result, 0);
  /*
    FOLD_CASE_EXCLUDE_SPECIAL_I
   * I (\u0049) -> ı (\u0131)
   * i (\u0069) -> i (\u0069)
   * İ (\u0130) -> i (\u0069)
   * ı (\u0131) -> ı (\u0131)
   *
   */
  s1 = I + I_DOT + i + i_DOTLESS;
  s2 = i_DOTLESS + i + i + i_DOTLESS;

  UnicodeUtils::FoldCase(s1, StringOptions::FOLD_CASE_EXCLUDE_SPECIAL_I);
  UnicodeUtils::FoldCase(s2, StringOptions::FOLD_CASE_EXCLUDE_SPECIAL_I);
  // std::cout << "Turkic folded s1: " << s1 << std::endl;
  // std::cout << "Turkic folded s2: " << s2 << std::endl;
  result = UnicodeUtils::Compare(s1, s2);
  EXPECT_EQ(result, 0);

  /*
   *  FOLD_CASE_DEFAULT
   * I (\u0049) -> i (\u0069)
   * İ (\u0130) -> i̇ (\u0069 \u0307)
   * i (\u0069) -> i (\u0069)
   * ı (\u0131) -> ı (\u0131)
   */
  s1 = "ABCÇDEFGĞHIJKLMNOÖPRSŞTUÜVYZ";
  s2 = "abcçdefgğhijklmnoöprsştuüvyz";

  // std::cout << "Turkic orig s1: " << s1 << std::endl;
  // std::cout << "Turkic orig s2: " << s2 << std::endl;

  UnicodeUtils::FoldCase(s1, StringOptions::FOLD_CASE_DEFAULT);
  UnicodeUtils::FoldCase(s2, StringOptions::FOLD_CASE_DEFAULT);
  // std::cout << "Turkic folded s1: " << s1 << std::endl;
  // std::cout << "Turkic folded s2: " << s2 << std::endl;

  result = UnicodeUtils::Compare(s1, s2);
  EXPECT_EQ(result, 0);
}

// static void FoldCase(std::wstring& str,
//                      const StringOptions opt = StringOptions::FOLD_CASE_DEFAULT);

TEST(TestUnicodeUtils, FoldCase_W)
{
  std::wstring w_s1 =
      Unicode::UTF8ToWString(std::string(TestUnicodeUtils::UTF8_MULTI_CODEPOINT_CHAR1_VARIENT_5));
  std::wstring w_s2 =
      Unicode::UTF8ToWString(std::string(TestUnicodeUtils::UTF8_MULTI_CODEPOINT_CHAR1_VARIENT_1));

  UnicodeUtils::FoldCase(w_s1);
  UnicodeUtils::FoldCase(w_s2);
  int32_t result = UnicodeUtils::Compare(w_s1, w_s2);
  EXPECT_NE(result, 0);

  w_s1 =
      Unicode::UTF8ToWString(std::string(TestUnicodeUtils::UTF8_MULTI_CODEPOINT_CHAR1_VARIENT_5));
  w_s2 =
      Unicode::UTF8ToWString(std::string(TestUnicodeUtils::UTF8_MULTI_CODEPOINT_CHAR1_VARIENT_1));
  UnicodeUtils::FoldCase(w_s1, StringOptions::FOLD_CASE_DEFAULT);
  UnicodeUtils::FoldCase(w_s2, StringOptions::FOLD_CASE_DEFAULT);
  // td::cout << "Turkic folded s1: " << s1 << std::endl;
  // std::cout << "Turkic folded s2: " << s2 << std::endl;
  result = UnicodeUtils::Compare(w_s1, w_s2);
  EXPECT_NE(result, 0);

  std::string s1 = "I İ İ i ı";
  std::string s2 = "i i̇ i̇ i ı";
  w_s1 = Unicode::UTF8ToWString(s1);
  w_s2 = Unicode::UTF8ToWString(s2);
  UnicodeUtils::FoldCase(w_s1, StringOptions::FOLD_CASE_DEFAULT);
  UnicodeUtils::FoldCase(w_s2, StringOptions::FOLD_CASE_DEFAULT);
  // std::cout << "Turkic folded s1: " << s1 << std::endl;
  // std::cout << "Turkic folded s2: " << s2 << std::endl;
  result = UnicodeUtils::Compare(w_s1, w_s2);
  EXPECT_EQ(result, 0);

  std::string I = "I";
  std::string I_DOT = "İ";
  std::string I_DOT_I_DOT = "İİ";
  std::string i = "i";
  std::string ii = "ii";
  std::string i_DOTLESS = "ı";
  std::string i_DOTLESS_i_DOTTLESS = "ıı";
  std::string i_COMBINING_DOUBLE_DOT = "i̇";

  s1 = I + I_DOT + i + i_DOTLESS + i_COMBINING_DOUBLE_DOT;
  s2 = i + i_COMBINING_DOUBLE_DOT + i + i_DOTLESS + i_COMBINING_DOUBLE_DOT;

  w_s1 = Unicode::UTF8ToWString(s1);
  w_s2 = Unicode::UTF8ToWString(s2);
  UnicodeUtils::FoldCase(w_s1, StringOptions::FOLD_CASE_DEFAULT);
  UnicodeUtils::FoldCase(w_s2, StringOptions::FOLD_CASE_DEFAULT);
  // std::cout << "Turkic folded s1: " << s1 << std::endl;
  // std::cout << "Turkic folded s2: " << s2 << std::endl;
  result = UnicodeUtils::Compare(w_s1, w_s2);

  EXPECT_EQ(result, 0);
  /*
    FOLD_CASE_EXCLUDE_SPECIAL_I
   * I (\u0049) -> ı (\u0131)
   * i (\u0069) -> i (\u0069)
   * İ (\u0130) -> i (\u0069)
   * ı (\u0131) -> ı (\u0131)
   *
   */
  s1 = I + I_DOT + i + i_DOTLESS;
  s2 = i_DOTLESS + i + i + i_DOTLESS;

  w_s1 = Unicode::UTF8ToWString(s1);
  w_s2 = Unicode::UTF8ToWString(s2);
  UnicodeUtils::FoldCase(w_s1, StringOptions::FOLD_CASE_EXCLUDE_SPECIAL_I);
  UnicodeUtils::FoldCase(w_s2, StringOptions::FOLD_CASE_EXCLUDE_SPECIAL_I);
  // std::cout << "Turkic folded s1: " << s1 << std::endl;
  // std::cout << "Turkic folded s2: " << s2 << std::endl;
  result = UnicodeUtils::Compare(w_s1, w_s2);
  EXPECT_EQ(result, 0);

  /*
   *  FOLD_CASE_DEFAULT
   * I (\u0049) -> i (\u0069)
   * İ (\u0130) -> i̇ (\u0069 \u0307)
   * i (\u0069) -> i (\u0069)
   * ı (\u0131) -> ı (\u0131)
   */
  s1 = "ABCÇDEFGĞHIJKLMNOÖPRSŞTUÜVYZ";
  s2 = "abcçdefgğhijklmnoöprsştuüvyz";

  // std::cout << "Turkic orig s1: " << s1 << std::endl;
  // std::cout << "Turkic orig s2: " << s2 << std::endl;

  w_s1 = Unicode::UTF8ToWString(s1);
  w_s2 = Unicode::UTF8ToWString(s2);
  UnicodeUtils::FoldCase(w_s1, StringOptions::FOLD_CASE_DEFAULT);
  UnicodeUtils::FoldCase(w_s2, StringOptions::FOLD_CASE_DEFAULT);
  // std::cout << "Turkic folded s1: " << s1 << std::endl;
  // std::cout << "Turkic folded s2: " << s2 << std::endl;
  result = UnicodeUtils::Compare(w_s1, w_s2);
  EXPECT_EQ(result, 0);
}

//static void ToCapitalize(std::string& str);

TEST(TestUnicodeUtils, ToCapitalize)
{
  std::string refstr = "Test";
  std::string varstr = "test";

  UnicodeUtils::ToCapitalize(varstr);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "Just A Test";
  varstr = "just a test";
  UnicodeUtils::ToCapitalize(varstr);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "Test -1;2:3, String For Case";
  varstr = "test -1;2:3, string for Case";
  UnicodeUtils::ToCapitalize(varstr);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "  JuST Another\t\tTEst:\nWoRKs ";
  varstr = "  juST another\t\ttEst:\nwoRKs ";
  UnicodeUtils::ToCapitalize(varstr);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "N.Y.P.D";
  varstr = "n.y.p.d";
  UnicodeUtils::ToCapitalize(varstr);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "N-Y-P-D";
  varstr = "n-y-p-d";
  UnicodeUtils::ToCapitalize(varstr);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());
}

// static void ToCapitalize(std::wstring& str);

TEST(TestUnicodeUtils, ToCapitalize_w)
{
  std::wstring refstr = L"Test";
  std::wstring varstr = L"test";

  UnicodeUtils::ToCapitalize(varstr);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = L"Just A Test";
  varstr = L"just a test";
  UnicodeUtils::ToCapitalize(varstr);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = L"Test -1;2:3, String For Case";
  varstr = L"test -1;2:3, string for Case";
  UnicodeUtils::ToCapitalize(varstr);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = L"  JuST Another\t\tTEst:\nWoRKs ";
  varstr = L"  juST another\t\ttEst:\nwoRKs ";
  UnicodeUtils::ToCapitalize(varstr);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = L"N.Y.P.D";
  varstr = L"n.y.p.d";
  UnicodeUtils::ToCapitalize(varstr);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = L"N-Y-P-D";
  varstr = L"n-y-p-d";
  UnicodeUtils::ToCapitalize(varstr);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());
}

/*!
 *  \brief TitleCase a string using locale.
 *
 *  Similar too, but more language friendly version of ToCapitalize.
 *  Uses ICU library.
 *
 *  Best results are when a complete sentence/paragraph is TitleCased rather than
 *  individual words.
 *
 *  \param str string to TitleCase
 *  \param locale
 *  \return str in TitleCase
 */
//static std::string TitleCase(const std::string_view& str, const std::locale& locale);
//static std::string TitleCase(const std::string_view& str);

TEST(TestUnicodeUtils, TitleCase)
{
  // Different from ToCapitalize (single word not title cased)

  std::string refstr = "Test";
  std::string varstr = "test";
  std::string result;

  result = UnicodeUtils::TitleCase(varstr);
  EXPECT_STREQ(refstr.c_str(), result.c_str());

  refstr = "Just A Test";
  varstr = "just a test";
  result = UnicodeUtils::TitleCase(varstr);
  EXPECT_STREQ(refstr.c_str(), result.c_str());

  refstr = "Test -1;2:3, String For Case";
  varstr = "test -1;2:3, string for Case";
  result = UnicodeUtils::TitleCase(varstr);
  EXPECT_STREQ(refstr.c_str(), result.c_str());

  refstr = "  Just Another\t\tTest:\nWorks ";
  varstr = "  juST another\t\ttEst:\nwoRKs ";
  result = UnicodeUtils::TitleCase(varstr);
  EXPECT_STREQ(refstr.c_str(), result.c_str());

  refstr = "N.y.p.d";
  varstr = "n.y.p.d";
  result = UnicodeUtils::TitleCase(varstr);
  EXPECT_STREQ(refstr.c_str(), result.c_str());

  refstr = "N-Y-P-D";
  varstr = "n-y-p-d";
  result = UnicodeUtils::TitleCase(varstr);
  EXPECT_STREQ(refstr.c_str(), result.c_str());
}

// static std::wstring TitleCase(const std::wstring_view& str, const std::locale& locale);
// static std::wstring TitleCase(const std::wstring_view& str);

TEST(TestUnicodeUtils, TitleCase_w)
{
  // Different from ToCapitalize (single word not title cased)

  std::wstring refstr = L"Test";
  std::wstring varstr = L"test";
  std::wstring result;

  result = UnicodeUtils::TitleCase(varstr);
  EXPECT_STREQ(refstr.c_str(), result.c_str());

  refstr = L"Just A Test";
  varstr = L"just a test";
  result = UnicodeUtils::TitleCase(varstr);
  EXPECT_STREQ(refstr.c_str(), result.c_str());

  refstr = L"Test -1;2:3, String For Case";
  varstr = L"test -1;2:3, string for Case";
  result = UnicodeUtils::TitleCase(varstr);
  EXPECT_STREQ(refstr.c_str(), result.c_str());

  refstr = L"  Just Another\t\tTest:\nWorks ";
  varstr = L"  juST another\t\ttEst:\nwoRKs ";
  result = UnicodeUtils::TitleCase(varstr);
  EXPECT_STREQ(refstr.c_str(), result.c_str());

  refstr = L"N.y.p.d";
  varstr = L"n.y.p.d";
  result = UnicodeUtils::TitleCase(varstr);
  EXPECT_STREQ(refstr.c_str(), result.c_str());

  refstr = L"N-Y-P-D";
  varstr = L"n-y-p-d";
  result = UnicodeUtils::TitleCase(varstr);
  EXPECT_STREQ(refstr.c_str(), result.c_str());
}

/*!
 *  \brief Determines if two strings are identical in content.
 *
 * Performs a bitwise comparison of the two strings. Locale is
 * not considered.
 *
 * \param str1 one of the strings to compare
 * \param str2 the other string to compare
 * \return true if both strings are identical, otherwise false
 */

//static bool Equals(const std::string_view& str1, const std::string_view& str2);
//static bool Equals(const std::wstring_view& str1, const std::wstring_view& str2);

TEST(TestUnicodeUtils, Equals)
{
  std::string left;
  std::string right;

  left = "TeSt";
  right = "TeSt";

  EXPECT_TRUE(UnicodeUtils::Equals(left, right));

  left = "TeSt";
  right = "TeSt2"; //Longer

  EXPECT_FALSE(UnicodeUtils::Equals(left, right));

  left = "TeSt";
  right = "TeSt\0"; //Longer, but null. Embedded nulls can confuse code

  EXPECT_TRUE(UnicodeUtils::Equals(left, right));

  left = "TeSt";
  right = "TeSt\0"; // Null will terminate.

  EXPECT_TRUE(UnicodeUtils::Equals(left, right));
  EXPECT_TRUE(UnicodeUtils::Equals(left, right, false));
  EXPECT_TRUE(UnicodeUtils::Equals(left, right, true)); // Normalized

  left = "Te\0St";
  right = "Te\0ST";

  EXPECT_TRUE(UnicodeUtils::Equals(left, right)); // Null stops comparison
  EXPECT_TRUE(UnicodeUtils::Equals(left, right, false)); // Null stops comparison
  EXPECT_TRUE(UnicodeUtils::Equals(left, right, true)); // Normalized

  left = "";
  right = "x";

  EXPECT_FALSE(UnicodeUtils::Equals(left, right));
  EXPECT_FALSE(UnicodeUtils::Equals(left, right, true));
  EXPECT_FALSE(UnicodeUtils::Equals(left, right, false));

  left = "\0";
  right = "x";

  EXPECT_FALSE(UnicodeUtils::Equals(left, right));
  EXPECT_FALSE(UnicodeUtils::Equals(left, right, true));
  EXPECT_FALSE(UnicodeUtils::Equals(left, right, false));

  left = "";
  right = "";

  EXPECT_TRUE(UnicodeUtils::Equals(left, right));
  EXPECT_TRUE(UnicodeUtils::Equals(left, right, true)); // Normalized

  left = "\0";
  right = "\0";

  EXPECT_TRUE(UnicodeUtils::Equals(left, right));
  EXPECT_TRUE(UnicodeUtils::Equals(left, right, true)); // Normalized

  left = std::string(TestUnicodeUtils::UTF8_MULTI_CODEPOINT_CHAR1_VARIENT_5);
  right = std::string(TestUnicodeUtils::UTF8_MULTI_CODEPOINT_CHAR1_VARIENT_1);
  // Both strings ARE equivalent, but you have to normalize them first.
  EXPECT_FALSE(UnicodeUtils::Equals(left, right));

  EXPECT_TRUE(UnicodeUtils::Equals(left, right, true)); // Normalized
}

TEST(TestUnicodeUtils, Equals_w)
{
  std::wstring left;
  std::wstring right;

  left = L"TeSt";
  right = L"TeSt";
  EXPECT_TRUE(UnicodeUtils::Equals(left, right));
  EXPECT_TRUE(UnicodeUtils::Equals(left, right, true));
  EXPECT_TRUE(UnicodeUtils::Equals(left, right, false));

  left = L"TeSt";
  right = L"teSt";
  EXPECT_FALSE(UnicodeUtils::Equals(left, right));
  EXPECT_FALSE(UnicodeUtils::Equals(left, right, true));
  EXPECT_FALSE(UnicodeUtils::Equals(left, right, false));

  left = L"TeSt";
  right = L"TeSt2"; //Longer

  EXPECT_FALSE(UnicodeUtils::Equals(left, right));

  left = L"TeSt";
  right = L"TeSt\0"; //Longer, but null. (Compiler strips it)

  EXPECT_TRUE(UnicodeUtils::Equals(left, right));

  left = L"TeSt";
  right = L"TeST\0";

  EXPECT_FALSE(UnicodeUtils::Equals(left, right));
  EXPECT_FALSE(UnicodeUtils::Equals(left, right, false));
  EXPECT_FALSE(UnicodeUtils::Equals(left, right, true)); // Normalized

  left = L"Te\0St"; // Compiler strips everything beginning with null
  right = L"Te\0ST";

  EXPECT_TRUE(UnicodeUtils::Equals(left, right)); // Null stops comparison
  EXPECT_TRUE(UnicodeUtils::Equals(left, right, false)); // Null stops comparison
  EXPECT_TRUE(UnicodeUtils::Equals(left, right, true)); // Normalized

  left = L"\0TeSt";
  right = L"\0TeST";

  EXPECT_TRUE(UnicodeUtils::Equals(left, right)); // Null stops comparison
  EXPECT_TRUE(UnicodeUtils::Equals(left, right, false)); // Null stops comparison
  EXPECT_TRUE(UnicodeUtils::Equals(left, right, true)); // Normalized

  left = L"\0";
  right = L"x";

  EXPECT_FALSE(UnicodeUtils::Equals(left, right));
  EXPECT_FALSE(UnicodeUtils::Equals(left, right, true));
  EXPECT_FALSE(UnicodeUtils::Equals(left, right, false));

  left = L"";
  right = L"x";

  EXPECT_FALSE(UnicodeUtils::Equals(left, right));
  EXPECT_FALSE(UnicodeUtils::Equals(left, right, true));
  EXPECT_FALSE(UnicodeUtils::Equals(left, right, false));

  left = L"";
  right = L"";

  EXPECT_TRUE(UnicodeUtils::Equals(left, right));
  EXPECT_TRUE(UnicodeUtils::Equals(left, right, true)); // Normalized

  left = L"\0";
  right = L"\0";

  EXPECT_TRUE(UnicodeUtils::Equals(left, right));
  EXPECT_TRUE(UnicodeUtils::Equals(left, right, true)); // Normalized

  left = Unicode::UTF8ToWString(std::string(TestUnicodeUtils::UTF8_MULTI_CODEPOINT_CHAR1_VARIENT_5));
  right = Unicode::UTF8ToWString(std::string(TestUnicodeUtils::UTF8_MULTI_CODEPOINT_CHAR1_VARIENT_1));
  // Both strings ARE equivalent, but you have to normalize them first.
  EXPECT_FALSE(UnicodeUtils::Equals(left, right));
  EXPECT_FALSE(UnicodeUtils::Equals(left, right, false));

  EXPECT_TRUE(UnicodeUtils::Equals(left, right, true)); // Normalized
}

/*!
 * \brief Determines if two strings are the same, after case folding each.
 *
 * Logically equivalent to Equals(FoldCase(str1, opt)), FoldCase(str2, opt))
 * or, if Normalize == true: Equals(NFD(FoldCase(NFD(str1))), NFD(FoldCase(NFD(str2))))
 * (NFD is a type of normalization)
 *
 * Note: When normalization = true, the string comparison is done incrementally
 * as the strings are Normalized and folded. Otherwise, case folding is applied
 * to the entire string first.
 *
 * Note In most cases normalization should not be required, using Normalize
 * should yield better results for those cases where it is required.
 *
 * \param str1 one of the strings to compare
 * \param str2 one of the strings to compare
 * \param opt StringOptions to apply. Generally leave at default value.
 * \param Normalize Controls whether normalization is performed before and after
 *        case folding
 * \return true if both strings compare after case folding, otherwise false
 */
//static bool EqualsNoCase(const std::string_view& str1,
//                         const std::string_view& str2,
//                         const StringOptions opt = StringOptions::FOLD_CASE_DEFAULT,
//                         const bool Normalize = false);

TEST(TestUnicodeUtils, EqualsNoCase)
{
  std::string refstr = "TeSt";

  EXPECT_TRUE(UnicodeUtils::EqualsNoCase(refstr, "TeSt"));
  EXPECT_TRUE(UnicodeUtils::EqualsNoCase(refstr, "tEsT"));
}

TEST(TestUnicodeUtils, EqualsNoCase_Normalize)
{
  const std::string refstr = TestUnicodeUtils::UTF8_MULTI_CODEPOINT_CHAR1_VARIENT_5;
  const std::string varstr = std::string(TestUnicodeUtils::UTF8_MULTI_CODEPOINT_CHAR1_VARIENT_1);
  EXPECT_FALSE(UnicodeUtils::EqualsNoCase(refstr, varstr));
  EXPECT_TRUE(UnicodeUtils::EqualsNoCase(refstr, varstr, StringOptions::FOLD_CASE_DEFAULT, true));
}

/*!
 * \brief Compares two wstrings using codepoint order. Locale does not matter.
 *
 * DO NOT USE for collation
 *
 * \param str1 one of the strings to compare
 * \param str2 one of the strings to compare
 * \return <0 or 0 or >0 as usual for string comparisons
 */
// static int Compare(const std::wstring_view& str1, const std::wstring_view& str2);

/*!
 * \brief Compares two strings using codepoint order. Locale does not matter.
 *
 * DO NOT USE for collation
 * \param str1 one of the strings to compare
 * \param str2 one of the strings to compare
 * \return <0 or 0 or >0 as usual for string comparisons
 */
//  static int Compare(const std::string_view& str1, const std::string_view& str2);

TEST(TestUnicodeUtils, Compare)
{

  std::string left;
  std::string right;
  int expectedResult;

  left = "abciI123ABC ";
  right = "ABCIi123abc ";
  expectedResult = 0;
  EXPECT_NE(UnicodeUtils::Compare(left, right), expectedResult);

  left = TestUnicodeUtils::UTF8_GERMAN_UPPER; // ÓÓSSCHLOË
  right = TestUnicodeUtils::UTF8_GERMAN_SAMPLE; // óóßChloë
  expectedResult = 0;
  EXPECT_NE(UnicodeUtils::Compare(left, right), expectedResult);

  left = TestUnicodeUtils::UTF8_GERMAN_UPPER; // ÓÓSSCHLOË
  right = TestUnicodeUtils::UTF8_GERMAN_SAMPLE; // óóßChloë
  expectedResult = 0;
  EXPECT_NE(UnicodeUtils::Compare(left, right), expectedResult);

  left = TestUnicodeUtils::UTF8_GERMAN_UPPER; // ÓÓSSCHLOË
  right = TestUnicodeUtils::UTF8_GERMAN_LOWER_SS; // óósschloë // Does not convert SS to ß
  expectedResult = 0;
  EXPECT_NE(UnicodeUtils::Compare(left, right), expectedResult);
}

/*!
 * \brief Performs a bit-wise comparison of two wstrings, after case folding each.
 *
 * Logically equivalent to Compare(FoldCase(str1, opt)), FoldCase(str2, opt))
 * or, if Normalize == true: Compare(NFD(FoldCase(NFD(str1))), NFD(FoldCase(NFD(str2))))
 * (NFD is a type of normalization)
 *
 * Note: When normalization = true, the string comparison is done incrementally
 * as the strings are Normalized and folded. Otherwise, case folding is applied
 * to the entire string first.
 *
 * Note In most cases normalization should not be required, using Normalize
 * may yield better results.
 *
 * \param str1 one of the wstrings to compare
 * \param str2 one of the wstrings to compare
 * \param opt StringOptions to apply. Generally leave at the default value
 * \param Normalize Controls whether normalization is performed before and after
 *        case folding
 * \return The result of bitwise character comparison:
 * < 0 if the characters str1 are bitwise less than the characters in str2,
 * = 0 if str1 contains the same characters as str2,
 * > 0 if the characters in str1 are bitwise greater than the characters in str2.
 */
/*
 static int CompareNoCase(const std::wstring_view& str1,
                          const std::wstring_view& str2,
                          StringOptions opt = StringOptions::FOLD_CASE_DEFAULT,
                          const bool Normalize = false);
 */
/*!
 * \brief Performs a bit-wise comparison of two strings, after case folding each.
 *
 * Logically equivalent to Compare(FoldCase(str1, opt)), FoldCase(str2, opt))
 * or, if Normalize == true: Compare(NFD(FoldCase(NFD(str1))), NFD(FoldCase(NFD(str2))))
 * (NFD is a type of normalization)
 *
 * Note: When normalization = true, the string comparison is done incrementally
 * as the strings are Normalized and folded. Otherwise, case folding is applied
 * to the entire string first.
 *
 * Note In most cases normalization should not be required, using Normalize
 * may yield better results.
 *
 * \param str1 one of the strings to compare
 * \param str2 one of the strings to compare
 * \param opt StringOptions to apply. Generally leave at the default value
 * \param Normalize Controls whether normalization is performed before and after
 *        case folding
 * \return The result of bitwise character comparison:
 * < 0 if the characters str1 are bitwise less than the characters in str2,
 * = 0 if str1 contains the same characters as str2,
 * > 0 if the characters in str1 are bitwise greater than the characters in str2.
 */
/*
 static int CompareNoCase(const std::string_view& str1,
                          const std::string_view& str2,
                          StringOptions opt = StringOptions::FOLD_CASE_DEFAULT,
                          const bool Normalize = false);
 */
/*!
 * \brief Performs a bit-wise comparison of two strings, after case folding each.
 *
 * Logically equivalent to Compare(FoldCase(s1, opt)), FoldCase(s2, opt))
 * or, if Normalize == true: Compare(NFD(FoldCase(NFD(s1))), NFD(FoldCase(NFD(s2))))
 * (NFD is a type of normalization)
 *
 * Note: When normalization = true, the string comparison is done incrementally
 * as the strings are Normalized and folded. Otherwise, case folding is applied
 * to the entire string first.
 *
 * Note In most cases normalization should not be required, using Normalize
 * may yield better results.
 *
 * \param s1 one of the strings to compare
 * \param s2 one of the strings to compare
 * \param opt StringOptions to apply. Generally leave at the default value
 * \param Normalize Controls whether normalization is performed before and after
 *        case folding
 * \return The result of bitwise character comparison:
 * < 0 if the characters s1 are bitwise less than the characters in s2,
 * = 0 if s1 contains the same characters as s2,
 * > 0 if the characters in s1 are bitwise greater than the characters in s2.
 */
/*
 static int CompareNoCase(const char* s1,
                          const char* s2,
                          StringOptions opt = StringOptions::FOLD_CASE_DEFAULT,
                          const bool Normalize = false);
 */
/*!
 * \brief Performs a bit-wise comparison of two strings, after case folding each.
 *
 * Logically equivalent to Compare(FoldCase(str1, opt)), FoldCase(str2, opt))
 * or, if Normalize == true: Compare(NFD(FoldCase(NFD(str1))), NFD(FoldCase(NFD(str2))))
 * (NFD is a type of normalization)
 *
 * Note: Use of the byte-length argument n is STRONGLY discouraged since
 * it can easily result in malformed Unicode. Further, byte-length does not
 * correlate to character length in multi-byte languages.
 *
 * Note: When normalization = true, the string comparison is done incrementally
 * as the strings are Normalized and folded. Otherwise, case folding is applied
 * to the entire string first.
 *
 * Note In most cases normalization should not be required, using Normalize
 * may yield better results.
 *
 * \param str1 one of the strings to compare
 * \param str2 one of the strings to compare
 * \param n maximum number of bytes to compare. A value of 0 means no limit
 * \param opt StringOptions to apply. Generally leave at the default value
 * \param Normalize Controls whether normalization is performed before and after
 *        case folding
 * \return The result of bitwise character comparison:
 * < 0 if the characters str1 are bitwise less than the characters in str2,
 * = 0 if str1 contains the same characters as str2,
 * > 0 if the characters in str1 are bitwise greater than the characters in str2.
 */
/*
 [[deprecated("StartsWith/EndsWith may be better choices. Multibyte characters, case folding and "
              "byte lengths don't mix.")]] static int
 CompareNoCase(const std::string_view& str1,
               const std::string_view& str2,
               size_t n,
               StringOptions opt = StringOptions::FOLD_CASE_DEFAULT,
               const bool Normalize = false);
 */
/*!
 * \brief Performs a bit-wise comparison of two strings, after case folding each.
 *
 * Logically equivalent to Compare(FoldCase(s1, opt)), FoldCase(s2, opt))
 * or, if Normalize == true: Compare(NFD(FoldCase(NFD(s1))), NFD(FoldCase(NFD(s2))))
 * (NFD is a type of normalization)
 *
 * NOTE: Limiting the number of bytes to compare via the option n may produce
 *       unexpected results for multi-byte characters.
 *
 * Note: When normalization = true, the string comparison is done incrementally
 * as the strings are Normalized and folded. Otherwise, case folding is applied
 * to the entire string first.
 *
 * Note In most cases normalization should not be required, using Normalize
 * may yield better results.
 *
 * \param s1 one of the strings to compare
 * \param s2 one of the strings to compare
 * \param n maximum number of bytes to compare.  A value of 0 means no limit
 * \param opt StringOptions to apply. Generally leave at the default value
 * \param Normalize Controls whether normalization is performed before and after
 *        case folding
 * \return The result of bitwise character comparison:
 * < 0 if the characters s1 are bitwise less than the characters in s2,
 * = 0 if s1 contains the same characters as s2,
 * > 0 if the characters in s1 are bitwise greater than the characters in s2.
 */
/*
 [[deprecated("StartsWith/EndsWith may be better choices. Multibyte characters, case folding and "
              "byte lengths don't mix.")]] static int
 CompareNoCase(const char* s1,
               const char* s2,
               size_t n,
               StringOptions opt = StringOptions::FOLD_CASE_DEFAULT,
               const bool Normalize = false);
 */
TEST(TestUnicodeUtils, CompareNoCase)
{

  std::string left;
  std::string right;
  int expectedResult;

  left = "abciI123ABC ";
  right = "ABCIi123abc ";
  expectedResult = 0;
  EXPECT_EQ(UnicodeUtils::CompareNoCase(left, right), expectedResult);

  left = TestUnicodeUtils::UTF8_GERMAN_UPPER; // ÓÓSSCHLOË
  right = TestUnicodeUtils::UTF8_GERMAN_SAMPLE; // óóßChloë
  expectedResult = 0;
  EXPECT_EQ(UnicodeUtils::CompareNoCase(left, right), expectedResult);

  left = TestUnicodeUtils::UTF8_GERMAN_UPPER; // ÓÓSSCHLOË
  right = TestUnicodeUtils::UTF8_GERMAN_SAMPLE; // óóßChloë
  expectedResult = 0;
  EXPECT_EQ(UnicodeUtils::CompareNoCase(left, right), expectedResult);

  left = TestUnicodeUtils::UTF8_GERMAN_UPPER; // ÓÓSSCHLOË
  right = TestUnicodeUtils::UTF8_GERMAN_LOWER_SS; // óósschloë // Does not convert SS to ß
  expectedResult = 0;
  EXPECT_EQ(UnicodeUtils::CompareNoCase(left, right), expectedResult);
}

TEST(TestUnicodeUtils, CompareNoCase_Advanced)
{

  std::string_view left;
  std::string_view right;
  bool normalize;
  StringOptions opt = StringOptions::FOLD_CASE_DEFAULT;
  int expectedResult;

  normalize = false;
  left = {"abciI123ABC "};
  right = {"ABCIi123abc "};
  expectedResult = 0;
  EXPECT_EQ(UnicodeUtils::CompareNoCase(left, right, opt, normalize), expectedResult);

  left = {"abciI123ABC ", 5};
  right = {"ABCIi123abc ", 5};
  EXPECT_EQ(UnicodeUtils::CompareNoCase(left, right, opt, normalize), expectedResult);

  left = {"abciI123ABC ", 5};
  right = {"ABCIi ", 5};
  EXPECT_EQ(UnicodeUtils::CompareNoCase(left, right, opt, normalize), expectedResult);

  left = {"abciI123ABC ", 6};
  right = {"ABCIi ", 6};
  EXPECT_NE(UnicodeUtils::CompareNoCase(left, right, opt, normalize), expectedResult);

  left = {"abciI123ABC ", 4};
  right = {"ABCIi ", 4};
  EXPECT_EQ(UnicodeUtils::CompareNoCase(left, right, opt, normalize), expectedResult);

  // Make left shorter this time.

  normalize = false;
  left = {"abciI123ABC "};
  right = {"ABCIi123abc "};
  expectedResult = 0;
  EXPECT_EQ(UnicodeUtils::CompareNoCase(left, right, opt, normalize), expectedResult);

  left = {"abciI123ABC ", 5};
  right = {"ABCIi123abc ", 5};
  EXPECT_EQ(UnicodeUtils::CompareNoCase(left, right, opt, normalize), expectedResult);

  left = {"abciI ", 5};
  right = {"ABCIi123abc ", 5};
  EXPECT_EQ(UnicodeUtils::CompareNoCase(left, right, opt, normalize), expectedResult);

  left = {"abciI ", 6}; // length longer than string
  right = {"ABCIi123abc ", 6};
  EXPECT_NE(UnicodeUtils::CompareNoCase(left, right, opt, normalize), expectedResult);

  left = {"abciI ", 4};
  right = {"ABCIi123abc ", 4};
  EXPECT_EQ(UnicodeUtils::CompareNoCase(left, right, opt, normalize), expectedResult);

  // Easy tests

  left = TestUnicodeUtils::UTF8_GERMAN_UPPER; // ÓÓSSCHLOË
  right = TestUnicodeUtils::UTF8_GERMAN_SAMPLE; // óóßChloë
  expectedResult = 0;
  EXPECT_EQ(UnicodeUtils::CompareNoCase(left, right), expectedResult);

  left = TestUnicodeUtils::UTF8_GERMAN_UPPER; // ÓÓSSCHLOË
  right = TestUnicodeUtils::UTF8_GERMAN_SAMPLE; // óóßChloë
  expectedResult = 0;
  EXPECT_EQ(UnicodeUtils::CompareNoCase(left, right), expectedResult);

  left = TestUnicodeUtils::UTF8_GERMAN_UPPER; // ÓÓSSCHLOË
  right = TestUnicodeUtils::UTF8_GERMAN_LOWER_SS; // óósschloë // Does not convert SS to ß
  expectedResult = 0;
  EXPECT_EQ(UnicodeUtils::CompareNoCase(left, right), expectedResult);

  // More interesting guessing the byte length for multibyte characters, eh?
  // These are ALL foldcase equivalent. Curious that the byte lengths are same
  // but character counts are not.

  //
  // const char UTF8_GERMAN_SAMPLE[] =
  //      ó      ó       ß        C   h   l   o   ë
  // { "\xc3\xb3\xc3\xb3\xc3\x9f\x43\x68\x6c\x6f\xc3\xab" };
  //     1       2       3        4   5   6   7   8          character count
  //     1   2   3   4   5   6   7   8   9   10   11  13     byte count
  //
  // const char* UTF8_GERMAN_UPPER =
  //  u"  Ó       Ó       S   S   C   H   L   O  Ë";
  // { "\xc3\x93\xc3\x93\x53\x53\x43\x48\x4c\x4f\xc3\x8b" };
  //     1       2       3    4   5  6   7   8   9           character count
  //     1   2   3   4   5   6   7   8   9   10   11  13     byte count  //
  // const char UTF8_GERMAN_LOWER_SS =
  //     ó        ó       s   s   c   h   l   o   ë"
  // { "\xc3\xb3\xc3\xb3\x73\x73\x63\x68\x6c\x6f\xc3\xab" };
  //     1        2       3   4   5   6   7   8   9
  //     1   2   3   4   5   6   7   8   9   10   11  13     byte count
  // const char* UTF8_GERMAN_LOWER =
  //   u"ó       ó       ß       C   h   l   o   ë         // "ß" becomes "ss" during fold
  // { "\xc3\xb3\xc3\xb3\xc3\x9f\x43\x68\x6c\x6f\xc3\xab" };
  //     1       2       3       4    5  6   7   8           character count
  //     1   2   3   4   5   6   7   8   9   10   11  13     byte count

  left = TestUnicodeUtils::UTF8_GERMAN_UPPER; // ÓÓSSCHLOË
  right = TestUnicodeUtils::UTF8_GERMAN_SAMPLE; // óóßChloë
  expectedResult = 0;
  EXPECT_EQ(UnicodeUtils::CompareNoCase(left, right), expectedResult);

  left = {TestUnicodeUtils::UTF8_GERMAN_UPPER, 4}; // ÓÓSSCHLOË
  right = {TestUnicodeUtils::UTF8_GERMAN_SAMPLE, 4}; // óóßChloë
  // and for lower case version
  expectedResult = 0;
  EXPECT_EQ(UnicodeUtils::CompareNoCase(left, right), expectedResult);

  left = {TestUnicodeUtils::UTF8_GERMAN_UPPER, 3}; // ÓÓSSCHLOË
  right ={ TestUnicodeUtils::UTF8_GERMAN_SAMPLE, 3}; // óóßChloë
  // both strings and lower case version
  expectedResult = 0;
  EXPECT_EQ(UnicodeUtils::CompareNoCase(left, right), expectedResult);

  // sharp-s "ß" and after folding would be the second s, if all other
  // characters were single byte, but it may still work out, I'm not sure
  // how the other characters fold.
  // RESUME
  left = {TestUnicodeUtils::UTF8_GERMAN_UPPER, 6}; // ÓÓSSCHLOË
  right = {TestUnicodeUtils::UTF8_GERMAN_SAMPLE, 6}; // óóßChloë
  expectedResult = 0;
  EXPECT_EQ(UnicodeUtils::CompareNoCase(left, right), expectedResult);

  left = {TestUnicodeUtils::UTF8_GERMAN_UPPER}; // ÓÓSSCHLOË
  right = {TestUnicodeUtils::UTF8_GERMAN_SAMPLE}; // óóßChloë
  expectedResult = 0;
  EXPECT_EQ(UnicodeUtils::CompareNoCase(left, right), expectedResult);

  left = TestUnicodeUtils::UTF8_GERMAN_UPPER; // ÓÓSSCHLOË
  right = TestUnicodeUtils::UTF8_GERMAN_LOWER_SS; // óósschloë // Does not convert SS to ß
  expectedResult = 0;
  EXPECT_EQ(UnicodeUtils::CompareNoCase(left, right), expectedResult);

  left = {TestUnicodeUtils::UTF8_MULTI_CODEPOINT_CHAR1_VARIENT_1}; // 6 bytes
  right = {TestUnicodeUtils::UTF8_MULTI_CODEPOINT_CHAR1_VARIENT_5}; // 4 bytes
  expectedResult = 0;
  EXPECT_NE(UnicodeUtils::CompareNoCase(left, right), expectedResult);

  // Boundary Tests
  // length of 0 means no limit

  normalize = false;
  left = {"abciI123ABC "};
  right = {"ABCIi123abc "};
  expectedResult = 0;
  EXPECT_EQ(UnicodeUtils::CompareNoCase(left, right, opt, normalize), expectedResult);

  normalize = false;
  left = {""};
  right = {"ABCIi123abc "};
  expectedResult = -1;
  EXPECT_EQ(UnicodeUtils::CompareNoCase(left, right, opt, normalize), expectedResult);

  normalize = false;
  left = {"abciI123ABC "};
  right = {""};
  expectedResult = 1;
  EXPECT_EQ(UnicodeUtils::CompareNoCase(left, right, opt, normalize), expectedResult);

  normalize = false;
  left = "abciI123ABC ";
  right = "";
  expectedResult = 1;
  EXPECT_EQ(UnicodeUtils::CompareNoCase(left, right, opt, normalize), expectedResult);

  normalize = false;
  left = "";
  right = "";
  expectedResult = 0;
  EXPECT_EQ(UnicodeUtils::CompareNoCase(left, right, opt, normalize), expectedResult);

  normalize = false;
  left = "";
  right = "";
  expectedResult = 0;
  EXPECT_EQ(UnicodeUtils::CompareNoCase(left, right, opt, normalize), expectedResult);
}

/*!
 * Note: Embedded nulls in str1 or str2 behaves as a null-terminated string behaves
 */
// static bool StartsWith(const std::string_view& str1, const std::string_view& str2);

/*!
 *  Note: Embedded nulls in str1 or str2 behaves as a null-terminated string behaves
 */
// static bool StartsWith(const std::string_view& str1, const char* s2);

/*!
 *  Note: Embedded nulls in str1 or str2 behaves as a null-terminated string behaves
 */
//static bool StartsWith(const char* s1, const char* s2);

TEST(TestUnicodeUtils, StartsWith)
{
  std::string refstr = "test";
  std::string input;
  std::string_view  p;

  EXPECT_FALSE(UnicodeUtils::StartsWith(refstr, "x"));
  EXPECT_TRUE(UnicodeUtils::StartsWith(refstr, "te"));
  EXPECT_TRUE(UnicodeUtils::StartsWith(refstr, "test"));
  EXPECT_FALSE(UnicodeUtils::StartsWith(refstr, "Te"));
  EXPECT_FALSE(UnicodeUtils::StartsWith(refstr, "test "));
  EXPECT_TRUE(UnicodeUtils::StartsWith(refstr, "test\0")); // Embedded null terminates string

  p = {"tes"};
  EXPECT_TRUE(UnicodeUtils::StartsWith(refstr, p));

  // Boundary

  input = "";
  EXPECT_TRUE(UnicodeUtils::StartsWith(input, ""));
  EXPECT_FALSE(UnicodeUtils::StartsWith(input, "Four score and seven years ago"));

  EXPECT_TRUE(UnicodeUtils::StartsWith(refstr, ""));
  // EXPECT_FALSE(UnicodeUtils::StartsWith(refstr, nullptr)); // Blows up
  // EXPECT_FALSE(UnicodeUtils::StartsWith(refstr, NULL));    // Blows up

  input = "";
  p = "";
  EXPECT_TRUE(UnicodeUtils::StartsWith(input, p));
  EXPECT_TRUE(UnicodeUtils::StartsWith(refstr, p));
}

/*
 static bool StartsWithNoCase(const std::string_view& str1,
                              const std::string_view& str2,
                              StringOptions opt = StringOptions::FOLD_CASE_DEFAULT);
 */

/*
 static bool StartsWithNoCase(const std::string_view& str1,
                              const char* s2,
                              StringOptions opt = StringOptions::FOLD_CASE_DEFAULT);
 */

/*
 static bool StartsWithNoCase(const char* s1,
                              const char* s2,
                              StringOptions opt = StringOptions::FOLD_CASE_DEFAULT);
 */

TEST(TestUnicodeUtils, StartsWithNoCase)
{
  std::string refstr = "test";
  std::string input;
  std::string_view p;

  EXPECT_FALSE(UnicodeUtils::StartsWithNoCase(refstr, "x"));
  EXPECT_TRUE(UnicodeUtils::StartsWithNoCase(refstr, "Te"));
  EXPECT_TRUE(UnicodeUtils::StartsWithNoCase(refstr, "TesT"));
  EXPECT_FALSE(UnicodeUtils::StartsWithNoCase(refstr, "Te st"));
  EXPECT_FALSE(UnicodeUtils::StartsWithNoCase(refstr, "test "));
  EXPECT_TRUE(UnicodeUtils::StartsWithNoCase(
      refstr, "test\0")); // Embedded null terminates string operation

  p = "tEs";
  EXPECT_TRUE(UnicodeUtils::StartsWithNoCase(refstr, p));

  // Boundary

  EXPECT_FALSE(UnicodeUtils::StartsWithNoCase(refstr, ""));
  // EXPECT_FALSE(UnicodeUtils::StartsWithNoCase(refstr, nullptr)); // Blows up
  // EXPECT_FALSE(UnicodeUtils::StartsWithNoCase(refstr, NULL));    // Blows up

  p = "";
  // TODO: Verify Non-empty string  doesn't begin with empty string.
  EXPECT_FALSE(UnicodeUtils::StartsWithNoCase(refstr, p));
  // Same behavior with char * and string
  EXPECT_FALSE(UnicodeUtils::StartsWithNoCase(refstr, ""));

  input = "";
  // TODO: Empty string does begin with empty string
  EXPECT_TRUE(UnicodeUtils::StartsWithNoCase(input, ""));

  // Blows up with null pointers

  // EXPECT_FALSE(UnicodeUtils::StartsWithNoCase(refstr, nullptr));
  // EXPECT_FALSE(UnicodeUtils::StartsWithNoCase(refstr, NULL));

  EXPECT_FALSE(UnicodeUtils::StartsWithNoCase(input, "Four score and seven years ago"));

  input = "";
  p = "";
  EXPECT_TRUE(UnicodeUtils::StartsWithNoCase(input, p));
}

/*!
 * Note: Embedded nulls in str1 or str2 behaves as a null-terminated string behaves
 */
//  static bool EndsWith(const std::string_view& str1, const std::string_view& str2);

/*!
 * Note: Embedded nulls in str1 or str2 behaves as a null-terminated string behaves
 */
//  static bool EndsWith(const std::string_view& str1, const char* s2);

TEST(TestUnicodeUtils, EndsWith)
{
  std::string refstr = "test";
  EXPECT_TRUE(UnicodeUtils::EndsWith(refstr, "st"));
  EXPECT_TRUE(UnicodeUtils::EndsWith(refstr, "test"));
  EXPECT_FALSE(UnicodeUtils::EndsWith(refstr, "sT"));
}

/*
static bool EndsWithNoCase(const std::string_view& str1,
                            const std::string_view& str2,
                            StringOptions opt = StringOptions::FOLD_CASE_DEFAULT);
 */
/*
 static bool EndsWithNoCase(const std::string_view& str1,
                            const char* s2,
                            StringOptions opt = StringOptions::FOLD_CASE_DEFAULT);
 */
TEST(TestUnicodeUtils, EndsWithNoCase)
{
  // TODO: Test with FoldCase option

  std::string refstr = "test";
  EXPECT_FALSE(UnicodeUtils::EndsWithNoCase(refstr, "x"));
  EXPECT_TRUE(UnicodeUtils::EndsWithNoCase(refstr, "sT"));
  EXPECT_TRUE(UnicodeUtils::EndsWithNoCase(refstr, "TesT"));
}

/*!
 * \brief Get the leftmost side of a UTF-8 string, using the character
 *        boundary rules from the current icu Locale.
 *
 * Unicode characters are of variable length. This function's
 * parameters are based on characters and NOT bytes. Byte-length can change during
 * processing (from normalization).
 *
 * \param str to get a substring of
 * \param charCount if keepLeft: charCount is number of characters to
 *                  copy from left end (limited by str length)
 *                  if ! keepLeft: number of characters to omit from right end
 * \param keepLeft controls how charCount is interpreted
 * \return leftmost characters of string, length determined by charCount
 *
 * Ex: Copy all but the rightmost two characters from str:
 *
 * std::string x = Left(str, 2, false);
 */
/*
static std::string Left(const std::string_view& str,
                        const size_t charCount,
                        const bool keepLeft = true);
 */
/*!
 * \brief Get the leftmost side of a UTF-8 string, using character boundary
 * rules defined by the given locale.
 *
 * Unicode characters are of variable length. This function's
 * parameters are based on characters and NOT bytes. Byte-length can change during
 * processing (from normalization).
 *
 * \param str to get a substring of
 * \param charCount if keepLeft: charCount is number of characters to
 *                  copy from left end (limited by str length)
 *                  if ! keepLeft: number of characters to omit from right end
 * \param icuLocale determines where the character breaks are
 * \param keepLeft controls how charCount is interpreted
 * \return leftmost characters of string, length determined by charCount
 *
 * Ex: Copy all but the rightmost two characters from str:
 *
 * std::string x = Left(str, 2, false, Unicode::GetDefaultICULocale());
 */
/*
static std::string Left(const std::string_view& str,
                        const size_t charCount,
                        const icu::Locale& icuLocale,
                        const bool keepLeft = true);
 */
TEST(TestUnicodeUtils, Left_Basic)
{
  std::string refstr;
  std::string varstr;
  std::string origstr = "Test";

  // First, request n chars to copy from left end

  refstr = "";
  varstr = UnicodeUtils::Left(origstr, 0);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "T";
  varstr = UnicodeUtils::Left(origstr, 1);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "Te";
  varstr = UnicodeUtils::Left(origstr, 2);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "Test";
  varstr = UnicodeUtils::Left(origstr, 4);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "Test";
  varstr = UnicodeUtils::Left(origstr, 10);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "Test";
  varstr = UnicodeUtils::Left(origstr, std::string::npos);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  // # of characters to omit from right end

  refstr = "Tes";
  varstr = UnicodeUtils::Left(origstr, 1, false);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "Test";
  varstr = UnicodeUtils::Left(origstr, 0, false);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "T";
  varstr = UnicodeUtils::Left(origstr, 3, false);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "";
  varstr = UnicodeUtils::Left(origstr, 4, false);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "";
  varstr = UnicodeUtils::Left(origstr, 5, false);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "";
  varstr = UnicodeUtils::Left(origstr, std::string::npos, false);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  // TODO: Add test to ensure count works properly for multi-codepoint characters
}

TEST(TestUnicodeUtils, Left_Advanced)
{
  std::string origstr;
  std::string refstr;
  std::string varstr;

  // Multi-byte characters character count != byte count

  origstr =
      std::string(TestUnicodeUtils::UTF8_MULTI_CODEPOINT_CHAR1_VARIENT_1); // 3 codepoints, 1 char
  refstr = "";
  varstr = UnicodeUtils::Left(origstr, 0, getUSEnglishLocale(), true);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  // Interesting case. All five VARIENTs can be normalized
  // to a single codepoint. We are NOT normalizing here.

  origstr = std::string(TestUnicodeUtils::UTF8_MULTI_CODEPOINT_CHAR1_VARIENT_1);
  refstr =
      std::string(TestUnicodeUtils::UTF8_MULTI_CODEPOINT_CHAR1_VARIENT_5); // 2 codepoints, 1 char
  varstr = UnicodeUtils::Left(origstr, 2, getUSEnglishLocale());
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  origstr = std::string(TestUnicodeUtils::UTF8_GERMAN_SAMPLE); // u"óóßChloë"
  refstr = "óó";
  varstr = UnicodeUtils::Left(origstr, 2, getUSEnglishLocale(), true);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  // Get leftmost substring removing n characters from end of string

  origstr = std::string(TestUnicodeUtils::UTF8_MULTI_CODEPOINT_CHAR1_VARIENT_1);
  refstr = std::string("");
  varstr = UnicodeUtils::Left(origstr, 1, getUSEnglishLocale(), false);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  // Interesting case. All five VARIENTs can be normalized
  // to a single codepoint. We are NOT normalizing here.

  origstr = std::string(TestUnicodeUtils::UTF8_MULTI_CODEPOINT_CHAR1_VARIENT_1);
  refstr = std::string("");
  varstr = UnicodeUtils::Left(origstr, 2, getUSEnglishLocale(), false);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  origstr = std::string(TestUnicodeUtils::UTF8_MULTI_CODEPOINT_CHAR1_VARIENT_1);
  refstr = "";
  varstr = UnicodeUtils::Left(origstr, 5, getUSEnglishLocale(), false);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  origstr = std::string(TestUnicodeUtils::UTF8_GERMAN_SAMPLE); // u"óóßChloë"
  refstr = "óóßChl";
  varstr = UnicodeUtils::Left(origstr, 2, getUSEnglishLocale(), false);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  // TODO: Add test to ensure count works properly for multi-codepoint characters
}

/*!
 * \brief Get a substring of a UTF-8 string using character boundary rules
 * defined by the current icu::Locale.
 *
 * Unicode characters may consist of multiple codepoints. This function's
 * parameters are based on characters and NOT bytes. Due to normalization,
 * the byte-length of the strings may change, although the character counts
 * will not.
 *
 * \param str string to extract substring from
 * \param startChar the leftmost n-th character (0-based) in str to include in substring
 * \param charCount number of characters to include in substring (the actual number
 *                  of characters copied is limited by the length of str)
 * \return substring of str, beginning with character 'startChar',
 *         length determined by charCount
 */
/*
 static std::string Mid(const std::string_view& str,
                        const size_t startChar,
                        const size_t charCount = std::string::npos);
 */
TEST(TestUnicodeUtils, Mid)
{
  // TODO: Need more tests
  std::string refstr;
  std::string varstr;
  std::string origstr = "test";

  refstr = "";
  varstr = UnicodeUtils::Mid(origstr, 0, 0);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "te";
  varstr = UnicodeUtils::Mid(origstr, 0, 2);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "test";
  varstr = UnicodeUtils::Mid(origstr, 0, 10);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "st";
  varstr = UnicodeUtils::Mid(origstr, 2);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "st";
  varstr = UnicodeUtils::Mid(origstr, 2, 2);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "es";
  varstr = UnicodeUtils::Mid(origstr, 1, 2);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "est";
  varstr = UnicodeUtils::Mid(origstr, 1, std::string::npos);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "est";
  varstr = UnicodeUtils::Mid(origstr, 1, 4);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "";
  varstr = UnicodeUtils::Mid(origstr, 1, 0);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "t";
  varstr = UnicodeUtils::Mid(origstr, 3, 1);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "";
  varstr = UnicodeUtils::Mid(origstr, 4, 1);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());
}

/*!
 * \brief Get the rightmost end of a UTF-8 string, using character boundary
 * rules defined by the current icu Locale.
 *
 * Unicode characters may consist of multiple codepoints. This function's
 * parameters are based on characters and NOT bytes. Due to normalization,
 * the byte-length of the strings may change, although the character counts
 * will not.
 *
 * \param str to get a substring of
 * \param charCount if keepRight: charCount is number of characters to
 *                  copy from right end (limited by str length)
 *                  if ! keepRight: charCount number of characters to omit from right end
 * \param keepRight controls how charCount is interpreted
 * \return rightmost characters of string, length determined by charCount
 *
 * Ex: Copy all but the leftmost two characters from str:
 *
 * std::string x = Right(str, 2, false);
 */
//static std::string Right(const std::string_view& str, const size_t charCount, bool keepRight = true);

/*!
 * \brief Get the rightmost end of a UTF-8 string, using character boundary
 * rules defined by the given locale.
 *
 * Unicode characters may consist of multiple codepoints. This function's
 * parameters are based on characters and NOT bytes. Due to normalization,
 * the byte-length of the strings may change, although the character counts
 * will not.
 *
 * \param str to get a substring of
 * \param charCount if keepRight: charCount is number of characters to
 *                  copy from right end (limited by str length)
 *                  if ! keepRight: charCount number of characters to omit from right end
 * \param icuLocale determines character boundaries
 * \param keepRight controls how charCount is interpreted
 * \return rightmost characters of string, length determined by charCount
 *
 * Ex: Copy all but the leftmost two characters from str:
 *
 * std::string x = Right(str, 2, false, Unicode::GetDefaultICULocale());
 */
/*
  static std::string Right(const std::string_view& str,
                           const size_t charCount,
                           const icu::Locale& icuLocale,
                           bool keepRight = true);
 */
/*!
 * \brief Get the rightmost side of a UTF-8 string, using character boundary
 * rules defined by the given locale.
 *
 * Unicode characters may consist of multiple codepoints. This function's
 * parameters are based on characters and NOT bytes. Due to normalization,
 * the byte-length of the strings may change, although the character counts
 * will not.
 *
 * \param str to get a substring of
 * \param charCount if rightReference: charCount is number of characters to
 *                  copy from right end (limited by str length)
 *                  if ! rightReference: number of characters to omit from left end
 * \param rightReference controls how charCount is interpreted
 * \param icuLocale determines how character breaks are made
 * \return rightmost characters of string, length determined by charCount
 *
 * Ex: Copy all but the leftmost two characters from str:
 *
 * std::string x = Right(str, 2, false, Unicode::GetDefaultICULocale());
 */
/*
   static std::string Right(const std::string_view& str,
                            const size_t charCount,
                            bool rightReference,
                            const icu::Locale& icuLocale);
 */
TEST(TestUnicodeUtils, Right)
{
  // TODO: Create Right_Advanced test
  std::string refstr;
  std::string varstr;
  std::string origstr = "Test";

  // First, request n chars to copy from right end

  refstr = "";
  varstr = UnicodeUtils::Right(origstr, 0);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "st";
  varstr = UnicodeUtils::Right(origstr, 2);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "Test";
  varstr = UnicodeUtils::Right(origstr, 4);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "Test";
  varstr = UnicodeUtils::Right(origstr, 10);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "Test";
  varstr = UnicodeUtils::Right(origstr, std::string::npos);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  // # of characters to omit from left end

  refstr = "est";
  varstr = UnicodeUtils::Right(origstr, 1, false);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "Test";
  varstr = UnicodeUtils::Right(origstr, 0, false);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "t";
  varstr = UnicodeUtils::Right(origstr, 3, false);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "";
  varstr = UnicodeUtils::Right(origstr, 4, false);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "";
  varstr = UnicodeUtils::Right(origstr, 5, false);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "";
  varstr = UnicodeUtils::Right(origstr, std::string::npos, false);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());
}

/*!
 * \brief Gets the byte-offset of a Unicode character relative to a reference
 *
 * This function is primarily used by Left, Right and Mid. See comment at end for details on use.
 *
 *  Unicode characters may consist of multiple codepoints. This function's parameters
 * are based on characters NOT bytes.
 *
 * \param str UTF-8 string to get index from
 * \param charCount number of characters from reference point to get byte index for
 * \param left + keepLeft define how character index is measured. See comment below
 * \param keepLeft + left define how character index is measured. See comment below
 * \param icuLocale fine-tunes character boundary rules
 * \return code-unit index, relative to str for the given character count
 *                  Unicode::BEFORE_START or Unicode::AFTER::END is returned if
 *                  charCount exceeds the string's length. std::string::npos is
 *                  returned on other errors.
 *
 * left=true  keepLeft=true   Returns offset of last byte of nth character (0-n). Used by Left.
 * left=true  keepLeft=false  Returns offset of last byte of nth character from right end (0-n). Used by Left(x, false)
 * left=false keepLeft=true   Returns offset of first byte of nth character (0-n). Used by Right(x, false)
 * left=false keepLeft=false  Returns offset of first byte of nth char from right end.
 *                            Character 0 is AFTER the last character.  Used by Right(x)
 */
/*
 static size_t GetCharPosition(const std::string_view& str,
                               size_t charCount,
                               const bool left,
                               const bool keepLeft,
                               const icu::Locale& icuLocale);
 */
/*!
 * \brief Gets the byte-offset of a Unicode character relative to a reference
 *
 * This function is primarily used by Left, Right and Mid. See comment at end for details on use.
 *
 * Unicode characters may consist of multiple codepoints. This function's parameters
 * are based on characters NOT bytes.
 *
 * \param str UTF-8 string to get index from
 * \param charCount number of characters from reference point to get byte index for
 * \param left + keepLeft define how character index is measured. See comment below
 * \param keepLeft + left define how character index is measured. See comment below
 *                   std::string::npos is returned if charCount is outside of the string
 * \return code-unit index, relative to str for the given character count
 *                  Unicode::BEFORE_START or Unicode::AFTER::END is returned if
 *                  charCount exceeds the string's length. std::string::npos is
 *                  returned on other errors.
 *
 * left=true  keepLeft=true   Returns offset of last byte of nth character (0-n). Used by Left.
 * left=true  keepLeft=false  Returns offset of last byte of nth character from right end (0-n). Used by Left(x, false)
 * left=false keepLeft=true   Returns offset of first byte of nth character (0-n). Used by Right(x, false)
 * left=false keepLeft=false  Returns offset of first byte of nth char from right end.
 *                            Character 0 is AFTER the last character.  Used by Right(x)
 */
/*
 static size_t GetCharPosition(const std::string_view& str,
                               size_t charCount,
                               const bool left,
                               const bool keepLeft,
                               const std::locale& locale);
 */
/*!
 * \brief Gets the byte-offset of a Unicode character relative to a reference
 *
 * This function is primarily used by Left, Right and Mid. See comment at end for details on use.
 * The currently configured locale is used to tweak character boundaries.
 *
 * Unicode characters may consist of multiple codepoints. This function's parameters
 * are based on characters NOT bytes.
 *
 * \param str UTF-8 string to get index from
 * \param charCount number of characters from reference point to get byte index for
 * \param left + keepLeft define how character index is measured. See comment below
 * \param keepLeft + left define how character index is measured. See comment below
 * \return code-unit index, relative to str for the given character count
 *                  Unicode::BEFORE_START or Unicode::AFTER::END is returned if
 *                  charCount exceeds the string's length. std::string::npos is
 *                  returned on other errors.
 *
 * left=true  keepLeft=true   Returns offset of last byte of nth character (0-n). Used by Left.
 * left=true  keepLeft=false  Returns offset of last byte of nth character from right end (0-n). Used by Left(x, false)
 * left=false keepLeft=true   Returns offset of first byte of nth character (0-n). Used by Right(x, false)
 * left=false keepLeft=false  Returns offset of first byte of nth char from right end.
 *                            Character 0 is AFTER the last character.  Used by Right(x)
 */
/*
 static size_t GetCharPosition(const std::string_view& str,
                               size_t charCount,
                               const bool left,
                               const bool keepLeft);
 */
TEST(TestUnicodeUtils, GetCharPosition)
{
  std::string testString;
  size_t result;
  size_t expectedResult;

  icu::Locale icuLocale = icu::Locale::getEnglish();

  /*
   * left=true  keepLeft=true   Returns offset of last byte of nth character (0-n). Used by Left.
   * left=true  keepLeft=false  Returns offset of last byte of nth character from right end (0-n). Used by Left(x, false)
   * left=false keepLeft=true   Returns offset of first byte of nth character (0-n). Used by Right(x, false)
   * left=false keepLeft=false  Returns offset of first byte of nth char from right end.
   *                            Character 0 is AFTER the last character.  Used by Right(x)
   */

  bool left = true;
  bool keepLeft = true;
  testString = "Hello";
  result = UnicodeUtils::GetCharPosition(testString, 0, left, keepLeft, icuLocale);
  expectedResult = 0;
  EXPECT_EQ(expectedResult, result);

  result = UnicodeUtils::GetCharPosition(testString, 1, left, keepLeft, icuLocale);
  expectedResult = 1;
  EXPECT_EQ(expectedResult, result);

  result = UnicodeUtils::GetCharPosition(testString, 4, left, keepLeft, icuLocale);
  expectedResult = 4;
  EXPECT_EQ(expectedResult, result);

  result = UnicodeUtils::GetCharPosition(testString, 5, left, keepLeft, icuLocale);
  expectedResult = Unicode::AFTER_END;
  EXPECT_EQ(expectedResult, result);

  result = UnicodeUtils::GetCharPosition(testString, 6, left, keepLeft, icuLocale);
  expectedResult = Unicode::AFTER_END;
  EXPECT_EQ(expectedResult, result);

  result = UnicodeUtils::GetCharPosition(testString, std::string::npos, left, keepLeft, icuLocale);
  expectedResult = Unicode::AFTER_END;
  EXPECT_EQ(expectedResult, result);

  //  left=true  getBeginIndex=false Returns offset of last byte of nth character from right end. Used by Left(x, false)

  left = true;
  keepLeft = false;

  result = UnicodeUtils::GetCharPosition(testString, 0, left, keepLeft, icuLocale);
  expectedResult = 4;
  EXPECT_EQ(expectedResult, result);

  result = UnicodeUtils::GetCharPosition(testString, 1, left, keepLeft, icuLocale);
  expectedResult = 3;
  EXPECT_EQ(expectedResult, result);

  result = UnicodeUtils::GetCharPosition(testString, 4, left, keepLeft, icuLocale);
  expectedResult = 0;
  EXPECT_EQ(expectedResult, result);

  result = UnicodeUtils::GetCharPosition(testString, 5, left, keepLeft, icuLocale);
  expectedResult = Unicode::BEFORE_START;
  EXPECT_EQ(expectedResult, result);

  result = UnicodeUtils::GetCharPosition(testString, 6, left, keepLeft, icuLocale);
  expectedResult = Unicode::BEFORE_START;
  EXPECT_EQ(expectedResult, result);

  result = UnicodeUtils::GetCharPosition(testString, std::string::npos, left, keepLeft, icuLocale);
  expectedResult = Unicode::BEFORE_START;
  EXPECT_EQ(expectedResult, result);

  //  left=false keepLeft=true  Return offset of first byte of nth character

  left = false;
  keepLeft = true;

  result = UnicodeUtils::GetCharPosition(testString, 0, left, keepLeft, icuLocale);
  expectedResult = 0;
  EXPECT_EQ(expectedResult, result);

  result = UnicodeUtils::GetCharPosition(testString, 4, left, keepLeft, icuLocale);
  expectedResult = 4;
  EXPECT_EQ(expectedResult, result);

  result = UnicodeUtils::GetCharPosition(testString, 6, left, keepLeft, icuLocale);
  expectedResult = Unicode::AFTER_END;
  EXPECT_EQ(expectedResult, result);

  result = UnicodeUtils::GetCharPosition(testString, std::string::npos, left, keepLeft, icuLocale);
  expectedResult = Unicode::AFTER_END;
  EXPECT_EQ(expectedResult, result);

  // left=false keepLeft=false  Returns offset of first byte of nth char from right end. Used by Right(x)

  // Hello
  left = false;
  keepLeft = false;
  result = UnicodeUtils::GetCharPosition(testString, 0, left, keepLeft, icuLocale);
  expectedResult = 4; // Unicode::AFTER_END;
  EXPECT_EQ(expectedResult, result);

  result = UnicodeUtils::GetCharPosition(testString, 1, left, keepLeft, icuLocale);
  expectedResult = 3; //4;
  EXPECT_EQ(expectedResult, result);

  result = UnicodeUtils::GetCharPosition(testString, 4, left, keepLeft, icuLocale);
  expectedResult = 0;
  EXPECT_EQ(expectedResult, result);

  result = UnicodeUtils::GetCharPosition(testString, std::string::npos, left, keepLeft, icuLocale);
  expectedResult = Unicode::BEFORE_START;
  EXPECT_EQ(expectedResult, result);

  result = UnicodeUtils::GetCharPosition(testString, std::string::npos, left, keepLeft, icuLocale);
  expectedResult = Unicode::BEFORE_START;
  EXPECT_EQ(expectedResult, result);

  testString = "„Wiener Übereinkommen über den Straßenverkehr\"";
  //testString = UnicodeUtils::Normalize(testString,  StringOptions::FOLD_CASE_DEFAULT, NormalizerType::NFC);
  // At 0 \xe2\x80\x9e
  // Ü \xc3\x9c
  // ü \xc3\xbc
  // ß \xc3\x9f
  //  left=true  keepLeft=true   Returns offset of last byte of nth character (0-n). Used by Left.

  icuLocale = icu::Locale::getGerman();
  left = true;
  keepLeft = true;
  result = UnicodeUtils::GetCharPosition(testString, 0, left, keepLeft, icuLocale);
  expectedResult = 2; // Last byte of lower open double quote
  EXPECT_EQ(expectedResult, result);

  left = true;
  keepLeft = true;
  result = UnicodeUtils::GetCharPosition(testString, 1, left, keepLeft, icuLocale);
  expectedResult = 3; // W after quote
  EXPECT_EQ(expectedResult, result);

  //  left=false keepLeft=true   Returns offset of first byte of nth character (0-n). Used by Right(x, false)
  left = false;
  keepLeft = true;
  result = UnicodeUtils::GetCharPosition(testString, 0, left, keepLeft, icuLocale);
  expectedResult = 0; // First Byte of lower open double quote
  EXPECT_EQ(expectedResult, result);

  //  left=false keepLeft=true   Returns offset of first byte of nth character (0-n). Used by Right(x, false)

  left = false;
  keepLeft = true;
  result = UnicodeUtils::GetCharPosition(testString, 1, left, keepLeft, icuLocale);
  expectedResult = 3; // First Byte of W
  EXPECT_EQ(expectedResult, result);

  // left=true  keepLeft=false  Returns offset of last byte of nth character from right end (0-n). Used by Left(x, false)

  left = true;
  keepLeft = false;
  result = UnicodeUtils::GetCharPosition(testString, 45, left, keepLeft, icuLocale);
  expectedResult = 2; // Last Byte of \xe2\x80\x9e
  EXPECT_EQ(expectedResult, result);

  // left=false keepLeft=false  Returns offset of first byte of nth char from right end (0-n). Used by Right(x)
  // Note that char 0 is BEYOND the last character, which is sad to say, different from char 0 from Left
  // end which is the first character.

  left = false;
  keepLeft = false;
  result = UnicodeUtils::GetCharPosition(testString, 45, left, keepLeft, icuLocale);
  expectedResult = 0; // First Byte first char
  EXPECT_EQ(expectedResult, result);

  //  left=false keepLeft=true   Returns offset of first byte of (n -1)th character (0-n). Used by Right(x, false)

  left = false;
  keepLeft = true;
  result = UnicodeUtils::GetCharPosition(testString, 8, left, keepLeft, icuLocale);
  expectedResult = 10;
  EXPECT_EQ(expectedResult, result);

  //  left=true  keepLeft=true   Returns offset of last byte of nth character (0-n). Used by Left.

  left = true;
  keepLeft = true;
  result = UnicodeUtils::GetCharPosition(testString, 7, left, keepLeft, icuLocale);
  expectedResult = 9; // Byte BEFORE U-umlaut
  EXPECT_EQ(expectedResult, result);

  left = true;
  keepLeft = true;
  result = UnicodeUtils::GetCharPosition(testString, 8, left, keepLeft, icuLocale);
  expectedResult = 11; // Last Byte of U-umlaut
  EXPECT_EQ(expectedResult, result);

  //  left=false keepLeft=true   Returns offset of first byte of nth character (0-n). Used by Right(x, false)

  left = false;
  keepLeft = true;
  result = UnicodeUtils::GetCharPosition(testString, 8, left, keepLeft, icuLocale);
  expectedResult = 10; // First Byte of U-umlaut
  EXPECT_EQ(expectedResult, result);

  // left=false keepLeft=false  Returns offset of first byte of nth char from right end (0-n). Used by Right(x)

  left = false;
  keepLeft = false;
  result = UnicodeUtils::GetCharPosition(testString, 10, left, keepLeft, icuLocale);
  expectedResult = 39;
  EXPECT_EQ(expectedResult, result);

  // left=true  keepLeft=false  Returns offset of last byte of nth character from right end (0-n).
  // Used by Left(x, false)

  left = true;
  keepLeft = false;
  result =
      UnicodeUtils::GetCharPosition(testString, 10, left, keepLeft, icuLocale); // S-sharp \xc3\x9f
  expectedResult = 40; // Last Byte of S-Sharp
  EXPECT_EQ(expectedResult, result);

  //  left=false keepLeft=true   Returns offset of first byte of nth character (0-n). Used by Right(x, false)

  left = false;
  keepLeft = true;
  result =
      UnicodeUtils::GetCharPosition(testString, 35, left, keepLeft, icuLocale); // S-sharp \xc3\x9f
  expectedResult = 39;
  EXPECT_EQ(expectedResult, result);

  left = true;
  keepLeft = true;
  result =
      UnicodeUtils::GetCharPosition(testString, 35, left, keepLeft, icuLocale); // S-sharp \xc3\x9f
  expectedResult = 40;
  EXPECT_EQ(expectedResult, result);

  // Test with consecutive multi-byte characters at beginning and end of string

  testString = "诺贝尔生理学于1901年首次颁发";
  // UTF-8, Some beginning and ending characters separated by space
  // e8afba e8b49d e5b094 e7949f e79086,e5ada6,e4ba8e,31,39,30,31,e5b9b4
  // e9a696 e6aca1 e9a281 e58f91
  // 诺 e8afba
  // 贝 e8b49d
  // 尔 e5b094
  // ...
  // 次 e6aca1
  // 颁 e9a281
  // 发 e58f91

  //  left=true  keepLeft=true   Returns offset of last byte of nth character (0-n). Used by Left.

  icuLocale = icu::Locale::getChinese();
  left = true;
  keepLeft = true;
  result = UnicodeUtils::GetCharPosition(testString, 0, left, keepLeft, icuLocale);
  expectedResult = 2; // Last byte of 诺 e8 af ba
  EXPECT_EQ(expectedResult, result);

  left = true;
  keepLeft = true;
  result = UnicodeUtils::GetCharPosition(testString, 1, left, keepLeft, icuLocale);
  expectedResult = 5; // Last byte of 贝 e8 b4 9d
  EXPECT_EQ(expectedResult, result);

  //  left=false keepLeft=true   Returns offset of first byte of (n - 1)th character (0-n). Used by Right(x, false)
  left = false;
  keepLeft = true;
  result = UnicodeUtils::GetCharPosition(testString, 0, left, keepLeft, icuLocale);
  expectedResult = 0; // First Byte of 诺 e8 af ba
  EXPECT_EQ(expectedResult, result);

  //  left=false keepLeft=true   Returns offset of first byte of (n - 1)th character (0-n). Used by Right(x, false)

  left = false;
  keepLeft = true;
  result = UnicodeUtils::GetCharPosition(testString, 1, left, keepLeft, icuLocale);
  expectedResult = 3; // First Byte of 贝 e8 b4 9d
  EXPECT_EQ(expectedResult, result);
}

/*!
 * \brief Remove all whitespace from beginning and end of str in-place
 *
 * \param str to trim
 *
 * Whitespace is defined for Unicode as:  [\t\n\f\r\p{Z}] where \p{Z} means marked as white space
 * which includes ASCII space, plus a number of Unicode space characters. See Unicode::Trim for
 * a complete list.
 *
 * \return trimmed string, same as str argument.
 *
 * Note: Prior to Kodi 20 Trim defined whitespace as: isspace() which is [ \t\n\v\f\r] and
 *       as well as other characters, depending upon locale.
 */
//static std::string& Trim(std::string& str);

/*!
 * \brief Remove a set of characters from beginning and end of str in-place
 *
 *  Remove any leading or trailing characters from the set chars from str.
 *
 *  Ex: Trim("abc1234bxa", "acb") ==> "1234bx"
 *
 * \param str to trim
 * \param chars characters to remove from str
 * \return trimmed string, same as str argument.
 *
 * Note: Prior algorithm only supported chars containing ASCII characters.
 * This implementation allows for chars to be any utf-8 characters. (Does NOT Normalize).
 */
//static std::string& Trim(std::string& str, const char* const chars);

TEST(TestUnicodeUtils, Trim)
{
  std::string refstr = "test test";

  std::string varstr = " test test   ";
  std::string result;

  result = UnicodeUtils::Trim(varstr);
  EXPECT_STREQ(result.c_str(), refstr.c_str());

  refstr = "";
  varstr = " \n\r\t   ";
  result = UnicodeUtils::Trim(varstr);
  EXPECT_STREQ(result.c_str(), refstr.c_str());

  refstr = "\nx\r\t   x";
  varstr = "$ \nx\r\t   x?\t";
  result = UnicodeUtils::Trim(varstr, "$? \t");
  EXPECT_STREQ(result.c_str(), refstr.c_str());

  refstr = "";
  varstr = " ";
  result = UnicodeUtils::Trim(varstr, " \t");
  EXPECT_STREQ(result.c_str(), refstr.c_str());
}

/*!
 *  \brief Remove all whitespace from beginning of str in-place
 *
 *  See UnicodeUtils::Trim(str) for a description of whitespace characters
 *
 * \param str to trim
 * \return trimmed string, same as str argument.
 */
// static std::string& TrimLeft(std::string& str);

/*!
 * \brief Remove a set of characters from beginning of str in-place
 *
 *  Remove any leading characters from the set chars from str.
 *
 *  Ex: TrimLeft("abc1234bxa", "acb") ==> "1234bxa"
 *
 * \param str to trim
 * \param chars (characters) to remove from str
 * \return trimmed string, same as str argument.
 *
 * Note: Prior algorithm only supported chars containing ASCII characters.
 * This implementation allows for chars to be any utf-8 characters. (Does NOT Normalize).
 */
// static std::string& TrimLeft(std::string& str, const char* const chars);

TEST(TestUnicodeUtils, TrimLeft)
{
  std::string refstr = "test test   ";

  std::string varstr = " test test   ";
  std::string result;

  result = UnicodeUtils::TrimLeft(varstr);
  EXPECT_STREQ(refstr.c_str(), result.c_str());

  refstr = "";
  varstr = " \n\r\t   ";
  result = UnicodeUtils::TrimLeft(varstr);
  EXPECT_STREQ(refstr.c_str(), result.c_str());

  refstr = "\nx\r\t   x?\t";
  varstr = "$ \nx\r\t   x?\t";
  result = UnicodeUtils::TrimLeft(varstr, "$? \t");
  EXPECT_STREQ(refstr.c_str(), result.c_str());
}

/*!
 * \brief Remove all whitespace from end of str in-place
 *
 * See Trim(str) for information about what characters are considered whitespace
 *
 * \param str to trim
 * \return trimmed string, same as str argument.
 */
// static std::string& TrimRight(std::string& str);

/*!
 *  \brief Remove trailing characters from the set of chars from str in-place
 *
 *  Ex: TrimRight("abc1234bxa", "acb") ==> "abc1234bx"
 *
 * \param str to trim
 * \param chars (characters) to remove from str
 * \return trimmed string, same as str argument.
 *
 * Note: Prior algorithm only supported chars containing ASCII characters.
 * This implementation allows for chars to be any utf-8 characters. (Does NOT Normalize).
 */
//static std::string& TrimRight(std::string& str, const char* const chars);

TEST(TestUnicodeUtils, TrimRight)
{
  std::string refstr = " test test";

  std::string varstr = " test test   ";
  std::string result;

  result = UnicodeUtils::TrimRight(varstr);
  EXPECT_STREQ(refstr.c_str(), result.c_str());

  refstr = "";
  varstr = " \n\r\t   ";
  result = UnicodeUtils::TrimRight(varstr);
  EXPECT_STREQ(refstr.c_str(), result.c_str());

  refstr = "$ \nx\r\t   x";
  varstr = "$ \nx\r\t   x?\t";
  result = UnicodeUtils::TrimRight(varstr, "$? \t");
  EXPECT_STREQ(refstr.c_str(), result.c_str());
}

TEST(TestUnicodeUtils, Trim_Multiple)
{
  std::string input;
  std::string trimmableChars;
  std::string result;
  std::string expectedResult;

  input = " Test Test   ";
  trimmableChars = " Tt";
  expectedResult = "est Tes";

  result = UnicodeUtils::Trim(input, trimmableChars.data());
  EXPECT_STREQ(result.c_str(), expectedResult.c_str());

  input = " \n\r\t   ";
  trimmableChars = "\t \n \r";
  expectedResult = "";
  result = UnicodeUtils::Trim(input, trimmableChars.data());
  EXPECT_STREQ(result.c_str(), expectedResult.c_str());

  input = "$ \nx\r\t  x?\t";
  trimmableChars = "$\n?";
  expectedResult = " \nx\r\t  x?\t";
  result = UnicodeUtils::Trim(input, trimmableChars.data());
  EXPECT_STREQ(result.c_str(), expectedResult.c_str());

  // "„Wiener Übereinkommen über den Straßenverkehr\"";

  bool trimStart;
  bool trimEnd;

  std::string_view sv = "„ßÜ„Wiener Übereinkommen über den Straßenverkehrüß\"";
  trimmableChars = "„ßÜ\"";
  std::vector<std::string_view> trimmableStrs = {"„", "ß", "Ü", "\""};
  trimStart = true;
  trimEnd = true;
  expectedResult = "Wiener Übereinkommen über den Straßenverkehrü";
  result = Unicode::Trim(sv, trimmableStrs, trimStart, trimEnd);
  EXPECT_STREQ(result.c_str(), expectedResult.c_str());
}

/*!
 * \brief Replaces every occurrence of a char in string.
 *
 * Somewhat less efficient than FindAndReplace because this one returns a count
 * of the number of changes.
 *
 * \param str String to make changes to in-place
 * \param oldChar character to be replaced
 * \param newChar character to replace with
 * \return Count of the number of changes
 */
// [[deprecated("FindAndReplace is faster, returned count not used.")]] static int Replace(
//     std::string& str, char oldChar, char newChar);

/*!
 * \brief Replaces every occurrence of a string within another string.
 *
 * Somewhat less efficient than FindAndReplace because this one returns a count
 * of the number of changes.
 *
 * \param str String to make changes to in-place
 * \param oldStr string to be replaced
 * \param newStr string to replace with
 * \return Count of the number of changes
 */
// [[deprecated("FindAndReplace is faster, returned count not used.")]] static int Replace(
//     std::string& str, const std::string_view& oldStr, const std::string_view& newStr);

/*!
 * \brief Replaces every occurrence of a wstring within another wstring in-place
 *
 *  Somewhat less efficient than FindAndReplace because this one returns a count
 *  of the number of changes.
 *
 * \param str String to make changes to
 * \param oldStr string to be replaced
 * \parm newStr string to replace with
 * \return Count of the number of changes
 */
// [[deprecated("FindAndReplace is faster, returned count not used.")]] static int Replace(
//     std::wstring& str, const std::wstring_view& oldStr, const std::wstring_view& newStr);


TEST(TestUnicodeUtils, Replace)
{
  std::string refstr = "text text";
  std::string varstr = "test test";

  EXPECT_TRUE(UnicodeUtils::Replace(varstr, "s", "x"));
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  EXPECT_FALSE(UnicodeUtils::Replace(varstr, "s", "x"));
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  varstr = "test test";
  EXPECT_TRUE(UnicodeUtils::Replace(varstr, "s", "x"));
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  EXPECT_FALSE(UnicodeUtils::Replace(varstr, "s", "x"));
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());
}

/*!
 * \brief Replaces every occurrence of a string within another string
 *
 * Should be more efficient than Replace since it directly uses an icu library
 * routine and does not have to count changes.
 *
 * \param str String to make changes to
 * \param oldStr string to be replaced
 * \parm newStr string to replace with
 * \return the modified string.
 */
/* static std::string FindAndReplace(const std::string_view& str,
                                   const std::string_view oldText,
                                   const std::string_view newText);
 */
/*!
 * \brief Replaces every occurrence of a string within another string.
 *
 * Should be more efficient than Replace since it directly uses an icu library
 * routine and does not have to count changes.
 *
 * \param str String to make changes to
 * \param oldStr string to be replaced
 * \parm newStr string to replace with
 * \return the modified string
 */
/*
 static std::string FindAndReplace(const std::string& str,
                                   const char* oldText,
                                   const char* newText);
 */
/*!
 * \brief Replaces every occurrence of a regex pattern with a string in another string.
 *
 * Regex based version of Replace. See:
 * https://unicode-org.github.io/icu/userguide/strings/regexp.html
 *
 * \param str string being altered
 * \param pattern regular expression pattern
 * \param newStr new value to replace with
 * \param flags controls behavior of regular expression engine
 * \return result of regular expression
 */
/*
  std::string RegexReplaceAll(const std::string_view& str,
                              const std::string_view pattern,
                              const std::string_view newStr,
                              const int flags);
 */
/*!
 *  \brief Normalizes a wstring. Not expected to be used outside of UnicodeUtils.
 *
 *  Made public to facilitate testing.
 *
 *  There are multiple Normalizations that can be performed on Unicode. Fortunately
 *  normalization is not needed in many situations. An introduction can be found
 *  at: https://unicode-org.github.io/icu/userguide/transforms/normalization/
 *
 *  \param str string to Normalize.
 *  \param options fine tunes behavior. See StringOptions. Frequently can leave
 *         at default value.
 *  \param NormalizerType select the appropriate Normalizer for the job
 *  \return Normalized string
 */
/*
   static const std::wstring Normalize(const std::wstring_view& src,
                                       const StringOptions opt = StringOptions::FOLD_CASE_DEFAULT,
                                       const NormalizerType NormalizerType = NormalizerType::NFKC);
 */
/*!
 *  \brief Normalizes a string. Not expected to be used outside of UnicodeUtils.
 *
 *  Made public to facilitate testing.
 *
 * There are multiple Normalizations that can be performed on Unicode. Fortunately
 * normalization is not needed in many situations. An introduction can be found
 * at: https://unicode-org.github.io/icu/userguide/transforms/normalization/
 *
 * \param str string to Normalize.
 * \param options fine tunes behavior. See StringOptions. Frequently can leave
 *        at default value.
 * \param NormalizerType select the appropriate Normalizer for the job
 * \return Normalized string
 */
/*
   static const std::string Normalize(const std::string_view& src,
                                      const StringOptions opt = StringOptions::FOLD_CASE_DEFAULT,
                                      const NormalizerType NormalizerType = NormalizerType::NFKC);
 */

TEST(TestUnicodeUtils, Normalize)
{
  // TODO: These tests are all on essentially the same caseless string.
  // The FOLD_CASE_DEFAULT option (which only impacts NFKD and maybe NFKC)
  // is not being tested, neither are the other alternatives to it.

  std::string s1 = TestUnicodeUtils::UTF8_MULTI_CODEPOINT_CHAR1_VARIENT_1;
  std::string result =
      UnicodeUtils::Normalize(s1, StringOptions::FOLD_CASE_DEFAULT, NormalizerType::NFD);
  int cmp = UnicodeUtils::Compare(result, TestUnicodeUtils::UTF8_MULTI_CODEPOINT_CHAR1_VARIENT_2);
  EXPECT_EQ(cmp, 0);

  s1 = TestUnicodeUtils::UTF8_MULTI_CODEPOINT_CHAR1_VARIENT_1;
  result = UnicodeUtils::Normalize(s1, StringOptions::FOLD_CASE_DEFAULT, NormalizerType::NFC);
  cmp = UnicodeUtils::Compare(result, TestUnicodeUtils::UTF8_MULTI_CODEPOINT_CHAR1_VARIENT_5);
  EXPECT_EQ(cmp, 0);

  s1 = TestUnicodeUtils::UTF8_MULTI_CODEPOINT_CHAR1_VARIENT_1;
  result = UnicodeUtils::Normalize(s1, StringOptions::FOLD_CASE_DEFAULT, NormalizerType::NFKC);
  cmp = UnicodeUtils::Compare(result, TestUnicodeUtils::UTF8_MULTI_CODEPOINT_CHAR1_VARIENT_5);
  EXPECT_EQ(cmp, 0);

  s1 = TestUnicodeUtils::UTF8_MULTI_CODEPOINT_CHAR1_VARIENT_1;
  result = UnicodeUtils::Normalize(s1, StringOptions::FOLD_CASE_DEFAULT, NormalizerType::NFD);
  cmp = UnicodeUtils::Compare(result, TestUnicodeUtils::UTF8_MULTI_CODEPOINT_CHAR1_VARIENT_2);
  EXPECT_EQ(cmp, 0);

  s1 = TestUnicodeUtils::UTF8_MULTI_CODEPOINT_CHAR1_VARIENT_1;
  result = UnicodeUtils::Normalize(s1, StringOptions::FOLD_CASE_DEFAULT, NormalizerType::NFKD);
  cmp = UnicodeUtils::Compare(result, TestUnicodeUtils::UTF8_MULTI_CODEPOINT_CHAR1_VARIENT_2);
  EXPECT_EQ(cmp, 0);
}

/*!
 * \brief Splits the given input string into separate strings using the given delimiter.
 *
 * If the given input string is empty the result will be an empty vector (not
 * a vector containing an empty string).
 *
 * \param input string to be split
 * \param delimiter used to split the input string
 * \param iMaxStrings (optional) Maximum number of generated split strings
 */
/*
  static std::vector<std::string> Split(const std::string& input,
                                        const std::string_view& delimiter,
                                        size_t iMaxStrings = 0);
 */
/*!
 *  \brief Splits the given input string into separate strings using the given delimiter.
 *
 * If the given input string is empty the result will be an empty vector (not
 * an vector containing an empty string).
 *
 * If iMaxStrings limit is reached, the unprocessed input is returned along with the
 * incomplete split strings.
 *
 *  Ex: input = "a/b#c/d/e/f/g/h"
 *     delimiter = "/"
 *     iMaxStrings = 5
 *     returned = {"a", "b#c", "d", "e", "f", "/g/h"}
 *
 * \param input string to be split
 * \param delimiter used to split the input string
 * \param iMaxStrings Maximum number of generated split strings. The default value
 *        of 0 places no limit on the generated strings.
 */
/*
  static std::vector<std::string> Split(const std::string& input,
                                        const char delimiter,
                                        size_t iMaxStrings = 0);
 */
/*!
 * \brief Splits the given input string into separate strings using the given delimiters.
 *
 * \param input string to be split
 * \param delimiters used to split the input string as described above
 * \return a Vector of substrings
 */
/*
  static std::vector<std::string> Split(const std::string& input,
                                        const std::vector<std::string>& delimiters);
 */

TEST(TestUnicodeUtils, Split)
{
  std::vector<std::string> varresults;

  // test overload with string as delimiter
  varresults = UnicodeUtils::Split("g,h,ij,k,lm,,n", ",");
  EXPECT_STREQ("g", varresults.at(0).c_str());
  EXPECT_STREQ("h", varresults.at(1).c_str());
  EXPECT_STREQ("ij", varresults.at(2).c_str());
  EXPECT_STREQ("k", varresults.at(3).c_str());
  EXPECT_STREQ("lm", varresults.at(4).c_str());
  EXPECT_STREQ("", varresults.at(5).c_str());
  EXPECT_STREQ("n", varresults.at(6).c_str());

  varresults = UnicodeUtils::Split(",a,b,cd,", ",");
  EXPECT_STREQ("", varresults.at(0).c_str());
  EXPECT_STREQ("a", varresults.at(1).c_str());
  EXPECT_STREQ("b", varresults.at(2).c_str());
  EXPECT_STREQ("cd", varresults.at(3).c_str());
  EXPECT_STREQ("", varresults.at(4).c_str());

  std::vector<std::string> expectedResult;
  varresults = UnicodeUtils::Split(",a,,aa,,", ",");
  expectedResult = {"", "a", "", "aa", "", ""};
  size_t idx = 0;
  EXPECT_EQ(expectedResult.size(), varresults.size());

  for (auto i : expectedResult)
  {
    if (idx >= 0)
    {
      EXPECT_STREQ(i.c_str(), varresults.at(idx).c_str());
      idx++;
    }
  }

  varresults = UnicodeUtils::Split(",a,,,aa,,,", ",");
  expectedResult = {"", "a", "", "", "aa", "", "", ""};
  EXPECT_EQ(expectedResult.size(), varresults.size());
  idx = 0;
  for (auto i : expectedResult)
  {
    if (idx >= 0)
    {
      EXPECT_STREQ(i.c_str(), varresults.at(idx).c_str());
      idx++;
    }
  }

  std::string_view src = ",-+a,,--++aa,,-+";
  std::vector<std::string_view> splitSet = {",", "-", "+"};
  varresults = UnicodeUtils::Split(src, splitSet);
  expectedResult = {"", "", "", "a", "", "", "", "", "", "aa", "", "", "", ""};
  EXPECT_EQ(expectedResult.size(), varresults.size());
  idx = 0;
  for (auto i : expectedResult)
  {
    if (idx >= 0)
    {
      EXPECT_STREQ(i.c_str(), varresults.at(idx).c_str());
      idx++;
    }
  }
  src = ",-+a,,--++aa,,-+";
  splitSet = {"-", "+", ","};
  varresults = UnicodeUtils::Split(src, splitSet);
  expectedResult = {"", "", "", "a", "", "", "", "", "", "aa", "", "", "", ""};
  EXPECT_EQ(expectedResult.size(), varresults.size());
  idx = 0;
  for (auto i : expectedResult)
  {
    if (idx >= 0)
    {
      EXPECT_STREQ(i.c_str(), varresults.at(idx).c_str());
      idx++;
    }
  }
  src = ",-+a,,--++aa,,-+";
  splitSet = {"+", ",", "-"};
  varresults = UnicodeUtils::Split(src, splitSet);
  expectedResult = {"", "", "", "a", "", "", "", "", "", "aa", "", "", "", ""};
  EXPECT_EQ(expectedResult.size(), varresults.size());
  idx = 0;
  for (auto i : expectedResult)
  {
    if (idx >= 0)
    {
      EXPECT_STREQ(i.c_str(), varresults.at(idx).c_str());
      idx++;
    }
  }
  // This result looks incorrect, but verified against Matrix 19.4 behavior
  src = "a,,,a,,,,,,aa,,,a";
  splitSet = {",", "-", "+"};
  varresults = UnicodeUtils::Split(src, splitSet);
  expectedResult = {"a", "", "", "a", "", "", "", "", "", "aa", "", "", "a"};
  EXPECT_EQ(expectedResult.size(), varresults.size());
  idx = 0;
  for (auto i : expectedResult)
  {
    if (idx >= 0)
    {
      EXPECT_STREQ(i.c_str(), varresults.at(idx).c_str());
      idx++;
    }
  }

  src = {"a,-+a,,--++aa,-+a"};
  splitSet = {",", "-", "+"};
  varresults = UnicodeUtils::Split(src, splitSet);
  expectedResult = {"a", "", "", "a", "", "", "", "", "", "aa", "", "", "a"};
  EXPECT_EQ(expectedResult.size(), varresults.size());
  idx = 0;
  for (auto i : expectedResult)
  {
    if (idx >= 0)
    {
      EXPECT_STREQ(i.c_str(), varresults.at(idx).c_str());
      idx++;
    }
  }

  EXPECT_TRUE(UnicodeUtils::Split("", "|").empty());

  EXPECT_EQ(4U, UnicodeUtils::Split("a bc  d ef ghi ", " ", 4).size());
  EXPECT_STREQ("d ef ghi ", UnicodeUtils::Split("a bc  d ef ghi ", " ", 4).at(3).c_str())
  << "Last part must include rest of the input string";
  EXPECT_EQ(7U, UnicodeUtils::Split("a bc  d ef ghi ", " ").size())
  << "Result must be 7 strings including two empty strings";
  EXPECT_STREQ("bc", UnicodeUtils::Split("a bc  d ef ghi ", " ").at(1).c_str());
  EXPECT_STREQ("", UnicodeUtils::Split("a bc  d ef ghi ", " ").at(2).c_str());
  EXPECT_STREQ("", UnicodeUtils::Split("a bc  d ef ghi ", " ").at(6).c_str());

  EXPECT_EQ(2U, UnicodeUtils::Split("a bc  d ef ghi ", "  ").size());
  EXPECT_EQ(2U, UnicodeUtils::Split("a bc  d ef ghi ", "  ", 10).size());
  EXPECT_STREQ("a bc", UnicodeUtils::Split("a bc  d ef ghi ", "  ", 10).at(0).c_str());

  EXPECT_EQ(1U, UnicodeUtils::Split("a bc  d ef ghi ", " z").size());
  EXPECT_STREQ("a bc  d ef ghi ", UnicodeUtils::Split("a bc  d ef ghi ", " z").at(0).c_str());

  EXPECT_EQ(1U, UnicodeUtils::Split("a bc  d ef ghi ", "").size());
  EXPECT_STREQ("a bc  d ef ghi ", UnicodeUtils::Split("a bc  d ef ghi ", "").at(0).c_str());

  // test overload with char as delimiter
  EXPECT_EQ(4U, UnicodeUtils::Split("a bc  d ef ghi ", ' ', 4).size());
  EXPECT_STREQ("d ef ghi ", UnicodeUtils::Split("a bc  d ef ghi ", ' ', 4).at(3).c_str());
  EXPECT_EQ(7U, UnicodeUtils::Split("a bc  d ef ghi ", ' ').size())
  << "Result must be 7 strings including two empty strings";
  EXPECT_STREQ("bc", UnicodeUtils::Split("a bc  d ef ghi ", ' ').at(1).c_str());
  EXPECT_STREQ("", UnicodeUtils::Split("a bc  d ef ghi ", ' ').at(2).c_str());
  EXPECT_STREQ("", UnicodeUtils::Split("a bc  d ef ghi ", ' ').at(6).c_str());

  EXPECT_EQ(1U, UnicodeUtils::Split("a bc  d ef ghi ", 'z').size());
  EXPECT_STREQ("a bc  d ef ghi ", UnicodeUtils::Split("a bc  d ef ghi ", 'z').at(0).c_str());

  EXPECT_EQ(1U, UnicodeUtils::Split("a bc  d ef ghi ", "").size());
  EXPECT_STREQ("a bc  d ef ghi ", UnicodeUtils::Split("a bc  d ef ghi ", 'z').at(0).c_str());

  std::string_view input;
  std::vector<std::string_view> delimiters;
  std::vector<std::string> result;
  input = "a/b#c/d/e/foo/g::h/"; // "#p/q/r:s/x&extraNarfy"};
  delimiters = {"/", "#", ":", "Narf"};
  expectedResult = {"a", "b", "c", "d", "e", "foo", "g", "", "h", ""}; // , "", "p", "q", "r", "s",
  // "x&extra", "y"};
  result.clear();
  Unicode::SplitTo(std::back_inserter(result), input, delimiters, 0);

  EXPECT_EQ(expectedResult.size(), result.size());
  idx = 0;
  for (auto i : expectedResult)
  {
    if (idx < result.size())
    {
      EXPECT_STREQ(i.c_str(), result.at(idx).c_str());
      idx++;
    }
  }

  std::vector<std::string_view> inputs = {"a/b#c/d/e/foo/g::h/", "#p/q/r:s/x&extraNarfy"};
  delimiters = {"/", "#", ":", "Narf"};
  expectedResult = {"a", "b#c", "d", "e", "foo", "/g::h/", "p", "q", "/r:s/x&extraNarfy"};
  // TODO:  Now what?

}

static void compareStrings(std::vector<std::string> result, std::vector<std::string> expected)
{
  EXPECT_EQ(expected.size(), result.size());
  size_t idx = 0;
  for (auto i : expected)
  {
    if (idx < result.size())
    {
      EXPECT_STREQ(i.c_str(), result.at(idx).c_str());
      idx++;
    }
  }
}

/*!
 * \brief Splits each input string with each delimiter string producing a vector of split strings
 * omitting any null strings.
 *
 * \param input vector of strings to be split
 * \param delimiters when found in a string, a delimiter splits a string into two strings
 * \param iMaxStrings (optional) Maximum number of resulting split strings
 * \result the accumulation of all split strings (and any unfinished splits, due to iMaxStrings
 *          being exceeded) omitting null strings
 *
 *
 * SplitMulti is essentially equivalent to running Split(string, vector<delimiters>, maxstrings) over multiple
 * strings with the same delimiters and returning the aggregate results. Null strings are not returned.
 *
 * There are some significant differences when maxstrings alters the process. Here are the boring details:
 *
 * For each delimiter, Split(string<n> input, delimiter, maxstrings) is called for each input string.
 * The results are appended to results <vector<string>>
 *
 * After a delimiter has been applied to all input strings, the process is repeated with the
 * next delimiter, but this time with the vector<string> input being replaced with the
 * results of the previous pass.
 *
 * If the maxstrings limit is not reached, then, as stated above, the results are similar to
 * running Split(string, vector<delimiters> maxstrings) over multiple strings. But when the limit is reached
 * differences emerge.
 *
 * Before a delimiter is applied to a string a check is made to see if maxstrings is exceeded. If so,
 * then splitting stops and all split string results are returned, including any strings that have not
 * been split by as many delimiters as others, leaving the delimiters in the results.
 *
 * Differences between current behavior and prior versions: Earlier versions removed most empty strings,
 * but a few slipped past. Now, all empty strings are removed. This means not as many empty strings
 * will count against the maxstrings limit. This change should cause no harm since there is no reliable
 * way to correlate a result with an input; they all get thrown in together.
 *
 * If an input vector element is empty the result will be an empty vector element (not
 * an empty string).
 *
 * Examples:
 *
 * Delimiter strings are applied in order, so once iMaxStrings
 * items is produced no other delimiters are applied. This produces different results
 * than applying all delimiters at once:
 *
 * Ex: input = {"a/b#c/d/e/foo/g::h/", "#p/q/r:s/x&extraNarfy"}
 *     delimiters = {"/", "#", ":", "Narf"}
 *     if iMaxStrings=7
 *        return value = {"a", "b#c", "d" "e", "foo", "/g::h/", "p", "q", "/r:s/x&extraNarfy}"
 *
 *     if iMaxStrings=0
 *        return value = {"a", "b", "c", "d", "e", "f", "g", "", "h", "", "", "p", "q", "r", "s",
 *                        "x&extra", "y"}
 *
 * e.g. "a/b#c/d" becomes "a", "b#c", "d" rather
 * than "a", "b", "c/d"
 *
 * \param input vector of strings each to be split
 * \param delimiters strings to be used to split the input strings
 * \param iMaxStrings limits number of resulting split strings. A value of 0
 *        means no limit.
 * \return vector of split strings
 */
/*
 *
  static std::vector<std::string> SplitMulti(const std::vector<std::string>& input,
                                            const std::vector<std::string>& delimiters,
                                            size_t iMaxStrings = 0);
 */
TEST(TestUnicodeUtils, SplitMulti)
{
  /*
   * SplitMulti is essentially equivalent to running Split(string, vector<delimiters>, maxstrings) over multiple
   * strings with the same delimiters and returning the aggregate results. Null strings are not returned.
   *
   * There are some significant differences when maxstrings alters the process. Here are the boring details:
   *
   * For each delimiter, Split(string<n> input, delimiter, maxstrings) is called for each input string.
   * The results are appended to results <vector<string>>
   *
   * After a delimiter has been applied to all input strings, the process is repeated with the
   * next delimiter, but this time with the vector<string> input being replaced with the
   * results of the previous pass.
   *
   * If the maxstrings limit is not reached, then, as stated above, the results are similar to
   *  running Split(string, vector<delimiters> maxstrings) over multiple strings. But when the limit is reached
   *  differences emerge.
   *
   *  Before a delimiter is applied to a string a check is made to see if maxstrings is exceeded. If so,
   *  then splitting stops and all split string results are returned, including any strings that have not
   *  been split by as many delimiters as others, leaving the delimiters in the results.
   *
   *  Differences between current behavior and prior versions: Earlier versions removed most empty strings,
   *  but a few slipped past. Now, all empty strings are removed. This means not as many empty strings
   *  will count against the maxstrings limit. This change should cause no harm since there is no reliable
   *  way to correlate a result with an input; they all get thrown in together.
   *
   * \param input vector of strings to be split
   * \param delimiters strings to be used to split the input strings
   * \param iMaxStrings (optional) Maximum number of resulting split strings
   *   *
   * static std::vector<std::string> SplitMulti(const std::vector<std::string>& input,
   *                                           const std::vector<std::string>& delimiters,
   *                                           size_t iMaxStrings = 0);
   */
  size_t idx;
  std::vector<std::string> expectedResult;
  std::vector<std::string_view> input;
  std::vector<std::string_view> delimiters;
  std::vector<std::string> result;

  input.push_back(",h,ij,k,lm,,n,");
  delimiters.push_back(",");
  result = UnicodeUtils::SplitMulti(input, delimiters);
  // Legacy (Matrix 19.4) behavior (strips most, but not all null strings)
  // EXPECT_STREQ("", result.at(0).c_str());
  // EXPECT_STREQ("h", result.at(1).c_str());
  // EXPECT_STREQ("ij", result.at(2).c_str());
  // EXPECT_STREQ("k", result.at(3).c_str());
  // EXPECT_STREQ("lm", result.at(4).c_str());
  // EXPECT_STREQ("", result.at(5).c_str());
  // EXPECT_STREQ("n", result.at(6).c_str());
  // EXPECT_STREQ("", result.at(7).c_str());

  expectedResult = {"h", "ij", "k", "lm", "n"};
  compareStrings(result, expectedResult);

  input.clear();
  delimiters.clear();
  input = {"abcde", "Where is the beef?", "cbcefa"};
  delimiters = {"a", "bc", "ef", "c"};
  result = UnicodeUtils::SplitMulti(input, delimiters);
  expectedResult = {"de", "Where is the be", "?"};
  result = UnicodeUtils::SplitMulti(input, delimiters);
  EXPECT_EQ(expectedResult.size(), result.size());
  idx = 0;
  for (auto i : expectedResult)
  {
    if (idx < result.size())
    {
      EXPECT_STREQ(i.c_str(), result.at(idx).c_str());
      idx++;
    }
  }

  // These two tests verify the example in UnicodeUtils documentation for
  // SplitMulti.

  input = {"a/b#c/d/e/foo/g::h/", "#p/q/r:s/x&extraNarfy"};
  delimiters = {"/", "#", ":", "Narf"};

  // Legacy (Matrix 19.4) result does not remove all null strings
  // expectedResult = {"a", "b", "c", "d", "e", "foo", "g", "", "h", "", "", "p", "q", "r", "s",
  //                  "x&extra", "y"};
  expectedResult = {"a", "b", "c", "d", "e", "foo", "g", "h", "p", "q", "r", "s", "x&extra", "y"};
  result = UnicodeUtils::SplitMulti(input, delimiters, 0);
  EXPECT_EQ(expectedResult.size(), result.size());
  idx = 0;
  for (auto i : expectedResult)
  {
    if (idx < result.size())
    {
      EXPECT_STREQ(i.c_str(), result.at(idx).c_str());
      idx++;
    }
  }

  input = {"a/b#c/d/e/foo/g::h/", "#p/q/r:s/x&extraNarfy"};
  delimiters = {"/", "#", ":", "Narf"};
  expectedResult = {"a", "b#c", "d", "e", "foo", "g::h/", "#p/q/r:s/x&extraNarfy"};
  result = UnicodeUtils::SplitMulti(input, delimiters, 7);
  EXPECT_EQ(expectedResult.size(), result.size());
  idx = 0;
  for (auto i : expectedResult)
  {
    if (idx < result.size())
    {
      EXPECT_STREQ(i.c_str(), result.at(idx).c_str());
      idx++;
    }
  }

  input = {" a,b-e e-f,c d-", "-sworn enemy, is not here"};
  delimiters = {",", " ", "-"};
  // The following has a minor bug and is present in Matrix 19.4. Should not return null strings
  // expectedResult =  {"a", "b", "e", "e", "f", "c", "d", "", "", "sworn", "enemy", "is", "not", "here"};
  expectedResult = {"a", "b", "e", "e", "f", "c", "d", "sworn", "enemy", "is", "not", "here"};

  result = UnicodeUtils::SplitMulti(input, delimiters);
  EXPECT_EQ(expectedResult.size(), result.size());
  idx = 0;
  for (auto i : expectedResult)
  {
    if (idx < result.size())
    {
      EXPECT_STREQ(i.c_str(), result.at(idx).c_str());
      idx++;
    }
  }
}

/*! \brief Counts the occurrences of strFind in strInput
 *
 * \param strInput string to be searched
 * \param strFind string to count occurrences in strInput
 * \return count of the number of occurrences found
 */
// static int FindNumber(const std::string_view& strInput, const std::string_view& strFind);


TEST(TestUnicodeUtils, FindNumber)
{
  EXPECT_EQ(3, UnicodeUtils::FindNumber("aabcaadeaa", "aa"));
  EXPECT_EQ(1, UnicodeUtils::FindNumber("aabcaadeaa", "b"));
}

/*!
 * \brief Initializes the Collator for this thread, such as before sorting a
 * table.
 *
 * Assumes that all collation will occur in this thread.
 *
 * Note: Only has an impact if icu collation is configured instead of legacy
 *       AlphaNumericCompare.
 *
 *       Also starts the elapsed-time timer for the sort. See SortCompleted.
 *
 * \param icuLocale Collation order will be based on the given locale.
 * \param Normalize Controls whether normalization is performed prior to collation.
 *                  Frequently not required. Some free normalization always occurs.
 * \return true if initialization was successful, otherwise false.
 */
// static bool InitializeCollator(const icu::Locale& icuLocale, bool Normalize = false);

/*!
 * \brief Initializes the Collator for this thread, such as before sorting a
 * table.
 *
 * Assumes that all collation will occur in this thread.
 *
 * Note: Only has an impact if icu collation is configured instead of legacy
 *       AlphaNumericCompare.
 *
 *       Also starts the elapsed-time timer for the sort. See SortCompleted.
 *
 * \param locale Collation order will be based on the given locale.
 * \param Normalize Controls whether normalization is performed prior to collation.
 *                  Frequently not required. Some free normalization always occurs.
 * \return true if initialization was successful, otherwise false.
 */
// static bool InitializeCollator(const std::locale& locale, bool Normalize = false);

/*!
 * \brief Initializes the Collator for this thread using LangInfo::GetSystemLocale,
 * such as before sorting a table.
 *
 * Assumes that all collation will occur in this thread.
 *
 *
 * Note: Only has an impact if icu collation is configured instead of legacy
 *       AlphaNumericCompare.
 *
 *       Also starts the elapsed-time timer for the sort. See SortCompleted.
 *
 * \param Normalize Controls whether normalization is performed prior to collation.
 *                  Frequently not required. Some free normalization always occurs.
 * \return true of initialization was successful, otherwise false.
 */
//static bool InitializeCollator(bool Normalize = false);

/*!
 * \brief Provides the ability to collect basic performance info for the previous sort
 *
 * Must be run in the same thread that InitializeCollator was run. May require some setting
 * or #define to be set to enable recording of data in log.
 *
 * \param sortItems simple count of how many items sorted.
 */
//  static void SortCompleted(int sortItems);


/*!
 * \brief Performs locale sensitive string comparison.
 *
 * Must be run in the same thread that InitializeCollator that configured the Collator
 * for this was run.
 *
 * \param left string to compare
 * \param right string to compare
 * \return  < 0 if left collates < right
 *         == 0 if left collates the same as right
 *          > 0 if left collates > right
 */

//   static int32_t Collate(const std::wstring_view& left, const std::wstring_view& right);

/*!
 * \brief Performs locale sensitive wchar_t* comparison.
 *
 * Must be run in the same thread that InitializeCollator that configured the Collator
 * for this was run.
 *
 * \param left string to compare
 * \param right string to compare
 * \return  < 0 if left collates < right
 *         == 0 if left collates the same as right
 *          > 0 if left collates > right
 */
static int32_t CollateWstring(std::wstring_view left, std::wstring_view right)
{
  return UnicodeUtils::Collate(left, right);
}

static int32_t CollateU16(std::u16string_view left, std::u16string_view right)
{
  return Unicode::Collate(left, right);
}

TEST(TestUnicodeUtils, Collate)
{
  int32_t ref = 0;
  int32_t var;
  icu::Locale icuLocale = icu::Locale::getGerman();
  EXPECT_TRUE(Unicode::InitializeCollator(icuLocale, false));

  const std::wstring s1 = std::wstring(L"The Farmer's Daughter");
  const std::wstring s2 = std::wstring(L"Ate Pie");
  var = UnicodeUtils::Collate(s1, s2);
  EXPECT_GT(var, ref);

  EXPECT_TRUE(Unicode::InitializeCollator(icuLocale, true));
  const std::wstring s3 =
      std::wstring(Unicode::UTF8ToWString(TestUnicodeUtils::UTF8_MULTI_CODEPOINT_CHAR1_VARIENT_5));
  const std::wstring s4 =
      std::wstring(Unicode::UTF8ToWString(TestUnicodeUtils::UTF8_MULTI_CODEPOINT_CHAR1_VARIENT_1));
  var = UnicodeUtils::Collate(s3, s4);
  EXPECT_EQ(var, 0);

  EXPECT_TRUE(Unicode::InitializeCollator(icuLocale, false));
  var = UnicodeUtils::Collate(s3, s4); // No (extra) Normalization
  EXPECT_NE(var, 0);
}
/*! \brief Compares two strings based on the rules of the given locale
 *
 *  TODO: DRAFT
 *
 *  This is a complex comparison. Broadly speaking, it tries to compare
 *  numbers using math rules and Alphabetic characters as words in a caseless
 *  manner. Punctuation characters, etc. are also included in comparison.
 *
 * \param left string to compare
 * \param right other string to compare
 * \param locale supplies rules for comparison
 * \return < 0 if left < right based upon comparison based on comparison rules
 */
/*
  static int64_t AlphaNumericCompare(const wchar_t* left,
                                     const wchar_t* right,
                                     const std::locale& locale);
 */
/*! \brief Compares two strings based on the rules of LocaleInfo.GestSystemLocale
 *
 *  TODO: DRAFT
 *
 *  This is a complex comparison. Broadly speaking, it tries to compare
 *  numbers using math rules and Alphabetic characters as words in a caseless
 *  manner. Punctuation characters, etc. are also included in comparison.
 *
 * \param left string to compare
 * \param right other string to compare
 * \param locale supplies rules for comparison
 * \return < 0 if left < right based upon comparison based on comparison rules
 */
/*
  static int64_t AlphaNumericCompare(const wchar_t* left, const wchar_t* right);
 */


TEST(TestUnicodeUtils, AlphaNumericCompare)
{
  int64_t ref;
  int64_t var;

  ref = 0;
  var = UnicodeUtils::AlphaNumericCompare(L"123abc", L"abc123");
  EXPECT_LT(var, ref);
}

/*!
 * SQLite collating function, see sqlite3_create_collation
 * The equivalent of AlphaNumericCompare() but for comparing UTF8 encoded data using
 * LangInfo::GetSystemLocale
 *
 * This only processes enough data to find a difference, and avoids expensive data conversions.
 * When sorting in memory item data is converted once to wstring in advance prior to sorting, the
 * SQLite callback function can not do that kind of preparation. Instead, in order to use
 * AlphaNumericCompare(), it would have to repeatedly convert the full input data to wstring for
 * every pair comparison made. That approach was found to be 10 times slower than using this
 * separate routine.
 *
 * /param nKey1 byte-length of first UTF-8 string
 * /param pKey1 pointer to byte array for the first UTF-8 string to compare
 * /param nKey2 byte-length of second UTF-8 string
 * /param pKey2 pointer to byte array for the second UTF-8 string to compare
 * /return 0 if strings are the same,
 *       < 0 if first string should come before the second
 *       > 0 if the first string should come after the second
 */
/*
  static int AlphaNumericCollation(int nKey1, const void* pKey1, int nKey2, const void* pKey2);
/*
  /*!
 * \brief converts timeString (hh:mm:ss or nnn min) to seconds.
 *
 * \param timeString string to convert to seconds may be in "hh:mm:ss" or "nnn min" format
 *                   missing values are assumed zero. Whitespace is trimmed first.
 * \return parsed value in seconds
 *
 *   ex: " 14:57 " or "  23 min"
 */
//   static long TimeStringToSeconds(const std::string_view& timeString);

TEST(TestUnicodeUtils, TimeStringToSeconds)
{
  EXPECT_EQ(77455, UnicodeUtils::TimeStringToSeconds("21:30:55"));
  EXPECT_EQ(7 * 60, UnicodeUtils::TimeStringToSeconds("7 min"));
  EXPECT_EQ(7 * 60, UnicodeUtils::TimeStringToSeconds("7 min\t"));
  EXPECT_EQ(154 * 60, UnicodeUtils::TimeStringToSeconds("   154 min"));
  EXPECT_EQ(1 * 60 + 1, UnicodeUtils::TimeStringToSeconds("1:01"));
  EXPECT_EQ(4 * 60 + 3, UnicodeUtils::TimeStringToSeconds("4:03"));
  EXPECT_EQ(2 * 3600 + 4 * 60 + 3, UnicodeUtils::TimeStringToSeconds("2:04:03"));
  EXPECT_EQ(2 * 3600 + 4 * 60 + 3, UnicodeUtils::TimeStringToSeconds("   2:4:3"));
  EXPECT_EQ(2 * 3600 + 4 * 60 + 3, UnicodeUtils::TimeStringToSeconds("  \t\t 02:04:03 \n "));
  EXPECT_EQ(1 * 3600 + 5 * 60 + 2, UnicodeUtils::TimeStringToSeconds("01:05:02:04:03 \n "));
  EXPECT_EQ(0, UnicodeUtils::TimeStringToSeconds("blah"));
  EXPECT_EQ(0, UnicodeUtils::TimeStringToSeconds("ля-ля"));
}

/*!
 * \brief Strip any trailing \n and \r characters.
 *
 * \param strLine input string to have consecutive trailing \n and \r characters removed in-place
 */
//  static void RemoveCRLF(std::string& strLine);

TEST(TestUnicodeUtils, RemoveCRLF)
{
  std::string refstr;
  std::string varstr;

  refstr = "test\r\nstring\nblah blah";
  varstr = "test\r\nstring\nblah blah\n";
  UnicodeUtils::RemoveCRLF(varstr);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());
}


/*!
 * \brief detects when a string contains non-ASCII to aide in debugging or error reporting
 *
 * \param str String to be examined for non-ASCII
 * \return true if non-ASCII characters found, otherwise false
 */
// inline static bool ContainsNonAscii(const std::string_view str)

/*!
 * \brief detects when a wstring contains non-ASCII to aide in debugging or error reporting
 *
 * \param str String to be examined for non-ASCII
 * \return true if non-ASCII characters found, otherwise false
 */
// inline static bool ContainsNonAscii(const std::wstring_view str)



/*!
 * \brief Determine if "word" is present in string
 *
 * \param str string to search
 * \param word to search for in str
 * \return true if word found, otherwise false
 *
 * Search algorithm:
 *   Both str and word are case-folded (see FoldCase)
 *   For each character in str
 *     Return false if word not found in str
 *     Return true if word found starting at beginning of substring
 *     If partial match found:
 *      If non-matching character is a digit, then skip past every
 *      digit in str. Same for Latin letters. Otherwise, skip one character
 *      Skip any whitespace characters
 */

// static bool FindWord(const std::string_view& str, const std::string_view& word);

TEST(TestUnicodeUtils, FindWord)
{
  bool ref;
  bool var;

  ref = true;
  var = UnicodeUtils::FindWord("test string", "string");
  EXPECT_EQ(ref, var);
  var = UnicodeUtils::FindWord("12345string", "string");
  EXPECT_EQ(ref, var);
  var = UnicodeUtils::FindWord("apple2012", "2012");
  EXPECT_EQ(ref, var);

  ref = false;
  var = UnicodeUtils::FindWord("12345string", "ring");
  EXPECT_EQ(ref, var);
  var = UnicodeUtils::FindWord("12345string", "345");
  EXPECT_EQ(ref, var);
  var = UnicodeUtils::FindWord("apple2012", "e2012");
  EXPECT_EQ(ref, var);
  var = UnicodeUtils::FindWord("apple2012", "12");
  EXPECT_EQ(ref, var);
  var = UnicodeUtils::FindWord("anyt]h(z_iILi234#!? 1a34#56bbc7 ", "1a34#56bbc7");
  ref = true;
  EXPECT_EQ(ref, var);
}

TEST(TestUnicodeUtils, FindWord_NonAscii)
{
  bool ref;
  bool var;

  ref = true;
  var = UnicodeUtils::FindWord("我的视频", "视频");
  EXPECT_EQ(ref, var);
  var = UnicodeUtils::FindWord("我的视频", "视");
  EXPECT_EQ(ref, var);
  ref = true;
  var = UnicodeUtils::FindWord("Apple ple", "ple");
  EXPECT_EQ(ref, var);
  ref = false;
  var = UnicodeUtils::FindWord("Apple ple", "le");
  EXPECT_EQ(ref, var);

  ref = true;
  var = UnicodeUtils::FindWord(" Apple ple", " Apple");
  EXPECT_EQ(ref, var);

  ref = true;
  var = UnicodeUtils::FindWord(" Apple ple", "Apple");
  EXPECT_EQ(ref, var);

  ref = true;
  var = UnicodeUtils::FindWord("Äpfel.pfel", "pfel");
  EXPECT_EQ(ref, var);

  ref = false;
  var = UnicodeUtils::FindWord("Äpfel.pfel", "pfeldumpling");
  EXPECT_EQ(ref, var);

  ref = true;
  var = UnicodeUtils::FindWord("Äpfel.pfel", "Äpfel.pfel");
  EXPECT_EQ(ref, var);

  // Yea old Turkic I problem....
  ref = true;
  var = UnicodeUtils::FindWord("abcçdefgğh ıİi jklmnoöprsştuüvyz", "ıiİ jklmnoöprsştuüvyz");
}

/*!
 * \brief Converts a date string into an integer format
 *
 * \param dateString to be converted. See note
 * \return integer format of dateString. See note
 *
 * No validation of dateString is performed. It is assumed to be
 * in one of the following formats:
 *    YYYY-DD-MM, YYYY--DD, YYYY
 *
 *    Examples:
 *      1974-10-18 => 19741018
 *      1974-10    => 197410
 *      1974       => 1974
 */
//  static int DateStringToYYYYMMDD(const std::string& dateString);

TEST(TestUnicodeUtils, DateStringToYYYYMMDD)
{
  // Must accept: YYYY, YYYY-MM, YYYY-MM-DD
  int ref;
  int var;

  ref = 20120706;
  var = UnicodeUtils::DateStringToYYYYMMDD("2012-07-06");
  EXPECT_EQ(ref, var);

  ref = 201207;
  var = UnicodeUtils::DateStringToYYYYMMDD("2012-07");
  EXPECT_EQ(ref, var);

  ref = 2012;
  var = UnicodeUtils::DateStringToYYYYMMDD("2012");
  EXPECT_EQ(ref, var);
}


/*!
 * \brief Escapes the given string to be able to be used as a parameter.
 *
 * Escapes backslashes and double-quotes with an additional backslash and
 * adds double-quotes around the whole string.
 *
 * \param param String to escape/paramify
 * \return Escaped/Paramified string
 */
// static std::string Paramify(const std::string_view& param);

/*
struct sortstringbyname
{
  bool operator()(const std::string_view& strItem1, const std::string_view& strItem2) const
  {
    return UnicodeUtils::CompareNoCase(strItem1, strItem2) < 0;
  }
 */
TEST(TestUnicodeUtils, sortstringbyname)
{
  std::vector<std::string> strarray;
  strarray.emplace_back("B");
  strarray.emplace_back("c");
  strarray.emplace_back("a");
  std::sort(strarray.begin(), strarray.end(), sortstringbyname());

  EXPECT_STREQ("a", strarray[0].c_str());
  EXPECT_STREQ("B", strarray[1].c_str());
  EXPECT_STREQ("c", strarray[2].c_str());
}

TEST(TestUnicodeUtils, Paramify)
{
  // TODO: Something broken about these tests

  std::string input;
  std::string expectedResult;
  std::string result;

  input = "Vanilla string";
  expectedResult = R"("Vanilla string")";
  result = UnicodeUtils::Paramify(input);
  EXPECT_STREQ(result.c_str(), expectedResult.c_str());

  input = "\\";
  expectedResult = R"("\\")";
  result = UnicodeUtils::Paramify(input);
  EXPECT_STREQ(result.c_str(), expectedResult.c_str());

  input = R"(")";
  expectedResult = R"("\"")";
  result = UnicodeUtils::Paramify(input);
  EXPECT_STREQ(result.c_str(), expectedResult.c_str());

  input = R"("""Three quotes \\\ Three slashes "\"\\""\\\""")";
  expectedResult = R"("\"\"\"Three quotes \\\\\\ Three slashes \"\\\"\\\\\"\"\\\\\\\"\"\"")";
  result = UnicodeUtils::Paramify(input);
  EXPECT_STREQ(result.c_str(), expectedResult.c_str());
}

static std::string CopyStringUtest(std::string_view title,  std::string_view label, std::string_view language, std::string_view text, int iterations,
    std::chrono::steady_clock::time_point clock);

static std::string RunToUnicodeStringTest(std::string_view title, std::string_view label,  std::string_view language, std::string_view text, int iterations,
    std::chrono::steady_clock::time_point clock);

static std::string RunHeaplessUTF8ToUTF16Test(std::string_view title,  std::string_view label, std::string_view language, std::string_view text, int iterations,
    std::chrono::steady_clock::time_point clock);

static std::string RunHeapLessUTF16ToUTF8(std::string_view title, std::string_view label, std::string_view language, std::string_view text, int iterations,
    std::chrono::steady_clock::time_point clock);

static std::string RunUnicodeStringToUTF8(std::string_view title, std::string_view label, std::string_view language, std::string_view text, int iterations,
    std::chrono::steady_clock::time_point clock);

static std::string TestFoldUTF16(std::string_view title, std::string_view label, std::string_view language, std::string_view text, int iterations,
    std::chrono::steady_clock::time_point clock);

static std::string TestFoldUnicodeString(std::string_view title, std::string_view label, std::string_view language, std::string_view text, int iterations,
    std::chrono::steady_clock::time_point clock);

static std::string TestEndToEndFold (std::string_view title, std::string_view label, std::string_view language, std::string_view text, int iterations,
    std::chrono::steady_clock::time_point clock);

static std::string TestEndToEndFold_w (std::string_view title, std::string_view label, std::string_view language, std::string_view text, int iterations,
    std::chrono::steady_clock::time_point clock);

static std::string TestEndToEndFoldUnicodeString (std::string_view title, std::string_view label, std::string_view language, std::string_view text, int iterations,
    std::chrono::steady_clock::time_point clock);

static std::string TestEndToEndLeft(std::string_view title, std::string_view label, std::string_view language, std::string_view text, int iterations,
    std::chrono::steady_clock::time_point clock);

static std::string TestEndToEndLeftUnicodeString (std::string_view title, std::string_view label, std::string_view language, std::string_view text, int iterations,
    std::chrono::steady_clock::time_point clock);

static std::string TestEndToEndEqualsNoCase (std::string_view title, std::string_view label, std::string_view language, std::string_view text, int iterations,
    std::chrono::steady_clock::time_point clock);

static std::string TestEndToEndEqualsNoCaseNormalize (std::string_view title, std::string_view label, std::string_view language, std::string_view text, int iterations,
    std::chrono::steady_clock::time_point clock);

static std::string TestEndToEndEqualsNoCaseUnicodeString (std::string_view title, std::string_view label, std::string_view language, std::string_view text, int iterations,
    std::chrono::steady_clock::time_point clock);

static std::string TestEndToEndEqualsNoCaseNormalizeUnicodeString (std::string_view title, std::string_view label, std::string_view language, std::string_view text, int iterations,
    std::chrono::steady_clock::time_point clock);


static std::string TestEndToEndCollateUTF16 (std::string_view title, std::string_view label, std::string_view language, std::string_view text, int iterations,
    std::chrono::steady_clock::time_point clock);

static std::string TestEndToEndCollateWString (std::string_view title, std::string_view label, std::string_view language, std::string_view text, int iterations,
    std::chrono::steady_clock::time_point clock);

static std::string appendStats(std::string_view label, std::string_view language, std::string_view text, int micros, int iterations);

TEST(TestUnicodeUtils, Performance_All)
{
  std::string refstr = "test";
  std::string input;

  input = "Vanilla string";
  std::string expectedResult = R"("Vanilla string")";
  bool result = UnicodeUtils::StartsWith(input, expectedResult);
  EXPECT_TRUE(result);
  EXPECT_FALSE(UnicodeUtils::StartsWith(refstr, "x"));

  //std::basic_ostringstream<char> ss;
  const std::string testStr1 = "This is a short string";
  const std::string testStr2 = "This is longer 123456789012345678901234567890123456789012345678901234567890";
  const std::string testStr3 = "Note: This function serves a similar purpose that ToLower/ToUpper is \
 *       frequently used for ASCII maps indexed by keyword. ToLower/ToUpper does \
 *       NOT work to 'Normalize' Unicode to a consistent value regardless of\
 *       the case variations of the input string. A particular problem is the behavior\
*       of \"The Turkic I\". FOLD_CASE_DEFAULT is effective at \
 *       eliminating this problem. Below are the four \"I\" characters in\
 *       Turkic and the result of FoldCase for each";
  std::string longRussian = {TestUnicodeUtils::UTF8_LONG_RUSSIAN};
  std::string longChinese = {TestUnicodeUtils::UTF8_LONG_CHINESE};

  int hundredThousand = 100000;
  std::string msg;
  std::string title;
  std::string language;
  std::string text;
  std::chrono::steady_clock::time_point clock;

  std::string english = "English";
  std::string russian = "Russian";
  std::string chinese = "Chinese";
  std::tuple<std::string, std::string> t1 {english, testStr1};
  std::tuple<std::string, std::string> t2 (english, testStr2);
  std::tuple<std::string, std::string> t3 (english, testStr3);
  std::tuple<std::string, std::string> t4 (russian, longRussian);
  std::tuple<std::string, std::string> t5 (chinese, longChinese);

  std::vector<std::tuple<std::string, std::string>> TESTS {t1, t2, t3, t4, t5};

  for (auto t : TESTS)
  {

    title = "";
    std::string_view label = "";
    language = std::get<0>(t);
    text = std::get<1>(t);
    int iterations = hundredThousand;

    msg = CopyStringUtest(title, label, language, text, iterations, clock);
    EXPECT_STREQ(msg.c_str(), "");

    msg = RunToUnicodeStringTest(title, label, language, text, iterations, clock);
    EXPECT_STREQ(msg.c_str(), "");

    msg = RunHeaplessUTF8ToUTF16Test(title, label, language, text, iterations, clock);
    EXPECT_STREQ(msg.c_str(), "");

    msg = RunHeapLessUTF16ToUTF8(title, label, language, text, iterations, clock);
    EXPECT_STREQ(msg.c_str(), "");

    msg = RunUnicodeStringToUTF8(title, label, language, text, iterations, clock);
    EXPECT_STREQ(msg.c_str(), "");

    msg = TestFoldUTF16(title, label, language, text, iterations, clock);
    EXPECT_STREQ(msg.c_str(), "");

    msg = TestFoldUnicodeString(title, label, language, text, iterations, clock);
    EXPECT_STREQ(msg.c_str(), "");

    msg = TestEndToEndFold(title, label, language, text, iterations, clock);
    EXPECT_STREQ(msg.c_str(), "");

    msg = TestEndToEndFold_w(title, label, language, text, iterations, clock);
    EXPECT_STREQ(msg.c_str(), "");

    msg = TestEndToEndFoldUnicodeString(title, label, language, text, iterations, clock);
    EXPECT_STREQ(msg.c_str(), "");

    /*
    msg = TestEndToEndLeft(title, label, language, text, iterations, clock);
    EXPECT_STREQ(msg.c_str(), "");

    msg = TestEndToEndLeftUnicodeString (title, label, language, text, iterations, clock);
    EXPECT_STREQ(msg.c_str(), "");
     */

    msg = TestEndToEndEqualsNoCase (title, label, language, text, iterations, clock);
    EXPECT_STREQ(msg.c_str(), "");

    msg = TestEndToEndEqualsNoCaseNormalize(title, label, language, text, iterations, clock);
    EXPECT_STREQ(msg.c_str(), "");

    msg = TestEndToEndEqualsNoCaseUnicodeString (title, label, language, text, iterations, clock);
    EXPECT_STREQ(msg.c_str(), "");

    //    msg = TestEndToEndEqualsNoCaseNormalizeUnicodeString(title, label, language, text, iterations, clock);
    //    EXPECT_STREQ(msg.c_str(), "");

    msg = TestEndToEndCollateUTF16(title, label, language, text, iterations, clock);
    EXPECT_STREQ(msg.c_str(), "");

    msg = TestEndToEndCollateWString (title, label, language, text, iterations, clock);
    EXPECT_STREQ(msg.c_str(), "");
  }
}


TEST(TestUnicodeUtils, Performance_Basic)
{
  std::string refstr = "test";
  std::string input;

  input = "Vanilla string";
  std::string expectedResult = R"("Vanilla string")";
  bool result = UnicodeUtils::StartsWith(input, expectedResult);
  EXPECT_TRUE(result);
  EXPECT_FALSE(UnicodeUtils::StartsWith(refstr, "x"));

  //std::basic_ostringstream<char> ss;
  const std::string testStr1 = "This is a short string";
  const std::string testStr2 = "This is longer 123456789012345678901234567890123456789012345678901234567890";
  const std::string testStr3 = "Note: This function serves a similar purpose that ToLower/ToUpper is \
 *       frequently used for ASCII maps indexed by keyword. ToLower/ToUpper does \
 *       NOT work to 'Normalize' Unicode to a consistent value regardless of\
 *       the case variations of the input string. A particular problem is the behavior\
*       of \"The Turkic I\". FOLD_CASE_DEFAULT is effective at \
 *       eliminating this problem. Below are the four \"I\" characters in\
 *       Turkic and the result of FoldCase for each";
  std::string longRussian = {TestUnicodeUtils::UTF8_LONG_RUSSIAN};
  std::string longChinese = {TestUnicodeUtils::UTF8_LONG_CHINESE};

  int hundredThousand = 100000;
  std::string msg;
  std::string title;
  std::string language;
  std::string text;
  std::chrono::steady_clock::time_point clock;

  std::string english = "English";
  std::string russian = "Russian";
  std::string chinese = "Chinese";
  std::tuple<std::string, std::string> t1 {english, testStr1};
  std::tuple<std::string, std::string> t2 (english, testStr2);
  std::tuple<std::string, std::string> t3 (english, testStr3);
  std::tuple<std::string, std::string> t4 (russian, longRussian);
  std::tuple<std::string, std::string> t5 (chinese, longChinese);

  std::vector<std::tuple<std::string, std::string>> TESTS {t1, t2, t3, t4, t5};

  for (auto t : TESTS)
  {

    title = "";
    std::string_view label = "";
    language = std::get<0>(t);
    text = std::get<1>(t);
    int iterations = hundredThousand;

    msg = CopyStringUtest(title, label, language, text, iterations, clock);
    EXPECT_STREQ(msg.c_str(), "");

    msg = RunToUnicodeStringTest(title, label, language, text, iterations, clock);
    EXPECT_STREQ(msg.c_str(), "");

    msg = RunHeaplessUTF8ToUTF16Test(title, label, language, text, iterations, clock);
    EXPECT_STREQ(msg.c_str(), "");

    msg = RunHeapLessUTF16ToUTF8(title, label, language, text, iterations, clock);
    EXPECT_STREQ(msg.c_str(), "");

    msg = RunUnicodeStringToUTF8(title, label, language, text, iterations, clock);
    EXPECT_STREQ(msg.c_str(), "");

    msg = TestFoldUTF16(title, label, language, text, iterations, clock);
    EXPECT_STREQ(msg.c_str(), "");

    msg = TestFoldUnicodeString(title, label, language, text, iterations, clock);
    EXPECT_STREQ(msg.c_str(), "");
  }
}

TEST(TestUnicodeUtils, Performance_TestEndToEndFold)
{
  std::string refstr = "test";
  std::string input;

  input = "Vanilla string";
  std::string expectedResult = R"("Vanilla string")";
  bool result = UnicodeUtils::StartsWith(input, expectedResult);
  EXPECT_TRUE(result);
  EXPECT_FALSE(UnicodeUtils::StartsWith(refstr, "x"));

  //std::basic_ostringstream<char> ss;
  const std::string testStr1 = "This is a short string";
  const std::string testStr2 = "This is longer 123456789012345678901234567890123456789012345678901234567890";
  const std::string testStr3 = "Note: This function serves a similar purpose that ToLower/ToUpper is \
 *       frequently used for ASCII maps indexed by keyword. ToLower/ToUpper does \
 *       NOT work to 'Normalize' Unicode to a consistent value regardless of\
 *       the case variations of the input string. A particular problem is the behavior\
*       of \"The Turkic I\". FOLD_CASE_DEFAULT is effective at \
 *       eliminating this problem. Below are the four \"I\" characters in\
 *       Turkic and the result of FoldCase for each";
  std::string longRussian = {TestUnicodeUtils::UTF8_LONG_RUSSIAN};
  std::string longChinese = {TestUnicodeUtils::UTF8_LONG_CHINESE};

  int hundredThousand = 100000;
  std::string msg;
  std::string title;
  std::string language;
  std::string text;
  std::chrono::steady_clock::time_point clock;

  std::string english = "English";
  std::string russian = "Russian";
  std::string chinese = "Chinese";
  std::tuple<std::string, std::string> t1 {english, testStr1};
  std::tuple<std::string, std::string> t2 (english, testStr2);
  std::tuple<std::string, std::string> t3 (english, testStr3);
  std::tuple<std::string, std::string> t4 (russian, longRussian);
  std::tuple<std::string, std::string> t5 (chinese, longChinese);

  std::vector<std::tuple<std::string, std::string>> TESTS {t1, t2, t3, t4, t5};

  for (auto t : TESTS)
  {

    title = "";
    std::string_view label = "";
    language = std::get<0>(t);
    text = std::get<1>(t);
    int iterations = hundredThousand;

    msg = TestEndToEndFold(title, label, language, text, iterations, clock);
    EXPECT_STREQ(msg.c_str(), "");
  }
}


TEST(TestUnicodeUtils, Performance_TestEndToEndFold_w)
{
  std::string refstr = "test";
  std::string input;

  input = "Vanilla string";
  std::string expectedResult = R"("Vanilla string")";
  bool result = UnicodeUtils::StartsWith(input, expectedResult);
  EXPECT_TRUE(result);
  EXPECT_FALSE(UnicodeUtils::StartsWith(refstr, "x"));

  //std::basic_ostringstream<char> ss;
  const std::string testStr1 = "This is a short string";
  const std::string testStr2 = "This is longer 123456789012345678901234567890123456789012345678901234567890";
  const std::string testStr3 = "Note: This function serves a similar purpose that ToLower/ToUpper is \
 *       frequently used for ASCII maps indexed by keyword. ToLower/ToUpper does \
 *       NOT work to 'Normalize' Unicode to a consistent value regardless of\
 *       the case variations of the input string. A particular problem is the behavior\
*       of \"The Turkic I\". FOLD_CASE_DEFAULT is effective at \
 *       eliminating this problem. Below are the four \"I\" characters in\
 *       Turkic and the result of FoldCase for each";
  std::string longRussian = {TestUnicodeUtils::UTF8_LONG_RUSSIAN};
  std::string longChinese = {TestUnicodeUtils::UTF8_LONG_CHINESE};

  int hundredThousand = 100000;
  std::string msg;
  std::string title;
  std::string language;
  std::string text;
  std::chrono::steady_clock::time_point clock;

  std::string english = "English";
  std::string russian = "Russian";
  std::string chinese = "Chinese";
  std::tuple<std::string, std::string> t1 {english, testStr1};
  std::tuple<std::string, std::string> t2 (english, testStr2);
  std::tuple<std::string, std::string> t3 (english, testStr3);
  std::tuple<std::string, std::string> t4 (russian, longRussian);
  std::tuple<std::string, std::string> t5 (chinese, longChinese);

  std::vector<std::tuple<std::string, std::string>> TESTS {t1, t2, t3, t4, t5};

  for (auto t : TESTS)
  {

    title = "";
    std::string_view label = "";
    language = std::get<0>(t);
    text = std::get<1>(t);
    int iterations = hundredThousand;

    msg = TestEndToEndFold_w(title, label, language, text, iterations, clock);
    EXPECT_STREQ(msg.c_str(), "");
  }
}

TEST(TestUnicodeUtils, Performance_TestEndToEndFoldUnicodeString)
{
  std::string refstr = "test";
  std::string input;

  input = "Vanilla string";
  std::string expectedResult = R"("Vanilla string")";
  bool result = UnicodeUtils::StartsWith(input, expectedResult);
  EXPECT_TRUE(result);
  EXPECT_FALSE(UnicodeUtils::StartsWith(refstr, "x"));

  //std::basic_ostringstream<char> ss;
  const std::string testStr1 = "This is a short string";
  const std::string testStr2 = "This is longer 123456789012345678901234567890123456789012345678901234567890";
  const std::string testStr3 = "Note: This function serves a similar purpose that ToLower/ToUpper is \
 *       frequently used for ASCII maps indexed by keyword. ToLower/ToUpper does \
 *       NOT work to 'Normalize' Unicode to a consistent value regardless of\
 *       the case variations of the input string. A particular problem is the behavior\
*       of \"The Turkic I\". FOLD_CASE_DEFAULT is effective at \
 *       eliminating this problem. Below are the four \"I\" characters in\
 *       Turkic and the result of FoldCase for each";
  std::string longRussian = {TestUnicodeUtils::UTF8_LONG_RUSSIAN};
  std::string longChinese = {TestUnicodeUtils::UTF8_LONG_CHINESE};

  int hundredThousand = 100000;
  std::string msg;
  std::string title;
  std::string language;
  std::string text;
  std::chrono::steady_clock::time_point clock;

  std::string english = "English";
  std::string russian = "Russian";
  std::string chinese = "Chinese";
  std::tuple<std::string, std::string> t1 {english, testStr1};
  std::tuple<std::string, std::string> t2 (english, testStr2);
  std::tuple<std::string, std::string> t3 (english, testStr3);
  std::tuple<std::string, std::string> t4 (russian, longRussian);
  std::tuple<std::string, std::string> t5 (chinese, longChinese);

  std::vector<std::tuple<std::string, std::string>> TESTS {t1, t2, t3, t4, t5};

  for (auto t : TESTS)
  {

    title = "";
    std::string_view label = "";
    language = std::get<0>(t);
    text = std::get<1>(t);
    int iterations = hundredThousand;

    msg = TestEndToEndFoldUnicodeString(title, label, language, text, iterations, clock);
    EXPECT_STREQ(msg.c_str(), "");
  }
}

TEST(TestUnicodeUtils, Performance_Left)
{
  std::string refstr = "test";
  std::string input;

  input = "Vanilla string";
  std::string expectedResult = R"("Vanilla string")";
  bool result = UnicodeUtils::StartsWith(input, expectedResult);
  EXPECT_TRUE(result);
  EXPECT_FALSE(UnicodeUtils::StartsWith(refstr, "x"));

  //std::basic_ostringstream<char> ss;
  const std::string testStr1 = "This is a short string";
  const std::string testStr2 = "This is longer 123456789012345678901234567890123456789012345678901234567890";
  const std::string testStr3 = "Note: This function serves a similar purpose that ToLower/ToUpper is \
 *       frequently used for ASCII maps indexed by keyword. ToLower/ToUpper does \
 *       NOT work to 'Normalize' Unicode to a consistent value regardless of\
 *       the case variations of the input string. A particular problem is the behavior\
*       of \"The Turkic I\". FOLD_CASE_DEFAULT is effective at \
 *       eliminating this problem. Below are the four \"I\" characters in\
 *       Turkic and the result of FoldCase for each";
  std::string longRussian = {TestUnicodeUtils::UTF8_LONG_RUSSIAN};
  std::string longChinese = {TestUnicodeUtils::UTF8_LONG_CHINESE};

  int hundredThousand = 100000;
  std::string msg;
  std::string title;
  std::string language;
  std::string text;
  std::chrono::steady_clock::time_point clock;

  std::string english = "English";
  std::string russian = "Russian";
  std::string chinese = "Chinese";
  std::tuple<std::string, std::string> t1 {english, testStr1};
  std::tuple<std::string, std::string> t2 (english, testStr2);
  std::tuple<std::string, std::string> t3 (english, testStr3);
  std::tuple<std::string, std::string> t4 (russian, longRussian);
  std::tuple<std::string, std::string> t5 (chinese, longChinese);

  std::vector<std::tuple<std::string, std::string>> TESTS {t1, t2, t3, t4, t5};

  for (auto t : TESTS)
  {

    title = "";
    std::string_view label = "";
    language = std::get<0>(t);
    text = std::get<1>(t);
    int iterations = hundredThousand;


    /*
    msg = TestEndToEndLeft(title, label, language, text, iterations, clock);
    EXPECT_STREQ(msg.c_str(), "");

    msg = TestEndToEndLeftUnicodeString (title, label, language, text, iterations, clock);
    EXPECT_STREQ(msg.c_str(), "");
     */

  }
}

TEST(TestUnicodeUtils, Performance_TestEndToEndEqualsNoCase)
{
  std::string refstr = "test";
  std::string input;

  input = "Vanilla string";
  std::string expectedResult = R"("Vanilla string")";
  bool result = UnicodeUtils::StartsWith(input, expectedResult);
  EXPECT_TRUE(result);
  EXPECT_FALSE(UnicodeUtils::StartsWith(refstr, "x"));

  //std::basic_ostringstream<char> ss;
  const std::string testStr1 = "This is a short string";
  const std::string testStr2 = "This is longer 123456789012345678901234567890123456789012345678901234567890";
  const std::string testStr3 = "Note: This function serves a similar purpose that ToLower/ToUpper is \
 *       frequently used for ASCII maps indexed by keyword. ToLower/ToUpper does \
 *       NOT work to 'Normalize' Unicode to a consistent value regardless of\
 *       the case variations of the input string. A particular problem is the behavior\
*       of \"The Turkic I\". FOLD_CASE_DEFAULT is effective at \
 *       eliminating this problem. Below are the four \"I\" characters in\
 *       Turkic and the result of FoldCase for each";
  std::string longRussian = {TestUnicodeUtils::UTF8_LONG_RUSSIAN};
  std::string longChinese = {TestUnicodeUtils::UTF8_LONG_CHINESE};

  int hundredThousand = 100000;
  std::string msg;
  std::string title;
  std::string language;
  std::string text;
  std::chrono::steady_clock::time_point clock;

  std::string english = "English";
  std::string russian = "Russian";
  std::string chinese = "Chinese";
  std::tuple<std::string, std::string> t1 {english, testStr1};
  std::tuple<std::string, std::string> t2 (english, testStr2);
  std::tuple<std::string, std::string> t3 (english, testStr3);
  std::tuple<std::string, std::string> t4 (russian, longRussian);
  std::tuple<std::string, std::string> t5 (chinese, longChinese);

  std::vector<std::tuple<std::string, std::string>> TESTS {t1, t2, t3, t4, t5};

  for (auto t : TESTS)
  {

    title = "";
    std::string_view label = "";
    language = std::get<0>(t);
    text = std::get<1>(t);
    int iterations = hundredThousand;

    msg = TestEndToEndEqualsNoCase (title, label, language, text, iterations, clock);
    EXPECT_STREQ(msg.c_str(), "");

    //    msg = TestEndToEndEqualsNoCaseNormalizeUnicodeString(title, label, language, text, iterations, clock);
    //    EXPECT_STREQ(msg.c_str(), "");

  }
}


TEST(TestUnicodeUtils, Performance_TestEndToEndEqualsNoCaseNormalize)
{
  std::string refstr = "test";
  std::string input;

  input = "Vanilla string";
  std::string expectedResult = R"("Vanilla string")";
  bool result = UnicodeUtils::StartsWith(input, expectedResult);
  EXPECT_TRUE(result);
  EXPECT_FALSE(UnicodeUtils::StartsWith(refstr, "x"));

  //std::basic_ostringstream<char> ss;
  const std::string testStr1 = "This is a short string";
  const std::string testStr2 = "This is longer 123456789012345678901234567890123456789012345678901234567890";
  const std::string testStr3 = "Note: This function serves a similar purpose that ToLower/ToUpper is \
 *       frequently used for ASCII maps indexed by keyword. ToLower/ToUpper does \
 *       NOT work to 'Normalize' Unicode to a consistent value regardless of\
 *       the case variations of the input string. A particular problem is the behavior\
*       of \"The Turkic I\". FOLD_CASE_DEFAULT is effective at \
 *       eliminating this problem. Below are the four \"I\" characters in\
 *       Turkic and the result of FoldCase for each";
  std::string longRussian = {TestUnicodeUtils::UTF8_LONG_RUSSIAN};
  std::string longChinese = {TestUnicodeUtils::UTF8_LONG_CHINESE};

  int hundredThousand = 100000;
  std::string msg;
  std::string title;
  std::string language;
  std::string text;
  std::chrono::steady_clock::time_point clock;

  std::string english = "English";
  std::string russian = "Russian";
  std::string chinese = "Chinese";
  std::tuple<std::string, std::string> t1 {english, testStr1};
  std::tuple<std::string, std::string> t2 (english, testStr2);
  std::tuple<std::string, std::string> t3 (english, testStr3);
  std::tuple<std::string, std::string> t4 (russian, longRussian);
  std::tuple<std::string, std::string> t5 (chinese, longChinese);

  std::vector<std::tuple<std::string, std::string>> TESTS {t1, t2, t3, t4, t5};

  for (auto t : TESTS)
  {

    title = "";
    std::string_view label = "";
    language = std::get<0>(t);
    text = std::get<1>(t);
    int iterations = hundredThousand;

    msg = TestEndToEndEqualsNoCaseNormalize(title, label, language, text, iterations, clock);
    EXPECT_STREQ(msg.c_str(), "");


    //    msg = TestEndToEndEqualsNoCaseNormalizeUnicodeString(title, label, language, text, iterations, clock);
    //    EXPECT_STREQ(msg.c_str(), "");

  }
}


TEST(TestUnicodeUtils, Performance_TestEndToEndEqualsNoCaseUnicodeString )
{
  std::string refstr = "test";
  std::string input;

  input = "Vanilla string";
  std::string expectedResult = R"("Vanilla string")";
  bool result = UnicodeUtils::StartsWith(input, expectedResult);
  EXPECT_TRUE(result);
  EXPECT_FALSE(UnicodeUtils::StartsWith(refstr, "x"));

  //std::basic_ostringstream<char> ss;
  const std::string testStr1 = "This is a short string";
  const std::string testStr2 = "This is longer 123456789012345678901234567890123456789012345678901234567890";
  const std::string testStr3 = "Note: This function serves a similar purpose that ToLower/ToUpper is \
 *       frequently used for ASCII maps indexed by keyword. ToLower/ToUpper does \
 *       NOT work to 'Normalize' Unicode to a consistent value regardless of\
 *       the case variations of the input string. A particular problem is the behavior\
*       of \"The Turkic I\". FOLD_CASE_DEFAULT is effective at \
 *       eliminating this problem. Below are the four \"I\" characters in\
 *       Turkic and the result of FoldCase for each";
  std::string longRussian = {TestUnicodeUtils::UTF8_LONG_RUSSIAN};
  std::string longChinese = {TestUnicodeUtils::UTF8_LONG_CHINESE};

  int hundredThousand = 100000;
  std::string msg;
  std::string title;
  std::string language;
  std::string text;
  std::chrono::steady_clock::time_point clock;

  std::string english = "English";
  std::string russian = "Russian";
  std::string chinese = "Chinese";
  std::tuple<std::string, std::string> t1 {english, testStr1};
  std::tuple<std::string, std::string> t2 (english, testStr2);
  std::tuple<std::string, std::string> t3 (english, testStr3);
  std::tuple<std::string, std::string> t4 (russian, longRussian);
  std::tuple<std::string, std::string> t5 (chinese, longChinese);

  std::vector<std::tuple<std::string, std::string>> TESTS {t1, t2, t3, t4, t5};

  for (auto t : TESTS)
  {

    title = "";
    std::string_view label = "";
    language = std::get<0>(t);
    text = std::get<1>(t);
    int iterations = hundredThousand;

    msg = TestEndToEndEqualsNoCaseUnicodeString (title, label, language, text, iterations, clock);
    EXPECT_STREQ(msg.c_str(), "");

    //    msg = TestEndToEndEqualsNoCaseNormalizeUnicodeString(title, label, language, text, iterations, clock);
    //    EXPECT_STREQ(msg.c_str(), "");

  }
}

TEST(TestUnicodeUtils, Performance_CollateU16)
{
  std::string refstr = "test";
  std::string input;

  input = "Vanilla string";
  std::string expectedResult = R"("Vanilla string")";
  bool result = UnicodeUtils::StartsWith(input, expectedResult);
  EXPECT_TRUE(result);
  EXPECT_FALSE(UnicodeUtils::StartsWith(refstr, "x"));

  //std::basic_ostringstream<char> ss;
  const std::string testStr1 = "This is a short string";
  const std::string testStr2 = "This is longer 123456789012345678901234567890123456789012345678901234567890";
  const std::string testStr3 = "Note: This function serves a similar purpose that ToLower/ToUpper is \
 *       frequently used for ASCII maps indexed by keyword. ToLower/ToUpper does \
 *       NOT work to 'Normalize' Unicode to a consistent value regardless of\
 *       the case variations of the input string. A particular problem is the behavior\
*       of \"The Turkic I\". FOLD_CASE_DEFAULT is effective at \
 *       eliminating this problem. Below are the four \"I\" characters in\
 *       Turkic and the result of FoldCase for each";
  std::string longRussian = {TestUnicodeUtils::UTF8_LONG_RUSSIAN};
  std::string longChinese = {TestUnicodeUtils::UTF8_LONG_CHINESE};

  int hundredThousand = 100000;
  std::string msg;
  std::string title;
  std::string language;
  std::string text;
  std::chrono::steady_clock::time_point clock;

  std::string english = "English";
  std::string russian = "Russian";
  std::string german = "German";

  // testStr1 ignored. Sort data read from files.

  std::tuple<std::string, std::string> t1 {german, testStr1};

  std::vector<std::tuple<std::string, std::string>> TESTS {t1};

  for (auto t : TESTS)
  {
    title = "";
    std::string_view label = "Collate";
    language = std::get<0>(t);
    text = std::get<1>(t);
    int iterations = hundredThousand;

    msg = TestEndToEndCollateUTF16(title, label, language, text, iterations, clock);
    EXPECT_STREQ(msg.c_str(), "");
  }
}
TEST(TestUnicodeUtils, Performance_CollateWstr)
{
  std::string refstr = "test";
  std::string input;

  input = "Vanilla string";
  std::string expectedResult = R"("Vanilla string")";
  bool result = UnicodeUtils::StartsWith(input, expectedResult);
  EXPECT_TRUE(result);
  EXPECT_FALSE(UnicodeUtils::StartsWith(refstr, "x"));

  //std::basic_ostringstream<char> ss;
  const std::string testStr1 = "This is a short string";
  const std::string testStr2 = "This is longer 123456789012345678901234567890123456789012345678901234567890";
  const std::string testStr3 = "Note: This function serves a similar purpose that ToLower/ToUpper is \
 *       frequently used for ASCII maps indexed by keyword. ToLower/ToUpper does \
 *       NOT work to 'Normalize' Unicode to a consistent value regardless of\
 *       the case variations of the input string. A particular problem is the behavior\
*       of \"The Turkic I\". FOLD_CASE_DEFAULT is effective at \
 *       eliminating this problem. Below are the four \"I\" characters in\
 *       Turkic and the result of FoldCase for each";
  std::string longRussian = {TestUnicodeUtils::UTF8_LONG_RUSSIAN};
  std::string longChinese = {TestUnicodeUtils::UTF8_LONG_CHINESE};

  int hundredThousand = 100000;
  std::string msg;
  std::string title;
  std::string language;
  std::string text;
  std::chrono::steady_clock::time_point clock;

  std::string english = "English";
  std::string russian = "Russian";
  std::string german = "German";

  // testStr1 ignored. Sort data read from files.

  std::tuple<std::string, std::string> t1 {german, testStr1};

  std::vector<std::tuple<std::string, std::string>> TESTS {t1};

  for (auto t : TESTS)
  {
    title = "";
    std::string_view label = "Collate";
    language = std::get<0>(t);
    text = std::get<1>(t);
    int iterations = hundredThousand;

    msg =  TestEndToEndCollateWString(title, label, language, text, iterations, clock);
    EXPECT_STREQ(msg.c_str(), "");
  }
}


static std::string CopyStringUtest(std::string_view title, std::string_view label, std::string_view language, std::string_view text, int iterations,
    std::chrono::steady_clock::time_point clock)
{

  clock = std::chrono::steady_clock::now();

  for (int i = 0; i <iterations; i++)
  {
    std::string tmp = std::string(text);
  }
  // muntrace();
  std::chrono::steady_clock::time_point stop_time = std::chrono::steady_clock::now();
  int micros =
      std::chrono::duration_cast<std::chrono::microseconds>(stop_time - clock)
      .count();
  label = "string copy";
  std::ostringstream ss;
  ss << "Create " << iterations << " " << language << " std::string copies " << title << " length: " << text.length()
          << appendStats(label, language,  text, micros, iterations);
  return ss.str();
}

static std::string RunToUnicodeStringTest(std::string_view title, std::string_view label,  std::string_view language, std::string_view text, int iterations,
    std::chrono::steady_clock::time_point clock)
{
  clock = std::chrono::steady_clock::now();


  for (int i = 0; i <iterations; i++)
  {
    icu::UnicodeString tmp = icu::UnicodeString::fromUTF8(text);
  }
  // muntrace();
  std::chrono::steady_clock::time_point stop_time = std::chrono::steady_clock::now();
  int micros =
      std::chrono::duration_cast<std::chrono::microseconds>(stop_time - clock)
      .count();
  label = "UTF8 to UnicodeString";
  std::ostringstream ss;
  ss << "Create " << iterations << " " << language << " UnicodeStrings " << title << " length: " << text.length()
          << appendStats(label, language,  text,  micros, iterations);
  return ss.str();
}

static std::string RunHeaplessUTF8ToUTF16Test(std::string_view title, std::string_view label, std::string_view language,  std::string_view text, int iterations,
    std::chrono::steady_clock::time_point clock)
{
  clock = std::chrono::steady_clock::now();


  for (int i = 0; i <iterations; i++)
  {
    std::u16string x = Unicode::TestHeapLessUTF8ToUTF16(text);
  }
  // muntrace();
  std::chrono::steady_clock::time_point stop_time = std::chrono::steady_clock::now();
  int micros =
      std::chrono::duration_cast<std::chrono::microseconds>(stop_time - clock)
      .count();
  label = "UTF8 to UTF16";
  std::ostringstream ss;
  ss << "Create " << iterations << " " << language << " UTF8 -> UTF16 strings " << title << " length: " << text.length()
           << appendStats(label, language,  text,  micros, iterations);

  return ss.str();
}

std::string RunHeapLessUTF16ToUTF8(std::string_view title, std::string_view label, std::string_view language, std::string_view text, int iterations,
    std::chrono::steady_clock::time_point clock)
{
  // Get utf16 string to translate

  std::u16string u16Text = Unicode::TestHeapLessUTF8ToUTF16(text);

  clock = std::chrono::steady_clock::now();


  for (int i = 0; i <iterations; i++)
  {
    std::string x = Unicode::TestHeapLessUTF16ToUTF8(u16Text);
  }
  // muntrace();
  std::chrono::steady_clock::time_point stop_time = std::chrono::steady_clock::now();
  int micros =
      std::chrono::duration_cast<std::chrono::microseconds>(stop_time - clock)
      .count();
  label = "UTF16 to UTF8";
  std::ostringstream ss;
  ss << "Create " << iterations << " " << language << " UTF16 -> UTF8 strings " << title << " length: " << text.length()
             << appendStats(label, language,  text,  micros, iterations);
  return ss.str();
}

std::string RunUnicodeStringToUTF8(std::string_view title, std::string_view label, std::string_view language, std::string_view text, int iterations,
    std::chrono::steady_clock::time_point clock)
{
  // Create test instance of UnicodeString
  icu::UnicodeString testUnicodeString = icu::UnicodeString::fromUTF8(text);
  clock = std::chrono::steady_clock::now();


  for (int i = 0; i <iterations; i++)
  {
    std::string tmp{""};
    tmp = testUnicodeString.toUTF8String(tmp);
  }
  // muntrace();
  std::chrono::steady_clock::time_point stop_time = std::chrono::steady_clock::now();
  int micros =
      std::chrono::duration_cast<std::chrono::microseconds>(stop_time - clock)
      .count();
  label = "UnicodeString to UTF8";
  std::ostringstream ss;
  ss << "Convert " << iterations << " " << language << " UnicodeString to UTF8 " << " length: " << title << text.length()
          << appendStats(label, language,  text,  micros, iterations);
  return ss.str();
}

std::string TestFoldUTF16(std::string_view title, std::string_view label, std::string_view language, std::string_view text, int iterations,
    std::chrono::steady_clock::time_point clock)
{
  // Get utf16 string to Fold

  std::u16string u16Text = Unicode::TestHeapLessUTF8ToUTF16(text);
  // Create buffer to hold case fold

  size_t s2BufferSize = Unicode::GetUTF16WorkingSize(u16Text.length(), 2.0);
  UChar s2Buffer[s2BufferSize];
  std::u16string_view sv2 = {s2Buffer, s2BufferSize};

  clock = std::chrono::steady_clock::now();


  for (int i = 0; i <iterations; i++)
  {
    std::u16string_view svFolded = Unicode::_FoldCase(u16Text, sv2, StringOptions::FOLD_CASE_DEFAULT);
  }
  // muntrace();
  std::chrono::steady_clock::time_point stop_time = std::chrono::steady_clock::now();
  int micros =
      std::chrono::duration_cast<std::chrono::microseconds>(stop_time - clock)
      .count();
  label = "Fold UTF16";
  std::ostringstream ss;
  ss << "" << iterations << " " << language << " UTF16 Fold " << " length: " << title << text.length()
          << appendStats(label, language,  text,  micros, iterations);

  return ss.str();
}

std::string TestFoldUnicodeString(std::string_view title, std::string_view label, std::string_view language, std::string_view text, int iterations,
    std::chrono::steady_clock::time_point clock)
{
  // Get utf16 string to Fold

  icu::UnicodeString unicodeStr = icu::UnicodeString::fromUTF8(text);
  clock = std::chrono::steady_clock::now();


  for (int i = 0; i <iterations; i++)
  {
    icu::UnicodeString folded = unicodeStr.foldCase(0);
  }
  // muntrace();
  std::chrono::steady_clock::time_point stop_time = std::chrono::steady_clock::now();
  int micros =
      std::chrono::duration_cast<std::chrono::microseconds>(stop_time - clock)
      .count();
  label = "Fold UnicodeString";
  std::ostringstream ss;
  ss << "" << iterations << " " << language << " UnicodeString Fold " << " length: " << title << text.length()
                  << appendStats(label, language,  text,  micros, iterations);
  return ss.str();
}

std::string TestEndToEndFold(std::string_view title, std::string_view label, std::string_view language, std::string_view text, int iterations,
    std::chrono::steady_clock::time_point clock)
{
  // Get utf16 string to Fold

  clock = std::chrono::steady_clock::now();


  for (int i = 0; i <iterations; i++)
  {
    std::string folded = Unicode::FoldCase(text);
  }
  // muntrace();

  std::chrono::steady_clock::time_point stop_time = std::chrono::steady_clock::now();
  int micros =
      std::chrono::duration_cast<std::chrono::microseconds>(stop_time - clock)
      .count();
  label = "e2e Fold";
  std::ostringstream ss;
  ss << "" << iterations << " " << language << " EndToEnd Fold UTF8" << " length: " << title << text.length()
                  << appendStats(label, language,  text,  micros, iterations);
  return ss.str();
}

std::string TestEndToEndFold_w (std::string_view title, std::string_view label, std::string_view language, std::string_view text, int iterations,
    std::chrono::steady_clock::time_point clock)
{
  //Convert input to wstr

  std::wstring wText = Unicode::UTF8ToWString(text);
  clock = std::chrono::steady_clock::now();


  for (int i = 0; i <iterations; i++)
  {
    std::wstring folded = Unicode::FoldCase(wText);
  }
  // muntrace();

  std::chrono::steady_clock::time_point stop_time = std::chrono::steady_clock::now();
  int micros =
      std::chrono::duration_cast<std::chrono::microseconds>(stop_time - clock)
      .count();
  label = "e2e Fold_w";
  std::ostringstream ss;
  ss << "" << iterations << " " << language << " EndToEnd Fold wstring" << " length: " << title << text.length()
                  << appendStats(label, language,  text,  micros, iterations);
  return ss.str();
}

std::string TestEndToEndFoldUnicodeString (std::string_view title, std::string_view label, std::string_view language, std::string_view text, int iterations,
    std::chrono::steady_clock::time_point clock)
{
  //Convert input to wstr
  clock = std::chrono::steady_clock::now();


  for (int i = 0; i <iterations; i++)
  {
    icu::UnicodeString unicodeStr = icu::UnicodeString::fromUTF8(text);
    icu::UnicodeString folded = unicodeStr.foldCase(0);
    std::string result{""};
    result = folded.toUTF8String(result);
  }
  // muntrace();

  std::chrono::steady_clock::time_point stop_time = std::chrono::steady_clock::now();
  int micros =
      std::chrono::duration_cast<std::chrono::microseconds>(stop_time - clock)
      .count();
  label = "e2e fold Ustring";
  std::ostringstream ss;
  ss << "" << iterations << " " << language << " EndToEnd Fold UnicodeString" << " length: " << title << text.length()
                  << appendStats(label, language,  text,  micros, iterations);
  return ss.str();
}

std::string TestEndToEndLeft(std::string_view title, std::string_view label, std::string_view language, std::string_view text, int iterations,
    std::chrono::steady_clock::time_point clock)
{
  //Convert input to wstr
  clock = std::chrono::steady_clock::now();


  for (int i = 0; i <iterations; i++)
  {
    icu::UnicodeString unicodeStr = icu::UnicodeString::fromUTF8(text);
    icu::UnicodeString folded = unicodeStr.foldCase(0);
    std::string result{""};
    result = folded.toUTF8String(result);
  }
  // muntrace();

  std::chrono::steady_clock::time_point stop_time = std::chrono::steady_clock::now();
  int micros =
      std::chrono::duration_cast<std::chrono::microseconds>(stop_time - clock)
      .count();
  label = "e2e left";
  std::ostringstream ss;
  ss << "" << iterations << " " << language << " EndToEnd Left" << " length: " << title << text.length()
                  << appendStats(label, language,  text,  micros, iterations);
  return ss.str();
}

std::string TestEndToEndLeftUnicodeString (std::string_view title, std::string_view label, std::string_view language, std::string_view text, int iterations,
    std::chrono::steady_clock::time_point clock)
{
  //Convert input to wstr
  clock = std::chrono::steady_clock::now();


  for (int i = 0; i <iterations; i++)
  {
    icu::UnicodeString unicodeStr = icu::UnicodeString::fromUTF8(text);
    icu::UnicodeString folded = unicodeStr.foldCase(0);
  }
  // muntrace();

  std::chrono::steady_clock::time_point stop_time = std::chrono::steady_clock::now();
  int micros =
      std::chrono::duration_cast<std::chrono::microseconds>(stop_time - clock)
      .count();
  label = "e2e Left Ustring";
  std::ostringstream ss;
  ss << "" << iterations << " " << language << " EndToEnd Left UnicodeString" << " length: " << title << text.length()
                  << appendStats(label, language,  text,  micros, iterations);
  return ss.str();
}

std::string TestEndToEndEqualsNoCase (std::string_view title, std::string_view label, std::string_view language, std::string_view text, int iterations,
    std::chrono::steady_clock::time_point clock)
{
  //Convert input to wstr
  clock = std::chrono::steady_clock::now();
  std::string text2{text};


  for (int i = 0; i <iterations; i++)
  {
    bool equals = Unicode::StrCaseCmp(text, text2, StringOptions::FOLD_CASE_DEFAULT);
  }
  // muntrace();

  std::chrono::steady_clock::time_point stop_time = std::chrono::steady_clock::now();
  int micros =
      std::chrono::duration_cast<std::chrono::microseconds>(stop_time - clock)
      .count();
  label = "e2e EqualsNoCase";
  std::ostringstream ss;
  ss << "" << iterations << " " << language << " EndToEnd EqualsNoCase" << " length: " << title << text.length()
                  << appendStats(label, language,  text,  micros, iterations);
  return ss.str();
}


std::string TestEndToEndEqualsNoCaseNormalize (std::string_view title, std::string_view label, std::string_view language, std::string_view text, int iterations,
    std::chrono::steady_clock::time_point clock)
{
  //Convert input to wstr
  clock = std::chrono::steady_clock::now();
  std::string text2{text};


  for (int i = 0; i <iterations; i++)
  {
    bool equals = Unicode::StrCaseCmp(text, text2, StringOptions::FOLD_CASE_DEFAULT, true);
  }
  // muntrace();

  std::chrono::steady_clock::time_point stop_time = std::chrono::steady_clock::now();
  int micros =
      std::chrono::duration_cast<std::chrono::microseconds>(stop_time - clock)
      .count();
  label = "e2e EqualsNoCase + norm";
  std::ostringstream ss;
  ss << "" << iterations << " " << language << " EndToEnd EqualsNoCase + Normalize " << " length: " << title << text.length()
                  << appendStats(label, language,  text,  micros, iterations);
  return ss.str();
}

std::string TestEndToEndEqualsNoCaseUnicodeString (std::string_view title, std::string_view label, std::string_view language, std::string_view text, int iterations,
    std::chrono::steady_clock::time_point clock)
{
  //Convert input to wstr
  clock = std::chrono::steady_clock::now();


  for (int i = 0; i <iterations; i++)
  {
    icu::UnicodeString unicodeStr = icu::UnicodeString::fromUTF8(text);
    icu::UnicodeString unicodeStr2 = icu::UnicodeString::fromUTF8(text);

    bool equals = unicodeStr.compare(unicodeStr2) == 0;
  }
  // muntrace();

  std::chrono::steady_clock::time_point stop_time = std::chrono::steady_clock::now();
  int micros =
      std::chrono::duration_cast<std::chrono::microseconds>(stop_time - clock)
      .count();
  label = "e2e eEqualsNoCase Ustring";
  std::ostringstream ss;
  ss << "" << iterations << " " << language << " EndToEnd EqualsNoCase UnicodeString" << " length: " << title << text.length()
                  << appendStats(label, language,  text,  micros, iterations);
  return ss.str();
}

// TODO: NOT READY
std::string TestEndToEndEqualsNoCaseNormalizeUnicodeString (std::string_view title, std::string_view label, std::string_view language, std::string_view text, int iterations,
    std::chrono::steady_clock::time_point clock)
{
  //Convert input to wstr
  clock = std::chrono::steady_clock::now();
  icu::UnicodeString unicodeStr2 = icu::UnicodeString::fromUTF8(text);


  for (int i = 0; i <iterations; i++)
  {
    icu::UnicodeString unicodeStr = icu::UnicodeString::fromUTF8(text);

    bool equals = unicodeStr.compare(unicodeStr2) == 0;
  }
  // muntrace();

  std::chrono::steady_clock::time_point stop_time = std::chrono::steady_clock::now();
  int micros =
      std::chrono::duration_cast<std::chrono::microseconds>(stop_time - clock)
      .count();
  label = "e2e EqualsNoCase+norm Ustring";
  std::ostringstream ss;
  ss << "" << iterations << " " << language << " EndToEnd EqualsNoCase + Normalize UnicodeString " << " length: " << title << text.length()
                  << appendStats(label, language,  text,  micros, iterations);
  return ss.str();
}


std::string TestEndToEndCollateWString (std::string_view title, std::string_view label, std::string_view language, std::string_view text, int iterations,
    std::chrono::steady_clock::time_point clock)
{
  //Convert input to wstr
  clock = std::chrono::steady_clock::now();

  std::string filename = "/tmp/german.titles";
  std::vector<std::wstring> lines = std::vector<std::wstring> ();

  std::ifstream input = std::ifstream(filename, std::ios_base::in);
  std::string line;

  while( std::getline( input, line ) ) {
    std::wstring wstring = Unicode::UTF8ToWString(line);
    lines.push_back(wstring);
  }
  icu::Locale icuLocale = icu::Locale::getGerman();
  for (int i = 0; i < 100; i++); // To make 100, 000 sorts
  {
    if (UnicodeUtils::InitializeCollator(icuLocale, false)) // Used by ICU Collator
    {
      // Do the sorting
      // int32_t Unicode::Collate(std::u16string_view left, std::u16string_view right);

      std::stable_sort(lines.begin(), lines.end(), CollateWstring);
      Unicode::SortCompleted(lines.size());
    }
  }
  std::chrono::steady_clock::time_point stop_time = std::chrono::steady_clock::now();
  int micros =
      std::chrono::duration_cast<std::chrono::microseconds>(stop_time - clock)
      .count();
  label = "e2e Collate wString";
  std::ostringstream ss;
  ss << "" << iterations << " " << language << " EndToEnd Collate wString" << " length: " << title << text.length()
                  << appendStats(label, language,  text,  micros, iterations);
  return ss.str();

}

std::string TestEndToEndCollateUTF16 (std::string_view title, std::string_view label, std::string_view language, std::string_view text, int iterations,
    std::chrono::steady_clock::time_point clock)
{

  std::string filename = "/tmp/german.titles";
  std::vector<std::u16string> lines = std::vector<std::u16string> ();

  std::ifstream input = std::ifstream(filename, std::ios_base::in);

  std::string line;

  while( std::getline( input, line ) ) {
    std::u16string u16String = Unicode::UTF8ToUTF16(line);
    lines.push_back(u16String);
  }
  clock = std::chrono::steady_clock::now();

  icu::Locale icuLocale = icu::Locale::getGerman();
  for (int i = 0; i < 100; i++); // To make 100, 000 sorts
  {
    if (UnicodeUtils::InitializeCollator(icuLocale, false)) // Used by ICU Collator
    {
      // Do the sorting
      // int32_t Unicode::Collate(std::u16string_view left, std::u16string_view right);

      std::stable_sort(lines.begin(), lines.end(), CollateU16);
      Unicode::SortCompleted(lines.size());
    }
  }
  std::chrono::steady_clock::time_point stop_time = std::chrono::steady_clock::now();
  int micros =
      std::chrono::duration_cast<std::chrono::microseconds>(stop_time - clock)
      .count();
  label = "e2e Collate u16";
  std::ostringstream ss;
  ss << "" << iterations << " " << language << " End-to-end Collation UTF16" << " length: " << title << text.length()
                  << appendStats(label, language,  text,  micros, iterations);
  return ss.str();
}

std::string appendStats(std::string_view label, std::string_view language, std::string_view text, int micros, int iterations)
{
  float microsecondsPerIteration = float(micros) / float(iterations);
  // Bytes measured from UTF8 version
  float microsecondsPerByte = microsecondsPerIteration / float(text.length());
  std::ostringstream ss;
  ss << " took: " << micros << " micros/iteration: " << microsecondsPerIteration
      << " micros/byte-iteration: " << microsecondsPerByte << " µs\n"
      << "csv: " << label << ", " << language << ", " << text.length() << ", " << micros << ", " << iterations << "\n";

  return ss.str();
}
