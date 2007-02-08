#include <tchar.h>
#include <iostream>
#include <curl/curl.h>
#include "ScraperParser.h"
#include "Scraper.h"
#include "RegExp.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

Scraper::Scraper(const string xml)
{
  s_xml = xml;
  s_load = false;
}

Scraper::~Scraper()
{
}
bool Scraper::Load()
{
  if(!s_load && !s_parser.Load(s_xml)) 
  {
      cout << "Error: Could not load scraper " << s_xml << endl;
		  s_load = false;
      return false;
  }
  s_load = true;
  return true;
}
bool Scraper::SetBuffer(int i, string strValue)
{
  if(i > 9)
  {
    cout << "Error: Could not set buffer " << i << ". There is a maximum of 9 buffers" << endl;
    return false;
  }
  s_parser.m_param[i - 1] = strValue;
  return true;
}
void Scraper::PrintBuffer(int i)
{
  cout << "Buffer " << i << ": " <<  s_parser.m_param[i - 1] << endl;
}
bool Scraper::WriteResult(const CStdString& strFilename)
{
  FILE* f = fopen(strFilename.c_str(),"w");
  if(fwrite(s_result.c_str(),s_result.size(),1,f))
  {
    fclose(f);
    return true;
  }
  fclose(f);
  return false;
}
string Scraper::readFile(const CStdString& strFilename)
{
  char temp[128*1024+1];
  FILE* f = fopen(strFilename.c_str(),"r");
	if (!f)
  {
    cout << "Error: Couldn't read" << strFilename.c_str() << endl;
    return "";
  }
  int read = fread(temp,1,128*1024,f);
	fclose(f);
	temp[read] = '\0';
  return temp;
}
CStdString Scraper::PrepareSearchString(CStdString& strSearchterm)
{
  strSearchterm.Replace(".","+");
  strSearchterm.Replace("-","+");
  strSearchterm.Replace(" ","+");

  strSearchterm = strSearchterm.ToLower();
  
  CRegExp reTags;
  reTags.RegComp("\\+(ac3|custom|dc|divx|dsr|dsrip|dutch|dvd|dvdrip|dvdscr|fragment|fs|hdtv|internal|limited|multisubs|ntsc|ogg|ogm|pal|pdtv|proper|repack|rerip|retail|se|svcd|swedish|unrated|ws|xvid|xxx|cd[1-9]|\\[.*\\])(\\+|$)");
  int i=0;
  if ((i=reTags.RegFind(strSearchterm.c_str())) >= 0) // new logic - select the crap then drop anything to the right of it
  {
    return strSearchterm.Mid(0,i);
 }
  else
    return strSearchterm;
}
int Scraper::CustomFunctions(const CStdString& strFilename)
{
  CStdString strFilenameHtml;
  //find custom functions
  // ok, now parse the xml file
	TiXmlDocument doc(strFilename.c_str());
	doc.LoadFile();
	if (!doc.RootElement())
  {
    cout << "Error: Unable to parse " << strFilename.c_str() << endl;
    return -1;
  }
  TiXmlElement* pRoot = doc.RootElement();
  TiXmlElement* url = pRoot->FirstChildElement("url");
  while (url && url->FirstChild())
  {
    const char* szFunction = url->Attribute("function");
    if (szFunction)
    {
      //wirte url
      strFilenameHtml.Format("%s.html",szFunction);
      get_url(strFilenameHtml.c_str(),url->FirstChild()->Value());

      //buffer 1 contents of results.html
      SetBuffer(1,readFile(strFilenameHtml.c_str()) );
      Parse(szFunction);
    }
    url = url->NextSiblingElement("url");
  }
  return 0;
}
bool Scraper::PrepareParsing(const CStdString& function)
{
  if(strcmp(function.c_str(),"CreateSearchUrl") == 0)
  {
    //modify content of buffer 1 
    cout << "Searching for: " << PrepareSearchString(s_parser.m_param[0]) << endl;
    SetBuffer(1,PrepareSearchString(s_parser.m_param[0]));
  }
  else if(strcmp(function.c_str(),"GetSearchResults") == 0)
  {
    /*TiXmlDocument doc("url.xml");
    doc.LoadFile();
	  if (!doc.RootElement())
    {
      cout << "Error: Unable to parse url.xml" <<  endl;
      return false;
    }
    TiXmlHandle docHandle( &doc );
    TiXmlElement *link = docHandle.FirstChild("url").Element();
    cout << "Downloading: " << link->FirstChild()->Value() <<  endl;
    get_url("results.html", link->FirstChild()->Value() );*/
    
    cout << "Downloading: " << s_result.c_str()  <<  endl;
    get_url("results.html", s_result.c_str() );
    SetBuffer(1,readFile("results.html"));
    SetBuffer(2,s_result.c_str());
  }
  else if(strcmp(function.c_str(),"GetDetails") == 0) 
  {
    CStdString strFilename;
    //Parse XML
	  TiXmlDocument doc("results.xml");
	  doc.LoadFile();
	  if (!doc.RootElement())
    {
      cout << "Error: Unable to parse results.xml" <<  endl;
      return false;
    }
    TiXmlHandle docHandle( &doc );
    TiXmlElement *link = docHandle.FirstChild( "results" ).FirstChild( "entity" ).FirstChild("url").Element();
	  int i = 0;
    
    do
      {
        i++;
        
        strFilename.Format("details.%i.html",i);
        cout << "Details URL " << i << ":"<< link->FirstChild()->Value() << endl;
        get_url(strFilename.c_str(),link->FirstChild()->Value() );
        SetBuffer(i,readFile(strFilename.c_str()));
      }  
    while( link = link->NextSiblingElement("url") );
    if(link = docHandle.FirstChild( "results" ).FirstChild( "entity" ).FirstChild( "id" ).Element() )
    {    
      SetBuffer(i+1,link->FirstChild()->Value()) ;
      cout << "ID: " << i << s_parser.m_param[i] << endl;
    }
  } 
  return true;
}
int Scraper::Parse(const CStdString& function)
{
  CStdString strFilename;
  if(!(Load() && PrepareParsing(function))) return -1;

  s_result = s_parser.Parse(function.c_str());

  cout << function.c_str()  << " returned : " << s_result.c_str() << endl;
  //what's the next step?
  if(strcmp(function.c_str(),"CreateSearchUrl") == 0)
  {
    WriteResult("url.xml");
    Scraper::Parse("GetSearchResults");
  }
  else if(strcmp(function.c_str(),"GetSearchResults") == 0)
  {
    WriteResult("results.xml");
    Scraper::Parse("GetDetails");
  }
  else if(strcmp(function.c_str(),"GetDetails") == 0) 
  {
    WriteResult("details.xml");
    CustomFunctions("details.xml");
  }
  else 
  {
    strFilename.Format("%s.xml",function);
    WriteResult(strFilename.c_str());
    CustomFunctions(strFilename.c_str());
  }
} 