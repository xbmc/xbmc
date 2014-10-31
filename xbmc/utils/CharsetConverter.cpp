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

#include "CharsetConverter.h"
#include "LangInfo.h"
#include "utils/Utf8Utils.h"
#include "utils/log.h"

#include <unicode/ucnv.h>
#include <unicode/urename.h>
#include <unicode/ubidi.h>
#include <unicode/uniset.h>
#include <unicode/normalizer2.h>
#include <memory.h>

#if !defined(TARGET_WINDOWS) && defined(HAVE_CONFIG_H)
  #include "config.h"
#endif

#define UTF8_CHARSET "UTF-8"
#define UTF16_CHARSET "UTF16_PlatformEndian"
#define UTF16LE_CHARSET "UTF-16LE"
#define UTF16BE_CHARSET "UTF-16BE"
#define UTF32_CHARSET "UTF32_PlatformEndian"
#define UTF32LE_CHARSET "UTF-32LE"
#define UTF32BE_CHARSET "UTF-32BE"

#if defined(TARGET_DARWIN)
#define WCHAR_CHARSET UTF32_CHARSET
#elif defined(TARGET_WINDOWS)
  #define WCHAR_CHARSET UTF16_CHARSET 
#ifdef NDEBUG
  #pragma comment(lib, "icuuc.lib")
#else
  #pragma comment(lib, "icuucd.lib")
#endif
#elif defined(TARGET_ANDROID)
#define WCHAR_CHARSET UTF32_CHARSET 
#else
#define WCHAR_CHARSET UTF16_CHARSET
#endif

//specify default normalization, on Darwin we want to compose
#if defined(TARGET_DARWIN)
  #define NORMALIZE 1
#else
  #define NORMALIZE 0
#endif

/**
 * \class CCharsetConverter::CInnerConverter
 *
 * We don't want to pollute header file with many additional includes and definitions, so put
 * here all stuff that require usage of types defined in this file or in additional headers
 */
class CCharsetConverter::CInnerConverter
{
private:
  /**
   * Helper function to perform BiDi flip and reverse hebrew/arabic text
   *
   * \param[in]  srcBuffer      array of UChar containing the string to process
   * \param[in]  srcLength      length in characters of srcBuffer
   * \param[out] dstBuffer      empty pointer to store result in
   *                            on success will contain the processed buffer.
   *                            it's up to the caller to delete buffer on any successful calls.
   * \param[out] dstLength      length in characters of processed string
   * \param[in]  bidiOptions    options for logical to visual processing
   *
   * \return true on success, false on any error
   * \sa CCharsetConverter::BiDiOptions
   */
  static bool BidiHelper(const UChar* srcBuffer, int32_t srcLength, 
                                 UChar** dstBuffer, int32_t& dstLength,
                                 const uint16_t bidiOptions);

  /**
  * Helper function to perform unicode normalization
  *
  * \param[in]  srcBuffer           array of UChar containing the string to process
  * \param[in]  srcLength           length in characters of srcBuffer
  * \param[out] dstBuffer           empty pointer to store result in
  *                                 on success will contain the processed buffer.
  *                                 it's up to the caller to delete buffer on any successful calls.
  * \param[out] dstLength           length in characters of processed string
  * \param[in]  normalizeOptions    options for logical to visual processing
  *
  * \return true on success, false on any error
  * \sa CCharsetConverter::NormalizationOptions
  */
  static bool NormalizationHelper(const UChar* srcBuffer, int32_t srcLength,
                                          UChar** dstBuffer, int32_t& dstLength,
                                          const uint16_t normalizeOptions);
public:

  /**
   * Converts between two encodings and optionally performs normalization and bidi conversion
   *
   * Input string is converted to UTF-16 and then normaliztions and
   * BiDi processing is performed before converting the result
   * to the specified output encoding
   *
   * \param[in]  sourceCharset        charset of source string
   * \param[in]  targetCharset        charset of destination string
   * \param[in]  strSource            source string to be processed
   * \param[out] strDest              output string after processing
   * \param[in]  bidiOptions          options for logical to visual processing
   * \param[in]  normalizeOptions     options for normalization
   * \param[in]  failOnBadString      determines if invalid byte sequences aborts processing
   *                                  or gets skipped
   *
   * \return true on success, false on any error
   * \sa CCharsetConverter::BiDiOptions
   * \sa CCharsetConverter::NormalizationOptions
   */
  template<class INPUT, class OUTPUT>
  static bool Convert(const std::string& sourceCharset, const std::string& targetCharset,
                                  const INPUT& strSource, OUTPUT& strDest,
                                  const uint16_t bidiOptions, const uint16_t normalizeOptions,
                                  const bool failOnBadString = false);

};


