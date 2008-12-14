///
///	@file 	php5Handler.cpp
/// @brief 	PHP content handler
///	@overview The PHP handler provides an efficient way to process 
///		PHP content in-process.
//
/////////////////////////////////// Copyright //////////////////////////////////
//
//	@copy	default.p
//	 +----------------------------------------------------------------------+
//	 | PHP Version 4,5                                                      |
//	 +----------------------------------------------------------------------+
//	 | Portions Copyright (c) 1997-2002 The PHP Group.                      |
//	 | Portions Copyright (c) Mbedthis Software LLC, 2003-2007.             |
//	 +----------------------------------------------------------------------+
//	 | This source file is subject to version 2.02 of the PHP license,      |
//	 | that is bundled with this package in the documentation, and is       |
//	 | available at through the world-wide-web at                           |
//	 |                                                                      |
//	 |     http://www.php.net/license/2_02.txt.                             |
//	 |                                                                      |
//	 | If you did not receive a copy of the PHP license and are unable to   |
//	 | obtain it through the world-wide-web, please send a note to          |
//	 | license@php.net so we can mail you a copy immediately.               |
//	 +----------------------------------------------------------------------+
//	 | Authors: Michael O'Brien                                             |
//	 |          Eric Colinet <ecolinet@php.net>                             |
//	 +----------------------------------------------------------------------+
//	@end
//
/////////////////////////////////// Includes ///////////////////////////////////

#include	"php5Handler.h"

#if BLD_FEATURE_PHP5_MODULE

///////////////////////////////// SAPI Module //////////////////////////////////
#if UNUSED
//
//	This file can be built as an Appweb module that links in the PHP library.
//	It can also be built to be a SAPI module so that when PHP is built, it
//	creates an appweb module. 
//	
//	The following is the PHP module linkage required to create a SAPI module
//
PHP_MINIT_FUNCTION(appweb);
PHP_MSHUTDOWN_FUNCTION(appweb);
PHP_RINIT_FUNCTION(appweb);
PHP_RSHUTDOWN_FUNCTION(appweb);
PHP_MINFO_FUNCTION(appweb);
PHP_FUNCTION(appweb_virtual);
PHP_FUNCTION(appweb_request_headers);
PHP_FUNCTION(appweb_response_headers);

function_entry appweb_functions[] = {
	{0, 0, 0}
};

zend_module_entry appweb_module_entry = {
	STANDARD_MODULE_HEADER,
	BLD_PRODUCT,
	appweb_functions,   
	PHP_MINIT(appweb),
	PHP_MSHUTDOWN(appweb),
	NULL,
	NULL,
	PHP_MINFO(appweb),
	NO_VERSION_YET,
	STANDARD_MODULE_PROPERTIES
};

PHP_MINIT_FUNCTION(appweb)
{
	return SUCCESS;
}

PHP_MSHUTDOWN_FUNCTION(appweb)
{
	return SUCCESS;
}

PHP_MINFO_FUNCTION(appweb)
{
	php_info_print_table_start();
	php_info_print_table_row(2, "Mbedthis Appweb Module Revision", 
		BLD_VERSION);
	php_info_print_table_row(2, "Server Version", mprGetMpr()->getVersion());
	php_info_print_table_end();
}

#endif
/////////////////////////////// SAPI Interface /////////////////////////////////
//
//	NOTE: php_ functions are supplied by PHP, whereas phpCapital functions 
//	are local to this handler.
//
static void php5LogMessage(char *message);
static char	*php5ReadCookies(TSRMLS_D);
static int	php5ReadPostData(char *buffer, uint len TSRMLS_DC);
static void	php5RegisterVariables(zval *track_vars_array TSRMLS_DC);
static int	php5SapiStartup(sapi_module_struct *sapi_module);
static int	php5SendHeaders(sapi_headers_struct *sapi_headers TSRMLS_DC);
static void	php5Flush(void *server_context);
static int	php5Write(const char *str, uint str_length TSRMLS_DC);
static int	php5WriteHeader(sapi_header_struct *sapi_header, 
				sapi_headers_struct *sapi_headers TSRMLS_DC);

//
//	PHP Server API Module Structure
//
static sapi_module_struct php5SapiBlock = {
	BLD_PRODUCT,					// name 
	BLD_NAME,						// long name
	php5SapiStartup,				// startup 
	php_module_shutdown_wrapper,	// shutdown 
	0,								// activate 
	0,								// deactivate 
	php5Write,		   		 		// unbuffered write 
	php5Flush,						// flush
	0,								// get uid
	0,								// getenv 
	php_error,						// error handler 
	php5WriteHeader,				// header handler 
	php5SendHeaders,				// send headers 
	0,								// send header 
	php5ReadPostData,				// read POST data
	php5ReadCookies,				// read Cookies 
	php5RegisterVariables,			// register server variables 
	php5LogMessage,					// Log message
	NULL,							// php_ini_path_override (set below)
	0,								// Block interruptions 
	0,								// Unblock interruptions 
	STANDARD_SAPI_MODULE_PROPERTIES
};

