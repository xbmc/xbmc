///
///	@file 	cgiHandler.h
/// @brief 	Header for cgiHandler
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
////////////////////////////////// Includes ////////////////////////////////////

#ifndef _h_CGI_MODULE
#define _h_CGI_MODULE 1

#include	"http.h"

/////////////////////////////// Forward Definitions ////////////////////////////

#if BLD_FEATURE_CGI_MODULE
class	MaCgiModule;
class	MaCgiHandler;
class	MaCgiHandlerService;

extern "C" {
	extern int mprCgiInit(void *handle);
}

////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////// CgiModule //////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

class MaCgiModule : public MaModule {
  private:
	MaCgiHandlerService	
					*cgiHandlerService;
  public:
					MaCgiModule(void *handle);
					~MaCgiModule();
};

////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////// CgiHandler /////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

class MaCgiHandlerService : public MaHandlerService {
  private:
	MprLogModule	*log;
	MaCgiHandler	*cgiHandler;

  public:
					MaCgiHandlerService();
					~MaCgiHandlerService();
	MaHandler		*newHandler(MaServer *server, MaHost *host, char *ex);
};

//
//	cgiFlags
//
#define MPR_CGI_NON_PARSED_HEADER	0x1		// CGI program creates HTTP headers
#define MPR_CGI_HEADER_SEEN			0x2		// Server has parsed CGI response

class MaCgiHandler : public MaHandler {
  private:
	MprBuf			*headerBuf;
	int				cgiFlags;
	MprCmd			*cmd;
	MprLogModule	*log;
	MprStr			newLocation;

  public:
					MaCgiHandler(char *ext, MprLogModule *log);
					~MaCgiHandler();
	void			buildArgs(int *argcp, char ***argvp, MprCmd *cmd, 
						MaRequest *rq);
	int				cgiDone(MaRequest *rq, int exitCode);
	MaHandler		*cloneHandler();
	void			parseHeader(MaRequest *rq);
	void			postData(MaRequest *rq, char *buf, int buflen);
	int				gatherOutputData(MaRequest *rq);
	int				run(MaRequest *rq);
	int				setup(MaRequest *rq);
#if WIN
	void			findExecutable(char **program, char **script, 
						char **bangScript, MaRequest *rq, char *fileName);
#endif
#if BLD_FEATURE_CONFIG_PARSE
	int				parseConfig(char *key, char *value, MaServer *server, 
						MaHost *host, MaAuth *auth, MaDir* dir, 
						MaLocation *location);
#endif
};

#endif // BLD_FEATURE_CGI_MODULE
////////////////////////////////////////////////////////////////////////////////
#endif // _h_CGI_MODULE 

//
// Local variables:
// tab-width: 4
// c-basic-offset: 4
// End:
// vim: sw=4 ts=4 
//
