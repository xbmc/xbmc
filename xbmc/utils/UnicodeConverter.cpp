/*
 *  Copyright (C) 2023 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

//
// This code is based upon CharacterSetConverter with significant differences:
//
// - Only supports Unicode conversions (but easily extended)
// - Is initialized before main starts.
// - string_view arguments used, reducing copies of dynamic memory
// - Converted string returned instead of pass by reference.
// - Option to replace bad-character conversions with Unicode Substitution
//   codepoint: U"\x0fffd"sv == '�'.
// - iconv converters in thread_local cache rather than using locks.
// - Use buffer allocated from stack instead of malloc.
// - On a sample of one Linux system, UnicodeConverter takes less than 1/2 the
//   time for CharSetConverter to process short strings. Long strings take about
//   95% of time.

#include "UnicodeConverter.h"

#include "filesystem/SpecialProtocol.h"
#include "utils/StringUtils.h"
#include "utils/Utf8Utils.h"
#include "utils/log.h"

#include <algorithm>
#include <bit>
#include <stdio.h>
#include <string>
#include <string_view>

#include <iconv.h>

using namespace std::literals;

//
// Unicode.org states that as of 2011 UTF-32 & UCS-4 are synonyms.
// Unicode.org states that UCS-2 does NOT support surrogate pairs while UTF-16
// does (ie. all Unicode codepoints). Other sources seem to think UCS-2 and UTF-16
// are the same.
//
// Windows documentation states that wchar_t is encoded as UTF-16LE.
// UTF-16 (without endian) defaults to big endian, but implementations may not
// exactly follow. When read, if a BOM (byte order mark) is present, then the correct
// encoding to used. When written to disk a BOM should be written, but that is beyond
// the scope here. BOMs should be used only when transmitting to an external source,
// they should not be used for in memory processing. It is assumed that BOMs are
// handled elsewhere.
//
// Another issue is normalization. For DARWIN (Mac O/S) the UTF-8-MAC converter is used
// when converting FROM UTF-8. Converting TO UTF-8 does NOT normalize (however, CharSetConverter
// does convert 'System' charset to UTF-8-MAC).
// UTF-8-MAC produces Unicode which is (mostly) in NFD (Normalization Form Canonical
// Decomposition). This normalization is needed by AFS to compare file paths.
// It is incorrect to process Unicode in this fashion, but it is consistent with
// prior versions of Kodi on OSX. The problem is 1) that some text is in NFD form
// while some is in NFC (that which escapes being converted to NFD). Further,
// different normalizations are useful for different tasks (sorting vs comparison, etc.).
// Unicode is normally in NFC normalization form.
//
// Fixing this would require proper normalization of strings
// requiring full Unicode processing found in libraries such as ICUC4.
// For now, leaving encoding as is.
//
// Note that to properly handle file paths on the Mac may require knowledge of what filesystem
// a file is from: AFS, NFS, NAS, SAMBA, NTFS, FAT32, etc.  Also this problem is not
// likely to be unique to the Mac platform.
//
// Android encodes paths as UTF-8, but there is some indication that underlying
// filesystem or media may limit to ASCII.
//
constexpr int UTF8_CHAR_MAX_SIZE = 4;

constexpr std::string_view STANDARD_UTF8_SOURCE{"UTF-8"};
constexpr std::string_view MAC_FILEPATH_CHARSET{"UTF-8-MAC"};

// CharsetConverter only used UTF-8-MAC when encoding from utf8, but not to utf8
// (except from 'System' charset, which is not done here, but left to CharsetConverter).

constexpr std::string_view UTF8_SINK = STANDARD_UTF8_SOURCE;

#if defined(WORDS_BIGENDIAN)
constexpr std::string_view UTF16_CHARSET{"UTF-16BE"};
constexpr std::string_view UTF32_CHARSET{"UCS-4BE"};
#else
constexpr std::string_view UTF16_CHARSET{"UTF-16LE"};
constexpr std::string_view UTF32_CHARSET{"UCS-4LE"};
#endif

#if defined(TARGET_DARWIN)
constexpr bool IS_TARGET_DARWIN{true};
#else
constexpr bool IS_TARGET_DARWIN{false};
#endif

constexpr std::string_view UTF8_SOURCE = []() {
  if constexpr (IS_TARGET_DARWIN)
  {
    return MAC_FILEPATH_CHARSET; // UTF-8 + NFD normalization
  }
  return STANDARD_UTF8_SOURCE;
}();

constexpr std::string_view WCHAR_CHARSET = []() {
  if constexpr (sizeof(wchar_t) == 2)
    return UTF16_CHARSET;

  return UTF32_CHARSET;
}();

#define NO_ICONV ((iconv_t)-1)

// Replace bad codepoints with the semi-offical Unicode 'substitution' character
// 0xfffd. This is commonly used by browsers, etc. This character displays as �

static constexpr std::string_view UTF8_SUBSTITUTE_CHARACTER{"\xef\xbf\xbd"};
static constexpr std::wstring_view WCHAR_T_SUBSTITUTE_CHARACTER{L"\xfffd"};
static constexpr std::u32string_view CHAR32_T_SUBSTITUTE_CHARACTER{U"\x0000fffd"};

// Define size of stack buffer for iconv. Large enough for 512 32-bit unicode chars
// or 2048 UTF-8 codeunits

constexpr size_t BUFFER_SIZE{512 * sizeof(char32_t)};

class CUnicodeConverterType
{
  // Simple class to keep the necessary information to create an iconv instance

public:
  CUnicodeConverterType(std::string_view sourceCharset,
                        std::string_view targetCharset,
                        size_t targetSingleCharMaxLen = 1);
  ~CUnicodeConverterType();

  iconv_t PrepareConverter();
  CUnicodeConverterType& operator=(const CUnicodeConverterType& other) = delete;

  std::string GetSourceCharset() const { return m_sourceCharset; }
  std::string GetTargetCharset() const { return m_targetCharset; }
  size_t GetTargetSingleCharMaxLen() const { return m_targetSingleCharMaxLen; }

private:
  std::string m_sourceCharset;
  std::string m_targetCharset;
  iconv_t m_iconv;
  size_t m_targetSingleCharMaxLen;
}; // CUnicodeConverterType

CUnicodeConverterType::CUnicodeConverterType(std::string_view sourceCharset,
                                             std::string_view targetCharset,
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
      CLog::LogF(LOGERROR, "iconv_open() for \"{}\" -> \"{}\" failed, errno = {} ({})",
                 m_sourceCharset, m_targetCharset, errno, strerror(errno));
  }
  return m_iconv;
}

enum class StdConversionType : int
{
  NoConversion = -1,
  Utf8ToUtf32 = 0,
  Utf32ToUtf8,
  Utf32ToW,
  WToUtf32,
  WToUtf8,
  Utf8ToW,
  NumberOfStdConversionTypes /* Dummy sentinel entry */
}; // StdConversionType

