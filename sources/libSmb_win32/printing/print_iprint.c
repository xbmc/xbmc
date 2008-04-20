/*
 * Support code for Novell iPrint using the Common UNIX Printing
 * System ("CUPS") libraries
 *
 * Copyright 1999-2003 by Michael R Sweet.
 * Portions Copyright 2005 by Joel J. Smith.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include "includes.h"
#include "printing.h"

#ifdef HAVE_IPRINT
#include <cups/cups.h>
#include <cups/language.h>

#define OPERATION_NOVELL_LIST_PRINTERS          0x401A
#define OPERATION_NOVELL_MGMT                   0x401C
#define NOVELL_SERVER_SYSNAME			"sysname="
#define NOVELL_SERVER_SYSNAME_NETWARE		"NetWare IA32"
#define NOVELL_SERVER_VERSION_STRING		"iprintserverversion="
#define NOVELL_SERVER_VERSION_OES_SP1		33554432

/*
 * 'iprint_passwd_cb()' - The iPrint password callback...
 */

static const char *				/* O - Password or NULL */
iprint_passwd_cb(const char *prompt)	/* I - Prompt */
{
       /*
	* Always return NULL to indicate that no password is available...
	*/

	return (NULL);
}

static const char *iprint_server(void)
{
	if ((lp_iprint_server() != NULL) && (strlen(lp_iprint_server()) > 0)) {
		DEBUG(10, ("iprint server explicitly set to %s\n",
			   lp_iprint_server()));
		return lp_iprint_server();
	}

	DEBUG(10, ("iprint server left to default %s\n", cupsServer()));
	return cupsServer();
}

/*
 * Pass in an already connected http_t*
 * Returns the server version if one can be found, multiplied by
 * -1 for all NetWare versions.  Returns 0 if a server version
 * cannot be determined
 */

static int iprint_get_server_version(http_t *http, char* serviceUri)
{
	ipp_t		*request = NULL,	/* IPP Request */
			*response = NULL;	/* IPP Response */
	ipp_attribute_t	*attr;			/* Current attribute */
	cups_lang_t	*language = NULL;	/* Default language */
	char		*ver;			/* server version pointer */
	char		*vertmp;		/* server version tmp pointer */
	int		serverVersion = 0;	/* server version */
	char		*os;			/* server os */
	int		osFlag = 0;		/* 0 for NetWare, 1 for anything else */
	char		*temp;			/* pointer for string manipulation */

       /*
	* Build an OPERATION_NOVELL_MGMT("get-server-version") request,
	* which requires the following attributes:
	*
	*    attributes-charset
	*    attributes-natural-language
	*    operation-name
	*    service-uri
	*/

	request = ippNew();

	request->request.op.operation_id = OPERATION_NOVELL_MGMT;
	request->request.op.request_id   = 1;

	language = cupsLangDefault();

	ippAddString(request, IPP_TAG_OPERATION, IPP_TAG_CHARSET,
	             "attributes-charset", NULL, "utf-8");

	ippAddString(request, IPP_TAG_OPERATION, IPP_TAG_LANGUAGE,
	             "attributes-natural-language", NULL, language->language);

	ippAddString(request, IPP_TAG_OPERATION, IPP_TAG_URI,
	             "service-uri", NULL, serviceUri);

	ippAddString(request, IPP_TAG_OPERATION, IPP_TAG_KEYWORD,
	              "operation-name", NULL, "get-server-version");

       /*
	* Do the request and get back a response...
	*/

	if (((response = cupsDoRequest(http, request, "/ipp/")) == NULL) ||
	    (response->request.status.status_code >= IPP_OK_CONFLICT))
		goto out;

	if (((attr = ippFindAttribute(response, "server-version",
	                              IPP_TAG_STRING)) != NULL)) {
		if ((ver = strstr(attr->values[0].string.text,
                                  NOVELL_SERVER_VERSION_STRING)) != NULL) {
			ver += strlen(NOVELL_SERVER_VERSION_STRING);
		       /*
			* Strangely, libcups stores a IPP_TAG_STRING (octet
			* string) as a null-terminated string with no length
			* even though it could be binary data with nulls in
			* it.  Luckily, in this case the value is not binary.
			*/
			serverVersion = strtol(ver, &vertmp, 10);

			/* Check for not found, overflow or negative version */
			if ((ver == vertmp) || (serverVersion < 0))
				serverVersion = 0;
		}

		if ((os = strstr(attr->values[0].string.text,
                                  NOVELL_SERVER_SYSNAME)) != NULL) {
			os += strlen(NOVELL_SERVER_SYSNAME);
			if ((temp = strchr(os,'<')) != NULL)
				*temp = '\0';
			if (strcmp(os,NOVELL_SERVER_SYSNAME_NETWARE))
				osFlag = 1; /* 1 for non-NetWare systems */
		}
	}

 out:
	if (response)
		ippDelete(response);

	if (language)
		cupsLangFree(language);

	if (osFlag == 0)
		serverVersion *= -1;

	return serverVersion;
}


