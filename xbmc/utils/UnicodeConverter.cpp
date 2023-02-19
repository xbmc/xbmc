/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

//
// This code is heavily based upon CharacterSetConverter. This version attempts
// to resolve several problems. I hope to get these problems resolved in
// CharacterSetConverter, but did not want to depend on that to block this effort.
//
// The resolved issues are:
//
// - StringUtils could NOT reliably use CharacterSetConverter because the later
//   does not initialize before StringUtils uses it. I made numerous attempts to
//   fix this problem without major changes to CharacterSetConverter and failed.
//   But I don't claim to be a C++ expert.
//
// - string_view arguments are used, reducing copies of dynamic memory
//
// - converted string returned instead of pass by reference. The boolean status value
//   returned earlier was ignored anyway.
//
// - Added option to replace bad-character conversions with the psuedo-offical
//   Unicode Substitution codepoint: U"\x0fffd"sv == '�'.
//
// - Make cache of iconv converters thread_local rather than using locks. This
//   resolved the first (initialization issue) and should improve performance since
//   locks aren't needed, nor will there be contention for them.
//
// - Use buffer allocated from stack instead of malloc. Should improve performance.
//
//
// What was lost:
//
// - Only supports Unicode conversions. This could easily be reversed
//
// I spent a LOT of time trying to leverage the use of templates. I think that I
// partially succeeded, but I am not happy with the results yet. It is clearly
// overkill and needs cleaning up. Templating is a BIG subject. I am eager
// for any suggestions.
//
// The vast majority of the code doesn't do so much. The heart is
// CUnicodeConverter::CInnerUCodeConverter::convert, which is fairly short
//
// If less frequently used converters are added, then should examine cost of
// keeping those in thread_local memory vs creation.
//
// TODO:  Review and correct platform specific conversions. Focused on Linux
//

#include "UnicodeConverter.h"

#include "utils/Utf8Utils.h"
#include "utils/log.h"
// TODO Remove For debug print
#include "utils/StringUtils.h"

#include <algorithm>
#include <iostream>
#include <string>
#include <string_view>

#include <iconv.h>

#ifdef WORDS_BIGENDIAN
#define ENDIAN_SUFFIX "BE"
#else
#define ENDIAN_SUFFIX "LE"
#endif

#if defined(TARGET_DARWIN)
// Mac uses NFD normalization for UTF-8-MAC
// Unicode allows for some characters 'graphemes' to be represented multiple ways.
// For example, some characters with an accent can be represented as a single
// codepoint, OR, can be represented as two (or more) codepoints: letter + accent mark
//
// Both logically equivalent, but for byte comparisons you want to use the same
// representation everywhere.

#define WCHAR_IS_UCS_4 1
#define UTF16_CHARSET "UTF-16" ENDIAN_SUFFIX
#define UTF32_CHARSET "UTF-32" ENDIAN_SUFFIX
#define UTF8_SOURCE "UTF-8-MAC"
#define WCHAR_CHARSET UTF32_CHARSET
#elif defined(TARGET_WINDOWS)
#define WCHAR_IS_UTF16 1
#define UTF16_CHARSET "UTF-16" ENDIAN_SUFFIX
#define UTF32_CHARSET "UTF-32" ENDIAN_SUFFIX
#define UTF8_SOURCE "UTF-8"
#define WCHAR_CHARSET UTF16_CHARSET
#elif defined(TARGET_FREEBSD)
#define WCHAR_IS_UCS_4 1
#define UTF16_CHARSET "UTF-16" ENDIAN_SUFFIX
#define UTF32_CHARSET "UTF-32" ENDIAN_SUFFIX
#define UTF8_SOURCE "UTF-8"
#define WCHAR_CHARSET UTF32_CHARSET
#elif defined(TARGET_ANDROID)
#define WCHAR_IS_UCS_4 1
#define UTF16_CHARSET "UTF-16" ENDIAN_SUFFIX
#define UTF32_CHARSET "UTF-32" ENDIAN_SUFFIX
#define UTF8_SOURCE "UTF-8"
#define WCHAR_CHARSET UTF32_CHARSET
#else
#define UTF16_CHARSET "UTF-16" ENDIAN_SUFFIX
#define UTF32_CHARSET "UTF-32" ENDIAN_SUFFIX
#define UTF8_SOURCE "UTF-8"
#define WCHAR_CHARSET "WCHAR_T"
#if __STDC_ISO_10646__
#ifdef SIZEOF_WCHAR_T
#if SIZEOF_WCHAR_T == 4
#define WCHAR_IS_UCS_4 1
#elif SIZEOF_WCHAR_T == 2
#define WCHAR_IS_UCS_2 1
#endif
#endif
#endif
#endif

#define NO_ICONV ((iconv_t)-1)
#define UCS4_CHARSET "UCS-4LE"

using namespace std::literals;

// TODO: Remove Useful for debugging remote Windows

static constexpr bool TRACE_OUTPUT{false};
// static constexpr bool STOP_TRACE{false};

// Replace bad codepoints with the semi-offical Unicode 'substitution' character
// 0xfffd. This is commonly used by browsers, etc. This character displays as �

static constexpr std::string_view UTF8_SUBSTITUTE_CHARACTER{"\xef\xbf\xbd"sv};
static constexpr std::wstring_view WCHAR_T_SUBSTITUTE_CHARACTER{L"\xfffd"sv};
static constexpr std::u32string_view CHAR32_T_SUBSTITUTE_CHARACTER{U"\x0000fffd"sv};

// over-allocate the size of our conversion buffer. Shouldn't need any padding,
// but it is allocated from stack and will be gone soon, so what is the harm?

static constexpr size_t paddingBytes{512};

class CUnicodeConverterType
{
  //
  // Since this is based on CharSetConverter there are some artifacts that are not
  // so necessary now, however, they are kept to help any effort to merge any of
  // the changes here into CharSetConverter. Much was changed, but the skeleton is
  // still present. The CUnicodeConverterType is one of the items that is not
  // so necessary with UnicodeConverter.
  //
  // Simple class to keep the necessary information to create an iconv instance
  // as well as that instance once created.

public:
  CUnicodeConverterType(const std::string_view sourceCharset,
                        const std::string_view targetCharset,
                        size_t targetSingleCharMaxLen = 1);
  ~CUnicodeConverterType();

  iconv_t PrepareConverter();
  CUnicodeConverterType& operator=(const CUnicodeConverterType& other)
  {
    m_sourceCharset = other.m_sourceCharset;
    m_targetCharset = other.m_targetCharset;
    m_iconv = other.m_iconv;
    m_targetSingleCharMaxLen = other.m_targetSingleCharMaxLen;

    return *this;
  }

  std::string GetSourceCharset(void) const { return m_sourceCharset; }
  std::string GetTargetCharset(void) const { return m_targetCharset; }
  size_t GetTargetSingleCharMaxLen(void) const { return m_targetSingleCharMaxLen; }

private:
  std::string m_sourceCharset;
  std::string m_targetCharset;
  iconv_t m_iconv;
  size_t m_targetSingleCharMaxLen;
}; // CUnicodeConverterType

CUnicodeConverterType::CUnicodeConverterType(const std::string_view sourceCharset,
                                             const std::string_view targetCharset,
                                             size_t targetSingleCharMaxLen /*= 1*/)
  : m_sourceCharset(sourceCharset),
    m_targetCharset(targetCharset),
    m_iconv(NO_ICONV),
    m_targetSingleCharMaxLen(targetSingleCharMaxLen)
{
}

CUnicodeConverterType::~CUnicodeConverterType()
{
  if (m_iconv != NO_ICONV)
  {
    iconv_close(m_iconv);
    m_iconv = NO_ICONV;
  }
}

iconv_t CUnicodeConverterType::PrepareConverter()
{
  if (m_iconv == NO_ICONV)
  {
    m_iconv = iconv_open(m_targetCharset.data(), m_sourceCharset.data());

    if (m_iconv == NO_ICONV)
      CLog::Log(LOGERROR, "{}: iconv_open() for \"{}\" -> \"{}\" failed, errno = {} ({})",
                __FUNCTION__, m_sourceCharset, m_targetCharset, errno, strerror(errno));
  }
  return m_iconv;
}

enum StdConversionType /* Keep it in sync with CUnicodeConverter::CInnerUCodeConverter::m_stdConversion */
{
  // Removed converters not needed for Unicode. It should not be difficult to re-add them.

  NoConversion = -1,
  Utf8ToUtf32 = 0,
  Utf32ToUtf8,
  Utf32ToW,
  WToUtf32,
  WToUtf8,
  Utf8ToW,
  NumberOfStdConversionTypes /* Dummy sentinel entry */
}; // StdConversionType

// This is where I went nuts creating templates. I am brand-new to Templates
// and have much to learn. It turns out that templating strings/string_views
// is fairly complicated since the basic_string and basic_string_view family
// are complex templates themselves.
//
// The main idea was to 1) reduce the amount of custom code/casting 2)
// have more type checking. I'm not sure if this is any better than the
// bare-bones templating that exists in CharSetConverter.

//
// The following Templates are building-blocks for the templates used
// for function parameters. These Templates, such as "enable_if_basic_string"
// tells the compiler (and reader) that the parameter is expected to be
// one of the std::basic_strings itemized (char, wchar_t, char32_t). The
// compiler only takes it as a hint, but with enough hints and the static
// code it can frequently determine all of the possible usages and then
// can do better type checking and code generation.
//
// When enable_if_basic_string finds a match with a parameter, it returns
// the matching parameter type info (or some other specified value) to
// the caller. This makes it easier for hints to be passed along to
// other Templates, such as the character type of a basic_string.
//
// Documentation will be sparse here, since they tend to follow the same
// pattern.

