#include <iostream>
#include "ScraperParser.h"
#include "Scraper.h"
#include "utils.h"
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
  if(!s_load && !m_parser.Load(s_xml)) 
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
  m_parser.m_param[i - 1] = strValue;
  return true;
}
void Scraper::PrintBuffer(int i)
{
  cout << "Buffer " << i << ": " <<  m_parser.m_param[i - 1] << endl;
}
bool Scraper::WriteResult(const CStdString& strFilename)
{
  FILE* f = fopen(strFilename.c_str(),"w");
  if(fwrite(m_result.c_str(),m_result.size(),1,f))
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
      CScraperUrl srcUrl(url);
      get_url(strFilenameHtml.c_str(),srcUrl);

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
    cout << "Searching for: " << PrepareSearchString(m_parser.m_param[0]) << endl;
    SetBuffer(1,PrepareSearchString(m_parser.m_param[0]));
  }
  else if(strcmp(function.c_str(),"GetSearchResults") == 0)
  {  
    CScraperUrl srcUrl(m_result);
    cout << "Downloading: " << srcUrl.m_url.c_str()  <<  endl;
    get_url("results.html", srcUrl );
    SetBuffer(1,readFile("results.html"));
    SetBuffer(2,srcUrl.m_url.c_str());
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
        CScraperUrl srcUrl(link);
        strFilename.Format("details.%i.html",i);
        cout << "Details URL " << i << ":"<< srcUrl.m_url << endl;
        get_url(strFilename.c_str(),srcUrl );
        SetBuffer(i,readFile(strFilename.c_str()));
      }  
    while( link = link->NextSiblingElement("url") );
    if(link = docHandle.FirstChild( "results" ).FirstChild( "entity" ).FirstChild( "id" ).Element() )
    {    
      SetBuffer(i+1,link->FirstChild()->Value()) ;
      cout << "ID: " << i << m_parser.m_param[i] << endl;
    }
  } 
  else if(strcmp(function.c_str(),"GetEpisodeList") == 0) 
  {
    TiXmlDocument doc("details.xml");
    doc.LoadFile();
	  if (!doc.RootElement())
    {
      cout << "Error: Unable to parse details.xml" <<  endl;
      return false;
    }
    TiXmlHandle docHandle( &doc );
    TiXmlElement *link = docHandle.FirstChild( "details" ).FirstChild( "episodeguide" ).FirstChild( "url" ).Element();
	  int i = 0;
    CStdString strFilename;
    do
    {
      i++;
      CScraperUrl srcUrl(link);
      strFilename.Format("episodelist.%i.html",i);
      cout << "Episodelist URL " << i << ":"<< srcUrl.m_url << endl;
      get_url(strFilename.c_str(),srcUrl );
      if(i > 1) 
      {
        //abuse strFilename
        strFilename.Format("GetEpisodeListInternal %i",i);
        Parse(strFilename);
      }
    }  
    while( link = link->NextSiblingElement("url") );
    SetBuffer(1,readFile("episodelist.1.html"));
  }
  else if(strstr(function.c_str(),"GetEpisodeListInternal") != NULL) 
  {
    CStdString strFilename;
    int i;
    sscanf (function,"%*s %d",&i);
    //files get downloaded in GetEpisodeList so we only rad them
    strFilename.Format("episodelist.%i.html",i);
    SetBuffer(1,readFile(strFilename.c_str()));
  }
  else if(strcmp(function.c_str(),"GetEpisodeDetails") == 0) 
  {
    TiXmlDocument doc("episodelist.1.xml");
    doc.LoadFile();
	  if (!doc.RootElement())
    {
      cout << "Error: Unable to parse episodelist.xml" <<  endl;
      return false;
    }
    TiXmlHandle docHandle( &doc );
    TiXmlElement *link = docHandle.FirstChild( "episodeguide" ).FirstChild( "episode" ).FirstChild("url").Element();
    CScraperUrl srcUrl(link);
    CStdString strFilename = "episodedetails.html";
    cout << "Episode details URL:"<< srcUrl.m_url << endl;
    get_url(strFilename.c_str(),srcUrl );
    SetBuffer(1,readFile(strFilename.c_str()));
  }
  return true;
}
int Scraper::Parse(const CStdString& function)
{
  CStdString strFilename;
  if(!(Load() && PrepareParsing(function))) return -1;

  if(strstr(function.c_str(),"GetEpisodeListInternal") != NULL) 
    m_result = m_parser.Parse("GetEpisodeList");
  else m_result = m_parser.Parse(function.c_str());

  cout << function.c_str()  << " returned : " << m_result.c_str() << endl;
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
    if(strcmp(m_parser.GetContent().c_str(),"tvshows") == 0)
      Scraper::Parse("GetEpisodeList");
    else if(strcmp(m_parser.GetContent().c_str(),"movies") == 0) 
      CustomFunctions("details.xml");
  }
  else if(strcmp(function.c_str(),"GetEpisodeList") == 0) 
  {
    WriteResult("episodelist.1.xml");
    Parse("GetEpisodeDetails");
  }
  else if(strcmp(function.c_str(),"GetEpisodeDetails") == 0) 
  {
    WriteResult("episodedetails.xml");
    CustomFunctions("episodedetails.xml");
  }   
  else if(strstr(function.c_str(),"GetEpisodeListInternal") != NULL) 
  {
    int i;
    sscanf (function,"%*s %d",&i);
    strFilename.Format("episodelist.%i.xml",i);
    WriteResult(strFilename.c_str());
  }
  else 
  {
    strFilename.Format("%s.xml",function);
    WriteResult(strFilename.c_str());
    CustomFunctions(strFilename.c_str());
  }
  return 0;
} 