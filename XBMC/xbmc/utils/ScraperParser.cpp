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

#include "stdafx.h"
#include "ScraperParser.h"

#ifdef _LINUX
#include "system.h"
#endif

#include "RegExp.h"
#include "HTMLUtil.h"
#include "CharsetConverter.h"
#include "ScraperSettings.h"
#include "FileSystem/Directory.h"
#include "Util.h"
#include "Settings.h"

#include <sstream>
#include <cstring>

using namespace std;

CScraperParser::CScraperParser()
{
  m_pRootElement = NULL;
  m_name = m_content = NULL;
  m_document = NULL;
  m_settings = NULL;
  m_SearchStringEncoding = "UTF-8";
}

CScraperParser::~CScraperParser()
{
  Clear();
}

void CScraperParser::Clear()
{
  m_pRootElement = NULL;
  if (m_document)
    delete m_document;

  m_document = NULL;
  m_name = m_content = NULL;
  m_settings = NULL;
}

bool CScraperParser::Load(const CStdString& strXMLFile)
{
  Clear();

  m_document = new TiXmlDocument(strXMLFile);

  if (!m_document)
    return false;

  if (m_document->LoadFile())
  {
    m_pRootElement = m_document->RootElement();
    CStdString strValue = m_pRootElement->Value();
    if (strValue != "scraper")
    {
      delete m_document;
      m_document = NULL;
      m_pRootElement = NULL;
      return false;
    }

    m_name = m_pRootElement->Attribute("name");
    m_content = m_pRootElement->Attribute("content");
    m_language = m_pRootElement->Attribute("language");

    if (!m_name || !m_content) // FIXME
    {
      delete m_document;
      m_document = NULL;
      m_pRootElement = NULL;
      return false;
    }
    // check for known content
    if (stricmp(m_content,"tvshows") && stricmp(m_content,"movies") && stricmp(m_content,"musicvideos") && stricmp(m_content,"albums"))
    {
      delete m_document;
      m_document = NULL;
      m_pRootElement = NULL;
      return false;
    }

    TiXmlElement* pChildElement = m_pRootElement->FirstChildElement("CreateSearchUrl");
    if (pChildElement)
    {
      if (!(m_SearchStringEncoding = pChildElement->Attribute("SearchStringEncoding")))
        m_SearchStringEncoding = "UTF-8";
    }
  }
  else
  {
    delete m_document;
    m_document = NULL;
    return false;
  }

  return true;
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
  while ((size_t)(iIndex = strDest.find("$INFO[",iIndex)) != CStdString::npos && m_settings)
  {
    int iEnd = strDest.Find("]",iIndex);
    CStdString strInfo = strDest.Mid(iIndex+6,iEnd-iIndex-6);
    CStdString strReplace = m_settings->Get(strInfo);
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
    CRegExp reg;
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

    const char* szNoClean = pExpression->Attribute("noclean");
    bool bClean[MAX_SCRAPER_BUFFERS];
    for (int iBuf=0;iBuf<MAX_SCRAPER_BUFFERS;++iBuf)
      bClean[iBuf] = true;
    if (szNoClean)
    {
      vector<CStdString> vecBufs;
      CUtil::Tokenize(szNoClean,vecBufs,",");
      for (size_t nToken=0; nToken < vecBufs.size(); nToken++)
        bClean[atoi(vecBufs[nToken].c_str())-1] = false;
    }

    const char* szTrim = pExpression->Attribute("trim");
    bool bTrim[MAX_SCRAPER_BUFFERS];
    for (int iBuf=0;iBuf<MAX_SCRAPER_BUFFERS;++iBuf)
      bTrim[iBuf] = false;
    if (szTrim)
    {
      vector<CStdString> vecBufs;
      CUtil::Tokenize(szTrim,vecBufs,",");
      for (size_t nToken=0; nToken < vecBufs.size(); nToken++)
        bTrim[atoi(vecBufs[nToken].c_str())-1] = true;
    }

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
      {
        char temp[4];
        sprintf(temp,"\\%i",iBuf+1);
        size_t i2=0;
        while ((i2 = strOutput.Find(temp,i2)) != CStdString::npos)
        {
          strOutput.Insert(i2,"!!!CLEAN!!!");
          i2 += 11;
          strOutput.Insert(i2+2,"!!!CLEAN!!!");
          i2 += 2;
        }
      }
      if (bTrim[iBuf])
      {
        char temp[4];
        sprintf(temp,"\\%i",iBuf+1);
        size_t i2=0;
        while ((i2 = strOutput.Find(temp,i2)) != CStdString::npos)
        {
          strOutput.Insert(i2,"!!!TRIM!!!");
          i2 += 10;
          strOutput.Insert(i2+2,"!!!TRIM!!!");
          i2 += 2;
        }
      }
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
        if (szParam)
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
          if ((size_t) strResultNoCase.Find(m_param[iCompare-1]) != CStdString::npos)
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
        if (m_settings)
           strSetting = m_settings->Get(szConditional);
        bExecute = bInverse != strSetting.Equals("true");
      }

      if (bExecute)
        ParseExpression(strInput, m_param[iDest-1],pReg,bAppend);

      pReg = pReg->NextSiblingElement("RegExp");
  }
}

