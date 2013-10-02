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
#include "Util.h"
#include "utils/StringUtils.h"
#include <fribidi/fribidi.h>
#include "LangInfo.h"
#include "guilib/LocalizeStrings.h"
#include "settings/Setting.h"
#include "threads/SingleLock.h"
#include "log.h"

#include <errno.h>
#include <iconv.h>

#if !defined(TARGET_WINDOWS) && defined(HAVE_CONFIG_H)
  #include "config.h"
#endif

#ifdef WORDS_BIGENDIAN
  #define ENDIAN_SUFFIX "BE"
#else
  #define ENDIAN_SUFFIX "LE"
#endif

#if defined(TARGET_DARWIN)
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
  #pragma comment(lib, "libfribidi.lib")
  #pragma comment(lib, "libiconv.lib")
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


static iconv_t m_iconvUtf8ToUtf32                = (iconv_t)-1;
static iconv_t m_iconvUtf32ToUtf8                = (iconv_t)-1;
static iconv_t m_iconvUtf32ToW                   = (iconv_t)-1;
static iconv_t m_iconvWToUtf32                   = (iconv_t)-1;
static iconv_t m_iconvSubtitleCharsetToW         = (iconv_t)-1;
static iconv_t m_iconvUtf8ToStringCharset        = (iconv_t)-1;
static iconv_t m_iconvStringCharsetToUtf8        = (iconv_t)-1;
static iconv_t m_iconvUtf32ToStringCharset       = (iconv_t)-1;
static iconv_t m_iconvWtoUtf8                    = (iconv_t)-1;
static iconv_t m_iconvUtf16LEtoW                 = (iconv_t)-1;
static iconv_t m_iconvUtf16BEtoUtf8              = (iconv_t)-1;
static iconv_t m_iconvUtf16LEtoUtf8              = (iconv_t)-1;
static iconv_t m_iconvUtf8toW                    = (iconv_t)-1;
static iconv_t m_iconvUcs2CharsetToUtf8          = (iconv_t)-1;

#if defined(FRIBIDI_CHAR_SET_NOT_FOUND)
static FriBidiCharSet m_stringFribidiCharset     = FRIBIDI_CHAR_SET_NOT_FOUND;
#define FRIBIDI_UTF8 FRIBIDI_CHAR_SET_UTF8
#define FRIBIDI_NOTFOUND FRIBIDI_CHAR_SET_NOT_FOUND
#else /* compatibility to older version */
static FriBidiCharSet m_stringFribidiCharset     = FRIBIDI_CHARSET_NOT_FOUND;
#define FRIBIDI_UTF8 FRIBIDI_CHARSET_UTF8
#define FRIBIDI_NOTFOUND FRIBIDI_CHARSET_NOT_FOUND
#endif

static CCriticalSection            m_critSection;

static struct SFribidMapping
{
  FriBidiCharSet name;
  const char*    charset;
} g_fribidi[] = {
#if defined(FRIBIDI_CHAR_SET_NOT_FOUND)
  { FRIBIDI_CHAR_SET_ISO8859_6, "ISO-8859-6"   }
, { FRIBIDI_CHAR_SET_ISO8859_8, "ISO-8859-8"   }
, { FRIBIDI_CHAR_SET_CP1255   , "CP1255"       }
, { FRIBIDI_CHAR_SET_CP1255   , "Windows-1255" }
, { FRIBIDI_CHAR_SET_CP1256   , "CP1256"       }
, { FRIBIDI_CHAR_SET_CP1256   , "Windows-1256" }
, { FRIBIDI_CHAR_SET_NOT_FOUND, NULL           }
#else /* compatibility to older version */
  { FRIBIDI_CHARSET_ISO8859_6, "ISO-8859-6"   }
, { FRIBIDI_CHARSET_ISO8859_8, "ISO-8859-8"   }
, { FRIBIDI_CHARSET_CP1255   , "CP1255"       }
, { FRIBIDI_CHARSET_CP1255   , "Windows-1255" }
, { FRIBIDI_CHARSET_CP1256   , "CP1256"       }
, { FRIBIDI_CHARSET_CP1256   , "Windows-1256" }
, { FRIBIDI_CHARSET_NOT_FOUND, NULL           }
#endif
};

