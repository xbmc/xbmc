/* rip_manager.c
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
/*
 * This is the top level for the streamripper library. The it handles the 
 * connecting the rippers (ripstream, ripshout, riplive) to the sockets 
 * (socklib), the output stuff (filelib) and the relay server thing (relaylib)
 * and also all the "features" like auto connecting, directory names.
 *
 * Lots of stuff, and in retrospect it should probably be more splite up. My 
 * excuss is that this grew from a bunch of unit tests for the various 
 * "sub-systems". In the end i just called it "rip_mananger".
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <errno.h>
#include <sys/types.h>
#if !defined (WIN32)
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif

#include "filelib.h"
#include "socklib.h"
#include "http.h"
#include "mchar.h"
#include "findsep.h"
#include "relaylib.h"
#include "rip_manager.h"
#include "ripstream.h"
#include "threadlib.h"
#include "debug.h"
#include "compat.h"
#include "parse.h"

/******************************************************************************
 * Private functions
 *****************************************************************************/
static void			ripthread(void *bla);
static error_code		start_relay(int content_type);
static void			post_status(int status);
static error_code start_ripping (RIP_MANAGER_OPTIONS* rmo);
static void			destroy_subsystems();


/******************************************************************************
 * Private Vars
 *****************************************************************************/
static SR_HTTP_HEADER		m_http_info;
static HSOCKET			m_sock;
#if defined (commentout)
static void			(*m_destroy_func)();
#endif
static RIP_MANAGER_INFO		m_ripinfo;		// used for UPDATE callback messages
//static RIP_MANAGER_OPTIONS	m_options;		// local copy of the options passed to rip_manager_start()
static THREAD_HANDLE		m_hthread;		// rip thread handle
static void			(*m_status_callback)(int message, void *data);
static BOOL			m_ripping = FALSE;
static u_long			m_bytes_ripped;
static BOOL			m_write_data = TRUE;	// Should we actually write to a file or not
static HSEM			m_started_sem;	// to prevent deadlocks when ripping is stopped before its started.

static char* overwrite_opt_strings[] = {
    "",		// UNKNOWN
    "always",
    "never",
    "larger"
};

/*
 * Needs english type messages, just copy pasted for now
 */
static char m_error_str[NUM_ERROR_CODES][MAX_ERROR_STR];
#define SET_ERR_STR(str, code)	strncpy(m_error_str[code], str, MAX_ERROR_STR);

