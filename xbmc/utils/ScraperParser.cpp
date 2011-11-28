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

#include "ScraperParser.h"

#include "addons/AddonManager.h"
#include "RegExp.h"
#include "HTMLUtil.h"
#include "addons/Scraper.h"
#include "URL.h"
#include "Util.h"
#include "log.h"
#include "CharsetConverter.h"

#include <sstream>
#include <cstring>

using namespace std;
using namespace ADDON;
using namespace XFILE;

CScraperParser::CScraperParser()
{
  m_pRootElement = NULL;
  m_document = NULL;
  m_SearchStringEncoding = "UTF-8";
}

CScraperParser::CScraperParser(const CScraperParser& parser)
{
  m_document = NULL;
  m_SearchStringEncoding = "UTF-8";
  *this = parser;
}

CScraperParser &CScraperParser::operator=(const CScraperParser &parser)
{
  if (this != &parser)
  {
    Clear();
    if (parser.m_document)
    {
      m_scraper = parser.m_scraper;
      m_document = new TiXmlDocument(*parser.m_document);
      LoadFromXML();
    }
  }
  return *this;
}

CScraperParser::~CScraperParser()
{
  Clear();
}

void CScraperParser::Clear()
{
  m_pRootElement = NULL;
  delete m_document;

  m_document = NULL;
  m_strFile.Empty();
}

bool CScraperParser::Load(const CStdString& strXMLFile)
{
  Clear();

  m_document = new TiXmlDocument(strXMLFile);

  if (!m_document)
    return false;

  m_strFile = strXMLFile;

  if (m_document->LoadFile())
    return LoadFromXML();

  delete m_document;
  m_document = NULL;
  return false;
}

bool CScraperParser::LoadFromXML()
{
  if (!m_document)
    return false;

  m_pRootElement = m_document->RootElement();
  CStdString strValue = m_pRootElement->Value();
  if (strValue == "scraper")
  {
    TiXmlElement* pChildElement = m_pRootElement->FirstChildElement("CreateSearchUrl");
    if (pChildElement)
    {
      if (!(m_SearchStringEncoding = pChildElement->Attribute("SearchStringEncoding")))
        m_SearchStringEncoding = "UTF-8";
    }

    pChildElement = m_pRootElement->FirstChildElement("CreateArtistSearchUrl");
    if (pChildElement)
    {
      if (!(m_SearchStringEncoding = pChildElement->Attribute("SearchStringEncoding")))
        m_SearchStringEncoding = "UTF-8";
    }
    pChildElement = m_pRootElement->FirstChildElement("CreateAlbumSearchUrl");
    if (pChildElement)
    {
      if (!(m_SearchStringEncoding = pChildElement->Attribute("SearchStringEncoding")))
        m_SearchStringEncoding = "UTF-8";
    }

    return true;
  }

  delete m_document;
  m_document = NULL;
  m_pRootElement = NULL;
  return false;
}

void CScraperParser::ReplaceBuffers(CStdString& strDest)
{
  // insert buffers
  int iIndex;
  for (int i=MAX_SCRAPER_BUFFERS-1; i>=0; i--)
  {
    CStdString temp;
    iIndex = 0;
    temp.Format("$$%i",i+1);
    while ((size_t)(iIndex = strDest.find(temp,iIndex)) != CStdString::npos) // COPIED FROM CStdString WITH THE ADDITION OF $ ESCAPING
    {
      strDest.replace(strDest.begin()+iIndex,strDest.begin()+iIndex+temp.GetLength(),m_param[i]);
      iIndex += m_param[i].length();
    }
  }
  // insert settings
  iIndex = 0;
  while ((size_t)(iIndex = strDest.find("$INFO[",iIndex)) != CStdString::npos)
  {
    int iEnd = strDest.Find("]",iIndex);
    CStdString strInfo = strDest.Mid(iIndex+6,iEnd-iIndex-6);
    CStdString strReplace;
    if (m_scraper)
      strReplace = m_scraper->GetSetting(strInfo);
    strDest.replace(strDest.begin()+iIndex,strDest.begin()+iEnd+1,strReplace);
    iIndex += strReplace.length();
  }
  iIndex = 0;
  while ((size_t)(iIndex = strDest.find("\\n",iIndex)) != CStdString::npos)
    strDest.replace(strDest.begin()+iIndex,strDest.begin()+iIndex+2,"\n");
}

