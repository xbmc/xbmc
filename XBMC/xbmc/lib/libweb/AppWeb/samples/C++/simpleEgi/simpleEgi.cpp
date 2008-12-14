///
///	@file 	simpleEgi.cpp
/// @brief 	Demonstrate the use of Embedded Server Pages (EGI) in a 
///			simple multi-threaded application.
///
////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) Mbedthis Software LLC, 2003-2007. All Rights Reserved.
//	The latest version of this code is available at http://www.mbedthis.com
//
//	This software is open source; you can redistribute it and/or modify it 
//	under the terms of the GNU General Public License as published by the 
//	Free Software Foundation; either version 2 of the License, or (at your 
//	option) any later version.
//
//	This program is distributed WITHOUT ANY WARRANTY; without even the 
//	implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. 
//	See the GNU General Public License for more details at:
//	http://www.mbedthis.com/downloads/gplLicense.html
//	
//	This General Public License does NOT permit incorporating this software 
//	into proprietary programs. If you are unable to comply with the GPL, a 
//	commercial license for this software and support services are available
//	from Mbedthis Software at http://www.mbedthis.com
//
/////////////////////////////// Includes ///////////////////////////////

#include	"appweb/appweb.h"

//////////////////////////////// Defines ///////////////////////////////
#if BLD_FEATURE_EGI_MODULE
//
//	Define the our EGI object to be called when the web form is posted.
//
class MyEgi : public MaEgiForm {
  public:
			MyEgi(char *egiName);
			~MyEgi();
	void	run(MaRequest *rq, char *script, char *path, char *query, 
				char *postData, int postLen);
};

/////////////////////////////////// Code ///////////////////////////////

int main(int argc, char** argv)
{
	MaHttp		*http;					// Http service inside our app
	MaServer	*server;				// For the HTTP server
	Mpr			mpr("simpleEgi");		// Initialize the run time

#if BLD_FEATURE_LOG
	//
	//	Do the following two statements only if you want debug trace
	//
	mpr.addListener(new MprLogToFile());
	mpr.setLogSpec("stdout:4");
#endif

	//
	//	Start the Mbedthis Portable Runtime
	//
	mpr.start(0);

	//
	//	Create Http and Server objects for this application. We set the
	//	ServerName to be "default" and the initial serverRoot to be ".".
	//	This will be overridden in simpleEgi.conf.
	//
	http = new MaHttp();
	server = new MaServer(http, "default", ".");
	
	//
	//	Activate the copy module and handler
	//
	new MaCopyModule(0);
	new MaEgiModule(0);

	//
	//	Configure the server with the configuration directive file
	//
	if (server->configure("simpleEgi.conf") < 0) {
		mprFprintf(MPR_STDERR, 
			"Can't configure the server. Error on line %d\n", 
			server->getLine());
		exit(2);
	}

	//
	//	Define our EGI procedures
	//
	new MyEgi("/myEgi.egi");
	
	//
	//	Start the server
	//
	if (http->start() < 0) {
		mprFprintf(MPR_STDERR, "Can't start the server\n");
		exit(2);
	}

	//
	//	Tell the MPR to loop servicing incoming requests. We can 
	//	replace this call with a variety of event servicing 
	//	mechanisms offered by AppWeb.
	//
	mpr.serviceEvents(0, -1);

	//
	//	Orderly shutdown
	//
	http->stop();
	delete server;
	delete http;

	//
	//	MPR run-time will automatically stop and be cleaned up
	//
	return 0;
}

////////////////////////////////////////////////////////////////////////

MyEgi::MyEgi(char *name) : MaEgiForm(name)
{
	//	Put required initialization (if any) here
}

////////////////////////////////////////////////////////////////////////

MyEgi::~MyEgi()
{
	//	Put cleanup herre
}

////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////
//
//	Method that is run when the EGI form is called from the web
//	page. Rq is the request context. URI is the bare URL minus query.
//	Query is the string after a "?" in the URL. Post data is posted
//	HTTP form data.
//

void MyEgi::run(MaRequest *rq, char *script, char *uri, char *query, 
	char *postData, int postLen)
{
#if TEST_MULTI_THREADED_ACCESS
	mprPrintf("In MyEgi::run, thread %s\n", mprGetCurrentThreadName());

	//
	//	To test multithreaded access, this will sleep now 
	//
	mprPrintf("Sleeping for 15 seconds\n");
	mprSleep(15 * 1000);
#else
	mprPrintf("In MyEgi::run, single threaded\n");
#endif

#if TEST || 1
	MaClient    *client;
	int code,contentLen;
	char* content;

	client = new MaClient();
	if (client->getRequest("http://www.mbedthis.com/index.html") < 0) {
		rq->requestError(500, "Can't get client URL");
		return;
	}

	//
	//  Examine the HTTP response HTTP code. 200 is success.
	//
	code = client->getResponseCode();
	if (code != 200) {
		rq->requestError(500, "Bad status code %d", code);
		return;
	}
                                                                               
	//
	//  Get the actual response content
	//
	content = client->getResponseContent(&contentLen);
	if (content) {
		mprPrintf("Server responded with:\n%s\n", content);
	}
   
	rq->setResponseCode(200);
	rq->write(content);
	delete client;
#endif

	rq->write("<HTML><TITLE>simpleEgi</TITLE><BODY>\r\n");
	rq->writeFmt("<p>Name: %s</p>\n", 
		rq->getVar(MA_FORM_OBJ, "name", "-")); 
	rq->writeFmt("<p>Address: %s</p>\n", 
		rq->getVar(MA_FORM_OBJ, "address", "-")); 
	rq->write("</BODY></HTML>\r\n");

#if TEST_MULTI_THREADED_ACCESS
	mprPrintf("Exiting thread %s\n", mprGetCurrentThreadName());
#endif

#if UNUSED
	//
	//	Possible useful things to do in egi forms
	//
	rq->setResponseCode(200);
	rq->setContentType("text/html");
	rq->setHeaderFlags(MPR_HTTP_DONT_CACHE);
	rq->requestError(409, "My message : %d", 5);

	rq->redirect(302, "/myURl");
	rq->flushOutput(MPR_HTTP_FOREGROUND_FLUSH, MPR_HTTP_FINISH_REQUEST);
#endif
}

////////////////////////////////////////////////////////////////////////
#else
int main()
{
	fprintf(stderr, "BLD_FEATURE_EGI_MODULE is not defined in config.h\n");
	exit(2);
}
#endif /* BLD_FEATURE_EGI_MODULE */

//
// Local variables:
// tab-width: 4
// c-basic-offset: 4
// End:
// vim: sw=4 ts=4 
//
