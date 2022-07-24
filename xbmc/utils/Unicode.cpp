#include "Unicode.h"

#include "../LangInfo.h"
#include "../commons/ilog.h"
#include "../utils/log.h"
#include "Locale.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <codecvt>
#include <locale>
#include <stdlib.h>
#include <string>
#include <string_view>

#include <unicode/bytestream.h>
#include <unicode/casemap.h>
#include <unicode/coll.h>
#include <unicode/normalizer2.h>
#include <unicode/regex.h>
#include <unicode/schriter.h>
#include <unicode/stringoptions.h>
#include <unicode/ucol.h>
#include <unicode/unistr.h>
#include <unicode/urename.h>
#include <unicode/ustring.h>
#include <unicode/utext.h>


/*
 * Keep in mind that this implementation is the first draft. Focus is on features and
 * correct behavior. Performance will be worked in a bit at a time.  The ultimate
 * goal is to utilize UnicodeStrings, or similar, instead of wstring and string.
 *
 * At the very least, change string/wstring to u16string so that the data
 * conversion to/from UChars is much less expensive.
 *
 */


/*
 * A note about UnicodeStrings memory
 *
 * UnicodeStrings keep strings over ~27 UTF-16 code units in heap. Otherwise
 * they are kept on the stack as part of the UnicodeString instance. The
 * the capacity of code units that can be stored in the instance can
 * be changed by compiling ICU with -DUNISTR_OBJECT_SIZE. The default
 * is 64. See more info in unistr.h
 *
 *   UnicodeString(UBool isTerminated, ConstChar16Ptr text, int32_t textLength);
 *
 * Creates an instance which uses text as a read-only buffer. A copy
 * will be made if a write is required to it. UnicodeString will not release
 * the memory.
 *
 *   UnicodeString(char16_t *buffer, int32_t buffLength, int32_t buffCapacity);
 *
 * Creates an instance using buffer as a read-write buffer. A copy will be
 * made if the capacity is too small for any change. Also any clones will
 * be copied. UnicodeString will not release the memory
 */

/*
 * Using C++ built in conversions with the assumption that it is easier
 * and faster. However, it is possible that the system does not have the
 * same language support as the ICU library. Perhaps one normalizes
 * and the other does not, so perhaps it is a mistake.
 *
 * Fortunately it would not be difficult to rectify.
 */


static const int BUFFER_PAD = 0; // 200;

std::u16string Unicode::UTF8ToUTF16(const std::string_view& str)
{
  // Note: Faster, but does not do Unicode validation and
  // replacing bad Unicode with substitute Unicode to mark it
  // as malformed. Besides reporting the problem, the substitute
  // code units prevent further breakage.
  //
  // Going in the other direction, from UChar, any damage or
  // substitutions have likely already been addressed, so unless
  // experience dictates otherwise, use the faster, UTF16-> xx
  // versions instead of the ones that can replace bad unicode
  // with substitute.

  std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> conv;
  std::u16string result = conv.from_bytes(&str[0], &str[0] + str.length());
  return result;
}

std::u32string Unicode::UTF8ToUTF32(const std::string_view &str)
{
  std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t> conv;
  return conv.from_bytes(str.data(), str.data() + str.length());
}

std::wstring Unicode::UTF8ToWString(const std::string_view& str)
{
  std::wstring_convert<std::codecvt_utf8<wchar_t>> conv;
  std::wstring result = conv.from_bytes(str.data(), str.data() + str.length());
  return result;
}

std::string Unicode::UTF16ToUTF8(const std::u16string_view& str)
{
  std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> conv;
  std::string result = conv.to_bytes(str.data(), str.data() + str.length());
  return result;
}

std::u32string Unicode::UTF16ToUTF32(const std::u16string_view& str)
{
  std::wstring_convert<std::codecvt_utf16<char32_t>, char32_t> conv;
  std::u32string result = conv.from_bytes(reinterpret_cast<const char*>(str.data()),
      reinterpret_cast<const char*>(str.data() + str.length()));
  return result;
}

std::wstring Unicode::UTF16ToWString(const std::u16string_view &str)
{
  std::wstring_convert<std::codecvt_utf16<wchar_t, 0x10ffff, std::little_endian>, wchar_t> conv;

  std::wstring wstr = conv.from_bytes(reinterpret_cast<const char*>(str.data()),
      reinterpret_cast<const char*>(str.data() + str.size()));
  return wstr;
}

std::string Unicode::UTF32ToUTF8(const std::u32string_view &str)
{
  std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t> conv;
  return conv.to_bytes(str.data(), str.data() + str.length());
}

std::u16string Unicode::UTF32ToUTF16(const std::u32string_view &str)
{
  std::wstring_convert<std::codecvt_utf16<char32_t>, char32_t> conv;
  std::string bytes = conv.to_bytes(str.data(), str.data() + str.length());
  return std::u16string(reinterpret_cast<const char16_t*>(bytes.c_str()),
      bytes.length() / sizeof(char16_t));
}

std::string Unicode::WStringToUTF8(const std::wstring_view& str)
{
  std::wstring_convert<std::codecvt_utf8<wchar_t>> conv;
  std::string result = conv.to_bytes(str.data(), str.data() + str.length());
  return result;
}

inline std::string ToString(icu::UnicodeString& str)
{
  std::string tempStr = std::string();
  tempStr = str.toUTF8String(tempStr);
  return tempStr;
}

inline std::wstring ToWstring(icu::UnicodeString& ustr)
{
  // Could use UnicodeString.toUTF32, but is a bit messier. But it
  // may be better to consistently use same conversion routines.
  // Also have to worry about systems where sizeof(wchar_t) is 2

  std::u16string_view buffer = std::u16string_view(ustr.getBuffer(), ustr.length());
  std::wstring result = Unicode::UTF16ToWString(buffer);
  return result;
}

/**
 * Calculates a 'reasonable' buffer size to give some room for growth in a utf-8
 * string to accommodate basic transformations (folding, normalization, etc.).
 *
 * param utf8Length byte-length of UTF-8 string to be converted
 * param scale multiplier to apply to get larger buffer
 *
 * Note that the returned size has a pad of BUFFER_PAD bytes added to leave room
 * for growth.
 */

size_t Unicode::GetUTF8WorkingSize(const size_t utf8Length, float scale /* = 1.5 */)
{

  float scaleFactor = std::fmax((float)scale, 1.5);
  return (size_t)(BUFFER_PAD + int(utf8Length * scaleFactor));
}


/**
 * Calculates the maximum number of UChars (UTF-16) required by a UTF-8
 * string.
 *
 * param utf8_length byte-length of UTF-8 string to be converted
 * param scale multiplier to apply to get larger buffer
 *
 * Note that the returned size has a pad of BUFFER_PAD UChars added to leave room
 * for growth.
 *
 * Note that a UTF-16 string will be at most as long as the UTF-8 string.
 */

size_t Unicode::GetUTF8ToUTF16BufferSize(const size_t utf8_length, const float scale /* = 1.0*/)
{
  // utf8 string will comfortably fit in a UChar string
  // of same char length. Add a bit more for some growth.

  int scale_factor = std::fmax((int)scale, 1.0);
  return (size_t)(BUFFER_PAD + (int)(utf8_length * scale_factor));
}

/*!
 * \brief Calculates the maximum number of UChars (UTF-16) required by a wchar_t
 * string.
 *
 * \param wchar_length Char32 codepoints to be converted to UTF-16.
 * \param scale multiplier to apply to get larger buffer
 * \return A size a bit larger than wchar_length, plus BUFFER_PAD.
 */

size_t Unicode::GetWcharToUTF16BufferSize(const size_t wchar_length, const size_t scale /* = 1*/)
{

  // Most UTF-32 strings will be BMP-only and result in a same-length
  // UTF-16 string. We overestimate the capacity just slightly,
  // just in case there are a few supplementary characters.
  // Add a bit more for some growth.
  int scale_factor = std::max((int)scale, 1);
  return (size_t)(BUFFER_PAD + (wchar_length + (wchar_length >> 4) + 4) * scale_factor);
}

size_t Unicode::GetUTF16ToUTF32BufferSize(const size_t uchar_length, const size_t scale /* = 1*/)
{

  // Most UTF-32 strings will be BMP-only and result in a same-length
  // UTF-16 string. We overestimate the capacity just slightly,
  // just in case there are a few supplementary characters.
  // Add a bit more for some growth.
  int scale_factor = std::max((int)scale, 1);
  return (size_t)(BUFFER_PAD + (uchar_length + (uchar_length >> 4) + 4) * scale_factor);
}

/*!
 * \brief Calculates the maximum number of wchar_t code-units (UTF-32)
 * required by a UTF-16 string.
 *
 * \param wchar_length Uchar-16 code-units to be converted to UTF-32.
 * \param scale multiplier to apply to get larger buffer
 * \return A size a bit larger than wchar_length, plus BUFFER_PAD.
 */

size_t Unicode::GetUTF16ToWcharBufferSize(const size_t uchar_length, const size_t scale /* = 1*/)
{
#if U_SIZEOF_WCHAR_T == 4
  return Unicode::GetUTF16ToUTF32BufferSize(uchar_length, scale);
#else
  return Unicode::GetUTF16WorkingSize(uchar_length, scale);
#endif
}

/**
 * Calculates a reasonably sized UChar buffer based upon the size of existing
 * UChar string.
 *
 * param uchar_length byte-length of UTF-8 string to be converted
 * param scale multiplier to apply to get larger buffer, default = 2.0
 *
 * Note that the returned size has a pad of BUFFER_PAD UChars added to leave room
 * for growth.
 *
 * Note that a UTF-16 string will be at most as long as the UTF-8 string.
 */

size_t Unicode::GetUTF16WorkingSize(const size_t utf16length, const float scale)
{
  float scale_factor = std::fmax((float)scale, 2.0);
  return (size_t)(BUFFER_PAD + int(utf16length * scale_factor));
}

/**
 * Calculates the maximum number of UTF-8 bytes required by a UTF-16 string.
 *
 * param uchar_length Number of UTF-16 code units to convert
 * param scale multiplier to apply to get a larger buffer
 *
 * Note that BUFFER_PAD bytes is added to the result to give some room for growth.
 *
 * Note that in addition to any scale factor, the number of UTF-8 bytes
 * returned is 3 times the number of UTF-16 code unites. This leaves enough
 * room for the 'worst case' UTF-8  expansion required for Indic, Thai and CJK.
 */

size_t Unicode::GetUTF16ToUTF8BufferSize(const size_t uchar_length, const size_t scale)
{
  // When converting from UTF-16 to UTF-8, the result will have at most 3 times
  // as many bytes as the source has UChars.
  // The "worst cases" are writing systems like Indic, Thai and CJK with
  // 3:1 bytes:UChars.
  int scale_factor = std::fmax((int)scale, 1);
  return (size_t)(BUFFER_PAD + uchar_length * 3 * scale_factor);
}

/**
 * Calculates the maximum number of wchar_t code units required to
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
 * param uchar_length number of UTF-16 code units in original string
 * param scale scale factor to multiply uchar_length by to allow for growth
 * returns Number of wchar_t to allocate for the desired buffer.
 *
 * Note that an additional BUFFER_PAD code units is added to the result to allow for growth
 *
 */
size_t Unicode::GetUTF16ToWCharBufferSize(const size_t uchar_length, const size_t scale)
{
  size_t scale_factor = std::max((int)scale, 1);
#if U_SIZEOF_WCHAR_T == 4

  // When converting from UTF-16 to UTF-32, the result will have at most 2 times
  // as many bytes as the source has UChars.

  return (size_t)(BUFFER_PAD + uchar_length * 2 * scale_factor);
#else
  // When converting from UTF-16 to UTF-16, the size is the same
  return (size_t)(BUFFER_PAD + uchar_length * scale_factor);
#endif
}

std::u16string_view Unicode::UTF8ToUTF16WithSub(const std::string_view& src,
    UChar* buffer,
    const size_t bufferSize,
    const size_t src_offset /* = 0 */,
    const size_t src_length /* = std::string::npos */)

{
  size_t length = std::min(src.length(), src_length);
  int32_t srcLength;

  // Number of corrupt UTF8 codepoints in string. Can be caused by truncating beginning
  // or end of string in mid multi-byte character

  int32_t numberOfSubstitutions = 0;

  // Substitute codepoint to use when a corrupt UTF8 code unit is encountered.

  const UChar32 subchar = (UChar32)0xFFFD; // Displays as (�)

  if (length == std::string::npos)
    srcLength = -1;
  else
    srcLength = length;

  UErrorCode status = U_ZERO_ERROR;
  int32_t utf8ActualLength = 0;
  u_strFromUTF8WithSub(buffer, bufferSize, &utf8ActualLength, src.data() + src_offset, srcLength, subchar,
      &numberOfSubstitutions, &status);

  if (numberOfSubstitutions > 0)
  {
    std::string newStr = Unicode::UTF16ToUTF8({buffer, (size_t)utf8ActualLength});

    CLog::Log(LOGDEBUG, "{} codepoint substitutions Unicode::UTF8ToUTF16WithSub utf8: {} UChar: {}\n",
        numberOfSubstitutions, src, newStr);
  }

  size_t resultLength = (size_t) utf8ActualLength;
  if (U_FAILURE(status))
  {
    CLog::Log(LOGERROR, "Error in Unicode::UTF8ToUTF16WithSub: {}\n", status);
    buffer[0] = '\0';
    resultLength = 0;
  }
  return {buffer, resultLength};
}

