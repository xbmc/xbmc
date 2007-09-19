#include "ScraperParser.h"

#ifdef _LINUX
#include "system.h"
#endif

#include <cstring>
#include "RegExp.h"
#include "HTMLUtil.h"
#include "CharsetConverter.h"
#include "Util.h"

#include <sstream>

CScraperUrl::CScraperUrl(const CStdString& strUrl)
{
  ParseString(strUrl);
}

CScraperUrl::CScraperUrl(const TiXmlElement* element)
{
  ParseElement(element);
}

CScraperUrl::CScraperUrl()
{
}

CScraperUrl::~CScraperUrl()
{
}

void CScraperUrl::Clear()
{
  m_url.clear();
  m_spoof.clear();
  m_xml.clear();
}

bool CScraperUrl::Parse()
{
  return ParseString(m_xml);
}

bool CScraperUrl::ParseElement(const TiXmlElement* element)
{
  if (!element || !element->FirstChild()) return false;

  m_url.clear();
  std::stringstream stream;
  stream << *element;
  m_xml = stream.str();
  if (element->FirstChildElement("thumb"))
    element = element->FirstChildElement("thumb");
  while (element)
  {
    SUrlEntry url;
    url.m_url = element->FirstChild()->Value();
    const char* pSpoof = element->Attribute("spoof");
    if (pSpoof)
      url.m_spoof = pSpoof;
    const char* szPost=element->Attribute("post");
    if (szPost && stricmp(szPost,"yes") == 0) 
      url.m_post = true;
    else
      url.m_post = false;
    const char* szType = element->Attribute("type");
    url.m_type = URL_TYPE_GENERAL;
    if (szType && stricmp(szType,"season") == 0)
    {
      url.m_type = URL_TYPE_SEASON;
      const char* szSeason = element->Attribute("season");
      if (szSeason)
        url.m_season = atoi(szSeason);
      else
        url.m_season = -1;
    }
    else
      url.m_season = -1;

    m_url.push_back(url);
    element = element->NextSiblingElement("thumb");
  }
  return true;
}

bool CScraperUrl::ParseString(CStdString strUrl)
{
  if (strUrl.IsEmpty())
    return false;
  
  // ok, now parse the xml file
  if (strUrl.Find("encoding=\"utf-8\"") < 0)
    g_charsetConverter.stringCharsetToUtf8(strUrl);
  
  TiXmlDocument doc;
  doc.Parse(strUrl.c_str(),0,TIXML_ENCODING_UTF8);
  m_xml = strUrl;

  TiXmlElement* pElement = doc.RootElement();
  if (pElement)
    ParseElement(pElement);
  else
  {
    SUrlEntry url;
    url.m_url = strUrl;
    url.m_type = URL_TYPE_GENERAL;
    url.m_season = -1;
    url.m_post = false;
    m_url.push_back(url);
  }
  return true;
}

const CScraperUrl::SUrlEntry CScraperUrl::GetFirstThumb() const
{
  for (std::vector<SUrlEntry>::const_iterator iter=m_url.begin();iter != m_url.end();++iter)
  {
    if (iter->m_type == URL_TYPE_GENERAL)
      return *iter;
  }
  SUrlEntry result;
  result.m_season = -1;
  return result;
}

const CScraperUrl::SUrlEntry CScraperUrl::GetSeasonThumb(int season) const
{
  for (std::vector<SUrlEntry>::const_iterator iter=m_url.begin();iter != m_url.end();++iter)
  {
    if (iter->m_type == URL_TYPE_SEASON && iter->m_season == season)
      return *iter;
  }
  SUrlEntry result;
  result.m_season = -1;
  return result;
}

CScraperParser::CScraperParser()
{
  m_pRootElement = NULL;
  m_name = m_content = NULL;
  m_document = NULL;
}

CScraperParser::~CScraperParser()
{
  m_pRootElement = NULL;
  if (m_document)
    delete m_document;

  m_document = NULL;
  m_name = m_content = NULL;
}

