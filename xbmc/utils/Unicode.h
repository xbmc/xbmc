#pragma once

//#include <locale>

#include <bits/stdint-intn.h>
#include <bits/stdint-uintn.h>
#include <stddef.h>
#include <tuple>
#include <unicode/brkiter.h>
#include <unicode/locid.h>
#include <unicode/platform.h>
#include <unicode/stringpiece.h>
#include <unicode/umachine.h>
#include <unicode/unistr.h>
#include <unicode/utypes.h>
#include <unicode/uversion.h>
#include <climits>
#include <iterator>
#include <string>
#include <type_traits>
#include <vector>

template<typename E>
constexpr auto to_underlying(E e) noexcept
{
  return static_cast<std::underlying_type_t<E>>(e);
}

/**
 *
 * local version of RegexpFlag from uregex.h. Avoids import and
 */

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
       * @see UnicodeString::toTitle
       * @see CaseMap::toTitle
       * @see ucasemap_setOptions
       * @see ucasemap_toTitle
       * @see ucasemap_utf8ToTitle
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
       * @see UnicodeString::toTitle
       * @see CaseMap::toTitle
       * @see ucasemap_setOptions
       * @see ucasemap_toTitle
       * @see ucasemap_utf8ToTitle
       * @stable ICU 3.8
       */

      TITLE_CASE_NO_BREAK_ADJUSTMENT = 0x200, // U_TITLECASE_NO_BREAK_ADJUSTMENT

      /**
       * Adjust each titlecasing BreakIterator index to the next cased character.
       * (See the Unicode Standard, chapter 3, Default Case Conversion, R3 toTitlecase(X).)
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
  NFC = 0, NFD = 1, NFKC = 2, NFKD = 3, NFCKCASEFOLD = 4
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
   *  \brief Convert a UTF8 string to a wString
   *
   *  \param str UTF8 string to convert
   *  \return converted wstring
   */
  static std::wstring UTF8ToWString(const std::string &str);

  /*!
   * \brief Convert a wstring to UTF8
   *
   * \param str wstring to convert
   * \return string converted to UTF8
   */
  static std::string WStringToUTF8(const std::wstring &str);

