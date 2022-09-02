#pragma once

//#include <locale>

#include <climits>
#include <iterator>
#include <stddef.h>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <vector>

#include <bits/stdint-intn.h>
#include <bits/stdint-uintn.h>
#include <unicode/brkiter.h>
#include <unicode/locid.h>
#include <unicode/platform.h>
#include <unicode/stringpiece.h>
#include <unicode/umachine.h>
#include <unicode/unistr.h>
#include <unicode/utypes.h>
#include <unicode/uversion.h>

template<typename E>
constexpr auto to_underlying(E e) noexcept
{
  return static_cast<std::underlying_type_t<E>>(e);
}

// #define PRIVATE private:

// Disable private for testing

#define PRIVATE

enum class StringOptions : uint32_t
{

  /**
   * Normalization see: https://unicode.org/reports/tr15/#Norm_Forms
   *
   * Fold character case according to ICU rules:
   * Strip insignificant accents, translate chars to more simple equivalents (German sharp-s),
   * etc.
   *
   * FOLD_CASE_DEFAULT causes FoldCase to behave similar to ToLower for the "en" locale
   * FOLD_CASE_SPECIAL_I causes FoldCase to behave similar to ToLower for the "tr_TR" locale.
   *
   * Case folding also ignores insignificant differences in strings (some accent marks,
   * etc.).
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
   */
  FOLD_CASE_DEFAULT = 0, // U_FOLD_CASE_DEFAULT,
  FOLD_CASE_EXCLUDE_SPECIAL_I = 1, // U_FOLD_CASE_EXCLUDE_SPECIAL_I

  /**
       * Titlecase the string as a whole rather than each word.
       * (Titlecase only the character at index 0, possibly adjusted.)
       * Option bits value for titlecasing APIs that take an options bit set.
       *
       * It is an error to specify multiple titlecasing iterator options together,
       * including both an options bit and an explicit BreakIterator.
       *
       * @see U_TITLECASE_ADJUST_TO_CASED
       * @stable ICU 60
       */
  TITLECASE_WHOLE_STRING = 0x20,
  /**
       * Titlecase sentences rather than words.
       * (Titlecase only the first character of each sentence, possibly adjusted.)
       * Option bits value for titlecasing APIs that take an options bit set.
       *
       * It is an error to specify multiple titlecasing iterator options together,
       * including both an options bit and an explicit BreakIterator.
       *
       * @see U_TITLECASE_ADJUST_TO_CASED
       * @stable ICU 60
       */

  TITLE_CASE_SENTENCES = 0X40, // U_TITLECASE_SENTENCES

  /**
       * Do not lowercase non-initial parts of words when titlecasing.
       * Option bit for titlecasing APIs that take an options bit set.
       *
       * By default, titlecasing will titlecase the character at each
       * (possibly adjusted) BreakIterator index and
       * lowercase all other characters up to the next iterator index.
       * With this option, the other characters will not be modified.
       *
       * @see U_TITLECASE_ADJUST_TO_CASED
       * @see UnicodeString::TitleCase
       * @see CaseMap::TitleCase
       * @see ucasemap_setOptions
       * @see ucasemap_TitleCase
       * @see ucasemap_utf8TitleCase
       * @stable ICU 3.8
       */

  TITLE_CASE_NO_LOWERCASE = 0x100, // U_TITLECASE_NO_LOWERCASE

  /**
       * Do not adjust the titlecasing BreakIterator indexes;
       * titlecase exactly the characters at breaks from the iterator.
       * Option bit for titlecasing APIs that take an options bit set.
       *
       * By default, titlecasing will take each break iterator index,
       * adjust it to the next relevant character (see U_TITLECASE_ADJUST_TO_CASED),
       * and titlecase that one.
       *
       * Other characters are lowercased.
       *
       * It is an error to specify multiple titlecasing adjustment options together.
       *
       * @see U_TITLECASE_ADJUST_TO_CASED
       * @see U_TITLECASE_NO_LOWERCASE
       * @see UnicodeString::TitleCase
       * @see CaseMap::TitleCase
       * @see ucasemap_setOptions
       * @see ucasemap_TitleCase
       * @see ucasemap_utf8TitleCase
       * @stable ICU 3.8
       */

  TITLE_CASE_NO_BREAK_ADJUSTMENT = 0x200, // U_TITLECASE_NO_BREAK_ADJUSTMENT

  /**
       * Adjust each titlecasing BreakIterator index to the next cased character.
       * (See the Unicode Standard, chapter 3, Default Case Conversion, R3 TitleCasecase(X).)
       * Option bit for titlecasing APIs that take an options bit set.
       *
       * This used to be the default index adjustment in ICU.
       * Since ICU 60, the default index adjustment is to the next character that is
       * a letter, number, symbol, or private use code point.
       * (Uncased modifier letters are skipped.)
       * The difference in behavior is small for word titlecasing,
       * but the new adjustment is much better for whole-string and sentence titlecasing:
       * It yields "49ers" and "«丰(abc)»" instead of "49Ers" and "«丰(Abc)»".
       *
       * It is an error to specify multiple titlecasing adjustment options together.
       *
       * @see U_TITLECASE_NO_BREAK_ADJUSTMENT
       * @stable ICU 60
       */

  TITLE_CASE_ADJUST_TO_CASED = 0X400, // U_TITLECASE_ADJUST_TO_CASED

  /**
       * Option for string transformation functions to not first reset the Edits object.
       * Used for example in some case-mapping and normalization functions.
       *
       * @see CaseMap
       * @see Edits
       * @see Normalizer2
       * @stable ICU 60
       */

  EDITS_NO_RESET = 0X2000, // U_EDITS_NO_RESET

  /**
       * Omit unchanged text when recording how source substrings
       * relate to changed and unchanged result substrings.
       * Used for example in some case-mapping and normalization functions.
       *
       * @see CaseMap
       * @see Edits
       * @see Normalizer2
       * @stable ICU 60
       */
  OMIT_UNCHANGED_TEXT = 0x4000, // U_OMIT_UNCHANGED_TEXT

  /**
       * Option bit for u_strCaseCompare, u_strcasecmp, unorm_compare, etc:
       * Compare strings in code point order instead of code unit order.
       * @stable ICU 2.2
       */
  COMPARE_CODE_POINT_ORDER = 0x8000, // U_COMPARE_CODE_POINT_ORDER
  /**
       * Option bit for unorm_compare:
       * Perform case-insensitive comparison.
       * @stable ICU 2.2
       */
  COMPARE_IGNORE_CASE = 0x10000, // U_COMPARE_IGNORE_CASE

  /**
       * Option bit for unorm_compare:
       * Both input strings are assumed to fulfill FCD conditions.
       * @stable ICU 2.2
       */
  NORM_INPUT_IS_FCD = 0X20000 // UNORM_INPUT_IS_FCD

  // Related definitions elsewhere.
  // Options that are not meaningful in the same functions
  // can share the same bits.
  //
  // Public:
  // unicode/unorm.h #define UNORM_COMPARE_NORM_OPTIONS_SHIFT 20
  //
  // Internal: (may change or be removed)
  // ucase.h #define _STRCASECMP_OPTIONS_MASK 0xffff
  // ucase.h #define _FOLD_CASE_OPTIONS_MASK 7
  // ucasemap_imp.h #define U_TITLECASE_ITERATOR_MASK 0xe0
  // ucasemap_imp.h #define U_TITLECASE_ADJUSTMENT_MASK 0x600
  // ustr_imp.h #define _STRNCMP_STYLE 0x1000
  // unormcmp.cpp #define _COMPARE_EQUIV 0x80000

};

inline StringOptions operator|(StringOptions a, StringOptions b)
{
  return static_cast<StringOptions>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}

/*
 inline StringOptions& operator |=(StringOptions& a, StringOptions& b)
 {
 return static_cast<StringOptions>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
 } */
/**
 * From uregex.h RegexpFlag. It uses older non-scoped enum.
 */
enum class RegexpFlag : uint32_t
{
  /**  Enable case insensitive matching.  @stable ICU 2.4 */
  UREGEX_CASE_INSENSITIVE = 2,

  /**  Allow white space and comments within patterns  @stable ICU 2.4 */
  UREGEX_COMMENTS = 4,

  /**  If set, '.' matches line terminators,  otherwise '.' matching stops at line end.
       *  @stable ICU 2.4 */
  UREGEX_DOTALL = 32,