namespace
{
/*!
 * Template: enable_if_char_type
 *
 * \brief Verifies that the given type is for one of the supported character
 *        types (std::basic_string).
 *
 * \tparam CHAR_T_ARG  a character type, such as "char", "wchar_t" or "char32_t"
 *
 * \tparam T  the type of the evaluated template on successful match.
 *            Default is CHAR_T_ARG
 */
template<typename CHAR_T_ARG, typename T = CHAR_T_ARG>
using enable_if_char_type =
    std::enable_if<std::is_base_of_v<char, CHAR_T_ARG> || std::is_base_of_v<wchar_t, CHAR_T_ARG> ||
                       std::is_base_of_v<char32_t, CHAR_T_ARG>,
                   T>;

/*!
 *
 * \brief Verifies that the given argument is one of the supported
 *        basic_string types
 *
 * \tparam STR_T must be one of string, wstring, u32string
 *
 * \tparam T the type of the evaluated template if a match is found. Default is STR_T
 */
template<typename STR_T, typename T = STR_T>
using enable_if_basic_string =
    std::enable_if<std::is_base_of_v<std::basic_string<char>, STR_T> ||
                       std::is_base_of_v<std::basic_string<wchar_t>, STR_T> ||
                       std::is_base_of_v<std::basic_string<char32_t>, STR_T>,
                   T>;

/*!
 * \brief Ensures that an argument is string, wstring or u32string. Also
 *        returns type field of string, which can be used to determine
 *        the appropriate character type of the string
 *
 * \tparam STR_T must be an approved string type
 *
 * \tparam T The type of the evaluated template. Default STR_T
 */
template<typename STR_T, typename T = STR_T>
using enable_if_basic_string_t = typename enable_if_basic_string<STR_T, T>::type;

/*!
 *
 * \brief Verifies that the given argument is one of the supported
 *        string_view types
 *
 * \tparam STR_T must be one of string_view, wstring_view, u32string_view
 *
 * \tparam T the type of the evaluated template if a match is found, the default is STR_T
 */
template<typename STR_T, typename T = STR_T>
using enable_if_basic_string_view =
    typename std::enable_if<std::is_base_of_v<std::basic_string_view<char>, STR_T> ||
                                std::is_base_of_v<std::basic_string_view<wchar_t>, STR_T> ||
                                std::is_base_of_v<std::basic_string_view<char32_t>, STR_T>,
                            T>;

/*!
 * \brief Ensures that an argument is an approved string_view type. Also
 *        returns type field of string, which can be used to determine
 *        the appropriate character type of the string
 *
 * \tparam STR_T must be one of string_view, wstring_view or u32string_view
 *
 * \tparam T the type of the evaluated template if a match is found, the default is STR_T
 */
template<typename STR_T, typename T = STR_T>
using enable_if_basic_string_view_t = typename enable_if_basic_string_view<STR_T, T>::type;

/*!
 * \brief Ensures that an argument is an approved string or string_view type.
 *
 * \tparam STR_T must be one of: string, wstring, u32string, string_view,
 *         wstring_view or u32string_view
 *
 * \tparam T the type of the evaluated template if a match is found, the default is STR_T
 */
template<typename STR_T, typename T = STR_T>
using enable_if_any_string_type =
    typename std::enable_if<std::is_base_of_v<std::basic_string_view<char>, STR_T> ||
                                std::is_base_of_v<std::basic_string_view<wchar_t>, STR_T> ||
                                std::is_base_of_v<std::basic_string_view<char32_t>, STR_T> ||
                                std::is_base_of_v<std::basic_string<char>, STR_T> ||
                                std::is_base_of_v<std::basic_string<wchar_t>, STR_T> ||
                                std::is_base_of_v<std::basic_string<char32_t>, STR_T>,
                            T>;

/*!
 * \brief Ensures that an argument is an approved string or string_view type. Also
 *        returns type field of string, which can be used to determine
 *        the appropriate character type of the string
 *
 * \tparam STR_T must be one of: string, wstring, u32string, string_view,
 *         wstring_view or u32string_view
 *
 * \tparam T the Type of the evaluated template. Default STR_T
 */
template<typename STR_T, typename T = STR_T>
using enable_if_any_string_type_t = typename enable_if_any_string_type<STR_T, T>::type;

} // namespace

class CUnicodeConverter::CInnerUCodeConverter
{

public:
  class CConversionUtils
  {
    /**
     * Define a number of utility functions.
     * Most likely went overboard here, but attempting to keep
     * the main body of code focused on their task and move all of the little
     * calculations over here. Not sure how well that is achieved.
     *
     * iconv works in bytes, but we work in code-units (sizeof(CharT)).
     */
  private:
  public:
    /*!
     * \brief extract the number of bytes per code-unit for the char-type
     *        from the given template.
     *
     * \tparam STR_T_ARG Type of string-like argument to get the code-unit size of.
     *               Must be one of: string, wstring, u32string, string_view,
     *               wstring_view or u32string_view
     *
     * \tparam [OPT] STR_T automatically set to the type field of the
     *                    matching STR_T_ARG string type
     *
     * \tparam [OPT] CHAR_T automatically set to the string type of
     *                      the matching STR_T_ARG
     *
     * \return  Number of bytes in the
     */
    template<typename STR_T_ARG,
             typename STR_T = enable_if_any_string_type_t<STR_T_ARG>,
             typename CHAR_T = typename STR_T::value_type>