void init_error_strings()
{
    SET_ERR_STR("SR_SUCCESS",				0x00);
    SET_ERR_STR("SR_ERROR_CANT_FIND_TRACK_SEPERATION",	0x01);
    SET_ERR_STR("SR_ERROR_DECODE_FAILURE",			0x02);
    SET_ERR_STR("SR_ERROR_INVALID_URL",			0x03);
    SET_ERR_STR("SR_ERROR_WIN32_INIT_FAILURE",		0x04);
    SET_ERR_STR("Could not connect to the stream. Try checking that the stream is up\n"
		"and that your proxy settings are correct.",0x05);
    SET_ERR_STR("SR_ERROR_CANT_RESOLVE_HOSTNAME",		0x06);
    SET_ERR_STR("SR_ERROR_RECV_FAILED",				0x07);
    SET_ERR_STR("SR_ERROR_SEND_FAILED",				0x08);
    SET_ERR_STR("SR_ERROR_PARSE_FAILURE",			0x09);
    SET_ERR_STR("SR_ERROR_NO_RESPOSE_HEADER",			0x0a);
    SET_ERR_STR("Server returned an unknown error code",0x0b);
    SET_ERR_STR("SR_ERROR_NO_META_INTERVAL",			0x0c);
    SET_ERR_STR("SR_ERROR_INVALID_PARAM",			0x0d);
    SET_ERR_STR("SR_ERROR_NO_HTTP_HEADER",			0x0e);
    SET_ERR_STR("SR_ERROR_CANT_GET_LIVE365_ID",			0x0f);
    SET_ERR_STR("SR_ERROR_CANT_ALLOC_MEMORY",			0x10);
    SET_ERR_STR("SR_ERROR_CANT_FIND_IP_PORT",			0x11);
    SET_ERR_STR("SR_ERROR_CANT_FIND_MEMBERNAME",		0x12);
    SET_ERR_STR("SR_ERROR_CANT_FIND_TRACK_NAME",		0x13);
    SET_ERR_STR("SR_ERROR_NULL_MEMBER_NAME",			0x14);
    SET_ERR_STR("SR_ERROR_CANT_FIND_TIME_TAG",			0x15);
    SET_ERR_STR("SR_ERROR_BUFFER_EMPTY",			0x16);
    SET_ERR_STR("SR_ERROR_BUFFER_FULL",				0x17);
    SET_ERR_STR("SR_ERROR_CANT_INIT_XAUDIO",			0x18);
    SET_ERR_STR("SR_ERROR_BUFFER_TOO_SMALL",			0x19);
    SET_ERR_STR("SR_ERROR_CANT_CREATE_THREAD",			0x1A);
    SET_ERR_STR("SR_ERROR_CANT_FIND_MPEG_HEADER",		0x1B);
    SET_ERR_STR("SR_ERROR_INVALID_METADATA",			0x1C);
    SET_ERR_STR("SR_ERROR_NO_TRACK_INFO",			0x1D);
    SET_ERR_STR("SR_EEROR_CANT_FIND_SUBSTR",			0x1E);
    SET_ERR_STR("SR_ERROR_CANT_BIND_ON_PORT",			0x1F);
    SET_ERR_STR("SR_ERROR_HOST_NOT_CONNECTED",			0x20);
    SET_ERR_STR("HTTP:404 Not Found",				0x21);
    SET_ERR_STR("HTTP:401 Unauthorized",			0x22);
    SET_ERR_STR("HTTP:502 Bad Gateway",				0x23);	// Connection Refused
    SET_ERR_STR("SR_ERROR_CANT_CREATE_FILE",			0x24);
    SET_ERR_STR("SR_ERROR_CANT_WRITE_TO_FILE",			0x25);
    SET_ERR_STR("SR_ERROR_CANT_CREATE_DIR",			0x26);
    SET_ERR_STR("HTTP:400 Bad Request ",			0x27);	// Server Full
    SET_ERR_STR("SR_ERROR_CANT_SET_SOCKET_OPTIONS",		0x28);
    SET_ERR_STR("SR_ERROR_SOCK_BASE",				0x29);
    SET_ERR_STR("SR_ERROR_INVALID_DIRECTORY",			0x2a);
    SET_ERR_STR("SR_ERROR_FAILED_TO_MOVE_FILE",			0x2b);
    SET_ERR_STR("SR_ERROR_CANT_LOAD_MPGLIB",			0x2c);
    SET_ERR_STR("SR_ERROR_CANT_INIT_MPGLIB",			0x2d);
    SET_ERR_STR("SR_ERROR_CANT_UNLOAD_MPGLIB",			0x2e);
    SET_ERR_STR("SR_ERROR_PCM_BUFFER_TO_SMALL",			0x2f);
    SET_ERR_STR("SR_ERROR_CANT_DECODE_MP3",			0x30);
    SET_ERR_STR("SR_ERROR_SOCKET_CLOSED",			0x31);
    SET_ERR_STR("Due to legal reasons Streamripper can no longer work with Live365(tm).\r\n"
		"See streamripper.sourceforge.net for more on this matter.", 0x32);
    SET_ERR_STR("The maximum number of bytes ripped has been reached", 0x33);
    SET_ERR_STR("SR_ERROR_CANT_WAIT_ON_THREAD",			0x34);
    SET_ERR_STR("SR_ERROR_CANT_CREATE_EVENT",			0x35);
    SET_ERR_STR("SR_ERROR_NOT_SHOUTCAST_STREAM",		0x36);
    SET_ERR_STR("HTTP:407 - Proxy Authentication Required",	0x37);
    SET_ERR_STR("HTTP:403 - Access Forbidden (try changing the UserAgent)", 0x38);
    SET_ERR_STR("The output directory length is too long",      0x39);
    SET_ERR_STR("SR_ERROR_PROGRAM_ERROR",                       0x3a);
    SET_ERR_STR("SR_ERROR_TIMEOUT",                             0x3b);
    SET_ERR_STR("SR_ERROR_SELECT_FAILED",                       0x3c);
    SET_ERR_STR("SR_ERROR_RESERVED_WINDOW_EMPTY",               0x3d);
    SET_ERR_STR("SR_ERROR_CANT_BIND_ON_INTERFACE",              0x3e);
    SET_ERR_STR("SR_ERROR_NO_OGG_PAGES_FOR_RELAY",              0x3f);
    SET_ERR_STR("SR_ERROR_CANT_PARSE_PLS",                      0x40);
    SET_ERR_STR("SR_ERROR_CANT_PARSE_M3U",                      0x41);
}