  /**  If set, treat the entire pattern as a literal string.
       *  Metacharacters or escape sequences in the input sequence will be given
       *  no special meaning.
       *
       *  The flag UREGEX_CASE_INSENSITIVE retains its impact
       *  on matching when used in conjunction with this flag.
       *  The other flags become superfluous.
       *
       * @stable ICU 4.0
       */
  UREGEX_LITERAL = 16,

  /**   Control behavior of "$" and "^"
       *    If set, recognize line terminators within string,
       *    otherwise, match only at start and end of input string.
       *   @stable ICU 2.4 */
  UREGEX_MULTILINE = 8,

  /**   Unix-only line endings.
       *   When this mode is enabled, only \\u000a is recognized as a line ending
       *    in the behavior of ., ^, and $.
       *   @stable ICU 4.0
       */
  UREGEX_UNIX_LINES = 1,

  /**  Unicode word boundaries.
       *     If set, \b uses the Unicode TR 29 definition of word boundaries.
       *     Warning: Unicode word boundaries are quite different from
       *     traditional regular expression word boundaries.  See
       *     http://unicode.org/reports/tr29/#Word_Boundaries
       *     @stable ICU 2.8
       */
  UREGEX_UWORD = 256,

  /**  Error on Unrecognized backslash escapes.
       *     If set, fail with an error on patterns that contain
       *     backslash-escaped ASCII letters without a known special
       *     meaning.  If this flag is not set, these
       *     escaped letters represent themselves.
       *     @stable ICU 4.0
       */
  UREGEX_ERROR_ON_UNKNOWN_ESCAPES = 512

};
inline RegexpFlag operator|(RegexpFlag a, RegexpFlag b)
{
  return static_cast<RegexpFlag>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}

enum class NormalizerType : int
{
  NFC = 0,
  NFD = 1,
  NFKC = 2,
  NFKD = 3,
  NFCKCASEFOLD = 4
};

class Unicode
{
public:
  /*
   * Constants, based on std::string::npos which allow the ability to
   * distinguish between several states. Currently only in use by GetCharPosition.
   *
   * BEFORE_START indicates that the method was called with an argument
   * which exceeds the length of the string:
   *
   * BEFORE_START: the character iterator runs off the left end of the string
   * AFTER_END: the character iterator runs off the right end of the string
   * ERROR: (same as string::npos) indicates some error occurred, such as
   * malformed utf-8.
   */
  static constexpr size_t ERROR = std::string::npos;
  static constexpr size_t BEFORE_START = std::string::npos - 1;
  static constexpr size_t AFTER_END = std::string::npos - 2;

  /*!
   * \brief Convert utf-8 string_view to utf-16 (native ICU format)
   *   Does NOT handle malformed Unicode
   *
   *   \param str UTF8 string_view to convert
   *   \return converted utf16 string
   */
  static std::u16string UTF8ToUTF16(std::string_view str);

  /*!
   * \brief Convert utf-8 string_view to utf-32 (native Unicode codepoint format)
   *   Does NOT handle malformed Unicode
   *
   *   \param str UTF8 string_view to convert
   *   \return converted utf32 string
   */
  static std::u32string UTF8ToUTF32(const std::string_view &s);

  /*!
   *  \brief Convert a UTF8 string_view to a wString
   *   Does NOT handle malformed Unicode
   *
   *  \param str UTF8 string_view to convert
   *  \return converted wstring
   */
  static std::wstring UTF8ToWString(std::string_view str);

  /*!
   *  \brief Convert a UTF16 u16string_view to utf8
   *   Does NOT handle malformed Unicode
   *
   *  \param str u16string_view to convert
   *  \return converted utf8
   */
  static std::string UTF16ToUTF8(const std::u16string_view& str);

  /*!
   *  \brief Convert a u16string_view to utf32
   *   Does NOT handle malformed Unicode
   *
   *  \param str u16string_view to convert
   *  \return converted utf32
   */
  static std::u32string UTF16ToUTF32(const std::u16string_view& str);

  /*!
   *  \brief Convert a u16string_view to wstring
   *   Does NOT handle malformed Unicode
   *
   *  \param str u16string_view to convert
   *  \return converted wstring
   */
  static std::wstring UTF16ToWString(const std::u16string_view& str);

  /*!
   *  \brief Convert a u32string_view to utf8
   *   Does NOT handle malformed Unicode
   *
   *  \param str u32string_view to convert
   *  \return converted utf8
   */
  static std::string UTF32ToUTF8(const std::u32string_view &s);

  /*!
   *  \brief Convert a u32string_view to u16string
   *   Does NOT handle malformed Unicode
   *   Does NOT add null terminator to un-terminated string
   *
   *  \param str u32string_view to convert
   *  \return converted u16string
   */
  static std::u16string UTF32ToUTF16(const std::u32string_view& str);


  /*!
   *  \brief Convert a wstring_view to u16string
   *   Does NOT handle malformed Unicode
   *   Does NOT add null terminator to un-terminated string
   *
   *  \param str u32string_view to convert
   *  \return converted u16string
   */
  static std::u16string WStringToUTF16(std::wstring_view sv1);

  /*!
   * \brief Convert a wstring_view to UTF8
   *   Does NOT handle malformed Unicode
   *
   * \param str wstring_view to convert
   * \return string converted to UTF8
   */
  static std::string WStringToUTF8(std::wstring_view str);

  /*!
   * \brief Convert a wstring_view to UnicodeString
   *   Does handle malformed Unicode
   *
   * \param str wstring_view to convert
   * \return string converted to UnicodeString
   */
  static icu::UnicodeString WStringToUnicodeString(std::wstring_view sv1);

  PRIVATE
  /*!
   * \brief Convert a string_view into a u16string. Malformed codepoints are
   * replaced by 0xFFFD which displays as "�"
   *
   * NOTE: returned u16string_view is only valid as long as the buffer exists
   *
   * \param src UTF8 string to convert to UChars and write to a UTF16 buffer
   * \param buffer to write to
   * \param bufferSize maximum number of UChars that can be written to
   *         the buffer.
   * \return a reference to buffer
   *
   * Avoids allocating temp space from the heap by having buffer be a local
   * (stack-based) array.
   *
   * Substituting malformed codepoints to 0xFFFD does not significantly impact
   * performance.
   */
  static std::u16string_view _UTF8ToUTF16WithSub(std::string_view src,
                              UChar* buffer,
                              size_t bufferSize);

  /*!
   * \brief Convert a wchar_t array into a u16string_view.  Malformed codepoints are
   * replaced by 0xFFFD which displays as "�"
   *
   * NOTE: returned string_vew is only valid as long as the buffer exists
   *
   * \param src wstring_view to convert to UChars and write to a UChar buffer
   * \param buffer backs the returned u16string_view

   * \param length maximum number of Unicode codepoints (wchar_t) to convert to UChars
   * \return u16string_view backed by buffer
   *
   * Avoids allocating temp space from the heap by having buffer be a local
   * (stack-based) array.
   *
   * Substituting malformed codepoints to 0xFFFD does not significantly impact
   * performance.
   */
  static std::u16string_view _WStringToUTF16WithSub(std::wstring_view src,
                               char16_t * buffer,
                               const size_t bufferSize);

  /*!
   * \brief Convert a UChar (UTF-16) array into a UTF-8 std::string_view  Malformed codepoints are
   * replaced by 0xFFFD which displays as "�"
   *
   * NOTE: returned string_vew is only valid as long as the buffer exists
   *
   * Note: Should probably use UTF16ToUTF8 instead. This function substitutes malformed
   *       unicode into substitute unicode. Given that this is conversion FROM Unicode,
   *       it is most likely that any malformed unicode has been detected and
   *       replaced with substitute unicode, leaving little value here.
   *
   * \param src u16string_view to convert to UTF-8 and write to a std::string
   * \param buffer scratch char* to write UTF-8 characters to
   * \param bufferSize maximum number of bytes that can be written to
   *         the buffer.
   * \return std::string_view backed by buffer
   *
   * Avoids allocating temp space from the heap by having buffer be a local
   * (stack-based) array.
   *
   * Substituting malformed codepoints to 0xFFFD does not significantly impact
   * performance.
   */
  static std::string_view _UTF16ToUTF8WithSub(const std::u16string_view src,
                                   char* buffer,
                                   size_t bufferSize);