    static constexpr size_t BytesPerCodeUnit()
    {
      constexpr size_t bytesPerCodeUnit{sizeof(CHAR_T)};
      return bytesPerCodeUnit;
    }

    /*!
     * \brief Gets the size of a string in bytes
     *
     * \tparam STR_T_ARG Type of string-like argument to get the byte length of.
     *               Must be one of: string, wstring, u32string, string_view,
     *               wstring_view or u32string_view
     *
     * \tparam [OPT] STR_T automatically set to the type field of the
     *                    matching STR_T_ARG string type
     *
     * \tparam [OPT] CHAR_T automatically set to the string type of
     *                      the matching STR_T_ARG
     *
     * \param str    string in CHAR_T to get length of
     *
     * \return  size of str, in bytes
     */

    template<typename STR_T_ARG,
             typename STR_T = enable_if_any_string_type<STR_T_ARG>,
             typename CHAR_T = typename STR_T::value_type>

    static size_t SizeInBytes(STR_T str)
    {
      const size_t bytesPerCodeUnit{sizeof(typename STR_T::value_type)};
      return bytesPerCodeUnit * str.length();
    }

    /*!
     * \brief Gets a char (byte) pointer to the string contained in the
     *        given string/string_view
     *
     * \param STR_T_ARG is a basic_string_view
     *
     * \tparam STR_T      STR_T_ARG if it matched any std::basic_string_view
     *
     * \tparam CHAR_T     The character type of any matching STR_T_ARG
     *
     * \param str         string in CHAR_T to cast the pointer to
     *
     * \return  size of str, in bytes
     */
    template<typename STR_T_ARG,
             typename STR_T = enable_if_basic_string_view_t<STR_T_ARG>,
             typename CHAR_T = typename STR_T::value_type>

    static const char* BytePointer(STR_T str)
    {
      return static_cast<const char*>(static_cast<const void*>(str.data()));
    }

    /*!
     * \brief Appends the converted bytes from a char buffer to the output basic_string
     *
     * \tparam STR_T_ARG        type of the basic_string to move the buffer contents to
     * \tparam [OPT] STR_T      set to STR_T_ARG if enable_if_basic_string finds a match
     * \tparam [OPT] CHAR_T     set to the character type of STR_T
     * \param buffer            the buffer contents of type OUTPUT_BUF_T
     * \param outputString      the contents of STR_T type basic_string to copy to
     * \param lengthInCU the CHAR_T codeunits of data to copy
     */
    template<typename STR_T_ARG,
             typename STR_T = enable_if_basic_string<STR_T_ARG>,
             typename CHAR_T = typename STR_T::value_type>

    static void append(const char* buffer, STR_T& outputString, size_t lengthInCU)
    {
      const CHAR_T* codeUnitPointer = static_cast<const CHAR_T*>(static_cast<const void*>(buffer));
      outputString.append(codeUnitPointer, lengthInCU);
    }

    /*!
     * \brief Appends a substitution codepoint for a bad character to the output basic_string
     *
     * \tparam STR_OUT_T_ARG    type of the basic_string to copy the substitution
     *                          codepoint to
     * \tparam STR_OUT_T        set to STR_OUT_T_ARG if enable_if_basic_string finds a match
     * \tparam CHAR_T           set to the character type of STR_OUT_T_ARG
     * \param result            the contents of STR_OUT_T_ARG type basic_string to copy to
     */
    template<typename ST_OUT_T_ARG,
             typename ST_OUT_T = enable_if_basic_string<ST_OUT_T_ARG>,
             typename CHAR_T = typename ST_OUT_T::value_type>

    static void AppendSubChar(ST_OUT_T& result)
    {
      if constexpr (std::is_same_v<CHAR_T, char>)
      {
        result.append(UTF8_SUBSTITUTE_CHARACTER);
      }
      else if constexpr (std::is_same_v<CHAR_T, wchar_t>)
      {
        result.append(WCHAR_T_SUBSTITUTE_CHARACTER);
      }
      else if constexpr (std::is_same_v<CHAR_T, char32_t>)
      {
        result.append(CHAR32_T_SUBSTITUTE_CHARACTER);
      }
      else
      {
        CLog::Log(LOGERROR, "{}: Unknown character type\n", __FUNCTION__);
      }
    }
  }; // CConversionUtils

  /*!
   * \brief Fetches the appropriate converter and returns the converted
   *        output of a given string
   *
   * \tparam       STR_IN_T_ARG  type of string-like converter input
   * \tparam       STR_OUT_T_ARG type of basic_string converter output
   * \tparam [OPT] STR_IN_T      STR_IN_T_ARG if it matched any std::basic_string_view
   * \tparam [OPT] STR_OUT_T     STR_OUT_T_ARG if it matched any std::basic_string
   * \tparam [OPT] IN_CHAR_T     The character type of any matching STR_IN_T_ARG
   * \tparam [OPT] OUT_CHAR_T    The character type of any matching STR_OUT_T_ARG
   *
   * \param converterType  the converter to use
   *
   * \param input          the string contents of type STR_IN_T to be converted
   *
   * \param failOnBadChar  if true then function will fail on invalid character,
   *                       otherwise substituteBadCodepoints will control behavior
   * \param substituteBadCodepoints If true, then when an invalid codepoint is encountered
   *                                it is replaced with a semi-official substitution
   *                                codepoint:  U"\x0fffd" which is displayed as: '�'.
   *                                Otherwise, when false, then the bad codepoints/codeunits
   *                                are skipped over.
   * \return converted STR_OUT_T string-type on successful conversion
   *         OR conversion with possible substitution codepoints when
   *         substituteBadCodepoints is true,
   *         OR empty string on any error when failOnBadChar is true
   */