namespace
{

// Initialize Converters.
//
// Create a thread_local vector of all of the Unicode converters that we use.
// Making them thread_local means that we don't have to worry about locking,
// since all activity occurs on one thread and one call.

thread_local std::vector<CUnicodeConverterType> converters{
    // See comments about DARWIN at the top of this file for the use of UTF8_SOURCE
    // and UTF8_SINK.
    CUnicodeConverterType{UTF8_SOURCE, UTF32_CHARSET},
    CUnicodeConverterType{UTF32_CHARSET, UTF8_SINK, UTF8_CHAR_MAX_SIZE},
    CUnicodeConverterType{UTF32_CHARSET, WCHAR_CHARSET},
    CUnicodeConverterType{WCHAR_CHARSET, UTF32_CHARSET},
    CUnicodeConverterType{WCHAR_CHARSET, UTF8_SINK, UTF8_CHAR_MAX_SIZE},
    CUnicodeConverterType{UTF8_SOURCE, WCHAR_CHARSET}};

/*!
 * \brief retrieves the specified iconv converter from our thread_local collection
 * \param converterType specifies the characteristics of the converter
 * \return an iconv converter
 */
static CUnicodeConverterType& GetConverter(StdConversionType converterType)
{
  CUnicodeConverterType& myConverter = converters[static_cast<int>(converterType)];
  return myConverter;
}

/*!
 * \brief Verifies that the given type is for one of the supported character
 *        types: string, wstring or u32string
 * \tparam CHAR_T_ARG  a character type, such as "char", "wchar_t" or "char32_t"
 * \tparam T  the type of the evaluated template on successful match.
 *            Default is CHAR_T_ARG
 */
template<typename CHAR_T_ARG, typename T = CHAR_T_ARG>
using enable_if_char_type =
    std::enable_if<std::is_base_of_v<char, CHAR_T_ARG> || std::is_base_of_v<wchar_t, CHAR_T_ARG> ||
                       std::is_base_of_v<char32_t, CHAR_T_ARG>,
                   T>;

/*!
 * \brief Verifies that the given argument is one of the supported
 *        basic_string types
 * \tparam STR_T must be one of string, wstring, u32string
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
 *        the appropriate character type of the string, or create an
 *        object with appropriate character type
 * \tparam STR_T must be an approved string type
 * \tparam T The type of the evaluated template. Default STR_T
 */
template<typename STR_T, typename T = STR_T>
using enable_if_basic_string_t = typename enable_if_basic_string<STR_T, T>::type;

/*!
 * \brief Verifies that the given argument is one of the supported
 *        string_view types
 * \tparam STR_T must be one of string_view, wstring_view, u32string_view
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
 * \tparam STR_T must be one of string_view, wstring_view or u32string_view
 * \tparam T the type of the evaluated template if a match is found, the default is STR_T
 */
template<typename STR_T, typename T = STR_T>
using enable_if_basic_string_view_t = typename enable_if_basic_string_view<STR_T, T>::type;

/*!
 * \brief Ensures that an argument is an approved string or string_view type.
 * \tparam STR_T must be one of: string, wstring, u32string, string_view,
 *         wstring_view or u32string_view
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
 * \tparam STR_T must be one of: string, wstring, u32string, string_view,
 *         wstring_view or u32string_view
 * \tparam T the Type of the evaluated template. Default STR_T
 */
template<typename STR_T, typename T = STR_T>
using enable_if_any_string_type_t = typename enable_if_any_string_type<STR_T, T>::type;

/**
 * Define a number of utility functions.
 * iconv works in bytes, but we work in code-units (sizeof(CharT)).
 */

/*!
 * \brief extract the number of bytes per code-unit for the char-type
 *        from the given template.
 * \tparam STR_T_ARG Type of string-like argument to get the code-unit size of.
 *               Must be one of: string, wstring, u32string, string_view,
 *               wstring_view or u32string_view
 * \tparam [OPT] STR_T automatically set to the type field of the
 *                    matching STR_T_ARG string type
 * \tparam [OPT] CHAR_T automatically set to the string type of
 *                      the matching STR_T_ARG
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
 * \tparam STR_T_ARG Type of string-like argument to get the byte length of.
 *               Must be one of: string, wstring, u32string, string_view,
 *               wstring_view or u32string_view
 * \tparam [OPT] STR_T automatically set to the type field of the
 *                    matching STR_T_ARG string type
 * \tparam [OPT] CHAR_T automatically set to the string type of
 *                      the matching STR_T_ARG
 * \param  str    string in CHAR_T to get length of
 * \return size of str, in bytes
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
 * \brief Appends the converted bytes from a char buffer to the output basic_string
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
  const CHAR_T* codeUnitPointer = reinterpret_cast<const CHAR_T*>(buffer);
  outputString.append(codeUnitPointer, lengthInCU);
}

/*!
 * \brief Appends a substitution codepoint for a bad character to the output basic_string
 * \tparam STR_OUT_T_ARG    type of the basic_string to copy the substitution
 *                          codepoint to
 * \tparam STR_OUT_T        set to STR_OUT_T_ARG if enable_if_basic_string finds a match
 * \tparam CHAR_T           set to the character type of STR_OUT_T_ARG
 * \param result            the contents of STR_OUT_T_ARG type basic_string to copy to
 */
template<typename STR_OUT_T_ARG,
         typename STR_OUT_T = enable_if_basic_string<STR_OUT_T_ARG>,
         typename CHAR_T = typename STR_OUT_T::value_type>

static void AppendSubChar(STR_OUT_T& result)
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
  // Sanity check. If it compiles, then it shows that
  // we are handling every char type that can pass emable_if_basic_string
  else if constexpr (std::is_same<STR_OUT_T, STR_OUT_T_ARG>::value)
  {
    static_assert("STR_OUT_T_ART is Invalid type");
  }
}

