/*
 * TestUnicodeUtils_Performance.cpp
 *
 *  Created on: Aug 4, 2022
 *      Author: fbacher
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


//      u"óóßChloë" // German "Sharp-S" ß is (mostly) equivalent to ss (lower case).
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

// static std::string TestFoldUTF16(std::string_view title, std::string_view label, std::string_view language, std::string_view text, int iterations,
//    std::chrono::steady_clock::time_point clock);

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

TEST(TestUnicodeUtils_Performance, Performance_All)
{
  const std::string testStr1 = "This is a short string";
  const std::string testStr2 = "This is longer 123456789012345678901234567890123456789012345678901234567890";
  const std::string testStr3 = "Note: This function serves a similar purpose that ToLower/ToUpper is \
 *       frequently used for ASCII maps indexed by keyword. ToLower/ToUpper does \
 *       NOT work to 'Normalize' Unicode to a consistent value regardless of\
 *       the case variations of the input string. A particular problem is the behavior\
*       of \"The Turkic I\". FOLD_CASE_DEFAULT is effective at \
 *       eliminating this problem. Below are the four \"I\" characters in\
 *       Turkic and the result of FoldCase for each";
  std::string longRussian = {UTF8_LONG_RUSSIAN};
  std::string longChinese = {UTF8_LONG_CHINESE};

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
    SUCCEED() << msg;

    msg = RunToUnicodeStringTest(title, label, language, text, iterations, clock);
    SUCCEED() << msg;

    msg = RunHeaplessUTF8ToUTF16Test(title, label, language, text, iterations, clock);
    SUCCEED() << msg;

    msg = RunHeapLessUTF16ToUTF8(title, label, language, text, iterations, clock);
    SUCCEED() << msg;

    msg = RunUnicodeStringToUTF8(title, label, language, text, iterations, clock);
    SUCCEED() << msg;

    // msg = TestFoldUTF16(title, label, language, text, iterations, clock);
    // SUCCEED() << msg;

    msg = TestFoldUnicodeString(title, label, language, text, iterations, clock);
    SUCCEED() << msg;

    msg = TestEndToEndFold(title, label, language, text, iterations, clock);
    SUCCEED() << msg;

    msg = TestEndToEndFold_w(title, label, language, text, iterations, clock);
    SUCCEED() << msg;

    msg = TestEndToEndFoldUnicodeString(title, label, language, text, iterations, clock);
    SUCCEED() << msg;

    /*
    msg = TestEndToEndLeft(title, label, language, text, iterations, clock);
    SUCCEED() << msg;

    msg = TestEndToEndLeftUnicodeString (title, label, language, text, iterations, clock);
    SUCCEED() << msg;
     */

    msg = TestEndToEndEqualsNoCase (title, label, language, text, iterations, clock);
    SUCCEED() << msg;

    msg = TestEndToEndEqualsNoCaseNormalize(title, label, language, text, iterations, clock);
    SUCCEED() << msg;

    msg = TestEndToEndEqualsNoCaseUnicodeString (title, label, language, text, iterations, clock);
    SUCCEED() << msg;

    //    msg = TestEndToEndEqualsNoCaseNormalizeUnicodeString(title, label, language, text, iterations, clock);
    //    SUCCEED() << msg;

    msg = TestEndToEndCollateUTF16(title, label, language, text, iterations, clock);
    SUCCEED() << msg;

    msg = TestEndToEndCollateWString (title, label, language, text, iterations, clock);
    SUCCEED() << msg;
  }
  EXPECT_TRUE(true);

}


TEST(TestUnicodeUtils_Performance, Performance_Basic)
{
  const std::string testStr1 = "This is a short string";
  const std::string testStr2 = "This is longer 123456789012345678901234567890123456789012345678901234567890";
  const std::string testStr3 = "Note: This function serves a similar purpose that ToLower/ToUpper is \
 *       frequently used for ASCII maps indexed by keyword. ToLower/ToUpper does \
 *       NOT work to 'Normalize' Unicode to a consistent value regardless of\
 *       the case variations of the input string. A particular problem is the behavior\
*       of \"The Turkic I\". FOLD_CASE_DEFAULT is effective at \
 *       eliminating this problem. Below are the four \"I\" characters in\
 *       Turkic and the result of FoldCase for each";
  std::string longRussian = {UTF8_LONG_RUSSIAN};
  std::string longChinese = {UTF8_LONG_CHINESE};

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
    SUCCEED() << msg;

    msg = RunToUnicodeStringTest(title, label, language, text, iterations, clock);
    SUCCEED() << msg;

    msg = RunHeaplessUTF8ToUTF16Test(title, label, language, text, iterations, clock);
    SUCCEED() << msg;

    msg = RunHeapLessUTF16ToUTF8(title, label, language, text, iterations, clock);
    SUCCEED() << msg;

    msg = RunUnicodeStringToUTF8(title, label, language, text, iterations, clock);
    SUCCEED() << msg;

    // msg = TestFoldUTF16(title, label, language, text, iterations, clock);
    // SUCCEED() << msg;

    msg = TestFoldUnicodeString(title, label, language, text, iterations, clock);
    SUCCEED() << msg;
  }
  EXPECT_TRUE(true);

}