std::string_view Unicode::UTF16ToUTF8WithSub(const UChar* u_str,
    char* utf8Buffer,
    size_t utf8BufferSize,
    const size_t u_str_length)
{
  UErrorCode status = U_ZERO_ERROR;


  // Number of corrupt UTF8 codepoints in string. Can be caused by truncating beginning
  // or end of string in mid multi-byte character. May have occurred on utf8 to UChar
  // conversion.

  int32_t numberOfSubstitutions = 0;

  // Substitute codepoint to use when a corrupt UTF8 codepoint is encountered.

  const UChar32 subchar = (UChar32)0xFFFD; // Displays as "�"
  // UChar string can expand to up to 3x UTF8 chars

  int32_t u_str_length_param;
  if (u_str_length == std::string::npos)
    u_str_length_param = -1;
  else
    u_str_length_param = u_str_length;

  status = U_ZERO_ERROR;
  int32_t utf8ActualLength = 0;
  u_strToUTF8WithSub(utf8Buffer, utf8BufferSize, &utf8ActualLength, u_str, u_str_length_param,
      subchar, &numberOfSubstitutions, &status);

  if (numberOfSubstitutions > 0)
  {
    CLog::Log(LOGDEBUG,
        "Unicode::UTF16ToUTF8WithSub {} codepoint substitutions made\n",
        numberOfSubstitutions);
  }
  size_t length = (size_t) utf8ActualLength;
  if (U_FAILURE(status))
  {
    CLog::Log(LOGERROR, "Error in Unicode::UTF16ToUTF8WithSub: {}\n", status);

    utf8Buffer[0] = '\0';
    length = 0;
  }
  return {utf8Buffer, length};
}

wchar_t* Unicode::UTF16ToWChar(const UChar* u_str,
    wchar_t* buffer,
    size_t bufferSize,
    int32_t& destLength,
    const size_t u_str_length)
{
  UErrorCode status = U_ZERO_ERROR;

  int32_t u_str_length_param;
  if (u_str_length == std::string::npos)
    u_str_length_param = -1;
  else
    u_str_length_param = u_str_length;

  status = U_ZERO_ERROR;
  u_strToWCS(buffer, bufferSize, &destLength, u_str, u_str_length_param,
      &status);

  if (U_FAILURE(status))
  {
    CLog::Log(LOGERROR, "Error in Unicode::UTF16ToWChar: {}\n", status);
    return nullptr;
  }
  return buffer;
}

std::u16string_view Unicode::WStringToUTF16WithSub(const std::wstring_view src,
    char16_t * buffer,
    const size_t bufferSize,
    const size_t srcOffset /* = 0 */,
    const size_t src_length /* = std::string::npos */)
{
  // Number of corrupt UTF16 codepoints in string. Can be caused by truncating beginning
  // or end of string in mid multi-byte character

  int32_t numberOfSubstitutions = 0;

  // Substitute codepoint to use when a corrupt codepoint is encountered.

  const UChar32 subchar = (UChar32)0xFFFD; // Displays as "�"
  UErrorCode status = U_ZERO_ERROR;

  int32_t destLength;
#if U_SIZEOF_WCHAR_T == 4
  u_strFromUTF32WithSub((char16_t *) buffer, (int32_t) bufferSize, &destLength,
      (int32_t*)src.data(), (int32_t)(src.length() + srcOffset), subchar,
      &numberOfSubstitutions, &status);
#else
  // Should be a way to do substitutions on bad 'codepoints' as above.
  u_strFromWCS(buffer, bufferSize, &destLength, src.data(), srcLength, /* subchar,
			&numberOfSubstitutions, */
      &status);
#endif

  if (numberOfSubstitutions > 0)
  {
    CLog::Log(LOGDEBUG, "{} codepoint substitutions made in Unicode::WStringToUTF16WithSub\n",
        numberOfSubstitutions);
  }
  size_t resultLength = (size_t) destLength;
  if (U_FAILURE(status))
  {
    CLog::Log(LOGERROR, "Error in WStringToUTF16WithSub: {}\n", status);
    buffer[0] = '\0';
    resultLength = 0;
  }
  return {buffer, resultLength};
}

/*
 * Determines if UChar32 codepoint is a 'latin' character.
 *
 * Used to determine how letters are processed. (Some languages,
 * like Chinese, have words which are also letters. Code which
 * expects words to be composed of letters with spaces as separators
 * don't work so well with such languages.)
 *
 * A better solution would be to use Word BreakIterator, but
 * that would not fit well with current implementation.
 */
bool Unicode::IsLatinChar(const UChar32 codepoint)
{

  // Basic Latin, 0000–007F. This block corresponds to ASCII.
  // Latin-1 Supplement, 0080–00FF
  if (codepoint < 0x00ff)
    return true;

  // 	Latin Extended-A, 0100–017F
  if (codepoint >= 0x100 and codepoint <= 0x17f)
    return true;
  // 	Latin Extended-B, 0180–024F
  if (codepoint >= 0x180 and codepoint <= 0x24f)
    return true;

  return false;
}

icu::Locale Unicode::GetICULocale(const std::locale& locale)
{

  UErrorCode status = U_ZERO_ERROR;
  icu::Locale icu_locale = Unicode::GetICULocale(locale.name().data());
  if (U_FAILURE(status))
  {
    CLog::Log(LOGERROR, "Error in Unicode::GetICULocale: {}\n", status);
  }
  return icu_locale;
}

std::string Unicode::GetICULocaleId(const icu::Locale locale)
{
  std::string localeId = std::string();
  if (locale != nullptr)
  {
    if (locale.getLanguage() != nullptr)
    {
      localeId.append(locale.getLanguage());
      if (locale.getCountry() != nullptr)
      {
        localeId.append("_");
        localeId.append(locale.getCountry());
      }
    }
  }
  return localeId;
}

icu::Locale Unicode::GetDefaultICULocale()
{
  std::string language = g_langInfo.GetLocale().GetLanguageCode();
  if (language.length() < 2)
  {
    language = "en";
  }
  std::string country = g_langInfo.GetLocale().GetTerritoryCode();
  if (country.length() < 2)
  {
    country = "";
  }
  icu::Locale icu_locale = Unicode::GetICULocale(language.data(), country.data());
  return icu_locale;
}

icu::Locale Unicode::GetICULocale(const char* language,
    const char* country,
    const char* variant,
    const char* keywordsAndValues)
{

  UErrorCode status = U_ZERO_ERROR;

  icu::Locale icu_locale = icu::Locale(language, country, variant, keywordsAndValues = 0);
  if (U_FAILURE(status))
  {
    CLog::Log(LOGERROR, "Error in Unicode::GetICULocale: {}\n", status);
  }
  return icu_locale;
}

std::string Unicode::ToUpper(const std::string_view& s1, const icu::Locale& locale)
{

  icu::Locale currentLocale = locale;
  if (currentLocale == nullptr)
    currentLocale = Unicode::GetDefaultICULocale();

  if (s1.length() == 0)
    return std::string(s1);

  // Create buffer on stack

  size_t s1BufferSize = Unicode::GetUTF8ToUTF16BufferSize(s1.length(), 1.5);
  UChar s1Buffer[s1BufferSize];
  std::u16string_view sv1 = Unicode::UTF8ToUTF16WithSub(s1, s1Buffer, s1BufferSize);

  // Place to write the Upper-case text

  size_t s2BufferSize = Unicode::GetUTF16WorkingSize(sv1.length(), 2);
  UChar s2Buffer[s2BufferSize];
  std::u16string_view sv2 = {s2Buffer, s2BufferSize};

  UErrorCode status = U_ZERO_ERROR;
  std::string localeId = Unicode::GetICULocaleId(locale);

  int actualUpperLength = u_strToUpper((char16_t *)sv2.data(), sv2.length(), sv1.data(),
      sv1.length(), localeId.data(), &status);

  // Pack result in string_view. Data same as sv2, but length changed

  std::u16string_view svResult{sv2.data(), (size_t)actualUpperLength};
  if (U_FAILURE(status))
  {
    CLog::Log(LOGERROR, "Error in Unicode::ToUpper: {}\n", status);
    return std::string();
  }

  // Assuming that the returned UChar string has no invalid code-unit  we can use the
  // presumably cheaper conversion to UTF8.

  std::string result = Unicode::UTF16ToUTF8(svResult);
  return result;
}

const std::string Unicode::ToLower(const std::string& s1, const icu::Locale& locale)
{
  icu::Locale currentLocale = locale;
  if (currentLocale == nullptr)
    currentLocale = Unicode::GetDefaultICULocale();

  if (s1.length() == 0)
    return std::string(s1);

  // Create buffer on stack

  size_t s1BufferSize = Unicode::GetUTF8ToUTF16BufferSize(s1.length(), 1.5);
  UChar s1Buffer[s1BufferSize];
  std::u16string_view sv1 = Unicode::UTF8ToUTF16WithSub(s1, s1Buffer, s1BufferSize);

  // Place to write the Lower-case text

  size_t s2BufferSize = Unicode::GetUTF16WorkingSize(sv1.length(), 2);
  UChar s2Buffer[s2BufferSize];
  std::u16string_view sv2 = {s2Buffer, s2BufferSize};

  UErrorCode status = U_ZERO_ERROR;
  std::string localeId = Unicode::GetICULocaleId(locale);

  int actualLowerLength = u_strToLower((char16_t *)sv2.data(), sv2.length(), sv1.data(),
      sv1.length(), localeId.data(), &status);

  // Pack result in string_view. Data same as sv2, but length changed

  std::u16string_view svResult{sv2.data(), (size_t)actualLowerLength};
  if (U_FAILURE(status))
  {
    CLog::Log(LOGERROR, "Error in Unicode::ToLower: {}\n", status);
    return std::string();
  }

  // Assuming that the returned UChar string has no invalid code-unit  we can use the
  // presumably cheaper conversion to UTF8.

  std::string result = Unicode::UTF16ToUTF8(svResult);
  return result;
}

const std::wstring Unicode::ToCapitalize(const std::wstring_view& s1)
{
  // Convert s1 to utf16 using stack allocated buffer

  size_t s1BufferSize = Unicode::GetWcharToUTF16BufferSize(s1.length(), 1);
  UChar s1Buffer[s1BufferSize];
  std::u16string_view sv1 = Unicode::WStringToUTF16WithSub(s1, s1Buffer, s1BufferSize);

  // UnicodeString also using stack allocated memory
  // Base buffer size based on utf16 version of s1 that it will manipulate

  size_t s2BufferSize = Unicode::GetUTF16WorkingSize(sv1.length(), 2);
  UChar s2Buffer[s2BufferSize];
  std::u16string_view sv2 = {s2Buffer, s2BufferSize};

  // Allocate empty buffer to UnicodeString
  icu::UnicodeString uString = icu::UnicodeString((char16_t *)sv2.data(), 0, (int32_t) sv2.length());

  // Copy utf16 version of s1 to UnicodeString

  uString.append(sv1.data(), 0, sv1.length());

  icu::StringCharacterIterator iter = icu::StringCharacterIterator(uString);

  // Create buffer for second UnicodeString

  size_t s3BufferSize = Unicode::GetUTF16WorkingSize(sv1.length(), 2);
  UChar s3Buffer[s3BufferSize];
  std::u16string_view sv3 = {s3Buffer, s3BufferSize};

  icu::UnicodeString titleCase = icu::UnicodeString((char16_t *)sv3.data(), 0, (int32_t) sv3.length());

  // Use simple ToUpper, which changes a single codepoint. Using TitleCase would be
  // better

  bool isFirstLetter = true;
  while (iter.hasNext())
  {
    UChar32 cp = iter.next32PostInc();

    if (u_isspace(cp) || (u_ispunct(cp) && cp != '\''))
    {
      isFirstLetter = true;
    }
    else if (isFirstLetter)
    {
      cp = u_toupper(cp);
      isFirstLetter = false;
    }
    titleCase += cp;
  }

  // Assuming that the returned UChar string has no invalid code-units, other than the ones
  // marked invalid in the first place. Therefore, we can use the cheaper conversion to wchar_t.

  std::u16string_view  UCharStr = std::u16string_view(titleCase.getBuffer(), titleCase.length());
  std::wstring result = Unicode::UTF16ToWString(UCharStr);
  return result;
}

const std::string Unicode::ToCapitalize(const std::string_view& s1)
{
  // Convert s1 to utf16 using stack allocated buffer

  size_t s1BufferSize = Unicode::GetUTF8ToUTF16BufferSize(s1.length(), 1.5);
  UChar s1Buffer[s1BufferSize];
  std::u16string_view sv1 = Unicode::UTF8ToUTF16WithSub(s1, s1Buffer, s1BufferSize);

  // UnicodeString also using stack allocated memory
  // Base buffer size based on utf16 version of s1 that it will manipulate

  size_t s2BufferSize = Unicode::GetUTF16WorkingSize(sv1.length(), 2);
  UChar s2Buffer[s2BufferSize];
  std::u16string_view sv2 = {s2Buffer, s2BufferSize};

  // Allocate empty buffer to UnicodeString
  icu::UnicodeString uString = icu::UnicodeString((char16_t *)sv2.data(), 0, (int32_t) sv2.length());

  // Copy utf16 version of s1 to UnicodeString

  uString.append(sv1.data(), 0, sv1.length());

  icu::StringCharacterIterator iter = icu::StringCharacterIterator(uString);

  // Create buffer for second UnicodeString

  size_t s3BufferSize = Unicode::GetUTF16WorkingSize(sv1.length(), 2);
  UChar s3Buffer[s3BufferSize];
  std::u16string_view sv3 = {s3Buffer, s3BufferSize};

  icu::UnicodeString titleCase = icu::UnicodeString((char16_t *)sv3.data(), 0, (int32_t)sv3.length());

  // Use simple ToUpper, which changes a single codepoint. Using TitleCase would be
  // better

  bool isFirstLetter = true;
  while (iter.hasNext())
  {
    UChar32 cp = iter.next32PostInc();

    if (u_isspace(cp) || (u_ispunct(cp) && cp != '\''))
    {
      isFirstLetter = true;
    }
    else if (isFirstLetter)
    {
      cp = u_toupper(cp);
      isFirstLetter = false;
    }
    titleCase += cp;
  }

  std::string utf8Result = std::string();
  utf8Result = titleCase.toUTF8String(utf8Result);
  return utf8Result;
}