char*
rip_manager_get_error_str(error_code code)
{
    if (code > 0 || code < -NUM_ERROR_CODES)
        return NULL;
    return m_error_str[-code];
}

#if defined (commentout)
u_short
rip_mananger_get_relay_port()
{
    return rmo->relay_port;
}
#endif

void
post_error(int err)
{
    ERROR_INFO err_info;
    err_info._error_code = err;
    strcpy(err_info.error_str, m_error_str[abs(err)]);
    m_status_callback(RM_ERROR, &err_info);
}


void
post_status(int status)
{
    if (status != 0)
        m_ripinfo.status = status;
    m_status_callback(RM_UPDATE, &m_ripinfo);
}

void
compose_console_string (TRACK_INFO* ti)
{
    mchar console_string[SR_MAX_PATH];
    msnprintf (console_string, SR_MAX_PATH, m_S m_(" - ") m_S, 
	       ti->artist, ti->title);
    string_from_mstring (m_ripinfo.filename, SR_MAX_PATH, console_string,
			 CODESET_LOCALE);
}

/* 
 * This is called by ripstream when we get a new track. 
 * most logic is handled by filelib_start() so we just
 * make sure there are no bad characters in the name and 
 * update via the callback 
 */
error_code
rip_manager_start_track (TRACK_INFO* ti, int track_count)
{
    int ret;

#if defined (commmentout)
    /* GCS FIX -- here is where i would compose the incomplete filename */
    char* trackname = ti->raw_metadata;
    debug_printf("rip_manager_start_track: %s\n", trackname);
    if (m_write_data && (ret = filelib_start(trackname)) != SR_SUCCESS) {
        return ret;
    }
#endif

    m_write_data = ti->save_track;

    if (m_write_data && (ret = filelib_start (ti)) != SR_SUCCESS) {
        return ret;
    }

    m_ripinfo.filesize = 0;
    m_ripinfo.track_count = track_count;

    /* Compose the string for the console output */
    compose_console_string (ti);
    m_ripinfo.filename[SR_MAX_PATH-1] = '\0';
    m_status_callback(RM_NEW_TRACK, (void*) m_ripinfo.filename);
    post_status(0);

    return SR_SUCCESS;
}

/* Ok, the end_track()'s function is actually to move 
 * tracks out from the incomplete directory. It does 
 * get called, but only after the 2nd track is played. 
 * the first track is *never* complete.
 */
error_code
rip_manager_end_track (RIP_MANAGER_OPTIONS* rmo, TRACK_INFO* ti)
{
    mchar mfullpath[SR_MAX_PATH];
    char fullpath[SR_MAX_PATH];

    if (m_write_data) {
        filelib_end (ti, rmo->overwrite, 
		     GET_TRUNCATE_DUPS(rmo->flags),
		     mfullpath);
    }
    post_status(0);

    string_from_mstring (fullpath, SR_MAX_PATH, mfullpath, CODESET_FILESYS);
    m_status_callback (RM_TRACK_DONE, (void*)fullpath);

    return SR_SUCCESS;
}

error_code
rip_manager_put_data(char *buf, int size)
{
    if (m_write_data)
        filelib_write_track(buf, size);

    m_ripinfo.filesize += size;
    m_bytes_ripped += size;

    return SR_SUCCESS;
}

