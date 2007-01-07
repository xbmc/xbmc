// include file for HTMLScraper

#ifdef _XBOX
  #include <xtl.h>
#else
  #include <windows.h>
#endif

extern "C" 
{
	__declspec(dllexport) int IMDbGetSearchResults(char *szXML, const char *szHTML, const char *szFromURL);
	__declspec(dllexport) int IMDbGetDetails(char *szXML, const char *szHTML, const char *szPlotHTML);
};