const std::wstring Unicode::TitleCase(const std::wstring_view& s1, const icu::Locale& locale)
{
  UErrorCode status = U_ZERO_ERROR;
  std::wstring wResult;

  icu::BreakIterator* wordIterator = icu::BreakIterator::createWordInstance(locale, status);

  if (U_FAILURE(status))
  {
    CLog::Log(LOGERROR, "Error in Unicode::TitleCase_w: {}\n", status);
    return std::wstring(s1);
  }

  // Create buffer on stack

  size_t s1BufferSize = Unicode::GetWcharToUTF16BufferSize(s1.length(), 1.5);
  UChar s1Buffer[s1BufferSize];
  std::u16string_view sv1 = Unicode::WStringToUTF16WithSub(s1, s1Buffer, s1BufferSize);

  size_t s2BufferSize = Unicode::GetUTF16WorkingSize(sv1.length(), 2);
  UChar s2Buffer[s2BufferSize];
  std::u16string_view sv2 = {s2Buffer, s2BufferSize};

  uint32_t options = 0; // Some options are interesting
  int32_t result_length = icu::CaseMap::toTitle(Unicode::GetICULocaleId(locale).data(),
      options, wordIterator,
      (char16_t *) sv1.data(), sv1.length(), (char16_t *) sv2.data(), sv2.length(), NULL, status);

  std::u16string_view svTitle{sv2.data(), (size_t)result_length}; // Same data, different length
  if (U_FAILURE(status))
  {
    if (status == U_BUFFER_OVERFLOW_ERROR)
    {
      CLog::Log(LOGERROR, "Error in Unicode::TitleCase_w: buffer not large enough need: {}\n",
          result_length);
      delete wordIterator;
      return wResult = std::wstring(s1);
    }
    else
    {
      CLog::Log(LOGERROR, "Error in Unicode::TitleCase_w: {}\n", status);
      delete wordIterator;
      return wResult = std::wstring(s1);
    }
  }

  wResult = Unicode::UTF16ToWString(svTitle);

  delete wordIterator;
  return wResult;
}

const std::string Unicode::TitleCase(const std::string_view& s1, const icu::Locale& locale)
{
  UErrorCode status = U_ZERO_ERROR;
  std::string result;

  icu::BreakIterator* wordIterator = icu::BreakIterator::createWordInstance(locale, status);

  if (U_FAILURE(status))
  {
    CLog::Log(LOGERROR, "Error in Unicode::TitleCase w: {}\n", status);
    return std::string(s1);
  }
  status = U_ZERO_ERROR;

  // Create buffer on stack

  size_t s1BufferSize = Unicode::GetUTF8ToUTF16BufferSize(s1.length(), 1.5);
  UChar s1Buffer[s1BufferSize];
  std::u16string_view sv1 = Unicode::UTF8ToUTF16WithSub(s1, s1Buffer, s1BufferSize);

  size_t s2BufferSize = Unicode::GetUTF16WorkingSize(sv1.length(), 2);
  UChar s2Buffer[s2BufferSize];
  std::u16string_view sv2 = {s2Buffer, s2BufferSize};

  uint32_t options = 0; // Some options are interesting
  size_t result_length = icu::CaseMap::toTitle(Unicode::GetICULocaleId(locale).data(), options, wordIterator,
      sv1.data(), sv1.length(), (char16_t *)sv2.data(), sv2.length(), NULL, status);

  // svResult same buffer as sv2, but different length
  std::u16string_view svResult{sv2.data(), result_length};

  if (U_FAILURE(status))
  {
    result = std::string(s1);
    if (status == U_BUFFER_OVERFLOW_ERROR)
    {
      CLog::Log(LOGERROR, "Error in Unicode::TitleCase: buffer not large enough need: {}\n",
          result_length);
    }
    else
    {
      CLog::Log(LOGERROR, "Error in Unicode::TitleCase: {}\n", status);
    }
  }
  else
  {
    result = Unicode::UTF16ToUTF8(svResult);
  }
  return result;
}

std::wstring Unicode::FoldCase(const std::wstring_view& s1, const StringOptions options)
{
  std::wstring result;
  if (s1.length() == 0)
    return result = std::wstring(s1);

  // Create buffer on stack. sv1 input, sv2 = fold output

  size_t s1BufferSize = Unicode::GetWcharToUTF16BufferSize(s1.length(), 2);
  UChar s1Buffer[s1BufferSize];
  std::u16string_view sv1 = Unicode::WStringToUTF16WithSub(s1, s1Buffer, s1BufferSize);

  // Create buffer to hold case fold

  // Retry capability if size estimate too low. This should not be needed given
  // that buffer size estimates tend to be a bit generous.

  int forceBufferSize = 0;
  bool finished = false;
  UErrorCode status = U_ZERO_ERROR;
  while (not finished)
  {
    status = U_ZERO_ERROR;
    size_t s2BufferSize = forceBufferSize;
    if (forceBufferSize == 0)
      s2BufferSize = Unicode::GetUTF16WorkingSize(sv1.length(), 1);

    UChar s2Buffer[s2BufferSize];
    std::u16string_view sv2 = {s2Buffer, s2BufferSize};

    size_t u_folded_length = 0; // In UChars
    u_folded_length = (size_t) icu::CaseMap::fold(to_underlying(options), sv1.data(),
        sv1.length(), (UChar *)sv2.data(),
        (int)sv2.length(), nullptr, status);

    finished = true;
    if (U_FAILURE(status))
    {
      if (status == U_BUFFER_OVERFLOW_ERROR)
      {
        finished = false;
        forceBufferSize = u_folded_length + 1; // for null
        CLog::Log(LOGINFO, "Retry in Unicode::FoldCase_w: buffer not large enough need: {}\n",
            u_folded_length);
      }
      else
      {
        result = std::wstring(s1);
        CLog::Log(LOGERROR, "Error in Unicode::FoldCase: {}\n", status);
      }
    }
    else
    {
      // sv2.length is readonly. Create new stringView using u_folded_length

      std::u16string_view svFolded = {sv2.data(), u_folded_length};

      // Assuming that the returned UChar string has no invalid code-units, other than the ones
      // marked invalid in the first place. Therefore, we can use the cheaper conversion to wchar_t.

      result = Unicode::UTF16ToWString(svFolded);
    }
  }
  return result;
}

std::string Unicode::FoldCase(const std::string_view& s1, const StringOptions options)
{
  std::string result = std::string();
  if (s1.length() == 0)
    return result = std::string(s1);

  // Create buffer on stack

  size_t s1BufferSize = Unicode::GetUTF8WorkingSize(s1.length(), 1.5);
  char s1Buffer[s1BufferSize];

  // Using byte sink this time. Not many apis support it, but UnicodeString does

  icu::CheckedArrayByteSink sink = icu::CheckedArrayByteSink(s1Buffer, s1BufferSize);

  UErrorCode status = U_ZERO_ERROR;
  icu::StringPiece sp = icu::StringPiece(s1); // StringPiece similar to string_view
  icu::CaseMap::utf8Fold((size_t) options, sp, sink, nullptr, status);

  if (U_FAILURE(status))
  {
    // Buffer should be big enough
    if (sink.Overflowed())
    {
      CLog::Log(LOGERROR, "Error in Unicode::FoldCase: buffer not large enough need: {}\n",
          sink.NumberOfBytesAppended());
      return result = std::string(s1);
    }
    else
    {
      CLog::Log(LOGERROR, "Error in Unicode::FoldCase: {}\n", status);
      return result = std::string(s1);
    }
  }
  else
  {
    size_t size = sink.NumberOfBytesWritten();
    result = std::string(s1Buffer, size);
  }
  return result;
}

const std::wstring Unicode::Normalize(const std::wstring_view& s1,
    const StringOptions option,
    const NormalizerType NormalizerType)
{
  if (s1.length() == 0)
    return {s1.data(), s1.length()};

  UErrorCode status = U_ZERO_ERROR;

  const UNormalizer2* normalizer;

  switch (NormalizerType)
  {
    case NormalizerType::NFC:
    {
      normalizer = unorm2_getNFCInstance(&status);
      break;
    }
    case NormalizerType::NFD:
    {
      normalizer = unorm2_getNFDInstance(&status);
      break;
    }
    case NormalizerType::NFKC:
    {
      normalizer = unorm2_getNFKCInstance(&status);
      break;
    }
    case NormalizerType::NFKD:
    {
      normalizer = unorm2_getNFKDInstance(&status);
      break;
    }
    case NormalizerType::NFCKCASEFOLD:
    {
      normalizer = unorm2_getNFKCCasefoldInstance(&status);
      break;
    }
  }
  // Create stack buffer for result of normalization

  size_t s1BufferSize = Unicode::GetWcharToUTF16BufferSize(s1.length(), 2);
  UChar s1Buffer[s1BufferSize];

  std::u16string_view sv1 = Unicode::WStringToUTF16WithSub(s1, s1Buffer, s1BufferSize);

  size_t nBufferSize = Unicode::GetUTF16WorkingSize(sv1.length(), 2);
  char16_t nBuffer[nBufferSize];

  // Folds and Normalizes at same time.
  size_t normalizedLength;
  normalizedLength =
      unorm2_normalize(normalizer, (char16_t *)sv1.data(), sv1.length(), nBuffer,
          nBufferSize, &status);

  // Same data but different length from nBuffer

  std::u16string_view svNormalized = {nBuffer, normalizedLength};

  std::wstring result;
  if (not U_FAILURE(status))
  {
    result = Unicode::UTF16ToWString(svNormalized);
  }
  else
    result = {s1.data(), s1.length()}; // Do the best we can

  return result;
}

const std::string Unicode::Normalize(const std::string_view& s1,
    const StringOptions options,
    const NormalizerType NormalizerType)
{
  if (s1.length() == 0)
    return "";

  // Create buffer on stack

  size_t s1BufferSize = Unicode::GetUTF8WorkingSize(s1.length(), 2);
  char s1Buffer[s1BufferSize];
  std::string_view sv1{s1Buffer, s1BufferSize};

  icu::CheckedArrayByteSink s1Sink = icu::CheckedArrayByteSink((char *)sv1.data(), sv1.length());

  UErrorCode status = U_ZERO_ERROR;
  Unicode::Normalize(s1, s1Sink, status, to_underlying(options), NormalizerType);

  std::string result;
  if (U_FAILURE(status))
  {
    result = std::string(s1);
    if (s1Sink.Overflowed())
    {
      CLog::Log(LOGERROR, "Error in Unicode::Normalize buffer not large enough need: {}\n",
          s1Sink.NumberOfBytesAppended());
    }
    else
    {
      CLog::Log(LOGERROR, "Error in Unicode::Normalize: {}\n", status);
    }
  }
  else
  {
    size_t size = s1Sink.NumberOfBytesWritten();
    result = std::string(sv1.data(), size);
  }
  return result;
}

void Unicode::Normalize(const std::string_view src,
    icu::CheckedArrayByteSink& sink,
    UErrorCode& status,
    const int32_t options,
    const NormalizerType NormalizerType)
{
  const icu::Normalizer2* normalizer;
  bool composeMode;

  switch (NormalizerType)
  {
    case NormalizerType::NFC:
    {
      normalizer = icu::Normalizer2::getNFCInstance(status);
      composeMode = true;
      break;
    }
    case NormalizerType::NFD:
    {
      normalizer = icu::Normalizer2::getNFDInstance(status);
      composeMode = false;
      break;
    }
    case NormalizerType::NFKC:
    {
      normalizer = icu::Normalizer2::getNFKCInstance(status);
      composeMode = true;
      break;
    }
    case NormalizerType::NFKD:
    {
      normalizer = icu::Normalizer2::getNFKDInstance(status);
      composeMode = false;
      break;
    }
    case NormalizerType::NFCKCASEFOLD:
    {
      normalizer = icu::Normalizer2::getNFKCCasefoldInstance(status);
      composeMode = true;
      break;
    }
  }
  if (U_FAILURE(status))
  {
    CLog::Log(LOGINFO, "Error in Unicode::Normalize create:");
  }
  else
  {
    if (composeMode)
    {
      normalizer->normalizeUTF8(options, src, sink, nullptr, status);
    }
    else
    {
      icu::UnicodeString us = Unicode::ToUnicodeString(src);
      icu::UnicodeString result = normalizer->normalize(us, status);
      result.toUTF8(sink);
    }
    if (U_FAILURE(status))
    {
      CLog::Log(LOGINFO, "Error in Unicode::Normalize call: ");
    }
  }
  if (U_FAILURE(status))
  {
    sink.Reset(); // In case there is junk
    sink.Append(src.data(), src.length());
  }
}

int Unicode::StrCaseCmp(const std::wstring_view& s1,
    const std::wstring_view& s2,
    const StringOptions options,
    const bool normalize /* = false */)
{
  if (s1.empty() and s2.empty())
    return 0;

  if (s1.empty())
    return -1;

  if (s2.empty())
    return 1;

  // Create buffer on stack

  size_t s1BufferSize = Unicode::GetWcharToUTF16BufferSize(s1.length(), 2);
  UChar s1Buffer[s1BufferSize];
  size_t s2BufferSize = Unicode::GetWcharToUTF16BufferSize(s2.length(), 2);
  UChar s2Buffer[s2BufferSize];

  std::u16string_view sv1 = Unicode::WStringToUTF16WithSub(s1, s1Buffer, s1BufferSize);
  std::u16string_view sv2 = Unicode::WStringToUTF16WithSub(s2, s2Buffer, s2BufferSize);

  UErrorCode status = U_ZERO_ERROR;
  int result;
  if (not normalize)
  {
    result = u_strCaseCompare((char16_t *)sv1.data(), (int32_t) sv1.length(),
        (char16_t *)sv2.data(), (int32_t) sv2.length(),
        to_underlying(options),
        &status);
  }
  else
  {
    StringOptions opts = options | StringOptions::COMPARE_IGNORE_CASE;

    result = unorm_compare(sv1.data(), sv1.length(), sv2.data(), sv2.length(), to_underlying(opts), &status);
  }
  if (U_FAILURE(status))
  {
    CLog::Log(LOGERROR, "Error in Unicode::wstrcasecmp {}\n", status);
    result = 0; // Doesn't matter much
  }
  return result;
}