/*!
 * \brief Fetches the appropriate converter and returns the converted
 *        given string
 * \tparam       STR_IN_T_ARG  type of string-like converter input
 * \tparam       STR_OUT_T_ARG type of basic_string converter output
 * \tparam [OPT] STR_IN_T      STR_IN_T_ARG if it matched any std::basic_string_view
 * \tparam [OPT] STR_OUT_T     STR_OUT_T_ARG if it matched any std::basic_string
 * \tparam [OPT] IN_CHAR_T     The character type of any matching STR_IN_T_ARG
 * \tparam [OPT] OUT_CHAR_T    The character type of any matching STR_OUT_T_ARG
 * \param converterType  the converter to use
 * \param input          the string contents of type STR_IN_T to be converted
 * \param failOnInvalidChar  if true then function will fail on invalid character,
 *                       otherwise substituteBadCodepoints will control behavior
 * \param substituteBadCodepoints If true, then when an invalid codepoint is encountered
 *                                it is replaced with a semi-official substitution
 *                                codepoint:  U"\x0fffd" which is displayed as: '�'.
 *                                Otherwise, when false, then the bad codepoints/codeunits
 *                                are skipped over.
 * \return converted STR_OUT_T basic_string on successful conversion
 *         OR conversion with possible substitution codepoints when
 *         substituteBadCodepoints is true,
 *         OR empty string on any error when failOnInvalidChar is true
 */