error_code
rip_manager_put_raw_data(char *buf, int size)
{
#if defined (commentout)
    relaylib_send(buf, size, 1, 0);
#endif
    filelib_write_show (buf, size);
    return SR_SUCCESS;
}


/* 
 * Fires up the relaylib stuff. Most of it is so relaylib 
 * knows what to call the stream which is what the 'construct_sc_repsonse()'
 * call is about.
 */
error_code
start_relay(int content_type)
{	
    int ret;

    if (!relaylib_isrunning())
	if ((ret = relaylib_start()) != SR_SUCCESS)
	    return ret;

    return SR_SUCCESS;
}

char *
client_relay_header_generate (int icy_meta_support)
{
    SR_HTTP_HEADER info = m_http_info;
    char temp_icyname[MAX_SERVER_LEN];
    int ret;
    
    char *headbuf;	
    sprintf(temp_icyname, "[%s] %s", "relay stream", info.icy_name);
    strcpy(info.icy_name, temp_icyname);
    
    headbuf = (char *) malloc(MAX_HEADER_LEN);
    ret = httplib_construct_sc_response(&m_http_info, headbuf, MAX_HEADER_LEN, icy_meta_support);
    if (ret != SR_SUCCESS) {
	headbuf[0] = 0;
    }
    
    return headbuf;
}

void
client_relay_header_release (char *ch)
{
    free (ch);
}

void
ripthread (void *input_arg)
{
    error_code ret;
    RIP_MANAGER_OPTIONS* rmo = (RIP_MANAGER_OPTIONS*) input_arg;
    debug_printf ("ripthread RMO_URL = %s\n", rmo->url);

    if ((ret = start_ripping(rmo)) != SR_SUCCESS) {
	debug_printf ("Ripthread did start_ripping()\n");
	threadlib_signal_sem (&m_started_sem);
	post_error(ret);
	goto DONE;
    }
    m_status_callback(RM_STARTED, (void *)NULL);
    post_status(RM_STATUS_BUFFERING);
    debug_printf ("Ripthread did initialization\n");
    threadlib_signal_sem(&m_started_sem);

    while (TRUE) {
        ret = ripstream_rip(rmo);

	/* If the user told us to stop, well, then we bail */
	if (!m_ripping)
	    break;

	/* 
	 * Added 8/4/2001 jc --
	 * for max number of MB ripped stuff
	 * At the time of writting this, i honestly 
	 * am not sure i understand what happens 
	 * when once trys to stop the ripping from the lib and
	 * the interface it's a weird combonation of threads
	 * wnd locks, etc.. all i know is that this appeared to work
	 */
        /* GCS Aug 23, 2003: m_bytes_ripped can still overflow */
	if (m_bytes_ripped/1000000 >= (rmo->maxMB_rip_size) &&
		GET_CHECK_MAX_BYTES (rmo->flags)) {
	    socklib_close (&m_sock);
	    destroy_subsystems ();
	    post_error (SR_ERROR_MAX_BYTES_RIPPED);
	    break;
	}

	/*
	 * If the error was anything but a connection failure
	 * then we need to report it and bail
	 */
	if (ret == SR_SUCCESS_BUFFERING) {
	    post_status(RM_STATUS_BUFFERING);
	}
	else if (ret == SR_ERROR_CANT_DECODE_MP3) {
	    post_error(ret);
	    continue;
	}
	else if ((ret == SR_ERROR_RECV_FAILED || 
		  ret == SR_ERROR_TIMEOUT || 
		  ret == SR_ERROR_NO_TRACK_INFO || 
		  ret == SR_ERROR_SELECT_FAILED) && 
		 GET_AUTO_RECONNECT (rmo->flags)) {
	    /* Try to reconnect, if thats what the user wants */
	    post_status(RM_STATUS_RECONNECTING);
	    while (m_ripping) {
		// Hopefully this solves a lingering bug 
		// with auto-reconnects failing to bind to the relay port
		// (a long with other unknown problems)
		// it seems that maybe we need two levels of shutdown and startup
		// functions. init_system, init_rip, shutdown_system and shutdown_rip
		// this would be a shutdown_rip, which only lacks the filelib and socklib
		// shutdowns
		// because filelib needs to keep track of it's file counters, and socklib.. umm..
		// not sure about. no reasdon to i imagine.
		socklib_close(&m_sock);
		if (m_ripinfo.ep) {
		    debug_printf ("Close external\n");
		    close_external (&m_ripinfo.ep);
		}
		relaylib_shutdown();
		filelib_shutdown();
		ripstream_destroy();
		ret = start_ripping (rmo);
		if (ret == SR_SUCCESS)
		    break;

		/*
		 * Should send a message that we are trying 
		 * .. or something
		 */
		Sleep(1000);
	    }
	}
	else if (ret != SR_SUCCESS) {
	    destroy_subsystems();
	    post_error(ret);
	    break;
	}

	if (m_ripinfo.filesize > 0)
	    post_status(RM_STATUS_RIPPING);
    }

    // We get here when there was either a fatal error
    // or we we're not auto-reconnecting and the stream just stopped
    // or when we have been told to stop, via the m_ripping flag
 DONE:
    m_status_callback(RM_DONE, &m_ripinfo);
    m_ripping = FALSE;
    debug_printf ("ripthread() exiting!\n");
}


