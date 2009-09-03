#pragma once

#undef min
#undef max
#include <algorithm>
#include "tinyXML/tinyxml.h"

typedef char char_t;
typedef struct websRec *webs_t;

class CXbmcConfiguration
{
public:
	CXbmcConfiguration();
	~CXbmcConfiguration();

	int		BookmarkSize( int eid, webs_t wp, CStdString& response, int argc, char_t **argv);
	int		GetBookmark( int eid, webs_t wp, CStdString& response, int argc, char_t **argv);
	int		AddBookmark( int eid, webs_t wp, CStdString& response, int argc, char_t **argv);
	int		SaveBookmark( int eid, webs_t wp, CStdString& response, int argc, char_t **argv);
	int		RemoveBookmark( int eid, webs_t wp, CStdString& response, int argc, char_t **argv);
	int		SaveConfiguration( int eid, webs_t wp, CStdString& response, int argc, char_t **argv);
	int		GetOption( int eid, webs_t wp, CStdString& response, int argc, char_t **argv);
	int		SetOption( int eid, webs_t wp, CStdString& response, int argc, char_t **argv);

private:
	int		Load();
	bool	IsValidOption(char* option);

	TiXmlDocument	xbmcCfg;
	bool	xbmcCfgLoaded;
};