class UConverterGuard
{
  UConverter* converter;
public:
  UConverterGuard() : converter(NULL) {};
  UConverterGuard(UConverter* cnv) : converter(cnv) {};
  ~UConverterGuard() 
  {
    if (converter)
      ucnv_close(converter);
  }
  void Set(UConverter* cnv) { converter = cnv; }
  operator UConverter*() const
  {
    return this->converter;
  }
};

template<class INPUT, class OUTPUT>
bool CCharsetConverter::CInnerConverter::Convert(const std::string& sourceCharset, const std::string& targetCharset,
                                                            const INPUT& strSource, OUTPUT& strDest,
                                                            const uint16_t bidiOptions, const uint16_t normalizeOptions,
                                                            const bool failOnBadString)
{
  strDest.clear();
  if (strSource.empty())
    return true;

  UErrorCode err = U_ZERO_ERROR;
  UConverterGuard srcConv;
  UConverterGuard dstConv;

  int32_t srcLength = -1;
  int32_t srcLengthInBytes = -1;
  int32_t dstLength = -1;
  int32_t dstLengthInBytes = -1;
  int32_t resultLength = -1;

  const char* srcBuffer = NULL;
  char* dstBuffer = NULL;
  UChar* conversionBuffer = NULL;
  UChar* resultBuffer = NULL;

  //if any of the charsets are empty we fall back to the default
  //encoder
  if (sourceCharset.empty())
    srcConv.Set(ucnv_open(NULL, &err));
  else
    srcConv.Set(ucnv_open(sourceCharset.c_str(), &err));

  if (U_FAILURE(err))
    return false;

  if (targetCharset.empty())
    dstConv.Set(ucnv_open(NULL, &err));
  else
    dstConv.Set(ucnv_open(targetCharset.c_str(), &err));

  if (U_FAILURE(err))
    return false;

  if (failOnBadString)
  {
    ucnv_setToUCallBack(srcConv, UCNV_TO_U_CALLBACK_STOP, NULL, NULL, NULL, &err);
    ucnv_setFromUCallBack(dstConv, UCNV_FROM_U_CALLBACK_STOP, NULL, NULL, NULL, &err);
  }
  else
  {
    ucnv_setFromUCallBack(dstConv, UCNV_FROM_U_CALLBACK_SKIP, NULL, NULL, NULL, &err);
    ucnv_setToUCallBack(srcConv, UCNV_TO_U_CALLBACK_SKIP, NULL, NULL, NULL, &err);
  }

  if (U_FAILURE(err))
    return false;

  srcLength = strSource.length();
  srcLengthInBytes = strSource.length() * ucnv_getMinCharSize(srcConv);
  srcBuffer = reinterpret_cast<const char*>(strSource.c_str());

  //Point of no return, remember to free dstBuffer :)
  dstLengthInBytes = UCNV_GET_MAX_BYTES_FOR_STRING(srcLength, ucnv_getMaxCharSize(srcConv));
  dstLength = dstLengthInBytes / 2; //we know UChar is always 2 bytes
  conversionBuffer = new UChar[dstLength];

  do 
  {
    err = U_ZERO_ERROR;
    resultLength = ucnv_toUChars(srcConv, conversionBuffer, dstLength, srcBuffer, srcLengthInBytes, &err);

    if (err == U_BUFFER_OVERFLOW_ERROR)
    {
      delete[] conversionBuffer;
      dstLength = resultLength + 1;
      conversionBuffer = new UChar[dstLength];
    } 
    else if (U_FAILURE(err))
  {
      delete[] conversionBuffer;
      return false;
  }
  } while (err == U_BUFFER_OVERFLOW_ERROR);
  
#if U_ICU_VERSION_MAJOR_NUM > 48
  if (normalizeOptions != NO_NORMALIZATION)
  {
    if (CInnerConverter::NormalizationHelper(conversionBuffer, resultLength, &resultBuffer, resultLength, normalizeOptions))
  {
      //switch the buffers to avoid ifs further down
      delete[] conversionBuffer;
      conversionBuffer = resultBuffer;
      resultBuffer = NULL;
    }
    else
    {
      delete[] conversionBuffer;
      return false;
    }
  }
#endif
  if (bidiOptions != NO_BIDI)
  {
    if (CInnerConverter::BidiHelper(conversionBuffer, resultLength, &resultBuffer, resultLength, bidiOptions))
  {
      //switch the buffers to avoid ifs further down
      delete[] conversionBuffer;
      conversionBuffer = resultBuffer;
      resultBuffer = NULL;
    }
    else
    {
      delete[] conversionBuffer;
      return false;
    }
  }

  dstLengthInBytes = UCNV_GET_MAX_BYTES_FOR_STRING(resultLength, ucnv_getMaxCharSize(dstConv));
  dstBuffer = new char[dstLengthInBytes];

  resultLength = ucnv_fromUChars(dstConv, dstBuffer, dstLengthInBytes, conversionBuffer, resultLength, &err);

  if (U_FAILURE(err))
  {
    delete[] conversionBuffer;
    delete[] resultBuffer;
    delete[] dstBuffer;
    return false;
  }

  resultLength /= ucnv_getMinCharSize(dstConv);

  if (U_SUCCESS(err))
  {
    uint16_t bom = static_cast<uint16_t>(dstBuffer[0]);
    //check for a utf-16 or utf-32 bom
    if (bom == 65535 || bom == 65279)
      strDest.assign(reinterpret_cast<typename OUTPUT::value_type *>(dstBuffer + 2), resultLength - 1);
    else
      strDest.assign(reinterpret_cast<typename OUTPUT::value_type *>(dstBuffer), resultLength);
  }

  delete[] conversionBuffer;
  delete[] resultBuffer;
  delete[] dstBuffer;

  return true;
}

