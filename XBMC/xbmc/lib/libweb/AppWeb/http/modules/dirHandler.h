///
///	@file 	dirHandler.h
/// @brief 	Header for dirHandler
//	@copy	default
//	
//	Copyright (c) Mbedthis Software LLC, 2003-2007. All Rights Reserved.
//	
//	This software is distributed under commercial and open source licenses.
//	You may use the GPL open source license described below or you may acquire 
//	a commercial license from Mbedthis Software. You agree to be fully bound 
//	by the terms of either license. Consult the LICENSE.TXT distributed with 
//	this software for full details.
//	
//	This software is open source; you can redistribute it and/or modify it 
//	under the terms of the GNU General Public License as published by the 
//	Free Software Foundation; either version 2 of the License, or (at your 
//	option) any later version. See the GNU General Public License for more 
//	details at: http://www.mbedthis.com/downloads/gplLicense.html
//	
//	This program is distributed WITHOUT ANY WARRANTY; without even the 
//	implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. 
//	
//	This GPL license does NOT permit incorporating this software into 
//	proprietary programs. If you are unable to comply with the GPL, you must
//	acquire a commercial license to use this software. Commercial licenses 
//	for this software and support services are available from Mbedthis 
//	Software at http://www.mbedthis.com 
//	
//	@end
/////////////////////////////////// Includes ///////////////////////////////////

#ifndef _h_DIR_MODULE
#define _h_DIR_MODULE 1

#include	"http.h"

/////////////////////////////// Forward Definitions ////////////////////////////
#if BLD_FEATURE_DIR_MODULE

class MaDirHandler;
class MaDirHandlerService;
class MaDirModule;

extern "C" {
	extern int mprDirInit(void *handle);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////// DirModule //////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

class MaDirModule : public MaModule {
  private:
	MaDirHandlerService 
					*dirHandlerService;
  public:
					MaDirModule(void *handle);
					~MaDirModule();
};

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////// MaDirHandler /////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

typedef struct MaDirEntry {
	MprStr			name;
	uint64			lastModified;
	bool			isDir;
	uint			size;
} MaDirEntry;


class MaDirHandlerService : public MaHandlerService {
  private:
	MprLogModule	*log;
  public:
					MaDirHandlerService();
					~MaDirHandlerService();
	MaHandler		*newHandler(MaServer *server, MaHost *host, char *ext);
};


class MaDirHandler : public MaHandler {
  private:
	MprStr			defaultIcon;
	MprList			dirList;
	bool			doIndexes;
	MprStringList	extList;
	int				fancyIndexing;
	bool			foldersFirst;
	MprStringList	ignoreList;
	MprStr			pattern;
	MprStr			sortField;
	int				sortOrder;				// 1 == ascending, -1 descending

  public:
					MaDirHandler();
					~MaDirHandler();
	MaHandler		*cloneHandler();
	int 			filterDirList(MaRequest *rq, MaDirEntry *list, int count);
	void			generateListing(MaRequest *rq);
	int				matchRequest(MaRequest *rq, char *uri, int uriLen);
	int				run(MaRequest *rq);
	void			parseQuery(MaRequest *rq);
	void			outputFooter(MaRequest *rq);
	void			outputLine(MaRequest *rq, MaDirEntry *ep, const char *dir,
						int nameSize);
	void			outputHeader(MaRequest *rq, const char *dir, int nameSize);
	void			sortList(MaDirEntry *list, int count);

#if BLD_FEATURE_CONFIG_PARSE
	int				parseConfig(char *key, char *value, MaServer *server, 
						MaHost *host, MaAuth *auth, MaDir *dir, 
						MaLocation *location);
#endif
};

////////////////////////////////////////////////////////////////////////////////
#endif //	BLD_FEATURE_DIR_MODULE
#endif // 	_h_DIR_MODULE 

//
// Local variables:
// tab-width: 4
// c-basic-offset: 4
// End:
// vim: sw=4 ts=4 
//