void CScraperParser::ParseExpression(const CStdString& input, CStdString& dest, TiXmlElement* element, bool bAppend)
{
  CStdString strOutput = element->Attribute("output");

  TiXmlElement* pExpression = element->FirstChildElement("expression");
  if (pExpression)
  {
    bool bInsensitive=true;
    const char* sensitive = pExpression->Attribute("cs");
    if (sensitive)
      if (stricmp(sensitive,"yes") == 0)
        bInsensitive=false; // match case sensitive

    CRegExp reg(bInsensitive);
    CStdString strExpression;
    if (pExpression->FirstChild())
      strExpression = pExpression->FirstChild()->Value();
    else
      strExpression = "(.*)";
    ReplaceBuffers(strExpression);
    ReplaceBuffers(strOutput);

    if (!reg.RegComp(strExpression.c_str()))
    {
      return;
    }

    bool bRepeat = false;
    const char* szRepeat = pExpression->Attribute("repeat");
    if (szRepeat)
      if (stricmp(szRepeat,"yes") == 0)
        bRepeat = true;

    const char* szClear = pExpression->Attribute("clear");
    if (szClear)
      if (stricmp(szClear,"yes") == 0)
        dest=""; // clear no matter if regexp fails

    bool bClean[MAX_SCRAPER_BUFFERS];
    GetBufferParams(bClean,pExpression->Attribute("noclean"),true);

    bool bTrim[MAX_SCRAPER_BUFFERS];
    GetBufferParams(bTrim,pExpression->Attribute("trim"),false);

    bool bFixChars[MAX_SCRAPER_BUFFERS];
    GetBufferParams(bFixChars,pExpression->Attribute("fixchars"),false);

    bool bEncode[MAX_SCRAPER_BUFFERS];
    GetBufferParams(bEncode,pExpression->Attribute("encode"),false);

    int iOptional = -1;
    pExpression->QueryIntAttribute("optional",&iOptional);

    int iCompare = -1;
    pExpression->QueryIntAttribute("compare",&iCompare);
    if (iCompare > -1)
      m_param[iCompare-1].ToLower();
    CStdString curInput = input;
    for (int iBuf=0;iBuf<MAX_SCRAPER_BUFFERS;++iBuf)
    {
      if (bClean[iBuf])
        InsertToken(strOutput,iBuf+1,"!!!CLEAN!!!");
      if (bTrim[iBuf])
        InsertToken(strOutput,iBuf+1,"!!!TRIM!!!");
      if (bFixChars[iBuf])
        InsertToken(strOutput,iBuf+1,"!!!FIXCHARS!!!");
      if (bEncode[iBuf])
        InsertToken(strOutput,iBuf+1,"!!!ENCODE!!!");
    }
    int i = reg.RegFind(curInput.c_str());
    while (i > -1 && (i < (int)curInput.size() || curInput.size() == 0))
    {
      if (!bAppend)
      {
        dest = "";
        bAppend = true;
      }
      CStdString strCurOutput=strOutput;

      if (iOptional > -1) // check that required param is there
      {
        char temp[4];
        sprintf(temp,"\\%i",iOptional);
        char* szParam = reg.GetReplaceString(temp);
        CRegExp reg2;
        reg2.RegComp("(.*)(\\\\\\(.*\\\\2.*)\\\\\\)(.*)");
        int i2=reg2.RegFind(strCurOutput.c_str());
        while (i2 > -1)
        {
          char* szRemove = reg2.GetReplaceString("\\2");
          int iRemove = strlen(szRemove);
          int i3 = strCurOutput.find(szRemove);
          if (szParam && strcmp(szParam,""))
          {
            strCurOutput.erase(i3+iRemove,2);
            strCurOutput.erase(i3,2);
          }
          else
            strCurOutput.replace(strCurOutput.begin()+i3,strCurOutput.begin()+i3+iRemove+2,"");

          free(szRemove);

          i2 = reg2.RegFind(strCurOutput.c_str());
        }
        free(szParam);
      }

      int iLen = reg.GetFindLen();
      // nasty hack #1 - & means \0 in a replace string
      strCurOutput.Replace("&","!!!AMPAMP!!!");
      char* result = reg.GetReplaceString(strCurOutput.c_str());
      if (result && strlen(result))
      {
        CStdString strResult(result);
        strResult.Replace("!!!AMPAMP!!!","&");
        Clean(strResult);
        ReplaceBuffers(strResult);
        if (iCompare > -1)
        {
          CStdString strResultNoCase = strResult;
          strResultNoCase.ToLower();
          if (strResultNoCase.Find(m_param[iCompare-1]) != -1)
            dest += strResult;
        }
        else
          dest += strResult;

        free(result);
      }
      if (bRepeat && iLen > 0)
      {
        curInput.erase(0,i+iLen>(int)curInput.size()?curInput.size():i+iLen);
        i = reg.RegFind(curInput.c_str());
      }
      else
        i = -1;
    }
  }
}