static struct SCharsetMapping
{
  const char* charset;
  const char* caption;
} g_charsets[] = {
   { "ISO-8859-1", "Western Europe (ISO)" }
 , { "ISO-8859-2", "Central Europe (ISO)" }
 , { "ISO-8859-3", "South Europe (ISO)"   }
 , { "ISO-8859-4", "Baltic (ISO)"         }
 , { "ISO-8859-5", "Cyrillic (ISO)"       }
 , { "ISO-8859-6", "Arabic (ISO)"         }
 , { "ISO-8859-7", "Greek (ISO)"          }
 , { "ISO-8859-8", "Hebrew (ISO)"         }
 , { "ISO-8859-9", "Turkish (ISO)"        }
 , { "CP1250"    , "Central Europe (Windows)" }
 , { "CP1251"    , "Cyrillic (Windows)"       }
 , { "CP1252"    , "Western Europe (Windows)" }
 , { "CP1253"    , "Greek (Windows)"          }
 , { "CP1254"    , "Turkish (Windows)"        }
 , { "CP1255"    , "Hebrew (Windows)"         }
 , { "CP1256"    , "Arabic (Windows)"         }
 , { "CP1257"    , "Baltic (Windows)"         }
 , { "CP1258"    , "Vietnamesse (Windows)"    }
 , { "CP874"     , "Thai (Windows)"           }
 , { "BIG5"      , "Chinese Traditional (Big5)" }
 , { "GBK"       , "Chinese Simplified (GBK)" }
 , { "SHIFT_JIS" , "Japanese (Shift-JIS)"     }
 , { "CP949"     , "Korean"                   }
 , { "BIG5-HKSCS", "Hong Kong (Big5-HKSCS)"   }
 , { NULL        , NULL                       }
};


/* single symbol sizes in chars */
const int CCharsetConverter::m_Utf8CharMinSize = 1;
const int CCharsetConverter::m_Utf8CharMaxSize = 6;

#define ICONV_PREPARE(iconv) iconv=(iconv_t)-1
#define ICONV_SAFE_CLOSE(iconv) if (iconv!=(iconv_t)-1) { iconv_close(iconv); iconv=(iconv_t)-1; }

size_t iconv_const (void* cd, const char** inbuf, size_t* inbytesleft,
                    char** outbuf, size_t* outbytesleft)
{
    struct iconv_param_adapter {
        iconv_param_adapter(const char**p) : p(p) {}
        iconv_param_adapter(char**p) : p((const char**)p) {}
        operator char**() const
        {
            return(char**)p;
        }
        operator const char**() const
        {
            return(const char**)p;
        }
        const char** p;
    };

    return iconv((iconv_t)cd, iconv_param_adapter(inbuf), inbytesleft, outbuf, outbytesleft);
}