static int iprint_cache_add_printer(http_t *http,
				   int reqId,
				   char* url)
{
	ipp_t		*request = NULL,	/* IPP Request */
			*response = NULL;	/* IPP Response */
	ipp_attribute_t	*attr;			/* Current attribute */
	cups_lang_t	*language = NULL;	/* Default language */
	char		*name,			/* printer-name attribute */
			*info,			/* printer-info attribute */
			smb_enabled,		/* smb-enabled attribute */
			secure;			/* security-enabled attrib. */

	char		*httpPath;	/* path portion of the printer-uri */

	static const char *pattrs[] =	/* Requested printer attributes */
			{
			  "printer-name",
			  "security-enabled",
			  "printer-info",
			  "smb-enabled"
			};       

	request = ippNew();

	request->request.op.operation_id = IPP_GET_PRINTER_ATTRIBUTES;
	request->request.op.request_id   = reqId;

	language = cupsLangDefault();

	ippAddString(request, IPP_TAG_OPERATION, IPP_TAG_CHARSET,
	             "attributes-charset", NULL, "utf-8");

	ippAddString(request, IPP_TAG_OPERATION, IPP_TAG_LANGUAGE,
	             "attributes-natural-language", NULL, language->language);

	ippAddString(request, IPP_TAG_OPERATION, IPP_TAG_URI, "printer-uri", NULL, url);

	ippAddStrings(request, IPP_TAG_OPERATION, IPP_TAG_KEYWORD,
	              "requested-attributes",
	              (sizeof(pattrs) / sizeof(pattrs[0])),
	              NULL, pattrs);

	/*
	 * Do the request and get back a response...
	 */

	if ((httpPath = strstr(url,"://")) == NULL ||
			(httpPath = strchr(httpPath+3,'/')) == NULL)
	{
		ippDelete(request);
		request = NULL;
		goto out;
	}

	if ((response = cupsDoRequest(http, request, httpPath)) == NULL) {
		ipp_status_t lastErr = cupsLastError();

	       /*
		* Ignore printers that cannot be queried without credentials
		*/
		if (lastErr == IPP_FORBIDDEN || 
		    lastErr == IPP_NOT_AUTHENTICATED ||
		    lastErr == IPP_NOT_AUTHORIZED)
			goto out;

		DEBUG(0,("Unable to get printer list - %s\n",
		      ippErrorString(lastErr)));
		goto out;
	}

	for (attr = response->attrs; attr != NULL;) {
	       /*
		* Skip leading attributes until we hit a printer...
		*/

		while (attr != NULL && attr->group_tag != IPP_TAG_PRINTER)
			attr = attr->next;

		if (attr == NULL)
			break;

	       /*
		* Pull the needed attributes from this printer...
		*/

		name       = NULL;
		info       = NULL;
		smb_enabled= 1;
		secure     = 0;

		while (attr != NULL && attr->group_tag == IPP_TAG_PRINTER) {
			if (strcmp(attr->name, "printer-name") == 0 &&
			    attr->value_tag == IPP_TAG_NAME)
				name = attr->values[0].string.text;

			if (strcmp(attr->name, "printer-info") == 0 &&
			    (attr->value_tag == IPP_TAG_TEXT ||
			    attr->value_tag == IPP_TAG_TEXTLANG))
				info = attr->values[0].string.text;

		       /*
			* If the smb-enabled attribute is present and the
			* value is set to 0, don't show the printer.
			* If the attribute is not present, assume that the
			* printer should show up
			*/
			if (!strcmp(attr->name, "smb-enabled") &&
			    ((attr->value_tag == IPP_TAG_INTEGER &&
			    !attr->values[0].integer) ||
			    (attr->value_tag == IPP_TAG_BOOLEAN &&
			    !attr->values[0].boolean)))
				smb_enabled = 0;

		       /*
			* If the security-enabled attribute is present and the
			* value is set to 1, don't show the printer.
			* If the attribute is not present, assume that the
			* printer should show up
			*/
			if (!strcmp(attr->name, "security-enabled") &&
			    ((attr->value_tag == IPP_TAG_INTEGER &&
			    attr->values[0].integer) ||
			    (attr->value_tag == IPP_TAG_BOOLEAN &&
			    attr->values[0].boolean)))
				secure = 1;

			attr = attr->next;
		}

	       /*
		* See if we have everything needed...
		* Make sure the printer is not a secure printer
		* and make sure smb printing hasn't been explicitly
		* disabled for the printer
		*/

		if (name != NULL && !secure && smb_enabled) 
			pcap_cache_add(name, info);
	}

 out:
	if (response)
		ippDelete(response);
	return(0);
}