template<typename STR_IN_T_ARG,
         typename STR_OUT_T_ARG,
         typename STR_IN_T = enable_if_basic_string_view_t<STR_IN_T_ARG>,
         typename STR_OUT_T = enable_if_basic_string_t<STR_OUT_T_ARG>,
         typename IN_CHAR_T = typename STR_IN_T_ARG::value_type,
         typename OUT_CHAR_T = typename STR_OUT_T_ARG::value_type>

static STR_OUT_T stdConvert(StdConversionType convertType,
                            STR_IN_T input,
                            bool failOnInvalidChar = false,
                            bool substituteBadCodepoints = true);

/*!
 * \brief Performs the conversion and returns the converted result
 *
 * \tparam       STR_IN_T_ARG  type of string-like converter input
 * \tparam       STR_OUT_T_ARG type of basic_string converter output
 * \tparam [OPT] STR_IN_T      STR_IN_T_ARG if it matched any std::basic_string_view
 * \tparam [OPT] STR_OUT_T     STR_OUT_T_ARG if it matched any std::basic_string
 * \tparam [OPT] IN_CHAR_T     The character type of any matching STR_IN_T_ARG
 * \tparam [OPT] OUT_CHAR_T    The character type of any matching STR_OUT_T_ARG
 * \param converter     iconv converter to use
 * \param multiplier    the maximum expansion ratio, in bytes, between codepoints
 *                      (unicode 'characters') in the input encoding and the
 *                      output encoding
 * \param input         string contents of type STR_IN_T to be converted
 * \param failOnInvalidChar  if true then function will fail on invalid character,
 *                           otherwise substituteBadCodepoints will control behavior
 * \param substituteBadCodepoints If true, then when an invalid codepoint is encountered
 *                                it is replaced with a semi-official substitution
 *                                codepoint:  U"\x0fffd" which is displayed as: '�'.
 *                                Otherwise, when false, then the bad codepoints/codeunits
 *                                are skipped over.
 * \return converted STR_OUT_T string-type on successful conversion
 *         OR conversion with possible substitution codepoints when
 *         substituteBadCodepoints is true,
 *         OR empty string on any error when failOnInvalidChar is true
 */

template<typename STR_IN_T_ARG,
         typename STR_OUT_T_ARG,
         typename STR_IN_T = enable_if_basic_string_view_t<STR_IN_T_ARG>,
         typename STR_OUT_T = enable_if_basic_string_t<STR_OUT_T_ARG>,
         typename IN_CHAR_T = typename STR_IN_T_ARG::value_type,
         typename OUT_CHAR_T = typename STR_OUT_T_ARG::value_type>