int Unicode::StrCaseCmp(const std::string_view& s1,
    const std::string_view& s2,
    const StringOptions options,
    const bool normalize /* = false */)
{

  if (s1.empty() and s2.empty())
    return 0;

  if (s1.empty())
    return -1;

  if (s2.empty())
    return 1;

  // Create buffer on stack

  size_t s1BufferSize = Unicode::GetUTF8ToUTF16BufferSize(s1.length(), 2);
  UChar s1Buffer[s1BufferSize];
  size_t s2BufferSize = Unicode::GetUTF8ToUTF16BufferSize(s2.length(), 2);
  UChar s2Buffer[s2BufferSize];

  std::u16string_view sv1 = Unicode::UTF8ToUTF16WithSub(s1, s1Buffer, s1BufferSize);
  std::u16string_view sv2 = Unicode::UTF8ToUTF16WithSub(s2, s2Buffer, s2BufferSize);

  int result;
  UErrorCode status = U_ZERO_ERROR;
  if (not normalize)
  {
    result = u_strCaseCompare((char16_t *)sv1.data(), (int32_t) sv1.length(),
        (char16_t *)sv2.data(), (int32_t) sv2.length(),
        to_underlying(options),
        &status);
  }
  else
  {
    StringOptions opts = options | StringOptions::COMPARE_IGNORE_CASE;

    result = unorm_compare(sv1.data(), sv1.length(), sv2.data(),
        sv2.length(), to_underlying(opts), &status);
  }
  if (U_FAILURE(status))
  {
    CLog::Log(LOGERROR, "Error in Unicode::StrCaseCmp {}\n", status);
    result = 0; // Doesn't matter much
  }
  return result;
}

int Unicode::StrCaseCmp(const std::string_view& s1,
    size_t s1_start,
    size_t s1_length,
    const std::string_view& s2,
    size_t s2_start,
    size_t s2_length,
    const StringOptions options,
    const bool normalize /* = false */)
{

  // Lengths are relative to UTF8 encoding. Only send the bytes
  // specified to the Unicode lib, otherwise there would be
  // no way to apply the start and length later.
  // If a multi-byte character is truncated the resulting
  // UTF8ToUTF16WithSub conversion will contain U+FFFD codepoints Displays as "�"

  if (s1.empty() and s2.empty())
    return 0;

  if (s1.empty())
    return -1;

  if (s2.empty())
    return 1;

  // Create buffer on stack

  // TODO: Validate s1_start, s1_length, etc. are valid (within string and
  //       are on at least code-unit boundaries).

  // Use given length instead of from string (s1).

  size_t s1BufferSize = Unicode::GetUTF8ToUTF16BufferSize(s1_length, 2);
  UChar s1Buffer[s1BufferSize];
  size_t s2BufferSize = Unicode::GetUTF8ToUTF16BufferSize(s2_length, 2);
  UChar s2Buffer[s2BufferSize];

  std::u16string_view sv1 = Unicode::UTF8ToUTF16WithSub(s1, s1Buffer, s1BufferSize,
      s1_start, s1_length);
  std::u16string_view sv2 = Unicode::UTF8ToUTF16WithSub(s2, s2Buffer, s2BufferSize,
      s2_start, s2_length);

  UErrorCode status = U_ZERO_ERROR;
  int result;
  if (not normalize)
  {
    result = u_strCaseCompare((char16_t *)sv1.data(), (int32_t) sv1.length(),
        (char16_t *)sv2.data(), (int32_t) sv2.length(), to_underlying(options),
        &status);
  }
  else
  {
    StringOptions opts = options | StringOptions::COMPARE_IGNORE_CASE;
    result = unorm_compare(sv1.data(), sv1.length(), sv2.data(), sv2.length(),
        to_underlying(opts), &status);
  }
  if (U_FAILURE(status))
  {
    CLog::Log(LOGERROR, "Error in Unicode::StrCaseCmp {}\n", status);
    result = 0;  // Doesn't matter much
  }
  return result;
}

int Unicode::StrCaseCmp(const std::string_view& s1,
    const std::string_view& s2,
    size_t n,
    const StringOptions options,
    const bool normalize /* = false */)
{

  size_t n1 = std::min(n, s1.length());
  size_t n2 = std::min(n, s2.length());

  return Unicode::StrCaseCmp(s1, 0, n1, s2, 0, n2, options, normalize);
}

int8_t Unicode::StrCmp(const std::wstring_view& s1,
    size_t s1_start,
    size_t s1_length,
    const std::wstring_view& s2,
    size_t s2_start,
    size_t s2_length,
    const bool normalize /* = false */)
{

  // TODO: Use faster icu::Collator::compareUTF8()
  // TODO: Implement Normalize

  // Lengths are relative to wchar_t encoding (usually 32-bit Unicode codepoints,
  // but can be UTF-16). Only send the bytes specified to the Unicode lib, otherwise
  // there would be no way to apply the start and length later.

  // Create buffer on stack

  // TODO: Validate s1_start, s1_length, etc. are valid (within string and
  //       are on at least code-unit boundaries).

  // Use given length instead of from string (s1).

  size_t s1BufferSize = Unicode::GetWcharToUTF16BufferSize(s1_length, 2);
  UChar s1Buffer[s1BufferSize];
  size_t s2BufferSize = Unicode::GetWcharToUTF16BufferSize(s2_length, 2);
  UChar s2Buffer[s2BufferSize];

  std::u16string_view sv1 = Unicode::WStringToUTF16WithSub(s1, s1Buffer, s1BufferSize,
      s1_start, s1_length);
  std::u16string_view sv2 = Unicode::WStringToUTF16WithSub(s2, s2Buffer, s2BufferSize,
      s2_start, s2_length);

  // Bitwise comparison in codepoint order (no normalization, or locale considerations etc.).
  // Could use u_strcmp, but it does not take lengths

  int8_t result = u_strCompare(sv1.data(), sv1.length(), sv2.data(), sv2.length(), true);
  return result;
}

int8_t Unicode::StrCmp(const std::string_view& s1,
    size_t s1_start,
    size_t s1_length,
    const std::string_view& s2,
    size_t s2_start,
    size_t s2_length,
    const bool normalize /* = false */)
{

  // TODO: Use faster icu::Collator::compareUTF8()
  // TODO: Implement Normalize

  // Lengths are relative to UTF8 encoding. Only send the bytes
  // specified to the Unicode lib, otherwise there would be
  // no way to apply the start and length later.

  // Create buffer on stack

  size_t u_s1_size = Unicode::GetUTF8ToUTF16BufferSize(s1.length(), 2);
  UChar u_s1[u_s1_size];
  size_t u_s2_size = Unicode::GetUTF8ToUTF16BufferSize(s2.length(), 2);
  UChar u_s2[u_s2_size];

  std::u16string_view sv1 = Unicode::UTF8ToUTF16WithSub(s1, u_s1, u_s1_size);
  std::u16string_view sv2 = Unicode::UTF8ToUTF16WithSub(s2, u_s2, u_s2_size);

  // Bitwise comparison in codepoint order (no normalization, or locale considerations etc.).
  // Could use u_strcmp, but it does not take lengths

  int8_t result = u_strCompare(sv1.data(), sv1.length(), sv2.data(), sv2.length(), true);
  return result;
}

bool Unicode::StartsWith(const std::string_view& s1, const std::string_view& s2)
{
  // Create buffer on stack

  // TODO: Validate s1_start, s1_length, etc. are valid (within string and
  //       are on at least code-unit boundaries).

  // Use given length instead of from string (s1).

  size_t s1BufferSize = Unicode::GetUTF8ToUTF16BufferSize(s1.length(), 2);
  char16_t s1Buffer[s1BufferSize];
  size_t s2BufferSize = Unicode::GetUTF8ToUTF16BufferSize(s2.length(), 2);
  char16_t s2Buffer[s2BufferSize];

  std::u16string_view sv1 = Unicode::UTF8ToUTF16WithSub(s1, (char16_t *)s1Buffer, s1BufferSize);
  std::u16string_view sv2 = Unicode::UTF8ToUTF16WithSub(s2, (char16_t *)s2Buffer, s2BufferSize);

  // stack Buffers backing UnicodeString

  size_t us1BufferSize = Unicode::GetUTF16WorkingSize(sv1.length(), 3);
  char16_t us1Buffer[us1BufferSize];
  size_t us2BufferSize = Unicode::GetUTF16WorkingSize(sv2.length(), 3);
  char16_t us2Buffer[us2BufferSize];

  std::u16string_view usSV1{us1Buffer, us1BufferSize};
  std::u16string_view usSV2{us2Buffer, us2BufferSize};


  // Allocate empty buffer to UnicodeString
  icu::UnicodeString uString1 = icu::UnicodeString((char16_t *)usSV1.data(), 0, (int32_t) usSV1.length());
  icu::UnicodeString uString2 = icu::UnicodeString((char16_t *)usSV2.data(), 0, (int32_t) usSV2.length());

  // Copy utf16 version of s1 to UnicodeString

  uString1.append(sv1.data(), 0, sv1.length());
  uString2.append(sv2.data(), 0, sv2.length());

  return uString1.startsWith(uString2);
}

bool Unicode::StartsWithNoCase(const std::string_view& s1,
    const std::string_view& s2,
    const StringOptions options)
{
  // Create buffer on stack

  // TODO: Validate s1_start, s1_length, etc. are valid (within string and
  //       are on at least code-unit boundaries).

  // Use given length instead of from string (s1).

  size_t s1BufferSize = Unicode::GetUTF8ToUTF16BufferSize(s1.length(), 2);
  char16_t s1Buffer[s1BufferSize];
  size_t s2BufferSize = Unicode::GetUTF8ToUTF16BufferSize(s2.length(), 2);
  char16_t s2Buffer[s2BufferSize];

  std::u16string_view sv1 = Unicode::UTF8ToUTF16WithSub(s1, (char16_t *)s1Buffer, s1BufferSize);
  std::u16string_view sv2 = Unicode::UTF8ToUTF16WithSub(s2, (char16_t *)s2Buffer, s2BufferSize);

  // stack Buffers backing UnicodeString

  size_t us1BufferSize = Unicode::GetUTF16WorkingSize(sv1.length(), 3);
  char16_t us1Buffer[us1BufferSize];
  size_t us2BufferSize = Unicode::GetUTF16WorkingSize(sv2.length(), 3);
  char16_t us2Buffer[us2BufferSize];

  std::u16string_view usSV1{us1Buffer, us1BufferSize};
  std::u16string_view usSV2{us2Buffer, us2BufferSize};


  // Allocate empty buffer to UnicodeString
  icu::UnicodeString uString1 = icu::UnicodeString((char16_t *)usSV1.data(), 0, (int32_t) usSV1.length());
  icu::UnicodeString uString2 = icu::UnicodeString((char16_t *)usSV2.data(), 0, (int32_t) usSV2.length());

  // Copy utf16 version of s1 to UnicodeString

  uString1.append(sv1.data(), 0, sv1.length());
  uString2.append(sv2.data(), 0, sv2.length());

  uString1.foldCase(to_underlying(options));
  uString2.foldCase(to_underlying(options));

  UBool result = uString1.startsWith(uString2);
  return result;
}
bool Unicode::EndsWith(const std::string_view& s1, const std::string_view& s2)
{
  // Create buffer on stack

  // TODO: Validate s1_start, s1_length, etc. are valid (within string and
  //       are on at least code-unit boundaries).

  // Use given length instead of from string (s1).

  size_t s1BufferSize = Unicode::GetUTF8ToUTF16BufferSize(s1.length(), 2);
  char16_t s1Buffer[s1BufferSize];
  size_t s2BufferSize = Unicode::GetUTF8ToUTF16BufferSize(s2.length(), 2);
  char16_t s2Buffer[s2BufferSize];

  std::u16string_view sv1 = Unicode::UTF8ToUTF16WithSub(s1, (char16_t *)s1Buffer, s1BufferSize);
  std::u16string_view sv2 = Unicode::UTF8ToUTF16WithSub(s2, (char16_t *)s2Buffer, s2BufferSize);

  // stack Buffers backing UnicodeString

  size_t us1BufferSize = Unicode::GetUTF16WorkingSize(sv1.length(), 3);
  char16_t us1Buffer[us1BufferSize];
  size_t us2BufferSize = Unicode::GetUTF16WorkingSize(sv2.length(), 3);
  char16_t us2Buffer[us2BufferSize];

  std::u16string_view usSV1{us1Buffer, us1BufferSize};
  std::u16string_view usSV2{us2Buffer, us2BufferSize};


  // Allocate empty buffer to UnicodeString
  icu::UnicodeString uString1 = icu::UnicodeString((char16_t *)usSV1.data(), 0, (int32_t) usSV1.length());
  icu::UnicodeString uString2 = icu::UnicodeString((char16_t *)usSV2.data(), 0, (int32_t) usSV2.length());

  // Copy utf16 version of s1 to UnicodeString

  uString1.append(sv1.data(), 0, sv1.length());
  uString2.append(sv2.data(), 0, sv2.length());

  return uString1.endsWith(uString2);
}