  /*!
   * \brief Convert a u16string_view into a wstring
   *
   * NOTE: returned wstring_view is only valid as long as the buffer exists
   *
   * \param str u16string_view to convert to wchar_t
   * \param buffer scratch wchar_t* to write Unicode characters to
   * \param bufferSize maximum number of codepoints (32-bit) that can be written to
   *         the buffer.
   * \return a reference to buffer
   *
   * Avoids allocating temp space from the heap by having buffer be a local
   * (stack-based) array.
   */
  static std::wstring_view _UTF16ToWString(const std::u16string_view str,
                               wchar_t* buffer,
                               size_t bufferSize);

  /*!
   * \brief convert a wstring_view to a UnicodeString
   *         Malformed codepoints are replaced by 0xFFFD which displays as "�"
   *
   * \param wStr wstring to convert
   * \return UnicodeString
   */
  static icu::UnicodeString ToUnicodeString(std::wstring_view wStr,
      char16_t* buffer, size_t bufferLength, size_t bufferCapacity)
  {
    icu::UnicodeString us = icu::UnicodeString(buffer, bufferLength, bufferCapacity);

#if U_SIZEOF_WCHAR_T == 2
    return icu::UnicodeString(wStr.data(), wStr.length());
#else
    return icu::UnicodeString::fromUTF32((int32_t*)wStr.data(), wStr.length());
#endif
  }

  /*!
   * \brief Convert a std::string_view to a UnicodeString
   *         Malformed codepoints are replaced by 0xFFFD which displays as "�"
   *
   * \param src UTF-8 string to convert
   * \return UnicodeString
   */

  static icu::UnicodeString ToUnicodeString(std::string_view src)
  {
    return icu::UnicodeString::fromUTF8(src);
  }

  /*!
   * Calculates a 'reasonable' buffer size to give some room for growth in a utf-8
   * string to accommodate some basic transformations (folding, normalization, etc.).
   * Not guaranteed to be sufficient for all needs.
   *
   * param utf8_length byte-length of UTF-8 string to be converted
   * param scale multiplier to apply to get larger buffer
   *
   * Note that the returned size has a pad of 200 bytes added to leave room
   * for growth.
   *
   */
  static size_t GetUTF8WorkingSize(const size_t utf8_length, float scale = 1.5);

  /*!
   * \brief Calculates the maximum number of UChars (UTF-16) required by a UTF-8
   * string.
   *
   * \param utf8_length byte-length of UTF-8 string to be converted
   * \param scale multiplier to apply to get larger buffer
   * \return a fairly generous estimate of the number of UChars that will
   *         be required for a string of the given length with
   *         room for growth
   *
   * Note that the returned size has a pad of 200 UChars added to leave room
   * for growth.
   *
   * Note that a UTF-16 string will be at most as long as the UTF-8 string.
   */
  static size_t GetUTF8ToUTF16BufferSize(const size_t utf8_length, const float scale = 1.0);

  /*!
   * \brief Calculates the maximum number of UChars (UTF-16) required by a wchar_t
   * string.
   *
   * \param wchar_length Char32 codepoints to be converted to UTF-16.
   * \param scale multiplier to apply to get larger buffer
   * \return A size a bit larger than wchar_length, plus 200.
   */
  static size_t GetWcharToUTF16BufferSize(const size_t wchar_length, const float scale = 1.0);

  /*!
   * \brief Calculates the maximum number of UTF32 codeunits required by a UTF16
   * string.
   *
   * \param wchar_length number of UTF16 code units to be converted to UTF32.
   * \param scale multiplier to apply to get larger buffer
   * \return A size a bit larger than wchar_length, plus 200.
   */
  static size_t GetUTF16ToUTF32BufferSize(const size_t uchar_length, const float scale = 1.0);

  /*!
   * \brief Calculates the maximum number of wchar_t code-units (UTF32/UTF16)
   * required by a UTF-16 string.
   *
   * \param wchar_length Uchar-16 code-units to be converted to wchar_t buffer.
   * \param scale multiplier to apply to get larger buffer
   * \return A size a bit larger than wchar_length, plus 200.
   */
  static size_t GetUTF16ToWcharBufferSize(const size_t uchar_length, const float scale = 1.0);

  /*!
   * \brief Up-sizes the length of UTF16 string to accommodate the size
   *       increase of by basic string operations
   *
   * \param utf16length Number of UTF-16 code units now in string
   * \param scale multiplier to apply to get larger buffer
   * \return a fairly generous estimate of the nmberu of UChars that will
   *         be required to contained a string of the given length with
   *         room for growth
   *
   * Note that the returned size has a pad of 200 UChars added to leave room
   * for growth.
   *
   * Note that a UTF-16 string will be at most as long as the UTF-8 string.
   */
  static size_t GetUTF16WorkingSize(const size_t utf16length, const float scale = 2.0);

  /*!
   * \brief Calculates the maximum number of UTF-8 bytes required by a UTF-16 string.
   *
   * \param utf16length Number of UTF-16 code units to convert
   * \param scale multiplier to apply to get a larger buffer
   * \return a fairly generous estimate of the number of bytes that will
   *         be required to contained a string of the given length with
   *         room for growth
   *
   * Note that 200 bytes is added to the result to give some room for growth.
   *
   * Note that in addition to any scale factor, the number of UTF-8 bytes
   * returned is 3 times the number of UTF-16 code unites. This leaves enough
   * room for the 'worst case' UTF-8  expansion required for Indic, Thai and CJK.
   */
  static size_t GetUTF16ToUTF8BufferSize(const size_t utf16Length, const float scale = 2.0);

  /*!
   * \brief Calculates the maximum number of wchar_t code units required to
   * represent UTF-16 code units.
   *
   * On most systems a wchar_t represents a 32-bit Unicode codepoint. For such machines
   * a wchar_t string will require at most the same number of 32-bit codepoints as the
   * 16-bit has code-units (i.e. twice as many bytes).
   *
   * On the systems with a wchar_t representing a 16-bit UTF-16 code unit, then the
   * number of code units (and bytes) required to represent a UTF-16 UChar in wchar_t
   * will bhe the same as the original string.
   *
   * \param utf16length number of UTF-16 code units in original string
   * \param scale scale factor to multiply uchar_length by to allow for growth
   * \returns Number of wchar_t characters to allocate for the desired buffer
   *          with a fairly generous amount of room for growth
   *
   * Note that an additional 200 code units is added to the result to allow for growth
   *
   */
  static size_t GetUTF16ToWCharBufferSize(const size_t utf16Length, const float scale = 1.0);

public:
  /*!
   * \brief Get the icu::Locale for g_langInfo.GetLocale()
   *
   * \return icu::Locale defined by the language and country code from
   *         g_langInfo.GetLocale()
   */
  static icu::Locale GetDefaultICULocale();

  /*!
   * \brief Get the icu::Locale for the given std::locale
   *
   * \param locale C++ locale to use to get the icu::Locale
   * \return the icu::Locale defined by the language and country code
   *         specified by locale
   */
  static icu::Locale GetICULocale(const std::locale& locale);

  /*!
   * \brief A simple wrapper around icu::Locale
   *
   * Construct a locale from language, country, variant.
   * If an error occurs, then the constructed object will be "bogus"
   * (isBogus() will return true).
   *
   * \param language Lowercase two-letter or three-letter ISO-639 code.
   *  This parameter can instead be an ICU style C locale (e.g. "en_US"),
   *  but the other parameters must not be used.
   *  This parameter can be NULL; if so,
   *  the locale is initialized to match the current default locale.
   *  (This is the same as using the default constructor.)
   *
   * \param country  Uppercase two-letter ISO-3166 code. (optional)
   * \param variant  Uppercase vendor and browser specific code. See class
   *                 description. (optional)
   * \param keywordsAndValues A string consisting of keyword/values pairs, such as
   *                 "collation=phonebook;currency=euro
   * \return specified icu::Locale
   *
   * Note: More information can be found in unicode/locid.h (part of ICU library).
   */

  static icu::Locale GetICULocale(std::string_view language,
                                  std::string_view country = std::string_view(),
                                  std::string_view variant = std::string_view(),
                                  std::string_view keywordsAndValues = std::string_view());
  /*!
   * \brief Construct a simple locale id based upon an icu::Locale
   *
   * \param locale to get an id for
   * \return a string <ll>_<cc> where ll is language and cc is country.
   *         The default language is "en". If <cc> is not present then only <ll>
   *         is returned
   */
  static std::string GetICULocaleId(const icu::Locale locale);

