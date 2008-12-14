/*
 *	@file 	espEnv.h
 *	@brief 	ESP Environment Variables
 */
/********************************* Copyright **********************************/
/*
 *	@copy	default
 *	
 *	Copyright (c) Mbedthis Software LLC, 2003-2007. All Rights Reserved.
 *	
 *	This software is distributed under commercial and open source licenses.
 *	You may use the GPL open source license described below or you may acquire 
 *	a commercial license from Mbedthis Software. You agree to be fully bound 
 *	by the terms of either license. Consult the LICENSE.TXT distributed with 
 *	this software for full details.
 *	
 *	This software is open source; you can redistribute it and/or modify it 
 *	under the terms of the GNU General Public License as published by the 
 *	Free Software Foundation; either version 2 of the License, or (at your 
 *	option) any later version. See the GNU General Public License for more 
 *	details at: http://www.mbedthis.com/downloads/gplLicense.html
 *	
 *	This program is distributed WITHOUT ANY WARRANTY; without even the 
 *	implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. 
 *	
 *	This GPL license does NOT permit incorporating this software into 
 *	proprietary programs. If you are unable to comply with the GPL, you must
 *	acquire a commercial license to use this software. Commercial licenses 
 *	for this software and support services are available from Mbedthis 
 *	Software at http://www.mbedthis.com 
 *	
 *	@end
 */

/******************************************************************************/

#ifndef _h_ESP_ENV_h
#define _h_ESP_ENV_h 1

/*
 *	@brief Scripting environment variable array types
 */
typedef enum EspEnvType {
	ESP_UNDEFINED_OBJ = -1,

	/**	
	 *	Elements for server[]: 
	 *		DOCUMENT_ROOT GATEWAY_INTERFACE SERVER_ADDR SERVER_PORT SERVER_NAME 
	 *		SERVER_PROTOCOL SERVER_SOFTWARE SERVER_URL UPLOAD_DIR
	 *		FUTURE: SERVER_ADMIN 
 	 *		FUTURE: this could be shared across all hosts and be made read-only.
	 */
	ESP_SERVER_OBJ = 0,						/*! server[] data */

	/**
	 *	Elements for session[]: are user defined
	 */
	ESP_SESSION_OBJ = 1,					/*! session[] data */

	/**	
 	 *	Elements for request[]: 
	 *		AUTH_TYPE CONTENT_LENGTH CONTENT_TYPE QUERY_STRING PATH_INFO 
	 *		PATH_TRANSLATED REMOTE_ADDR REMOTE_HOST REMOTE_USER REQUEST_METHOD 
	 *		REQUEST_URI SCRIPT_FILENAME SCRIPT_NAME 
	 *		FUTURE: FILEPATH_INFO REDIRECT_URL SELF REMOTE_PORT AUTH_USER 
	 *		AUTH_GROUP AUTH_ACL 
	 */
	ESP_REQUEST_OBJ = 2,					/*! request[] data */

	/**	
	 *	Elements for headers[]: 
	 *		HTTP_ACCEPT HTTP_ACCEPT_CHARSET HTTP_CONNECTION HTTP_HOST 
 	 *		HTTP_REFERER HTTP_USER_AGENT and any other custom headers
	 */
	ESP_HEADERS_OBJ = 3,					/*! header [] data */

	/**
 	 *	Elements for cookies[]: are defined by the HTTP request
	 */
	ESP_COOKIES_OBJ = 4,					/*! cookies[] data */
	
	/**
 	 *	Elements for files[]: are defined by the HTTP request
 	 *		CLIENT_FILENAME CONTENT_TYPE FILENAME SIZE
 	 */
	ESP_FILES_OBJ = 5,						/*! files[] data */

	/**
 	 *	Elements for form[]: are defined by the HTTP request
 	 */
	ESP_FORM_OBJ = 6,						/*! form[] data */

	/**
 	 *	Elements for application[]: are user defined
 	 */
	ESP_APPLICATION_OBJ = 7,				/*! application[] data */

	/**
 	 *	Elements for global[]: are defined by ESP/EJS
 	 */
	ESP_GLOBAL_OBJ = 8,						/*! global [] data */

	/*
 	 *	Elements for local[]: are defined by ESP/EJS
 	 */
	ESP_LOCAL_OBJ = 9						/*! local [] data */
} EspEnvType;

#define ESP_OBJ_MAX 	10					/* Total objects */

#if BLD_SQUEEZE
#define ESP_HASH_SIZE	19					/* Size of hash tables */
#else
#define ESP_HASH_SIZE	37
#endif

/******************************************************************************/
#endif /* _h_ESP_ENV_h */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim: sw=4 ts=4 
 */
