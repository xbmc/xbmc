// scrap.cpp : Defines the entry point for the console application.
//

#include <tchar.h>
#include <iostream>
#include <curl/curl.h>
#include "ScraperParser.h"
#include "Scraper.h"

#include <vector>
//#include "../../xbmc/utils/HTTP.h"


using namespace std;

size_t write_data(void *ptr, size_t size, size_t nmemb, void *stream)
{
  int written = fwrite(ptr, size, nmemb, (FILE *)stream);
  return written;
}

void Tokenize(const string& str, vector<string>& tokens, const string& delimiters = " ")
{
  string::size_type lastPos = str.find_first_not_of(delimiters, 0);
  string::size_type pos     = str.find_first_of(delimiters, lastPos);

  while (string::npos != pos || string::npos != lastPos)
  {
    tokens.push_back(str.substr(lastPos, pos - lastPos));
    lastPos = str.find_first_not_of(delimiters, pos);
    pos = str.find_first_of(delimiters, lastPos);
  }
}
void get_url(const CStdString& strFilename,CScraperUrl& scrUrl)
{
  CURL *curl;
  curl = curl_easy_init();
  
  FILE* f = fopen(strFilename.c_str(),"w");
  curl_easy_setopt(curl, CURLOPT_REFERER, scrUrl.m_spoof.c_str());
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA ,f);
  
  curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, TRUE);
  curl_easy_setopt(curl, CURLOPT_MAXREDIRS, TRUE); 

  if(scrUrl.m_post)
  {
    vector<string> vecUrl;
    Tokenize(scrUrl.m_url, vecUrl,"?");
    scrUrl.ParseString(vecUrl.at(0));
    struct curl_httppost *formpost=NULL;
    struct curl_httppost *lastptr=NULL;

    vector<string> vecOptions;
    Tokenize(vecUrl.at(1), vecOptions,";&");

    for(int i=0;i < vecOptions.size(); i++)
    {
      vector<string> vecOption;
      Tokenize(vecOptions.at(i), vecOption,"=");
      string strName = vecOption.at(0);
      string strValue = vecOption.at(1);
      curl_formadd(&formpost, &lastptr, CURLFORM_COPYNAME, strName.c_str(), CURLFORM_COPYCONTENTS, strValue.c_str(), CURLFORM_END);
    }
    curl_easy_setopt(curl, CURLOPT_HTTPPOST, formpost);
  }
  curl_easy_setopt(curl, CURLOPT_URL, scrUrl.m_url.c_str());
	curl_easy_perform(curl);
  fclose(f);

  curl_easy_cleanup(curl);

}

int _tmain(int argc, _TCHAR* argv[])
{
 
	if (argc < 3)
	{
    cout << "Error: Not enough arguments. Need a xml file and a movie name" << endl;
    cout << "Usage: " << argv[0] << " imdb.xml \"Fight Club\" [CreateSearchUrl]" << endl;
		return -1;
	}
  Scraper scrap(argv[1]);
  
  //Default operation
  if(!argv[3]) argv[3] = "CreateSearchUrl";
  //only set movie name as buffer 1 if we use GetResults
  if( strcmp(argv[3], "CreateSearchUrl") == 0 )   scrap.SetBuffer(1,argv[2]);
  
  //Let the parser do its job
  if(!scrap.Parse(argv[3])) return -1;
  return 0;
}