  /*!
   *  \brief Folds the case of a string, independent of Locale.
   *
   * Similar to ToLower except in addition, insignificant accents are stripped
   * and other transformations are made (such as German sharp-S is converted to ss).
   * The transformation is independent of locale.
   *
   * \param str string to fold
   * \param opt StringOptions to fine-tune behavior. For most purposes, leave at
   *            default value, 0 (same as FOLD_CASE_DEFAULT)
   * \return UTF-8 same as src.
   *
   * Notes: length and number of bytes in string may change during folding/normalization
   *
   * When FOLD_CASE_DEFAULT is used, the Turkic Dotted I and Dotless
   * i follow the "en" locale rules for ToLower.
   *
   * While the case folding works very well, it is a good idea to test when
   * using non-ASCII characters.
   *
   * See StringOptions for the more details.
   */
  static std::wstring FoldCase(std::wstring_view src,
      const StringOptions options = StringOptions::FOLD_CASE_DEFAULT);

  /*!
   *  \brief Folds the case of a wstring, independent of Locale.
   *
   * Similar to ToLower except in addition, insignificant accents are stripped
   * and other transformations are made (such as German sharp-S is converted to ss).
   * The transformation is independent of locale.
   *
   * \param str string to fold.
   * \param opt StringOptions to fine-tune behavior. For most purposes, leave at
   *            default value, 0 (same as FOLD_CASE_DEFAULT)
   * \return UTF-8 folded wstring.
   *
   * Notes: length and number of bytes in string may change during folding/normalization
   *
   * When FOLD_CASE_DEFAULT is used, the Turkic Dotted I and Dotless
   * i follow the "en" locale rules for ToLower.
   *
   * While the case folding works very well, it is a good idea to test when
   * using non-ASCII characters.
   *
   * See StringOptions for the more details.
   */
  static std::string FoldCase(std::string_view src, const StringOptions options = StringOptions::FOLD_CASE_DEFAULT);

  /*!
   *  \brief Folds the case of a StringPiece (UTF-8), independent of Locale.
   *
   * Similar to ToLower except in addition, insignificant accents are stripped
   * and other transformations are made (such as German sharp-S is converted to ss).
   * The transformation is independent of locale.
   *
   * \param strPiece StringPiece to fold.
   * \param sink a ByteSink to accept the folded string
   * \param status UErrorCode indicating the status of the operation. Make sure to
   *        set to U_ZERO_ERROR before calling this function
   * \param options StringOptions to fine-tune behavior. For most purposes, leave at
   *            default value, 0 (same as FOLD_CASE_DEFAULT)
   *
   * Notes: length and number of bytes in string may change during folding/normalization
   *
   * A ByteSink wraps a buffer to give added flexibility on storage management, etc.
   *
   * When FOLD_CASE_DEFAULT is used, the Turkic Dotted I and Dotless
   * i follow the "en" locale rules for ToLower.
   *
   * While the case folding works very well, it is a good idea to test when
   * using non-ASCII characters.
   *
   * See StringOptions for the more details.
   * /
  static void FoldCase(const icu::StringPiece strPiece,
                       icu::CheckedArrayByteSink& sink,
                       UErrorCode& status,
                       const int32_t options);
*/
public:
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
   *  \param normalizerType select the appropriate Normalizer for the job
   *  \return Normalized string
   */
  static const std::wstring Normalize(std::wstring_view src,
                                      const NormalizerType normalizerType = NormalizerType::NFKC);

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
   * \param normalizerType select the appropriate Normalizer for the job
   * \return Normalized string
   */
  static const std::string Normalize(std::string_view src,
                                     const NormalizerType normalizerType = NormalizerType::NFKC);

  PRIVATE

  /*!
     *  \brief Normalizes a UnicodeString. Not expected to be used outside of
     *   Unicode.
     *
     * There are multiple Normalizations that can be performed on Unicode. Fortunately
     * normalization is not needed in many situations. An introduction can be found
     * at: https://unicode-org.github.io/icu/userguide/transforms/normalization/
     *
     * \param strPiece UTF-8 string to Normalize.
     * \param sink a CheckedArrayByteSink that wraps the output UTF-8 buffer
     * \param normalizerType select the appropriate Normalizer for the job\
     * \return UnicodeString containing the normalization
     */
  static const icu::UnicodeString Normalize(const icu::UnicodeString& us1,
      const NormalizerType NormalizerType);

  /*!
   *  \brief Normalizes a StringPiece (utf-8). Not expected to be used outside of
   *   Unicode.
   *
   * There are multiple Normalizations that can be performed on Unicode. Fortunately
   * normalization is not needed in many situations. An introduction can be found
   * at: https://unicode-org.github.io/icu/userguide/transforms/normalization/
   *
   * \param strPiece UTF-8 string to Normalize.
   * \param sink a CheckedArrayByteSink that wraps the output UTF-8 buffer
   * \param status UErrorCode indicating the status of the operation. Make sure to
   *        set to U_ZERO_ERROR before calling this function
   * \param options fine tunes behavior. See StringOptions. Frequently can leave
   *        at default value.
   * \param normalizerType select the appropriate Normalizer for the job
   */
  static void Normalize(const std::string_view strPiece,
                        icu::CheckedArrayByteSink& sink,
                        UErrorCode& status,
                        const int32_t options,
                        const NormalizerType normalizerType);

public:
  /*!
   * \brief Converts a string to Upper case according to locale.
   *
   * Note: The length of the string can change, depending upon the underlying
   * icu::locale.
   *
   * \param str string to change case on
   * \param locale the underlying icu::Locale is created using the language,
   *        country, etc. from this locale
   * \return str with every character changed to upper case
   */
  static std::string ToUpper(std::string_view src, const icu::Locale& locale);

PRIVATE
  /*!
   * \brief Converts a UChar buffer to Upper case according to locale.
   *
   * Note: The length of the string can change, depending upon the underlying
   * icu::locale.
   *
   * \param src u16strig_view buffer to change case on
   * \param icu::Locale governs the ToUpper behavior
   * \param p_u_toupper_buffer buffer to write the results of ToUpper on p_u_src_buffer
   * \param u_toupper_buffer_size size in UChars of p_u_toupper_buffer
   * \param to_upper_length the actual number of UChars written to p_u_toupper_buffer
   * \param status UErrorCode indicating the status of the operation. Make sure to
   *        set to U_ZERO_ERROR before calling this function
   */
  /*
  static void ToUpper(std::u16string_view src,
                      const icu::Locale locale,
                      UChar* p_u_toupper_buffer,
                      const int32_t u_toupper_buffer_size,
                      int32_t& to_upper_length,
                      UErrorCode& status);
*/
public:
  /*!
   * \brief Converts a string to Lower case according to icu:Locale.
   *
   * Note: The length of the string can change, depending upon the underlying
   * icu::locale.
   *
   * \param src string to change case on
   * \param icu::Locale provides the rules about changing case
   * \return the lower case version of src
   */
  static const std::string ToLower(std::string_view src, const icu::Locale& locale);

  /*!
   *  \brief Capitalizes a string using Legacy Kodi rules.
   *
   * Uses a simplistic approach familiar to English speakers.
   * See TitleCase for a more locale aware solution.
   *
   * \param src string to capitalize
   * \return src capitalized
   */
  static const std::string ToCapitalize(std::string_view src);
  /*!
   *  \brief Capitalizes a wstring using Legacy Kodi rules.
   *
   * Uses a simplistic approach familiar to English speakers.
   * See TitleCase for a more locale aware solution.
   *
   * \param src string to capitalize
   * \return src capitalized
   */
  static const std::wstring ToCapitalize(std::wstring_view src);


PRIVATE
  static const icu::UnicodeString ToCapitalize(icu::UnicodeString us);

public:

  /*!
   *  \brief TitleCase a wstring using icu::Locale.
   *
   *  Similar too, but more language friendly version of ToCapitalize. Uses ICU library.
   *
   *  Best results are when a complete sentence/paragraph is TitleCased rather than
   *  individual words.
   *
   *  \param src string to TitleCase
   *  \param locale
   *  \return src in TitleCase
   */
  static const std::wstring TitleCase(std::wstring_view src, const icu::Locale& locale);