bool CCharsetConverter::CInnerConverter::BidiHelper(const UChar* srcBuffer, int32_t srcLength,
                                                            UChar** dstBuffer, int32_t& dstLength,
                                                            const uint16_t bidiOptions)
{
  bool result = false;
  int32_t outputLength = -1;
  int32_t requiredLength = -1;
  UErrorCode err = U_ZERO_ERROR;
  UBiDiLevel level = 0;
  UBiDi* bidiConv = ubidi_open();
  
  //ubidi_setPara changes the srcBuffer pointer so make it const
  //and get a local copy of it
  UChar* inputBuffer = const_cast<UChar*>(srcBuffer);
  UChar* outputBuffer = NULL;

  if (bidiOptions & LTR)
    level = UBIDI_DEFAULT_LTR;
  else if (bidiOptions & RTL)
    level = UBIDI_DEFAULT_RTL;
  else
    level = UBIDI_DEFAULT_LTR;

  ubidi_setPara(bidiConv, inputBuffer, srcLength, level, NULL, &err);

  if (U_SUCCESS(err))
  {
    outputLength = ubidi_getProcessedLength(bidiConv) + 1; //allow for null terminator
    outputBuffer = new UChar[outputLength];

    do
  {
      uint16_t options = UBIDI_DO_MIRRORING;
      if (bidiOptions & REMOVE_CONTROLS)
        options |= UBIDI_REMOVE_BIDI_CONTROLS;
      if (bidiOptions & WRITE_REVERSE)
        options |= UBIDI_OUTPUT_REVERSE;

      requiredLength = ubidi_writeReordered(bidiConv, outputBuffer, outputLength, options, &err);

      if (U_SUCCESS(err))
        {
        outputLength = requiredLength;
        result = true;
          break;
        }

      if (err == U_BUFFER_OVERFLOW_ERROR)
      {
        delete[] outputBuffer;
        outputBuffer = new UChar[requiredLength];
        outputLength = requiredLength;

        //make sure our err is reset, some icu functions fail if it contains
        //a previous bad result
        err = U_ZERO_ERROR;
      }

      //We can't do much for other failures than buffer overflow
      //clean up and bail out
      if (U_FAILURE(err))
      {
        delete[] outputBuffer;
    break;
  }
    } while (1);

    if (result)
  {
      *dstBuffer = outputBuffer;
      dstLength = outputLength;
    }
  }

  ubidi_close(bidiConv);
  
  //we don't delete outBuffer, it's returned to caller and
  //caller is responsible for freeing it
  return result;
}