TEST(TestUnicodeUtils_Performance, TestEndToEndFold)
{
  const std::string testStr1 = "This is a short string";
  const std::string testStr2 = "This is longer 123456789012345678901234567890123456789012345678901234567890";
  const std::string testStr3 = "Note: This function serves a similar purpose that ToLower/ToUpper is \
 *       frequently used for ASCII maps indexed by keyword. ToLower/ToUpper does \
 *       NOT work to 'Normalize' Unicode to a consistent value regardless of\
 *       the case variations of the input string. A particular problem is the behavior\
*       of \"The Turkic I\". FOLD_CASE_DEFAULT is effective at \
 *       eliminating this problem. Below are the four \"I\" characters in\
 *       Turkic and the result of FoldCase for each";
  std::string longRussian = {UTF8_LONG_RUSSIAN};
  std::string longChinese = {UTF8_LONG_CHINESE};

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
    SUCCEED() << msg;
  }
  EXPECT_TRUE(true);
}


TEST(TestUnicodeUtils_Performance, TestEndToEndFold_w)
{
  const std::string testStr1 = "This is a short string";
  const std::string testStr2 = "This is longer 123456789012345678901234567890123456789012345678901234567890";
  const std::string testStr3 = "Note: This function serves a similar purpose that ToLower/ToUpper is \
 *       frequently used for ASCII maps indexed by keyword. ToLower/ToUpper does \
 *       NOT work to 'Normalize' Unicode to a consistent value regardless of\
 *       the case variations of the input string. A particular problem is the behavior\
*       of \"The Turkic I\". FOLD_CASE_DEFAULT is effective at \
 *       eliminating this problem. Below are the four \"I\" characters in\
 *       Turkic and the result of FoldCase for each";
  std::string longRussian = {UTF8_LONG_RUSSIAN};
  std::string longChinese = {UTF8_LONG_CHINESE};

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
    SUCCEED() << msg;
  }
  EXPECT_TRUE(true);
}

TEST(TestUnicodeUtils_Performance, TestEndToEndFoldUnicodeString)
{
  const std::string testStr1 = "This is a short string";
  const std::string testStr2 = "This is longer 123456789012345678901234567890123456789012345678901234567890";
  const std::string testStr3 = "Note: This function serves a similar purpose that ToLower/ToUpper is \
 *       frequently used for ASCII maps indexed by keyword. ToLower/ToUpper does \
 *       NOT work to 'Normalize' Unicode to a consistent value regardless of\
 *       the case variations of the input string. A particular problem is the behavior\
*       of \"The Turkic I\". FOLD_CASE_DEFAULT is effective at \
 *       eliminating this problem. Below are the four \"I\" characters in\
 *       Turkic and the result of FoldCase for each";
  std::string longRussian = {UTF8_LONG_RUSSIAN};
  std::string longChinese = {UTF8_LONG_CHINESE};

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
    SUCCEED() << msg;
  }
  EXPECT_TRUE(true);
}

TEST(TestUnicodeUtils_Performance, Left)
{
  const std::string testStr1 = "This is a short string";
  const std::string testStr2 = "This is longer 123456789012345678901234567890123456789012345678901234567890";
  const std::string testStr3 = "Note: This function serves a similar purpose that ToLower/ToUpper is \
 *       frequently used for ASCII maps indexed by keyword. ToLower/ToUpper does \
 *       NOT work to 'Normalize' Unicode to a consistent value regardless of\
 *       the case variations of the input string. A particular problem is the behavior\
*       of \"The Turkic I\". FOLD_CASE_DEFAULT is effective at \
 *       eliminating this problem. Below are the four \"I\" characters in\
 *       Turkic and the result of FoldCase for each";
  std::string longRussian = {UTF8_LONG_RUSSIAN};
  std::string longChinese = {UTF8_LONG_CHINESE};

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
    SUCCEED() << msg;

    msg = TestEndToEndLeftUnicodeString (title, label, language, text, iterations, clock);
    SUCCEED() << msg;
     */
  }
  EXPECT_TRUE(true);
}

TEST(TestUnicodeUtils_Performance, TestEndToEndEqualsNoCase)
{
  const std::string testStr1 = "This is a short string";
  const std::string testStr2 = "This is longer 123456789012345678901234567890123456789012345678901234567890";
  const std::string testStr3 = "Note: This function serves a similar purpose that ToLower/ToUpper is \
 *       frequently used for ASCII maps indexed by keyword. ToLower/ToUpper does \
 *       NOT work to 'Normalize' Unicode to a consistent value regardless of\
 *       the case variations of the input string. A particular problem is the behavior\
*       of \"The Turkic I\". FOLD_CASE_DEFAULT is effective at \
 *       eliminating this problem. Below are the four \"I\" characters in\
 *       Turkic and the result of FoldCase for each";
  std::string longRussian = {UTF8_LONG_RUSSIAN};
  std::string longChinese = {UTF8_LONG_CHINESE};

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
    SUCCEED() << msg;

    //    msg = TestEndToEndEqualsNoCaseNormalizeUnicodeString(title, label, language, text, iterations, clock);
    //    SUCCEED() << msg;
  }
}