  /*!
   *  \brief TitleCase a string using icu::Locale.
   *
   *  Similar too, but more language friendly version of ToCapitalize. Uses ICU library.
   *
   *  Best results are when a complete sentence/paragraph is TitleCased rather than
   *  individual words.
   *
   *  \param src string to TitleCase
   *  \param locale
   *  \return src in TitleCase
   */
  static const std::string TitleCase(std::string_view src, const icu::Locale& locale);

  /*!
   * \brief Compares two wstrings using codepoint order. Locale does not matter.
   *
   * \param s1 one of the strings to compare
   * \param s2 one of the strings to compare
   * \param normalize sometimes strings require normalization to compare properly.
   *        Not normally required.
   * \return <0 or 0 or >0 as usual for string comparisons
   *
   * "codepoint" order is one of the several types of comparisons that can be made.
   * Normally it is not important which you use as long as you are consistent.
   *
   * At this time there are few guidelines on the use of normalization. It is mostly
   * needed when other string manipulations which alter the order of bytes within
   * a string. More study is needed.
   */
  static int8_t StrCmp(std::wstring_view s1,
                       std::wstring_view s2,
                       const bool normalize = false);

  /*!
   * \brief Compares two strings using codepoint order.
   *
   * \param s1 one of the strings to compare
   * \param s2 one of the strings to compare
   * \param normalize sometimes strings require normalization to compare properly.
   *        Not normally required.
   * \return <0 or 0 or >0 as usual for string comparisons
   *
   * "codepoint" order is one of the several types of comparisons that can be made.
   * Normally it is not important which you use as long as you are consistent.
   *
   * At this time there are guidelines on the use of normalization. It is mostly
   * needed when other string manipulations which alter the order of bytes within
   * a string. More study is needed.
   */
  static int8_t StrCmp(std::string_view s1,
                       std::string_view s2,
                       const bool normalize = false);

  /*!
   * \brief Performs a bit-wise comparison of two wstrings, after case folding each.
   *
   * Logically equivalent to StrCmp(FoldCase(str1, opt)), FoldCase(str2, opt))
   * or, if normalize == true: Strcmp(NFD(FoldCase(NFD(str1))), NFD(FoldCase(NFD(str2))))
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
   * \param normalize Controls whether normalization is performed before and after
   *        case folding
   * \return The result of bitwise character comparison:
   * < 0 if the characters s1 are bitwise less than the characters in s2,
   * = 0 if str1 contains the same characters as s2,
   * > 0 if the characters in s1 are bitwise greater than the characters in s2.
   */
  static int StrCaseCmp(std::wstring_view s1,
                        std::wstring_view s2,
                        const StringOptions options,
                        const bool normalize = false);

  /*!
   * \brief Performs a bit-wise comparison of two strings, after case folding each.
   *
   * Logically equivalent to StrCmp(FoldCase(str1, opt)), FoldCase(str2, opt))
   * or, if normalize == true: Strcmp(NFD(FoldCase(NFD(str1))), NFD(FoldCase(NFD(str2))))
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
   * \param normalize Controls whether normalization is performed before and after
   *        case folding
   * \return The result of bitwise character comparison:
   * < 0 if the characters s1 are bitwise less than the characters in s2,
   * = 0 if str1 contains the same characters as s2,
   * > 0 if the characters in s1 are bitwise greater than the characters in s2.
   */
  static int StrCaseCmp(std::string_view s1,
                        std::string_view s2,
                        const StringOptions options,
                        const bool normalize = false);

  /*!
   * \brief Initializes the icu Collator for this thread, such as before sorting a
   * table.
   *
   * Assumes that all collation will occur in this thread.
   *
   * Note: Only has an impact if icu collation is configured instead of legacy
   *       AlphaNumericCompare.
   *
   *       Also starts the elapsed-time timer for the sort. See SortCompleted.
   *
   * \param locale Collation order will be based on the icu::Locale derived from this locale.
   * \param normalize Controls whether normalization is performed prior to collation.
   *                  Frequently not required. Some free normalization always occurs.
   * \return true if initialization was successful, otherwise false.
   *
   * Normally leave normalize off. Some free normalization is still performed.
   * To improve performance, normalize every record prior to sort instead of enabling
   * it here since normalization is performed on each comparison.
   *
   * See the documentation for UCOL_NORMALIZATION_MODE for more info.
   */
  static bool InitializeCollator(const std::locale locale, bool normalize = false);

  /*!
   * \brief Initializes the icu Collator for this thread, such as before sorting a
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
   * \param normalize Controls whether normalization is performed prior to collation.
   *                  Frequently not required. Some free normalization always occurs.
   * \return true if initialization was successful, otherwise false.
   *
   * Normally leave normalize off. Some free normalization is still performed.
   * To improve performance, normalize every record prior to sort instead of enabling
   * it here since normalization is performed on each comparison.
   *
   * See the documentation for UCOL_NORMALIZATION_MODE for more info.
   */
  static bool InitializeCollator(const icu::Locale icuLocale, bool normalize = false);

  /*!
   * \brief Allows some performance statistics to be generated based upon the previous sort
   *
   * Data collection may be controlled by a setting or #define
   */
  static void SortCompleted(const int sortItems);

  /*!
   * \brief Collates two wstrings using the most recent collator setup in the same
   * thread by InitializeCollator.
   *
   * \param left one of the strings to collate
   * \param right the other string to collate
   * \return A value < 0, 0 or > 0 depending upon whether left collates before,
   *         equal to or after right
   */
  static int32_t Collate(std::wstring_view left, std::wstring_view right);

  /*!
   * \brief Collates two u16string_vies using the most recent collator setup in the same
   * thread by InitializeCollator.
   *
   * \param left one of the strings to collate
   * \param right the other string to collate
   * \return A value < 0, 0 or > 0 depending upon whether left collates before,
   *         equal to or after right
   */
  static int32_t Collate(std::u16string_view left, std::u16string_view right);

  /*!
   * \brief Determines if a string begins with another string
   *
   * \param s1 string to be searched
   * \param s2 string to find at beginning of s1
   * \return true if s1 starts with s2, otherwise false
   */
  static bool StartsWith(std::string_view s1, std::string_view s2);
  /*
   * \brief Determines if a string begins with another string, ignoring case
   *
   * \param s1 string to be searched
   * \param s2 string to find at beginning of s1
   * \param options controls behavior of case folding, normally leave at default
   * \return true if s1 starts with s2, otherwise false
   */
  static bool StartsWithNoCase(std::string_view s1,
                               std::string_view s2,
                               const StringOptions options);

  /*!
   * \brief Determines if a string ends with another string
   *
   * \param s1 string to be searched
   * \param s2 string to find at end of s1
   * \return true if s1 ends with s2, otherwise false
   */
  static bool EndsWith(std::string_view s1, std::string_view s2);

  /*!
   * \brief Determines if a string ends with another string, ignoring case
   *
   * \param s1 string to be searched
   * \param s2 string to find at end of s1
   * \param options controls behavior of case folding, normally leave at default
   * \return true if s1 ends with s2, otherwise false
   */
  static bool EndsWithNoCase(std::string_view s1,
                             std::string_view s2,
                             const StringOptions options);

  /*!
   * \brief Get the leftmost side of a UTF-8 string, limited by character count
   *
   * Unicode characters are of variable byte length. This function's
   * parameters are based on characters and NOT bytes.
   *
   * \param str to get a substring of
   * \param charCount if keepLeft: charCount is number of characters to
   *                  copy from left end (limited by str length)
   *                  if ! keepLeft: number of characters to omit from right end
   * \param keepLeft controls how charCount is interpreted
   * \return leftmost characters of string, length determined by charCount
   */
  static std::string Left(std::string_view str,
                          size_t charCount,
                          const icu::Locale icuLocale,
                          const bool keepLeft = true);

  /*!
   *  \brief Get a substring of a UTF-8 string
   *
   * Unicode characters are of variable byte length. This function's
   * parameters are based on characters and NOT bytes.
   *
   * \param str string to extract substring from
   * \param startCharIndex leftmost character of substring [0 based]
   * \param charCount maximum number of characters to include in substring
   * \return substring of str, beginning with character 'firstCharIndex',
   *         length determined by charCount
   */
  static std::string Mid(std::string_view str,
                         size_t startCharIndex,
                         size_t charCount = std::string::npos);

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
   * \param charCount if keepRight: charCount is number of characters to
   *                  copy from right end (limited by str length)
   *                  if ! keepRight: number of characters to omit from left end
   * \param keepRight controls how charCount is interpreted
   * \param icuLocale determines how character breaks are made
   * \return rightmost characters of string, length determined by charCount
   *
   * Ex: Copy all but the leftmost two characters from str:
   *
   * std::string x = Right(str, 2, false, Unicode::GetDefaultICULocale());
   */
  static std::string Right(std::string_view str,
                           size_t charCount,
                           const icu::Locale& icuLocale,
                           bool keepRight = true);