bool Unicode::EndsWithNoCase(const std::string_view& s1,
    const std::string_view& s2,
    const StringOptions options)
{
  // Create buffer on stack

  // TODO: Validate s1_start, s1_length, etc. are valid (within string and
  //       are on at least code-unit boundaries).

  // Use given length instead of from string (s1).

  size_t s1BufferSize = Unicode::GetUTF8ToUTF16BufferSize(s1.length(), 2);
  char16_t s1Buffer[s1BufferSize];
  size_t s2BufferSize = Unicode::GetUTF8ToUTF16BufferSize(s2.length(), 2);
  char16_t s2Buffer[s2BufferSize];

  std::u16string_view sv1 = Unicode::UTF8ToUTF16WithSub(s1, (char16_t *)s1Buffer, s1BufferSize);
  std::u16string_view sv2 = Unicode::UTF8ToUTF16WithSub(s2, (char16_t *)s2Buffer, s2BufferSize);

  // stack Buffers backing UnicodeString

  size_t us1BufferSize = Unicode::GetUTF16WorkingSize(sv1.length(), 3);
  char16_t us1Buffer[us1BufferSize];
  size_t us2BufferSize = Unicode::GetUTF16WorkingSize(sv2.length(), 3);
  char16_t us2Buffer[us2BufferSize];

  std::u16string_view usSV1{us1Buffer, us1BufferSize};
  std::u16string_view usSV2{us2Buffer, us2BufferSize};


  // Allocate empty buffer to UnicodeString
  icu::UnicodeString uString1 = icu::UnicodeString((char16_t *)usSV1.data(), 0, (int32_t) usSV1.length());
  icu::UnicodeString uString2 = icu::UnicodeString((char16_t *)usSV2.data(), 0, (int32_t) usSV2.length());

  // Copy utf16 version of s1 to UnicodeString

  uString1.append(sv1.data(), 0, sv1.length());
  uString2.append(sv2.data(), 0, sv2.length());

  uString1.foldCase(to_underlying(options));
  uString2.foldCase(to_underlying(options));

  UBool result = uString1.endsWith(uString2);
  return result;
}

/*!
 * \brief Get the leftmost side of a UTF-8 string, limited by character count
 *
 * Unicode characters are of variable byte length. This function's
 * parameters are based on characters and NOT bytes.
 *
 * \param str to get a substring of
 * \param charIndex if keepLeft, maximum number of characters to keep from left end
 *                  else number of characters to remove from right end
 * \param keepLeft controls how charCount is interpreted
 * \return leftmost characters of string, length determined by charCount
 *
 */
std::string Unicode::Left(const std::string_view& str,
    const size_t charCount,
    const icu::Locale icuLocale,
    const bool keepLeft /* = true */)
{
  if (charCount == 0)
  {
    if (keepLeft)
      return std::string();
    else
      return std::string(str);
  }
  size_t utf8Index;
  std::string result;
  result = Unicode::Normalize(str, StringOptions::FOLD_CASE_DEFAULT, NormalizerType::NFC);
  if (result.compare(str) != 0)
    CLog::Log(LOGINFO, "Unicode::Left Normalized string different from original");

  size_t charIdx = charCount - 1;
  if (!keepLeft)
    charIdx++; // Need first byte of next char

  UText* ut = Unicode::ConfigUText(result);
  if (ut == nullptr)
  {
    result = std::string(str);
    return result;
  }
  icu::BreakIterator* cbi = Unicode::ConfigCharBreakIter(ut, icuLocale);
  if (cbi == nullptr)
  {
    result = std::string(str);
    ut = utext_close(ut);
    return result;
  }
  utf8Index = Unicode::GetCharPosition(cbi, result.length(), charIdx, true, keepLeft);
  delete (cbi);
  ut = utext_close(ut);

  size_t bytesToCopy = 0;
  if (keepLeft)
  {
    if (utf8Index == std::string::npos) // Error
    {
      bytesToCopy = result.length();
    }
    else if (utf8Index == BEFORE_START)
    {
      bytesToCopy = 0;
    }
    else if (utf8Index == AFTER_END) // Not enough chars
    {
      bytesToCopy = result.length();
    }
    else
      bytesToCopy = utf8Index + 1;
  }
  else // Remove chars from right end
  {
    if (utf8Index == std::string::npos) // Error
    {
      bytesToCopy = result.length(); // Remove none
    }
    else if (utf8Index == BEFORE_START)
    {
      bytesToCopy = 0; // Remove none
    }
    else if (utf8Index == AFTER_END)
    {
      bytesToCopy = 0;
    }
    else
      bytesToCopy = utf8Index + 1;
  }

  return result.substr(0, bytesToCopy);
}

/*!
 *  \brief Get a substring of a UTF-8 string
 *
 * Unicode characters are of variable byte length. This function's
 * parameters are based on characters and NOT bytes.
 *
 * \param str string to extract substring from
 * \param startCharCount leftmost character of substring [0 based]
 * \param charCount maximum number of characters to include in substring
 * \return substring of str, beginning with character 'firstCharIndex',
 *         length determined by charCount
 */
std::string Unicode::Mid(const std::string_view& str,
    size_t startCharCount,
    size_t charCount /* = std::string::npos */)
{
  size_t startUTF8Index;
  size_t endUTF8Index;
  std::string result;
  result = Unicode::Normalize(str, StringOptions::FOLD_CASE_DEFAULT, NormalizerType::NFC);
  if (result.compare(str) != 0)
    CLog::Log(LOGINFO, "Unicode::Mid Normalized string different from original");

  UText* ut = Unicode::ConfigUText(result);
  if (ut == nullptr)
  {
    result = std::string(str);
    return result;
  }
  icu::BreakIterator* cbi = Unicode::ConfigCharBreakIter(ut, Unicode::GetDefaultICULocale());
  if (cbi == nullptr)
  {
    result = std::string(str);
    ut = utext_close(ut);
    return result;
  }

  // We need the start byte to begin copy
  //   left=false keepLeft=true   Return offset of first byte of nth character

  bool left = false;
  bool keepLeft = true;

  startUTF8Index = Unicode::GetCharPosition(cbi, result.length(), startCharCount, left, keepLeft);
  if (startUTF8Index == std::string::npos) // Error
  {
    return std::string();
  }
  else if (startUTF8Index == BEFORE_START) // Should not occur
  {
    return std::string();
  }
  else if (startUTF8Index == AFTER_END) // Not enough chars
  {
    return std::string();
  }

  result = result.substr(startUTF8Index);
  ut = Unicode::ConfigUText(result, ut); // Reset config to modified string
  if (ut == nullptr)
  {
    result = std::string(str);
    return result;
  }
  cbi = Unicode::ReConfigCharBreakIter(ut, cbi);
  if (cbi == nullptr)
  {
    ut = utext_close(ut);
    return result;
  }

  // We specify here how many characters to copy from the first substring. But we have to translate
  // the character count into a byteindex. It does look like we want the normal Left(xxx, n) copy:
  //
  //  left=true keepLeft=true  Return offset of last byte of nth character
  //
  // So, treat this second step just like left (we could just call Left, but that would add a bit of
  // overhead

  left = true;
  keepLeft = true;
  endUTF8Index = Unicode::GetCharPosition(cbi, result.length(), charCount, left, keepLeft);
  ut = utext_close(ut); // Does free(ut)
  delete (cbi);

  size_t bytesToCopy = 0;
  if (endUTF8Index == std::string::npos) // Error
  {
    bytesToCopy = result.length();
  }
  else if (endUTF8Index == BEFORE_START)
  {
    bytesToCopy = 0;
  }
  else if (endUTF8Index == AFTER_END) // Not enough chars
  {
    bytesToCopy = result.length();
  }
  else
    bytesToCopy = endUTF8Index;

  return result.substr(0, bytesToCopy);
}

std::string Unicode::Right(const std::string_view& str,
    size_t charCount,
    const icu::Locale& icuLocale,
    bool keepRight)
{
  size_t utf8Index;
  std::string result;
  if (charCount == 0)
  {
    if (keepRight)
      return std::string();
    else
      return std::string(str);
  }

  result = Unicode::Normalize(str, StringOptions::FOLD_CASE_DEFAULT, NormalizerType::NFC);
  if (result.compare(str) != 0)
    CLog::Log(LOGINFO, "Unicode::Right Normalized string different from original");

  UText* ut = Unicode::ConfigUText(result);
  if (ut == nullptr)
  {
    result = std::string(str);
    return result;
  }
  icu::BreakIterator* cbi = Unicode::ConfigCharBreakIter(ut, icuLocale);
  if (cbi == nullptr)
  {
    result = std::string(str);
    ut = utext_close(ut);
    return result;
  }

  // charCount == 0 was handled above.

  size_t charIdx = charCount - 1;
  if (!keepRight) // Omit charCount chars from left end; need first byte of following char
    charIdx++;

  utf8Index = Unicode::GetCharPosition(cbi, result.length(), charIdx, false, !keepRight);
  ut = utext_close(ut); // Does delete
  delete (cbi);
  cbi = nullptr;

  if (keepRight)
  {
    if (utf8Index == std::string::npos) // Error
    {
      utf8Index = result.length();
    }
    else if (utf8Index == BEFORE_START)
    {
      utf8Index = 0;
    }
    else if (utf8Index == AFTER_END)
    {
      utf8Index = str.length();
    }
  }
  else
  {
    if (utf8Index == std::string::npos) // Error
    {
      utf8Index = 0;
    }
    else if (utf8Index == BEFORE_START)
    {
      utf8Index = 0;
      ;
    }
    else if (utf8Index == AFTER_END)
    {
      utf8Index = str.length();
    }
  }

  return result.substr(utf8Index, std::string::npos);
}

UText* Unicode::ConfigUText(const std::string_view& str, UText* ut /* = nullptr */)
{
  UErrorCode status = U_ZERO_ERROR;

  status = U_ZERO_ERROR;
  ut = utext_openUTF8(ut, str.data(), str.length(), &status);
  if (U_FAILURE(status))
  {
    CLog::Log(LOGERROR, "Error in Unicode::ConfigUText: {}\n", status);
    return nullptr;
  }
  return ut;
}

icu::BreakIterator* Unicode::ConfigCharBreakIter(UText* ut, const icu::Locale& icuLocale)
{
  UErrorCode status = U_ZERO_ERROR;
  icu::BreakIterator* cbi = nullptr;
  if (cbi == nullptr)
  {
    cbi = icu::BreakIterator::createCharacterInstance(icuLocale, status);
    if (U_FAILURE(status))
    {
      CLog::Log(LOGERROR, "Error in Unicode::ConfigCharBreakIter: {}\n", status);
      cbi = nullptr;
      return nullptr;
    }
  }
  status = U_ZERO_ERROR;
  cbi->setText(ut, status);

  if (U_FAILURE(status))
  {
    CLog::Log(LOGERROR, "Error in Unicode::ConfigCharBreakIter: {}\n", status);
    return nullptr;
  }
  if (not cbi->isBoundary(0))
  {
    // If was not on boundary, isBoundary advanced to next boundary (if there was one).
    // What happens if we were on last character of string?

    CLog::Log(
        LOGWARNING,
        "Unicode::ConfigCharBreakIter string is malformed, does not start with valid character.\n");
    return nullptr;
  }
  return cbi;
}

icu::BreakIterator* Unicode::ReConfigCharBreakIter(UText* ut, icu::BreakIterator* cbi)
{
  UErrorCode status = U_ZERO_ERROR;
  status = U_ZERO_ERROR;
  cbi->setText(ut, status);

  if (U_FAILURE(status))
  {
    CLog::Log(LOGERROR, "Error in Unicode::ReConfigCharBreakIter: {}\n", status);
    delete (cbi);
    return nullptr;
  }
  if (not cbi->isBoundary(0))
  {
    // If was not on boundary, isBoundary advanced to next boundary (if there was one).
    // What happens if we were on last character of string?

    CLog::Log(LOGWARNING, "Unicode::ReConfigCharBreakIter string is malformed, does not start with "
        "valid character.\n");
    delete (cbi);
    return nullptr;
  }
  return cbi;
}

size_t Unicode::GetCharPosition(icu::BreakIterator* cbi,
    size_t stringLength,
    size_t charCount,
    const bool left,
    const bool keepLeft)
{
  size_t uCharIndex = std::string::npos;
  if (charCount > stringLength) // Deal with unsignededness...
    charCount = INT_MAX;

  if (keepLeft) // Get to nth character from the left
  {
    uCharIndex = cbi->first(); // At charCount 0
    uCharIndex = cbi->next(charCount);
    // There is a boundary between the last character and the end of string
    // Have to check both against npos and length (sigh)
    if (uCharIndex == std::string::npos or uCharIndex >= stringLength)
    {
      // String not long enough
      if (charCount == 0)
        uCharIndex = Unicode::BEFORE_START;
      else
        uCharIndex = Unicode::AFTER_END;
    }
  }
  else // Get to nth character from right, character 0 is BEYOND last character
  {
    cbi->last(); // BEYOND last
    cbi->preceding(stringLength);
    uCharIndex = cbi->next(-charCount);
    if (uCharIndex == std::string::npos)
    {
      if (charCount == 0)
        uCharIndex = AFTER_END;
      else
        uCharIndex = BEFORE_START;
    }
  }
  // Now should be positioned to correct character.

  if (uCharIndex < AFTER_END)
  {
    if (left) // Return offset of last byte of current character
    {
      uCharIndex = cbi->next();
      uCharIndex--;
    }
  }
  return uCharIndex;
}


// TODO: Rework these to be reusable. That is, don't destroy the iterator on
//       a find of a single character. The destruction should be external.

icu::BreakIterator* Unicode::ConfigCharBreakIter(const icu::UnicodeString& str,
    const icu::Locale& icuLocale)
{
  UErrorCode status = U_ZERO_ERROR;
  icu::BreakIterator* cbi = nullptr;
  if (cbi == nullptr)
  {
    cbi = icu::BreakIterator::createCharacterInstance(icuLocale, status);
    if (U_FAILURE(status))
    {
      CLog::Log(LOGERROR, "Error in Unicode::ConfigCharBreakIter: {}\n", status);
      cbi = nullptr;
      return nullptr;
    }
    status = U_ZERO_ERROR;
  }

  cbi->setText(str);

  if (not cbi->isBoundary(0))
  {
    // If was not on boundary, isBoundary advanced to next boundary (if there was one).
    // What happens if we were on last character of string?

    CLog::Log(
        LOGWARNING,
        "Unicode::ConfigCharBreakIter string is malformed, does not start with valid character.");
    delete cbi;
    return nullptr;
  }
  return cbi;
}