bool CScraperParser::Load(const CStdString& strXMLFile)
{
  if (m_document)
    return true;

  m_document = new TiXmlDocument(_P(strXMLFile).c_str());

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

    if (!m_name || !m_content) // FIXME
    {
      delete m_document;
      m_document = NULL;
      m_pRootElement = NULL;
      return false;
    }
    // check for known content
    if (stricmp(m_content,"tvshows") && stricmp(m_content,"movies") && stricmp(m_content,"musicvideos"))
    {
      delete m_document;
      m_document = NULL;
      m_pRootElement = NULL;
      return false;
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
  char temp[5];
  for (int i=0;i<9;++i)
  {
    sprintf(temp,"$$%i",i+1);
    int iIndex = 0;
    while ((iIndex = strDest.find(temp,iIndex)) != CStdString::npos) // COPIED FROM CStdString WITH THE ADDITION OF $ ESCAPING
    {
      strDest.replace(strDest.begin()+iIndex,strDest.begin()+iIndex+strlen(temp),m_param[i]);
      iIndex += m_param[i].length();
    }
  }
  int iIndex = 0;
  while ((iIndex = strDest.find("\\n",iIndex))!=CStdString::npos)
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
      //std::cout << "error compiling regexp in scraper";
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
    bool bClean[10];
    for (int iBuf=0;iBuf<10;++iBuf)
      bClean[iBuf] = true;
    if (szNoClean)
    {
      int iChar=0;
      while (iChar > -1 && iChar < (int)strlen(szNoClean))
      {   
        char temp[3];
        if (szNoClean[iChar] <= '9' && szNoClean[iChar] >= '0')
        {
          temp[0] = szNoClean[iChar++];
          int j=1;
          if (szNoClean[iChar] <= '9' && szNoClean[iChar] >= '0')
            temp[j++] = szNoClean[iChar++];

          temp[j] = '\0';
        }
        else
          break;

        int param=atoi(temp);
        if (!param--)
        {
          iChar = -1;
          break;
        }
        //CLog::Log(LOGDEBUG,"not cleaning %i",param+1);
        bClean[param] = false;
        if (szNoClean[iChar++]!= ',')
          iChar = -1;
      }
    }

    const char* szTrim = pExpression->Attribute("trim");
    bool bTrim[10];
    for (int iBuf=0;iBuf<10;++iBuf)
      bTrim[iBuf] = false;
    if (szTrim)
    {
      int iChar=0;
      while (iChar > -1 && iChar < (int)strlen(szTrim))
      {   
        char temp[3];
        if (szTrim[iChar] <= '9' && szTrim[iChar] >= '0')
        {
          temp[0] = szTrim[iChar++];
          int j=1;
          if (szTrim[iChar] <= '9' && szTrim[iChar] >= '0')
            temp[j++] = szTrim[iChar++];

          temp[j] = '\0';
        }
        else
          break;

        int param=atoi(temp);
        if (!param--)
        {
          iChar = -1;
          break;
        }
        //CLog::Log(LOGDEBUG,"not cleaning %i",param+1);
        bTrim[param] = true;
        if (szTrim[iChar++]!= ',')
          iChar = -1;
      }
    }

    int iOptional = -1;
    pExpression->QueryIntAttribute("optional",&iOptional);

    int iCompare = -1;
    pExpression->QueryIntAttribute("compare",&iCompare);
    if (iCompare > -1)
      m_param[iCompare-1].ToLower();
    CStdString curInput = input;
    for (int iBuf=0;iBuf<9;++iBuf)
    {
      if (bClean[iBuf])
      {
        char temp[4];
        sprintf(temp,"\\%i",iBuf+1);
        int i2=0;
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
        int i2=0;
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
    int iPos=0;
    while (i > -1 && i < (int)curInput.size())
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
          if (strResultNoCase.Find(m_param[iCompare-1]) != CStdString::npos)
            dest += strResult;
        }
        else
          dest += strResult;

        free(result);
      }
      if (bRepeat)
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

      ParseExpression(strInput, m_param[iDest-1],pReg,bAppend);
      pReg = pReg->NextSiblingElement("RegExp");
  }
}