template<class INPUT,class OUTPUT>
static bool convert(iconv_t& type, int multiplier, const std::string& strFromCharset, const std::string& strToCharset, const INPUT& strSource, OUTPUT& strDest, bool failOnInvalidChar = false)
{
  strDest.clear();

  if (type == (iconv_t)-1)
  {
    type = iconv_open(strToCharset.c_str(), strFromCharset.c_str());
    if (type == (iconv_t)-1) //iconv_open failed
    {
      CLog::Log(LOGERROR, "%s iconv_open() failed from %s to %s, errno=%d(%s)",
                __FUNCTION__, strFromCharset.c_str(), strToCharset.c_str(), errno, strerror(errno));
      return false;
    }
  }

  if (strSource.empty())
    return true; //empty strings are easy

  //input buffer for iconv() is the buffer from strSource
  size_t      inBufSize  = (strSource.length() + 1) * sizeof(typename INPUT::value_type);
  const char* inBuf      = (const char*)strSource.c_str();

  //allocate output buffer for iconv()
  size_t      outBufSize = (strSource.length() + 1) * sizeof(typename OUTPUT::value_type) * multiplier;
  char*       outBuf     = (char*)malloc(outBufSize);
  if (outBuf == NULL)
  {
      CLog::Log(LOGSEVERE, "%s: malloc failed", __FUNCTION__);
      return false;
  }

  size_t      inBytesAvail  = inBufSize;  //how many bytes iconv() can read
  size_t      outBytesAvail = outBufSize; //how many bytes iconv() can write
  const char* inBufStart    = inBuf;      //where in our input buffer iconv() should start reading
  char*       outBufStart   = outBuf;     //where in out output buffer iconv() should start writing

  size_t returnV;
  while(1)
  {
    //iconv() will update inBufStart, inBytesAvail, outBufStart and outBytesAvail
    returnV = iconv_const(type, &inBufStart, &inBytesAvail, &outBufStart, &outBytesAvail);

    if (returnV == (size_t)-1)
    {
      if (errno == E2BIG) //output buffer is not big enough
      {
        //save where iconv() ended converting, realloc might make outBufStart invalid
        size_t bytesConverted = outBufSize - outBytesAvail;

        //make buffer twice as big
        outBufSize   *= 2;
        char* newBuf  = (char*)realloc(outBuf, outBufSize);
        if (!newBuf)
        {
          CLog::Log(LOGSEVERE, "%s realloc failed with errno=%d(%s)",
                    __FUNCTION__, errno, strerror(errno));
          break;
        }
        outBuf = newBuf;

        //update the buffer pointer and counter
        outBufStart   = outBuf + bytesConverted;
        outBytesAvail = outBufSize - bytesConverted;

        //continue in the loop and convert the rest
        continue;
      }
      else if (errno == EILSEQ) //An invalid multibyte sequence has been encountered in the input
      {
        if (failOnInvalidChar)
          break;

        //skip invalid byte
        inBufStart++;
        inBytesAvail--;
        //continue in the loop and convert the rest
        continue;
      }
      else if (errno == EINVAL) /* Invalid sequence at the end of input buffer */
      {
        if (!failOnInvalidChar)
          returnV = 0; /* reset error status to use converted part */

        break;
      }
      else //iconv() had some other error
      {
        CLog::Log(LOGERROR, "%s iconv() failed from %s to %s, errno=%d(%s)",
                  __FUNCTION__, strFromCharset.c_str(), strToCharset.c_str(), errno, strerror(errno));
      }
    }
    break;
  }

  //complete the conversion (reset buffers), otherwise the current data will prefix the data on the next call
  if (iconv_const(type, NULL, NULL, &outBufStart, &outBytesAvail) == (size_t)-1)
    CLog::Log(LOGERROR, "%s failed cleanup errno=%d(%s)", __FUNCTION__, errno, strerror(errno));

  if (returnV == (size_t)-1)
  {
    free(outBuf);
    return false;
  }
  //we're done

  const typename OUTPUT::size_type sizeInChars = (typename OUTPUT::size_type) (outBufSize - outBytesAvail) / sizeof(typename OUTPUT::value_type);
  typename OUTPUT::const_pointer strPtr = (typename OUTPUT::const_pointer) outBuf;
  /* Make sure that all buffer is assigned and string is stopped at end of buffer */
  if (strPtr[sizeInChars-1] == 0)
    strDest.assign(strPtr, sizeInChars-1);
  else
    strDest.assign(strPtr, sizeInChars);

  free(outBuf);

  return true;
}

using namespace std;