bool CCharsetConverter::CInnerConverter::NormalizationHelper(const UChar* srcBuffer, int32_t srcLength,
                                                             UChar** dstBuffer, int32_t& dstLength,
                                                             const uint16_t normalizeOptions)
{
#if U_ICU_VERSION_MAJOR_NUM > 48
  UErrorCode err = U_ZERO_ERROR;
  UnicodeString src(srcBuffer, srcLength);
  UnicodeString dst;

  if (normalizeOptions & COMPOSE)
  {
    //this should not be deleted, it's managed by icu
    const Normalizer2* norm = Normalizer2::getNFCInstance(err);
    if (U_FAILURE(err))
      return false;

    norm->normalize(src, dst, err);
    if (U_FAILURE(err))
      return false;
  }
  else if (normalizeOptions & DECOMPOSE)
  {
    //this should not be deleted, it's managed by icu
    const Normalizer2* norm = Normalizer2::getNFDInstance(err);
    if (U_FAILURE(err))
      return false;

    norm->normalize(src, dst, err);
    if (U_FAILURE(err))
      return false;
    }
  else if (normalizeOptions & DECOMPOSE_MAC)
  {
    //https://developer.apple.com/library/mac/qa/qa1173/_index.html
    //which U + 2000 through U + 2FFF, U + F900 through U + FAFF, and U + 2F800 through U + 2FAFF
    //are not decomposed(this avoids problems with round trip conversions from old Mac text encodings).
    UnicodeString usetString("[[^\\u2000-\\u2fff][^\\uf900-\\ufaff][^\\u2f800-\\u2faff]]");
    UnicodeSet uSet(usetString, err);

    if (U_FAILURE(err))
      return false;

    //this should not be deleted, it's managed by icu
    const Normalizer2* nfd = Normalizer2::getNFDInstance(err);

    if (U_FAILURE(err))
      return false;

    FilteredNormalizer2 filteredNfd(*nfd, uSet);

    filteredNfd.normalize(src, dst, err);

    if (U_FAILURE(err))
      return false;
  }

  dstLength = dst.length();
  *dstBuffer = new UChar[dstLength];
  
  memcpy(static_cast<void*>(*dstBuffer), static_cast<const void*>(dst.getTerminatedBuffer()), dstLength * 2);
#endif
  return true;
}

bool CCharsetConverter::Utf8ToUtf32(const std::string& utf8StringSrc, std::u32string& utf32StringDst)
{
  return CInnerConverter::Convert(UTF8_CHARSET, UTF32_CHARSET, utf8StringSrc, utf32StringDst,
                                  NO_BIDI, NO_NORMALIZATION, false);
}

std::u32string CCharsetConverter::Utf8ToUtf32(const std::string& utf8StringSrc)
{
  std::u32string converted;
  Utf8ToUtf32(utf8StringSrc, converted);
  return converted;
}

bool CCharsetConverter::Utf8ToUtf16(const std::string& utf8StringSrc, std::u16string& utf16StringDst)
{
  return CInnerConverter::Convert(UTF8_CHARSET, UTF16_CHARSET, utf8StringSrc, utf16StringDst,
                                  NO_BIDI, NO_NORMALIZATION, false);
}

bool CCharsetConverter::TryUtf8ToUtf16(const std::string & utf8StringSrc, std::u16string & utf16StringDst)
{
  return CInnerConverter::Convert(UTF8_CHARSET, UTF16_CHARSET, utf8StringSrc, utf16StringDst,
                                  NO_BIDI, NO_NORMALIZATION, true);
}

std::u16string CCharsetConverter::Utf8ToUtf16(const std::string& utf8StringSrc)
{
  std::u16string converted;
  Utf8ToUtf16(utf8StringSrc, converted);
  return converted;
}

bool CCharsetConverter::Utf8ToUtf16BE(const std::string& utf8StringSrc, std::u16string& utf16StringDst)
{
  return CInnerConverter::Convert(UTF8_CHARSET, UTF16BE_CHARSET, utf8StringSrc, utf16StringDst,
                                  NO_BIDI, NO_NORMALIZATION, false);
}

bool CCharsetConverter::Utf8ToUtf16LE(const std::string& utf8StringSrc, std::u16string& utf16StringDst)
{
  return CInnerConverter::Convert(UTF8_CHARSET, UTF16LE_CHARSET, utf8StringSrc, utf16StringDst,
                                  NO_BIDI, NO_NORMALIZATION, false);
}