const CStdString CScraperParser::Parse(const CStdString& strTag)
{
  TiXmlElement* pChildElement = m_pRootElement->FirstChildElement(strTag.c_str());
  if(pChildElement == NULL) return "";
  int iResult = 1; // default to param 1
  pChildElement->QueryIntAttribute("dest",&iResult);
  TiXmlElement* pChildStart = pChildElement->FirstChildElement("RegExp");
  ParseNext(pChildStart);

  CStdString tmp = m_param[iResult-1];
  ClearBuffers(); 
  return tmp;
}

void CScraperParser::Clean(CStdString& strDirty)
{
  int i=0;
  CStdString strBuffer;
  while ((i=strDirty.Find("!!!CLEAN!!!",i)) != CStdString::npos)
  {
    int i2;
    if ((i2=strDirty.Find("!!!CLEAN!!!",i+11)) != CStdString::npos)
    {
      strBuffer = strDirty.substr(i+11,i2-i-11);
      //char* szConverted = ConvertHTMLToAnsi(strBuffer.c_str());
      //const char* szTrimmed = RemoveWhiteSpace(szConverted);
      CStdString strConverted(strBuffer);
      HTML::CHTMLUtil::RemoveTags(strConverted);
      const char* szTrimmed = RemoveWhiteSpace(strConverted.c_str());
      strDirty.erase(i,i2-i+11);
      strDirty.Insert(i,szTrimmed);
      i += strlen(szTrimmed);
      //free(szConverted);
    }
    else
      break;
  }
  i=0;
  while ((i=strDirty.Find("!!!TRIM!!!",i)) != CStdString::npos)
  {
    int i2;
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

char* CScraperParser::ConvertHTMLToAnsi(const char *szHTML)
{
  if (!szHTML)
    return NULL;

  int i=0; 
  int len = (int)strlen(szHTML);
  if (len == 0)
    return NULL;

  int iAnsiPos=0;
  char *szAnsi = (char *)malloc(len*2*sizeof(char));

  while (i < len)
  {
    char kar=szHTML[i];
    if (kar=='&')
    {
      if (szHTML[i+1]=='#')
      {
        int ipos=0;
        char szDigit[12];
        i+=2;
        if (szHTML[i+2]=='x') i++;

        while ( ipos < 12 && szHTML[i] && isdigit(szHTML[i])) 
        {
          szDigit[ipos]=szHTML[i];
          szDigit[ipos+1]=0;
          ipos++;
          i++;
        }

        // is it a hex or a decimal string?
        if (szHTML[i+2]=='x')
          szAnsi[iAnsiPos++] = (char)(strtol(szDigit, NULL, 16) & 0xFF);
        else
          szAnsi[iAnsiPos++] = (char)(strtol(szDigit, NULL, 10) & 0xFF);
        i++;
      }
      else
      {
        i++;
        int ipos=0;
        char szKey[112];
        while (szHTML[i] && szHTML[i] != ';' && ipos < 12)
        {
          szKey[ipos]=(unsigned char)szHTML[i];
          szKey[ipos+1]=0;
          ipos++;
          i++;
        }
        i++;
        if (strcmp(szKey,"amp")==0)          szAnsi[iAnsiPos++] = '&';
        else if (strcmp(szKey,"quot")==0)    szAnsi[iAnsiPos++] = (char)0x22;
        else if (strcmp(szKey,"frasl")==0)   szAnsi[iAnsiPos++] = (char)0x2F;
        else if (strcmp(szKey,"lt")==0)      szAnsi[iAnsiPos++] = (char)0x3C;
        else if (strcmp(szKey,"gt")==0)      szAnsi[iAnsiPos++] = (char)0x3E;
        else if (strcmp(szKey,"trade")==0)   szAnsi[iAnsiPos++] = (char)0x99;
        else if (strcmp(szKey,"nbsp")==0)    szAnsi[iAnsiPos++] = ' ';
        else if (strcmp(szKey,"iexcl")==0)   szAnsi[iAnsiPos++] = (char)0xA1;
        else if (strcmp(szKey,"cent")==0)    szAnsi[iAnsiPos++] = (char)0xA2;
        else if (strcmp(szKey,"pound")==0)   szAnsi[iAnsiPos++] = (char)0xA3;
        else if (strcmp(szKey,"curren")==0)  szAnsi[iAnsiPos++] = (char)0xA4;
        else if (strcmp(szKey,"yen")==0)     szAnsi[iAnsiPos++] = (char)0xA5;
        else if (strcmp(szKey,"brvbar")==0)  szAnsi[iAnsiPos++] = (char)0xA6;
        else if (strcmp(szKey,"sect")==0)    szAnsi[iAnsiPos++] = (char)0xA7;
        else if (strcmp(szKey,"uml")==0)     szAnsi[iAnsiPos++] = (char)0xA8;
        else if (strcmp(szKey,"copy")==0)    szAnsi[iAnsiPos++] = (char)0xA9;
        else if (strcmp(szKey,"ordf")==0)    szAnsi[iAnsiPos++] = (char)0xAA;
        else if (strcmp(szKey,"laquo")==0)   szAnsi[iAnsiPos++] = (char)0xAB;
        else if (strcmp(szKey,"not")==0)     szAnsi[iAnsiPos++] = (char)0xAC;
        else if (strcmp(szKey,"shy")==0)     szAnsi[iAnsiPos++] = (char)0xAD;
        else if (strcmp(szKey,"reg")==0)     szAnsi[iAnsiPos++] = (char)0xAE;
        else if (strcmp(szKey,"macr")==0)    szAnsi[iAnsiPos++] = (char)0xAF;
        else if (strcmp(szKey,"deg")==0)     szAnsi[iAnsiPos++] = (char)0xB0;
        else if (strcmp(szKey,"plusmn")==0)  szAnsi[iAnsiPos++] = (char)0xB1;
        else if (strcmp(szKey,"sup2")==0)    szAnsi[iAnsiPos++] = (char)0xB2;
        else if (strcmp(szKey,"sup3")==0)    szAnsi[iAnsiPos++] = (char)0xB3;
        else if (strcmp(szKey,"acute")==0)   szAnsi[iAnsiPos++] = (char)0xB4;
        else if (strcmp(szKey,"micro")==0)   szAnsi[iAnsiPos++] = (char)0xB5;
        else if (strcmp(szKey,"para")==0)    szAnsi[iAnsiPos++] = (char)0xB6;
        else if (strcmp(szKey,"middot")==0)  szAnsi[iAnsiPos++] = (char)0xB7;
        else if (strcmp(szKey,"cedil")==0)   szAnsi[iAnsiPos++] = (char)0xB8;
        else if (strcmp(szKey,"sup1")==0)    szAnsi[iAnsiPos++] = (char)0xB9;
        else if (strcmp(szKey,"ordm")==0)    szAnsi[iAnsiPos++] = (char)0xBA;
        else if (strcmp(szKey,"raquo")==0)   szAnsi[iAnsiPos++] = (char)0xBB;
        else if (strcmp(szKey,"frac14")==0)  szAnsi[iAnsiPos++] = (char)0xBC;
        else if (strcmp(szKey,"frac12")==0)  szAnsi[iAnsiPos++] = (char)0xBD;
        else if (strcmp(szKey,"frac34")==0)  szAnsi[iAnsiPos++] = (char)0xBE;
        else if (strcmp(szKey,"iquest")==0)  szAnsi[iAnsiPos++] = (char)0xBF;
        else if (strcmp(szKey,"Agrave")==0)  szAnsi[iAnsiPos++] = (char)0xC0;
        else if (strcmp(szKey,"Aacute")==0)  szAnsi[iAnsiPos++] = (char)0xC1;
        else if (strcmp(szKey,"Acirc")==0)   szAnsi[iAnsiPos++] = (char)0xC2;
        else if (strcmp(szKey,"Atilde")==0)  szAnsi[iAnsiPos++] = (char)0xC3;
        else if (strcmp(szKey,"Auml")==0)    szAnsi[iAnsiPos++] = (char)0xC4;
        else if (strcmp(szKey,"Aring")==0)   szAnsi[iAnsiPos++] = (char)0xC5;
        else if (strcmp(szKey,"AElig")==0)   szAnsi[iAnsiPos++] = (char)0xC6;
        else if (strcmp(szKey,"Ccedil")==0)  szAnsi[iAnsiPos++] = (char)0xC7;
        else if (strcmp(szKey,"Egrave")==0)  szAnsi[iAnsiPos++] = (char)0xC8;
        else if (strcmp(szKey,"Eacute")==0)  szAnsi[iAnsiPos++] = (char)0xC9;
        else if (strcmp(szKey,"Ecirc")==0)   szAnsi[iAnsiPos++] = (char)0xCA;
        else if (strcmp(szKey,"Euml")==0)    szAnsi[iAnsiPos++] = (char)0xCB;
        else if (strcmp(szKey,"Igrave")==0)  szAnsi[iAnsiPos++] = (char)0xCC;
        else if (strcmp(szKey,"Iacute")==0)  szAnsi[iAnsiPos++] = (char)0xCD;
        else if (strcmp(szKey,"Icirc")==0)   szAnsi[iAnsiPos++] = (char)0xCE;
        else if (strcmp(szKey,"Iuml")==0)    szAnsi[iAnsiPos++] = (char)0xCF;
        else if (strcmp(szKey,"ETH")==0)     szAnsi[iAnsiPos++] = (char)0xD0;
        else if (strcmp(szKey,"Ntilde")==0)  szAnsi[iAnsiPos++] = (char)0xD1;
        else if (strcmp(szKey,"Ograve")==0)  szAnsi[iAnsiPos++] = (char)0xD2;
        else if (strcmp(szKey,"Oacute")==0)  szAnsi[iAnsiPos++] = (char)0xD3;
        else if (strcmp(szKey,"Ocirc")==0)   szAnsi[iAnsiPos++] = (char)0xD4;
        else if (strcmp(szKey,"Otilde")==0)  szAnsi[iAnsiPos++] = (char)0xD5;
        else if (strcmp(szKey,"Ouml")==0)    szAnsi[iAnsiPos++] = (char)0xD6;
        else if (strcmp(szKey,"times")==0)   szAnsi[iAnsiPos++] = (char)0xD7;
        else if (strcmp(szKey,"Oslash")==0)  szAnsi[iAnsiPos++] = (char)0xD8;
        else if (strcmp(szKey,"Ugrave")==0)  szAnsi[iAnsiPos++] = (char)0xD9;
        else if (strcmp(szKey,"Uacute")==0)  szAnsi[iAnsiPos++] = (char)0xDA;
        else if (strcmp(szKey,"Ucirc")==0)   szAnsi[iAnsiPos++] = (char)0xDB;
        else if (strcmp(szKey,"Uuml")==0)    szAnsi[iAnsiPos++] = (char)0xDC;
        else if (strcmp(szKey,"Yacute")==0)  szAnsi[iAnsiPos++] = (char)0xDD;
        else if (strcmp(szKey,"THORN")==0)   szAnsi[iAnsiPos++] = (char)0xDE;
        else if (strcmp(szKey,"szlig")==0)   szAnsi[iAnsiPos++] = (char)0xDF;
        else if (strcmp(szKey,"agrave")==0)  szAnsi[iAnsiPos++] = (char)0xE0;
        else if (strcmp(szKey,"aacute")==0)  szAnsi[iAnsiPos++] = (char)0xE1;
        else if (strcmp(szKey,"acirc")==0)   szAnsi[iAnsiPos++] = (char)0xE2;
        else if (strcmp(szKey,"atilde")==0)  szAnsi[iAnsiPos++] = (char)0xE3;
        else if (strcmp(szKey,"auml")==0)    szAnsi[iAnsiPos++] = (char)0xE4;
        else if (strcmp(szKey,"aring")==0)   szAnsi[iAnsiPos++] = (char)0xE5;
        else if (strcmp(szKey,"aelig")==0)   szAnsi[iAnsiPos++] = (char)0xE6;
        else if (strcmp(szKey,"ccedil")==0)  szAnsi[iAnsiPos++] = (char)0xE7;
        else if (strcmp(szKey,"egrave")==0)  szAnsi[iAnsiPos++] = (char)0xE8;
        else if (strcmp(szKey,"eacute")==0)  szAnsi[iAnsiPos++] = (char)0xE9;
        else if (strcmp(szKey,"ecirc")==0)   szAnsi[iAnsiPos++] = (char)0xEA;
        else if (strcmp(szKey,"euml")==0)    szAnsi[iAnsiPos++] = (char)0xEB;
        else if (strcmp(szKey,"igrave")==0)  szAnsi[iAnsiPos++] = (char)0xEC;
        else if (strcmp(szKey,"iacute")==0)  szAnsi[iAnsiPos++] = (char)0xED;
        else if (strcmp(szKey,"icirc")==0)   szAnsi[iAnsiPos++] = (char)0xEE;
        else if (strcmp(szKey,"iuml")==0)    szAnsi[iAnsiPos++] = (char)0xEF;
        else if (strcmp(szKey,"eth")==0)     szAnsi[iAnsiPos++] = (char)0xF0;
        else if (strcmp(szKey,"ntilde")==0)  szAnsi[iAnsiPos++] = (char)0xF1;
        else if (strcmp(szKey,"ograve")==0)  szAnsi[iAnsiPos++] = (char)0xF2;
        else if (strcmp(szKey,"oacute")==0)  szAnsi[iAnsiPos++] = (char)0xF3;
        else if (strcmp(szKey,"ocirc")==0)   szAnsi[iAnsiPos++] = (char)0xF4;
        else if (strcmp(szKey,"otilde")==0)  szAnsi[iAnsiPos++] = (char)0xF5;
        else if (strcmp(szKey,"ouml")==0)    szAnsi[iAnsiPos++] = (char)0xF6;
        else if (strcmp(szKey,"divide")==0)  szAnsi[iAnsiPos++] = (char)0xF7;
        else if (strcmp(szKey,"oslash")==0)  szAnsi[iAnsiPos++] = (char)0xF8;
        else if (strcmp(szKey,"ugrave")==0)  szAnsi[iAnsiPos++] = (char)0xF9;
        else if (strcmp(szKey,"uacute")==0)  szAnsi[iAnsiPos++] = (char)0xFA;
        else if (strcmp(szKey,"ucirc")==0)   szAnsi[iAnsiPos++] = (char)0xFB;
        else if (strcmp(szKey,"uuml")==0)    szAnsi[iAnsiPos++] = (char)0xFC;
        else if (strcmp(szKey,"yacute")==0)  szAnsi[iAnsiPos++] = (char)0xFD;
        else if (strcmp(szKey,"thorn")==0)   szAnsi[iAnsiPos++] = (char)0xFE;
        else if (strcmp(szKey,"yuml")==0)    szAnsi[iAnsiPos++] = (char)0xFF;
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
  szAnsi[iAnsiPos++]=0;
  return szAnsi;
}

char* CScraperParser::RemoveWhiteSpace(const char *string2)
{
  if (!string2) return "";
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
	for (int i=0;i<9;++i)
	{
		m_param[i].clear();
	}
}