BOOL iprint_cache_reload(void)
{
	http_t		*http = NULL;		/* HTTP connection to server */
	ipp_t		*request = NULL,	/* IPP Request */
			*response = NULL;	/* IPP Response */
	ipp_attribute_t	*attr;			/* Current attribute */
	cups_lang_t	*language = NULL;	/* Default language */
	int		i;
	BOOL ret = False;

	DEBUG(5, ("reloading iprint printcap cache\n"));

       /*
	* Make sure we don't ask for passwords...
	*/

	cupsSetPasswordCB(iprint_passwd_cb);

       /*
	* Try to connect to the server...
	*/

	if ((http = httpConnect(iprint_server(), ippPort())) == NULL) {
		DEBUG(0,("Unable to connect to iPrint server %s - %s\n", 
			 iprint_server(), strerror(errno)));
		goto out;
	}

       /*
	* Build a OPERATION_NOVELL_LIST_PRINTERS request, which requires the following attributes:
	*
	*    attributes-charset
	*    attributes-natural-language
	*/

	request = ippNew();

	request->request.op.operation_id = OPERATION_NOVELL_LIST_PRINTERS;
	request->request.op.request_id   = 1;

	language = cupsLangDefault();

	ippAddString(request, IPP_TAG_OPERATION, IPP_TAG_CHARSET,
	             "attributes-charset", NULL, "utf-8");

	ippAddString(request, IPP_TAG_OPERATION, IPP_TAG_LANGUAGE,
	             "attributes-natural-language", NULL, language->language);

	ippAddString(request, IPP_TAG_OPERATION, IPP_TAG_KEYWORD,
	             "ipp-server", NULL, "ippSrvr");

       /*
	* Do the request and get back a response...
	*/

	if ((response = cupsDoRequest(http, request, "/ipp")) == NULL) {
		DEBUG(0,("Unable to get printer list - %s\n",
			 ippErrorString(cupsLastError())));
		goto out;
	}

	for (attr = response->attrs; attr != NULL;) {
	       /*
		* Skip leading attributes until we hit a printer...
		*/

		while (attr != NULL && attr->group_tag != IPP_TAG_PRINTER)
			attr = attr->next;

		if (attr == NULL)
			break;

	       /*
		* Pull the needed attributes from this printer...
		*/

		while (attr != NULL && attr->group_tag == IPP_TAG_PRINTER)
		{
			if (strcmp(attr->name, "printer-name") == 0 &&
			    (attr->value_tag == IPP_TAG_URI ||
			     attr->value_tag == IPP_TAG_NAME ||
			     attr->value_tag == IPP_TAG_TEXT ||
			     attr->value_tag == IPP_TAG_NAMELANG ||
			     attr->value_tag == IPP_TAG_TEXTLANG))
			{
				for (i = 0; i<attr->num_values; i++)
				{
					char *url = attr->values[i].string.text;
					if (!url || !strlen(url))
						continue;
					iprint_cache_add_printer(http, i+2, url);
				}
			}
			attr = attr->next;
		}
	}

	ret = True;

 out:
	if (response)
		ippDelete(response);

	if (language)
		cupsLangFree(language);

	if (http)
		httpClose(http);

	return ret;
}


/*
 * 'iprint_job_delete()' - Delete a job.
 */

static int iprint_job_delete(const char *sharename, const char *lprm_command, struct printjob *pjob)
{
	int		ret = 1;		/* Return value */
	http_t		*http = NULL;		/* HTTP connection to server */
	ipp_t		*request = NULL,	/* IPP Request */
			*response = NULL;	/* IPP Response */
	cups_lang_t	*language = NULL;	/* Default language */
	char		uri[HTTP_MAX_URI];	/* printer-uri attribute */
	char		httpPath[HTTP_MAX_URI];	/* path portion of the printer-uri */


	DEBUG(5,("iprint_job_delete(%s, %p (%d))\n", sharename, pjob, pjob->sysjob));

       /*
	* Make sure we don't ask for passwords...
	*/

	cupsSetPasswordCB(iprint_passwd_cb);

       /*
	* Try to connect to the server...
	*/

	if ((http = httpConnect(iprint_server(), ippPort())) == NULL) {
		DEBUG(0,("Unable to connect to iPrint server %s - %s\n", 
			 iprint_server(), strerror(errno)));
		goto out;
	}

       /*
	* Build an IPP_CANCEL_JOB request, which uses the following
	* attributes:
	*
	*    attributes-charset
	*    attributes-natural-language
	*    printer-uri
	*    job-id
	*    requesting-user-name
	*/

	request = ippNew();

	request->request.op.operation_id = IPP_CANCEL_JOB;
	request->request.op.request_id   = 1;

	language = cupsLangDefault();

	ippAddString(request, IPP_TAG_OPERATION, IPP_TAG_CHARSET,
	             "attributes-charset", NULL, "utf-8");

	ippAddString(request, IPP_TAG_OPERATION, IPP_TAG_LANGUAGE,
	             "attributes-natural-language", NULL, language->language);

	slprintf(uri, sizeof(uri) - 1, "ipp://%s/ipp/%s", iprint_server(), sharename);

	ippAddString(request, IPP_TAG_OPERATION, IPP_TAG_URI, "printer-uri", NULL, uri);

	ippAddInteger(request, IPP_TAG_OPERATION, IPP_TAG_INTEGER, "job-id", pjob->sysjob);

	ippAddString(request, IPP_TAG_OPERATION, IPP_TAG_NAME, "requesting-user-name",
	             NULL, pjob->user);

       /*
	* Do the request and get back a response...
	*/

	slprintf(httpPath, sizeof(httpPath) - 1, "/ipp/%s", sharename);

	if ((response = cupsDoRequest(http, request, httpPath)) != NULL) {
		if (response->request.status.status_code >= IPP_OK_CONFLICT) {
			DEBUG(0,("Unable to cancel job %d - %s\n", pjob->sysjob,
				ippErrorString(cupsLastError())));
		} else {
			ret = 0;
		}
	} else {
		DEBUG(0,("Unable to cancel job %d - %s\n", pjob->sysjob,
			ippErrorString(cupsLastError())));
	}

 out:
	if (response)
		ippDelete(response);

	if (language)
		cupsLangFree(language);

	if (http)
		httpClose(http);

	return ret;
}