////////////////////////////////////////////////////////////////////////////////
///////////////////////////////// MaPhp5Module /////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
//
//	Called when the PHP module is first loaded as a DLL
//

int mprPhp5Init(void *handle)
{
	if (maGetHttp() == 0) {
		return MPR_ERR_NOT_INITIALIZED;
	}
	(void) new MaPhp5Module(handle);
	return 0;
}

////////////////////////////////////////////////////////////////////////////////
//
//	Create the PHP Handler service
//

MaPhp5Module::MaPhp5Module(void *handle) : MaModule(MA_PHP_MODULE_NAME, handle)
{
	phpHandlerService = new MaPhp5HandlerService();
}

////////////////////////////////////////////////////////////////////////////////

MaPhp5Module::~MaPhp5Module()
{
	delete phpHandlerService;
	//
	//	Should be done, but crashes in PHP
	//
	//	ts_free_thread();
}

////////////////////////////////////////////////////////////////////////////////
//////////////////////////// MaPhp5HandlerService //////////////////////////////
////////////////////////////////////////////////////////////////////////////////
//
//	One PHP Handler service for them all
//

MaPhp5HandlerService::MaPhp5HandlerService() :
	MaHandlerService(MA_PHP_HANDLER_NAME)
{
#if BLD_FEATURE_LOG
	log = new MprLogModule(MA_PHP_LOG_NAME);
#endif
}

////////////////////////////////////////////////////////////////////////////////

MaPhp5HandlerService::~MaPhp5HandlerService()
{
#if BLD_FEATURE_LOG
	delete log;
#endif
}

////////////////////////////////////////////////////////////////////////////////

int MaPhp5HandlerService::start()
{
	char					*serverRoot;
#ifdef ZTS
	void 					***tsrm_ls;
	php_core_globals 		*core_globals;
	sapi_globals_struct 	*sapi_globals;
	zend_llist				global_vars;
	zend_compiler_globals 	*compiler_globals;
	zend_executor_globals 	*executor_globals;

	tsrm_startup(128, 1, 0, 0);
	compiler_globals = (zend_compiler_globals*) 
		ts_resource(compiler_globals_id);
	executor_globals = (zend_executor_globals*) 
		ts_resource(executor_globals_id);
	core_globals = (php_core_globals*) ts_resource(core_globals_id);
	sapi_globals = (sapi_globals_struct*) ts_resource(sapi_globals_id);
	tsrm_ls = (void***) ts_resource(0);
#endif

	//
	//	Define the php.ini location to be the ServerRoot
	//
	serverRoot = MaServer::getDefaultServer()->getServerRoot();
	php5SapiBlock.php_ini_path_override = serverRoot;

	sapi_startup(&php5SapiBlock);

	if (php_module_startup(&php5SapiBlock, 0, 0) == FAILURE) {
		mprLog(0, log, "Can't startup PHP\n");
		return -1;
	}

#ifdef ZTS
	zend_llist_init(&global_vars, sizeof(char *), 0, 0);  
#endif

	//
	//	Set PHP defaults. As Appweb buffers output, we don't want PHP
	//	to call flush.
	//
	SG(options) |= SAPI_OPTION_NO_CHDIR;
	zend_alter_ini_entry("register_argc_argv", 19, "0", 1, PHP_INI_SYSTEM, 
		PHP_INI_STAGE_ACTIVATE);
	zend_alter_ini_entry("html_errors", 12, "0", 1, PHP_INI_SYSTEM, 
		PHP_INI_STAGE_ACTIVATE);
	zend_alter_ini_entry("implicit_flush", 15, "0", 1, PHP_INI_SYSTEM, 
		PHP_INI_STAGE_ACTIVATE);
	zend_alter_ini_entry("max_execution_time", 19, "0", 1, PHP_INI_SYSTEM, 
		PHP_INI_STAGE_ACTIVATE);
	return 0;
}

////////////////////////////////////////////////////////////////////////////////

int MaPhp5HandlerService::stop()
{
	TSRMLS_FETCH();
//	php_request_shutdown((void*) 0);
	php_module_shutdown(TSRMLS_C);
	sapi_shutdown();

#ifdef ZTS
    tsrm_shutdown();
#endif
	return 0;
}

////////////////////////////////////////////////////////////////////////////////
//
//	Create a new handler for a new connection (accept)
//