bool CCharsetConverter::Utf8ToUtf32LogicalToVisual(const std::string& utf8StringSrc, std::u32string& utf32StringDst,
                                                   uint16_t bidiOptions /* = LTR | REMOVE_CONTROLS */)
{
  return CInnerConverter::Convert(UTF8_CHARSET, UTF32_CHARSET, utf8StringSrc, utf32StringDst,
                                                bidiOptions, NO_NORMALIZATION, false);
}

bool CCharsetConverter::Utf32ToUtf8(const std::u32string& utf32StringSrc, std::string& utf8StringDst)
{
  return CInnerConverter::Convert(UTF32_CHARSET, UTF8_CHARSET, utf32StringSrc, utf8StringDst,
                                  NO_BIDI, NO_NORMALIZATION, false);
}

std::string CCharsetConverter::Utf32ToUtf8(const std::u32string& utf32StringSrc)
{
  std::string converted;
  Utf32ToUtf8(utf32StringSrc, converted);
  return converted;
}

bool CCharsetConverter::Utf8ToWLogicalToVisual(const std::string& utf8StringSrc, std::wstring& wStringDst,
                                               uint16_t bidiOptions /* = LTR | REMOVE_CONTROLS */)
{
  return CInnerConverter::Convert(UTF8_CHARSET, WCHAR_CHARSET, utf8StringSrc, wStringDst,
                                              bidiOptions, NO_NORMALIZATION | NORMALIZE, false);
}

bool CCharsetConverter::Utf8ToW(const std::string& utf8StringSrc, std::wstring& wStringDst)
{
  return CInnerConverter::Convert(UTF8_CHARSET, WCHAR_CHARSET, utf8StringSrc, wStringDst,
                                  NO_BIDI, NO_NORMALIZATION | NORMALIZE, false);
}

bool CCharsetConverter::TryUtf8ToW(const std::string & utf8StringSrc, std::wstring & wStringDst)
{
  return CInnerConverter::Convert(UTF8_CHARSET, WCHAR_CHARSET, utf8StringSrc, wStringDst,
                                  NO_BIDI, NO_NORMALIZATION | NORMALIZE, true);
}

bool CCharsetConverter::TryWToUtf8(const std::wstring & wStringSrc, std::string & utf8StringDst)
{
  return CInnerConverter::Convert(WCHAR_CHARSET, UTF8_CHARSET, wStringSrc, utf8StringDst,
                                  NO_BIDI, NO_NORMALIZATION, true);
}

bool CCharsetConverter::SubtitleCharsetToUtf8(const std::string& stringSrc, std::string& utf8StringDst)
{
  std::string subtitleCharset = g_langInfo.GetSubtitleCharSet();
  return CInnerConverter::Convert(subtitleCharset, UTF8_CHARSET, stringSrc, utf8StringDst,
                                  NO_BIDI, NO_NORMALIZATION, false);
}

bool CCharsetConverter::Utf8ToStringCharset(const std::string& utf8StringSrc, std::string& stringDst)
{
  std::string guiCharset = g_langInfo.GetGuiCharSet();
  return CInnerConverter::Convert(UTF8_CHARSET, guiCharset, utf8StringSrc, stringDst,
                                  NO_BIDI, NO_NORMALIZATION, false);
}

bool CCharsetConverter::Utf8ToStringCharset(std::string& stringSrcDst)
{
  std::string strSrc(stringSrcDst);
  return Utf8ToStringCharset(strSrc, stringSrcDst);
}

bool CCharsetConverter::ToUtf8(const std::string& strSourceCharset, const std::string& stringSrc, std::string& utf8StringDst)
{
  return CInnerConverter::Convert(strSourceCharset, UTF8_CHARSET, stringSrc, utf8StringDst,
                                  NO_BIDI, NO_NORMALIZATION, false);
}

bool CCharsetConverter::TryToUtf8(const std::string& strSourceCharset, const std::string& stringSrc, std::string& utf8StringDst)
{
  return CInnerConverter::Convert(strSourceCharset, UTF8_CHARSET, stringSrc, utf8StringDst,
                                  NO_BIDI, NO_NORMALIZATION, false);
}

bool CCharsetConverter::Utf8To(const std::string& strDestCharset, const std::string& utf8StringSrc, std::string& stringDst)
{
  return CInnerConverter::Convert(UTF8_CHARSET, strDestCharset, utf8StringSrc, stringDst,
                                  NO_BIDI, NO_NORMALIZATION, false);
}