static STR_OUT_T convert(const iconv_t converter,
                         size_t multiplier,
                         STR_IN_T input,
                         bool failOnInvalidChar = false,
                         bool substituteBadCodepoints = true);

template<typename STR_IN_T_ARG,
         typename STR_OUT_T_ARG,
         typename STR_IN_T, //= typename enable_if_basic_string_view_t<STR_IN_T_ARG>,
         typename STR_OUT_T, // = typename enable_if_basic_string_t<STR_OUT_T_ARG>,
         typename IN_CHAR_T, // = typename STR_IN_T_ARG::value_type,
         typename OUT_CHAR_T> //= typename STR_OUT_T_ARG::value_type>

STR_OUT_T stdConvert(StdConversionType convertType,
                     STR_IN_T input,
                     bool failOnInvalidChar, /* = false */
                     bool substituteBadCodepoints /* = true */)
{
  if (input.empty())
    return STR_OUT_T{};

  if (convertType == StdConversionType::NoConversion)
    return STR_OUT_T{}; // Empty string on error

  CUnicodeConverterType& converter = GetConverter(convertType);
  return convert<STR_IN_T, STR_OUT_T>(converter.PrepareConverter(),
                                      converter.GetTargetSingleCharMaxLen(), input,
                                      failOnInvalidChar, substituteBadCodepoints);
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

template<typename STR_IN_T_ARG,
         typename STR_OUT_T_ARG,
         typename STR_IN_T, // enable_if_basic_string_view_t<STR_IN_T_ARG>,
         typename STR_OUT_T, // = typename enable_if_basic_string_t<STR_OUT_T_ARG>,
         typename IN_CHAR_T, // = typename STR_IN_T_ARG::value_type,
         typename OUT_CHAR_T> //= typename STR_OUT_T_ARG::value_type>

static STR_OUT_T convert(const iconv_t type,
                         size_t multiplier,
                         STR_IN_T input,
                         bool failOnInvalidChar /* = false */,
                         bool substituteBadCodepoints /* = true */)
{

  if (type == NO_ICONV)
    return STR_OUT_T();

  // iconv works in bytes, but we work in code-units (sizeof(CharT)).

  const char* inBuf{reinterpret_cast<const char*>(input.data())};
  const char* inCursor{inBuf};
  size_t inBytesRemaining{SizeInBytes<STR_IN_T>(input)};
  constexpr size_t inBytesPerCU{BytesPerCodeUnit<STR_IN_T>()};
  constexpr size_t outBytesPerCU{BytesPerCodeUnit<STR_OUT_T>()};

  // ERROR handling:
  //
  // libiconv and gnu libc's iconv have some implementation differences.
  // Further, POSIX defines iconv command to handle errors differently
  // than iconv function. There are some ways around at least some of
  // this, but the real solution is to use ICU. Since handling of
  // of malformed chars is a 'nice to have' feature here, we just do the best
  // we can without going crazy. Of course protecting against walking
  // over memory is paramount.

  // To reduce overhead, allocate a buffer from the stack. To ensure proper
  // alignment for copying, allocate the buffer as char32_t. Otherwise bytes
  // get scrambled or padded with zeros during the copy.

  // Even if the array is not large enough iconv will automatically do
  // the conversion in chunks. All we have to do is empty the buffer, reset
  // it back to empty and continue.

  char32_t char32Buffer[BUFFER_SIZE];

  // iconv wants a byte buffer

  char* const outBuf{reinterpret_cast<char*>(char32Buffer)};
  char* outCursor{outBuf};
  const size_t outBufByteSize{BUFFER_SIZE * sizeof(char32_t)};
  size_t outBufAvail{outBufByteSize};

  size_t iconvRC{0};
  bool previousSubstitution{false};

  // Allocate the output string now. Later will expand, if needed.
  // Since basic_string most likely does one or two mallocs, depending
  // on how long the string is. Assuming that the initial empty string
  // is one malloc, then if the string turns out to be longer than what
  // can fit, a second malloc will be done as needed. Therefore it should
  // not cost more to increase size later.

  STR_OUT_T output;

  // Reset iconv
  iconv(type, NULL, NULL, NULL, NULL);

  // Note that iconv on some platforms may convert bad char sequences to the Substitute
  // character without our knowledge. Further, some (OSX) will not recognize some
  // characters as malformed and may convert them to bad UTF-8 without notice, etc.
  // We have no control over this and no worse than current behavior.

  while (inBytesRemaining > 0) // Buffer is large enough to normally require one pass
  {
    bool appendSubChar{false};
    iconvRC =
        iconv(type, CharPtrPtrAdapter(&inCursor), &inBytesRemaining, &outCursor, &outBufAvail);

    // Handle any problems

    if (iconvRC == std::string::npos)
    {
      if (errno == E2BIG) // Output buffer full
      {
        iconvRC = 0; // Not a real problem. Just empty buffer and continue
      }
      else if (errno == EILSEQ)
      {
        // An invalid multibyte sequence has been encountered in the input. inCursor
        // points at start of illegal sequence. We don't know how long the sequence
        // is.

        if (failOnInvalidChar) // Return with empty string
        {
          inBytesRemaining = 0; // Bail out
          outBufAvail = outBufByteSize; // Mark outbuf empty
          outCursor = outBuf;
        }
        else
        {
          iconvRC = 0; // Don't fail on invalid char
          if (substituteBadCodepoints &&
              !previousSubstitution) // Consider consecutive failures as one
          {
            // substitution occurs AFTER appending any good converted chars

            appendSubChar = true;
            previousSubstitution = true; // consecutive bad chars treated as one
          }

          // Skip offending codeunit. Would be best to advance to start of next
          // character, but keeping simple.

          inCursor += inBytesPerCU;
          if (inBytesRemaining >= inBytesPerCU)
            inBytesRemaining -= inBytesPerCU;
          else
            inBytesRemaining = 0;
        }
      }
      else if (errno == EINVAL)
      {
        // Invalid sequence at the end of input buffer. Either needs
        // more input, or the end is malformed. In our case, can not
        // be the former.
        //
        // if failOnInvalidChar: Exit with no output
        // else output any converted bytes in output, then, if enabled,
        // append substitute character, then, exit.

        if (failOnInvalidChar)
        {
          inBytesRemaining = 0; // Bail out
          outBufAvail = outBufByteSize; // Mark outbuf empty
          outCursor = outBuf;
        }
        else
        {
          iconvRC = 0; // Don't fail on invalid char
          if (substituteBadCodepoints and
              !previousSubstitution) // Consider consecutive failures as one.
          {
            appendSubChar = true;
            previousSubstitution = true; // Won't matter, will exit loop
          }
          inBytesRemaining = 0; // Don't attempt to process more input
        }
      }
      else // iconv() had some other error
      {
        CLog::LogF(LOGERROR, "iconv() failed, errno={} ({})", errno, strerror(errno));

        inBytesRemaining = 0; // Bail out
        outBufAvail = outBufByteSize; // Mark outbuf empty
        outCursor = outBuf;
      }
    } // Handled any error

    // Error processing complete, now move converted bytes to output string.
    // Convert buffer to result code-units then append any substitute chars
    //
    // If we are done converting, let append do the resize, otherwise try to
    // reduce mallocs by resizing to estimated final size before append.

    const size_t lengthInCU{(outBufByteSize - outBufAvail) / outBytesPerCU};
    if (inBytesRemaining > 0)
    {
      size_t expectedOutputRemaining = (inBytesRemaining / inBytesPerCU) + 1;
      if constexpr (IS_TARGET_DARWIN && (outBytesPerCU == 1))
      {
        // MacOS, as mentioned near the start of this module, uses UTF-8-MAC instead
        // of UTF-8 which normalizes as well as converts the string. This normalization
        // results in string expansion for some graphemes (characters).
        //
        // For lack of a better guide, increase estimate by 20%

        expectedOutputRemaining += (expectedOutputRemaining + 19) / 20;
      }
      size_t expectedOutputLength = output.size() + expectedOutputRemaining;
      output.reserve(expectedOutputLength);
    }

    if (lengthInCU > 0) // Write anything in output buffer
    {
      // Converts bytes to code-units
      append<char*, STR_OUT_T>(outBuf, output, lengthInCU);
      previousSubstitution = false;
    }
    if (appendSubChar)
    {
      AppendSubChar<STR_OUT_T>(output);
      previousSubstitution = true;
    }
    // buffer empty again
    outCursor = outBuf;
    outBufAvail = outBufByteSize;
  } // while more to convert

  if (iconvRC == std::string::npos)
    output.clear(); // empty string on error

  return output;
}

} // namespace
//  Public Converters

