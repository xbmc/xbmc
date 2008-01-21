
#include "stdafx.h"
#include "./HTMLUtil.h"
using namespace HTML;


CHTMLUtil::CHTMLUtil(void)
{}

CHTMLUtil::~CHTMLUtil(void)
{}

int CHTMLUtil::FindTag(const CStdString& strHTML, const CStdString& strTag, CStdString& strtagFound, int iPos) const
{
  CStdString strHTMLLow = strHTML;
  CStdString strTagLow = strTag;
  strHTMLLow.ToLower();
  strTagLow.ToLower();
  strtagFound = "";
  int iStart = strHTMLLow.Find(strTag, iPos);
  if (iStart < 0) return -1;
  int iEnd = strHTMLLow.Find(">", iStart);
  if (iEnd < 0) iEnd = (int)strHTMLLow.size();
  strtagFound = strHTMLLow.Mid(iStart, (iEnd + 1) - iStart);
  return iStart;
}

int CHTMLUtil::FindClosingTag(const CStdString& strHTML, const CStdString& strTag, CStdString& strtagFound, int iPos) const
{
  CStdString strHTMLLow = strHTML;
  CStdString strTagLow = strTag;
  strHTMLLow.ToLower();
  strTagLow.ToLower();
  strtagFound = "";
  int iStart = strHTMLLow.Find("</" + strTag, iPos);
  if (iStart < 0) return -1;
  int iOpenStart = strHTMLLow.Find("<" + strTag, iPos);
  while (iOpenStart < iStart && iOpenStart != -1)
  {
    iStart = strHTMLLow.Find("</" + strTag, iStart + 1);
    iOpenStart = strHTMLLow.Find("<" + strTag, iOpenStart + 1);
  }

  int iEnd = strHTMLLow.Find(">", iStart);
  if (iEnd < 0) iEnd = (int)strHTMLLow.size();
  strtagFound = strHTMLLow.Mid(iStart, (iEnd + 1) - iStart);
  return iStart;
}

void CHTMLUtil::getValueOfTag(const CStdString& strTagAndValue, CStdString& strValue)
{
  // strTagAndValue contains:
  // like <a href=blablabla.....>value</a>
  strValue = strTagAndValue;
  int iStart = strTagAndValue.Find(">");
  int iEnd = strTagAndValue.Find("<", iStart + 1);
  if (iStart >= 0 && iEnd >= 0)
  {
    iStart++;
    strValue = strTagAndValue.Mid(iStart, iEnd - iStart);
  }
}

void CHTMLUtil::getAttributeOfTag(const CStdString& strTagAndValue, const CStdString& strTag, CStdString& strValue)
{
  // strTagAndValue contains:
  // like <a href=""value".....
  strValue = strTagAndValue;
  int iStart = strTagAndValue.Find(strTag);
  if (iStart < 0) return ;
  iStart += (int)strTag.size();
  while (strTagAndValue[iStart + 1] == 0x20 || strTagAndValue[iStart + 1] == 0x27 || strTagAndValue[iStart + 1] == 34) iStart++;
  int iEnd = iStart + 1;
  while (strTagAndValue[iEnd] != 0x27 && strTagAndValue[iEnd] != 0x20 && strTagAndValue[iEnd] != 34 && strTagAndValue[iEnd] != '>') iEnd++;
  if (iStart >= 0 && iEnd >= 0)
  {
    strValue = strTagAndValue.Mid(iStart, iEnd - iStart);
  }
}

void CHTMLUtil::RemoveTags(CStdString& strHTML)
{
  int iNested = 0;
  CStdString strReturn = "";
  for (int i = 0; i < (int) strHTML.size(); ++i)
  {
    if (strHTML[i] == '<') iNested++;
    else if (strHTML[i] == '>') iNested--;
    else
    {
      if (!iNested)
      {
        strReturn += strHTML[i];
      }
    }
  }

  strReturn.Replace("&mdash;", "--");
  strReturn.Replace("&#160;", " ");

  strHTML = strReturn;
}

void CHTMLUtil::ConvertHTMLToUTF8(const CStdString& strHTML, string& strStripped)
{
  // TODO UTF8: This assumes the HTML is in the users charset
  ConvertHTMLToAnsi(strHTML, strStripped);
  CStdString utf8String;
  g_charsetConverter.stringCharsetToUtf8(strStripped, utf8String);
  strStripped = utf8String;
}