bool CCharsetConverter::Utf8To(const std::string& strDestCharset, const std::string& utf8StringSrc, std::u16string& utf16StringDst)
{
  return CInnerConverter::Convert(UTF8_CHARSET, strDestCharset, utf8StringSrc, utf16StringDst,
                                  NO_BIDI, NO_NORMALIZATION, false);
}

bool CCharsetConverter::Utf8To(const std::string& strDestCharset, const std::string& utf8StringSrc, std::u32string& utf32StringDst)
{
  return CInnerConverter::Convert(UTF8_CHARSET, strDestCharset, utf8StringSrc, utf32StringDst,
                                  NO_BIDI, NO_NORMALIZATION, false);
}

bool CCharsetConverter::UnknownToUtf8(std::string& stringSrcDst)
{
  std::string source(stringSrcDst);
  return UnknownToUtf8(source, stringSrcDst);
}

bool CCharsetConverter::UnknownToUtf8(const std::string& stringSrc, std::string& utf8StringDst)
{
  // checks whether it's utf8 already, and if not converts using the sourceCharset if given, else the string charset
  if (CUtf8Utils::isValidUtf8(stringSrc))
  {
    utf8StringDst = stringSrc;
    return true;
  }
  std::string guiCharset = g_langInfo.GetGuiCharSet();
  return CInnerConverter::Convert(guiCharset, UTF8_CHARSET, stringSrc, utf8StringDst,
                                  NO_BIDI, NO_NORMALIZATION, false);
}

bool CCharsetConverter::WToUtf8(const std::wstring& wStringSrc, std::string& utf8StringDst)
{
  return CInnerConverter::Convert(WCHAR_CHARSET, UTF8_CHARSET, wStringSrc, utf8StringDst,
                                  NO_BIDI, NO_NORMALIZATION, false);
}

bool CCharsetConverter::Utf16BEToUtf8(const std::u16string& utf16StringSrc, std::string& utf8StringDst)
{
  return CInnerConverter::Convert(UTF16BE_CHARSET, UTF8_CHARSET, utf16StringSrc, utf8StringDst,
                                  NO_BIDI, NO_NORMALIZATION, false);
}

bool CCharsetConverter::Utf16LEToUtf8(const std::u16string& utf16StringSrc, std::string& utf8StringDst)
{
  return CInnerConverter::Convert(UTF16LE_CHARSET, UTF8_CHARSET, utf16StringSrc, utf8StringDst,
                                  NO_BIDI, NO_NORMALIZATION, false);
}

bool CCharsetConverter::Utf16ToUtf8(const std::u16string& utf16StringSrc, std::string& utf8StringDst)
{
  return CInnerConverter::Convert(UTF16_CHARSET, UTF8_CHARSET, utf16StringSrc, utf8StringDst,
                                  NO_BIDI, NO_NORMALIZATION, false);
}

bool CCharsetConverter::TryUtf16ToUtf8(const std::u16string utf16StringSrc, std::string & utf8StringDst)
{
  return CInnerConverter::Convert(UTF16_CHARSET, UTF8_CHARSET, utf16StringSrc, utf8StringDst,
                                  NO_BIDI, NO_NORMALIZATION, true);
}

bool CCharsetConverter::Ucs2ToUtf8(const std::u16string& ucs2StringSrc, std::string& utf8StringDst)
{
  //UCS2 is technically only BE and ICU includes no LE version converter
  //Use UTF-16 converter since the only difference are lead bytes that are
  //illegal to use in UCS2 so should not cause any issues
  return CInnerConverter::Convert(UTF16LE_CHARSET, UTF8_CHARSET, ucs2StringSrc, utf8StringDst,
                                  NO_BIDI, NO_NORMALIZATION, false);
}


bool CCharsetConverter::Utf8ToSystem(std::string& stringSrcDst)
{
  std::string strSrc(stringSrcDst);
  return CInnerConverter::Convert(UTF8_CHARSET, "", strSrc, stringSrcDst,
                                  NO_BIDI, NO_NORMALIZATION, false);
}

bool CCharsetConverter::SystemToUtf8(const std::string& sysStringSrc, std::string& utf8StringDst)
{
  return CInnerConverter::Convert("", UTF8_CHARSET, sysStringSrc, utf8StringDst,
                                  NO_BIDI, NO_NORMALIZATION, false);
}