std::u32string CUnicodeConverter::Utf8ToUtf32(std::string_view utf8StringSrc,
                                              bool failOnInvalidChar /* = false */,
                                              bool substituteBadCodepoints /* = true */)
{
  std::u32string result = stdConvert<std::string_view, std::u32string>(
      StdConversionType::Utf8ToUtf32, utf8StringSrc, failOnInvalidChar, substituteBadCodepoints);
  return result;
}

std::wstring CUnicodeConverter::Utf8ToW(std::string_view stringSrc,
                                        bool failOnInvalidChar /* = false */,
                                        bool substituteBadCodepoints /* = true */)
{
  std::wstring result = stdConvert<std::string_view, std::wstring>(
      StdConversionType::Utf8ToW, stringSrc, failOnInvalidChar, substituteBadCodepoints);
  return result;
}

std::string CUnicodeConverter::Utf32ToUtf8(std::u32string_view utf32StringSrc,
                                           bool failOnInvalidChar /*= false*/,
                                           bool substituteBadCodepoints /* = true */)
{
  std::string result = stdConvert<std::u32string_view, std::string>(
      StdConversionType::Utf32ToUtf8, utf32StringSrc, failOnInvalidChar, substituteBadCodepoints);
  return result;
}

std::wstring CUnicodeConverter::Utf32ToW(std::u32string_view utf32StringSrc,
                                         bool failOnInvalidChar /*= false*/,
                                         bool substituteBadCodepoints /* = true */)
{
  // On systems where wchar_t is 4 bytes, then wstring == u32string. Just
  // copy string and exit. NOTE that this will mean that any malformed chars will not be
  // processed. This should be acceptable, since malformed chars should be rare,
  // especially since other encoders/decoders have already had a chance at it.

  std::wstring result;
  if constexpr (sizeof(wchar_t) == 4) // UCS4 and UTF32 are the same
  {
    // By explicitly setting length in returned string, it does not matter if
    // string_view.data() is null terminated

    result.assign(reinterpret_cast<const wchar_t*>(utf32StringSrc.data()), utf32StringSrc.length());
  }
  else
  {
    result = stdConvert<std::u32string_view, std::wstring>(
        StdConversionType::Utf32ToW, utf32StringSrc, failOnInvalidChar, substituteBadCodepoints);
  }
  return result;
}