TEST(TestUnicodeUtils_Performance, TestEndToEndEqualsNoCaseNormalize)
{
  const std::string testStr1 = "This is a short string";
  const std::string testStr2 = "This is longer 123456789012345678901234567890123456789012345678901234567890";
  const std::string testStr3 = "Note: This function serves a similar purpose that ToLower/ToUpper is \
 *       frequently used for ASCII maps indexed by keyword. ToLower/ToUpper does \
 *       NOT work to 'Normalize' Unicode to a consistent value regardless of\
 *       the case variations of the input string. A particular problem is the behavior\
*       of \"The Turkic I\". FOLD_CASE_DEFAULT is effective at \
 *       eliminating this problem. Below are the four \"I\" characters in\
 *       Turkic and the result of FoldCase for each";
  std::string longRussian = {UTF8_LONG_RUSSIAN};
  std::string longChinese = {UTF8_LONG_CHINESE};

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
    SUCCEED() << msg;
  }
  EXPECT_TRUE(true);
}

TEST(TestUnicodeUtils_Performance, TestEndToEndEqualsNoCaseUnicodeString )
{
  const std::string testStr1 = "This is a short string";
  const std::string testStr2 = "This is longer 123456789012345678901234567890123456789012345678901234567890";
  const std::string testStr3 = "Note: This function serves a similar purpose that ToLower/ToUpper is \
 *       frequently used for ASCII maps indexed by keyword. ToLower/ToUpper does \
 *       NOT work to 'Normalize' Unicode to a consistent value regardless of\
 *       the case variations of the input string. A particular problem is the behavior\
*       of \"The Turkic I\". FOLD_CASE_DEFAULT is effective at \
 *       eliminating this problem. Below are the four \"I\" characters in\
 *       Turkic and the result of FoldCase for each";
  std::string longRussian = {UTF8_LONG_RUSSIAN};
  std::string longChinese = {UTF8_LONG_CHINESE};

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
    SUCCEED() << msg;
  }
  EXPECT_TRUE(true);
}

TEST(TestUnicodeUtils_Performance, CollateU16)
{
  const std::string testStr1 = "This is a short string";
  const std::string testStr2 = "This is longer 123456789012345678901234567890123456789012345678901234567890";
  const std::string testStr3 = "Note: This function serves a similar purpose that ToLower/ToUpper is \
 *       frequently used for ASCII maps indexed by keyword. ToLower/ToUpper does \
 *       NOT work to 'Normalize' Unicode to a consistent value regardless of\
 *       the case variations of the input string. A particular problem is the behavior\
*       of \"The Turkic I\". FOLD_CASE_DEFAULT is effective at \
 *       eliminating this problem. Below are the four \"I\" characters in\
 *       Turkic and the result of FoldCase for each";
  std::string longRussian = {UTF8_LONG_RUSSIAN};
  std::string longChinese = {UTF8_LONG_CHINESE};

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
    SUCCEED() << msg;
  }
}
TEST(TestUnicodeUtils_Performance, CollateWstr)
{
  const std::string testStr1 = "This is a short string";
  const std::string testStr2 = "This is longer 123456789012345678901234567890123456789012345678901234567890";
  const std::string testStr3 = "Note: This function serves a similar purpose that ToLower/ToUpper is \
 *       frequently used for ASCII maps indexed by keyword. ToLower/ToUpper does \
 *       NOT work to 'Normalize' Unicode to a consistent value regardless of\
 *       the case variations of the input string. A particular problem is the behavior\
*       of \"The Turkic I\". FOLD_CASE_DEFAULT is effective at \
 *       eliminating this problem. Below are the four \"I\" characters in\
 *       Turkic and the result of FoldCase for each";
  std::string longRussian = {UTF8_LONG_RUSSIAN};
  std::string longChinese = {UTF8_LONG_CHINESE};

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
    SUCCEED() << msg;
  }
  EXPECT_TRUE(true);
}


static std::string CopyStringUtest(std::string_view title, std::string_view label, std::string_view language, std::string_view text, int iterations,
    std::chrono::steady_clock::time_point clock)
{
  clock = std::chrono::steady_clock::now();

  for (int i = 0; i <iterations; i++)
  {
    std::string tmp = std::string(text);
  }
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
/*
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
    std::string Unicode::FoldCase(std::string_view sv1, const StringOptions options)

    std::u16string_view svFolded = Unicode::_FoldCase(u16Text, sv2, StringOptions::FOLD_CASE_DEFAULT);
  }

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
*/

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

static int32_t CollateWstring(std::wstring_view left, std::wstring_view right)
{
  return UnicodeUtils::Collate(left, right);
}

static int32_t CollateU16(std::u16string_view left, std::u16string_view right)
{
  return Unicode::Collate(left, right);
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