static bool logicalToVisualBiDi(const std::string& stringSrc, std::string& stringDst, FriBidiCharSet fribidiCharset, FriBidiCharType base = FRIBIDI_TYPE_LTR, bool* bWasFlipped =NULL)
{
  stringDst.clear();
  vector<std::string> lines = StringUtils::Split(stringSrc, "\n");

  if (bWasFlipped)
    *bWasFlipped = false;

  // libfribidi is not threadsafe, so make sure we make it so
  CSingleLock lock(m_critSection);

  const size_t numLines = lines.size();
  for (size_t i = 0; i < numLines; i++)
  {
    int sourceLen = lines[i].length();
    if (sourceLen == 0)
      continue;

    // Convert from the selected charset to Unicode
    FriBidiChar* logical = (FriBidiChar*) malloc((sourceLen + 1) * sizeof(FriBidiChar));
    if (logical == NULL)
    {
      CLog::Log(LOGSEVERE, "%s: can't allocate memory", __FUNCTION__);
      return false;
    }
    int len = fribidi_charset_to_unicode(fribidiCharset, (char*) lines[i].c_str(), sourceLen, logical);

    FriBidiChar* visual = (FriBidiChar*) malloc((len + 1) * sizeof(FriBidiChar));
    FriBidiLevel* levels = (FriBidiLevel*) malloc((len + 1) * sizeof(FriBidiLevel));
    if (levels == NULL || visual == NULL)
    {
      free(logical);
      free(visual);
      free(levels);
      CLog::Log(LOGSEVERE, "%s: can't allocate memory", __FUNCTION__);
      return false;
    }

    if (fribidi_log2vis(logical, len, &base, visual, NULL, NULL, levels))
    {
      // Removes bidirectional marks
      len = fribidi_remove_bidi_marks(visual, len, NULL, NULL, NULL);

      // Apperently a string can get longer during this transformation
      // so make sure we allocate the maximum possible character utf8
      // can generate atleast, should cover all bases
      char* result = new char[len*4];

      // Convert back from Unicode to the charset
      int len2 = fribidi_unicode_to_charset(fribidiCharset, visual, len, result);
      assert(len2 <= len*4);
      stringDst += result;
      delete[] result;

      // Check whether the string was flipped if one of the embedding levels is greater than 0
      if (bWasFlipped && !*bWasFlipped)
      {
        for (int i = 0; i < len; i++)
        {
          if ((int) levels[i] > 0)
          {
            *bWasFlipped = true;
            break;
          }
        }
      }
    }

    free(logical);
    free(visual);
    free(levels);
  }

  return true;
}

CCharsetConverter::CCharsetConverter()
{
}

void CCharsetConverter::OnSettingChanged(const CSetting* setting)
{
  if (setting == NULL)
    return;

  const std::string& settingId = setting->GetId();
  // TODO: does this make any sense at all for subtitles and karaoke?
  if (settingId == "subtitles.charset" ||
      settingId == "karaoke.charset" ||
      settingId == "locale.charset")
    reset();
}

void CCharsetConverter::clear()
{
}

std::vector<std::string> CCharsetConverter::getCharsetLabels()
{
  vector<std::string> lab;
  for(SCharsetMapping* c = g_charsets; c->charset; c++)
    lab.push_back(c->caption);

  return lab;
}

std::string CCharsetConverter::getCharsetLabelByName(const std::string& charsetName)
{
  for(SCharsetMapping* c = g_charsets; c->charset; c++)
  {
    if (StringUtils::EqualsNoCase(charsetName,c->charset))
      return c->caption;
  }

  return "";
}

std::string CCharsetConverter::getCharsetNameByLabel(const std::string& charsetLabel)
{
  for(SCharsetMapping* c = g_charsets; c->charset; c++)
  {
    if (StringUtils::EqualsNoCase(charsetLabel, c->caption))
      return c->charset;
  }

  return "";
}

bool CCharsetConverter::isBidiCharset(const std::string& charset)
{
  for(SFribidMapping* c = g_fribidi; c->charset; c++)
  {
    if (StringUtils::EqualsNoCase(charset, c->charset))
      return true;
  }
  return false;
}