private:
  /*!
   * \brief Convert a string into a UChar buffer
   *
   * \param src UTF-8 sting to convert to UChars and write to a UChar buffer
   * \param buffer to write to
   * \param bufferSize maximum number of UChars that can be written to
   *         the buffer.
   * \param destLength set to the actual number of UChars written when complete
   * \param src_offset offset (in bytes) to begin the conversion to UChars
   * \param  src_length maximum number of UTF-8 bytes to convert to UChars
   * \return a reference to buffer
   *
   * This is meant to be used to avoid allocating temp space from the heap by
   * having buffer be a local (stack-based) array.
   *
   * Note that malformed Unicode codepoints will be converted to place-holder
   * "substitute" Unicode codepoints.
   */
  static UChar* StringToUChar(const std::string &src, UChar *buffer, size_t bufferSize,
      int32_t &destLength, const size_t src_offset = 0,
      const size_t src_length = std::string::npos);

  /*!
   * \brief Convert a UTF-8 string into a UChar buffer
   *
   * \param src char array to convert to UChars and write to a UChar buffer
   * \param buffer to write to
   * \param bufferSize maximum number of UChars that can be written to
   *         the buffer.
   * \param destLength set to the actual number of UChars written when complete
   * \param  length maximum number of UTF-8 bytes to convert to UChars
   * \return a reference to buffer
   *
   * This is meant to be used to avoid allocating temp space from the heap by
   * having buffer be a local (stack-based) array.
   *
   * Note that malformed Unicode codepoints will be converted to place-holder
   * "substitute" Unicode codepoints.
   */
  static UChar* StringToUChar(const char *src, UChar *buffer, size_t bufferSize,
      int32_t &destLength, const size_t length = std::string::npos);

  /*!
   * \brief Convert a wchar_t array into a UChar buffer
   *
   * \param src wchar_t array to convert to UChars and write to a UChar buffer
   * \param buffer to write to
   * \param bufferSize maximum number of UChars that can be written to
   *         the buffer.
   * \param destLength set to the actual number of UChars written when complete
   * \param length maximum number of Unicode codepoints (wchar_t) to convert to UChars
   * \return a reference to buffer
   *
   * This is meant to be used to avoid allocating temp space from the heap by
   * having buffer be a local (stack-based) array.
   *
   * Note that malformed Unicode codepoints will be converted to place-holder
   * "substitute" Unicode codepoints.
   */
  static UChar* WcharToUChar(const wchar_t *src, UChar *buffer, size_t bufferSize,
      int32_t &destLength, const size_t length = std::string::npos);

  /*!
   * \brief Convert a UChar (UTF-16) array into a UTF-8 std::string
   *
   * \param u_str UChar (UTF-16) array to convert to UTF-8 and write to a std::string
   * \param buffer scratch char* to write UTF-8 characters to
   * \param bufferSize maximum number of bytes that can be written to
   *         the buffer.
   * \param destLength set to the actual number of bytes written when complete
   * \param str_length maximum number of UChars to convert to UTF-8
   * \return buffer copied to a std::string
   *
   * This is meant to be used to avoid allocating temp space from the heap by
   * having buffer be a local (stack-based) array.
   *
   * Note that malformed Unicode codepoints will be converted to place-holder
   * "substitute" Unicode codepoints.
   */
  static std::string UCharToString(const UChar *u_str, char *buffer, size_t bufferSize,
      int32_t &destLength, const size_t u_str_length);

  /*!
   * \brief Convert a UChar (UTF-16) array into a wchar_t (Unicode, UChar32) buffer
   *
   * \param u_str UChar (UTF-16) array to convert to UTF-32 and write to a UChar buffer
   * \param buffer scratch wchar_t* to write Unicode characters to
   * \param bufferSize maximum number of codepoints (32-bit) that can be written to
   *         the buffer.
   * \param destLength set to the actual number of codepoints written when complete
   * \param str_length maximum number UChars to convert to WChar (Unicode)
   * \return a reference to buffer
   *
   * This is meant to be used to avoid allocating temp space from the heap by
   * having buffer be a local (stack-based) array.
   *
   * Note that malformed Unicode codepoints will be converted to place-holder
   * "substitute" Unicode codepoints.
   */
  static wchar_t* UCharToWChar(const UChar *u_str, wchar_t *buffer, size_t bufferSize,
      int32_t &destLength, const size_t length = std::string::npos);

  /*!
   * \brief convert a wstring to a UnicodeString
   *
   * \param wStr wstring to convert
   * \return UnicodeString containing UChar* encoding of wstring (Unicode), or
   *         for the rare machine that defines wchar_t as 16 bits then it is
   *         converted to UChar* or something similar
   */
  static icu::UnicodeString ToUnicodeString(const std::wstring &wStr)
  {
#if U_SIZEOF_WCHAR_T==2
    return icu::UnicodeString(wStr.data(), wStr.length());
#else
    return icu::UnicodeString::fromUTF32((int32_t*) wStr.data(), wStr.length());
#endif
  }

  /*!
   * Convert a std::string to a UnicodeString
   *
   * \param src UTF-8 string to convert
   * \return UnicodeString
   */

  static icu::UnicodeString ToUnicodeString(const std::string &src)
  {
    return icu::UnicodeString::fromUTF8(src);
  }

  /*!
   * Convert an icu::StringPiece to a UnicodeString
   *
   * \param src a UTF-8 StringPiece to convert
   * \return UnicodeString
   *
   * A StringPiece can be a std:string or a char*
   */
  static icu::UnicodeString ToUnicodeString(const icu::StringPiece &src)
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
  static size_t GetBasicUTF8BufferSize(const size_t utf8_length, const float scale);

  /*!
   * \brief Calculates the maximum number of UChars (UTF-16) required by a wchar_t
   * string.
   *
   * \param wchar_length Char32 codepoints to be converted to UTF-16.
   * \param scale multiplier to apply to get larger buffer
   * \return A size a bit larger than wchar_length, plus 200.
   */
  static size_t GetWcharToUCharBufferSize(const size_t wchar_length, const size_t scale);

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
  static size_t GetUCharBufferSize(const size_t utf8_length, const float scale);

  /*!
   * \brief Calculates a reasonably sized UChar buffer based upon the size of existing
   * UChar string.
   *
   * \param uchar_length byte-length of UTF-8 string to be converted
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
  static size_t GetUCharWorkingSize(const size_t uchar_length, const size_t scale);

  /*!
   * \brief Calculates the maximum number of UTF-8 bytes required by a UTF-16 string.
   *
   * \param uchar_length Number of UTF-16 code units to convert
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
  static size_t GetUTF8BufferSize(const size_t uchar_length, const size_t scale);

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
   * \param uchar_length number of UTF-16 code units in original string
   * \param scale scale factor to multiply uchar_length by to allow for growth
   * \returns Number of wchar_t characters to allocate for the desired buffer
   *          with a fairly generous amount of room for growth
   *
   * Note that an additional 200 code units is added to the result to allow for growth
   *
   */
  static size_t GetWCharBufferSize(const size_t uchar_length, const size_t scale);

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
  static icu::Locale GetICULocale(const std::locale &locale);

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

  static icu::Locale GetICULocale(const char *language, const char *country = 0,
      const char *variant = 0, const char *keywordsAndValues = 0);

  /*!
   * \brief Construct a simple locale id based upon an icu::Locale
   *
   * \param locale to get an id for
   * \return a string <ll>_<cc> where ll is language and cc is country.
   *         The default language is "en". If <cc> is not present then only <ll>
   *         is returned
   */
  static const std::string GetICULocaleId(const icu::Locale locale);

  /*!
   *  \brief Folds the case of a string, independent of Locale.
   *
   * Similar to ToLower except in addition, insignificant accents are stripped
   * and other transformations are made (such as German sharp-S is converted to ss).
   * The transformation is independent of locale.
   *
   * \param str string to fold in place.
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

  static const std::wstring FoldCase(const std::wstring &src, const StringOptions options);

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
  static const std::string FoldCase(const std::string &src, const StringOptions options);

private:

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
   */
  static void FoldCase(const icu::StringPiece strPiece, icu::CheckedArrayByteSink &sink,
      UErrorCode &status, const int32_t options);

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
   *  \param options fine tunes behavior. See StringOptions. Frequently can leave
   *         at default value.
   *  \param NormalizerType select the appropriate Normalizer for the job
   *  \return Normalized string
   */
  static const std::wstring Normalize(const std::wstring &src, const StringOptions option,
      const NormalizerType NormalizerType = NormalizerType::NFKC);

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
  static const std::string Normalize(const std::string &src, const StringOptions options,
      const NormalizerType NormalizerType = NormalizerType::NFKC);