void
rip_manager_stop()
{
    // Make sure this function isn't getting called twice
    if (!m_ripping) {
	debug_printf ("rip_manager_stop() called while not m_ripping\n");
	return;
    }
    
    // Make sure the ripping started before we try to stop
    debug_printf ("Waiting for m_started_sem...\n");
    threadlib_waitfor_sem(&m_started_sem);
    m_ripping = FALSE;

    // Causes the code running in the thread to bail
    debug_printf ("Closing m_sock...\n");
    socklib_close(&m_sock);

    // Kill external process
    if (m_ripinfo.ep) {
	debug_printf ("Close external\n");
	close_external (&m_ripinfo.ep);
    }

    // blocks until everything is ok and closed
    debug_printf ("Waiting for m_hthread to close...\n");
    threadlib_waitforclose(&m_hthread);
    debug_printf ("Destroying subsystems...\n");
    destroy_subsystems();
    debug_printf ("Destroying m_started_sem\n");
    threadlib_destroy_sem(&m_started_sem);
    debug_printf ("Done with rip_manager_stop\n");
}

void
destroy_subsystems()
{
    ripstream_destroy();
#if defined (commentout)
    if (m_destroy_func) {
	m_destroy_func();
    }
#endif
    relaylib_shutdown();
    socklib_cleanup();
    filelib_shutdown();
}

static int
create_pls_file (RIP_MANAGER_OPTIONS* rmo)
{
    FILE *fid;

    if  ('\0' == rmo->relay_ip[0]) {
	fprintf(stderr, "can not determine relaying ip, pass ip to -r \n");
	return -1;
    }

    fid = fopen (rmo->pls_file, "w");

    if (NULL == fid) {
	fprintf(stderr, "could not create playlist file '%s': %d '%s'\n",
		rmo->pls_file, errno, strerror(errno));
    } else {
	fprintf(fid, "[playlist]\n");
	fprintf(fid, "NumberOfEntries=1\n");
	fprintf(fid, "File1=http://%s:%d\n", rmo->relay_ip, rmo->relay_port);
	fclose(fid);
    }

    return 0;
}