MaHandler *MaPhp5HandlerService::newHandler(MaServer *server, MaHost *host, 
	char *extensions)
{
	return new MaPhp5Handler(log, extensions);
}

////////////////////////////////////////////////////////////////////////////////
/////////////////////////////// MaPhp5Handler //////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
//
//	One handler per request
//

MaPhp5Handler::MaPhp5Handler(MprLogModule *serviceLog, char *extensions) : 
	MaHandler(MA_PHP_HANDLER_NAME, extensions, 
		MPR_HANDLER_GET | MPR_HANDLER_POST | 
		MPR_HANDLER_HEAD | MPR_HANDLER_NEED_ENV | MPR_HANDLER_TERMINAL)
{
	log = serviceLog;
}

////////////////////////////////////////////////////////////////////////////////

MaPhp5Handler::~MaPhp5Handler()
{
}

////////////////////////////////////////////////////////////////////////////////
//
//	Clone a new handler for a new request on this connection (keep-alive)
//

MaHandler *MaPhp5Handler::cloneHandler()
{
	return new MaPhp5Handler(log, extensions);
}

////////////////////////////////////////////////////////////////////////////////
//
//	Service a new request
//
 
int MaPhp5Handler::run(MaRequest *rq)
{
	MaDataStream	*dynBuf;
	MprVar			*vp, *variables;
	int				i, index, flags, numItems;


	hitCount++;

	flags= rq->getFlags();
	dynBuf = rq->getDynBuf();
	rq->insertDataStream(dynBuf);
	rq->setResponseCode(200);
	rq->setHeaderFlags(MPR_HTTP_DONT_CACHE, 0);
	rq->setPullPost();

	//
	//	Initialize PHP
	//
 	TSRMLS_FETCH();

	zend_first_try {
		phpInitialized = 0;
		func_data = rq;
		var_array = 0;

		SG(server_context) = rq;
		SG(request_info).path_translated = rq->getFileName();
		SG(request_info).request_method = rq->getMethod();
		SG(request_info).request_uri = rq->getUri();
		SG(request_info).query_string = rq->getQueryString();
		SG(request_info).content_type = rq->getRequestContentMimeType();
		SG(request_info).content_length = rq->getContentLength();
		SG(sapi_headers).http_response_code = 200;
		SG(request_info).auth_user = rq->getUser();
		SG(request_info).auth_password = rq->getPassword();

		php_request_startup(TSRMLS_C);
		CG(zend_lineno) = 0;

	} zend_catch {

		mprLog(1, log, "PHP startup failed\n");
		zend_try {
			php_request_shutdown(0);
		} zend_end_try();
		rq->requestError(MPR_HTTP_INTERNAL_SERVER_ERROR, 
			"PHP initialization failed");
		return MPR_HTTP_HANDLER_FINISHED_PROCESSING;

	} zend_end_try();

	phpInitialized = 1;
	
	//
	//	Build environment variables
	//
	variables = rq->getVariables();
	numItems = rq->getNumEnvProperties();

	index = 0;
	for (i = 0; i < MA_HTTP_OBJ_MAX; i++) {
		if (variables[i].type == MPR_TYPE_OBJECT) {
			vp = mprGetFirstProperty(&variables[i], MPR_ENUM_DATA);
			while (vp) {
				if (vp->type == MPR_TYPE_STRING && vp->string != 0) {
					php_register_variable(vp->name, vp->string, 
						var_array TSRMLS_CC);
				} else {
					char	*buf;

					mprVarToString(&buf, MPR_MAX_STRING, 0, vp);
					php_register_variable(vp->name, buf, var_array TSRMLS_CC);
					mprFree(buf);
				}
				index++;
				vp = mprGetNextProperty(&variables[i], vp, MPR_ENUM_DATA);
			}
		}
	}

	//
	//	Execute the PHP script
	//
	if (execScript(rq) < 0) {
		zend_try {
			php_request_shutdown(0);
		} zend_end_try();

		rq->requestError(MPR_HTTP_INTERNAL_SERVER_ERROR, 
			"PHP script execution failed");
		return MPR_HTTP_HANDLER_FINISHED_PROCESSING;
	}

	zend_try {
		php_request_shutdown(0);
	} zend_end_try();

	//
	// Flush the output and write the headers
	//
	rq->flushOutput(MPR_HTTP_BACKGROUND_FLUSH, MPR_HTTP_FINISH_REQUEST);
	return MPR_HTTP_HANDLER_FINISHED_PROCESSING;
}

////////////////////////////////////////////////////////////////////////////////