size_t Unicode::GetCharPosition(const icu::UnicodeString& uStr,
    const size_t charCountArg,
    const bool left,
    const bool keepLeft,
    const icu::Locale& icuLocale)
{

  icu::BreakIterator* cbi = Unicode::ConfigCharBreakIter(uStr, icuLocale);
  if (cbi == nullptr)
  {
    return Unicode::ERROR;
  }

  size_t charIdx = Unicode::GetCharPosition(cbi, uStr.length(), charCountArg, left, keepLeft);
  delete (cbi);
  return charIdx;
}

size_t Unicode::GetCharPosition(const std::string_view& str,
    const size_t charCountArg,
    const bool left,
    const bool keepLeft,
    const icu::Locale& icuLocale)
{
  UText* ut = Unicode::ConfigUText(str);
  if (ut == nullptr)
    return Unicode::ERROR;

  icu::BreakIterator* cbi = Unicode::ConfigCharBreakIter(ut, icuLocale);
  if (cbi == nullptr)
  {
    ut = utext_close(ut);
    return Unicode::ERROR;
  }

  size_t utf8Index = Unicode::GetCharPosition(cbi, str.length(), charCountArg, left, keepLeft);
  delete (cbi);
  ut = utext_close(ut);
  return utf8Index;
}

std::string Unicode::Trim(const std::string_view& s1)
{
  // TODO: change to use uniset.h spanUTF8

  // Create buffer on stack

  // TODO: Validate s1_start, s1_length, etc. are valid (within string and
  //       are on at least code-unit boundaries).

  // Use given length instead of from string (s1).

  size_t s1BufferSize = Unicode::GetUTF8ToUTF16BufferSize(s1.length(), 2);
  char16_t s1Buffer[s1BufferSize];

  std::u16string_view sv1 = Unicode::UTF8ToUTF16WithSub(s1, (char16_t *)s1Buffer, s1BufferSize);

  // stack Buffers backing UnicodeString

  size_t us1BufferSize = Unicode::GetUTF16WorkingSize(sv1.length(), 3);
  char16_t us1Buffer[us1BufferSize];

  std::u16string_view usSV1{us1Buffer, us1BufferSize};

  // Allocate empty buffer to UnicodeString
  icu::UnicodeString uString1 = icu::UnicodeString((char16_t *)usSV1.data(), 0, (int32_t) usSV1.length());

  // Copy utf16 version of s1 to UnicodeString

  uString1.append(sv1.data(), 0, sv1.length());

  std::string result = std::string();
  result = uString1.trim().toUTF8String(result);
  return result;
}

std::string Unicode::TrimLeft(const std::string_view& s1)
{
  // TODO: change to use uniset.h spanUTF8

   // Create buffer on stack

   // TODO: Validate s1_start, s1_length, etc. are valid (within string and
   //       are on at least code-unit boundaries).

   // Use given length instead of from string (s1).

   size_t s1BufferSize = Unicode::GetUTF8ToUTF16BufferSize(s1.length(), 2);
   char16_t s1Buffer[s1BufferSize];

   std::u16string_view sv1 = Unicode::UTF8ToUTF16WithSub(s1, (char16_t *)s1Buffer, s1BufferSize);

   // stack Buffers backing UnicodeString

   size_t us1BufferSize = Unicode::GetUTF16WorkingSize(sv1.length(), 3);
   char16_t us1Buffer[us1BufferSize];

   std::u16string_view usSV1{us1Buffer, us1BufferSize};

   // Allocate empty buffer to UnicodeString
   icu::UnicodeString uString1 = icu::UnicodeString((char16_t *)usSV1.data(), 0, (int32_t) usSV1.length());

   // Copy utf16 version of s1 to UnicodeString

   uString1.append(sv1.data(), 0, sv1.length());

  std::string result = std::string();
  Unicode::Trim(uString1, true, false).toUTF8String(result);
  return result;
}

std::string Unicode::TrimLeft(const std::string_view& s1, const std::string_view s2)
{
  // Create buffer on stack

   // TODO: Validate s1_start, s1_length, etc. are valid (within string and
   //       are on at least code-unit boundaries).

   // Use given length instead of from string (s1).

   size_t s1BufferSize = Unicode::GetUTF8ToUTF16BufferSize(s1.length(), 2);
   char16_t s1Buffer[s1BufferSize];
   size_t s2BufferSize = Unicode::GetUTF8ToUTF16BufferSize(s2.length(), 2);
   char16_t s2Buffer[s2BufferSize];

   std::u16string_view sv1 = Unicode::UTF8ToUTF16WithSub(s1, (char16_t *)s1Buffer, s1BufferSize);
   std::u16string_view sv2 = Unicode::UTF8ToUTF16WithSub(s2, (char16_t *)s2Buffer, s2BufferSize);

   // stack Buffers backing UnicodeString

   size_t us1BufferSize = Unicode::GetUTF16WorkingSize(sv1.length(), 3);
   char16_t us1Buffer[us1BufferSize];
   size_t us2BufferSize = Unicode::GetUTF16WorkingSize(sv2.length(), 3);
   char16_t us2Buffer[us2BufferSize];

   std::u16string_view usSV1{us1Buffer, us1BufferSize};
   std::u16string_view usSV2{us2Buffer, us2BufferSize};


   // Allocate empty buffer to UnicodeString
   icu::UnicodeString uString1 = icu::UnicodeString((char16_t *)usSV1.data(), 0, (int32_t) usSV1.length());
   icu::UnicodeString uTrimChars = icu::UnicodeString((char16_t *)usSV2.data(), 0, (int32_t) usSV2.length());

   // Copy utf16 version of s1 to UnicodeString

   uString1.append(sv1.data(), 0, sv1.length());
   uTrimChars.append(sv2.data(), 0, sv2.length());

  std::string result = std::string();
  Unicode::Trim(uString1, uTrimChars, true, false).toUTF8String(result);
  return result;
}

std::string Unicode::TrimRight(const std::string_view& str)
{
  // TODO: change to use uniset.h spanBackUTF8

  icu::UnicodeString uString = Unicode::ToUnicodeString(str);
  std::string result = std::string();
  Unicode::Trim(uString, false, true).toUTF8String(result);
  return result;
}

std::string Unicode::TrimRight(const std::string_view& s1, const std::string_view trimChars)
{
  // Create buffer on stack

   // TODO: Validate s1_start, s1_length, etc. are valid (within string and
   //       are on at least code-unit boundaries).

   // Use given length instead of from string (s1).

   size_t s1BufferSize = Unicode::GetUTF8ToUTF16BufferSize(s1.length(), 2);
   char16_t s1Buffer[s1BufferSize];
   size_t s2BufferSize = Unicode::GetUTF8ToUTF16BufferSize(trimChars.length(), 2);
   char16_t s2Buffer[s2BufferSize];

   std::u16string_view sv1 = Unicode::UTF8ToUTF16WithSub(s1, (char16_t *)s1Buffer, s1BufferSize);
   std::u16string_view sv2 = Unicode::UTF8ToUTF16WithSub(trimChars, (char16_t *)s2Buffer, s2BufferSize);

   // stack Buffers backing UnicodeString

   size_t us1BufferSize = Unicode::GetUTF16WorkingSize(sv1.length(), 3);
   char16_t us1Buffer[us1BufferSize];
   size_t us2BufferSize = Unicode::GetUTF16WorkingSize(sv2.length(), 3);
   char16_t us2Buffer[us2BufferSize];

   std::u16string_view usSV1{us1Buffer, us1BufferSize};
   std::u16string_view usSV2{us2Buffer, us2BufferSize};

   // Allocate empty buffer to UnicodeStrings
   icu::UnicodeString uString1 = icu::UnicodeString((char16_t *)usSV1.data(), 0, (int32_t) usSV1.length());
   icu::UnicodeString uTrimChars = icu::UnicodeString((char16_t *)usSV2.data(), 0, (int32_t) usSV2.length());

   // Could create stack-based UnicodeString for return value. However it would
   // be pass by reference.... For now, allocate it from heap in the function and be
   // done with it. Cleaner, if a tiny bit slower.

  std::string result = std::string();
  Unicode::Trim(uString1, uTrimChars, false, true).toUTF8String(result);

  return result;
}

// TODO: Verify that offset is in correct units: bytes, characters, codepoints, etc.
// TODO: What about multi-codepoint characters?
// TODO: What if trimChars has a non-Character in it (i.e. an accent without a
// character that it is attached to?

icu::UnicodeString Unicode::Trim(const icu::UnicodeString& uStr,
    const icu::UnicodeString& trimChars,
    const bool trimLeft,
    const bool trimRight)
{
  // TODO: BROKEN for multi UChar characters. Use Character iterator, or similar.
  // TODO: Revisit use of size_t with int32_t
  // TODO: change to use uniset.h spanUTF8 & spanBackUTF8

  icu::UnicodeString str = icu::UnicodeString(uStr);
  if (str.length() == 0 or trimChars.length() == 0)
    return (str);

  size_t chars_to_delete = 0;
  if (trimLeft)
  {
    for (size_t i = 0; i < (size_t)str.length(); i += 1)
    {
      UChar ch = str[i];
      if (trimChars.indexOf(ch) == -1)
      {
        break;
      }
      else
      {
        chars_to_delete++;
      }
    }
    if (chars_to_delete > 0)
    {
      str.remove(0, chars_to_delete);
    }
  }

  chars_to_delete = 0;
  if (trimRight)
  {
    for (size_t i = str.length() - 1; i >= 0; i -= 1)
    {
      UChar ch = str[i];
      if (trimChars.indexOf(ch) == -1)
      {
        break;
      }
      else
      {
        chars_to_delete++;
      }
    }

    if (chars_to_delete > 0)
    {
      str.remove(str.length() - chars_to_delete, chars_to_delete);
    }
  }
  return str;
}

std::string Unicode::Trim(const std::string_view& str,
    const std::vector<std::string>& trimChars,
    const bool trimLeft,
    const bool trimRight)
{
  icu::UnicodeString uStr = Unicode::ToUnicodeString(str);
  std::vector<icu::UnicodeString> uDeleteStrings = std::vector<icu::UnicodeString>();
  for (auto delString : trimChars)
  {
    icu::UnicodeString utrimChars = Unicode::ToUnicodeString(delString);
    uDeleteStrings.push_back(utrimChars);
  }
  icu::UnicodeString uResult = Unicode::Trim(uStr, uDeleteStrings, trimLeft, trimRight);
  std::string result = std::string();
  result = uResult.toUTF8String(result);
  return result;
}

icu::UnicodeString Unicode::Trim(const icu::UnicodeString& uStr,
    const std::vector<icu::UnicodeString>& trimStrings,
    const bool trimLeft,
    const bool trimRight)
{
  // TODO: change to use uniset.h spanUTF8 & spanBackUTF8
  // TODO: Can be made a bit faster by sorting trimChars by codepoint prior to entry
  //       then you can tell when to quit trying for a match

  icu::UnicodeString str = icu::UnicodeString(uStr);
  if (str.length() == 0 or trimStrings.size() == 0)
    return str;

  icu::BreakIterator* cbi = Unicode::ConfigCharBreakIter(str, Unicode::GetDefaultICULocale());
  if (cbi == nullptr)
  {
    CLog::Log(LOGERROR, "Unicode::Trim ConfigCharBreakIter failed for {}\n", ToString(str));
    return str;
  }

  // Trim end of string first since the shorter string is, the cheaper it typically is
  // to remove chars from front. (Of course UnicodeStrings may behave differently).
  if (trimRight)
  {
    // left=false keepLeft=false  Returns offset of first byte of nth char from right end.
    bool left = false;
    bool keepLeft = false;
    size_t charCount = 0;
    size_t charStart = 0;
    size_t firstUCharToDelete = str.length();
    size_t charEnd = str.length(); // length in code-units (UTF-16), which is 1 or 2.

    while (charStart < Unicode::AFTER_END)
    {
      bool charDeleted = false;
      charStart = Unicode::GetCharPosition(cbi, str.length(), charCount, left, keepLeft);
      if (charStart >= Unicode::AFTER_END)
        break;

      for (auto trimString : trimStrings)
      {
        if (str.compareBetween(charStart, charEnd, trimString, 0, trimString.length()) == 0)
        {
          firstUCharToDelete = charStart;
          charDeleted = true;
          break;
        }
      }
      if (not charDeleted)
        break;
      charEnd = charStart;
      charCount++;
    } // while
    if (firstUCharToDelete < (size_t)str.length())
      str.remove(firstUCharToDelete, str.length());
  }

  if (trimLeft)
  {
    bool left = false; // Iterator to give index of first code-unit (UChar) of charCount
    bool keepLeft = true;
    size_t lastUCharToDelete = 0;
    size_t charCount = 0;
    size_t charStart = 0;
    while (charStart < Unicode::AFTER_END)
    {
      bool charDeleted = false;
      charCount++;
      size_t nextCharStart = Unicode::GetCharPosition(cbi, str.length(), charCount, left, keepLeft);
      if (nextCharStart >= Unicode::AFTER_END)
        break;

      for (auto trimString : trimStrings)
      {
        if (str.compareBetween(charStart, nextCharStart, trimString, 0, trimString.length()) == 0)
        {
          if (charStart < Unicode::AFTER_END)
            lastUCharToDelete = nextCharStart;
          else
            lastUCharToDelete = (size_t)(str.length() - 1);
          charDeleted = true;
          break;
        }
      }

      if (not charDeleted)
        break;
      charStart = nextCharStart;
    } // while
    if (lastUCharToDelete != 0)
      str.remove(0, lastUCharToDelete);
  }

  delete (cbi);
  return str;
}