std::u32string CUnicodeConverter::WToUtf32(std::wstring_view wStringSrc,
                                           bool failOnInvalidChar /*= false*/,
                                           bool substituteBadCodepoints /* = true */)
{
  if constexpr (sizeof(wchar_t) == 4) // UCS4 and UTF32 are the same
  {
    std::u32string result;

    // Current info from unicode.org indicates that by 2011 UTF-32 became a
    // synonym for UCS-4. So a copy should be just fine.
    //
    // By explicitly setting length in returned string, it does not matter if
    // string_view.data() is null terminated.
    //
    // Note that detection of malformed chars will NOT occur.

    result.assign(reinterpret_cast<const char32_t*>(wStringSrc.data()), wStringSrc.length());
    return result;
  }
  else
  {
    std::u32string result = stdConvert<std::wstring_view, std::u32string>(
        StdConversionType::WToUtf32, wStringSrc, failOnInvalidChar, substituteBadCodepoints);
    return result;
  }
}

std::string CUnicodeConverter::WToUtf8(std::wstring_view wStringSrc,
                                       bool failOnInvalidChar /*= false */,
                                       bool substituteBadCodepoints /* = true */)
{
  std::string result = stdConvert<std::wstring_view, std::string>(
      StdConversionType::WToUtf8, wStringSrc, failOnInvalidChar, substituteBadCodepoints);
  return result;
}
