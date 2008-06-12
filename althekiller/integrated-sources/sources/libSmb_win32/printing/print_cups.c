/*
 * Support code for the Common UNIX Printing System ("CUPS")
 *
 * Copyright 1999-2003 by Michael R Sweet.
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

#ifdef HAVE_CUPS
#include <cups/cups.h>
#include <cups/language.h>


/*
 * 'cups_passwd_cb()' - The CUPS password callback...
 */

static const char *				/* O - Password or NULL */
cups_passwd_cb(const char *prompt)	/* I - Prompt */
{
	/*
	 * Always return NULL to indicate that no password is available...
	 */

	return (NULL);
}

static const char *cups_server(void)
{
	if ((lp_cups_server() != NULL) && (strlen(lp_cups_server()) > 0)) {
		DEBUG(10, ("cups server explicitly set to %s\n",
			   lp_cups_server()));
		return lp_cups_server();
	}

	DEBUG(10, ("cups server left to default %s\n", cupsServer()));
	return cupsServer();
}

BOOL cups_cache_reload(void)
{
	http_t		*http = NULL;		/* HTTP connection to server */
	ipp_t		*request = NULL,	/* IPP Request */
			*response = NULL;	/* IPP Response */
	ipp_attribute_t	*attr;		/* Current attribute */
	cups_lang_t	*language = NULL;	/* Default language */
	char		*name,		/* printer-name attribute */
			*info;		/* printer-info attribute */
	static const char *requested[] =/* Requested attributes */
			{
			  "printer-name",
			  "printer-info"
			};       
	BOOL ret = False;

	DEBUG(5, ("reloading cups printcap cache\n"));

       /*
        * Make sure we don't ask for passwords...
	*/

        cupsSetPasswordCB(cups_passwd_cb);

       /*
	* Try to connect to the server...
	*/

	if ((http = httpConnect(cups_server(), ippPort())) == NULL) {
		DEBUG(0,("Unable to connect to CUPS server %s - %s\n", 
			 cups_server(), strerror(errno)));
		goto out;
	}

       /*
	* Build a CUPS_GET_PRINTERS request, which requires the following
	* attributes:
	*
	*    attributes-charset
	*    attributes-natural-language
	*    requested-attributes
	*/

	request = ippNew();

	request->request.op.operation_id = CUPS_GET_PRINTERS;
	request->request.op.request_id   = 1;

	language = cupsLangDefault();

	ippAddString(request, IPP_TAG_OPERATION, IPP_TAG_CHARSET,
                     "attributes-charset", NULL, cupsLangEncoding(language));

	ippAddString(request, IPP_TAG_OPERATION, IPP_TAG_LANGUAGE,
                     "attributes-natural-language", NULL, language->language);

        ippAddStrings(request, IPP_TAG_OPERATION, IPP_TAG_NAME,
	              "requested-attributes",
		      (sizeof(requested) / sizeof(requested[0])),
		      NULL, requested);

       /*
	* Do the request and get back a response...
	*/

	if ((response = cupsDoRequest(http, request, "/")) == NULL) {
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

		name       = NULL;
		info       = NULL;

		while (attr != NULL && attr->group_tag == IPP_TAG_PRINTER) {
        		if (strcmp(attr->name, "printer-name") == 0 &&
			    attr->value_tag == IPP_TAG_NAME)
				name = attr->values[0].string.text;

        		if (strcmp(attr->name, "printer-info") == 0 &&
			    attr->value_tag == IPP_TAG_TEXT)
				info = attr->values[0].string.text;

        		attr = attr->next;
		}

	       /*
		* See if we have everything needed...
		*/

		if (name == NULL)
			break;

		if (!pcap_cache_add(name, info)) {
			goto out;
		}
	}

	ippDelete(response);
	response = NULL;

       /*
	* Build a CUPS_GET_CLASSES request, which requires the following
	* attributes:
	*
	*    attributes-charset
	*    attributes-natural-language
	*    requested-attributes
	*/

	request = ippNew();

	request->request.op.operation_id = CUPS_GET_CLASSES;
	request->request.op.request_id   = 1;

	ippAddString(request, IPP_TAG_OPERATION, IPP_TAG_CHARSET,
                     "attributes-charset", NULL, cupsLangEncoding(language));

	ippAddString(request, IPP_TAG_OPERATION, IPP_TAG_LANGUAGE,
                     "attributes-natural-language", NULL, language->language);

        ippAddStrings(request, IPP_TAG_OPERATION, IPP_TAG_NAME,
	              "requested-attributes",
		      (sizeof(requested) / sizeof(requested[0])),
		      NULL, requested);

       /*
	* Do the request and get back a response...
	*/

	if ((response = cupsDoRequest(http, request, "/")) == NULL) {
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

		name       = NULL;
		info       = NULL;

		while (attr != NULL && attr->group_tag == IPP_TAG_PRINTER) {
        		if (strcmp(attr->name, "printer-name") == 0 &&
			    attr->value_tag == IPP_TAG_NAME)
				name = attr->values[0].string.text;

        		if (strcmp(attr->name, "printer-info") == 0 &&
			    attr->value_tag == IPP_TAG_TEXT)
				info = attr->values[0].string.text;

        		attr = attr->next;
		}

	       /*
		* See if we have everything needed...
		*/

		if (name == NULL)
			break;

		if (!pcap_cache_add(name, info)) {
			goto out;
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
 * 'cups_job_delete()' - Delete a job.
 */

static int cups_job_delete(const char *sharename, const char *lprm_command, struct printjob *pjob)
{
	int		ret = 1;		/* Return value */
	http_t		*http = NULL;		/* HTTP connection to server */
	ipp_t		*request = NULL,	/* IPP Request */
			*response = NULL;	/* IPP Response */
	cups_lang_t	*language = NULL;	/* Default language */
	char		uri[HTTP_MAX_URI]; /* printer-uri attribute */


	DEBUG(5,("cups_job_delete(%s, %p (%d))\n", sharename, pjob, pjob->sysjob));

       /*
        * Make sure we don't ask for passwords...
	*/

        cupsSetPasswordCB(cups_passwd_cb);

       /*
	* Try to connect to the server...
	*/

	if ((http = httpConnect(cups_server(), ippPort())) == NULL) {
		DEBUG(0,("Unable to connect to CUPS server %s - %s\n", 
			 cups_server(), strerror(errno)));
		goto out;
	}

       /*
	* Build an IPP_CANCEL_JOB request, which requires the following
	* attributes:
	*
	*    attributes-charset
	*    attributes-natural-language
	*    job-uri
	*    requesting-user-name
	*/

	request = ippNew();

	request->request.op.operation_id = IPP_CANCEL_JOB;
	request->request.op.request_id   = 1;

	language = cupsLangDefault();

	ippAddString(request, IPP_TAG_OPERATION, IPP_TAG_CHARSET,
        	     "attributes-charset", NULL, cupsLangEncoding(language));

	ippAddString(request, IPP_TAG_OPERATION, IPP_TAG_LANGUAGE,
        	     "attributes-natural-language", NULL, language->language);

	slprintf(uri, sizeof(uri) - 1, "ipp://localhost/jobs/%d", pjob->sysjob);

	ippAddString(request, IPP_TAG_OPERATION, IPP_TAG_URI, "job-uri", NULL, uri);

	ippAddString(request, IPP_TAG_OPERATION, IPP_TAG_NAME, "requesting-user-name",
        	     NULL, pjob->user);

       /*
	* Do the request and get back a response...
	*/

	if ((response = cupsDoRequest(http, request, "/jobs")) != NULL) {
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
 * 'cups_job_pause()' - Pause a job.
 */

static int cups_job_pause(int snum, struct printjob *pjob)
{
	int		ret = 1;		/* Return value */
	http_t		*http = NULL;		/* HTTP connection to server */
	ipp_t		*request = NULL,	/* IPP Request */
			*response = NULL;	/* IPP Response */
	cups_lang_t	*language = NULL;	/* Default language */
	char		uri[HTTP_MAX_URI]; /* printer-uri attribute */


	DEBUG(5,("cups_job_pause(%d, %p (%d))\n", snum, pjob, pjob->sysjob));

       /*
        * Make sure we don't ask for passwords...
	*/

        cupsSetPasswordCB(cups_passwd_cb);

       /*
	* Try to connect to the server...
	*/

	if ((http = httpConnect(cups_server(), ippPort())) == NULL) {
		DEBUG(0,("Unable to connect to CUPS server %s - %s\n", 
			 cups_server(), strerror(errno)));
		goto out;
	}

       /*
	* Build an IPP_HOLD_JOB request, which requires the following
	* attributes:
	*
	*    attributes-charset
	*    attributes-natural-language
	*    job-uri
	*    requesting-user-name
	*/

	request = ippNew();

	request->request.op.operation_id = IPP_HOLD_JOB;
	request->request.op.request_id   = 1;

	language = cupsLangDefault();

	ippAddString(request, IPP_TAG_OPERATION, IPP_TAG_CHARSET,
        	     "attributes-charset", NULL, cupsLangEncoding(language));

	ippAddString(request, IPP_TAG_OPERATION, IPP_TAG_LANGUAGE,
        	     "attributes-natural-language", NULL, language->language);

	slprintf(uri, sizeof(uri) - 1, "ipp://localhost/jobs/%d", pjob->sysjob);

	ippAddString(request, IPP_TAG_OPERATION, IPP_TAG_URI, "job-uri", NULL, uri);

	ippAddString(request, IPP_TAG_OPERATION, IPP_TAG_NAME, "requesting-user-name",
        	     NULL, pjob->user);

       /*
	* Do the request and get back a response...
	*/

	if ((response = cupsDoRequest(http, request, "/jobs")) != NULL) {
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
 * 'cups_job_resume()' - Resume a paused job.
 */

static int cups_job_resume(int snum, struct printjob *pjob)
{
	int		ret = 1;		/* Return value */
	http_t		*http = NULL;		/* HTTP connection to server */
	ipp_t		*request = NULL,	/* IPP Request */
			*response = NULL;	/* IPP Response */
	cups_lang_t	*language = NULL;	/* Default language */
	char		uri[HTTP_MAX_URI]; /* printer-uri attribute */


	DEBUG(5,("cups_job_resume(%d, %p (%d))\n", snum, pjob, pjob->sysjob));

       /*
        * Make sure we don't ask for passwords...
	*/

        cupsSetPasswordCB(cups_passwd_cb);

       /*
	* Try to connect to the server...
	*/

	if ((http = httpConnect(cups_server(), ippPort())) == NULL) {
		DEBUG(0,("Unable to connect to CUPS server %s - %s\n", 
			 cups_server(), strerror(errno)));
		goto out;
	}

       /*
	* Build an IPP_RELEASE_JOB request, which requires the following
	* attributes:
	*
	*    attributes-charset
	*    attributes-natural-language
	*    job-uri
	*    requesting-user-name
	*/

	request = ippNew();

	request->request.op.operation_id = IPP_RELEASE_JOB;
	request->request.op.request_id   = 1;

	language = cupsLangDefault();

	ippAddString(request, IPP_TAG_OPERATION, IPP_TAG_CHARSET,
        	     "attributes-charset", NULL, cupsLangEncoding(language));

	ippAddString(request, IPP_TAG_OPERATION, IPP_TAG_LANGUAGE,
        	     "attributes-natural-language", NULL, language->language);

	slprintf(uri, sizeof(uri) - 1, "ipp://localhost/jobs/%d", pjob->sysjob);

	ippAddString(request, IPP_TAG_OPERATION, IPP_TAG_URI, "job-uri", NULL, uri);

	ippAddString(request, IPP_TAG_OPERATION, IPP_TAG_NAME, "requesting-user-name",
        	     NULL, pjob->user);

       /*
	* Do the request and get back a response...
	*/

	if ((response = cupsDoRequest(http, request, "/jobs")) != NULL) {
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
 * 'cups_job_submit()' - Submit a job for printing.
 */

static int cups_job_submit(int snum, struct printjob *pjob)
{
	int		ret = 1;		/* Return value */
	http_t		*http = NULL;		/* HTTP connection to server */
	ipp_t		*request = NULL,	/* IPP Request */
			*response = NULL;	/* IPP Response */
	cups_lang_t	*language = NULL;	/* Default language */
	char		uri[HTTP_MAX_URI]; /* printer-uri attribute */
	char 		*clientname = NULL; 	/* hostname of client for job-originating-host attribute */
	pstring		new_jobname;
	int		num_options = 0; 
	cups_option_t 	*options = NULL;

	DEBUG(5,("cups_job_submit(%d, %p (%d))\n", snum, pjob, pjob->sysjob));

       /*
        * Make sure we don't ask for passwords...
	*/

        cupsSetPasswordCB(cups_passwd_cb);

       /*
	* Try to connect to the server...
	*/

	if ((http = httpConnect(cups_server(), ippPort())) == NULL) {
		DEBUG(0,("Unable to connect to CUPS server %s - %s\n", 
			 cups_server(), strerror(errno)));
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
        	     "attributes-charset", NULL, cupsLangEncoding(language));

	ippAddString(request, IPP_TAG_OPERATION, IPP_TAG_LANGUAGE,
        	     "attributes-natural-language", NULL, language->language);

	slprintf(uri, sizeof(uri) - 1, "ipp://localhost/printers/%s",
	         PRINTERNAME(snum));

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

        pstr_sprintf(new_jobname,"%s%.8u %s", PRINT_SPOOL_PREFIX, 
		(unsigned int)pjob->smbjob, pjob->jobname);

	ippAddString(request, IPP_TAG_OPERATION, IPP_TAG_NAME, "job-name", NULL,
        	     new_jobname);

	/* 
	 * add any options defined in smb.conf 
	 */

	num_options = 0;
	options     = NULL;
	num_options = cupsParseOptions(lp_cups_options(snum), num_options, &options);

	if ( num_options )
		cupsEncodeOptions(request, num_options, options); 

       /*
	* Do the request and get back a response...
	*/

	slprintf(uri, sizeof(uri) - 1, "/printers/%s", PRINTERNAME(snum));

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
 * 'cups_queue_get()' - Get all the jobs in the print queue.
 */

static int cups_queue_get(const char *sharename,
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
	int		qcount = 0,		/* Number of active queue entries */
			qalloc = 0;		/* Number of queue entries allocated */
	print_queue_struct *queue = NULL,	/* Queue entries */
			*temp;		/* Temporary pointer for queue */
	const char	*user_name,	/* job-originating-user-name attribute */
			*job_name;	/* job-name attribute */
	int		job_id;		/* job-id attribute */
	int		job_k_octets;	/* job-k-octets attribute */
	time_t		job_time;	/* time-at-creation attribute */
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
			  "printer-state-message"
			};

	*q = NULL;

	/* HACK ALERT!!!  The problem with support the 'printer name' 
	   option is that we key the tdb off the sharename.  So we will 
	   overload the lpq_command string to pass in the printername 
	   (which is basically what we do for non-cups printers ... using 
	   the lpq_command to get the queue listing). */

	fstrcpy( printername, lpq_command );

	DEBUG(5,("cups_queue_get(%s, %p, %p)\n", printername, q, status));

       /*
        * Make sure we don't ask for passwords...
	*/

        cupsSetPasswordCB(cups_passwd_cb);

       /*
	* Try to connect to the server...
	*/

	if ((http = httpConnect(cups_server(), ippPort())) == NULL) {
		DEBUG(0,("Unable to connect to CUPS server %s - %s\n", 
			 cups_server(), strerror(errno)));
		goto out;
	}

       /*
        * Generate the printer URI...
	*/

	slprintf(uri, sizeof(uri) - 1, "ipp://localhost/printers/%s", printername);

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
	request->request.op.request_id   = 1;

	language = cupsLangDefault();

	ippAddString(request, IPP_TAG_OPERATION, IPP_TAG_CHARSET,
                     "attributes-charset", NULL, cupsLangEncoding(language));

	ippAddString(request, IPP_TAG_OPERATION, IPP_TAG_LANGUAGE,
                     "attributes-natural-language", NULL, language->language);

        ippAddStrings(request, IPP_TAG_OPERATION, IPP_TAG_NAME,
	              "requested-attributes",
		      (sizeof(jattrs) / sizeof(jattrs[0])),
		      NULL, jattrs);

	ippAddString(request, IPP_TAG_OPERATION, IPP_TAG_URI,
                     "printer-uri", NULL, uri);

       /*
	* Do the request and get back a response...
	*/

	if ((response = cupsDoRequest(http, request, "/")) == NULL) {
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
				DEBUG(0,("cups_queue_get: Not enough memory!"));
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
				job_time = attr->values[0].integer;

        		if (strcmp(attr->name, "job-name") == 0 &&
			    attr->value_tag == IPP_TAG_NAME)
				job_name = attr->values[0].string.text;

        		if (strcmp(attr->name, "job-originating-user-name") == 0 &&
			    attr->value_tag == IPP_TAG_NAME)
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

	ippDelete(response);
	response = NULL;

       /*
	* Build an IPP_GET_PRINTER_ATTRIBUTES request, which requires the
	* following attributes:
	*
	*    attributes-charset
	*    attributes-natural-language
	*    requested-attributes
	*    printer-uri
	*/

	request = ippNew();

	request->request.op.operation_id = IPP_GET_PRINTER_ATTRIBUTES;
	request->request.op.request_id   = 1;

	ippAddString(request, IPP_TAG_OPERATION, IPP_TAG_CHARSET,
                     "attributes-charset", NULL, cupsLangEncoding(language));

	ippAddString(request, IPP_TAG_OPERATION, IPP_TAG_LANGUAGE,
                     "attributes-natural-language", NULL, language->language);

        ippAddStrings(request, IPP_TAG_OPERATION, IPP_TAG_NAME,
	              "requested-attributes",
		      (sizeof(pattrs) / sizeof(pattrs[0])),
		      NULL, pattrs);

	ippAddString(request, IPP_TAG_OPERATION, IPP_TAG_URI,
                     "printer-uri", NULL, uri);

       /*
	* Do the request and get back a response...
	*/

	if ((response = cupsDoRequest(http, request, "/")) == NULL) {
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
 * 'cups_queue_pause()' - Pause a print queue.
 */

static int cups_queue_pause(int snum)
{
	extern userdom_struct current_user_info;
	int		ret = 1;		/* Return value */
	http_t		*http = NULL;		/* HTTP connection to server */
	ipp_t		*request = NULL,	/* IPP Request */
			*response = NULL;	/* IPP Response */
	cups_lang_t	*language = NULL;	/* Default language */
	char		uri[HTTP_MAX_URI]; /* printer-uri attribute */


	DEBUG(5,("cups_queue_pause(%d)\n", snum));

	/*
	 * Make sure we don't ask for passwords...
	 */

        cupsSetPasswordCB(cups_passwd_cb);

	/*
	 * Try to connect to the server...
	 */

	if ((http = httpConnect(cups_server(), ippPort())) == NULL) {
		DEBUG(0,("Unable to connect to CUPS server %s - %s\n", 
			 cups_server(), strerror(errno)));
		goto out;
	}

	/*
	 * Build an IPP_PAUSE_PRINTER request, which requires the following
	 * attributes:
	 *
	 *    attributes-charset
	 *    attributes-natural-language
	 *    printer-uri
	 *    requesting-user-name
	 */

	request = ippNew();

	request->request.op.operation_id = IPP_PAUSE_PRINTER;
	request->request.op.request_id   = 1;

	language = cupsLangDefault();

	ippAddString(request, IPP_TAG_OPERATION, IPP_TAG_CHARSET,
        	     "attributes-charset", NULL, cupsLangEncoding(language));

	ippAddString(request, IPP_TAG_OPERATION, IPP_TAG_LANGUAGE,
        	     "attributes-natural-language", NULL, language->language);

	slprintf(uri, sizeof(uri) - 1, "ipp://localhost/printers/%s",
	         PRINTERNAME(snum));

	ippAddString(request, IPP_TAG_OPERATION, IPP_TAG_URI, "printer-uri", NULL, uri);

	ippAddString(request, IPP_TAG_OPERATION, IPP_TAG_NAME, "requesting-user-name",
        	     NULL, current_user_info.unix_name);

       /*
	* Do the request and get back a response...
	*/

	if ((response = cupsDoRequest(http, request, "/admin/")) != NULL) {
		if (response->request.status.status_code >= IPP_OK_CONFLICT) {
			DEBUG(0,("Unable to pause printer %s - %s\n", PRINTERNAME(snum),
				ippErrorString(cupsLastError())));
		} else {
			ret = 0;
		}
	} else {
		DEBUG(0,("Unable to pause printer %s - %s\n", PRINTERNAME(snum),
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
 * 'cups_queue_resume()' - Restart a print queue.
 */

static int cups_queue_resume(int snum)
{
	extern userdom_struct current_user_info;
	int		ret = 1;		/* Return value */
	http_t		*http = NULL;		/* HTTP connection to server */
	ipp_t		*request = NULL,	/* IPP Request */
			*response = NULL;	/* IPP Response */
	cups_lang_t	*language = NULL;	/* Default language */
	char		uri[HTTP_MAX_URI]; /* printer-uri attribute */


	DEBUG(5,("cups_queue_resume(%d)\n", snum));

       /*
        * Make sure we don't ask for passwords...
	*/

        cupsSetPasswordCB(cups_passwd_cb);

       /*
	* Try to connect to the server...
	*/

	if ((http = httpConnect(cups_server(), ippPort())) == NULL) {
		DEBUG(0,("Unable to connect to CUPS server %s - %s\n", 
			 cups_server(), strerror(errno)));
		goto out;
	}

       /*
	* Build an IPP_RESUME_PRINTER request, which requires the following
	* attributes:
	*
	*    attributes-charset
	*    attributes-natural-language
	*    printer-uri
	*    requesting-user-name
	*/

	request = ippNew();

	request->request.op.operation_id = IPP_RESUME_PRINTER;
	request->request.op.request_id   = 1;

	language = cupsLangDefault();

	ippAddString(request, IPP_TAG_OPERATION, IPP_TAG_CHARSET,
        	     "attributes-charset", NULL, cupsLangEncoding(language));

	ippAddString(request, IPP_TAG_OPERATION, IPP_TAG_LANGUAGE,
        	     "attributes-natural-language", NULL, language->language);

	slprintf(uri, sizeof(uri) - 1, "ipp://localhost/printers/%s",
	         PRINTERNAME(snum));

	ippAddString(request, IPP_TAG_OPERATION, IPP_TAG_URI, "printer-uri", NULL, uri);

	ippAddString(request, IPP_TAG_OPERATION, IPP_TAG_NAME, "requesting-user-name",
        	     NULL, current_user_info.unix_name);

       /*
	* Do the request and get back a response...
	*/

	if ((response = cupsDoRequest(http, request, "/admin/")) != NULL) {
		if (response->request.status.status_code >= IPP_OK_CONFLICT) {
			DEBUG(0,("Unable to resume printer %s - %s\n", PRINTERNAME(snum),
				ippErrorString(cupsLastError())));
		} else {
			ret = 0;
		}
	} else {
		DEBUG(0,("Unable to resume printer %s - %s\n", PRINTERNAME(snum),
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

/*******************************************************************
 * CUPS printing interface definitions...
 ******************************************************************/

struct printif	cups_printif =
{
	PRINT_CUPS,
	cups_queue_get,
	cups_queue_pause,
	cups_queue_resume,
	cups_job_delete,
	cups_job_pause,
	cups_job_resume,
	cups_job_submit,
};

#else
 /* this keeps fussy compilers happy */
 void print_cups_dummy(void) {}
#endif /* HAVE_CUPS */