bool CCharsetConverter::TrySystemToUtf8(const std::string& sysStringSrc, std::string& utf8StringDst)
{
  return CInnerConverter::Convert("", UTF8_CHARSET, sysStringSrc, utf8StringDst,
                                  NO_BIDI, NO_NORMALIZATION, true);
}

bool CCharsetConverter::LogicalToVisualBiDi(const std::string& utf8StringSrc, std::string& utf8StringDst,
                                            uint16_t bidiOptions /* = LTR | REMOVE_CONTROLS */)
{
  return CInnerConverter::Convert(UTF8_CHARSET, UTF8_CHARSET, utf8StringSrc, utf8StringDst,
                                  bidiOptions, NO_NORMALIZATION, false);
}

bool CCharsetConverter::LogicalToVisualBiDi(const std::u16string& utf16StringSrc, std::u16string& utf16StringDst,
                                            uint16_t bidiOptions /* = LTR | REMOVE_CONTROLS */)
{
  return CInnerConverter::Convert(UTF16_CHARSET, UTF16_CHARSET, utf16StringSrc, utf16StringDst,
                                  bidiOptions, NO_NORMALIZATION, false);
}

bool CCharsetConverter::LogicalToVisualBiDi(const std::wstring& wStringSrc, std::wstring& wStringDst,
                                            uint16_t bidiOptions /* = LTR | REMOVE_CONTROLS */)
{
  return CInnerConverter::Convert(WCHAR_CHARSET, WCHAR_CHARSET, wStringSrc, wStringDst,
                                  bidiOptions, NO_NORMALIZATION, false);
}

bool CCharsetConverter::LogicalToVisualBiDi(const std::u32string& utf32StringSrc, std::u32string& utf32StringDst,
                                            uint16_t bidiOptions /* = LTR | REMOVE_CONTROLS */)
{
  return CInnerConverter::Convert(UTF32_CHARSET, UTF32_CHARSET, utf32StringSrc, utf32StringDst,
                                  bidiOptions, NO_NORMALIZATION, false);
}

bool CCharsetConverter::ReverseRTL(const std::string& utf8StringSrc, std::string& utf8StringDst)
{
  return CInnerConverter::Convert(UTF8_CHARSET, UTF8_CHARSET, utf8StringSrc, utf8StringDst, 
                                  RTL | WRITE_REVERSE, NO_NORMALIZATION, false);
}

bool CCharsetConverter::Utf8ToSystemSafe(const std::string& stringSrc, std::string& stringDst)
{
  stringDst = stringSrc;

  return true;
}

bool CCharsetConverter::Utf8ToWSystemSafe(const std::string& stringSrc, std::wstring& stringDst)
{
  //W should be win32 only and requires no special handling, make sure we fail on bad chars
  //to avoid any weird behavior
  return CInnerConverter::Convert(UTF8_CHARSET, WCHAR_CHARSET, stringSrc, stringDst,
                                  NO_BIDI, NO_NORMALIZATION, true);
}

bool CCharsetConverter::WToUtf8SystemSafe(const std::wstring& wStringSrc, std::string& utf8StringDst)
{
  return CInnerConverter::Convert(WCHAR_CHARSET, UTF8_CHARSET, wStringSrc, utf8StringDst,
                                  NO_BIDI, NO_NORMALIZATION, true);
}

bool CCharsetConverter::Normalize(const std::string& source, std::string& destination, uint16_t options)
{
  return CInnerConverter::Convert(UTF8_CHARSET, UTF8_CHARSET, source, destination, NO_BIDI, options, false);
}

bool CCharsetConverter::Normalize(const std::u16string & source, std::u16string & destination, uint16_t options)
{
  return CInnerConverter::Convert(UTF16_CHARSET, UTF16_CHARSET, source, destination, NO_BIDI, options, false);
}

bool CCharsetConverter::Normalize(const std::u32string & source, std::u32string & destination, uint16_t options)
{
  return CInnerConverter::Convert(UTF32_CHARSET, UTF32_CHARSET, source, destination, NO_BIDI, options, false);
}

bool CCharsetConverter::Normalize(const std::wstring & source, std::wstring & destination, uint16_t options)
{
  return CInnerConverter::Convert(WCHAR_CHARSET, WCHAR_CHARSET, source, destination, NO_BIDI, options, false);
}
