/*
 *      Copyright (C) 2005-2008 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
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

#ifdef __APPLE__
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

static FriBidiCharSet m_stringFribidiCharset     = FRIBIDI_CHAR_SET_NOT_FOUND;

static CCriticalSection            m_critSection;

static struct SFribidMapping
{
  FriBidiCharSet name;
  const char*    charset;
} g_fribidi[] = {
  { FRIBIDI_CHAR_SET_ISO8859_6, "ISO-8859-6"   }
, { FRIBIDI_CHAR_SET_ISO8859_8, "ISO-8859-8"   }
, { FRIBIDI_CHAR_SET_CP1255   , "CP1255"       }
, { FRIBIDI_CHAR_SET_CP1255   , "Windows-1255" }
, { FRIBIDI_CHAR_SET_CP1256   , "CP1256"       }
, { FRIBIDI_CHAR_SET_CP1256   , "Windows-1256" }
, { FRIBIDI_CHAR_SET_NOT_FOUND, NULL           }
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
    strDest.Empty(); //empty strings are easy
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

    if (fribidi_log2vis(logical, len, &base, visual, NULL, NULL, NULL))
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


  m_stringFribidiCharset = FRIBIDI_CHAR_SET_NOT_FOUND;

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
    logicalToVisualBiDi(utf8String, strFlipped, FRIBIDI_CHAR_SET_UTF8, charset, bWasFlipped);
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
    strDest.Empty();
  iconv_close(iconvString);
}

void CCharsetConverter::utf8To(const CStdStringA& strDestCharset, const CStdStringA& strSource, CStdString32& strDest)
{
  iconv_t iconvString;
  ICONV_PREPARE(iconvString);
  if(!convert_checked(iconvString,UTF8_DEST_MULTIPLIER,UTF8_SOURCE,strDestCharset,strSource,strDest))
    strDest.Empty();
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
    strDest.empty();
}

void CCharsetConverter::utf16LEtoUTF8(const CStdString16& strSource,
                                      CStdStringA &strDest)
{
  CSingleLock lock(m_critSection);
  if(!convert_checked(m_iconvUtf16LEtoUtf8,UTF8_DEST_MULTIPLIER,"UTF-16LE","UTF-8",strSource,strDest))
    strDest.empty();
}

void CCharsetConverter::ucs2ToUTF8(const CStdString16& strSource, CStdStringA& strDest)
{
  CSingleLock lock(m_critSection);
  if(!convert_checked(m_iconvUcs2CharsetToUtf8,UTF8_DEST_MULTIPLIER,"UCS-2LE","UTF-8",strSource,strDest))
    strDest.empty();
}

void CCharsetConverter::utf16LEtoW(const CStdString16& strSource, CStdStringW &strDest)
{
  CSingleLock lock(m_critSection);
  if(!convert_checked(m_iconvUtf16LEtoW,sizeof(wchar_t),"UTF-16LE",WCHAR_CHARSET,strSource,strDest))
    strDest.empty();
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
  logicalToVisualBiDi(strSource, strDest, FRIBIDI_CHAR_SET_UTF8, FRIBIDI_TYPE_RTL);
}