void CCharsetConverter::reset(void)
{
  CSingleLock lock(m_critSection);

  ICONV_SAFE_CLOSE(m_iconvUtf8ToUtf32);
  ICONV_SAFE_CLOSE(m_iconvUtf32ToUtf8);
  ICONV_SAFE_CLOSE(m_iconvUtf32ToW);
  ICONV_SAFE_CLOSE(m_iconvWToUtf32);
  ICONV_SAFE_CLOSE(m_iconvUtf8ToStringCharset);
  ICONV_SAFE_CLOSE(m_iconvStringCharsetToUtf8);
  ICONV_SAFE_CLOSE(m_iconvSubtitleCharsetToW);
  ICONV_SAFE_CLOSE(m_iconvWtoUtf8);
  ICONV_SAFE_CLOSE(m_iconvUtf16BEtoUtf8);
  ICONV_SAFE_CLOSE(m_iconvUtf16LEtoUtf8);
  ICONV_SAFE_CLOSE(m_iconvUtf32ToStringCharset);
  ICONV_SAFE_CLOSE(m_iconvUtf8toW);
  ICONV_SAFE_CLOSE(m_iconvUcs2CharsetToUtf8);


  m_stringFribidiCharset = FRIBIDI_NOTFOUND;

  std::string strCharset=g_langInfo.GetGuiCharSet();
  for(SFribidMapping* c = g_fribidi; c->charset; c++)
  {
    if (StringUtils::EqualsNoCase(strCharset, c->charset))
    {
      m_stringFribidiCharset = c->name;
      break;
    }
  }
}

bool CCharsetConverter::utf8ToUtf32(const std::string& utf8StringSrc, std::u32string& utf32StringDst, bool failOnBadChar /*= true*/)
{
  CSingleLock lock(m_critSection);
  return convert(m_iconvUtf8ToUtf32, 1, UTF8_SOURCE, UTF32_CHARSET, utf8StringSrc, utf32StringDst, failOnBadChar);
}

std::u32string CCharsetConverter::utf8ToUtf32(const std::string& utf8StringSrc, bool failOnBadChar /*= true*/)
{
  std::u32string converted;
  utf8ToUtf32(utf8StringSrc, converted, failOnBadChar);
  return converted;
}

bool CCharsetConverter::utf8ToUtf32Visual(const std::string& utf8StringSrc, std::u32string& utf32StringDst, bool bVisualBiDiFlip /*= false*/, bool forceLTRReadingOrder /*= false*/, bool failOnBadChar /*= false*/)
{
  if (bVisualBiDiFlip)
  {
    std::string strFlipped;
    if (!logicalToVisualBiDi(utf8StringSrc, strFlipped, FRIBIDI_UTF8, forceLTRReadingOrder ? FRIBIDI_TYPE_LTR : FRIBIDI_TYPE_PDF))
      return false;
    CSingleLock lock(m_critSection);
    return convert(m_iconvUtf8ToUtf32, 1, UTF8_SOURCE, UTF32_CHARSET, strFlipped, utf32StringDst, failOnBadChar);
  }
  CSingleLock lock(m_critSection);
  return convert(m_iconvUtf8ToUtf32, 1, UTF8_SOURCE, UTF32_CHARSET, utf8StringSrc, utf32StringDst, failOnBadChar);
}

bool CCharsetConverter::utf32ToUtf8(const std::u32string& utf32StringSrc, std::string& utf8StringDst, bool failOnBadChar /*= true*/)
{
  CSingleLock lock(m_critSection);
  return convert(m_iconvUtf32ToUtf8, m_Utf8CharMaxSize, UTF32_CHARSET, "UTF-8", utf32StringSrc, utf8StringDst, failOnBadChar);
}

std::string CCharsetConverter::utf32ToUtf8(const std::u32string& utf32StringSrc, bool failOnBadChar /*= false*/)
{
  std::string converted;
  utf32ToUtf8(utf32StringSrc, converted, failOnBadChar);
  return converted;
}

bool CCharsetConverter::utf32ToW(const std::u32string& utf32StringSrc, std::wstring& wStringDst, bool failOnBadChar /*= true*/)
{
#ifdef WCHAR_IS_UCS_4
  wStringDst.assign((const wchar_t*)utf32StringSrc.c_str(), utf32StringSrc.length());
  return true;
#else // !WCHAR_IS_UCS_4
  CSingleLock lock(m_critSection);
  return convert(m_iconvUtf32ToW, 1, UTF32_CHARSET, WCHAR_CHARSET, utf32StringSrc, wStringDst, failOnBadChar);
#endif // !WCHAR_IS_UCS_4
}