static error_code
start_ripping (RIP_MANAGER_OPTIONS* rmo)
{
    error_code ret;

    char *pproxy = rmo->proxyurl[0] ? rmo->proxyurl : NULL;
    debug_printf ("start_ripping: checkpoint 1\n");

    /* If proxy URL not spec'd on command line (or plugin field), 
       check the environment variable */
    if (!pproxy) {
	char const *env_http_proxy = getenv ("http_proxy");
	if (env_http_proxy) {
	    strncpy (rmo->proxyurl, env_http_proxy, MAX_URL_LEN);
	}
    }

    debug_printf ("start_ripping: checkpoint 2\n");

    /* Connect to the stream */
    ret = httplib_sc_connect (&m_sock, rmo->url, pproxy, &m_http_info, 
			      rmo->useragent, rmo->if_name);
    if (ret != SR_SUCCESS) {
	goto RETURN_ERR;
    }

    /* If the icy_name exists, but is empty, set to a bogus name so 
       that we can create the directory correctly, etc. */
    if (strlen(m_http_info.icy_name) == 0) {
	strcpy (m_http_info.icy_name, "Streamripper_rips");
    }

    /* Set the ripinfo struct from the data we now know about the 
     * stream, this info are things like server name, type, 
     * bitrate etc.. */
    memset(&m_ripinfo, 0, sizeof(RIP_MANAGER_INFO));
    m_ripinfo.meta_interval = m_http_info.meta_interval;
    m_ripinfo.bitrate = m_http_info.icy_bitrate;
    strcpy(m_ripinfo.streamname, m_http_info.icy_name);
    strcpy(m_ripinfo.server_name, m_http_info.server);

    /* Initialize file writing code. */
    ret = filelib_init
	    (GET_INDIVIDUAL_TRACKS (rmo->flags),
	     GET_COUNT_FILES(rmo->flags),
	     rmo->count_start,
	     GET_KEEP_INCOMPLETE(rmo->flags),
	     GET_SINGLE_FILE_OUTPUT(rmo->flags),
	     m_http_info.content_type, 
	     rmo->output_directory,
	     rmo->output_pattern,
	     rmo->showfile_pattern,
	     GET_SEPERATE_DIRS(rmo->flags),
	     GET_DATE_STAMP(rmo->flags),
	     m_http_info.icy_name);
    if (ret != SR_SUCCESS)
	goto RETURN_ERR;

#if defined (commentout)
    /* This doesn't seem to be used */
    m_status_callback(RM_OUTPUT_DIR, (void*)filelib_get_output_directory);
#endif

    /* Start up external program to get metadata. */
    m_ripinfo.ep = 0;
    if (GET_EXTERNAL_CMD(rmo->flags)) {
	debug_printf ("Spawn external: %s\n", rmo->ext_cmd);
	m_ripinfo.ep = spawn_external (rmo->ext_cmd);
	if (m_ripinfo.ep) {
	    debug_printf ("Spawn external succeeded\n");
	} else {
	    debug_printf ("Spawn external failed\n");
	}
    }

    /* ripstream is good to go, it knows how to get data, and where
     * it's sending it to
     */
    ripstream_destroy();
    ret = ripstream_init(m_sock, 
			 GET_MAKE_RELAY(rmo->flags),
			 rmo->timeout, 
			 m_http_info.icy_name,
			 rmo->dropcount,
			 &rmo->sp_opt,
			 m_ripinfo.bitrate, 
			 m_http_info.meta_interval,
			 m_http_info.content_type, 
			 GET_ADD_ID3V1(rmo->flags),
			 GET_ADD_ID3V2(rmo->flags),
			 m_ripinfo.ep);
    if (ret != SR_SUCCESS) {
	ripstream_destroy();
	goto RETURN_ERR;
    }

    /*
     * tells the socket relay to start, this local wrapper is for 
     * setting the respond header, it needs to know server info to 
     * give winamp any usfull information about
     * the stream we are relaying.. this just sets the header to 
     * something very simulare to what we got from the stream.
     */
    if (GET_MAKE_RELAY (rmo->flags)) {
	u_short new_port = 0;
	ret = relaylib_init(GET_SEARCH_PORTS(rmo->flags), 
			    rmo->relay_port, rmo->max_port, 
			    &new_port, rmo->if_name, 
			    rmo->max_connections, rmo->relay_ip,
			    m_http_info.meta_interval != NO_META_INTERVAL);
	if (ret != SR_SUCCESS) {
		goto RETURN_ERR;
	}

	rmo->relay_port = new_port;
	start_relay(m_http_info.content_type);

	if (0 != rmo->pls_file[0]) {
		create_pls_file (rmo);
	}
    }
    post_status(RM_STATUS_BUFFERING);
    return SR_SUCCESS;

RETURN_ERR:
    socklib_close(&m_sock);
#if defined (commentout)
    if (m_destroy_func)
	m_destroy_func();
#endif
    return ret;	
}