void CScraperParser::ParseNext(TiXmlElement* element)
{
  TiXmlElement* pReg = element;
  while (pReg)
  {
    TiXmlElement* pChildReg = pReg->FirstChildElement("RegExp");
    if (pChildReg)
      ParseNext(pChildReg);
    else
    {
      TiXmlElement* pChildReg = pReg->FirstChildElement("clear");
      if (pChildReg)
        ParseNext(pChildReg);
    }

    int iDest = 1;
    bool bAppend = false;
    const char* szDest = pReg->Attribute("dest");
    if (szDest)
      if (strlen(szDest))
      {
        if (szDest[strlen(szDest)-1] == '+')
          bAppend = true;

        iDest = atoi(szDest);
      }

      const char *szInput = pReg->Attribute("input");
      CStdString strInput;
      if (szInput)
      {
        strInput = szInput;
        ReplaceBuffers(strInput);
      }
      else
        strInput = m_param[0];

      const char* szConditional = pReg->Attribute("conditional");
      bool bExecute = true;
      if (szConditional)
      {
        bool bInverse=false;
        if (szConditional[0] == '!')
        {
          bInverse = true;
          szConditional++;
        }
        CStdString strSetting;
        if (m_scraper && m_scraper->HasSettings())
           strSetting = m_scraper->GetSetting(szConditional);
        bExecute = bInverse != strSetting.Equals("true");
      }

      if (bExecute)
      {
        if (iDest-1 < MAX_SCRAPER_BUFFERS && iDest-1 > -1)
          ParseExpression(strInput, m_param[iDest-1],pReg,bAppend);
        else
          CLog::Log(LOGERROR,"CScraperParser::ParseNext: destination buffer "
                             "out of bounds, skipping expression");
      }

      pReg = pReg->NextSiblingElement("RegExp");
  }
}

const CStdString CScraperParser::Parse(const CStdString& strTag,
                                       CScraper* scraper)
{
  TiXmlElement* pChildElement = m_pRootElement->FirstChildElement(strTag.c_str());
  if(pChildElement == NULL)
  {
    CLog::Log(LOGERROR,"%s: Could not find scraper function %s",__FUNCTION__,strTag.c_str());
    return "";
  }
  int iResult = 1; // default to param 1
  pChildElement->QueryIntAttribute("dest",&iResult);
  TiXmlElement* pChildStart = pChildElement->FirstChildElement("RegExp");
  m_scraper = scraper;
  ParseNext(pChildStart);
  CStdString tmp = m_param[iResult-1];

  const char* szClearBuffers = pChildElement->Attribute("clearbuffers");
  if (!szClearBuffers || stricmp(szClearBuffers,"no") != 0)
    ClearBuffers();

  return tmp;
}

