#pragma once

#include "includes.h"

class CXbmcConfiguration
{
public:
	CXbmcConfiguration();
	~CXbmcConfiguration();

	int		BookmarkSize( int eid, webs_t wp, int argc, char_t **argv);
	int		GetBookmark( int eid, webs_t wp, int argc, char_t **argv);
	int		AddBookmark( int eid, webs_t wp, int argc, char_t **argv);
	int		SaveBookmark( int eid, webs_t wp, int argc, char_t **argv);
	int		RemoveBookmark( int eid, webs_t wp, int argc, char_t **argv);
	int		SaveConfiguration( int eid, webs_t wp, int argc, char_t **argv);
	int		GetOption( int eid, webs_t wp, int argc, char_t **argv);
	int		SetOption( int eid, webs_t wp, int argc, char_t **argv);

private:
	int		Load();
	bool	IsValidOption(char* option);

	TiXmlDocument	xbmcCfg;// ("Q:\\config.xml");
	bool	xbmcCfgLoaded;
};