  template<typename ST_IN_T_ARG,
           typename ST_OUT_T_ARG,
           typename ST_IN_T = enable_if_basic_string_view_t<ST_IN_T_ARG>,
           typename ST_OUT_T = enable_if_basic_string_t<ST_OUT_T_ARG>,
           typename IN_CHAR_T = typename ST_IN_T_ARG::value_type,
           typename OUT_CHAR_T = typename ST_OUT_T_ARG::value_type>

  static ST_OUT_T stdConvert(const StdConversionType convertType,
                             const ST_IN_T input,
                             const bool failOnInvalidChar = false,
                             const bool substituteBadCodepoints = true);

  /*!
   * \brief Performs the conversion and returns the converted result
   *
   * \tparam       STR_IN_T_ARG  type of string-like converter input
   * \tparam       STR_OUT_T_ARG type of basic_string converter output
   * \tparam [OPT] STR_IN_T      STR_IN_T_ARG if it matched any std::basic_string_view
   * \tparam [OPT] STR_OUT_T     STR_OUT_T_ARG if it matched any std::basic_string
   * \tparam [OPT] IN_CHAR_T     The character type of any matching STR_IN_T_ARG
   * \tparam [OPT] OUT_CHAR_T    The character type of any matching STR_OUT_T_ARG
   *
   *
   * \param converter     iconv converter to use
   *
   * \param multiplier    the maximum expansion ratio, in bytes, between codepoints
   *                      (unicode 'characters') in the input encoding and the
   *                      output encoding
   *
   * \param input         string contents of type ST_IN_T to be converted
   *
   *
   * \param failOnBadChar  if true then function will fail on invalid character,
   *                       otherwise substituteBadCodepoints will control behavior
   * \param substituteBadCodepoints If true, then when an invalid codepoint is encountered
   *                                it is replaced with a semi-official substitution
   *                                codepoint:  U"\x0fffd" which is displayed as: '�'.
   *                                Otherwise, when false, then the bad codepoints/codeunits
   *                                are skipped over.
   * \return converted STR_OUT_T string-type on successful conversion
   *         OR conversion with possible substitution codepoints when
   *         substituteBadCodepoints is true,
   *         OR empty string on any error when failOnBadChar is true
   */

  template<typename ST_IN_T_ARG,
           typename ST_OUT_T_ARG,
           typename ST_IN_T = enable_if_basic_string_view_t<ST_IN_T_ARG>,
           typename ST_OUT_T = enable_if_basic_string_t<ST_OUT_T_ARG>,
           typename IN_CHAR_T = typename ST_IN_T_ARG::value_type,
           typename OUT_CHAR_T = typename ST_OUT_T_ARG::value_type>

  static ST_OUT_T convert(const iconv_t converter,
                          const size_t multiplier,
                          const ST_IN_T input,
                          const bool failOnInvalidChar = false,
                          const bool substituteBadCodepoints = true);

  /*!
   * \brief Gets the specified iconv converter from thread_local collection
   *        of reusable converters
   *
   * \param converterType specifies the conversion to take place
   *
   * \return a reusable iconv converter
   */
  static CUnicodeConverterType& GetConverter(StdConversionType converterType);

  // Each thread has its own private set of converters. In CharSetConverter,
  // there is only one set of converters which are shared by locks. This is
  // unnecessary since conversions complete in one call.

}; // CUnicodeConverter::CInnerUCodeConverter

/* It can take between 1 and 4 UTF8 code-units (bytes) to represent a single
 * Unicode code-point (4 bytes). (For most purposes a code-point is a character/grapheme,
 * however there are some multi-codepoint graphemes; but this code does not support
 * them. However, emojis tend to be multi-codepoint graphemes and I have seen
 * some song titles which include them, so there is demand out there.)
 */

constexpr int CUnicodeConverter::m_Utf8CharMaxSize = 4;

// Define Converters.
//
// Limited testing suggests that creating a converter is fast,
// but perhaps the first access is expensive. It also appears that copies of
// converters are not that large. Therefore create a thread_local array
// of all of the Unicode converters that we may use. Making them thread_local
// means that we don't have to worry about locking, with the resulting blocking
// of code waiting for a converter that another thread is using.

/*!
 * \brief retrieves the specified iconv converter from our thread_local collection
 *
 * \param converterType specifies the characteristics of the converter
 *
 * \return an iconv converter
 */
thread_local std::vector<CUnicodeConverterType> converters;