/*
 * 'iprint_job_pause()' - Pause a job.
 */

static int iprint_job_pause(int snum, struct printjob *pjob)
{
	int		ret = 1;		/* Return value */
	http_t		*http = NULL;		/* HTTP connection to server */
	ipp_t		*request = NULL,	/* IPP Request */
			*response = NULL;	/* IPP Response */
	cups_lang_t	*language = NULL;	/* Default language */
	char		uri[HTTP_MAX_URI];	/* printer-uri attribute */
	char		httpPath[HTTP_MAX_URI];	/* path portion of the printer-uri */


	DEBUG(5,("iprint_job_pause(%d, %p (%d))\n", snum, pjob, pjob->sysjob));

       /*
	* Make sure we don't ask for passwords...
	*/

	cupsSetPasswordCB(iprint_passwd_cb);

       /*
	* Try to connect to the server...
	*/

	if ((http = httpConnect(iprint_server(), ippPort())) == NULL) {
		DEBUG(0,("Unable to connect to iPrint server %s - %s\n", 
			 iprint_server(), strerror(errno)));
		goto out;
	}

       /*
	* Build an IPP_HOLD_JOB request, which requires the following
	* attributes:
	*
	*    attributes-charset
	*    attributes-natural-language
	*    printer-uri
	*    job-id
	*    requesting-user-name
	*/

	request = ippNew();

	request->request.op.operation_id = IPP_HOLD_JOB;
	request->request.op.request_id   = 1;

	language = cupsLangDefault();

	ippAddString(request, IPP_TAG_OPERATION, IPP_TAG_CHARSET,
	             "attributes-charset", NULL, "utf-8");

	ippAddString(request, IPP_TAG_OPERATION, IPP_TAG_LANGUAGE,
	             "attributes-natural-language", NULL, language->language);

	slprintf(uri, sizeof(uri) - 1, "ipp://%s/ipp/%s", iprint_server(), PRINTERNAME(snum));

	ippAddString(request, IPP_TAG_OPERATION, IPP_TAG_URI, "printer-uri", NULL, uri);

	ippAddInteger(request, IPP_TAG_OPERATION, IPP_TAG_INTEGER, "job-id", pjob->sysjob);

	ippAddString(request, IPP_TAG_OPERATION, IPP_TAG_NAME, "requesting-user-name",
	             NULL, pjob->user);

       /*
	* Do the request and get back a response...
	*/

	slprintf(httpPath, sizeof(httpPath) - 1, "/ipp/%s", PRINTERNAME(snum));

	if ((response = cupsDoRequest(http, request, httpPath)) != NULL) {
		if (response->request.status.status_code >= IPP_OK_CONFLICT) {
			DEBUG(0,("Unable to hold job %d - %s\n", pjob->sysjob,
				ippErrorString(cupsLastError())));
		} else {
			ret = 0;
		}
	} else {
		DEBUG(0,("Unable to hold job %d - %s\n", pjob->sysjob,
			ippErrorString(cupsLastError())));
	}

 out:
	if (response)
		ippDelete(response);

	if (language)
		cupsLangFree(language);

	if (http)
		httpClose(http);

	return ret;
}


/*
 * 'iprint_job_resume()' - Resume a paused job.
 */