bool CCharsetConverter::utf32logicalToVisualBiDi(const std::u32string& logicalStringSrc, std::u32string& visualStringDst, bool forceLTRReadingOrder /*= false*/)
{
  visualStringDst.clear();
  std::string utf8Str;
  if (!utf32ToUtf8(logicalStringSrc, utf8Str, false))
    return false;

  return utf8ToUtf32Visual(utf8Str, visualStringDst, true, forceLTRReadingOrder);
}

bool CCharsetConverter::wToUtf32(const std::wstring& wStringSrc, std::u32string& utf32StringDst, bool failOnBadChar /*= true*/)
{
#ifdef WCHAR_IS_UCS_4
  /* UCS-4 is almost equal to UTF-32, but UTF-32 has strict limits on possible values, while UCS-4 is usually unchecked.
   * With this "conversion" we ensure that output will be valid UTF-32 string. */
#endif
  CSingleLock lock(m_critSection);
  return convert(m_iconvWToUtf32, 1, WCHAR_CHARSET, UTF32_CHARSET, wStringSrc, utf32StringDst, failOnBadChar);
}

// The bVisualBiDiFlip forces a flip of characters for hebrew/arabic languages, only set to false if the flipping
// of the string is already made or the string is not displayed in the GUI
bool CCharsetConverter::utf8ToW(const std::string& utf8StringSrc, std::wstring& wStringDst, bool bVisualBiDiFlip /*= true*/, 
                                bool forceLTRReadingOrder /*= false*/, bool failOnBadChar /*= false*/, bool* bWasFlipped /*= NULL*/)
{
  // Try to flip hebrew/arabic characters, if any
  if (bVisualBiDiFlip)
  {
    std::string strFlipped;
    FriBidiCharType charset = forceLTRReadingOrder ? FRIBIDI_TYPE_LTR : FRIBIDI_TYPE_PDF;
    logicalToVisualBiDi(utf8StringSrc, strFlipped, FRIBIDI_UTF8, charset, bWasFlipped);
    CSingleLock lock(m_critSection);
    return convert(m_iconvUtf8toW,1,UTF8_SOURCE,WCHAR_CHARSET,strFlipped,wStringDst, failOnBadChar);
  }
  else
  {
    CSingleLock lock(m_critSection);
    return convert(m_iconvUtf8toW,1,UTF8_SOURCE,WCHAR_CHARSET,utf8StringSrc,wStringDst, failOnBadChar);
  }
}

bool CCharsetConverter::subtitleCharsetToW(const std::string& stringSrc, std::wstring& wStringDst)
{
  // No need to flip hebrew/arabic as mplayer does the flipping
  CSingleLock lock(m_critSection);
  return convert(m_iconvSubtitleCharsetToW,1,g_langInfo.GetSubtitleCharSet(),WCHAR_CHARSET,stringSrc,wStringDst);
}

bool CCharsetConverter::fromW(const std::wstring& wStringSrc,
                              std::string& stringDst, const std::string& enc)
{
  iconv_t iconvString;
  ICONV_PREPARE(iconvString);
  const bool result = convert(iconvString,m_Utf8CharMaxSize,WCHAR_CHARSET,enc,wStringSrc,stringDst);
  iconv_close(iconvString);
  return result;
}

bool CCharsetConverter::toW(const std::string& stringSrc,
                            std::wstring& wStringDst, const std::string& enc)
{
  iconv_t iconvString;
  ICONV_PREPARE(iconvString);
  const bool result = convert(iconvString,1,enc,WCHAR_CHARSET,stringSrc,wStringDst);
  iconv_close(iconvString);
  return result;
}

bool CCharsetConverter::utf8ToStringCharset(const std::string& utf8StringSrc, std::string& stringDst)
{
  CSingleLock lock(m_critSection);
  return convert(m_iconvUtf8ToStringCharset,1,UTF8_SOURCE,g_langInfo.GetGuiCharSet(),utf8StringSrc,stringDst);
}

bool CCharsetConverter::utf8ToStringCharset(std::string& stringSrcDst)
{
  std::string strSrc(stringSrcDst);
  return utf8ToStringCharset(strSrc, stringSrcDst);
}