CUnicodeConverterType& CUnicodeConverter::CInnerUCodeConverter::GetConverter(
    StdConversionType converterType)
{
  if (converters.size() == 0)
  {
    converters.emplace_back(CUnicodeConverterType{UTF8_SOURCE, UTF32_CHARSET});
    converters.emplace_back(
        CUnicodeConverterType{UTF32_CHARSET, "UTF-8", CUnicodeConverter::m_Utf8CharMaxSize});
    converters.emplace_back(CUnicodeConverterType{UTF32_CHARSET, WCHAR_CHARSET});
    converters.emplace_back(CUnicodeConverterType{WCHAR_CHARSET, UTF32_CHARSET});
    converters.emplace_back(
        CUnicodeConverterType{WCHAR_CHARSET, "UTF-8", CUnicodeConverter::m_Utf8CharMaxSize});
    converters.emplace_back(CUnicodeConverterType{UTF8_SOURCE, WCHAR_CHARSET});
  }
  CUnicodeConverterType& myConverter = converters[converterType];
  return myConverter;
}

template<typename ST_IN_T_ARG,
         typename ST_OUT_T_ARG,
         typename ST_IN_T, //= typename enable_if_basic_string_view_t<ST_IN_T_ARG>,
         typename ST_OUT_T, // = typename enable_if_basic_string_t<ST_OUT_T_ARG>,
         typename IN_CHAR_T, // = typename ST_IN_T_ARG::value_type,
         typename OUT_CHAR_T> //= typename ST_OUT_T_ARG::value_type>

ST_OUT_T CUnicodeConverter::CInnerUCodeConverter::stdConvert(
    const enum StdConversionType convertType,
    const ST_IN_T input,
    const bool failOnInvalidChar, /* = false */
    const bool substituteBadCodepoints /* = true */)
{
  if (input.empty())
    return ST_OUT_T{};

  if (convertType < 0 || convertType >= NumberOfStdConversionTypes)
    return ST_OUT_T{}; // Empty string on error

  CUnicodeConverterType& converter =
      CUnicodeConverter::CInnerUCodeConverter::GetConverter(convertType);

  return convert<ST_IN_T, ST_OUT_T>(converter.PrepareConverter(),
                                    converter.GetTargetSingleCharMaxLen(), input, failOnInvalidChar,
                                    substituteBadCodepoints);
}

struct CharPtrPtrAdapter
{
  /* Ensures the char buffers fed to iconv are cast to the correct
   * type for the platform.
   *
   * iconv may declare inbuf to be char** rather than const char** depending
   * on platform and version, so provide a wrapper that handles both.
   */
  const char** pointer;
  explicit CharPtrPtrAdapter(const char** p) : pointer(p) {}
  operator char* *() { return const_cast<char**>(pointer); }
  operator const char* *() { return pointer; }
};

template<typename ST_IN_T_ARG,
         typename ST_OUT_T_ARG,
         typename ST_IN_T, // enable_if_basic_string_view_t<ST_IN_T_ARG>,
         typename ST_OUT_T, // = typename enable_if_basic_string_t<ST_OUT_T_ARG>,
         typename IN_CHAR_T, // = typename ST_IN_T_ARG::value_type,
         typename OUT_CHAR_T> //= typename ST_OUT_T_ARG::value_type>

