///
///	@file 	egiHandler.h
/// @brief 	Header for the egiHandler
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

#ifndef _h_EGI_MODULE
#define _h_EGI_MODULE 1

#include	"http.h"

/////////////////////////////// Forward Definitions ////////////////////////////

class MaEgiForm;
class MaEgiHandler;
class MaEgiHandlerService;

extern "C" {
	extern int mprEgiInit(void *handle);
}

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////// MaEgiModule ////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

class MaEgiModule : public MaModule {
  private:
  public:
					MaEgiModule(void *handle);
					~MaEgiModule();
};

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////// MaEgiForm /////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

///
///	@brief Create an EGI form
///
///	EGI forms are created via the MaEgiForm class. Instances are created 
///	for each EGI form. When a HTTP request is serviced that specifies the
///	EGI form, the run method will be invoked. The run method should return
///	to the browser the appropriate data.
///
class MaEgiForm : public MprHashEntry {
  private:
	char			*name;
  public:
	///
	///	@synopsis Constructor to create an EGI form
	///	@overview Instances of this class represent EGI forms. When an EGI
	///		form is invoked, the run method is called. For example:
	///		the URL: 
	///
	///	@code
	///		http://localhost/egi/myForm?name=Julie
	///	@endcode
	///
	///	could be enabled by calling maDefineEgiForm("myForm", myFormProc);	
	///	@param formName Name to publish the form as. This appears in the URL. 
	///		Names must therefore only contain valid URL characters.
	/// @stability Evolving.
	/// @library libappweb
	///	@see MaEgiForm
					MaEgiForm(char *formName);
					MaEgiForm(MaServer *server, MaHost *host, char *formName);
	virtual			~MaEgiForm();
	char			*getName();
	virtual void	run(MaRequest *rq, char *script, char *path, 
						char *query, char *postData, int postLen);
};

////////////////////////////////////////////////////////////////////////////////
///////////////////////////// MaEgiHandlerService //////////////////////////////
////////////////////////////////////////////////////////////////////////////////

class MaEgiHandlerService : public MaHandlerService {
  private:
	MprHashTable	*forms;					// Table of functions 
	MprList			handlerHeaders;			// List of handler headers
	MprLogModule	*log;

#if BLD_FEATURE_MULTITHREAD
	MprMutex		*mutex;
#endif

  public:
					MaEgiHandlerService();
					~MaEgiHandlerService();
	void			insertForm(MaServer *server, MaHost *host, 
						MaEgiForm *form);
	MaHandler		*newHandler(MaServer *server, MaHost *host, char *ex);

#if BLD_FEATURE_MULTITHREAD
	inline void		lock() { mutex->lock(); };
	inline void		unlock() { mutex->unlock(); };
#else
	inline void		lock() { };
	inline void		unlock() { };
#endif
};

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////// EgiHandler //////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
//
//	A master instance of the EgiHandler is create for each referencing host.
//	A request instance is cloned from this for each request.
//

//
//	egiFlags -- FUTURE -- use MA_ prefix
//
#define MPR_EGI_NON_PARSED_HEADER	0x1		// CGI program creates HTTP headers
#define MPR_EGI_HEADER_SEEN			0x2		// Server has parsed CGI response
#define MPR_EGI_CLONED				0x4		// Cloned handler
#define MPR_EGI_URL_ENCODED			0x8		// Post data is url encoded

class MaEgiHandler : public MaHandler {
  private:
	int				egiFlags;
	MprHashTable	*forms;					// Pointer to service forms
	MprLogModule	*log;					// Pointer to the service log
	MprBuf*			postBuf;

  public:
					MaEgiHandler(char *ext, MprLogModule *log, 
					MprHashTable *forms);
					~MaEgiHandler();
	MaHandler		*cloneHandler();
	char			*getHostName();
	void			insertForm(MaEgiForm *form);
	void			postData(MaRequest *rq, char *buf, int buflen);
	int				run(MaRequest *rq);
	int				setup(MaRequest *rq);
};

////////////////////////////////////////////////////////////////////////////////
//
//	C API
//
#ifdef  __cplusplus
extern "C" {
#endif

#if BLD_FEATURE_EGI_MODULE
//
//	Almost DEPRECATED 2.0 but back by user demand
//
typedef void (*MaEgiCb)(MaRequest *rq, char *script, char *uri, char *query,
	char *postData, int postLen);
extern int 		maDefineEgiForm(char *name, MaEgiCb fn);
#endif

#ifdef  __cplusplus
} 	// extern "C" 
#endif

////////////////////////////////////////////////////////////////////////////////
#endif // _h_EGI_MODULE 

//
// Local variables:
// tab-width: 4
// c-basic-offset: 4
// End:
// vim: sw=4 ts=4 
//