icu::UnicodeString Unicode::Trim(const icu::UnicodeString& uStr,
    const bool trimLeft,
    const bool trimRight)
{

  // TODO: Revisit use of size_t with int32_t
  // TODO: change to use uniset.h spanUTF8 & spanBackUTF8

  icu::UnicodeString str = icu::UnicodeString(uStr);
  if (str.length() == 0)
    return str;

  size_t chars_to_delete = 0;
  if (trimLeft)
  {
    for (size_t i = 0; i < (size_t)str.length(); i += 1)
    {
      UChar ch = str[i];
      if (!u_isWhitespace(ch))
      {
        break;
      }
      else
      {
        chars_to_delete++;
      }
    }
    if (chars_to_delete > 0)
    {
      str.remove(0, chars_to_delete);
    }
  }

  chars_to_delete = 0;
  if (trimRight)
  {
    for (size_t i = str.length() - 1; i >= 0; i -= 1)
    {
      UChar ch = str[i];
      if (!u_isWhitespace(ch))
      {
        break;
      }
      else
      {
        chars_to_delete++;
      }
    }

    if (chars_to_delete > 0)
    {
      str.remove(str.length() - chars_to_delete, chars_to_delete);
    }
  }
  return str;
}

std::string Unicode::Trim(const std::string_view& str,
    const std::string_view& trimChars,
    const bool trimLeft,
    const bool trimRight)
{
  // TODO: change to use uniset.h spanUTF8 & spanBackUTF8

  if (str.length() == 0 or trimChars.length() == 0)
  {
    std::string result = std::string(str);
    return result;
  }

  icu::UnicodeString uString = Unicode::ToUnicodeString(str);

  // trimChars is a set of characters to delete. With Unicode, it is
  // more convenient to split these up into a vector of individual characters

  std::vector<icu::UnicodeString> deleteSet = std::vector<icu::UnicodeString>();
  if (trimChars.length() == 1)
  {
    icu::UnicodeString kDelChars = Unicode::ToUnicodeString(trimChars);
    deleteSet.push_back(kDelChars);
  }
  else
  {
    UText* ut = Unicode::ConfigUText(trimChars);
    if (ut == nullptr)
    {
      std::string result = std::string(str);
      return result;
    }
    icu::BreakIterator* cbi = Unicode::ConfigCharBreakIter(ut, Unicode::GetDefaultICULocale());
    if (cbi == nullptr)
    {
      std::string result = std::string(str);
      ut = utext_close(ut);
      return result;
    }

    size_t charCount = 0;
    size_t charStart = 0;
    size_t lastByte =
        trimChars.length() + 1; // Anything larger that length should do, but not string::npos

    bool left = true;
    bool keepLeft = true;
    while (lastByte < Unicode::AFTER_END)
    {
      lastByte = Unicode::GetCharPosition(cbi, trimChars.length(), charCount, left, keepLeft);
      if (lastByte < Unicode::AFTER_END)
      {
        std::string_view aChar = trimChars.substr(charStart, (lastByte + 1 - charStart));
        icu::UnicodeString uChar = Unicode::ToUnicodeString(aChar);
        deleteSet.push_back(uChar);
        charCount++;
        charStart = lastByte + 1;
      }
    }
    ut = utext_close(ut); // Does free
    delete (cbi);
    cbi = nullptr;
  }
  uString = Unicode::Trim(uString, deleteSet, trimLeft, trimRight);

  if ((size_t)uString.length() == str.length())
  {
    std::string result = std::string(str);
    return result;
  }

  std::string result = std::string();
  result = uString.toUTF8String(result);
  return result;
}

[[deprecated("FindAndReplace is faster, returned count not used.")]] std::tuple<std::string, int>
Unicode::FindCountAndReplace(const std::string_view& src,
    const std::string_view& oldText,
    const std::string_view& newText)
{
  icu::UnicodeString kSrc = Unicode::ToUnicodeString(src);
  icu::UnicodeString kOldText = Unicode::ToUnicodeString(oldText);
  icu::UnicodeString kNewText = Unicode::ToUnicodeString(newText);
  std::tuple<icu::UnicodeString, int> result =
      Unicode::FindCountAndReplace(kSrc, kOldText, kNewText);
  icu::UnicodeString kResultStr = std::get<0>(result);
  int changes = std::get<1>(result);
  std::string resultStr = std::string();
  kResultStr.toUTF8String(resultStr);
  return {resultStr, changes};
}

[[deprecated(
    "FindAndReplace is faster, returned count not used.")]] std::tuple<icu::UnicodeString, int>
Unicode::FindCountAndReplace(const icu::UnicodeString& kSrc,
    const icu::UnicodeString& kOldText,
    const icu::UnicodeString& kNewText)
{
  return Unicode::FindCountAndReplace(kSrc, 0, kSrc.length(), kOldText, 0, kOldText.length(),
      kNewText, 0, kNewText.length());
}

[[deprecated(
    "FindAndReplace is faster, returned count not used.")]] std::tuple<icu::UnicodeString, int>
Unicode::FindCountAndReplace(const icu::UnicodeString& srcText,
    const int32_t srcStart,
    const int32_t srcLength,
    const icu::UnicodeString& oldText,
    const int32_t oldStart,
    const int32_t oldLength,
    const icu::UnicodeString& newText,
    const int32_t newStart,
    const int32_t newLength)
{
  int changes = 0;
  icu::UnicodeString resultStr = icu::UnicodeString(srcText);
  if (srcText.isBogus() || oldText.isBogus() || newText.isBogus())
  {
    return {resultStr, changes};
  }
  if (oldLength == 0)
  {
    return {resultStr, changes};
  }

  int32_t length = srcLength;
  int32_t start = srcStart;
  while (length > 0 && length >= oldLength)
  {
    int32_t pos = resultStr.indexOf(oldText, oldStart, oldLength, start, length);
    if (pos < 0)
    {
      // no more oldText's here: done
      break;
    }
    else
    {
      // we found oldText, replace it by newText and go beyond it

      resultStr.replace(pos, oldLength, newText, newStart, newLength);
      length -= pos + oldLength - start;
      start = pos + newLength;
      changes++;
    }
  }
  return {resultStr, changes};
}

bool Unicode::FindWord(const std::string_view& str, const std::string_view& word)
{

  icu::UnicodeString uString = Unicode::ToUnicodeString(str);
  uString.foldCase(U_FOLD_CASE_DEFAULT);
  icu::UnicodeString uWord = Unicode::ToUnicodeString(word);
  uWord.foldCase(U_FOLD_CASE_DEFAULT);

  int32_t wordLength = uWord.length(); // Code-unit (UTF16) length, not character length
  int32_t offset = 0;
  int32_t srcLength = uString.length(); // Code-unit length, not character length
  size_t index = std::string::npos;

  /**
   * We don't care what characters follow the found 'word' in src string.
   *
   * We also don't care what character proceeds word (if any).
   *
   * ------------- OLD Algorithm ----
   * When a match fails at the beginning of the string:
   *   * Any leading digits or alphabetic characters are trimmed from
   *     the beginning of the string.
   *   * If no digits or alphabetic characters are trimmed, then one
   *   * character is removed from the beginning of string
   *   * Any spaces are then trimmed from the beginning of the string
   *
   *  ---------- Equivalent Algorithm
   *
   * When a match fails in the middle of the string it must occur on a legal
   * "word" break. Valid word breaks are:
   * -  A space
   * -  The beginning of src
   * -  A space, or the beginning of src followed by all digits or all letters
   * -  A space, or beginning of src, followed by a single character
   *
   *
   * On each pass through the src string, the offset variable is advanced to the
   * segment of src under examination.
   *
   * At this point the process repeats
   */

  while (index == std::string::npos)
  {
    int32_t tempIndex = uString.indexOf(uWord, 0, wordLength, offset, srcLength);

    if (tempIndex == -1)
    {
      break;
    }
    if (tempIndex == offset)
    {
      index = tempIndex;
      break;
    }
    {
      /**
       * Advance offset to a valid break and repeat.
       *
       * If the first char is a digit, advance past all of the digits.
       * If the first char is a letter, advance past all of the digits
       * Otherwise, advance one char
       * Finally, advance past all spaces
       */

      if (offset < uString.length() and u_isdigit(uString.char32At(offset)))
      {
        do
        {
          offset = uString.moveIndex32(offset, 1); // Start of next codepoint
        } while (offset < uString.length() and u_isdigit(uString.char32At(offset)));

        // In some languages (Chinese, Japanese...) a single character
        // is a word, not a letter. Using u_isUAlphabetic for such
        // character returns true, but misleading in this context.
        // Use u_isalpha instead. Using a word breakiterator is much
        // more accurate, but it would change this function's behavior
      }
      else if (offset < uString.length() and Unicode::IsLatinChar(uString.char32At(offset)) and
          u_isalpha(uString.char32At(offset)))
      {
        do
        {
          offset = uString.moveIndex32(offset, 1);
        } while (offset < uString.length() and Unicode::IsLatinChar(uString.char32At(offset)) and
            u_isalpha(uString.char32At(offset)));
      }
      else
      {
        if (offset < uString.length())
          offset = uString.moveIndex32(offset, 1);
      }
      while (offset < uString.length())
      {
        if (u_isUWhiteSpace(uString.char32At(offset)))
        { // There are different whitespace characters
          offset = uString.moveIndex32(offset, 1);
        }
        else
        {
          break;
        }
      }
      // Try again with new src segment
    }
  }

  return index < Unicode::AFTER_END;
}

/*
 * Replaces every occurrence of oldText with newText within the string
 *
 */

std::string Unicode::FindAndReplace(const std::string_view& str,
    const std::string_view oldText,
    const std::string_view newText)
{
  icu::UnicodeString uString = Unicode::ToUnicodeString(str);
  icu::UnicodeString uOldText = Unicode::ToUnicodeString(oldText);
  icu::UnicodeString uNewText = Unicode::ToUnicodeString(newText);

  uString.findAndReplace(uOldText, uNewText);

  std::string resultStr = std::string();
  resultStr = uString.toUTF8String(resultStr);
  return resultStr;
}

/*
 * Regular expression patterns for this lib can be found at:
 * normal whitespace pattern: "\h"
 * https://unicode-org.github.io/icu/userguide/strings/regexp.html
 *
 */
size_t Unicode::RegexFind(const std::string_view& str, const std::string_view pattern, const int flags)
{
  // TODO: incomplete. Poorly defined

  icu::UnicodeString uString = Unicode::ToUnicodeString(str);
  icu::UnicodeString uPattern = Unicode::ToUnicodeString(pattern);
  size_t index = str.npos;

  UErrorCode status = U_ZERO_ERROR;
  icu::RegexMatcher* matcher = new icu::RegexMatcher(uPattern, uString, flags, status);

  if (U_FAILURE(status))
  {
    CLog::Log(LOGERROR, "Error in Unicode::RegexFind a {}\n", status);
    delete (matcher);
    return index;
  }

  while (matcher->find())
  {
    index = matcher->start(status);
    if (U_FAILURE(status))
    {
      CLog::Log(LOGERROR, "Error in Unicode::RegexFind 2 {}\n", status);
      delete (matcher);
      return index;
    }
    int groups = matcher->groupCount();

    for (int i = 0; i <= groups; i++) // group 0 is not included in count.
    {
      icu::UnicodeString match = matcher->group(i, status);
      if (U_FAILURE(status))
      {
        CLog::Log(LOGERROR, "Error in Unicode::RegexFind c {}\n", status);
        delete (matcher);
        return index;
      }
      if (match.length() > 0)
      {
        index = matcher->start(i, status);
        if (U_FAILURE(status))
        {
          CLog::Log(LOGERROR, "Error in Unicode::RegexFind d {}\n", status);
          delete (matcher);
          return index;
        }
      }
      std::string val = std::string();
      match.toUTF8String(val);
    }
  }
  delete (matcher);
  return index;
}

/*
 * Regular expression patterns for this lib can be found at:
 * normal whitespace pattern: "\h"
 * https://unicode-org.github.io/icu/userguide/strings/regexp.html
 *
 */
std::string Unicode::RegexReplaceAll(const std::string_view& str,
    const std::string_view pattern,
    const std::string_view replace,
    const int flags)
{
  icu::UnicodeString uString = Unicode::ToUnicodeString(str);
  icu::UnicodeString uPattern = Unicode::ToUnicodeString(pattern);
  icu::UnicodeString uReplace = Unicode::ToUnicodeString(replace);
  UErrorCode status = U_ZERO_ERROR;
  icu::RegexMatcher* matcher = new icu::RegexMatcher(uPattern, uString, flags, status);
  if (U_FAILURE(status))
  {
    CLog::Log(LOGERROR, "Error in Unicode::RegexReplaceAll a {}\n", status);
    if (matcher != nullptr)
      delete (matcher);
    std::string result = std::string(str);
    return result;
  }
  icu::UnicodeString uResult = matcher->replaceAll(uReplace, status);
  if (U_FAILURE(status))
  {
    CLog::Log(LOGERROR, "Error in Unicode::RegexReplaceAll b {}\n", status);
    delete (matcher);
    std::string result = std::string(str);
    return result;
  }
  std::string result = std::string();
  result = uResult.toUTF8String(result);
  delete (matcher);
  return result;
}