  PRIVATE
  /*!
   * \brief configures UText for a UTF-8 string
   *
   * UText provides a means to let some ICU APIs work with UTF8 without conversion to
   * UTF-16 or a UnicodeString first. It is fairly new to the api and its use is fairly
   * limited at this time. Currently, Kodi uses it for BreakIteration (
   * Character/Word BreakIterator). If the string is changed, the UText must be
   * reconfigured.
   *
   * \param str UTF-8 string to create the UText for
   * \param ut utext object to reuse. If nullptr, then create new utext object
   * \return a UText object that contains the passed UTF8 string for the iterator
   *           On error, nullptr is returned
   *
   * *** Note: utext_close(<return value>) MUST be called when finished using the Utext
   * or there will be a memory leak.
   */
  static UText* ConfigUText(std::string_view str, UText* ut = nullptr);

  /*!
     * \brief configures UText for a u16string_view
     *
     * UText provides a means to use the BreakIterator (
     * Character/Word BreakIterator). If the string is changed, the UText must be
     * reconfigured.
     *
     * \param str u16string_view to create the UText for
     * \param ut utext object to reuse. If nullptr, then create new utext object
     * \return a UText object that contains the passed UTF8 string for the iterator
     *           On error, nullptr is returned
     *
     * *** Note: utext_close(<return value>) MUST be called when finished using the Utext
     * or there will be a memory leak.
     */
  static UText* ConfigUText(const std::u16string_view& str, UText* ut = nullptr);

  /*!
   * \brief Configures a Character BreakIterator for use
   *
   *    A "character/grapheme" can be composed of one or more 32-bit Unicode codepoints.
   *    Rules for character breaks is locale dependent. See ICU's documentation
   *    on CharacterBreakIterator, or inspect the code here for more information.
   *
   * \param ut Utext to create the Character BreakIterator for using the current string
   *           that the UText has been configured for
   * \param icuLocale locale to configure the iterator with
   * \return a Character BreakIterator object that can iterate a over the UText's string
   *           On error, nullptr is returned.
   *
   * *** Note: utext_close(<return value>) MUST be called when finished using the iterator, including
   * when this method returns nullptr, or there will be a memory leak.
   */
  static icu::BreakIterator* ConfigCharBreakIter(UText* ut, const icu::Locale& icuLocale);

  /*!
   * \brief Modifies the configuration of a Character BreakIterator for use. Typically used
   *        after the UText's string has been changed
   *
   * \param ut UText configured by ConfigUText
   * \icuLocale locale to configure the iterator with
   * \return a Character BreakIterator object that uses the UText's string for iteration
   *           On error, nullptr is returned.
   *
   * *** Note: utext_close(<return value>) MUST be called when finished using the iterator, including
   * when this method returns nullptr, or there will be a memory leak.
   */
  static icu::BreakIterator* ReConfigCharBreakIter(UText* ut, icu::BreakIterator* cbi);

  /*!
   * \brief Configures a new Character BreakIterator for use
   *
   * \param str String to create the Character BreakIterator for
   * \icuLocale locale to configure the iterator for
   * \return true is returned on success, otherwise false.
   */
  static icu::BreakIterator* ConfigCharBreakIter(const icu::UnicodeString& str,
                                                 const icu::Locale& icuLocale);
  /*!
   * \brief Configures a Character BreakIterator for use
   *
   * \param str String to create the Character BreakIterator for
   * \icuLocale locale to configure the iterator for
   * \return true is returned on success, otherwise false.
   */
  static size_t GetCharPosition(icu::BreakIterator* cbi,
                                size_t stringLength,
                                size_t charCount,
                                const bool left,
                                const bool keepLeft);

public:
  static size_t GetCharPosition(icu::BreakIterator* cbi,
                                std::string_view str,
                                const size_t charCount,
                                const bool left,
                                const bool keepLeft,
                                const icu::Locale& icuLocale);

  /*!
   * \brief Gets the byte-offset of a Unicode character relative to a reference
   *
   * This function is primarily used by Left, Right and Mid. See comment at end for details on use.
   * The locale is used by underlying icuc4 library to tweak character boundaries.
   *
   * Unicode characters may consist of multiple codepoints. This function's parameters
   * are based on characters NOT bytes.
   *
   * \param str UTF-8 string to get index from
   * \param charCountArg number of characters from reference point to get byte index for
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
  static size_t GetCharPosition(std::string_view str,
                                const size_t charCountArg,
                                const bool left,
                                const bool keepLeft,
                                const icu::Locale& icuLocale);

  /*!
   * \brief Gets the byte-offset of a Unicode character relative to a reference
   *
   * This function is primarily used by Left, Right and Mid. See comment at end for details on use.
   * The locale is used by underlying icuc4 library to tweak character boundaries.
   *
   * Unicode characters may consist of multiple codepoints. This function's parameters
   * are based on characters NOT bytes.
   *
   * \param uStr UnicodeString to create get index from
   * \param charCountArg number of characters from reference point to get byte index for
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
  size_t GetCharPosition(const icu::UnicodeString& uStr,
                         const size_t charCountArg,
                         const bool left,
                         const bool keepLeft,
                         const icu::Locale& icuLocale);
public:
  /*!
   * \brief Removes leading and trailing whitespace from the string
   *
   * \param str string to trim
   * \return trimmed string
   *
   * Removes leading and trailing whitespace: [\t\n\f\r\p{Z}] where \p{Z} means characters with the
   * property Z. Full list is:
   * 0009..000D    ; White_Space # Cc   [5] <control-0009>..<control-000D>
   * 0020          ; White_Space # Zs       SPACE
   * 0085          ; White_Space # Cc       <control-0085>
   * 00A0          ; White_Space # Zs       NO-BREAK SPACE
   * 1680          ; White_Space # Zs       OGHAM SPACE MARK
   * 2000..200A    ; White_Space # Zs  [11] EN QUAD..HAIR SPACE
   * 2028          ; White_Space # Zl       LINE SEPARATOR
   * 2029          ; White_Space # Zp       PARAGRAPH SEPARATOR
   * 202F          ; White_Space # Zs       NARROW NO-BREAK SPACE
   * 205F          ; White_Space # Zs       MEDIUM MATHEMATICAL SPACE
   * 3000          ; White_Space # Zs       IDEOGRAPHIC SPACE
   */
  static std::string Trim(std::string_view str);

  /*!
   * \brief Removes leading whitespace from a string
   *
   * \param str string to trim
   * \return trimmed string
   *
   * See Trim(str) for more details about what counts as whitespace
   */
  static std::string TrimLeft(std::string_view str);

  /*!
   * \brief Remove a set of characters from beginning of str
   *
   *  Remove any leading characters from the set chars from str.
   *
   *  Ex: TrimLeft("abc1234bxa", "acb") ==> "1234bxa"
   *
   * \param str to trim
   * \param trimChars (characters) to remove from str
   * \return trimmed string
   */
  static std::string TrimLeft(std::string_view str, const std::string_view trimChars);

  /*!
   * \brief Removes trailing whitespace from a string
   *
   * \param str string to trim
   * \return trimmed string
   *
   * See Trim(str) for more details about what counts as whitespace
   */
  static std::string TrimRight(std::string_view str);

  /*!
   * \brief Remove a set of characters from end of str
   *
   *  Remove any trailing characters from the set chars from str.
   *
   *  Ex: TrimRight("abc1234bxa", "acb") ==> "abc1234bx"
   *
   * \param str to trim
   * \param trimChars (characters) to remove from str
   * \return trimmed string
   */
  static std::string TrimRight(std::string_view str, const std::string_view trimChars);