static int iprint_job_resume(int snum, struct printjob *pjob)
{
	int		ret = 1;		/* Return value */
	http_t		*http = NULL;		/* HTTP connection to server */
	ipp_t		*request = NULL,	/* IPP Request */
			*response = NULL;	/* IPP Response */
	cups_lang_t	*language = NULL;	/* Default language */
	char		uri[HTTP_MAX_URI];	/* printer-uri attribute */
	char		httpPath[HTTP_MAX_URI];	/* path portion of the printer-uri */


	DEBUG(5,("iprint_job_resume(%d, %p (%d))\n", snum, pjob, pjob->sysjob));

       /*
	* Make sure we don't ask for passwords...
	*/

	cupsSetPasswordCB(iprint_passwd_cb);

       /*
	* Try to connect to the server...
	*/

	if ((http = httpConnect(iprint_server(), ippPort())) == NULL) {
		DEBUG(0,("Unable to connect to iPrint server %s - %s\n", 
			 iprint_server(), strerror(errno)));
		goto out;
	}

       /*
	* Build an IPP_RELEASE_JOB request, which requires the following
	* attributes:
	*
	*    attributes-charset
	*    attributes-natural-language
	*    printer-uri
	*    job-id
	*    requesting-user-name
	*/

	request = ippNew();

	request->request.op.operation_id = IPP_RELEASE_JOB;
	request->request.op.request_id   = 1;

	language = cupsLangDefault();

	ippAddString(request, IPP_TAG_OPERATION, IPP_TAG_CHARSET,
	             "attributes-charset", NULL, "utf-8");

	ippAddString(request, IPP_TAG_OPERATION, IPP_TAG_LANGUAGE,
	             "attributes-natural-language", NULL, language->language);

	slprintf(uri, sizeof(uri) - 1, "ipp://%s/ipp/%s", iprint_server(), PRINTERNAME(snum));

	ippAddString(request, IPP_TAG_OPERATION, IPP_TAG_URI, "printer-uri", NULL, uri);

	ippAddInteger(request, IPP_TAG_OPERATION, IPP_TAG_INTEGER, "job-id", pjob->sysjob);

	ippAddString(request, IPP_TAG_OPERATION, IPP_TAG_NAME, "requesting-user-name",
	             NULL, pjob->user);

       /*
	* Do the request and get back a response...
	*/

	slprintf(httpPath, sizeof(httpPath) - 1, "/ipp/%s", PRINTERNAME(snum));

	if ((response = cupsDoRequest(http, request, httpPath)) != NULL) {
		if (response->request.status.status_code >= IPP_OK_CONFLICT) {
			DEBUG(0,("Unable to release job %d - %s\n", pjob->sysjob,
				ippErrorString(cupsLastError())));
		} else {
			ret = 0;
		}
	} else {
		DEBUG(0,("Unable to release job %d - %s\n", pjob->sysjob,
			ippErrorString(cupsLastError())));
	}

 out:
	if (response)
		ippDelete(response);

	if (language)
		cupsLangFree(language);

	if (http)
		httpClose(http);

	return ret;
}


/*
 * 'iprint_job_submit()' - Submit a job for printing.
 */

static int iprint_job_submit(int snum, struct printjob *pjob)
{
	int		ret = 1;		/* Return value */
	http_t		*http = NULL;		/* HTTP connection to server */
	ipp_t		*request = NULL,	/* IPP Request */
			*response = NULL;	/* IPP Response */
	ipp_attribute_t	*attr;		/* Current attribute */
	cups_lang_t	*language = NULL;	/* Default language */
	char		uri[HTTP_MAX_URI]; /* printer-uri attribute */
	char 		*clientname = NULL; 	/* hostname of client for job-originating-host attribute */

	DEBUG(5,("iprint_job_submit(%d, %p (%d))\n", snum, pjob, pjob->sysjob));

       /*
	* Make sure we don't ask for passwords...
	*/

	cupsSetPasswordCB(iprint_passwd_cb);

       /*
	* Try to connect to the server...
	*/

	if ((http = httpConnect(iprint_server(), ippPort())) == NULL) {
		DEBUG(0,("Unable to connect to iPrint server %s - %s\n", 
			 iprint_server(), strerror(errno)));
		goto out;
	}

       /*
	* Build an IPP_PRINT_JOB request, which requires the following
	* attributes:
	*
	*    attributes-charset
	*    attributes-natural-language
	*    printer-uri
	*    requesting-user-name
	*    [document-data]
	*/

	request = ippNew();

	request->request.op.operation_id = IPP_PRINT_JOB;
	request->request.op.request_id   = 1;

	language = cupsLangDefault();

	ippAddString(request, IPP_TAG_OPERATION, IPP_TAG_CHARSET,
	             "attributes-charset", NULL, "utf-8");

	ippAddString(request, IPP_TAG_OPERATION, IPP_TAG_LANGUAGE,
	             "attributes-natural-language", NULL, language->language);

	slprintf(uri, sizeof(uri) - 1, "ipp://%s/ipp/%s", iprint_server(), PRINTERNAME(snum));

	ippAddString(request, IPP_TAG_OPERATION, IPP_TAG_URI,
	             "printer-uri", NULL, uri);

	ippAddString(request, IPP_TAG_OPERATION, IPP_TAG_NAME, "requesting-user-name",
	             NULL, pjob->user);

	clientname = client_name();
	if (strcmp(clientname, "UNKNOWN") == 0) {
		clientname = client_addr();
	}

	ippAddString(request, IPP_TAG_OPERATION, IPP_TAG_NAME,
	             "job-originating-host-name", NULL,
	             clientname);

	ippAddString(request, IPP_TAG_OPERATION, IPP_TAG_NAME, "job-name", NULL,
	             pjob->jobname);

       /*
	* Do the request and get back a response...
	*/

	slprintf(uri, sizeof(uri) - 1, "/ipp/%s", PRINTERNAME(snum));

	if ((response = cupsDoFileRequest(http, request, uri, pjob->filename)) != NULL) {
		if (response->request.status.status_code >= IPP_OK_CONFLICT) {
			DEBUG(0,("Unable to print file to %s - %s\n", PRINTERNAME(snum),
			         ippErrorString(cupsLastError())));
		} else {
			ret = 0;
		}
	} else {
		DEBUG(0,("Unable to print file to `%s' - %s\n", PRINTERNAME(snum),
			 ippErrorString(cupsLastError())));
	}

	if ( ret == 0 )
		unlink(pjob->filename);
	/* else print_job_end will do it for us */

	if ( ret == 0 ) {

		attr = ippFindAttribute(response, "job-id", IPP_TAG_INTEGER);
		if (attr != NULL && attr->group_tag == IPP_TAG_JOB)
		{
			pjob->sysjob = attr->values[0].integer;
		}
	}

 out:
	if (response)
		ippDelete(response);

	if (language)
		cupsLangFree(language);

	if (http)
		httpClose(http);

	return ret;
}