icu::UnicodeString Unicode::RegexReplaceAll(const icu::UnicodeString& uString,
    const icu::UnicodeString kPattern,
    const icu::UnicodeString kReplace,
    const int flags)
{
  UErrorCode status = U_ZERO_ERROR;
  icu::RegexMatcher* matcher = new icu::RegexMatcher(kPattern, uString, flags, status);
  if (U_FAILURE(status))
  {
    CLog::Log(LOGERROR, "Error in Unicode::RegexReplaceAll a {}\n", status);
    delete matcher;
    icu::UnicodeString uResult = icu::UnicodeString(uString);
    return uResult;
  }
  icu::UnicodeString result = matcher->replaceAll(kReplace, status);
  if (U_FAILURE(status))
  {
    CLog::Log(LOGERROR, "Error in Unicode::RegexReplaceAll b {}\n", status);
    delete matcher;
    icu::UnicodeString uResult = icu::UnicodeString(uString);
    return uResult;
  }

  delete matcher;
  icu::UnicodeString uResult = icu::UnicodeString(uString);
  return uResult;
}

int32_t Unicode::countOccurances(const std::string_view& strInput,
    const std::string_view& strFind,
    const int flags)
{
  icu::UnicodeString kstr = Unicode::ToUnicodeString(strInput);
  icu::UnicodeString kstrFind = Unicode::ToUnicodeString(strFind);
  int32_t count = 0;

  UErrorCode status = U_ZERO_ERROR;

  icu::RegexMatcher* matcher = new icu::RegexMatcher(kstrFind, kstr, flags, status);
  if (U_FAILURE(status))
  {
    CLog::Log(LOGERROR, "Error in Unicode::countOccurances {}\n", status);
    delete (matcher);
    return count;
  }
  while (matcher->find())
    count++;

  delete (matcher);
  return count;
}

/**
 * UnicodeString version of SplitTo. Another function provides a utf-8 wrapper.
 */

template<typename OutputIt>
OutputIt Unicode::SplitTo(OutputIt d_first,
    const icu::UnicodeString& kInput,
    const icu::UnicodeString& kDelimiter,
    size_t iMaxStrings /* = 0 */,
    const bool omitEmptyStrings /* = false */)
{

  OutputIt dest = d_first;

  // If empty string in, then nothing out (not even empty string)
  if (kInput.isEmpty())
    return dest;

  // If Nothing to split with, then no change to the input

  if (kDelimiter.isEmpty())
  {
    *d_first++ = kInput;
    return dest;
  }

  const size_t delimLen = kDelimiter.length();
  size_t nextDelim;
  size_t textPos = 0;
  do
  {
    // Keep splitting until limit reached
    if (--iMaxStrings == 0)
    {
      // Once limit reached, append remaining text to result and leave

      if (!omitEmptyStrings or (textPos < (size_t)kInput.length()))
      {
        *d_first++ = kInput.tempSubString(textPos);
      }
      break;
    }

    nextDelim = kInput.indexOf(kDelimiter, textPos);
    size_t uNextDelim = nextDelim; // std::string::npos / -1 handled differently

    // UnicodeString.tempSubstring changes any length < 0 (npos) to 0
    // However, it will change any positive number that runs off the string
    // to the maximum usable size (like std::string functions).

    if (uNextDelim == std::string::npos)
      uNextDelim = kInput.length();

    if (!omitEmptyStrings or (uNextDelim > textPos))
    {
      *d_first++ = kInput.tempSubString(textPos, uNextDelim - textPos);
    }
    textPos = uNextDelim + delimLen;
  } while (nextDelim != ::std::string::npos);
  return dest;
}

template<typename OutputIt>
OutputIt Unicode::SplitTo(OutputIt d_first,
    icu::UnicodeString uInput,
    const std::vector<icu::UnicodeString>& uDelimiters,
    size_t iMaxStrings /* = 0*/,
    const bool omitEmptyStrings /*= false*/)
{

  OutputIt dest = d_first;

  if (uInput.isEmpty())
    return dest;

  if (uDelimiters.empty())
  {
    *d_first++ = uInput;
    return dest;
  }
  icu::UnicodeString uDelimiter = uDelimiters[0];

  // TODO: Review this change in behavior

  // First, transform every occurrence of delimiters in the string to the first delimiter
  // Then, split using the first delimiter.

  for (size_t di = 1; di < uDelimiters.size(); di++)
  {
    icu::UnicodeString uNextDelimiter = uDelimiters[di];
    uInput.findAndReplace(uNextDelimiter, uDelimiter);
  }

  if (uInput.isEmpty())
  {
    if (!omitEmptyStrings)
      *d_first++ = uInput;
    return dest;
  }

  Unicode::SplitTo(d_first, uInput, uDelimiter, iMaxStrings, omitEmptyStrings);
  return dest;
}

std::vector<icu::UnicodeString> Unicode::SplitMulti(
    const std::vector<icu::UnicodeString>& input,
    const std::vector<icu::UnicodeString>& delimiters,
    size_t iMaxStrings)
{
  // For SplitMulti, returning empty strings does not make much sense since you can't correlate
  // what it relates to.

  // TODO: This implementation differs from the StringUtils version in that, like
  //       Split, it first reduces multiple delimiters to one by modifying the input strings.
  //       When iMaxStrings is exceeded we can't easily return the same partially
  //       processed input strings as before. Need to determine if this is a problem or not.
  //       A cursory examination suggests that it is not a problem for the current use cases.
  //       Need to document any changes. If old behavior is required, then revert, even
  //       if a tad slower.

  constexpr bool omitEmptyStrings = true;

  if (input.empty())
    return std::vector<icu::UnicodeString>();

  std::vector<icu::UnicodeString> results(input);

  if (delimiters.empty() || (iMaxStrings > 0 && iMaxStrings <= input.size()))
    return results;

  std::vector<icu::UnicodeString> strings1;
  std::vector<icu::UnicodeString> substrings = std::vector<icu::UnicodeString>();
  if (iMaxStrings == 0)
  {
    for (size_t di = 0; di < delimiters.size(); di++)
    {
      for (size_t i = 0; i < results.size(); i++)
      {
        substrings.clear();
        Unicode::SplitTo(std::back_inserter(substrings), results[i], delimiters[di], iMaxStrings,
            omitEmptyStrings);
        for (size_t j = 0; j < substrings.size(); j++)
          strings1.push_back(substrings[j]);
      }
      results = strings1;
      strings1.clear();
    }
    return results;
  }

  // Control the number of strings input is split into, keeping the original strings.
  // Note iMaxStrings > input.size()

  int64_t iNew = iMaxStrings - results.size();
  for (size_t di = 0; di < delimiters.size(); di++)
  {
    for (size_t i = 0; i < input.size(); i++)
    {
      if (iNew > 0)
      {
        substrings.clear();
        Unicode::SplitTo(std::back_inserter(substrings), results[i], delimiters[di], iNew + 1,
            omitEmptyStrings);
        iNew = iNew - substrings.size() + 1;
        for (size_t j = 0; j < substrings.size(); j++)
          strings1.push_back(substrings[j]);
      }
      else
        strings1.push_back(results[i]);
    }
    results = strings1;
    iNew = iMaxStrings - results.size();
    strings1.clear();
    if (iNew <= 0)
      break; // Stop trying any more delimiters
  }
  return results;
}

std::vector<std::string> Unicode::SplitMulti(const std::vector<std::string>& input,
    const std::vector<std::string>& delimiters,
    size_t iMaxStrings)
{
  if (input.empty())
    return std::vector<std::string>();

  std::vector<std::string> results;

  if (delimiters.empty() || (iMaxStrings > 0 && iMaxStrings <= input.size()))
  {
    results = std::vector<std::string>(input);
    return results;
  }

  std::vector<icu::UnicodeString> kInput = std::vector<icu::UnicodeString>();
  std::vector<icu::UnicodeString> kDelimiters = std::vector<icu::UnicodeString>();

  for (size_t i = 0; i < input.size(); i++)
  {
    icu::UnicodeString kI = Unicode::ToUnicodeString(input[i]);
    kInput.push_back(kI);
  }

  for (size_t i = 0; i < delimiters.size(); i++)
  {
    icu::UnicodeString kD = Unicode::ToUnicodeString(delimiters[i]);
    kDelimiters.push_back(kD);
  }

  std::vector<icu::UnicodeString> kResults = SplitMulti(kInput, kDelimiters, iMaxStrings);

  results = std::vector<std::string>();
  for (size_t i = 0; i < kResults.size(); i++)
  {
    // Assuming that the returned UChar string has no invalid code-units, other than the ones
    // marked invalid in the first place. Therefore, we can use the cheaper conversion to UTF8.

    std::u16string_view  UCharStr = std::u16string_view(kResults[i].getBuffer(), kResults[i].length());
    std::string tempStr = Unicode::UTF16ToUTF8(UCharStr);
    results.push_back(tempStr);
  }

  return results;
}

bool Unicode::Contains(const std::string_view& str, const std::vector<std::string>& keywords)
{

  // Moved code to here because it may need more than just
  // byte compare, but for the moment just do byte compare, as was
  // originally done.

  for (std::vector<std::string>::const_iterator it = keywords.begin(); it != keywords.end(); ++it)
  {
    if (str.find(*it) != str.npos)
      return true;
  }
  return false;
}


thread_local icu::Collator* myCollator = nullptr;
thread_local std::chrono::steady_clock::time_point myCollatorStart =
    std::chrono::steady_clock::now();

bool Unicode::InitializeCollator(std::locale locale, bool Normalize /* = false */)
{
  icu::Locale icuLocale = Unicode::GetICULocale(locale);
  return Unicode::InitializeCollator(icuLocale, Normalize);
}

bool Unicode::InitializeCollator(icu::Locale icuLocale, bool Normalize /* = false */)
{
  UErrorCode status = U_ZERO_ERROR;
  icu::UnicodeString dispName;
  if (myCollator != nullptr)
  {
    delete myCollator;
  }
  std::string localeId = Unicode::GetICULocaleId(icuLocale);

  myCollator = icu::Collator::createInstance(icuLocale, status);
  if (U_FAILURE(status))
  {
    icuLocale.getDisplayName(dispName);
    CLog::Log(LOGWARNING,
        "Unicode::InitializeCollator failed to create the collator for : \"{}\"\n",
        ToString(dispName));
    return false;
  }
  myCollatorStart = std::chrono::steady_clock::now();

  // Go with default Normalization (off). Some free normalization
  // is still performed. Even with it on, you should do some
  // extra normalization up-front to handle certain
  // locales/codepoints. See the documentation for UCOL_NORMALIZATION_MODE
  // for a hint.

  status = U_ZERO_ERROR;

  if (Normalize)
    myCollator->setAttribute(UCOL_NORMALIZATION_MODE, UCOL_ON, status);
  else
    myCollator->setAttribute(UCOL_NORMALIZATION_MODE, UCOL_OFF, status);
  if (U_FAILURE(status))
  {
    icuLocale.getDisplayName(dispName);
    CLog::Log(LOGWARNING,
        "Unicode::InitializeCollator failed to set normalization for the collator: \"{}\"\n",
        ToString(dispName));
    return false;
  }
  myCollator->setAttribute(UCOL_NUMERIC_COLLATION, UCOL_ON, status);
  if (U_FAILURE(status))
  {
    icuLocale.getDisplayName(dispName);
    CLog::Log(LOGWARNING, "Unicode::InitializeCollator failed to set numeric collation: \"{}\"\n",
        ToString(dispName));
    return false;
  }
  return true;
}

void Unicode::SortCompleted(int sortItems)
{
  // using namespace std::chrono;
  std::chrono::steady_clock::time_point myCollatorStop = std::chrono::steady_clock::now();
  int micros =
      std::chrono::duration_cast<std::chrono::microseconds>(myCollatorStop - myCollatorStart)
      .count();
  CLog::Log(LOGINFO, "Sort of {} records took {} µs\n", sortItems, micros);
}

/**
 * Experimental Collation function aimed at being equivalent to UnicodeUtils::AlphaNumericCompare
 * but using ICU. The ICU should be superior. Need to do performance tests.
 *
 * Note that it would be faster to pass either UTF-16 (the native ICU format) or
 * UTF-8 strings. Currently wstrings are converted to UTF-16. It is fairly easy to
 * change SortUtils (and friends) to pass UTF-8 (instead of first converting to wchar),
 * however it seems that the GUI requires the wstring for BIDI. Most likely ICU has a
 * solution for that. At some point should use UTF-8 to do a performance comparison. ICU
 * has put a lot of work into performance.
 *
 * Normally Normalization does not have to be a worry, however, if say, you mix Vietnamese or Arabic
 * movie titles in an English locale, then the Collator will by default not do normalization
 * (because of the English locale). However, you might want to turn it on. I suspect
 * that you could more-or-less auto-detect this. See
 * https://unicode-org.github.io/icu/userguide/collation/faq.html
 *
 */
int32_t Unicode::Collate(const std::wstring_view& left, const std::wstring_view& right)
{

  // TODO: Create UTF8 version of Collate for coll.h compareUTF8

  UErrorCode status = U_ZERO_ERROR;

  if (myCollator == nullptr)
  {
    CLog::Log(LOGWARNING, "Unicode::Collate Collator NOT configured\n");
    return INT_MIN;
  }

  // Stack based memory. A bit tedious. There must be a way to do this in a template,
  // or not so awful Macro.

  size_t leftBufferSize = Unicode::GetWcharToUTF16BufferSize(left.length(), 1);
  UChar leftBuffer[leftBufferSize];
  std::u16string_view lsv = Unicode::WStringToUTF16WithSub(left, leftBuffer, leftBufferSize);

  size_t rightBufferSize = Unicode::GetWcharToUTF16BufferSize(right.length(), 1);
  UChar rightBuffer[rightBufferSize];
  std::u16string_view rsv = Unicode::WStringToUTF16WithSub(right, rightBuffer, rightBufferSize);

  status = U_ZERO_ERROR;
  UCollationResult result;
  result = myCollator->compare(lsv.data(), lsv.length(), rsv.data(), rsv.length(), status);
  if (U_FAILURE(status))
  {
    CLog::Log(LOGWARNING, "Unicode::Collate Failed compare");
  }
  return result;
}
