/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://www.xbmc.org
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
#include <fribidi/fribidi.h>
#include "LangInfo.h"
#include "threads/SingleLock.h"
#include "log.h"

#include <errno.h>
#include <iconv.h>

#if defined(TARGET_DARWIN)
#ifdef __POWERPC__
  #define WCHAR_CHARSET "UTF-32BE"
#else
  #define WCHAR_CHARSET "UTF-32LE"
#endif
  #define UTF8_SOURCE "UTF-8-MAC"
#elif defined(WIN32)
  #define WCHAR_CHARSET "UTF-16LE"
  #define UTF8_SOURCE "UTF-8"
  #pragma comment(lib, "libfribidi.lib")
  #pragma comment(lib, "libiconv.lib")
#elif defined(TARGET_ANDROID)
  #define UTF8_SOURCE "UTF-8"
#ifdef __BIG_ENDIAN__
  #define WCHAR_CHARSET "UTF-32BE"
#else
  #define WCHAR_CHARSET "UTF-32LE"
#endif
#else
  #define WCHAR_CHARSET "WCHAR_T"
  #define UTF8_SOURCE "UTF-8"
#endif


static iconv_t m_iconvStringCharsetToFontCharset = (iconv_t)-1;
static iconv_t m_iconvSubtitleCharsetToW         = (iconv_t)-1;
static iconv_t m_iconvUtf8ToStringCharset        = (iconv_t)-1;
static iconv_t m_iconvStringCharsetToUtf8        = (iconv_t)-1;
static iconv_t m_iconvUcs2CharsetToStringCharset = (iconv_t)-1;
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


#define UTF8_DEST_MULTIPLIER 6

#define ICONV_PREPARE(iconv) iconv=(iconv_t)-1
#define ICONV_SAFE_CLOSE(iconv) if (iconv!=(iconv_t)-1) { iconv_close(iconv); iconv=(iconv_t)-1; }

size_t iconv_const (void* cd, const char** inbuf, size_t *inbytesleft,
                    char* * outbuf, size_t *outbytesleft)
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
static bool convert_checked(iconv_t& type, int multiplier, const CStdString& strFromCharset, const CStdString& strToCharset, const INPUT& strSource, OUTPUT& strDest)
{
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

  if (strSource.IsEmpty())
  {
    strDest.clear(); //empty strings are easy
    return true;
  }

  //input buffer for iconv() is the buffer from strSource
  size_t      inBufSize  = (strSource.length() + 1) * sizeof(strSource[0]);
  const char* inBuf      = (const char*)strSource.c_str();

  //allocate output buffer for iconv()
  size_t      outBufSize = (strSource.length() + 1) * multiplier;
  char*       outBuf     = (char*)malloc(outBufSize);

  size_t      inBytesAvail  = inBufSize;  //how many bytes iconv() can read
  size_t      outBytesAvail = outBufSize; //how many bytes iconv() can write
  const char* inBufStart    = inBuf;      //where in our input buffer iconv() should start reading
  char*       outBufStart   = outBuf;     //where in out output buffer iconv() should start writing

  while(1)
  {
    //iconv() will update inBufStart, inBytesAvail, outBufStart and outBytesAvail
    size_t returnV = iconv_const(type, &inBufStart, &inBytesAvail, &outBufStart, &outBytesAvail);

    if ((returnV == (size_t)-1) && (errno != EINVAL))
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
          CLog::Log(LOGERROR, "%s realloc failed with buffer=%p size=%zu errno=%d(%s)",
                    __FUNCTION__, outBuf, outBufSize, errno, strerror(errno));
          free(outBuf);
          return false;
        }
        outBuf = newBuf;

        //update the buffer pointer and counter
        outBufStart   = outBuf + bytesConverted;
        outBytesAvail = outBufSize - bytesConverted;

        //continue in the loop and convert the rest
      }
      else if (errno == EILSEQ) //An invalid multibyte sequence has been encountered in the input
      {
        //skip invalid byte
        inBufStart++;
        inBytesAvail--;

        //continue in the loop and convert the rest
      }
      else //iconv() had some other error
      {
        CLog::Log(LOGERROR, "%s iconv() failed from %s to %s, errno=%d(%s)",
                  __FUNCTION__, strFromCharset.c_str(), strToCharset.c_str(), errno, strerror(errno));
        free(outBuf);
        return false;
      }
    }
    else
    {
      //complete the conversion, otherwise the current data will prefix the data on the next call
      returnV = iconv_const(type, NULL, NULL, &outBufStart, &outBytesAvail);
      if (returnV == (size_t)-1)
        CLog::Log(LOGERROR, "%s failed cleanup errno=%d(%s)", __FUNCTION__, errno, strerror(errno));

      //we're done
      break;
    }
  }

  size_t bytesWritten = outBufSize - outBytesAvail;
  char*  dest         = (char*)strDest.GetBuffer(bytesWritten);

  //copy the output from iconv() into the CStdString
  memcpy(dest, outBuf, bytesWritten);

  strDest.ReleaseBuffer();
  
  free(outBuf);

  return true;
}