/*
 * 'iprint_queue_get()' - Get all the jobs in the print queue.
 */

static int iprint_queue_get(const char *sharename,
			    enum printing_types printing_type,
			    char *lpq_command,
			    print_queue_struct **q, 
			    print_status_struct *status)
{
	fstring		printername;
	http_t		*http = NULL;		/* HTTP connection to server */
	ipp_t		*request = NULL,	/* IPP Request */
			*response = NULL;	/* IPP Response */
	ipp_attribute_t	*attr = NULL;		/* Current attribute */
	cups_lang_t	*language = NULL;	/* Default language */
	char		uri[HTTP_MAX_URI]; /* printer-uri attribute */
	char		serviceUri[HTTP_MAX_URI]; /* service-uri attribute */
	char		httpPath[HTTP_MAX_URI];	/* path portion of the uri */
	int		jobUseUnixTime = 0;	/* Whether job times should
                                                 * be assumed to be Unix time */
	int		qcount = 0,		/* Number of active queue entries */
			qalloc = 0;		/* Number of queue entries allocated */
	print_queue_struct *queue = NULL,	/* Queue entries */
			*temp;		/* Temporary pointer for queue */
	const char	*user_name,	/* job-originating-user-name attribute */
			*job_name;	/* job-name attribute */
	int		job_id;		/* job-id attribute */
	int		job_k_octets;	/* job-k-octets attribute */
	time_t		job_time;	/* time-at-creation attribute */
	time_t		printer_current_time = 0;	/* printer's current time */
	time_t		printer_up_time = 0;	/* printer's uptime */
	ipp_jstate_t	job_status;	/* job-status attribute */
	int		job_priority;	/* job-priority attribute */
	static const char *jattrs[] =	/* Requested job attributes */
			{
			  "job-id",
			  "job-k-octets",
			  "job-name",
			  "job-originating-user-name",
			  "job-priority",
			  "job-state",
			  "time-at-creation",
			};
	static const char *pattrs[] =	/* Requested printer attributes */
			{
			  "printer-state",
			  "printer-state-message",
			  "printer-current-time",
			  "printer-up-time"
			};

	*q = NULL;

	/* HACK ALERT!!!  The porblem with support the 'printer name' 
	   option is that we key the tdb off the sharename.  So we will 
	   overload the lpq_command string to pass in the printername 
	   (which is basically what we do for non-cups printers ... using 
	   the lpq_command to get the queue listing). */

	fstrcpy( printername, lpq_command );

	DEBUG(5,("iprint_queue_get(%s, %p, %p)\n", printername, q, status));

       /*
	* Make sure we don't ask for passwords...
	*/

	cupsSetPasswordCB(iprint_passwd_cb);

       /*
	* Try to connect to the server...
	*/

	if ((http = httpConnect(iprint_server(), ippPort())) == NULL) {
		DEBUG(0,("Unable to connect to iPrint server %s - %s\n", 
			 iprint_server(), strerror(errno)));
		goto out;
	}

       /*
	* Generate the printer URI and the service URI that goes with it...
	*/

	slprintf(uri, sizeof(uri) - 1, "ipp://%s/ipp/%s", iprint_server(), printername);
	slprintf(serviceUri, sizeof(serviceUri) - 1, "ipp://%s/ipp/", iprint_server());

       /*
	* For Linux iPrint servers from OES SP1 on, the iPrint server
	* uses Unix time for job start times unless it detects the iPrint
	* client in an http User-Agent header.  (This was done to accomodate
	* CUPS broken behavior.  According to RFC 2911, section 4.3.14, job
	* start times are supposed to be relative to how long the printer has
	* been up.)  Since libcups doesn't allow us to set that header before
	* the request is sent, this ugly hack allows us to detect the server
	* version and decide how to interpret the job time.
	*/
	if (iprint_get_server_version(http, serviceUri) >=
	    NOVELL_SERVER_VERSION_OES_SP1)
		jobUseUnixTime = 1;

	request = ippNew();

	request->request.op.operation_id = IPP_GET_PRINTER_ATTRIBUTES;
	request->request.op.request_id   = 2;

	language = cupsLangDefault();

	ippAddString(request, IPP_TAG_OPERATION, IPP_TAG_CHARSET,
	             "attributes-charset", NULL, "utf-8");

	ippAddString(request, IPP_TAG_OPERATION, IPP_TAG_LANGUAGE,
	             "attributes-natural-language", NULL, language->language);

	ippAddString(request, IPP_TAG_OPERATION, IPP_TAG_URI,
	             "printer-uri", NULL, uri);

	ippAddStrings(request, IPP_TAG_OPERATION, IPP_TAG_KEYWORD,
	              "requested-attributes",
	              (sizeof(pattrs) / sizeof(pattrs[0])),
	              NULL, pattrs);

       /*
	* Do the request and get back a response...
	*/

	slprintf(httpPath, sizeof(httpPath) - 1, "/ipp/%s", printername);

	if ((response = cupsDoRequest(http, request, httpPath)) == NULL) {
		DEBUG(0,("Unable to get printer status for %s - %s\n", printername,
			 ippErrorString(cupsLastError())));
		*q = queue;
		goto out;
	}

	if (response->request.status.status_code >= IPP_OK_CONFLICT) {
		DEBUG(0,("Unable to get printer status for %s - %s\n", printername,
			 ippErrorString(response->request.status.status_code)));
		*q = queue;
		goto out;
	}

       /*
	* Get the current printer status and convert it to the SAMBA values.
	*/

	if ((attr = ippFindAttribute(response, "printer-state", IPP_TAG_ENUM)) != NULL) {
		if (attr->values[0].integer == IPP_PRINTER_STOPPED)
			status->status = LPSTAT_STOPPED;
		else
			status->status = LPSTAT_OK;
	}

	if ((attr = ippFindAttribute(response, "printer-state-message",
	                             IPP_TAG_TEXT)) != NULL)
		fstrcpy(status->message, attr->values[0].string.text);

	if ((attr = ippFindAttribute(response, "printer-current-time",
	                             IPP_TAG_DATE)) != NULL)
		printer_current_time = ippDateToTime(attr->values[0].date);

	if ((attr = ippFindAttribute(response, "printer-up-time",
	                             IPP_TAG_INTEGER)) != NULL)
		printer_up_time = attr->values[0].integer;

	ippDelete(response);
	response = NULL;

       /*
	* Build an IPP_GET_JOBS request, which requires the following
	* attributes:
	*
	*    attributes-charset
	*    attributes-natural-language
	*    requested-attributes
	*    printer-uri
	*/

	request = ippNew();

	request->request.op.operation_id = IPP_GET_JOBS;
	request->request.op.request_id   = 3;

	ippAddString(request, IPP_TAG_OPERATION, IPP_TAG_CHARSET,
	             "attributes-charset", NULL, "utf-8");

	ippAddString(request, IPP_TAG_OPERATION, IPP_TAG_LANGUAGE,
	             "attributes-natural-language", NULL, language->language);

	ippAddString(request, IPP_TAG_OPERATION, IPP_TAG_URI,
	             "printer-uri", NULL, uri);

	ippAddStrings(request, IPP_TAG_OPERATION, IPP_TAG_KEYWORD,
	              "requested-attributes",
	              (sizeof(jattrs) / sizeof(jattrs[0])),
	              NULL, jattrs);

       /*
	* Do the request and get back a response...
	*/

	slprintf(httpPath, sizeof(httpPath) - 1, "/ipp/%s", printername);

	if ((response = cupsDoRequest(http, request, httpPath)) == NULL) {
		DEBUG(0,("Unable to get jobs for %s - %s\n", uri,
			 ippErrorString(cupsLastError())));
		goto out;
	}

	if (response->request.status.status_code >= IPP_OK_CONFLICT) {
		DEBUG(0,("Unable to get jobs for %s - %s\n", uri,
			 ippErrorString(response->request.status.status_code)));
		goto out;
	}

       /*
	* Process the jobs...
	*/

	qcount = 0;
	qalloc = 0;
	queue  = NULL;

	for (attr = response->attrs; attr != NULL; attr = attr->next) {
	       /*
		* Skip leading attributes until we hit a job...
		*/

		while (attr != NULL && attr->group_tag != IPP_TAG_JOB)
			attr = attr->next;

		if (attr == NULL)
			break;

	       /*
		* Allocate memory as needed...
		*/
		if (qcount >= qalloc) {
			qalloc += 16;

			queue = SMB_REALLOC_ARRAY(queue, print_queue_struct, qalloc);

			if (queue == NULL) {
				DEBUG(0,("iprint_queue_get: Not enough memory!"));
				qcount = 0;
				goto out;
			}
		}

		temp = queue + qcount;
		memset(temp, 0, sizeof(print_queue_struct));

	       /*
		* Pull the needed attributes from this job...
		*/

		job_id       = 0;
		job_priority = 50;
		job_status   = IPP_JOB_PENDING;
		job_time     = 0;
		job_k_octets = 0;
		user_name    = NULL;
		job_name     = NULL;

		while (attr != NULL && attr->group_tag == IPP_TAG_JOB) {
			if (attr->name == NULL) {
				attr = attr->next;
				break;
			}

			if (strcmp(attr->name, "job-id") == 0 &&
			    attr->value_tag == IPP_TAG_INTEGER)
				job_id = attr->values[0].integer;

			if (strcmp(attr->name, "job-k-octets") == 0 &&
			    attr->value_tag == IPP_TAG_INTEGER)
				job_k_octets = attr->values[0].integer;

			if (strcmp(attr->name, "job-priority") == 0 &&
			    attr->value_tag == IPP_TAG_INTEGER)
				job_priority = attr->values[0].integer;

			if (strcmp(attr->name, "job-state") == 0 &&
			    attr->value_tag == IPP_TAG_ENUM)
				job_status = (ipp_jstate_t)(attr->values[0].integer);

			if (strcmp(attr->name, "time-at-creation") == 0 &&
			    attr->value_tag == IPP_TAG_INTEGER)
			{
			       /*
				* If jobs times are in Unix time, the accuracy of the job
				* start time depends upon the iPrint server's time being
				* set correctly.  Otherwise, the accuracy depends upon
				* the Samba server's time being set correctly
				*/

				if (jobUseUnixTime)
					job_time = attr->values[0].integer; 
				else
					job_time = time(NULL) - printer_up_time + attr->values[0].integer;
			}

			if (strcmp(attr->name, "job-name") == 0 &&
			    (attr->value_tag == IPP_TAG_NAMELANG ||
			     attr->value_tag == IPP_TAG_NAME))
				job_name = attr->values[0].string.text;

			if (strcmp(attr->name, "job-originating-user-name") == 0 &&
			    (attr->value_tag == IPP_TAG_NAMELANG ||
			     attr->value_tag == IPP_TAG_NAME))
				user_name = attr->values[0].string.text;

			attr = attr->next;
		}

	       /*
		* See if we have everything needed...
		*/

		if (user_name == NULL || job_name == NULL || job_id == 0) {
			if (attr == NULL)
				break;
			else
				continue;
		}

		temp->job      = job_id;
		temp->size     = job_k_octets * 1024;
		temp->status   = job_status == IPP_JOB_PENDING ? LPQ_QUEUED :
		                 job_status == IPP_JOB_STOPPED ? LPQ_PAUSED :
                                 job_status == IPP_JOB_HELD ? LPQ_PAUSED :
                                 LPQ_PRINTING;
		temp->priority = job_priority;
		temp->time     = job_time;
		strncpy(temp->fs_user, user_name, sizeof(temp->fs_user) - 1);
		strncpy(temp->fs_file, job_name, sizeof(temp->fs_file) - 1);

		qcount ++;

		if (attr == NULL)
			break;
	}

       /*
	* Return the job queue...
	*/

	*q = queue;

 out:
	if (response)
		ippDelete(response);

	if (language)
		cupsLangFree(language);

	if (http)
		httpClose(http);

	return qcount;
}


/*
 * 'iprint_queue_pause()' - Pause a print queue.
 */

static int iprint_queue_pause(int snum)
{
	return(-1); /* Not supported without credentials */
}


/*
 * 'iprint_queue_resume()' - Restart a print queue.
 */

static int iprint_queue_resume(int snum)
{
	return(-1); /* Not supported without credentials */
}

/*******************************************************************
 * iPrint printing interface definitions...
 ******************************************************************/

struct printif	iprint_printif =
{
	PRINT_IPRINT,
	iprint_queue_get,
	iprint_queue_pause,
	iprint_queue_resume,
	iprint_job_delete,
	iprint_job_pause,
	iprint_job_resume,
	iprint_job_submit,
};

#else
 /* this keeps fussy compilers happy */
 void print_iprint_dummy(void) {}
#endif /* HAVE_IPRINT */