bool CCharsetConverter::ToUtf8(const std::string& strSourceCharset, const std::string& stringSrc, std::string& utf8StringDst)
{
  if (strSourceCharset == "UTF-8")
  { // simple case - no conversion necessary
    utf8StringDst = stringSrc;
    return true;
  }
  iconv_t iconvString;
  ICONV_PREPARE(iconvString);
  const bool result = convert(iconvString,m_Utf8CharMaxSize,strSourceCharset,"UTF-8",stringSrc,utf8StringDst);
  iconv_close(iconvString);
  return result;
}

bool CCharsetConverter::utf8To(const std::string& strDestCharset, const std::string& utf8StringSrc, std::string& stringDst)
{
  if (strDestCharset == "UTF-8")
  { // simple case - no conversion necessary
    stringDst = utf8StringSrc;
    return true;
  }
  iconv_t iconvString;
  ICONV_PREPARE(iconvString);
  const bool result = convert(iconvString,1,UTF8_SOURCE,strDestCharset,utf8StringSrc,stringDst);
  iconv_close(iconvString);
  return result;
}

bool CCharsetConverter::utf8To(const std::string& strDestCharset, const std::string& utf8StringSrc, std::u16string& utf16StringDst)
{
  iconv_t iconvString;
  ICONV_PREPARE(iconvString);
  const bool result = convert(iconvString,1,UTF8_SOURCE,strDestCharset,utf8StringSrc,utf16StringDst);
  iconv_close(iconvString);
  return result;
}

bool CCharsetConverter::utf8To(const std::string& strDestCharset, const std::string& utf8StringSrc, std::u32string& utf32StringDst)
{
  iconv_t iconvString;
  ICONV_PREPARE(iconvString);
  const bool result = convert(iconvString,1,UTF8_SOURCE,strDestCharset,utf8StringSrc,utf32StringDst);
  iconv_close(iconvString);
  return result;
}

bool CCharsetConverter::unknownToUTF8(std::string& stringSrcDst)
{
  std::string source(stringSrcDst);
  return unknownToUTF8(source, stringSrcDst);
}

bool CCharsetConverter::unknownToUTF8(const std::string& stringSrc, std::string& utf8StringDst, bool failOnBadChar /*= false*/)
{
  // checks whether it's utf8 already, and if not converts using the sourceCharset if given, else the string charset
  if (isValidUtf8(stringSrc))
  {
    utf8StringDst = stringSrc;
    return true;
  }
  CSingleLock lock(m_critSection);
  return convert(m_iconvStringCharsetToUtf8, m_Utf8CharMaxSize, g_langInfo.GetGuiCharSet(), "UTF-8", stringSrc, utf8StringDst, failOnBadChar);
}

bool CCharsetConverter::wToUTF8(const std::wstring& wStringSrc, std::string& utf8StringDst, bool failOnBadChar /*= false*/)
{
  CSingleLock lock(m_critSection);
  return convert(m_iconvWtoUtf8,m_Utf8CharMaxSize,WCHAR_CHARSET,"UTF-8",wStringSrc,utf8StringDst, failOnBadChar);
}

bool CCharsetConverter::utf16BEtoUTF8(const std::u16string& utf16StringSrc, std::string& utf8StringDst)
{
  CSingleLock lock(m_critSection);
  return convert(m_iconvUtf16BEtoUtf8,m_Utf8CharMaxSize,"UTF-16BE","UTF-8",utf16StringSrc,utf8StringDst);
}

bool CCharsetConverter::utf16LEtoUTF8(const std::u16string& utf16StringSrc,
                                      std::string& utf8StringDst)
{
  CSingleLock lock(m_critSection);
  return convert(m_iconvUtf16LEtoUtf8,m_Utf8CharMaxSize,"UTF-16LE","UTF-8",utf16StringSrc,utf8StringDst);
}

bool CCharsetConverter::ucs2ToUTF8(const std::u16string& ucs2StringSrc, std::string& utf8StringDst)
{
  CSingleLock lock(m_critSection);
  return convert(m_iconvUcs2CharsetToUtf8,m_Utf8CharMaxSize,"UCS-2LE","UTF-8",ucs2StringSrc,utf8StringDst);
}