template<class INPUT,class OUTPUT>
static void convert(iconv_t& type, int multiplier, const CStdString& strFromCharset, const CStdString& strToCharset, const INPUT& strSource,  OUTPUT& strDest)
{
  if(!convert_checked(type, multiplier, strFromCharset, strToCharset, strSource, strDest))
    strDest = strSource;
}

using namespace std;

static void logicalToVisualBiDi(const CStdStringA& strSource, CStdStringA& strDest, FriBidiCharSet fribidiCharset, FriBidiCharType base = FRIBIDI_TYPE_LTR, bool* bWasFlipped =NULL)
{
  // libfribidi is not threadsafe, so make sure we make it so
  CSingleLock lock(m_critSection);

  vector<CStdString> lines;
  CUtil::Tokenize(strSource, lines, "\n");
  CStdString resultString;

  if (bWasFlipped)
    *bWasFlipped = false;

  for (unsigned int i = 0; i < lines.size(); i++)
  {
    int sourceLen = lines[i].length();

    // Convert from the selected charset to Unicode
    FriBidiChar* logical = (FriBidiChar*) malloc((sourceLen + 1) * sizeof(FriBidiChar));
    int len = fribidi_charset_to_unicode(fribidiCharset, (char*) lines[i].c_str(), sourceLen, logical);

    FriBidiChar* visual = (FriBidiChar*) malloc((len + 1) * sizeof(FriBidiChar));
    FriBidiLevel* levels = (FriBidiLevel*) malloc((len + 1) * sizeof(FriBidiLevel));

    if (fribidi_log2vis(logical, len, &base, visual, NULL, NULL, levels))
    {
      // Removes bidirectional marks
      len = fribidi_remove_bidi_marks(visual, len, NULL, NULL, NULL);

      // Apperently a string can get longer during this transformation
      // so make sure we allocate the maximum possible character utf8
      // can generate atleast, should cover all bases
      char *result = strDest.GetBuffer(len*4);

      // Convert back from Unicode to the charset
      int len2 = fribidi_unicode_to_charset(fribidiCharset, visual, len, result);
      ASSERT(len2 <= len*4);
      strDest.ReleaseBuffer();

      resultString += strDest;

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

  strDest = resultString;
}

CCharsetConverter::CCharsetConverter()
{
}

void CCharsetConverter::clear()
{
}

vector<CStdString> CCharsetConverter::getCharsetLabels()
{
  vector<CStdString> lab;
  for(SCharsetMapping * c = g_charsets; c->charset; c++)
    lab.push_back(c->caption);

  return lab;
}

CStdString CCharsetConverter::getCharsetLabelByName(const CStdString& charsetName)
{
  for(SCharsetMapping * c = g_charsets; c->charset; c++)
  {
    if (charsetName.Equals(c->charset))
      return c->caption;
  }

  return "";
}

CStdString CCharsetConverter::getCharsetNameByLabel(const CStdString& charsetLabel)
{
  for(SCharsetMapping *c = g_charsets; c->charset; c++)
  {
    if (charsetLabel.Equals(c->caption))
      return c->charset;
  }

  return "";
}

bool CCharsetConverter::isBidiCharset(const CStdString& charset)
{
  for(SFribidMapping *c = g_fribidi; c->charset; c++)
  {
    if (charset.Equals(c->charset))
      return true;
  }
  return false;
}

void CCharsetConverter::reset(void)
{
  CSingleLock lock(m_critSection);

  ICONV_SAFE_CLOSE(m_iconvStringCharsetToFontCharset);
  ICONV_SAFE_CLOSE(m_iconvUtf8ToStringCharset);
  ICONV_SAFE_CLOSE(m_iconvStringCharsetToUtf8);
  ICONV_SAFE_CLOSE(m_iconvUcs2CharsetToStringCharset);
  ICONV_SAFE_CLOSE(m_iconvSubtitleCharsetToW);
  ICONV_SAFE_CLOSE(m_iconvWtoUtf8);
  ICONV_SAFE_CLOSE(m_iconvUtf16BEtoUtf8);
  ICONV_SAFE_CLOSE(m_iconvUtf16LEtoUtf8);
  ICONV_SAFE_CLOSE(m_iconvUtf32ToStringCharset);
  ICONV_SAFE_CLOSE(m_iconvUtf8toW);
  ICONV_SAFE_CLOSE(m_iconvUcs2CharsetToUtf8);


  m_stringFribidiCharset = FRIBIDI_NOTFOUND;

  CStdString strCharset=g_langInfo.GetGuiCharSet();
  for(SFribidMapping *c = g_fribidi; c->charset; c++)
  {
    if (strCharset.Equals(c->charset))
      m_stringFribidiCharset = c->name;
  }
}

// The bVisualBiDiFlip forces a flip of characters for hebrew/arabic languages, only set to false if the flipping
// of the string is already made or the string is not displayed in the GUI
void CCharsetConverter::utf8ToW(const CStdStringA& utf8String, CStdStringW &wString, bool bVisualBiDiFlip/*=true*/, bool forceLTRReadingOrder /*=false*/, bool* bWasFlipped/*=NULL*/)
{
  // Try to flip hebrew/arabic characters, if any
  if (bVisualBiDiFlip)
  {
    CStdStringA strFlipped;
    FriBidiCharType charset = forceLTRReadingOrder ? FRIBIDI_TYPE_LTR : FRIBIDI_TYPE_PDF;
    logicalToVisualBiDi(utf8String, strFlipped, FRIBIDI_UTF8, charset, bWasFlipped);
    CSingleLock lock(m_critSection);
    convert(m_iconvUtf8toW,sizeof(wchar_t),UTF8_SOURCE,WCHAR_CHARSET,strFlipped,wString);
  }
  else
  {
    CSingleLock lock(m_critSection);
    convert(m_iconvUtf8toW,sizeof(wchar_t),UTF8_SOURCE,WCHAR_CHARSET,utf8String,wString);
  }
}

void CCharsetConverter::subtitleCharsetToW(const CStdStringA& strSource, CStdStringW& strDest)
{
  // No need to flip hebrew/arabic as mplayer does the flipping
  CSingleLock lock(m_critSection);
  convert(m_iconvSubtitleCharsetToW,sizeof(wchar_t),g_langInfo.GetSubtitleCharSet(),WCHAR_CHARSET,strSource,strDest);
}

void CCharsetConverter::fromW(const CStdStringW& strSource,
                              CStdStringA& strDest, const CStdString& enc)
{
  iconv_t iconvString;
  ICONV_PREPARE(iconvString);
  convert(iconvString,4,WCHAR_CHARSET,enc,strSource,strDest);
  iconv_close(iconvString);
}

void CCharsetConverter::toW(const CStdStringA& strSource,
                            CStdStringW& strDest, const CStdString& enc)
{
  iconv_t iconvString;
  ICONV_PREPARE(iconvString);
  convert(iconvString,sizeof(wchar_t),enc,WCHAR_CHARSET,strSource,strDest);
  iconv_close(iconvString);
}

void CCharsetConverter::utf8ToStringCharset(const CStdStringA& strSource, CStdStringA& strDest)
{
  CSingleLock lock(m_critSection);
  convert(m_iconvUtf8ToStringCharset,1,UTF8_SOURCE,g_langInfo.GetGuiCharSet(),strSource,strDest);
}

void CCharsetConverter::utf8ToStringCharset(CStdStringA& strSourceDest)
{
  CStdString strDest;
  utf8ToStringCharset(strSourceDest, strDest);
  strSourceDest=strDest;
}

void CCharsetConverter::stringCharsetToUtf8(const CStdStringA& strSourceCharset, const CStdStringA& strSource, CStdStringA& strDest)
{
  iconv_t iconvString;
  ICONV_PREPARE(iconvString);
  convert(iconvString,UTF8_DEST_MULTIPLIER,strSourceCharset,"UTF-8",strSource,strDest);
  iconv_close(iconvString);
}

void CCharsetConverter::utf8ToUtf8NFC(CStdStringA& strSourceDest)
{
  // To keep stuff simple, we only check all decomposable
  // simple letter character candidates.
  // Sample: 0x41 0xcc 0x88 => 0xc3 0x84 (&adieresis; or &auml;)
  int NFD_NFC_tupels[] =
  {
    0x41cc80, 0xc380, 0x45cc80, 0xc388, 0x49cc80, 0xc38c, 0x4ecc80, 0xc7b8, 
    0x4fcc80, 0xc392, 0x55cc80, 0xc399, 0x61cc80, 0xc3a0, 0x65cc80, 0xc3a8, 
    0x69cc80, 0xc3ac, 0x6ecc80, 0xc7b9, 0x6fcc80, 0xc3b2, 0x75cc80, 0xc3b9, 
    0x41cc81, 0xc381, 0x43cc81, 0xc486, 0x45cc81, 0xc389, 0x47cc81, 0xc7b4, 
    0x49cc81, 0xc38d, 0x4ccc81, 0xc4b9, 0x4ecc81, 0xc583, 0x4fcc81, 0xc393, 
    0x52cc81, 0xc594, 0x53cc81, 0xc59a, 0x55cc81, 0xc39a, 0x59cc81, 0xc39d, 
    0x5acc81, 0xc5b9, 0x61cc81, 0xc3a1, 0x63cc81, 0xc487, 0x65cc81, 0xc3a9, 
    0x67cc81, 0xc7b5, 0x69cc81, 0xc3ad, 0x6ccc81, 0xc4ba, 0x6ecc81, 0xc584, 
    0x6fcc81, 0xc3b3, 0x72cc81, 0xc595, 0x73cc81, 0xc59b, 0x75cc81, 0xc3ba, 
    0x79cc81, 0xc3bd, 0x7acc81, 0xc5ba, 0x41cc82, 0xc382, 0x43cc82, 0xc488, 
    0x45cc82, 0xc38a, 0x47cc82, 0xc49c, 0x48cc82, 0xc4a4, 0x49cc82, 0xc38e, 
    0x4acc82, 0xc4b4, 0x4fcc82, 0xc394, 0x53cc82, 0xc59c, 0x55cc82, 0xc39b, 
    0x57cc82, 0xc5b4, 0x59cc82, 0xc5b6, 0x61cc82, 0xc3a2, 0x63cc82, 0xc489, 
    0x65cc82, 0xc3aa, 0x67cc82, 0xc49d, 0x68cc82, 0xc4a5, 0x69cc82, 0xc3ae, 
    0x6acc82, 0xc4b5, 0x6fcc82, 0xc3b4, 0x73cc82, 0xc59d, 0x75cc82, 0xc3bb, 
    0x77cc82, 0xc5b5, 0x79cc82, 0xc5b7, 0x41cc83, 0xc383, 0x49cc83, 0xc4a8, 
    0x4ecc83, 0xc391, 0x4fcc83, 0xc395, 0x55cc83, 0xc5a8, 0x61cc83, 0xc3a3, 
    0x69cc83, 0xc4a9, 0x6ecc83, 0xc3b1, 0x6fcc83, 0xc3b5, 0x75cc83, 0xc5a9, 
    0x41cc84, 0xc480, 0x45cc84, 0xc492, 0x49cc84, 0xc4aa, 0x4fcc84, 0xc58c, 
    0x55cc84, 0xc5aa, 0x59cc84, 0xc8b2, 0x61cc84, 0xc481, 0x65cc84, 0xc493, 
    0x69cc84, 0xc4ab, 0x6fcc84, 0xc58d, 0x75cc84, 0xc5ab, 0x79cc84, 0xc8b3, 
    0x41cc86, 0xc482, 0x45cc86, 0xc494, 0x47cc86, 0xc49e, 0x49cc86, 0xc4ac, 
    0x4fcc86, 0xc58e, 0x55cc86, 0xc5ac, 0x61cc86, 0xc483, 0x65cc86, 0xc495, 
    0x67cc86, 0xc49f, 0x69cc86, 0xc4ad, 0x6fcc86, 0xc58f, 0x75cc86, 0xc5ad, 
    0x41cc87, 0xc8a6, 0x43cc87, 0xc48a, 0x45cc87, 0xc496, 0x47cc87, 0xc4a0, 
    0x49cc87, 0xc4b0, 0x4fcc87, 0xc8ae, 0x5acc87, 0xc5bb, 0x61cc87, 0xc8a7, 
    0x63cc87, 0xc48b, 0x65cc87, 0xc497, 0x67cc87, 0xc4a1, 0x6fcc87, 0xc8af, 
    0x7acc87, 0xc5bc, 0x41cc88, 0xc384, 0x45cc88, 0xc38b, 0x49cc88, 0xc38f, 
    0x4fcc88, 0xc396, 0x55cc88, 0xc39c, 0x59cc88, 0xc5b8, 0x61cc88, 0xc3a4, 
    0x65cc88, 0xc3ab, 0x69cc88, 0xc3af, 0x6fcc88, 0xc3b6, 0x75cc88, 0xc3bc, 
    0x79cc88, 0xc3bf, 0x41cc8a, 0xc385, 0x55cc8a, 0xc5ae, 0x61cc8a, 0xc3a5, 
    0x75cc8a, 0xc5af, 0x4fcc8b, 0xc590, 0x55cc8b, 0xc5b0, 0x6fcc8b, 0xc591, 
    0x75cc8b, 0xc5b1, 0x41cc8c, 0xc78d, 0x43cc8c, 0xc48c, 0x44cc8c, 0xc48e, 
    0x45cc8c, 0xc49a, 0x47cc8c, 0xc7a6, 0x48cc8c, 0xc89e, 0x49cc8c, 0xc78f, 
    0x4bcc8c, 0xc7a8, 0x4ccc8c, 0xc4bd, 0x4ecc8c, 0xc587, 0x4fcc8c, 0xc791, 
    0x52cc8c, 0xc598, 0x53cc8c, 0xc5a0, 0x54cc8c, 0xc5a4, 0x55cc8c, 0xc793, 
    0x5acc8c, 0xc5bd, 0x61cc8c, 0xc78e, 0x63cc8c, 0xc48d, 0x64cc8c, 0xc48f, 
    0x65cc8c, 0xc49b, 0x67cc8c, 0xc7a7, 0x68cc8c, 0xc89f, 0x69cc8c, 0xc790, 
    0x6acc8c, 0xc7b0, 0x6bcc8c, 0xc7a9, 0x6ccc8c, 0xc4be, 0x6ecc8c, 0xc588, 
    0x6fcc8c, 0xc792, 0x72cc8c, 0xc599, 0x73cc8c, 0xc5a1, 0x74cc8c, 0xc5a5, 
    0x75cc8c, 0xc794, 0x7acc8c, 0xc5be, 0x41cc8f, 0xc880, 0x45cc8f, 0xc884, 
    0x49cc8f, 0xc888, 0x4fcc8f, 0xc88c, 0x52cc8f, 0xc890, 0x55cc8f, 0xc894, 
    0x61cc8f, 0xc881, 0x65cc8f, 0xc885, 0x69cc8f, 0xc889, 0x6fcc8f, 0xc88d, 
    0x72cc8f, 0xc891, 0x75cc8f, 0xc895, 0x41cc91, 0xc882, 0x45cc91, 0xc886, 
    0x49cc91, 0xc88a, 0x4fcc91, 0xc88e, 0x52cc91, 0xc892, 0x55cc91, 0xc896, 
    0x61cc91, 0xc883, 0x65cc91, 0xc887, 0x69cc91, 0xc88b, 0x6fcc91, 0xc88f, 
    0x72cc91, 0xc893, 0x75cc91, 0xc897, 0x4fcc9b, 0xc6a0, 0x55cc9b, 0xc6af, 
    0x6fcc9b, 0xc6a1, 0x75cc9b, 0xc6b0, 0x53cca6, 0xc898, 0x54cca6, 0xc89a, 
    0x73cca6, 0xc899, 0x74cca6, 0xc89b, 0x43cca7, 0xc387, 0x45cca7, 0xc8a8, 
    0x47cca7, 0xc4a2, 0x4bcca7, 0xc4b6, 0x4ccca7, 0xc4bb, 0x4ecca7, 0xc585, 
    0x52cca7, 0xc596, 0x53cca7, 0xc59e, 0x54cca7, 0xc5a2, 0x63cca7, 0xc3a7, 
    0x65cca7, 0xc8a9, 0x67cca7, 0xc4a3, 0x6bcca7, 0xc4b7, 0x6ccca7, 0xc4bc, 
    0x6ecca7, 0xc586, 0x72cca7, 0xc597, 0x73cca7, 0xc59f, 0x74cca7, 0xc5a3, 
    0x41cca8, 0xc484, 0x45cca8, 0xc498, 0x49cca8, 0xc4ae, 0x4fcca8, 0xc7aa, 
    0x55cca8, 0xc5b2, 0x61cca8, 0xc485, 0x65cca8, 0xc499, 0x69cca8, 0xc4af, 
    0x6fcca8, 0xc7ab, 0x75cca8, 0xc5b3, 0x41cd80, 0xc380, 0x45cd80, 0xc388, 
    0x49cd80, 0xc38c, 0x4ecd80, 0xc7b8, 0x4fcd80, 0xc392, 0x55cd80, 0xc399, 
    0x61cd80, 0xc3a0, 0x65cd80, 0xc3a8, 0x69cd80, 0xc3ac, 0x6ecd80, 0xc7b9, 
    0x6fcd80, 0xc3b2, 0x75cd80, 0xc3b9, 0x41cd81, 0xc381, 0x43cd81, 0xc486, 
    0x45cd81, 0xc389, 0x47cd81, 0xc7b4, 0x49cd81, 0xc38d, 0x4ccd81, 0xc4b9, 
    0x4ecd81, 0xc583, 0x4fcd81, 0xc393, 0x52cd81, 0xc594, 0x53cd81, 0xc59a, 
    0x55cd81, 0xc39a, 0x59cd81, 0xc39d, 0x5acd81, 0xc5b9, 0x61cd81, 0xc3a1, 
    0x63cd81, 0xc487, 0x65cd81, 0xc3a9, 0x67cd81, 0xc7b5, 0x69cd81, 0xc3ad, 
    0x6ccd81, 0xc4ba, 0x6ecd81, 0xc584, 0x6fcd81, 0xc3b3, 0x72cd81, 0xc595, 
    0x73cd81, 0xc59b, 0x75cd81, 0xc3ba, 0x79cd81, 0xc3bd, 0x7acd81, 0xc5ba, 
    0x55cd84, 0xc797, 0x75cd84, 0xc798, 0
  };

  CStdStringA strDest;

  strDest.reserve(strSourceDest.length());

  int i = 0;
  for (; i < (int)strSourceDest.size() - 2; ++i)
  {
    int kar  = (unsigned char)strSourceDest[i];
    int kar1 = (unsigned char)strSourceDest[i+1];
    int kar2 = (unsigned char)strSourceDest[i+2];

    if (((kar1 == 0xcc) || (kar1 == 0xcd)) && (kar2 >= 0x80))
    {
      int nfd  = (kar << 16) | (kar1 << 8) | kar2;
      int skip = false;

      for (int j = 0; NFD_NFC_tupels[j]; j+=2)
      {
        if (NFD_NFC_tupels[j] == nfd)
        {
          strDest += NFD_NFC_tupels[j+1] >> 8;
          strDest += NFD_NFC_tupels[j+1] & 0xff;
          skip = true;
          i+=2;
          break;
        }
      }
      if(skip) continue;
    }
    strDest += kar;
  }
  for (; i < (int)strSourceDest.size(); ++i)
    strDest += strSourceDest[i];
  strSourceDest = strDest;
}

void CCharsetConverter::utf8To(const CStdStringA& strDestCharset, const CStdStringA& strSource, CStdStringA& strDest)
{
  if (strDestCharset == "UTF-8")
  { // simple case - no conversion necessary
    strDest = strSource;
    return;
  }
  iconv_t iconvString;
  ICONV_PREPARE(iconvString);
  convert(iconvString,UTF8_DEST_MULTIPLIER,UTF8_SOURCE,strDestCharset,strSource,strDest);
  iconv_close(iconvString);
}

void CCharsetConverter::utf8To(const CStdStringA& strDestCharset, const CStdStringA& strSource, CStdString16& strDest)
{
  iconv_t iconvString;
  ICONV_PREPARE(iconvString);
  if(!convert_checked(iconvString,UTF8_DEST_MULTIPLIER,UTF8_SOURCE,strDestCharset,strSource,strDest))
    strDest.clear();
  iconv_close(iconvString);
}

void CCharsetConverter::utf8To(const CStdStringA& strDestCharset, const CStdStringA& strSource, CStdString32& strDest)
{
  iconv_t iconvString;
  ICONV_PREPARE(iconvString);
  if(!convert_checked(iconvString,UTF8_DEST_MULTIPLIER,UTF8_SOURCE,strDestCharset,strSource,strDest))
    strDest.clear();
  iconv_close(iconvString);
}

void CCharsetConverter::unknownToUTF8(CStdStringA &sourceAndDest)
{
  CStdString source = sourceAndDest;
  unknownToUTF8(source, sourceAndDest);
}

void CCharsetConverter::unknownToUTF8(const CStdStringA &source, CStdStringA &dest)
{
  // checks whether it's utf8 already, and if not converts using the sourceCharset if given, else the string charset
  if (isValidUtf8(source))
    dest = source;
  else
  {
    CSingleLock lock(m_critSection);
    convert(m_iconvStringCharsetToUtf8, UTF8_DEST_MULTIPLIER, g_langInfo.GetGuiCharSet(), "UTF-8", source, dest);
  }
}

void CCharsetConverter::wToUTF8(const CStdStringW& strSource, CStdStringA &strDest)
{
  CSingleLock lock(m_critSection);
  convert(m_iconvWtoUtf8,UTF8_DEST_MULTIPLIER,WCHAR_CHARSET,"UTF-8",strSource,strDest);
}

void CCharsetConverter::utf16BEtoUTF8(const CStdString16& strSource, CStdStringA &strDest)
{
  CSingleLock lock(m_critSection);
  if(!convert_checked(m_iconvUtf16BEtoUtf8,UTF8_DEST_MULTIPLIER,"UTF-16BE","UTF-8",strSource,strDest))
    strDest.clear();
}

void CCharsetConverter::utf16LEtoUTF8(const CStdString16& strSource,
                                      CStdStringA &strDest)
{
  CSingleLock lock(m_critSection);
  if(!convert_checked(m_iconvUtf16LEtoUtf8,UTF8_DEST_MULTIPLIER,"UTF-16LE","UTF-8",strSource,strDest))
    strDest.clear();
}

void CCharsetConverter::ucs2ToUTF8(const CStdString16& strSource, CStdStringA& strDest)
{
  CSingleLock lock(m_critSection);
  if(!convert_checked(m_iconvUcs2CharsetToUtf8,UTF8_DEST_MULTIPLIER,"UCS-2LE","UTF-8",strSource,strDest))
    strDest.clear();
}

void CCharsetConverter::utf16LEtoW(const CStdString16& strSource, CStdStringW &strDest)
{
  CSingleLock lock(m_critSection);
  if(!convert_checked(m_iconvUtf16LEtoW,sizeof(wchar_t),"UTF-16LE",WCHAR_CHARSET,strSource,strDest))
    strDest.clear();
}

void CCharsetConverter::ucs2CharsetToStringCharset(const CStdStringW& strSource, CStdStringA& strDest, bool swap)
{
  CStdStringW strCopy = strSource;
  if (swap)
  {
    char* s = (char*) strCopy.c_str();

    while (*s || *(s + 1))
    {
      char c = *s;
      *s = *(s + 1);
      *(s + 1) = c;

      s++;
      s++;
    }
  }
  CSingleLock lock(m_critSection);
  convert(m_iconvUcs2CharsetToStringCharset,4,"UTF-16LE",
          g_langInfo.GetGuiCharSet(),strCopy,strDest);
}

void CCharsetConverter::utf32ToStringCharset(const unsigned long* strSource, CStdStringA& strDest)
{
  CSingleLock lock(m_critSection);

  if (m_iconvUtf32ToStringCharset == (iconv_t) - 1)
  {
    CStdString strCharset=g_langInfo.GetGuiCharSet();
    m_iconvUtf32ToStringCharset = iconv_open(strCharset.c_str(), "UTF-32LE");
  }

  if (m_iconvUtf32ToStringCharset != (iconv_t) - 1)
  {
    const unsigned long* ptr=strSource;
    while (*ptr) ptr++;
    const char* src = (const char*) strSource;
    size_t inBytes = (ptr-strSource+1)*4;

    char *dst = strDest.GetBuffer(inBytes);
    size_t outBytes = inBytes;

    if (iconv_const(m_iconvUtf32ToStringCharset, &src, &inBytes, &dst, &outBytes) == (size_t)-1)
    {
      CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
      strDest.ReleaseBuffer();
      strDest = (const char *)strSource;
      return;
    }

    if (iconv(m_iconvUtf32ToStringCharset, NULL, NULL, &dst, &outBytes) == (size_t)-1)
    {
      CLog::Log(LOGERROR, "%s failed cleanup", __FUNCTION__);
      strDest.ReleaseBuffer();
      strDest = (const char *)strSource;
      return;
    }

    strDest.ReleaseBuffer();
  }
}

void CCharsetConverter::utf8ToSystem(CStdStringA& strSourceDest)
{
  CStdString strDest;
  g_charsetConverter.utf8To("", strSourceDest, strDest);
  strSourceDest = strDest;
}

// Taken from RFC2640
bool CCharsetConverter::isValidUtf8(const char *buf, unsigned int len)
{
  const unsigned char *endbuf = (unsigned char*)buf + len;
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

bool CCharsetConverter::isValidUtf8(const CStdString& str)
{
  return isValidUtf8(str.c_str(), str.size());
}

void CCharsetConverter::utf8logicalToVisualBiDi(const CStdStringA& strSource, CStdStringA& strDest)
{
  logicalToVisualBiDi(strSource, strDest, FRIBIDI_UTF8, FRIBIDI_TYPE_RTL);
}