error_code
rip_manager_start (void (*status_callback)(int message, void *data), 
			     RIP_MANAGER_OPTIONS *rmo)
{
    int ret = 0;
    m_started_sem = threadlib_create_sem();
    if (m_ripping)
	return SR_SUCCESS;		// to prevent reentrenty

    m_ripping = TRUE;

    register_codesets (&rmo->cs_opt);

    if (!rmo)
	return SR_ERROR_INVALID_PARAM;

    socklib_init();

    init_error_strings();
    m_status_callback = status_callback;
    m_bytes_ripped = 0;

    /* Initialize the parsing rules */
    init_metadata_parser (rmo->rules_file);

#if defined (commentout)
    /* Get a local copy of the options passed */
    memcpy(&m_options, options, sizeof(RIP_MANAGER_OPTIONS));
#endif

    /* Start the ripping thread */
    m_ripping = TRUE;
    ret = threadlib_beginthread (&m_hthread, ripthread, (void*) rmo);
    return ret;
}

/* Winamp plugin needs to get content type */
int
rip_manager_get_content_type (void)
{
    return m_http_info.content_type;
}

enum OverwriteOpt
string_to_overwrite_opt (char* str)
{
    int i;
    for (i = 0; i < 4; i++) {
	if (strcmp(str, overwrite_opt_strings[i]) == 0) {
	    return i;
	}
    }
    return OVERWRITE_UNKNOWN;
}

char*
overwrite_opt_to_string (enum OverwriteOpt oo)
{
    return overwrite_opt_strings[(int) oo];
}

void
set_rip_manager_options_defaults (RIP_MANAGER_OPTIONS *rmo)
{
    debug_printf ("- set_rip_manager_options_defaults -\n");
    rmo->url[0] = 0;
    rmo->proxyurl[0] = 0;
    strcpy(rmo->output_directory, "./");
    rmo->output_pattern[0] = 0;
    rmo->showfile_pattern[0] = 0;
    rmo->if_name[0] = 0;
    rmo->rules_file[0] = 0;
    rmo->pls_file[0] = 0;
    rmo->relay_ip[0] = 0;
    rmo->relay_port = 8000;
    rmo->max_port = 18000;
    rmo->max_connections = 1;
    rmo->maxMB_rip_size = 0;
    rmo->flags = OPT_AUTO_RECONNECT | 
	    OPT_SEPERATE_DIRS | 
	    OPT_SEARCH_PORTS |
	    /* OPT_ADD_ID3V1 | -- removed starting 1.62-beta-2 */
	    OPT_ADD_ID3V2 |
	    OPT_INDIVIDUAL_TRACKS;
    strcpy(rmo->useragent, "sr-POSIX/" SRVERSION);

    // Defaults for splitpoint - times are in ms
    rmo->sp_opt.xs = 1;
    rmo->sp_opt.xs_min_volume = 1;
    rmo->sp_opt.xs_silence_length = 1000;
    rmo->sp_opt.xs_search_window_1 = 6000;
    rmo->sp_opt.xs_search_window_2 = 6000;
    rmo->sp_opt.xs_offset = 0;
    rmo->sp_opt.xs_padding_1 = 300;
    rmo->sp_opt.xs_padding_2 = 300;

    /* GCS FIX: What is the difference between this timeout 
       and the one used in setsockopt()? */
#if defined (commentout)
    rmo->timeout = 0;
#endif
    rmo->timeout = 15;
    rmo->dropcount = 0;

    // Defaults for codeset
    memset (&rmo->cs_opt, 0, sizeof(CODESET_OPTIONS));
    set_codesets_default (&rmo->cs_opt);

    rmo->count_start = 0;
    rmo->overwrite = OVERWRITE_LARGER;
    rmo->ext_cmd[0] = 0;
}