  /*!
   * \brief Remove a set of characters from ends of str
   *
   *  Ex: Trim("abc1234bxa", "acb", true, true) ==> "1234bxa"
   *
   * \param str to trim
   * \param trimStrings (characters) to remove from str
   * \param trimLeft if true, then trim from left end of string
   * \param trimRight if true, then trim from right end of string
   * \return trimmed string
   */
  static std::string Trim(std::string_view str,
                          std::string_view trimStrings,
                          const bool trimLeft,
                          const bool trimRight);

  /*!
   * \brief Remove a set of strings from ends of str
   *
   *  Ex: Trim("abc1234bxa", {"acb", "12"}, true, true) ==> "abc34bxa"
   *
   * \param str to trim
   * \param trimStrings strings to remove from str
   * \param trimLeft if true, then trim from left end of string
   * \param trimRight if true, then trim from right end of string
   * \return trimmed string
   */
  static std::string Trim(std::string_view str,
                          const std::vector<std::string_view>& trimStrings,
                          const bool trimLeft,
                          const bool trimRight);

  PRIVATE
  /*!
   * \brief Remove a set of characters from ends of a UnicodeString
   *
   * \param str UnicodeString to trim
   * \param trimChars set of characters to remove from str
   * \param trimLeft if true, then trim from left end of string
   * \param trimRight if true, then trim from right end of string
   * \return trimmed UnicodeString
   */
  static icu::UnicodeString Trim(const icu::UnicodeString& str,
                                 const icu::UnicodeString& trimChars,
                                 const bool trimLeft,
                                 const bool trimRight);

  /*!
   * \brief Remove a set of UnicodeStrings from ends of a UnicodeString
   *
   * \param str UnicodeString to trim
   * \param trimStrings set of strings to remove from str
   * \param trimLeft if true, then trim from left end of string
   * \param trimRight if true, then trim from right end of string
   * \return trimmed UnicodeString
   */
  static icu::UnicodeString Trim(const icu::UnicodeString& uStr,
                                 const std::vector<icu::UnicodeString>& trimStrings,
                                 const bool trimLeft,
                                 const bool trimRight);

  /*!
   * \brief Remove whitespace from a UnicodeString
   *
   * \param str to trim
   * \param trimLeft if true, then trim from left end of string
   * \param trimRight if true, then trim from right end of string
   * \return trimmed string
   */
  static icu::UnicodeString Trim(const icu::UnicodeString& str,
                                 const bool trimLeft,
                                 const bool trimRight);

public:
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
  static std::vector<std::string> SplitMulti(const std::vector<std::string_view>& input,
                                             const std::vector<std::string_view>& delimiters,
                                             size_t iMaxStrings = 0);

  PRIVATE
  /*!
   * \brief Splits each input UnicodeString with each delimiter UnicodeString
   * producing a vector of split UnicodeStrings  omitting any null strings.
   *
   * \param input vector of UnicodeStrings to be split
   * \param delimiters when found in a UnicodeString, a delimiter splits a string into two strings
   * \param iMaxStrings (optional) Maximum number of resulting split strings
   * \result the accumulation of all split strings (and any unfinished splits, due to iMaxStrings
   *          being exceeded) omitting null strings
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
  static std::vector<icu::UnicodeString> SplitMulti(
      const std::vector<icu::UnicodeString>& input,
      const std::vector<icu::UnicodeString>& delimiters,
      size_t iMaxStrings /* = 0 */);

public:
  /*!
   * \brief Replaces every occurrence of a substring in string and returns the count of replacements
   *
   * Somewhat less efficient than FindAndReplace because this one returns a count
   * of the number of changes.
   *
   * \param str String to make changes to in-place
   * \param oldText string to be replaced
   * \param newText string to replace with
   * \return Count of the number of changes
   */
  /*
  [[deprecated(
      "FindAndReplace is faster, returned count not used.")]] static std::tuple<std::string, int>
  FindCountAndReplace(std::string_view src,
                      std::string_view oldText,
                      std::string_view newText);
*/
  PRIVATE
  /*!
   * \brief Replace all occurrences of characters in oldText with the characters
   *        in newText
   * \param src string to replace text in
   * \param oldText the text containing the search text
   * \param newText the text containing the replacement text
   * \return a Tuple with a reference to the modified src and an int containing
   *         the number of times the oldText was replaced.
   */
  /*
  static std::tuple<icu::UnicodeString, int> FindCountAndReplace(
      const icu::UnicodeString& src,
      const icu::UnicodeString& oldText,
      const icu::UnicodeString& newText);
*/
  /*!
   * \brief Replace all occurrences of characters in oldText with the characters
   *        in newText
   * \param srcText the text containing the search text
   * \param start offset in UChar units (16-bit) in srcText to begin searching for
   *        substrings to replace
   * \param length number of Uchar codeunits (16-bit) in srcText to allow to be
   *        replaced (provided the substring of this part of srcText matches the
   *        oldText
   * \param oldText contains the search string to look for in srcText substing
   *        specified by start and length
   * \param oldStart offset in Uchar units (16-bit) in oldText which will form
   *        the start of the substring of oldText to be used for the search
   * \param oldLength number of UChar units (16-bit) that will be in the
   *         substring of oldText for the search
   * \param newText contains the replacement text for each occurrence of the
   *        oldText substring found in the srcText substring
   * \param newStart the offset in UChar units (16-bit) in newText which will
   *        form the start of the substring of newText that will serve as
   *        the replacement text in the srcText substring
   * \param newLength the number of UChar units (16-bit) in newText which will
   *        be in the substring of newText that will replace any occurrences
   *        of the oldText substring in the srcText substring.
   * \return a Tuple with a reference to the modified src and an int containing
   *         the number of times the oldText was replaced.
   */
  /*
  static std::tuple<icu::UnicodeString, int> FindCountAndReplace(const icu::UnicodeString& srcText,
                                                                 const int32_t start,
                                                                 const int32_t length,
                                                                 const icu::UnicodeString& oldText,
                                                                 const int32_t oldStart,
                                                                 const int32_t oldLength,
                                                                 const icu::UnicodeString& newText,
                                                                 const int32_t newStart,
                                                                 const int32_t newLength);
*/
public:
  /*!
   * \brief Finds where two strings differ
   *
   * \param s1 string to search from
   * \param s2 string to search for in s1
   * \param caseless if true, then perform a caseless match
   * \return index of first non-natching character/grapheme
   */
  // static size_t FindDifference(const std::string_view s1, const std::string_view s2,
  //     bool caseless = false);

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
  static bool FindWord(std::string_view str, std::string_view word);

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
  static std::string FindAndReplace(std::string_view str,
                                    const std::string_view oldText,
                                    const std::string_view newText);

  /*!
   * \brief Replace a matching pattern in a string with another value
   *
   *  Regular expression patterns for this lib can be found at:
   *  https://unicode-org.github.io/icu/userguide/strings/regexp.html
   *
   * \param str string to copy and apply changes to (original not changed)
   * \param icu::Regex pattern to use to identify what substring to change (see uregex.h)
   * \param replace New string value to use in place of the matching pattern
   * \param flags:  See enum RegexpFlag in uregex.h
   *
   */
  static std::string RegexReplaceAll(std::string_view str,
                                     const std::string_view pattern,
                                     const std::string_view replace,
                                     const int flags);

  PRIVATE
  /*!
   * \brief Replace all occurrences of a regular expression in a string
   *
   * Regular expression patterns for this lib can be found at:
   * https://unicode-org.github.io/icu/userguide/strings/regexp.html
   *
   * \param uString string which will be copied and then modified before being returned
   * \param uPattern regular expression to search for in the copy of uString
   * \param uReplace each occurrence of uPattern in the copy of uString will be replaced
   *        by this string
   * \param flags see enum Unicode::RegexpFlag
   * \return copy of UString modified as necessary according to the parameters
   */
  icu::UnicodeString RegexReplaceAll(const icu::UnicodeString& uString,
                                     const icu::UnicodeString uPattern,
                                     const icu::UnicodeString uReplace,
                                     const int flags);