void CScraperParser::Clean(CStdString& strDirty)
{
  int i=0;
  CStdString strBuffer;
  while ((i=strDirty.Find("!!!CLEAN!!!",i)) != -1)
  {
    int i2;
    if ((i2=strDirty.Find("!!!CLEAN!!!",i+11)) != -1)
    {
      strBuffer = strDirty.substr(i+11,i2-i-11);
      CStdString strConverted(strBuffer);
      HTML::CHTMLUtil::RemoveTags(strConverted);
      RemoveWhiteSpace(strConverted);
      strDirty.erase(i,i2-i+11);
      strDirty.Insert(i,strConverted);
      i += strConverted.size();
    }
    else
      break;
  }
  i=0;
  while ((i=strDirty.Find("!!!TRIM!!!",i)) != -1)
  {
    int i2;
    if ((i2=strDirty.Find("!!!TRIM!!!",i+10)) != -1)
    {
      strBuffer = strDirty.substr(i+10,i2-i-10);
      RemoveWhiteSpace(strBuffer);
      strDirty.erase(i,i2-i+10);
      strDirty.Insert(i,strBuffer);
      i += strBuffer.size();
    }
    else
      break;
  }
  i=0;
  while ((i=strDirty.Find("!!!FIXCHARS!!!",i)) != -1)
  {
    int i2;
    if ((i2=strDirty.Find("!!!FIXCHARS!!!",i+14)) != -1)
    {
      strBuffer = strDirty.substr(i+14,i2-i-14);
      CStdStringW wbuffer;
      g_charsetConverter.toW(strBuffer,wbuffer,GetSearchStringEncoding());
      CStdStringW wConverted;
      HTML::CHTMLUtil::ConvertHTMLToW(wbuffer,wConverted);
      g_charsetConverter.fromW(wConverted,strBuffer,GetSearchStringEncoding());
      RemoveWhiteSpace(strBuffer);
      ConvertJSON(strBuffer);
      strDirty.erase(i,i2-i+14);
      strDirty.Insert(i,strBuffer);
      i += strBuffer.size();
    }
    else
      break;
  }
  i=0;
  while ((i=strDirty.Find("!!!ENCODE!!!",i)) != -1)
  {
    int i2;
    if ((i2=strDirty.Find("!!!ENCODE!!!",i+12)) != -1)
    {
      strBuffer = strDirty.substr(i+12,i2-i-12);
      CURL::Encode(strBuffer);
      strDirty.erase(i,i2-i+12);
      strDirty.Insert(i,strBuffer);
      i += strBuffer.size();
    }
    else
      break;
  }
}

void CScraperParser::RemoveWhiteSpace(CStdString &string)
{
  string.TrimLeft(" \t\r\n");
  string.TrimRight(" \t\r\n");
}

void CScraperParser::ConvertJSON(CStdString &string)
{
  CRegExp reg;
  reg.RegComp("\\\\u([0-f]{4})");
  while (reg.RegFind(string.c_str()) > -1)
  {
    int pos = reg.GetSubStart(1);
    char* szReplace = reg.GetReplaceString("\\1");

    CStdString replace;
    replace.Format("&#x%s;", szReplace);
    string.replace(string.begin()+pos-2, string.begin()+pos+4, replace);

    free(szReplace);
  }

  CRegExp reg2;
  reg2.RegComp("\\\\x([0-9]{2})([^\\\\]+;)");
  while (reg2.RegFind(string.c_str()) > -1)
  {
    int pos1 = reg2.GetSubStart(1);
    int pos2 = reg2.GetSubStart(2);
    char* szHexValue = reg2.GetReplaceString("\\1");

    CStdString replace;
    replace.Format("%c", strtol(szHexValue, NULL, 16));
    string.replace(string.begin()+pos1-2, string.begin()+pos2+reg2.GetSubLength(2), replace);

    free(szHexValue);
  }

  string.Replace("\\\"","\"");
}

void CScraperParser::ClearBuffers()
{
  //clear all m_param strings
  for (int i=0;i<MAX_SCRAPER_BUFFERS;++i)
    m_param[i].clear();
}

void CScraperParser::GetBufferParams(bool* result, const char* attribute, bool defvalue)
{
  for (int iBuf=0;iBuf<MAX_SCRAPER_BUFFERS;++iBuf)
    result[iBuf] = defvalue;;
  if (attribute)
  {
    vector<CStdString> vecBufs;
    CUtil::Tokenize(attribute,vecBufs,",");
    for (size_t nToken=0; nToken < vecBufs.size(); nToken++)
    {
      int index = atoi(vecBufs[nToken].c_str())-1;
      if (index < MAX_SCRAPER_BUFFERS)
        result[index] = !defvalue;
    }
  }
}

void CScraperParser::InsertToken(CStdString& strOutput, int buf, const char* token)
{
  char temp[4];
  sprintf(temp,"\\%i",buf);
  int i2=0;
  while ((i2 = strOutput.Find(temp,i2)) != -1)
  {
    strOutput.Insert(i2,token);
    i2 += strlen(token);
    strOutput.Insert(i2+strlen(temp),token);
    i2 += strlen(temp);
  }
}

void CScraperParser::AddDocument(const TiXmlDocument* doc)
{
  const TiXmlNode* node = doc->RootElement()->FirstChild();
  while (node)
  {
    m_pRootElement->InsertEndChild(*node);
    node = node->NextSibling();
  }
}