bool CCharsetConverter::utf16LEtoW(const std::u16string& utf16String, std::wstring& wString)
{
  CSingleLock lock(m_critSection);
  return convert(m_iconvUtf16LEtoW,1,"UTF-16LE",WCHAR_CHARSET,utf16String,wString);
}

bool CCharsetConverter::utf32ToStringCharset(const std::u32string& utf32StringSrc, std::string& stringDst)
{
  CSingleLock lock(m_critSection);
  return convert(m_iconvUtf32ToStringCharset, 1, g_langInfo.GetGuiCharSet().c_str(), UTF32_CHARSET, utf32StringSrc, stringDst);
}

bool CCharsetConverter::utf8ToSystem(std::string& stringSrcDst, bool failOnBadChar /*= false*/)
{
  std::string strSrc(stringSrcDst);
  return utf8To("", strSrc, stringSrcDst);
}

// Taken from RFC2640
bool CCharsetConverter::isValidUtf8(const char* buf, unsigned int len)
{
  const unsigned char* endbuf = (unsigned char*)buf + len;
  unsigned char byte2mask=0x00, c;
  int trailing=0; // trailing (continuation) bytes to follow

  while ((unsigned char*)buf != endbuf)
  {
    c = *buf++;
    if (trailing)
      if ((c & 0xc0) == 0x80) // does trailing byte follow UTF-8 format ?
      {
        if (byte2mask) // need to check 2nd byte for proper range
        {
          if (c & byte2mask) // are appropriate bits set ?
            byte2mask = 0x00;
          else
            return false;
        }
        trailing--;
      }
      else
        return 0;
    else
      if ((c & 0x80) == 0x00) continue; // valid 1-byte UTF-8
      else if ((c & 0xe0) == 0xc0)      // valid 2-byte UTF-8
        if (c & 0x1e)                   //is UTF-8 byte in proper range ?
          trailing = 1;
        else
          return false;
      else if ((c & 0xf0) == 0xe0)      // valid 3-byte UTF-8
       {
        if (!(c & 0x0f))                // is UTF-8 byte in proper range ?
          byte2mask = 0x20;             // if not set mask
        trailing = 2;                   // to check next byte
      }
      else if ((c & 0xf8) == 0xf0)      // valid 4-byte UTF-8
      {
        if (!(c & 0x07))                // is UTF-8 byte in proper range ?
          byte2mask = 0x30;             // if not set mask
        trailing = 3;                   // to check next byte
      }
      else if ((c & 0xfc) == 0xf8)      // valid 5-byte UTF-8
      {
        if (!(c & 0x03))                // is UTF-8 byte in proper range ?
          byte2mask = 0x38;             // if not set mask
        trailing = 4;                   // to check next byte
      }
      else if ((c & 0xfe) == 0xfc)      // valid 6-byte UTF-8
      {
        if (!(c & 0x01))                // is UTF-8 byte in proper range ?
          byte2mask = 0x3c;             // if not set mask
        trailing = 5;                   // to check next byte
      }
      else
        return false;
  }
  return trailing == 0;
}

bool CCharsetConverter::isValidUtf8(const std::string& str)
{
  return isValidUtf8(str.c_str(), str.size());
}

bool CCharsetConverter::utf8logicalToVisualBiDi(const std::string& utf8StringSrc, std::string& utf8StringDst)
{
  return logicalToVisualBiDi(utf8StringSrc, utf8StringDst, FRIBIDI_UTF8, FRIBIDI_TYPE_RTL);
}

void CCharsetConverter::SettingOptionsCharsetsFiller(const CSetting* setting, std::vector< std::pair<std::string, std::string> >& list, std::string& current)
{
  vector<std::string> vecCharsets = g_charsetConverter.getCharsetLabels();
  sort(vecCharsets.begin(), vecCharsets.end(), sortstringbyname());

  list.push_back(make_pair(g_localizeStrings.Get(13278), "DEFAULT")); // "Default"
  for (int i = 0; i < (int) vecCharsets.size(); ++i)
    list.push_back(make_pair(vecCharsets[i], g_charsetConverter.getCharsetNameByLabel(vecCharsets[i])));
}