void CHTMLUtil::ConvertHTMLToAnsi(const CStdString& strHTML, string& strStripped)
{
  int i = 0;
  if (strHTML.size() == 0)
  {
    strStripped = "";
    return ;
  }
  int iAnsiPos = 0;
  char *szAnsi = new char[strHTML.size() * 2];

  while (i < (int)strHTML.size() )
  {
    char kar = strHTML[i];
    if (kar == '&')
    {
      if (strHTML[i + 1] == '#')
      {
        int ipos = 0;
        char szDigit[12];
        i += 2;
        if (strHTML[i + 2] == 'x') i++;

        while ( ipos < 12 && strHTML[i] && isdigit(strHTML[i]))
        {
          szDigit[ipos] = strHTML[i];
          szDigit[ipos + 1] = 0;
          ipos++;
          i++;
        }

        // is it a hex or a decimal string?
        if (strHTML[i + 2] == 'x')
          szAnsi[iAnsiPos++] = (char)(strtol(szDigit, NULL, 16) & 0xFF);
        else
          szAnsi[iAnsiPos++] = (char)(strtol(szDigit, NULL, 10) & 0xFF);
        i++;
      }
      else
      {
        i++;
        int ipos = 0;
        char szKey[112];
        while (strHTML[i] && strHTML[i] != ';' && ipos < 12)
        {
          szKey[ipos] = (unsigned char)strHTML[i];
          szKey[ipos + 1] = 0;
          ipos++;
          i++;
        }
        i++;
        if (strcmp(szKey, "amp") == 0) szAnsi[iAnsiPos++] = '&';
        else if (strcmp(szKey, "quot") == 0) szAnsi[iAnsiPos++] = (char)0x22;
        else if (strcmp(szKey, "frasl") == 0) szAnsi[iAnsiPos++] = (char)0x2F;
        else if (strcmp(szKey, "lt") == 0) szAnsi[iAnsiPos++] = (char)0x3C;
        else if (strcmp(szKey, "gt") == 0) szAnsi[iAnsiPos++] = (char)0x3E;
        else if (strcmp(szKey, "trade") == 0) szAnsi[iAnsiPos++] = (char)0x99;
        else if (strcmp(szKey, "nbsp") == 0) szAnsi[iAnsiPos++] = ' ';
        else if (strcmp(szKey, "iexcl") == 0) szAnsi[iAnsiPos++] = (char)0xA1;
        else if (strcmp(szKey, "cent") == 0) szAnsi[iAnsiPos++] = (char)0xA2;
        else if (strcmp(szKey, "pound") == 0) szAnsi[iAnsiPos++] = (char)0xA3;
        else if (strcmp(szKey, "curren") == 0) szAnsi[iAnsiPos++] = (char)0xA4;
        else if (strcmp(szKey, "yen") == 0) szAnsi[iAnsiPos++] = (char)0xA5;
        else if (strcmp(szKey, "brvbar") == 0) szAnsi[iAnsiPos++] = (char)0xA6;
        else if (strcmp(szKey, "sect") == 0) szAnsi[iAnsiPos++] = (char)0xA7;
        else if (strcmp(szKey, "uml") == 0) szAnsi[iAnsiPos++] = (char)0xA8;
        else if (strcmp(szKey, "copy") == 0) szAnsi[iAnsiPos++] = (char)0xA9;
        else if (strcmp(szKey, "ordf") == 0) szAnsi[iAnsiPos++] = (char)0xAA;
        else if (strcmp(szKey, "laquo") == 0) szAnsi[iAnsiPos++] = (char)0xAB;
        else if (strcmp(szKey, "not") == 0) szAnsi[iAnsiPos++] = (char)0xAC;
        else if (strcmp(szKey, "shy") == 0) szAnsi[iAnsiPos++] = (char)0xAD;
        else if (strcmp(szKey, "reg") == 0) szAnsi[iAnsiPos++] = (char)0xAE;
        else if (strcmp(szKey, "macr") == 0) szAnsi[iAnsiPos++] = (char)0xAF;
        else if (strcmp(szKey, "deg") == 0) szAnsi[iAnsiPos++] = (char)0xB0;
        else if (strcmp(szKey, "plusmn") == 0) szAnsi[iAnsiPos++] = (char)0xB1;
        else if (strcmp(szKey, "sup2") == 0) szAnsi[iAnsiPos++] = (char)0xB2;
        else if (strcmp(szKey, "sup3") == 0) szAnsi[iAnsiPos++] = (char)0xB3;
        else if (strcmp(szKey, "acute") == 0) szAnsi[iAnsiPos++] = (char)0xB4;
        else if (strcmp(szKey, "micro") == 0) szAnsi[iAnsiPos++] = (char)0xB5;
        else if (strcmp(szKey, "para") == 0) szAnsi[iAnsiPos++] = (char)0xB6;
        else if (strcmp(szKey, "middot") == 0) szAnsi[iAnsiPos++] = (char)0xB7;
        else if (strcmp(szKey, "cedil") == 0) szAnsi[iAnsiPos++] = (char)0xB8;
        else if (strcmp(szKey, "sup1") == 0) szAnsi[iAnsiPos++] = (char)0xB9;
        else if (strcmp(szKey, "ordm") == 0) szAnsi[iAnsiPos++] = (char)0xBA;
        else if (strcmp(szKey, "raquo") == 0) szAnsi[iAnsiPos++] = (char)0xBB;
        else if (strcmp(szKey, "frac14") == 0) szAnsi[iAnsiPos++] = (char)0xBC;
        else if (strcmp(szKey, "frac12") == 0) szAnsi[iAnsiPos++] = (char)0xBD;
        else if (strcmp(szKey, "frac34") == 0) szAnsi[iAnsiPos++] = (char)0xBE;
        else if (strcmp(szKey, "iquest") == 0) szAnsi[iAnsiPos++] = (char)0xBF;
        else if (strcmp(szKey, "Agrave") == 0) szAnsi[iAnsiPos++] = (char)0xC0;
        else if (strcmp(szKey, "Aacute") == 0) szAnsi[iAnsiPos++] = (char)0xC1;
        else if (strcmp(szKey, "Acirc") == 0) szAnsi[iAnsiPos++] = (char)0xC2;
        else if (strcmp(szKey, "Atilde") == 0) szAnsi[iAnsiPos++] = (char)0xC3;
        else if (strcmp(szKey, "Auml") == 0) szAnsi[iAnsiPos++] = (char)0xC4;
        else if (strcmp(szKey, "Aring") == 0) szAnsi[iAnsiPos++] = (char)0xC5;
        else if (strcmp(szKey, "AElig") == 0) szAnsi[iAnsiPos++] = (char)0xC6;
        else if (strcmp(szKey, "Ccedil") == 0) szAnsi[iAnsiPos++] = (char)0xC7;
        else if (strcmp(szKey, "Egrave") == 0) szAnsi[iAnsiPos++] = (char)0xC8;
        else if (strcmp(szKey, "Eacute") == 0) szAnsi[iAnsiPos++] = (char)0xC9;
        else if (strcmp(szKey, "Ecirc") == 0) szAnsi[iAnsiPos++] = (char)0xCA;
        else if (strcmp(szKey, "Euml") == 0) szAnsi[iAnsiPos++] = (char)0xCB;
        else if (strcmp(szKey, "Igrave") == 0) szAnsi[iAnsiPos++] = (char)0xCC;
        else if (strcmp(szKey, "Iacute") == 0) szAnsi[iAnsiPos++] = (char)0xCD;
        else if (strcmp(szKey, "Icirc") == 0) szAnsi[iAnsiPos++] = (char)0xCE;
        else if (strcmp(szKey, "Iuml") == 0) szAnsi[iAnsiPos++] = (char)0xCF;
        else if (strcmp(szKey, "ETH") == 0) szAnsi[iAnsiPos++] = (char)0xD0;
        else if (strcmp(szKey, "Ntilde") == 0) szAnsi[iAnsiPos++] = (char)0xD1;
        else if (strcmp(szKey, "Ograve") == 0) szAnsi[iAnsiPos++] = (char)0xD2;
        else if (strcmp(szKey, "Oacute") == 0) szAnsi[iAnsiPos++] = (char)0xD3;
        else if (strcmp(szKey, "Ocirc") == 0) szAnsi[iAnsiPos++] = (char)0xD4;
        else if (strcmp(szKey, "Otilde") == 0) szAnsi[iAnsiPos++] = (char)0xD5;
        else if (strcmp(szKey, "Ouml") == 0) szAnsi[iAnsiPos++] = (char)0xD6;
        else if (strcmp(szKey, "times") == 0) szAnsi[iAnsiPos++] = (char)0xD7;
        else if (strcmp(szKey, "Oslash") == 0) szAnsi[iAnsiPos++] = (char)0xD8;
        else if (strcmp(szKey, "Ugrave") == 0) szAnsi[iAnsiPos++] = (char)0xD9;
        else if (strcmp(szKey, "Uacute") == 0) szAnsi[iAnsiPos++] = (char)0xDA;
        else if (strcmp(szKey, "Ucirc") == 0) szAnsi[iAnsiPos++] = (char)0xDB;
        else if (strcmp(szKey, "Uuml") == 0) szAnsi[iAnsiPos++] = (char)0xDC;
        else if (strcmp(szKey, "Yacute") == 0) szAnsi[iAnsiPos++] = (char)0xDD;
        else if (strcmp(szKey, "THORN") == 0) szAnsi[iAnsiPos++] = (char)0xDE;
        else if (strcmp(szKey, "szlig") == 0) szAnsi[iAnsiPos++] = (char)0xDF;
        else if (strcmp(szKey, "agrave") == 0) szAnsi[iAnsiPos++] = (char)0xE0;
        else if (strcmp(szKey, "aacute") == 0) szAnsi[iAnsiPos++] = (char)0xE1;
        else if (strcmp(szKey, "acirc") == 0) szAnsi[iAnsiPos++] = (char)0xE2;
        else if (strcmp(szKey, "atilde") == 0) szAnsi[iAnsiPos++] = (char)0xE3;
        else if (strcmp(szKey, "auml") == 0) szAnsi[iAnsiPos++] = (char)0xE4;
        else if (strcmp(szKey, "aring") == 0) szAnsi[iAnsiPos++] = (char)0xE5;
        else if (strcmp(szKey, "aelig") == 0) szAnsi[iAnsiPos++] = (char)0xE6;
        else if (strcmp(szKey, "ccedil") == 0) szAnsi[iAnsiPos++] = (char)0xE7;
        else if (strcmp(szKey, "egrave") == 0) szAnsi[iAnsiPos++] = (char)0xE8;
        else if (strcmp(szKey, "eacute") == 0) szAnsi[iAnsiPos++] = (char)0xE9;
        else if (strcmp(szKey, "ecirc") == 0) szAnsi[iAnsiPos++] = (char)0xEA;
        else if (strcmp(szKey, "euml") == 0) szAnsi[iAnsiPos++] = (char)0xEB;
        else if (strcmp(szKey, "igrave") == 0) szAnsi[iAnsiPos++] = (char)0xEC;
        else if (strcmp(szKey, "iacute") == 0) szAnsi[iAnsiPos++] = (char)0xED;
        else if (strcmp(szKey, "icirc") == 0) szAnsi[iAnsiPos++] = (char)0xEE;
        else if (strcmp(szKey, "iuml") == 0) szAnsi[iAnsiPos++] = (char)0xEF;
        else if (strcmp(szKey, "eth") == 0) szAnsi[iAnsiPos++] = (char)0xF0;
        else if (strcmp(szKey, "ntilde") == 0) szAnsi[iAnsiPos++] = (char)0xF1;
        else if (strcmp(szKey, "ograve") == 0) szAnsi[iAnsiPos++] = (char)0xF2;
        else if (strcmp(szKey, "oacute") == 0) szAnsi[iAnsiPos++] = (char)0xF3;
        else if (strcmp(szKey, "ocirc") == 0) szAnsi[iAnsiPos++] = (char)0xF4;
        else if (strcmp(szKey, "otilde") == 0) szAnsi[iAnsiPos++] = (char)0xF5;
        else if (strcmp(szKey, "ouml") == 0) szAnsi[iAnsiPos++] = (char)0xF6;
        else if (strcmp(szKey, "divide") == 0) szAnsi[iAnsiPos++] = (char)0xF7;
        else if (strcmp(szKey, "oslash") == 0) szAnsi[iAnsiPos++] = (char)0xF8;
        else if (strcmp(szKey, "ugrave") == 0) szAnsi[iAnsiPos++] = (char)0xF9;
        else if (strcmp(szKey, "uacute") == 0) szAnsi[iAnsiPos++] = (char)0xFA;
        else if (strcmp(szKey, "ucirc") == 0) szAnsi[iAnsiPos++] = (char)0xFB;
        else if (strcmp(szKey, "uuml") == 0) szAnsi[iAnsiPos++] = (char)0xFC;
        else if (strcmp(szKey, "yacute") == 0) szAnsi[iAnsiPos++] = (char)0xFD;
        else if (strcmp(szKey, "thorn") == 0) szAnsi[iAnsiPos++] = (char)0xFE;
        else if (strcmp(szKey, "yuml") == 0) szAnsi[iAnsiPos++] = (char)0xFF;
        else
        {
          // its not an ampersand code, so just copy the contents
          szAnsi[iAnsiPos++] = '&';
          for (unsigned int iLen=0; iLen<strlen(szKey); iLen++)
            szAnsi[iAnsiPos++] = (unsigned char)szKey[iLen];
        }
      }
    }
    else
    {
      szAnsi[iAnsiPos++] = kar;
      i++;
    }
  }
  szAnsi[iAnsiPos++] = 0;
  strStripped = szAnsi;
  delete [] szAnsi;;
}