ST_OUT_T CUnicodeConverter::CInnerUCodeConverter::convert(
    const iconv_t type,
    const size_t multiplier,
    const ST_IN_T input,
    const bool failOnInvalidChar /* = false */,
    const bool substituteBadCodepoints /* = true */)
{
  ST_OUT_T output{};

  if (type == NO_ICONV)
    return output;

  // iconv works in bytes, but we work in code-units (sizeof(CharT)).

  const char* inBuf{CConversionUtils::BytePointer<ST_IN_T>(input)};
  const char* inCursor{inBuf};
  size_t inBytesRemaining{CConversionUtils::SizeInBytes<ST_IN_T>(input)};
  const size_t inBytesPerCU{sizeof(IN_CHAR_T)}; //typename ST_IN_T::value_type);

  // To reduce overhead, allocate a buffer from the stack. Malloc ensures
  // that everything it allocates is on a boundary that any type will be
  // properly aligned on, but we must do it manually here.

  // We want a byte-buffer, but, to ensure proper alignment for copying,
  // allocate the buffer as char32_t. Otherwise bytes get scrambled or
  // padded with zeros during the copy.

#ifdef TARGET_WINDOWS
  // Windows follows C++ array size rules: you must
  // specify a constant expression value.
  // clang++ & gcc++ are not so strict.
  //
  // So, for Windows, use an array large enough to handle 2K char32
  // iconv can handle doing its work in chunks. Avoiding malloc
  // for performance reasons. Even if the array is not large enough
  // iconv will automatically do the conversion in chunks. All we
  // have to do is empty the buffer, reset it back to empty and
  // continue.

  constexpr size_t WINDOWS_BUFFER_SIZE{2048 * 4};
  constexpr size_t outBytesPerCU{CConversionUtils::BytesPerCodeUnit<ST_OUT_T>()};
  const size_t outBufSizeInChar32{WINDOWS_BUFFER_SIZE / 4};
  char32_t char32Buffer[WINDOWS_BUFFER_SIZE]; // not a constant
#else
  // For non-Windows we can allocate whatever local array size that we like

  constexpr size_t outBytesPerCU{CConversionUtils::BytesPerCodeUnit<ST_OUT_T>()};

  const size_t tmpOutBufSizeInBytes{
      // Not a constant
      (input.length() + 1) * outBytesPerCU * multiplier + paddingBytes};

  // Calculate size after rounding up
  const size_t outBufSizeInChar32{(tmpOutBufSizeInBytes / sizeof(char32_t)) + 1};
  char32_t char32Buffer[outBufSizeInChar32]; // not a constant
#endif

  char* const outBuf{static_cast<char*>(static_cast<void*>(char32Buffer))};
  char* outCursor{outBuf};
  const size_t outBufByteSize{outBufSizeInChar32 * sizeof(char32_t)};
  size_t outBufAvail{outBufByteSize};

  size_t iconvRC{0};
  bool previousFailure{false};

  while (inBytesRemaining > 0) // Should complete in one pass if no bad chars
  {
    bool appendSubChar{false};

    //iconv() will update inCursor, inBytesRemaining, outCursor and outBytesAvail
    iconvRC =
        iconv(type, CharPtrPtrAdapter(&inCursor), &inBytesRemaining, &outCursor, &outBufAvail);

    // Handle any problems

    if (iconvRC == std::string::npos)
    {
      // Buffer not large enough to convert everything in one chunk. Not to worry.
      // Just empty buffer and continue

      if (errno == E2BIG) // Should only occur for Windows, since buffer size
      { // is constexpr
        if (TRACE_OUTPUT) // TODO: remove before merge. Helps with debugging on
        { // Jenkins build machines.
          std::cout << "iconv E2BIG input: " << StringUtils::ToHex(input) << std::endl;
          std::cout << "output: " << StringUtils::ToHex(output) << std::endl;
          std::cout << "inCursor: " << inCursor << std::endl;
        }
        iconvRC = 0; // Not a serious problem. Just empty buffer and continue
      }
      else if (errno == EILSEQ) // An invalid multibyte sequence has been
      { // encountered in the input. Note that iconv may not
        // precisely define which is the offending byte, but
        // it will be close. Also, we should acknowledge
        // that, say, a u32string output type, may cause
        // multiple bad characters identified, but this is
        // likely due to an error occurring in, say, a
        // u32string, where four bytes is the minimum
        // code unit size. If an error occurs, it is
        // impossible to identify which byte is responsible.
        if (TRACE_OUTPUT)
        {
          std::cout << "iconv EILSEO input: " << StringUtils::ToHex(input) << std::endl;
          std::cout << "output: " << StringUtils::ToHex(output) << std::endl;
          std::cout << "inCursor: " << inCursor << " previousFailure: " << previousFailure
                    << std::endl;
        }
        if (!failOnInvalidChar)
        {
          iconvRC = 0; // Don't fail on invalid char
          if (substituteBadCodepoints and
              !previousFailure) // Consider consecutive failures as one. Could skip to next start byte
          {
            // substitution occurs AFTER appending any good converted chars

            if (TRACE_OUTPUT)
              std::cout << "Append substitution codepoint" << std::endl;
            appendSubChar = true;
            previousFailure = true;
          }

          // Skip offending codeunit

          inCursor += inBytesPerCU;
          inBytesRemaining -= inBytesPerCU;
        }
        else // failed & failOnInvalidChar
        {
          inBytesRemaining = 0; // Bail out
          outBufAvail = outBufByteSize; // Nothing to copy
        }
      }
      else if (errno == EINVAL) // Invalid sequence at the end of input buffer. Either needs
      { // more input, or the end is malformed. In our case, can not be the former.
        // Insert substitute character, if chosen

        if (TRACE_OUTPUT)
        {
          std::cout << "iconv EINVAL input: " << StringUtils::ToHex(input) << std::endl;
          std::cout << "output: " << StringUtils::ToHex(output) << std::endl;
          std::cout << "inCursor: " << inCursor << " previousFailure: " << previousFailure
                    << std::endl;
        }
        if (!failOnInvalidChar)
        {
          iconvRC = 0; // Don't fail on invalid char

          if (substituteBadCodepoints and
              !previousFailure) // Consider consecutive failures as one. Could skip to next start byte
          {
            if (TRACE_OUTPUT)
              std::cout << "Append substitution codepoint" << std::endl;

            appendSubChar = true;
            previousFailure = true; // Won't matter, will exit loop
          }
        }
        inBytesRemaining = 0;
        outBufAvail = outBufByteSize; // Seems that on this error outBufAvail can be wrong
      }
      else // iconv() had some other error
      {
        if (TRACE_OUTPUT)
        {
          std::cout << "iconv OTHER error input: " << StringUtils::ToHex(input) << std::endl;
          std::cout << "output: " << StringUtils::ToHex(output) << std::endl;
          std::cout << "inCursor: " << inCursor << " previousFailure: " << previousFailure
                    << std::endl;
        }
        CLog::Log(LOGERROR, "{}: iconv() failed, errno={} ({})", __FUNCTION__, errno,
                  strerror(errno));
        inBytesRemaining = 0; // Force exit after saving what we can
        outBufAvail = outBufByteSize; // Make sure we don't write junk
      }
    } // Handled any error

    // Error processing complete, now move converted bytes to output string
    // Convert buffer to result code-units then append

    const size_t lengthInCU{(outBufByteSize - outBufAvail) / outBytesPerCU};
    if (lengthInCU > 0)
    {
      CConversionUtils::append<char*, ST_OUT_T>(outBuf, output, lengthInCU);
      previousFailure = false;
      if (TRACE_OUTPUT)
        std::cout << " Appending output: " << StringUtils::ToHex(output) << std::endl;
    }
    if (appendSubChar)
    {
      CConversionUtils::AppendSubChar<ST_OUT_T>(output);
      //CConversionUtils::AppendSubChar<ST_OUT_T>(output2);
      //if (output2.length() > 5 and ! STOP_TRACE)
      //{
      //  output2.clear();
      //  std::cout << "output2 works length: " << output2.length() <<std::endl;
      //  STOP_TRACE=true;
      //}
      if (TRACE_OUTPUT)
        std::cout << " Appended SUBSTITUTION char output: " << StringUtils::ToHex(output)
                  << std::endl;
    }

    // buffer empty again

    outCursor = outBuf;
    outBufAvail = outBufByteSize;
  } // while more to convert

  if (iconvRC == std::string::npos)
    output.clear(); // empty string on error

  return output;
}