public:
  /*!
   * \brief Count the number of occurances of strFind within strInput
   *
   * Used by UnicodeUtils::FindNumber
   *
   * \param strInput input string to search
   * \param strFind substring to search for in strInput
   * \param flags which influence the call to RegexFind. The default is UREGEX_LITERAL,
   *        which prevents interpreting strFind as a regular expression.
   * \return a count of the number of occurrences found.
   */
  static int32_t countOccurances(std::string_view strInput,
                                 std::string_view strFind,
                                 const int flags);

  /*! \brief Splits the given input string using the given delimiter into separate strings.

   If the given input string is empty nothing will be put into the target iterator.

   \param d_first the beginning of the destination range
   \param input Input string to be split
   \param delimiter Delimiter to be used to split the input string
   \param iMaxStrings (optional) Maximum number of split strings. 0 means infinite
   \return output iterator to the element in the destination range, one past the last element
   *       that was put there
   */
  /*
  template<typename OutputIt>
  static OutputIt SplitTo(OutputIt d_first,
                          std::string_view input,
                          const char delimiter,
                          size_t iMaxStrings / *= 0* /)
  {
    return SplitTo(d_first, input, std::string(1, delimiter), iMaxStrings);
  } */

  /*!
   * \brief Splits the given input string at each delimiter in the set delimiters
   *
   * An algorithm that can be used to explain the behavior:
   *   For the second .. n delimiters, replace every occurrence of that delimiter
   *   in uInput with the first delimiter
   *   Finally, split uInput using the first delimiter
   *
   * \param d_first output iterator to send split strings as they are created
   * \param uInput is the string to be searched to make split strings
   * \param uDelimiters contains a set of strings which cause uInput to split.
   *        First, follow the algorithm outlined above. then split the remaining
   *        uInput using the first delimiter. If found in uInput will cause any text
   *        preceding it to be sent as a string to d_first. If no text proceeds
   *        d_first, then an empty string will be sent, unless omitEmptyStrings
   *        is true. In addition, if no characters follow the last uDelimiter, an
   *        empty string will be sent to d_first
   * \param iMaxStrings, if non-zero, limits the number of splits that will be
   *        created. Once reached, the remaining un-split text in uInput will be copied
   *        as a string to d_first
   * \param omitEmptyStrings controls whether empty strings are sent to d_first or not
   * \return output iterator to the element in the destination range, one past the last element
   *       that was put there
   *
   *  A Few examples can be found in UnicodeUtils as well as TestUnicodeUtils
   */
  template<typename OutputIt>
  static OutputIt SplitTo(OutputIt d_first,
                          const std::string_view input,
                          const std::vector<std::string_view>& delimiters,
                          size_t iMaxStrings /* = 0 */)
  {
    // TODO: Verify why this can not be done with plain string.

    OutputIt dest = d_first;
    if (input.empty()) // Return nothing, not even empty string.
      return dest;

    if (delimiters.empty())
    {
      *dest++ = {input.data(), input.length()};
      return dest;
    }
    icu::UnicodeString uInput = ToUnicodeString(input);
    icu::UnicodeString uDelimiter = ToUnicodeString(delimiters[0]);

    // First, transform every occurrence of delimiters in the string with the first delimiter
    /// Then, split using the first delimiter.

    for (size_t di = 1; di < delimiters.size(); di++)
    {
      icu::UnicodeString uNextDelimiter = ToUnicodeString(delimiters[di]);
      uInput.findAndReplace(uNextDelimiter, uDelimiter);
    }

    std::vector<icu::UnicodeString> uDest = std::vector<icu::UnicodeString>();
    constexpr bool omitEmptyStrings = false;
    Unicode::SplitTo(std::back_inserter(uDest), uInput, uDelimiter, iMaxStrings, omitEmptyStrings);

    std::string tempStr = std::string();
    for (size_t i = 0; i < uDest.size(); i++)
    {
      tempStr.clear();
      tempStr = uDest[i].toUTF8String(tempStr);
      *d_first++ = tempStr;
    }
    return dest;
  }

  /*!
   * \brief Splits the given input string using the given delimiter into separate strings.
   *
   * If the given input string is empty nothing will be put into the target iterator.
   *
   * \param d_first the beginning of the destination range
   * \param input string to be split
   * \param delimiter Delimiter to be used to split the input string
   * \param iMaxStrings (optional) Maximum number of split strings. 0 means infinite
   * \return output iterator to the element in the destination range, one past the last element
   *       that was put there
   *
   *  A Few examples can be found in UnicodeUtils as well as TestUnicodeUtils
   */
  template<typename OutputIt>
  static OutputIt SplitTo(OutputIt d_first,
                          const std::string_view input,
                          const std::string_view delimiter,
                          size_t iMaxStrings = 0)
  {
    OutputIt dest = d_first;

    if (input.empty())
      return dest;
    if (delimiter.empty())
    {
      *d_first++ = {input.data(), input.length()};
      return dest;
    }

    icu::UnicodeString uInput = ToUnicodeString(input);
    icu::UnicodeString uDelimiter = ToUnicodeString(delimiter);
    std::vector<icu::UnicodeString> uResult = std::vector<icu::UnicodeString>();

    bool omitEmptyStrings = false;
    Unicode::SplitTo(std::back_inserter(uResult), uInput, uDelimiter, iMaxStrings,
                     omitEmptyStrings);

    // Convert back to utf-8
    // std::cout << "Unicode.SplitTo input: " << input << " delim: " << delimiter
    //				<< " Max split: " << iMaxStrings << std::endl;
    for (size_t i = 0; i < uResult.size(); i++)
    {
      std::string tempStr = std::string();
      tempStr = uResult[i].toUTF8String(tempStr);
      // std::cout << "splitTo idx: " << i << " value: " << tempStr << std::endl;
      *dest++ = tempStr;
    }
    return dest;
  }

  PRIVATE
  /*!
   * \brief Splits the given input string at each delimiter
   *
   * \param d_first output iterator to send split strings as they are created
   * \param uInput is the string to be searched to make split strings
   * \param uDelimiter contains a string which if found in uInput will cause any text
   *        preceding it to be sent as a string to d_first. If no text proceeds
   *        d_first, then an empty string will be sent, unless omitEmptyStrings
   *        is true. In addition, if no characters follow the last uDelimiter, an
   *        empty string will be sent to d_first
   * \param iMaxStrings, if non-zero, limits the number of splits that will be
   *        created. Once reached, the remaining un-split text in uInput will be copied
   *        as a string to d_first
   * \param omitEmptyStrings controls whether empty strings are sent to d_first or not
   */
  template<typename OutputIt>
  static OutputIt SplitTo(OutputIt d_first,
                          const icu::UnicodeString& uInput,
                          const icu::UnicodeString& uDelimiter,
                          size_t iMaxStrings = 0,
                          const bool omitEmptyStrings = false);

  /*!
   * \brief Splits the given input string at each of the delimiters
   *
   * \param d_first output iterator to send split strings as they are created
   * \param uInput is the string to be searched to make split strings
   * \param uDelimiters contains a set of strings which if any are found in
   *        uInput will cause any text
   *        preceding it to be sent as a string to d_first. If no text proceeds
   *        d_first, then an empty string will be sent, unless omitEmptyStrings
   *        is true. In addition, if no characters follow the last uDelimiter, an
   *        empty string will be sent to d_first
   * \param iMaxStrings, if non-zero, limits the number of splits that will be
   *        created. Once reached, the remaining un-split text in uInput will be copied
   *        as a string to d_first
   * \param omitEmptyStrings controls whether empty strings are sent to d_first or not
   */
  template<typename OutputIt>
  static OutputIt SplitTo(OutputIt d_first,
                          icu::UnicodeString uInput,
                          const std::vector<icu::UnicodeString>& uDelimiters,
                          size_t iMaxStrings = 0,
                          const bool omitEmptyStrings = false);

  /*!
   * \brief indicates whether a string contains any of a vector of keywords
   *
   * \param str string to check
   * \param keywords if any of these keywords are contained in str, then true is
   * returned
   * \return true of str contains any of the strings in keywords
   */
  static bool Contains(std::string_view str, const std::vector<std::string_view>& keywords);

public:
  static const std::u16string TestHeapLessUTF8ToUTF16(std::string_view s1);

  static const std::string TestHeapLessUTF16ToUTF8(std::u16string_view s1);

  PRIVATE
  /*!
   * \brief detects whether a codepoint is a latin character or not
   *
   * Normally a character can fit in a single codepoint, but even if not,
   * the first codepoint is always the most important one,
   * it specifies the base character, any other codepoints add information.
   * Therefore, with one codepoint you can determine if it is a Latin
   * character or not
   *
   * \param codepoint to examine
   * \return true if the codepoint is proof that this is a Latin character,
   *         otherwise false
   */
  static bool IsLatinChar(const UChar32 codepoint);
};