int MaPhp5Handler::execScript(MaRequest *rq)
{
	zend_file_handle file_handle;

	TSRMLS_FETCH();

	file_handle.filename = rq->getFileName();
	file_handle.free_filename = 0;
	file_handle.type = ZEND_HANDLE_FILENAME;
	file_handle.opened_path = 0;

	zend_try {
		php_execute_script(&file_handle TSRMLS_CC);
		if (!SG(headers_sent)) {
			sapi_send_headers(TSRMLS_C);
		}
	} zend_catch {
		mprLog(1, log, "PHP exec failed\n");
		return -1;
	} zend_end_try();

	return 0;
}

////////////////////////////////////////////////////////////////////////////////
///////////////////////////// PHP Support Functions ////////////////////////////
////////////////////////////////////////////////////////////////////////////////
//
//	Flush write data back to the client. NOTE: this uses the Appweb DataStream
//	buffering mechanism.
//

static void php5Flush(void *server_context)
{
	MaRequest		*rq = (MaRequest*) server_context;

	if (rq) {
		/*
 		 *	PHP may call here on shutdown, so we need to test 
		 */
		if (! mprGetMpr()->isExiting()) {
			rq->setHeaderFlags(MPR_HTTP_FLUSHED, 0);
			rq->flushOutput(MPR_HTTP_FOREGROUND_FLUSH, 0);
		}
	}
}

////////////////////////////////////////////////////////////////////////////////
//
//	Write data back to the client. NOTE: this uses the Appweb DataStream
//	buffering mechanism.
//

static int php5Write(const char *str, uint str_length TSRMLS_DC)
{
	MaRequest		*rq = (MaRequest*) SG(server_context);
	long 			written;

	if (rq == 0) {
		return -1;
	}

	written = rq->write((char*) str, str_length);
	if (written <= 0) {
		php_handle_aborted_connection();
	}
	return written;
}

////////////////////////////////////////////////////////////////////////////////

static void php5RegisterVariables(zval *track_vars_array TSRMLS_DC)
{
	MaRequest		*rq = (MaRequest*) SG(server_context);
	MaPhp5Handler	*hp = (MaPhp5Handler*) rq->getHandler();

	php_import_environment_variables(track_vars_array TSRMLS_CC);

	if (SG(request_info).request_uri) {
		php_register_variable("PHP_SELF", SG(request_info).request_uri, 
			track_vars_array TSRMLS_CC);
	}
	hp->var_array = track_vars_array;
}

////////////////////////////////////////////////////////////////////////////////
//
//	Log a message
//

static void php5LogMessage(char *message)
{
   mprLog(0, "phpModule: %s", message);
}

////////////////////////////////////////////////////////////////////////////////
//
//	Read the cookie associated with this request.
//
static char *php5ReadCookies(TSRMLS_D)
{
#if BLD_FEATURE_COOKIE
	MaRequest	*rq = (MaRequest*) SG(server_context);
	return (char*) rq->getCookies();
#else
	return 0;
#endif
}

////////////////////////////////////////////////////////////////////////////////
//
//	Appweb will automatically flush headers before the output when flushOutput
//	is called.
//

static int php5SendHeaders(sapi_headers_struct *sapi_headers TSRMLS_DC)
{
	MaRequest	*rq = (MaRequest*) SG(server_context);

	rq->setResponseCode(sapi_headers->http_response_code);

    if (SG(sapi_headers).send_default_content_type) {
		rq->setResponseMimeType("text/html");
    }

	return SAPI_HEADER_SENT_SUCCESSFULLY;
}

////////////////////////////////////////////////////////////////////////////////

static int php5WriteHeader(sapi_header_struct *sapi_header, 
	sapi_headers_struct *sapi_headers TSRMLS_DC)
{
	MaRequest	*rq = (MaRequest*) SG(server_context);

	rq->setHeader(sapi_header->header, !sapi_header->replace); 
	//
	//	This causes a crash
	// 		efree(sapi_header->header);
	//
	// sapi_free_header(sapi_header);
	//
	return SAPI_HEADER_ADD;
}

////////////////////////////////////////////////////////////////////////////////

static int php5ReadPostData(char *buffer, uint len TSRMLS_DC)
{
	MaRequest	*rq = (MaRequest*) SG(server_context);

	return rq->readPostData(buffer, len);
}

////////////////////////////////////////////////////////////////////////////////

static int php5SapiStartup(sapi_module_struct *sapi_module)
{
	return php_module_startup(sapi_module, 0, 0);
}

////////////////////////////////////////////////////////////////////////////////
#else
void mprPhp5HandlerDummy() {}

#endif // BLD_FEATURE_PHP5_MODULE

//
// Local variables:
// tab-width: 4
// c-basic-offset: 4
// End:
// vim: sw=4 ts=4 
//