//  Public Converters

std::u32string CUnicodeConverter::Utf8ToUtf32(
    const std::string_view utf8StringSrc,
    const bool failOnBadChar,
    /* = false */ const bool substituteBadCodepoints /* = true */)
{
  std::u32string result = CInnerUCodeConverter::stdConvert<std::string_view, std::u32string>(
      StdConversionType::Utf8ToUtf32, utf8StringSrc, failOnBadChar, substituteBadCodepoints);
  return result;
}

std::wstring CUnicodeConverter::Utf8ToW(const std::string_view stringSrc,
                                        const bool failOnBadChar /* = false */,
                                        const bool substituteBadCodepoints /* = true */)
{
  std::wstring result = CInnerUCodeConverter::stdConvert<std::string_view, std::wstring>(
      StdConversionType::Utf8ToW, stringSrc, failOnBadChar, substituteBadCodepoints);
  return result;
}

std::string CUnicodeConverter::Utf32ToUtf8(const std::u32string_view utf32StringSrc,
                                           const bool failOnBadChar /*= false*/,
                                           const bool substituteBadCodepoints /* = true */)
{
  std::string result = CInnerUCodeConverter::stdConvert<std::u32string_view, std::string>(
      StdConversionType::Utf32ToUtf8, utf32StringSrc, failOnBadChar, substituteBadCodepoints);
  return result;
}

std::wstring CUnicodeConverter::Utf32ToW(const std::u32string_view utf32StringSrc,
                                         const bool failOnBadChar /*= false*/,
                                         const bool substituteBadCodepoints /* = true */)
{
  // On systems where wchar_t is 4 bytes, then wstring === u32string. We could just
  // copy string over and exit.
  std::wstring result;
#ifdef WCHAR_IS_UCS_4 // Nothing to do, all UTF8 codepoints are UCS4 codepoints (but not viceversa)
  result.assign((const wchar_t*)utf32StringSrc.data(), utf32StringSrc.length());
#else // !WCHAR_IS_UCS_4
  result = CInnerUCodeConverter::stdConvert<std::u32string_view, std::wstring>(
      StdConversionType::Utf32ToW, utf32StringSrc, failOnBadChar, substituteBadCodepoints);
#endif // !WCHAR_IS_UCS_4
  return result;
}

std::u32string CUnicodeConverter::WToUtf32(const std::wstring_view wStringSrc,
                                           const bool failOnBadChar /*= false*/,
                                           const bool substituteBadCodepoints /* = true */)
{
#ifdef WCHAR_IS_UCS_4
  /* UCS-4 is almost equal to UTF-32, but UTF-32 has strict limits on possible values,
   * while UCS-4 is usually unchecked.
   * With this "conversion" we ensure that output will be valid UTF-32 string. */

  // Current info from unicode.org indicates that by 2011 UTF-32 became a
  // synonym for UCS-4. So a copy should be just fine. Measurements indicate
  // that no significant time will be saved doing this. Perhaps iconv does
  // the same thing in this situation.
#endif
  std::u32string result = CInnerUCodeConverter::stdConvert<std::wstring_view, std::u32string>(
      StdConversionType::WToUtf32, wStringSrc, failOnBadChar, substituteBadCodepoints);
  return result;
}

std::string CUnicodeConverter::WToUtf8(const std::wstring_view wStringSrc,
                                       const bool failOnBadChar /*= false */,
                                       const bool substituteBadCodepoints /* = true */)
{
  std::string result = CInnerUCodeConverter::stdConvert<std::wstring_view, std::string>(
      StdConversionType::WToUtf8, wStringSrc, failOnBadChar, substituteBadCodepoints);
  return result;
}