const CStdString CScraperParser::Parse(const CStdString& strTag, CScraperSettings* pSettings)
{
  TiXmlElement* pChildElement = m_pRootElement->FirstChildElement(strTag.c_str());
  if(pChildElement == NULL) return "";
  int iResult = 1; // default to param 1
  pChildElement->QueryIntAttribute("dest",&iResult);
  TiXmlElement* pChildStart = pChildElement->FirstChildElement("RegExp");
  if (pSettings)
    m_settings = pSettings;
  else
    m_settings = NULL;
  ParseNext(pChildStart);
  CStdString tmp = m_param[iResult-1];

  const char* szClearBuffers = pChildElement->Attribute("clearbuffers");
  if (!szClearBuffers || stricmp(szClearBuffers,"no") != 0)
    ClearBuffers();

  return tmp;
}

bool CScraperParser::HasFunction(const CStdString& strTag)
{
  TiXmlElement* pChildElement = m_pRootElement->FirstChildElement(strTag.c_str());

  if (!pChildElement)
    return false;

  return true;
}

void CScraperParser::Clean(CStdString& strDirty)
{
  size_t i=0;
  CStdString strBuffer;
  while ((i=strDirty.Find("!!!CLEAN!!!",i)) != CStdString::npos)
  {
    size_t i2;
    if ((i2=strDirty.Find("!!!CLEAN!!!",i+11)) != CStdString::npos)
    {
      strBuffer = strDirty.substr(i+11,i2-i-11);
      CStdString strConverted(strBuffer);
      HTML::CHTMLUtil::RemoveTags(strConverted);
      const char* szTrimmed = RemoveWhiteSpace(strConverted.c_str());
      strDirty.erase(i,i2-i+11);
      strDirty.Insert(i,szTrimmed);
      i += strlen(szTrimmed);
    }
    else
      break;
  }
  i=0;
  while ((i=strDirty.Find("!!!TRIM!!!",i)) != CStdString::npos)
  {
    size_t i2;
    if ((i2=strDirty.Find("!!!TRIM!!!",i+10)) != CStdString::npos)
    {
      strBuffer = strDirty.substr(i+10,i2-i-10);
      const char* szTrimmed = RemoveWhiteSpace(strBuffer.c_str());
      strDirty.erase(i,i2-i+10);
      strDirty.Insert(i,szTrimmed);
      i += strlen(szTrimmed);
    }
    else
      break;
  }
}

char* CScraperParser::RemoveWhiteSpace(const char *string2)
{
  if (!string2) return (char*)"";
  char* string = (char*)string2;
  size_t pos = strlen(string)-1;
  while ((string[pos] == ' ' || string[pos] == '\n') && string[pos] && pos)
    string[pos--] = '\0';
  while ((*string == ' ' || *string == '\n') && *string != '\0')
    string++;
  return string;
}

void CScraperParser::ClearBuffers()
{
  //clear all m_param strings
  for (int i=0;i<MAX_SCRAPER_BUFFERS;++i)
    m_param[i].clear();
}

void CScraperParser::ClearCache()
{
  // wipe cache
  CStdString strCachePath;
  CUtil::AddFileToFolder(g_advancedSettings.m_cachePath,"scrapers",strCachePath);
  CUtil::WipeDir(strCachePath);
  DIRECTORY::CDirectory::Create(strCachePath);
}