private:
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
   * \param NormalizerType select the appropriate Normalizer for the job
   */
  static void Normalize(const icu::StringPiece strPiece, icu::CheckedArrayByteSink &sink,
      UErrorCode &status, const int32_t options, const NormalizerType NormalizerType);
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
  static const std::string ToUpper(const std::string &src, const icu::Locale &locale);

private:

  /*!
   * \brief Converts a UChar buffer to Upper case according to locale.
   *
   * Note: The length of the string can change, depending upon the underlying
   * icu::locale.
   *
   * \param p_u_src_buffer UChar buffer to change case on
   * \param u_src_length length of p_u_src in UChars
   * \param icu::Locale governs the ToUpper behavior
   * \param p_u_toupper_buffer buffer to write the results of ToUpper on p_u_src_buffer
   * \param u_toupper_buffer_size size in UChars of p_u_toupper_buffer
   * \param to_upper_length the actual number of UChars written to p_u_toupper_buffer
   * \param status UErrorCode indicating the status of the operation. Make sure to
   *        set to U_ZERO_ERROR before calling this function
   */
  static void ToUpper(UChar *p_u_src_buffer, int32_t u_src_length, const icu::Locale locale,
      UChar *p_u_toupper_buffer, const int32_t u_toupper_buffer_size, int32_t &to_upper_length,
      UErrorCode &status);

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
  static const std::string ToLower(const std::string &src, const icu::Locale &locale);

  /*!
   *  \brief Capitalizes a string using Legacy Kodi rules.
   *
   * Uses a simplistic approach familiar to English speakers.
   * See TitleCase for a more locale aware solution.
   *
   * \param src string to capitalize
   * \return src capitalized
   */
  static const std::string ToCapitalize(const std::string &src, const icu::Locale &locale);
  /*!
   *  \brief Capitalizes a wstring using Legacy Kodi rules.
   *
   * Uses a simplistic approach familiar to English speakers.
   * See TitleCase for a more locale aware solution.
   *
   * \param src string to capitalize
   * \return src capitalized
   */
  static const std::wstring ToCapitalize(const std::wstring &src, const icu::Locale &locale);

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
  static const std::wstring ToTitle(const std::wstring &src, const icu::Locale &locale);

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
  static const std::string ToTitle(const std::string &src, const icu::Locale &locale);

  /*!
   * \brief Compares two wstrings using codepoint order. Locale does not matter.
   *
   * \param s1 one of the strings to compare
   * \param s1_start specifies the beginning byte of s1 to compare
   * \param s1_length specifies the number of bytes of s1 to compare
   * \param s2 one of the strings to compare
   * \param s2_start specifies the beginning byte of s2 to compare
   * \param s2_length specifies the number of bytes of s2 to compare
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
  static int8_t StrCmp(const std::wstring &s1, size_t s1_start, size_t s1_length,
      const std::wstring &s2, size_t s2_start, size_t s2_length, const bool Normalize = false);

  /*!
   * \brief Compares two strings using codepoint order.
   *
   * \param s1 one of the strings to compare
   * \param s1_start specifies the beginning byte of s1 to compare
   * \param s1_length specifies the number of bytes of s1 to compare
   * \param s2 one of the strings to compare
   * \param s2_start specifies the beginning byte of s2 to compare
   * \param s2_length specifies the number of bytes of s2 to compare
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
  static int8_t StrCmp(const std::string &s1, size_t s1_start, size_t s1_length,
      const std::string &s2, size_t s2_start, size_t s2_length, const bool Normalize = false);

  /*!
   * \brief Performs a bit-wise comparison of two wstrings, after case folding each.
   *
   * Logically equivalent to StrCmp(FoldCase(str1, opt)), FoldCase(str2, opt))
   * or, if Normalize == true: Strcmp(NFD(FoldCase(NFD(str1))), NFD(FoldCase(NFD(str2))))
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
   * = 0 if str1 contains the same characters as s2,
   * > 0 if the characters in s1 are bitwise greater than the characters in s2.
   */
  static int StrCaseCmp(const std::wstring &s1, const std::wstring &s2,
      const StringOptions options, const bool Normalize = false);

  /*!
   * \brief Performs a bit-wise comparison of two strings, after case folding each.
   *
   * Logically equivalent to StrCmp(FoldCase(str1, opt)), FoldCase(str2, opt))
   * or, if Normalize == true: Strcmp(NFD(FoldCase(NFD(str1))), NFD(FoldCase(NFD(str2))))
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
   * = 0 if str1 contains the same characters as s2,
   * > 0 if the characters in s1 are bitwise greater than the characters in s2.
   */
  static int StrCaseCmp(const std::string &s1, const std::string &s2,
      const StringOptions options, const bool normalize = false);

  /*!
   * \brief Performs a bit-wise comparison of two strings, after case folding each.
   *
   * Logically equivalent to StrCmp(FoldCase(str1, options)), 0, n, FoldCase(str2, options), 0, n)
   * or, if Normalize == true: Strcmp(NFD(FoldCase(NFD(str1))), 0, n,  NFD(FoldCase(NFD(str2))), 0, n)
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
   * \param n number of bytes to compare from each string
   * \param opt StringOptions to apply. Generally leave at the default value
   * \param Normalize Controls whether normalization is performed before and after
   *        case folding
   * \return The result of bitwise character comparison:
   * < 0 if the characters s1 are bitwise less than the characters in s2,
   * = 0 if str1 contains the same characters as s2,
   * > 0 if the characters in s1 are bitwise greater than the characters in s2.
   */
  static int StrCaseCmp(const std::string &s1, const std::string &s2, size_t n,
      const StringOptions options, const bool Normalize = false);

  /*!
   * \brief Performs a bit-wise comparison of two strings, after case folding each.
   *
   * Logically equivalent to StrCmp(FoldCase(str1, opt)), FoldCase(str2, opt))
   * or, if Normalize == true: Strcmp(NFD(FoldCase(NFD(str1))), NFD(FoldCase(NFD(str2))))
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
   * \param s1_start specifies the starting byte of s1 to compare
   * \param s1_length specifies the number of bytes of s1 to compare
   * \param s2 one of the strings to compare
   * \param s2_start specifies the starting byte of s2 to compare
   * \param s2_length specifies the number of bytes of s2 to compare
   * \param opt StringOptions to apply. Generally leave at the default value
   * \param Normalize Controls whether normalization is performed before and after
   *        case folding
   * \return The result of bitwise character comparison:
   * < 0 if the characters s1 are bitwise less than the characters in s2,
   * = 0 if str1 contains the same characters as s2,
   * > 0 if the characters in s1 are bitwise greater than the characters in s2.
   */
  static int StrCaseCmp(const std::string &s1, size_t s1_start, size_t s1_length,
      const std::string &s2, size_t s2_start, size_t s2_length, const StringOptions options,
      const bool Normalize = false);
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
   * \param Normalize Controls whether normalization is performed prior to collation.
   *                  Frequently not required. Some free normalization always occurs.
   * \return true if initialization was successful, otherwise false.
   *
   * Normally leave normalize off. Some free normalization is still performed.
   * To improve performance, normalize every record prior to sort instead of enabling
   * it here since normalization is performed on each comparison.
   *
   * See the documentation for UCOL_NORMALIZATION_MODE for more info.
   */
  static bool InitializeCollator(const std::locale locale, bool Normalize = false);

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
   * \param Normalize Controls whether normalization is performed prior to collation.
   *                  Frequently not required. Some free normalization always occurs.
   * \return true if initialization was successful, otherwise false.
   *
   * Normally leave normalize off. Some free normalization is still performed.
   * To improve performance, normalize every record prior to sort instead of enabling
   * it here since normalization is performed on each comparison.
   *
   * See the documentation for UCOL_NORMALIZATION_MODE for more info.
   */
  static bool InitializeCollator(const icu::Locale icuLocale, bool Normalize = false);

  /*!
   * \brief Allows some performance statistics to be generated based upon the previous sort
   *
   * Data collection may be controlled by a setting or #define
   */
  static void SortCompleted(int sortItems);

  /*!
   * \brief Collates two wstrings using the most recent collator setup in the same
   * thread by InitializeCollator.
   *
   * \param left one of the strings to collate
   * \param right the other string to collate
   * \return A value < 0, 0 or > 0 depending upon whether left collates before,
   *         equal to or after right
   */
  static int32_t Collate(const std::wstring &left, const std::wstring &right);

  /*!
   * \brief Determines if a string begins with another string
   *
   * \param s1 string to be searched
   * \param s2 string to find at beginning of s1
   * \return true if s1 starts with s2, otherwise false
   */
  static bool StartsWith(const std::string &s1, const std::string &s2);
  /*
   * \brief Determines if a string begins with another string, ignoring case
   *
   * \param s1 string to be searched
   * \param s2 string to find at beginning of s1
   * \param options controls behavior of case folding, normally leave at default
   * \return true if s1 starts with s2, otherwise false
   */
  static bool StartsWithNoCase(const std::string &s1, const std::string &s2,
      const StringOptions options);

  /*!
   * \brief Determines if a string ends with another string
   *
   * \param s1 string to be searched
   * \param s2 string to find at end of s1
   * \return true if s1 ends with s2, otherwise false
   */
  static bool EndsWith(const std::string &s1, const std::string &s2);

  /*!
   * \brief Determines if a string ends with another string, ignoring case
   *
   * \param s1 string to be searched
   * \param s2 string to find at end of s1
   * \param options controls behavior of case folding, normally leave at default
   * \return true if s1 ends with s2, otherwise false
   */
  static bool EndsWithNoCase(const std::string &s1, const std::string &s2,
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
  static std::string Left(const std::string &str, size_t charCount,
      const icu::Locale icuLocale, const bool keepLeft = true);

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
  static std::string Mid(const std::string &str, size_t startCharIndex,
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
  static std::string Right(const std::string &str, size_t charCount,
      const icu::Locale &icuLocale, bool keepRight = true);

private:

  /*!
   * \brief configures UText for a UTF-8 string
   *
   * For those methods which can take a UText, this allows for a UTF-8 string to be
   * used without converting it to UTF-16/UnicodeString up-front. Instead it can be done
   * on demand as the string is iterated. Having to manually free one or more pointers can
   * be a pain. Further, some methods have to scan the entire string up-front, depending upon
   * what is being done.
   *
   * \param str UTF-8 string to create the UText for
   * \return a UText object that contains the passed UTF8 string for the iterator
   *           On error, nullptr is returned
   *
   * *** Note: utext_close(<return value>) MUST be called when finished using the iterator
   * or there will be a memory leak.
   */
  static UText * ConfigUText(const std::string& str, UText* ut = nullptr);

  /*!
   * \brief Configures a Character BreakIterator for use
   *
   * \param str UTF-8 string to create the Character BreakIterator for
   * \icuLocale locale to configure the iterator for
   * \return a Character BreakIterator object that uses the passed UTF8 string for the iterator
   *           On error, nullptr is returned.
   *
   * *** Note: utext_close(<return value>) MUST be called when finished using the iterator, including
   * when this method returns nullptr, or there will be a memory leak.
   */
  static icu::BreakIterator* ConfigCharBreakIter(UText* ut, const icu::Locale& icuLocale);

  static icu::BreakIterator* ReConfigCharBreakIter(UText* ut, icu::BreakIterator* cbi);

  /*!
   * \brief Configures a Character BreakIterator for use
   *
   * \param str String to create the Character BreakIterator for
   * \icuLocale locale to configure the iterator for
   * \return true is returned on success, otherwise false.
   */
  static icu::BreakIterator* ConfigCharBreakIter(const icu::UnicodeString& str, const icu::Locale& icuLocale);


  static size_t GetCharPosition(icu::BreakIterator* cbi, size_t stringLength, size_t charCount, const bool left, const bool keepLeft);

public:

  static size_t GetCharPosition(icu::BreakIterator * cbi, const std::string &str, const size_t charCount,
      const bool left, const bool keepLeft, const icu::Locale& icuLocale);

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
  static size_t GetCharPosition(const std::string &str, const size_t charCountArg, const bool left,
      const bool keepLeft, const icu::Locale &icuLocale);

  size_t GetCharPosition(const icu::UnicodeString &uStr, const size_t charCountArg, const bool left,
      const bool keepLeft, const icu::Locale &icuLocale);
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
   * /
  static size_t GetCharPosition(const icu::UnicodeString &str, const size_t charCount,
      const bool left, const bool keepLeft, const icu::Locale& icuLocale);
   */

  /*!
   * \brief Removes leading and trailing whitespace from the string
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
  static std::string Trim(const std::string &str);

  /*!
   * \brief Removes leading whitespace from a string
   *
   * \param str string to trim
   * \return trimmed string
   *
   * See Trim(str) for more details about what counts as whitespace
   */
  static std::string TrimLeft(const std::string &str);

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
  static std::string TrimLeft(const std::string &str, const std::string trimChars);

  /*!
   * \brief Removes trailing whitespace from a string
   *
   * \param str string to trim
   * \return trimmed string
   *
   * See Trim(str) for more details about what counts as whitespace
   */
  static std::string TrimRight(const std::string &str);

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
  static std::string TrimRight(const std::string &str, const std::string trimChars);

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
  static std::string Trim(const std::string &str, const std::string &trimStrings, const bool trimLeft,
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
  static std::string Trim(const std::string &str,
      const std::vector<std::string> &trimStrings, const bool trimLeft, const bool trimRight);

private:
  /*!
   * \brief Remove a set of characters from ends of a UnicodeString
   *
   * \param str UnicodeString to trim
   * \param trimChars set of characters to remove from str
   * \param trimLeft if true, then trim from left end of string
   * \param trimRight if true, then trim from right end of string
   * \return trimmed UnicodeString
   */
  static icu::UnicodeString Trim(const icu::UnicodeString &str, const icu::UnicodeString &trimChars,
      const bool trimLeft, const bool trimRight);

  /*!
   * \brief Remove a set of UnicodeStrings from ends of a UnicodeString
   *
   * \param str UnicodeString to trim
   * \param trimStrings set of strings to remove from str
   * \param trimLeft if true, then trim from left end of string
   * \param trimRight if true, then trim from right end of string
   * \return trimmed UnicodeString
   */
  static icu::UnicodeString Trim(const icu::UnicodeString &uStr,
      const std::vector<icu::UnicodeString> &trimStrings, const bool trimLeft, const bool trimRight);

  /*!
   * \brief Remove whitespace from a UnicodeString
   *
   * \param str to trim
   * \param trimLeft if true, then trim from left end of string
   * \param trimRight if true, then trim from right end of string
   * \return trimmed string
   */
  static icu::UnicodeString Trim(const icu::UnicodeString &str, const bool trimLeft,
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
  static std::vector<std::string> SplitMulti(const std::vector<std::string> &input,
      const std::vector<std::string> &delimiters, size_t iMaxStrings = 0);

private:
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
  static std::vector<icu::UnicodeString> SplitMulti(const std::vector<icu::UnicodeString> &input,
      const std::vector<icu::UnicodeString> &delimiters, size_t iMaxStrings/* = 0 */);
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
  [[deprecated("FindAndReplace is faster, returned count not used.") ]]
   static std::tuple<std::string, int> FindCountAndReplace(const std::string &src, const std::string &oldText,
       const std::string &newText);

private:

  /*!
   * \brief Replace all occurrences of characters in oldText with the characters
   *        in newText
   * \param oldText the text containing the search text
   * \param newText the text containing the replacement text
   * \return a Tuple with a reference to the modified src and an int containing
   *         the number of times the oldText was replaced.
   */
  static std::tuple<icu::UnicodeString, int> FindCountAndReplace(const icu::UnicodeString &kSrc,
      const icu::UnicodeString &kOldText, const icu::UnicodeString &kNewText);

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
  static std::tuple<icu::UnicodeString, int>
  FindCountAndReplace(const icu::UnicodeString &srcText, const int32_t start, const int32_t length,
      const icu::UnicodeString &oldText, const int32_t oldStart, const int32_t oldLength,
      const icu::UnicodeString &newText, const int32_t newStart, const int32_t newLength);

public:
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
  static bool FindWord(const std::string &str, const std::string &word);

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
  static std::string FindAndReplace(const std::string &str, const std::string oldText,
      const std::string newText);

  /*!
   * \brief Find a regular expressoin pattern in a str
   *
   * \param str string to search
   * \param pattern icu::regex pattern
   * \param flags that control behavior of regex. See enum RegexpFlag
   * \return byte offset at beginning of first match of the pattern or
   *         string::npos if no match found
   *
   * Regular expression patterns for this lib can be found at:
   * https://unicode-org.github.io/icu/userguide/strings/regexp.html
   *
   */
  static size_t RegexFind(const std::string &str, const std::string pattern, const int flags);

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
  static std::string RegexReplaceAll(const std::string &str, const std::string pattern,
      const std::string replace, const int flags);

private:

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
  icu::UnicodeString RegexReplaceAll(const icu::UnicodeString &uString,
      const icu::UnicodeString uPattern, const icu::UnicodeString uReplace, const int flags);

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
  static int32_t countOccurances(const std::string &strInput, const std::string &strFind,
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
  template<typename OutputIt>
  static OutputIt SplitTo(OutputIt d_first, const std::string &input, const char delimiter, size_t iMaxStrings /*= 0*/)
  {
    return SplitTo(d_first, input, std::string(1, delimiter), iMaxStrings);
  }

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
  static OutputIt SplitTo(OutputIt d_first, const std::string &input,
      const std::vector<std::string> &delimiters, size_t iMaxStrings /* = 0 */)
  {
    // TODO: Verify why this can not be done with plain string.

    OutputIt dest = d_first;
    if (input.empty()) // Return nothing, not even empty string.
      return dest;

    if (delimiters.empty())
    {
      *dest++ = input;
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
  static OutputIt SplitTo(OutputIt d_first, const std::string &input, const std::string &delimiter,
      size_t iMaxStrings = 0)
  {
    OutputIt dest = d_first;

    if (input.empty())
      return dest;
    if (delimiter.empty())
    {
      *d_first++ = input;
      return dest;
    }

    icu::UnicodeString uInput = ToUnicodeString(input);
    icu::UnicodeString uDelimiter = ToUnicodeString(delimiter);
    std::vector<icu::UnicodeString> uResult = std::vector<icu::UnicodeString>();

    bool omitEmptyStrings = false;
    Unicode::SplitTo(std::back_inserter(uResult), uInput, uDelimiter, iMaxStrings, omitEmptyStrings);

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

private:

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
  static OutputIt SplitTo(OutputIt d_first, const icu::UnicodeString &uInput,
      const icu::UnicodeString &uDelimiter, size_t iMaxStrings = 0, const bool omitEmptyStrings = false);

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
  static OutputIt SplitTo(OutputIt d_first, icu::UnicodeString uInput,
      const std::vector<icu::UnicodeString> &uDelimiters,  size_t iMaxStrings = 0, const bool omitEmptyStrings = false);

  /*!
   * \brief indicates whether a string contains any of a vector of keywords
   *
   * \param str string to check
   * \param keywords if any of these keywords are contained in str, then true is
   * returned
   * \return true of str contains any of the strings in keywords
   */
  static bool Contains(const std::string &str, const std::vector<std::string> &keywords);

private:

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
