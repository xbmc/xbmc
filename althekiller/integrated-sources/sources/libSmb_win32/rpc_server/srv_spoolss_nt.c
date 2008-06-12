/*
 *  Unix SMB/CIFS implementation.
 *  RPC Pipe client / server routines
 *  Copyright (C) Andrew Tridgell              1992-2000,
 *  Copyright (C) Luke Kenneth Casson Leighton 1996-2000,
 *  Copyright (C) Jean François Micouleau      1998-2000,
 *  Copyright (C) Jeremy Allison               2001-2002,
 *  Copyright (C) Gerald Carter		       2000-2004,
 *  Copyright (C) Tim Potter                   2001-2002.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

/* Since the SPOOLSS rpc routines are basically DOS 16-bit calls wrapped
   up, all the errors returned are DOS errors, not NT status codes. */

#include "includes.h"

extern userdom_struct current_user_info;

#undef DBGC_CLASS
#define DBGC_CLASS DBGC_RPC_SRV

#ifndef MAX_OPEN_PRINTER_EXS
#define MAX_OPEN_PRINTER_EXS 50
#endif

#define MAGIC_DISPLAY_FREQUENCY 0xfade2bad
#define PHANTOM_DEVMODE_KEY "_p_f_a_n_t_0_m_"

struct table_node {
	const char    *long_archi;
	const char    *short_archi;
	int     version;
};

static Printer_entry *printers_list;

typedef struct _counter_printer_0 {
	struct _counter_printer_0 *next;
	struct _counter_printer_0 *prev;
	
	int snum;
	uint32 counter;
} counter_printer_0;

static counter_printer_0 *counter_list;

static struct rpc_pipe_client *notify_cli_pipe; /* print notify back-channel pipe handle*/
static uint32 smb_connections=0;


/* in printing/nt_printing.c */

extern STANDARD_MAPPING printer_std_mapping, printserver_std_mapping;

#define OUR_HANDLE(hnd) (((hnd)==NULL)?"NULL":(IVAL((hnd)->data5,4)==(uint32)sys_getpid()?"OURS":"OTHER")), \
((unsigned int)IVAL((hnd)->data5,4)),((unsigned int)sys_getpid())


/* API table for Xcv Monitor functions */

struct xcv_api_table {
	const char *name;
	WERROR(*fn) (NT_USER_TOKEN *token, RPC_BUFFER *in, RPC_BUFFER *out, uint32 *needed);
};


/* translate between internal status numbers and NT status numbers */
static int nt_printj_status(int v)
{
	switch (v) {
	case LPQ_QUEUED:
		return 0;
	case LPQ_PAUSED:
		return JOB_STATUS_PAUSED;
	case LPQ_SPOOLING:
		return JOB_STATUS_SPOOLING;
	case LPQ_PRINTING:
		return JOB_STATUS_PRINTING;
	case LPQ_ERROR:
		return JOB_STATUS_ERROR;
	case LPQ_DELETING:
		return JOB_STATUS_DELETING;
	case LPQ_OFFLINE:
		return JOB_STATUS_OFFLINE;
	case LPQ_PAPEROUT:
		return JOB_STATUS_PAPEROUT;
	case LPQ_PRINTED:
		return JOB_STATUS_PRINTED;
	case LPQ_DELETED:
		return JOB_STATUS_DELETED;
	case LPQ_BLOCKED:
		return JOB_STATUS_BLOCKED;
	case LPQ_USER_INTERVENTION:
		return JOB_STATUS_USER_INTERVENTION;
	}
	return 0;
}

static int nt_printq_status(int v)
{
	switch (v) {
	case LPQ_PAUSED:
		return PRINTER_STATUS_PAUSED;
	case LPQ_QUEUED:
	case LPQ_SPOOLING:
	case LPQ_PRINTING:
		return 0;
	}
	return 0;
}

/****************************************************************************
 Functions to handle SPOOL_NOTIFY_OPTION struct stored in Printer_entry.
****************************************************************************/

static void free_spool_notify_option(SPOOL_NOTIFY_OPTION **pp)
{
	if (*pp == NULL)
		return;

	SAFE_FREE((*pp)->ctr.type);
	SAFE_FREE(*pp);
}

/***************************************************************************
 Disconnect from the client
****************************************************************************/

static void srv_spoolss_replycloseprinter(int snum, POLICY_HND *handle)
{
	WERROR result;

	/* 
	 * Tell the specific printing tdb we no longer want messages for this printer
	 * by deregistering our PID.
	 */

	if (!print_notify_deregister_pid(snum))
		DEBUG(0,("print_notify_register_pid: Failed to register our pid for printer %s\n", lp_const_servicename(snum) ));

	/* weird if the test succeds !!! */
	if (smb_connections==0) {
		DEBUG(0,("srv_spoolss_replycloseprinter:Trying to close non-existant notify backchannel !\n"));
		return;
	}

	result = rpccli_spoolss_reply_close_printer(notify_cli_pipe, notify_cli_pipe->cli->mem_ctx, handle);
	
	if (!W_ERROR_IS_OK(result))
		DEBUG(0,("srv_spoolss_replycloseprinter: reply_close_printer failed [%s].\n",
			dos_errstr(result)));

	/* if it's the last connection, deconnect the IPC$ share */
	if (smb_connections==1) {

		cli_shutdown( notify_cli_pipe->cli );
		notify_cli_pipe = NULL; /* The above call shuts downn the pipe also. */

		message_deregister(MSG_PRINTER_NOTIFY2);

        	/* Tell the connections db we're no longer interested in
		 * printer notify messages. */

		register_message_flags( False, FLAG_MSG_PRINT_NOTIFY );
	}

	smb_connections--;
}

/****************************************************************************
 Functions to free a printer entry datastruct.
****************************************************************************/

static void free_printer_entry(void *ptr)
{
	Printer_entry *Printer = (Printer_entry *)ptr;

	if (Printer->notify.client_connected==True) {
		int snum = -1;

		if ( Printer->printer_type == SPLHND_SERVER) {
			snum = -1;
			srv_spoolss_replycloseprinter(snum, &Printer->notify.client_hnd);
		} else if (Printer->printer_type == SPLHND_PRINTER) {
			snum = print_queue_snum(Printer->sharename);
			if (snum != -1)
				srv_spoolss_replycloseprinter(snum,
						&Printer->notify.client_hnd);
		}
	}

	Printer->notify.flags=0;
	Printer->notify.options=0;
	Printer->notify.localmachine[0]='\0';
	Printer->notify.printerlocal=0;
	free_spool_notify_option(&Printer->notify.option);
	Printer->notify.option=NULL;
	Printer->notify.client_connected=False;
	
	free_nt_devicemode( &Printer->nt_devmode );
	free_a_printer( &Printer->printer_info, 2 );
	
	talloc_destroy( Printer->ctx );

	/* Remove from the internal list. */
	DLIST_REMOVE(printers_list, Printer);

	SAFE_FREE(Printer);
}

/****************************************************************************
 Functions to duplicate a SPOOL_NOTIFY_OPTION struct stored in Printer_entry.
****************************************************************************/

static SPOOL_NOTIFY_OPTION *dup_spool_notify_option(SPOOL_NOTIFY_OPTION *sp)
{
	SPOOL_NOTIFY_OPTION *new_sp = NULL;

	if (!sp)
		return NULL;

	new_sp = SMB_MALLOC_P(SPOOL_NOTIFY_OPTION);
	if (!new_sp)
		return NULL;

	*new_sp = *sp;

	if (sp->ctr.count) {
		new_sp->ctr.type = (SPOOL_NOTIFY_OPTION_TYPE *)memdup(sp->ctr.type, sizeof(SPOOL_NOTIFY_OPTION_TYPE) * sp->ctr.count);

		if (!new_sp->ctr.type) {
			SAFE_FREE(new_sp);
			return NULL;
		}
	}

	return new_sp;
}

/****************************************************************************
  find printer index by handle
****************************************************************************/

static Printer_entry *find_printer_index_by_hnd(pipes_struct *p, POLICY_HND *hnd)
{
	Printer_entry *find_printer = NULL;

	if(!find_policy_by_hnd(p,hnd,(void **)(void *)&find_printer)) {
		DEBUG(2,("find_printer_index_by_hnd: Printer handle not found: "));
		return NULL;
	}

	return find_printer;
}

/****************************************************************************
 Close printer index by handle.
****************************************************************************/

static BOOL close_printer_handle(pipes_struct *p, POLICY_HND *hnd)
{
	Printer_entry *Printer = find_printer_index_by_hnd(p, hnd);

	if (!Printer) {
		DEBUG(2,("close_printer_handle: Invalid handle (%s:%u:%u)\n", OUR_HANDLE(hnd)));
		return False;
	}

	close_policy_hnd(p, hnd);

	return True;
}	

/****************************************************************************
 Delete a printer given a handle.
****************************************************************************/
WERROR delete_printer_hook( NT_USER_TOKEN *token, const char *sharename )
{
	char *cmd = lp_deleteprinter_cmd();
	pstring command;
	int ret;
	SE_PRIV se_printop = SE_PRINT_OPERATOR;
	BOOL is_print_op = False;
		
	/* can't fail if we don't try */
	
	if ( !*cmd )
		return WERR_OK;
		
	pstr_sprintf(command, "%s \"%s\"", cmd, sharename);

	if ( token )
		is_print_op = user_has_privileges( token, &se_printop );
	
	DEBUG(10,("Running [%s]\n", command));

	/********** BEGIN SePrintOperatorPrivlege BLOCK **********/
	
	if ( is_print_op )
		become_root();
		
	if ( (ret = smbrun(command, NULL)) == 0 ) {
		/* Tell everyone we updated smb.conf. */
		message_send_all(conn_tdb_ctx(), MSG_SMB_CONF_UPDATED, NULL, 0, False, NULL);
	}
		
	if ( is_print_op )
		unbecome_root();

	/********** END SePrintOperatorPrivlege BLOCK **********/
	
	DEBUGADD(10,("returned [%d]\n", ret));

	if (ret != 0) 
		return WERR_BADFID; /* What to return here? */

	/* go ahead and re-read the services immediately */
	reload_services( False );
	
	if ( lp_servicenumber( sharename )  < 0 )
		return WERR_ACCESS_DENIED;
		
	return WERR_OK;
}

/****************************************************************************
 Delete a printer given a handle.
****************************************************************************/

static WERROR delete_printer_handle(pipes_struct *p, POLICY_HND *hnd)
{
	Printer_entry *Printer = find_printer_index_by_hnd(p, hnd);

	if (!Printer) {
		DEBUG(2,("delete_printer_handle: Invalid handle (%s:%u:%u)\n", OUR_HANDLE(hnd)));
		return WERR_BADFID;
	}

	/* 
	 * It turns out that Windows allows delete printer on a handle
	 * opened by an admin user, then used on a pipe handle created
	 * by an anonymous user..... but they're working on security.... riiight !
	 * JRA.
	 */

	if (Printer->access_granted != PRINTER_ACCESS_ADMINISTER) {
		DEBUG(3, ("delete_printer_handle: denied by handle\n"));
		return WERR_ACCESS_DENIED;
	}
	
	/* this does not need a become root since the access check has been 
	   done on the handle already */
	   
	if (del_a_printer( Printer->sharename ) != 0) {
		DEBUG(3,("Error deleting printer %s\n", Printer->sharename));
		return WERR_BADFID;
	}

	return delete_printer_hook( p->pipe_user.nt_user_token, Printer->sharename );
}

/****************************************************************************
 Return the snum of a printer corresponding to an handle.
****************************************************************************/

static BOOL get_printer_snum(pipes_struct *p, POLICY_HND *hnd, int *number)
{
	Printer_entry *Printer = find_printer_index_by_hnd(p, hnd);
		
	if (!Printer) {
		DEBUG(2,("get_printer_snum: Invalid handle (%s:%u:%u)\n", OUR_HANDLE(hnd)));
		return False;
	}
	
	switch (Printer->printer_type) {
		case SPLHND_PRINTER:		
			DEBUG(4,("short name:%s\n", Printer->sharename));			
			*number = print_queue_snum(Printer->sharename);
			return (*number != -1);
		case SPLHND_SERVER:
			return False;
		default:
			return False;
	}
}

/****************************************************************************
 Set printer handle type.
 Check if it's \\server or \\server\printer
****************************************************************************/

static BOOL set_printer_hnd_printertype(Printer_entry *Printer, char *handlename)
{
	DEBUG(3,("Setting printer type=%s\n", handlename));

	if ( strlen(handlename) < 3 ) {
		DEBUGADD(4,("A print server must have at least 1 char ! %s\n", handlename));
		return False;
	}

	/* it's a print server */
	if (*handlename=='\\' && *(handlename+1)=='\\' && !strchr_m(handlename+2, '\\')) {
		DEBUGADD(4,("Printer is a print server\n"));
		Printer->printer_type = SPLHND_SERVER;		
	}
	/* it's a printer (set_printer_hnd_name() will handle port monitors */
	else {
		DEBUGADD(4,("Printer is a printer\n"));
		Printer->printer_type = SPLHND_PRINTER;
	}

	return True;
}

/****************************************************************************
 Set printer handle name..  Accept names like \\server, \\server\printer, 
 \\server\SHARE, & "\\server\,XcvMonitor Standard TCP/IP Port"    See
 the MSDN docs regarding OpenPrinter() for details on the XcvData() and 
 XcvDataPort() interface.
****************************************************************************/

static BOOL set_printer_hnd_name(Printer_entry *Printer, char *handlename)
{
	int snum;
	int n_services=lp_numservices();
	char *aprinter, *printername;
	const char *servername;
	fstring sname;
	BOOL found=False;
	NT_PRINTER_INFO_LEVEL *printer = NULL;
	WERROR result;
	
	DEBUG(4,("Setting printer name=%s (len=%lu)\n", handlename, (unsigned long)strlen(handlename)));

	aprinter = handlename;
	if ( *handlename == '\\' ) {
		servername = handlename + 2;
		if ( (aprinter = strchr_m( handlename+2, '\\' )) != NULL ) {
			*aprinter = '\0';
			aprinter++;
		}
	}
	else {
		servername = "";
	}
	
	/* save the servername to fill in replies on this handle */
	
	if ( !is_myname_or_ipaddr( servername ) )
		return False;

	fstrcpy( Printer->servername, servername );
	
	if ( Printer->printer_type == SPLHND_SERVER )
		return True;

	if ( Printer->printer_type != SPLHND_PRINTER )
		return False;

	DEBUGADD(5, ("searching for [%s]\n", aprinter ));
	
	/* check for the Port Monitor Interface */
	
	if ( strequal( aprinter, SPL_XCV_MONITOR_TCPMON ) ) {
		Printer->printer_type = SPLHND_PORTMON_TCP;
		fstrcpy(sname, SPL_XCV_MONITOR_TCPMON);
		found = True;
	}
	else if ( strequal( aprinter, SPL_XCV_MONITOR_LOCALMON ) ) {
		Printer->printer_type = SPLHND_PORTMON_LOCAL;
		fstrcpy(sname, SPL_XCV_MONITOR_LOCALMON);
		found = True;
	}

	/* Search all sharenames first as this is easier than pulling 
	   the printer_info_2 off of disk. Don't use find_service() since
	   that calls out to map_username() */
	
	/* do another loop to look for printernames */
	
	for (snum=0; !found && snum<n_services; snum++) {

		/* no point going on if this is not a printer */

		if ( !(lp_snum_ok(snum) && lp_print_ok(snum)) )
			continue;

		fstrcpy(sname, lp_servicename(snum));
		if ( strequal( aprinter, sname ) ) {
			found = True;
			break;
		}

		/* no point looking up the printer object if
		   we aren't allowing printername != sharename */
		
		if ( lp_force_printername(snum) )
			continue;

		fstrcpy(sname, lp_servicename(snum));

		printer = NULL;
		result = get_a_printer( NULL, &printer, 2, sname );
		if ( !W_ERROR_IS_OK(result) ) {
			DEBUG(0,("set_printer_hnd_name: failed to lookup printer [%s] -- result [%s]\n",
				sname, dos_errstr(result)));
			continue;
		}
		
		/* printername is always returned as \\server\printername */
		if ( !(printername = strchr_m(&printer->info_2->printername[2], '\\')) ) {
			DEBUG(0,("set_printer_hnd_name: info2->printername in wrong format! [%s]\n",
				printer->info_2->printername));
			free_a_printer( &printer, 2);
			continue;
		}
		
		printername++;
		
		if ( strequal(printername, aprinter) ) {
			free_a_printer( &printer, 2);
			found = True;
			break;
		}
		
		DEBUGADD(10, ("printername: %s\n", printername));
		
		free_a_printer( &printer, 2);
	}

	free_a_printer( &printer, 2);

	if ( !found ) {
		DEBUGADD(4,("Printer not found\n"));
		return False;
	}
	
	DEBUGADD(4,("set_printer_hnd_name: Printer found: %s -> %s\n", aprinter, sname));

	fstrcpy(Printer->sharename, sname);

	return True;
}

/****************************************************************************
 Find first available printer slot. creates a printer handle for you.
 ****************************************************************************/

static BOOL open_printer_hnd(pipes_struct *p, POLICY_HND *hnd, char *name, uint32 access_granted)
{
	Printer_entry *new_printer;

	DEBUG(10,("open_printer_hnd: name [%s]\n", name));

	if((new_printer=SMB_MALLOC_P(Printer_entry)) == NULL)
		return False;

	ZERO_STRUCTP(new_printer);
	
	if (!create_policy_hnd(p, hnd, free_printer_entry, new_printer)) {
		SAFE_FREE(new_printer);
		return False;
	}
	
	/* Add to the internal list. */
	DLIST_ADD(printers_list, new_printer);
	
	new_printer->notify.option=NULL;
				
	if ( !(new_printer->ctx = talloc_init("Printer Entry [%p]", hnd)) ) {
		DEBUG(0,("open_printer_hnd: talloc_init() failed!\n"));
		close_printer_handle(p, hnd);
		return False;
	}
	
	if (!set_printer_hnd_printertype(new_printer, name)) {
		close_printer_handle(p, hnd);
		return False;
	}
	
	if (!set_printer_hnd_name(new_printer, name)) {
		close_printer_handle(p, hnd);
		return False;
	}

	new_printer->access_granted = access_granted;

	DEBUG(5, ("%d printer handles active\n", (int)p->pipe_handles->count ));

	return True;
}

/***************************************************************************
 check to see if the client motify handle is monitoring the notification
 given by (notify_type, notify_field).
 **************************************************************************/

static BOOL is_monitoring_event_flags(uint32 flags, uint16 notify_type,
				      uint16 notify_field)
{
	return True;
}

static BOOL is_monitoring_event(Printer_entry *p, uint16 notify_type,
				uint16 notify_field)
{
	SPOOL_NOTIFY_OPTION *option = p->notify.option;
	uint32 i, j;

	/* 
	 * Flags should always be zero when the change notify
	 * is registered by the client's spooler.  A user Win32 app
	 * might use the flags though instead of the NOTIFY_OPTION_INFO 
	 * --jerry
	 */

	if (!option) {
		return False;
	}

	if (p->notify.flags)
		return is_monitoring_event_flags(
			p->notify.flags, notify_type, notify_field);

	for (i = 0; i < option->count; i++) {
		
		/* Check match for notify_type */
		
		if (option->ctr.type[i].type != notify_type)
			continue;

		/* Check match for field */
		
		for (j = 0; j < option->ctr.type[i].count; j++) {
			if (option->ctr.type[i].fields[j] == notify_field) {
				return True;
			}
		}
	}
	
	DEBUG(10, ("Open handle for \\\\%s\\%s is not monitoring 0x%02x/0x%02x\n",
		   p->servername, p->sharename, notify_type, notify_field));
	
	return False;
}

/* Convert a notification message to a SPOOL_NOTIFY_INFO_DATA struct */

static void notify_one_value(struct spoolss_notify_msg *msg,
			     SPOOL_NOTIFY_INFO_DATA *data,
			     TALLOC_CTX *mem_ctx)
{
	data->notify_data.value[0] = msg->notify.value[0];
	data->notify_data.value[1] = 0;
}

static void notify_string(struct spoolss_notify_msg *msg,
			  SPOOL_NOTIFY_INFO_DATA *data,
			  TALLOC_CTX *mem_ctx)
{
	UNISTR2 unistr;
	
	/* The length of the message includes the trailing \0 */

	init_unistr2(&unistr, msg->notify.data, UNI_STR_TERMINATE);

	data->notify_data.data.length = msg->len * 2;
	data->notify_data.data.string = TALLOC_ARRAY(mem_ctx, uint16, msg->len);

	if (!data->notify_data.data.string) {
		data->notify_data.data.length = 0;
		return;
	}
	
	memcpy(data->notify_data.data.string, unistr.buffer, msg->len * 2);
}

static void notify_system_time(struct spoolss_notify_msg *msg,
			       SPOOL_NOTIFY_INFO_DATA *data,
			       TALLOC_CTX *mem_ctx)
{
	SYSTEMTIME systime;
	prs_struct ps;

	if (msg->len != sizeof(time_t)) {
		DEBUG(5, ("notify_system_time: received wrong sized message (%d)\n",
			  msg->len));
		return;
	}

	if (!prs_init(&ps, RPC_MAX_PDU_FRAG_LEN, mem_ctx, MARSHALL)) {
		DEBUG(5, ("notify_system_time: prs_init() failed\n"));
		return;
	}

	if (!make_systemtime(&systime, gmtime((time_t *)msg->notify.data))) {
		DEBUG(5, ("notify_system_time: unable to make systemtime\n"));
		prs_mem_free(&ps);
		return;
	}

	if (!spoolss_io_system_time("", &ps, 0, &systime)) {
		prs_mem_free(&ps);
		return;
	}

	data->notify_data.data.length = prs_offset(&ps);
	data->notify_data.data.string = TALLOC(mem_ctx, prs_offset(&ps));
	if (!data->notify_data.data.string) {
		prs_mem_free(&ps);
		return;
	}

	prs_copy_all_data_out((char *)data->notify_data.data.string, &ps);

	prs_mem_free(&ps);
}

struct notify2_message_table {
	const char *name;
	void (*fn)(struct spoolss_notify_msg *msg,
		   SPOOL_NOTIFY_INFO_DATA *data, TALLOC_CTX *mem_ctx);
};

static struct notify2_message_table printer_notify_table[] = {
	/* 0x00 */ { "PRINTER_NOTIFY_SERVER_NAME", notify_string },
	/* 0x01 */ { "PRINTER_NOTIFY_PRINTER_NAME", notify_string },
	/* 0x02 */ { "PRINTER_NOTIFY_SHARE_NAME", notify_string },
	/* 0x03 */ { "PRINTER_NOTIFY_PORT_NAME", notify_string },
	/* 0x04 */ { "PRINTER_NOTIFY_DRIVER_NAME", notify_string },
	/* 0x05 */ { "PRINTER_NOTIFY_COMMENT", notify_string },
	/* 0x06 */ { "PRINTER_NOTIFY_LOCATION", notify_string },
	/* 0x07 */ { "PRINTER_NOTIFY_DEVMODE", NULL },
	/* 0x08 */ { "PRINTER_NOTIFY_SEPFILE", notify_string },
	/* 0x09 */ { "PRINTER_NOTIFY_PRINT_PROCESSOR", notify_string },
	/* 0x0a */ { "PRINTER_NOTIFY_PARAMETERS", NULL },
	/* 0x0b */ { "PRINTER_NOTIFY_DATATYPE", notify_string },
	/* 0x0c */ { "PRINTER_NOTIFY_SECURITY_DESCRIPTOR", NULL },
	/* 0x0d */ { "PRINTER_NOTIFY_ATTRIBUTES", notify_one_value },
	/* 0x0e */ { "PRINTER_NOTIFY_PRIORITY", notify_one_value },
	/* 0x0f */ { "PRINTER_NOTIFY_DEFAULT_PRIORITY", NULL },
	/* 0x10 */ { "PRINTER_NOTIFY_START_TIME", NULL },
	/* 0x11 */ { "PRINTER_NOTIFY_UNTIL_TIME", NULL },
	/* 0x12 */ { "PRINTER_NOTIFY_STATUS", notify_one_value },
};

static struct notify2_message_table job_notify_table[] = {
	/* 0x00 */ { "JOB_NOTIFY_PRINTER_NAME", NULL },
	/* 0x01 */ { "JOB_NOTIFY_MACHINE_NAME", NULL },
	/* 0x02 */ { "JOB_NOTIFY_PORT_NAME", NULL },
	/* 0x03 */ { "JOB_NOTIFY_USER_NAME", notify_string },
	/* 0x04 */ { "JOB_NOTIFY_NOTIFY_NAME", NULL },
	/* 0x05 */ { "JOB_NOTIFY_DATATYPE", NULL },
	/* 0x06 */ { "JOB_NOTIFY_PRINT_PROCESSOR", NULL },
	/* 0x07 */ { "JOB_NOTIFY_PARAMETERS", NULL },
	/* 0x08 */ { "JOB_NOTIFY_DRIVER_NAME", NULL },
	/* 0x09 */ { "JOB_NOTIFY_DEVMODE", NULL },
	/* 0x0a */ { "JOB_NOTIFY_STATUS", notify_one_value },
	/* 0x0b */ { "JOB_NOTIFY_STATUS_STRING", NULL },
	/* 0x0c */ { "JOB_NOTIFY_SECURITY_DESCRIPTOR", NULL },
	/* 0x0d */ { "JOB_NOTIFY_DOCUMENT", notify_string },
	/* 0x0e */ { "JOB_NOTIFY_PRIORITY", NULL },
	/* 0x0f */ { "JOB_NOTIFY_POSITION", NULL },
	/* 0x10 */ { "JOB_NOTIFY_SUBMITTED", notify_system_time },
	/* 0x11 */ { "JOB_NOTIFY_START_TIME", NULL },
	/* 0x12 */ { "JOB_NOTIFY_UNTIL_TIME", NULL },
	/* 0x13 */ { "JOB_NOTIFY_TIME", NULL },
	/* 0x14 */ { "JOB_NOTIFY_TOTAL_PAGES", notify_one_value },
	/* 0x15 */ { "JOB_NOTIFY_PAGES_PRINTED", NULL },
	/* 0x16 */ { "JOB_NOTIFY_TOTAL_BYTES", notify_one_value },
	/* 0x17 */ { "JOB_NOTIFY_BYTES_PRINTED", NULL },
};


/***********************************************************************
 Allocate talloc context for container object
 **********************************************************************/
 
static void notify_msg_ctr_init( SPOOLSS_NOTIFY_MSG_CTR *ctr )
{
	if ( !ctr )
		return;

	ctr->ctx = talloc_init("notify_msg_ctr_init %p", ctr);
		
	return;
}

/***********************************************************************
 release all allocated memory and zero out structure
 **********************************************************************/
 
static void notify_msg_ctr_destroy( SPOOLSS_NOTIFY_MSG_CTR *ctr )
{
	if ( !ctr )
		return;

	if ( ctr->ctx )
		talloc_destroy(ctr->ctx);
		
	ZERO_STRUCTP(ctr);
		
	return;
}

/***********************************************************************
 **********************************************************************/
 
static TALLOC_CTX* notify_ctr_getctx( SPOOLSS_NOTIFY_MSG_CTR *ctr )
{
	if ( !ctr )
		return NULL;
		
	return ctr->ctx;
}

/***********************************************************************
 **********************************************************************/
 
static SPOOLSS_NOTIFY_MSG_GROUP* notify_ctr_getgroup( SPOOLSS_NOTIFY_MSG_CTR *ctr, uint32 idx )
{
	if ( !ctr || !ctr->msg_groups )
		return NULL;
	
	if ( idx >= ctr->num_groups )
		return NULL;
		
	return &ctr->msg_groups[idx];

}

/***********************************************************************
 How many groups of change messages do we have ?
 **********************************************************************/
 
static int notify_msg_ctr_numgroups( SPOOLSS_NOTIFY_MSG_CTR *ctr )
{
	if ( !ctr )
		return 0;
		
	return ctr->num_groups;
}

/***********************************************************************
 Add a SPOOLSS_NOTIFY_MSG_CTR to the correct group
 **********************************************************************/
 
static int notify_msg_ctr_addmsg( SPOOLSS_NOTIFY_MSG_CTR *ctr, SPOOLSS_NOTIFY_MSG *msg )
{
	SPOOLSS_NOTIFY_MSG_GROUP	*groups = NULL;
	SPOOLSS_NOTIFY_MSG_GROUP	*msg_grp = NULL;
	SPOOLSS_NOTIFY_MSG		*msg_list = NULL;
	int				i, new_slot;
	
	if ( !ctr || !msg )
		return 0;
	
	/* loop over all groups looking for a matching printer name */
	
	for ( i=0; i<ctr->num_groups; i++ ) {
		if ( strcmp(ctr->msg_groups[i].printername, msg->printer) == 0 )
			break;
	}
	
	/* add a new group? */
	
	if ( i == ctr->num_groups ) {
		ctr->num_groups++;

		if ( !(groups = TALLOC_REALLOC_ARRAY( ctr->ctx, ctr->msg_groups, SPOOLSS_NOTIFY_MSG_GROUP, ctr->num_groups)) ) {
			DEBUG(0,("notify_msg_ctr_addmsg: talloc_realloc() failed!\n"));
			return 0;
		}
		ctr->msg_groups = groups;

		/* clear the new entry and set the printer name */
		
		ZERO_STRUCT( ctr->msg_groups[ctr->num_groups-1] );
		fstrcpy( ctr->msg_groups[ctr->num_groups-1].printername, msg->printer );
	}
	
	/* add the change messages; 'i' is the correct index now regardless */
	
	msg_grp = &ctr->msg_groups[i];
	
	msg_grp->num_msgs++;
	
	if ( !(msg_list = TALLOC_REALLOC_ARRAY( ctr->ctx, msg_grp->msgs, SPOOLSS_NOTIFY_MSG, msg_grp->num_msgs )) ) {
		DEBUG(0,("notify_msg_ctr_addmsg: talloc_realloc() failed for new message [%d]!\n", msg_grp->num_msgs));
		return 0;
	}
	msg_grp->msgs = msg_list;
	
	new_slot = msg_grp->num_msgs-1;
	memcpy( &msg_grp->msgs[new_slot], msg, sizeof(SPOOLSS_NOTIFY_MSG) );
	
	/* need to allocate own copy of data */
	
	if ( msg->len != 0 ) 
		msg_grp->msgs[new_slot].notify.data = TALLOC_MEMDUP( ctr->ctx, msg->notify.data, msg->len );
	
	return ctr->num_groups;
}

/***********************************************************************
 Send a change notication message on all handles which have a call 
 back registered
 **********************************************************************/

static void send_notify2_changes( SPOOLSS_NOTIFY_MSG_CTR *ctr, uint32 idx )
{
	Printer_entry 		 *p;
	TALLOC_CTX		 *mem_ctx = notify_ctr_getctx( ctr );
	SPOOLSS_NOTIFY_MSG_GROUP *msg_group = notify_ctr_getgroup( ctr, idx );
	SPOOLSS_NOTIFY_MSG       *messages;
	int			 sending_msg_count;
	
	if ( !msg_group ) {
		DEBUG(5,("send_notify2_changes() called with no msg group!\n"));
		return;
	}
	
	messages = msg_group->msgs;
	
	if ( !messages ) {
		DEBUG(5,("send_notify2_changes() called with no messages!\n"));
		return;
	}
	
	DEBUG(8,("send_notify2_changes: Enter...[%s]\n", msg_group->printername));
	
	/* loop over all printers */
	
	for (p = printers_list; p; p = p->next) {
		SPOOL_NOTIFY_INFO_DATA *data;
		uint32	data_len = 0;
		uint32 	id;
		int 	i;

		/* Is there notification on this handle? */

		if ( !p->notify.client_connected )
			continue;

		DEBUG(10,("Client connected! [\\\\%s\\%s]\n", p->servername, p->sharename));

		/* For this printer?  Print servers always receive 
                   notifications. */

		if ( ( p->printer_type == SPLHND_PRINTER )  &&
		    ( !strequal(msg_group->printername, p->sharename) ) )
			continue;

		DEBUG(10,("Our printer\n"));
		
		/* allocate the max entries possible */
		
		data = TALLOC_ARRAY( mem_ctx, SPOOL_NOTIFY_INFO_DATA, msg_group->num_msgs);
		if (!data) {
			return;
		}

		ZERO_STRUCTP(data);
		
		/* build the array of change notifications */
		
		sending_msg_count = 0;
		
		for ( i=0; i<msg_group->num_msgs; i++ ) {
			SPOOLSS_NOTIFY_MSG	*msg = &messages[i];
			
			/* Are we monitoring this event? */

			if (!is_monitoring_event(p, msg->type, msg->field))
				continue;

			sending_msg_count++;
			
			
			DEBUG(10,("process_notify2_message: Sending message type [0x%x] field [0x%2x] for printer [%s]\n",
				msg->type, msg->field, p->sharename));

			/* 
			 * if the is a printer notification handle and not a job notification 
			 * type, then set the id to 0.  Other wise just use what was specified
			 * in the message.  
			 *
			 * When registering change notification on a print server handle 
			 * we always need to send back the id (snum) matching the printer
			 * for which the change took place.  For change notify registered
			 * on a printer handle, this does not matter and the id should be 0.
			 *
			 * --jerry
			 */

			if ( ( p->printer_type == SPLHND_PRINTER ) && ( msg->type == PRINTER_NOTIFY_TYPE ) )
				id = 0;
			else
				id = msg->id;


			/* Convert unix jobid to smb jobid */

			if (msg->flags & SPOOLSS_NOTIFY_MSG_UNIX_JOBID) {
				id = sysjob_to_jobid(msg->id);

				if (id == -1) {
					DEBUG(3, ("no such unix jobid %d\n", msg->id));
					goto done;
				}
			}

			construct_info_data( &data[data_len], msg->type, msg->field, id );

			switch(msg->type) {
			case PRINTER_NOTIFY_TYPE:
				if ( printer_notify_table[msg->field].fn )
					printer_notify_table[msg->field].fn(msg, &data[data_len], mem_ctx);
				break;
			
			case JOB_NOTIFY_TYPE:
				if ( job_notify_table[msg->field].fn )
					job_notify_table[msg->field].fn(msg, &data[data_len], mem_ctx);
				break;

			default:
				DEBUG(5, ("Unknown notification type %d\n", msg->type));
				goto done;
			}

			data_len++;
		}

		if ( sending_msg_count ) {
			rpccli_spoolss_rrpcn( notify_cli_pipe, mem_ctx, &p->notify.client_hnd, 
					data_len, data, p->notify.change, 0 );
		}
	}
	
done:
	DEBUG(8,("send_notify2_changes: Exit...\n"));
	return;
}

/***********************************************************************
 **********************************************************************/

static BOOL notify2_unpack_msg( SPOOLSS_NOTIFY_MSG *msg, struct timeval *tv, void *buf, size_t len )
{

	uint32 tv_sec, tv_usec;
	size_t offset = 0;

	/* Unpack message */

	offset += tdb_unpack((char *)buf + offset, len - offset, "f",
			     msg->printer);
	
	offset += tdb_unpack((char *)buf + offset, len - offset, "ddddddd",
				&tv_sec, &tv_usec,
				&msg->type, &msg->field, &msg->id, &msg->len, &msg->flags);

	if (msg->len == 0)
		tdb_unpack((char *)buf + offset, len - offset, "dd",
			   &msg->notify.value[0], &msg->notify.value[1]);
	else
		tdb_unpack((char *)buf + offset, len - offset, "B", 
			   &msg->len, &msg->notify.data);

	DEBUG(3, ("notify2_unpack_msg: got NOTIFY2 message for printer %s, jobid %u type %d, field 0x%02x, flags 0x%04x\n",
		  msg->printer, (unsigned int)msg->id, msg->type, msg->field, msg->flags));

	tv->tv_sec = tv_sec;
	tv->tv_usec = tv_usec;

	if (msg->len == 0)
		DEBUG(3, ("notify2_unpack_msg: value1 = %d, value2 = %d\n", msg->notify.value[0],
			  msg->notify.value[1]));
	else
		dump_data(3, msg->notify.data, msg->len);

	return True;
}

/********************************************************************
 Receive a notify2 message list
 ********************************************************************/

static void receive_notify2_message_list(int msg_type, struct process_id src,
					 void *msg, size_t len)
{
	size_t 			msg_count, i;
	char 			*buf = (char *)msg;
	char 			*msg_ptr;
	size_t 			msg_len;
	SPOOLSS_NOTIFY_MSG	notify;
	SPOOLSS_NOTIFY_MSG_CTR	messages;
	int			num_groups;

	if (len < 4) {
		DEBUG(0,("receive_notify2_message_list: bad message format (len < 4)!\n"));
		return;
	}
	
	msg_count = IVAL(buf, 0);
	msg_ptr = buf + 4;

	DEBUG(5, ("receive_notify2_message_list: got %lu messages in list\n", (unsigned long)msg_count));

	if (msg_count == 0) {
		DEBUG(0,("receive_notify2_message_list: bad message format (msg_count == 0) !\n"));
		return;
	}

	/* initialize the container */
	
	ZERO_STRUCT( messages );
	notify_msg_ctr_init( &messages );
	
	/* 
	 * build message groups for each printer identified
	 * in a change_notify msg.  Remember that a PCN message
	 * includes the handle returned for the srv_spoolss_replyopenprinter()
	 * call.  Therefore messages are grouped according to printer handle.
	 */
	 
	for ( i=0; i<msg_count; i++ ) {
		struct timeval msg_tv;

		if (msg_ptr + 4 - buf > len) {
			DEBUG(0,("receive_notify2_message_list: bad message format (len > buf_size) !\n"));
			return;
		}

		msg_len = IVAL(msg_ptr,0);
		msg_ptr += 4;

		if (msg_ptr + msg_len - buf > len) {
			DEBUG(0,("receive_notify2_message_list: bad message format (bad len) !\n"));
			return;
		}
		
		/* unpack messages */
		
		ZERO_STRUCT( notify );
		notify2_unpack_msg( &notify, &msg_tv, msg_ptr, msg_len );
		msg_ptr += msg_len;

		/* add to correct list in container */
		
		notify_msg_ctr_addmsg( &messages, &notify );
		
		/* free memory that might have been allocated by notify2_unpack_msg() */
		
		if ( notify.len != 0 )
			SAFE_FREE( notify.notify.data );
	}
	
	/* process each group of messages */
	
	num_groups = notify_msg_ctr_numgroups( &messages );
	for ( i=0; i<num_groups; i++ )
		send_notify2_changes( &messages, i );
	
	
	/* cleanup */
		
	DEBUG(10,("receive_notify2_message_list: processed %u messages\n", (uint32)msg_count ));
		
	notify_msg_ctr_destroy( &messages );
	
	return;
}

/********************************************************************
 Send a message to ourself about new driver being installed
 so we can upgrade the information for each printer bound to this
 driver
 ********************************************************************/
 
static BOOL srv_spoolss_drv_upgrade_printer(char* drivername)
{
	int len = strlen(drivername);
	
	if (!len)
		return False;

	DEBUG(10,("srv_spoolss_drv_upgrade_printer: Sending message about driver upgrade [%s]\n",
		drivername));
		
	message_send_pid(pid_to_procid(sys_getpid()),
			 MSG_PRINTER_DRVUPGRADE, drivername, len+1, False);

	return True;
}

/**********************************************************************
 callback to receive a MSG_PRINTER_DRVUPGRADE message and interate
 over all printers, upgrading ones as necessary 
 **********************************************************************/
 
void do_drv_upgrade_printer(int msg_type, struct process_id src, void *buf, size_t len)
{
	fstring drivername;
	int snum;
	int n_services = lp_numservices();
	
	len = MIN(len,sizeof(drivername)-1);
	strncpy(drivername, buf, len);
	
	DEBUG(10,("do_drv_upgrade_printer: Got message for new driver [%s]\n", drivername ));

	/* Iterate the printer list */
	
	for (snum=0; snum<n_services; snum++)
	{
		if (lp_snum_ok(snum) && lp_print_ok(snum) ) 
		{
			WERROR result;
			NT_PRINTER_INFO_LEVEL *printer = NULL;
			
			result = get_a_printer(NULL, &printer, 2, lp_const_servicename(snum));
			if (!W_ERROR_IS_OK(result))
				continue;
				
			if (printer && printer->info_2 && !strcmp(drivername, printer->info_2->drivername)) 
			{
				DEBUG(6,("Updating printer [%s]\n", printer->info_2->printername));
				
				/* all we care about currently is the change_id */
				
				result = mod_a_printer(printer, 2);
				if (!W_ERROR_IS_OK(result)) {
					DEBUG(3,("do_drv_upgrade_printer: mod_a_printer() failed with status [%s]\n", 
						dos_errstr(result)));
				}
			}
			
			free_a_printer(&printer, 2);			
		}
	}
	
	/* all done */	
}

/********************************************************************
 Update the cache for all printq's with a registered client 
 connection
 ********************************************************************/

void update_monitored_printq_cache( void )
{
	Printer_entry *printer = printers_list;
	int snum;
	
	/* loop through all printers and update the cache where 
	   client_connected == True */
	while ( printer ) 
	{
		if ( (printer->printer_type == SPLHND_PRINTER) 
			&& printer->notify.client_connected ) 
		{
			snum = print_queue_snum(printer->sharename);
			print_queue_status( snum, NULL, NULL );
		}
		
		printer = printer->next;
	}
	
	return;
}
/********************************************************************
 Send a message to ourself about new driver being installed
 so we can upgrade the information for each printer bound to this
 driver
 ********************************************************************/
 
static BOOL srv_spoolss_reset_printerdata(char* drivername)
{
	int len = strlen(drivername);
	
	if (!len)
		return False;

	DEBUG(10,("srv_spoolss_reset_printerdata: Sending message about resetting printerdata [%s]\n",
		drivername));
		
	message_send_pid(pid_to_procid(sys_getpid()),
			 MSG_PRINTERDATA_INIT_RESET, drivername, len+1, False);

	return True;
}

/**********************************************************************
 callback to receive a MSG_PRINTERDATA_INIT_RESET message and interate
 over all printers, resetting printer data as neessary 
 **********************************************************************/
 
void reset_all_printerdata(int msg_type, struct process_id src,
			   void *buf, size_t len)
{
	fstring drivername;
	int snum;
	int n_services = lp_numservices();
	
	len = MIN( len, sizeof(drivername)-1 );
	strncpy( drivername, buf, len );
	
	DEBUG(10,("reset_all_printerdata: Got message for new driver [%s]\n", drivername ));

	/* Iterate the printer list */
	
	for ( snum=0; snum<n_services; snum++ )
	{
		if ( lp_snum_ok(snum) && lp_print_ok(snum) ) 
		{
			WERROR result;
			NT_PRINTER_INFO_LEVEL *printer = NULL;
			
			result = get_a_printer( NULL, &printer, 2, lp_const_servicename(snum) );
			if ( !W_ERROR_IS_OK(result) )
				continue;
				
			/* 
			 * if the printer is bound to the driver, 
			 * then reset to the new driver initdata 
			 */
			
			if ( printer && printer->info_2 && !strcmp(drivername, printer->info_2->drivername) ) 
			{
				DEBUG(6,("reset_all_printerdata: Updating printer [%s]\n", printer->info_2->printername));
				
				if ( !set_driver_init(printer, 2) ) {
					DEBUG(5,("reset_all_printerdata: Error resetting printer data for printer [%s], driver [%s]!\n",
						printer->info_2->printername, printer->info_2->drivername));
				}	
				
				result = mod_a_printer( printer, 2 );
				if ( !W_ERROR_IS_OK(result) ) {
					DEBUG(3,("reset_all_printerdata: mod_a_printer() failed!  (%s)\n", 
						get_dos_error_msg(result)));
				}
			}
			
			free_a_printer( &printer, 2 );
		}
	}
	
	/* all done */	
	
	return;
}

/********************************************************************
 Copy routines used by convert_to_openprinterex()
 *******************************************************************/

static DEVICEMODE* dup_devicemode(TALLOC_CTX *ctx, DEVICEMODE *devmode)
{
	DEVICEMODE *d;
	int len;

	if (!devmode)
		return NULL;
		
	DEBUG (8,("dup_devmode\n"));
	
	/* bulk copy first */
	
	d = TALLOC_MEMDUP(ctx, devmode, sizeof(DEVICEMODE));
	if (!d)
		return NULL;
		
	/* dup the pointer members separately */
	
	len = unistrlen(devmode->devicename.buffer);
	if (len != -1) {
		d->devicename.buffer = TALLOC_ARRAY(ctx, uint16, len);
		if (!d->devicename.buffer) {
			return NULL;
		}
		if (unistrcpy(d->devicename.buffer, devmode->devicename.buffer) != len)
			return NULL;
	}
		

	len = unistrlen(devmode->formname.buffer);
	if (len != -1) {
		d->devicename.buffer = TALLOC_ARRAY(ctx, uint16, len);
		if (!d->devicename.buffer) {
			return NULL;
		}
		if (unistrcpy(d->formname.buffer, devmode->formname.buffer) != len)
			return NULL;
	}

	d->dev_private = TALLOC_MEMDUP(ctx, devmode->dev_private, devmode->driverextra);
	if (!d->dev_private) {
		return NULL;
	}	
	return d;
}

static void copy_devmode_ctr(TALLOC_CTX *ctx, DEVMODE_CTR *new_ctr, DEVMODE_CTR *ctr)
{
	if (!new_ctr || !ctr)
		return;
		
	DEBUG(8,("copy_devmode_ctr\n"));
	
	new_ctr->size = ctr->size;
	new_ctr->devmode_ptr = ctr->devmode_ptr;
	
	if(ctr->devmode_ptr)
		new_ctr->devmode = dup_devicemode(ctx, ctr->devmode);
}

static void copy_printer_default(TALLOC_CTX *ctx, PRINTER_DEFAULT *new_def, PRINTER_DEFAULT *def)
{
	if (!new_def || !def)
		return;
	
	DEBUG(8,("copy_printer_defaults\n"));
	
	new_def->datatype_ptr = def->datatype_ptr;
	
	if (def->datatype_ptr)
		copy_unistr2(&new_def->datatype, &def->datatype);
	
	copy_devmode_ctr(ctx, &new_def->devmode_cont, &def->devmode_cont);
	
	new_def->access_required = def->access_required;
}

/********************************************************************
 * Convert a SPOOL_Q_OPEN_PRINTER structure to a 
 * SPOOL_Q_OPEN_PRINTER_EX structure
 ********************************************************************/

static WERROR convert_to_openprinterex(TALLOC_CTX *ctx, SPOOL_Q_OPEN_PRINTER_EX *q_u_ex, SPOOL_Q_OPEN_PRINTER *q_u)
{
	if (!q_u_ex || !q_u)
		return WERR_OK;

	DEBUG(8,("convert_to_openprinterex\n"));
				
	if ( q_u->printername ) {
		q_u_ex->printername = TALLOC_ZERO_P( ctx, UNISTR2 );
		if (q_u_ex->printername == NULL)
			return WERR_NOMEM;
		copy_unistr2(q_u_ex->printername, q_u->printername);
	}
	
	copy_printer_default(ctx, &q_u_ex->printer_default, &q_u->printer_default);

	return WERR_OK;
}

/********************************************************************
 * spoolss_open_printer
 *
 * called from the spoolss dispatcher
 ********************************************************************/

WERROR _spoolss_open_printer(pipes_struct *p, SPOOL_Q_OPEN_PRINTER *q_u, SPOOL_R_OPEN_PRINTER *r_u)
{
	SPOOL_Q_OPEN_PRINTER_EX q_u_ex;
	SPOOL_R_OPEN_PRINTER_EX r_u_ex;
	
	if (!q_u || !r_u)
		return WERR_NOMEM;
	
	ZERO_STRUCT(q_u_ex);
	ZERO_STRUCT(r_u_ex);
	
	/* convert the OpenPrinter() call to OpenPrinterEx() */
	
	r_u_ex.status = convert_to_openprinterex(p->mem_ctx, &q_u_ex, q_u);
	if (!W_ERROR_IS_OK(r_u_ex.status))
		return r_u_ex.status;
	
	r_u_ex.status = _spoolss_open_printer_ex(p, &q_u_ex, &r_u_ex);
	
	/* convert back to OpenPrinter() */
	
	memcpy(r_u, &r_u_ex, sizeof(*r_u));
	
	return r_u->status;
}

/********************************************************************
 ********************************************************************/

WERROR _spoolss_open_printer_ex( pipes_struct *p, SPOOL_Q_OPEN_PRINTER_EX *q_u, SPOOL_R_OPEN_PRINTER_EX *r_u)
{
	PRINTER_DEFAULT 	*printer_default = &q_u->printer_default;
	POLICY_HND 		*handle = &r_u->handle;

	fstring name;
	int snum;
	struct current_user user;
	Printer_entry *Printer=NULL;

	if ( !q_u->printername )
		return WERR_INVALID_PRINTER_NAME;

	/* some sanity check because you can open a printer or a print server */
	/* aka: \\server\printer or \\server */

	unistr2_to_ascii(name, q_u->printername, sizeof(name)-1);

	DEBUGADD(3,("checking name: %s\n",name));

	if (!open_printer_hnd(p, handle, name, 0))
		return WERR_INVALID_PRINTER_NAME;
	
	Printer=find_printer_index_by_hnd(p, handle);
	if ( !Printer ) {
		DEBUG(0,(" _spoolss_open_printer_ex: logic error.  Can't find printer "
			"handle we created for printer %s\n", name ));
		close_printer_handle(p,handle);
		return WERR_INVALID_PRINTER_NAME;
	}

	get_current_user(&user, p);

	/*
	 * First case: the user is opening the print server:
	 *
	 * Disallow MS AddPrinterWizard if parameter disables it. A Win2k
	 * client 1st tries an OpenPrinterEx with access==0, MUST be allowed.
	 *
	 * Then both Win2k and WinNT clients try an OpenPrinterEx with
	 * SERVER_ALL_ACCESS, which we allow only if the user is root (uid=0)
	 * or if the user is listed in the smb.conf printer admin parameter.
	 *
	 * Then they try OpenPrinterEx with SERVER_READ which we allow. This lets the
	 * client view printer folder, but does not show the MSAPW.
	 *
	 * Note: this test needs code to check access rights here too. Jeremy
	 * could you look at this?
	 * 
	 * Second case: the user is opening a printer:
	 * NT doesn't let us connect to a printer if the connecting user
	 * doesn't have print permission.
	 * 
	 * Third case: user is opening a Port Monitor
	 * access checks same as opening a handle to the print server.
	 */

	switch (Printer->printer_type ) 
	{
	case SPLHND_SERVER:
	case SPLHND_PORTMON_TCP:
	case SPLHND_PORTMON_LOCAL:
		/* Printserver handles use global struct... */

		snum = -1;

		/* Map standard access rights to object specific access rights */
		
		se_map_standard(&printer_default->access_required, 
				&printserver_std_mapping);
	
		/* Deny any object specific bits that don't apply to print
		   servers (i.e printer and job specific bits) */

		printer_default->access_required &= SPECIFIC_RIGHTS_MASK;

		if (printer_default->access_required &
		    ~(SERVER_ACCESS_ADMINISTER | SERVER_ACCESS_ENUMERATE)) {
			DEBUG(3, ("access DENIED for non-printserver bits\n"));
			close_printer_handle(p, handle);
			return WERR_ACCESS_DENIED;
		}

		/* Allow admin access */

		if ( printer_default->access_required & SERVER_ACCESS_ADMINISTER ) 
		{
			SE_PRIV se_printop = SE_PRINT_OPERATOR;

			if (!lp_ms_add_printer_wizard()) {
				close_printer_handle(p, handle);
				return WERR_ACCESS_DENIED;
			}

			/* if the user is not root, doesn't have SE_PRINT_OPERATOR privilege,
			   and not a printer admin, then fail */
			
			if ((user.ut.uid != 0) &&
			    !user_has_privileges(user.nt_user_token,
						 &se_printop ) &&
			    !token_contains_name_in_list(
				    uidtoname(user.ut.uid), NULL,
				    user.nt_user_token,
				    lp_printer_admin(snum))) {
				close_printer_handle(p, handle);
				return WERR_ACCESS_DENIED;
			}
			
			printer_default->access_required = SERVER_ACCESS_ADMINISTER;
		}
		else
		{
			printer_default->access_required = SERVER_ACCESS_ENUMERATE;
		}

		DEBUG(4,("Setting print server access = %s\n", (printer_default->access_required == SERVER_ACCESS_ADMINISTER) 
			? "SERVER_ACCESS_ADMINISTER" : "SERVER_ACCESS_ENUMERATE" ));
			
		/* We fall through to return WERR_OK */
		break;

	case SPLHND_PRINTER:
		/* NT doesn't let us connect to a printer if the connecting user
		   doesn't have print permission.  */

		if (!get_printer_snum(p, handle, &snum)) {
			close_printer_handle(p, handle);
			return WERR_BADFID;
		}

		se_map_standard(&printer_default->access_required, &printer_std_mapping);
		
		/* map an empty access mask to the minimum access mask */
		if (printer_default->access_required == 0x0)
			printer_default->access_required = PRINTER_ACCESS_USE;

		/*
		 * If we are not serving the printer driver for this printer,
		 * map PRINTER_ACCESS_ADMINISTER to PRINTER_ACCESS_USE.  This
		 * will keep NT clients happy  --jerry	
		 */
		 
		if (lp_use_client_driver(snum) 
			&& (printer_default->access_required & PRINTER_ACCESS_ADMINISTER))
		{
			printer_default->access_required = PRINTER_ACCESS_USE;
		}

		/* check smb.conf parameters and the the sec_desc */
		
		if ( !check_access(smbd_server_fd(), lp_hostsallow(snum), lp_hostsdeny(snum)) ) {    
			DEBUG(3, ("access DENIED (hosts allow/deny) for printer open\n"));
			return WERR_ACCESS_DENIED;
		}

		if (!user_ok_token(uidtoname(user.ut.uid), user.nt_user_token,
				   snum) ||
		    !print_access_check(&user, snum,
					printer_default->access_required)) {
			DEBUG(3, ("access DENIED for printer open\n"));
			close_printer_handle(p, handle);
			return WERR_ACCESS_DENIED;
		}

		if ((printer_default->access_required & SPECIFIC_RIGHTS_MASK)& ~(PRINTER_ACCESS_ADMINISTER|PRINTER_ACCESS_USE)) {
			DEBUG(3, ("access DENIED for printer open - unknown bits\n"));
			close_printer_handle(p, handle);
			return WERR_ACCESS_DENIED;
		}

		if (printer_default->access_required & PRINTER_ACCESS_ADMINISTER)
			printer_default->access_required = PRINTER_ACCESS_ADMINISTER;
		else
			printer_default->access_required = PRINTER_ACCESS_USE;

		DEBUG(4,("Setting printer access = %s\n", (printer_default->access_required == PRINTER_ACCESS_ADMINISTER) 
			? "PRINTER_ACCESS_ADMINISTER" : "PRINTER_ACCESS_USE" ));

		break;

	default:
		/* sanity check to prevent programmer error */
		return WERR_BADFID;
	}
	
	Printer->access_granted = printer_default->access_required;
	
	/* 
	 * If the client sent a devmode in the OpenPrinter() call, then
	 * save it here in case we get a job submission on this handle
	 */
	
	 if ( (Printer->printer_type != SPLHND_SERVER)
	 	&& q_u->printer_default.devmode_cont.devmode_ptr )
	 { 
	 	convert_devicemode( Printer->sharename, q_u->printer_default.devmode_cont.devmode,
			&Printer->nt_devmode );
	 }

#if 0	/* JERRY -- I'm doubtful this is really effective */
	/* HACK ALERT!!! Sleep for 1/3 of a second to try trigger a LAN/WAN 
	   optimization in Windows 2000 clients  --jerry */

	if ( (printer_default->access_required == PRINTER_ACCESS_ADMINISTER) 
		&& (RA_WIN2K == get_remote_arch()) )
	{
		DEBUG(10,("_spoolss_open_printer_ex: Enabling LAN/WAN hack for Win2k clients.\n"));
		sys_usleep( 500000 );
	}
#endif

	return WERR_OK;
}

/****************************************************************************
****************************************************************************/

static BOOL convert_printer_info(const SPOOL_PRINTER_INFO_LEVEL *uni,
				NT_PRINTER_INFO_LEVEL *printer, uint32 level)
{
	BOOL ret;

	switch (level) {
		case 2:
			/* allocate memory if needed.  Messy because 
			   convert_printer_info is used to update an existing 
			   printer or build a new one */

			if ( !printer->info_2 ) {
				printer->info_2 = TALLOC_ZERO_P( printer, NT_PRINTER_INFO_LEVEL_2 );
				if ( !printer->info_2 ) {
					DEBUG(0,("convert_printer_info: talloc() failed!\n"));
					return False;
				}
			}

			ret = uni_2_asc_printer_info_2(uni->info_2, printer->info_2);
			printer->info_2->setuptime = time(NULL);

			return ret;
	}

	return False;
}

static BOOL convert_printer_driver_info(const SPOOL_PRINTER_DRIVER_INFO_LEVEL *uni,
                                 	NT_PRINTER_DRIVER_INFO_LEVEL *printer, uint32 level)
{
	BOOL result = True;

	switch (level) {
		case 3:
			printer->info_3=NULL;
			if (!uni_2_asc_printer_driver_3(uni->info_3, &printer->info_3))
				result = False;
			break;
		case 6:
			printer->info_6=NULL;
			if (!uni_2_asc_printer_driver_6(uni->info_6, &printer->info_6))
				result = False;
			break;
		default:
			break;
	}

	return result;
}

BOOL convert_devicemode(const char *printername, const DEVICEMODE *devmode,
				NT_DEVICEMODE **pp_nt_devmode)
{
	NT_DEVICEMODE *nt_devmode = *pp_nt_devmode;

	/*
	 * Ensure nt_devmode is a valid pointer
	 * as we will be overwriting it.
	 */
		
	if (nt_devmode == NULL) {
		DEBUG(5, ("convert_devicemode: allocating a generic devmode\n"));
		if ((nt_devmode = construct_nt_devicemode(printername)) == NULL)
			return False;
	}

	rpcstr_pull(nt_devmode->devicename,devmode->devicename.buffer, 31, -1, 0);
	rpcstr_pull(nt_devmode->formname,devmode->formname.buffer, 31, -1, 0);

	nt_devmode->specversion=devmode->specversion;
	nt_devmode->driverversion=devmode->driverversion;
	nt_devmode->size=devmode->size;
	nt_devmode->fields=devmode->fields;
	nt_devmode->orientation=devmode->orientation;
	nt_devmode->papersize=devmode->papersize;
	nt_devmode->paperlength=devmode->paperlength;
	nt_devmode->paperwidth=devmode->paperwidth;
	nt_devmode->scale=devmode->scale;
	nt_devmode->copies=devmode->copies;
	nt_devmode->defaultsource=devmode->defaultsource;
	nt_devmode->printquality=devmode->printquality;
	nt_devmode->color=devmode->color;
	nt_devmode->duplex=devmode->duplex;
	nt_devmode->yresolution=devmode->yresolution;
	nt_devmode->ttoption=devmode->ttoption;
	nt_devmode->collate=devmode->collate;

	nt_devmode->logpixels=devmode->logpixels;
	nt_devmode->bitsperpel=devmode->bitsperpel;
	nt_devmode->pelswidth=devmode->pelswidth;
	nt_devmode->pelsheight=devmode->pelsheight;
	nt_devmode->displayflags=devmode->displayflags;
	nt_devmode->displayfrequency=devmode->displayfrequency;
	nt_devmode->icmmethod=devmode->icmmethod;
	nt_devmode->icmintent=devmode->icmintent;
	nt_devmode->mediatype=devmode->mediatype;
	nt_devmode->dithertype=devmode->dithertype;
	nt_devmode->reserved1=devmode->reserved1;
	nt_devmode->reserved2=devmode->reserved2;
	nt_devmode->panningwidth=devmode->panningwidth;
	nt_devmode->panningheight=devmode->panningheight;

	/*
	 * Only change private and driverextra if the incoming devmode
	 * has a new one. JRA.
	 */

	if ((devmode->driverextra != 0) && (devmode->dev_private != NULL)) {
		SAFE_FREE(nt_devmode->nt_dev_private);
		nt_devmode->driverextra=devmode->driverextra;
		if((nt_devmode->nt_dev_private=SMB_MALLOC_ARRAY(uint8, nt_devmode->driverextra)) == NULL)
			return False;
		memcpy(nt_devmode->nt_dev_private, devmode->dev_private, nt_devmode->driverextra);
	}

	*pp_nt_devmode = nt_devmode;

	return True;
}

/********************************************************************
 * _spoolss_enddocprinter_internal.
 ********************************************************************/

static WERROR _spoolss_enddocprinter_internal(pipes_struct *p, POLICY_HND *handle)
{
	Printer_entry *Printer=find_printer_index_by_hnd(p, handle);
	int snum;

	if (!Printer) {
		DEBUG(2,("_spoolss_enddocprinter_internal: Invalid handle (%s:%u:%u)\n", OUR_HANDLE(handle)));
		return WERR_BADFID;
	}
	
	if (!get_printer_snum(p, handle, &snum))
		return WERR_BADFID;

	Printer->document_started=False;
	print_job_end(snum, Printer->jobid,NORMAL_CLOSE);
	/* error codes unhandled so far ... */

	return WERR_OK;
}

/********************************************************************
 * api_spoolss_closeprinter
 ********************************************************************/

WERROR _spoolss_closeprinter(pipes_struct *p, SPOOL_Q_CLOSEPRINTER *q_u, SPOOL_R_CLOSEPRINTER *r_u)
{
	POLICY_HND *handle = &q_u->handle;

	Printer_entry *Printer=find_printer_index_by_hnd(p, handle);

	if (Printer && Printer->document_started)
		_spoolss_enddocprinter_internal(p, handle);          /* print job was not closed */

	if (!close_printer_handle(p, handle))
		return WERR_BADFID;	
		
	/* clear the returned printer handle.  Observed behavior 
	   from Win2k server.  Don't think this really matters.
	   Previous code just copied the value of the closed
	   handle.    --jerry */

	memset(&r_u->handle, '\0', sizeof(r_u->handle));

	return WERR_OK;
}

/********************************************************************
 * api_spoolss_deleteprinter

 ********************************************************************/

WERROR _spoolss_deleteprinter(pipes_struct *p, SPOOL_Q_DELETEPRINTER *q_u, SPOOL_R_DELETEPRINTER *r_u)
{
	POLICY_HND *handle = &q_u->handle;
	Printer_entry *Printer=find_printer_index_by_hnd(p, handle);
	WERROR result;

	if (Printer && Printer->document_started)
		_spoolss_enddocprinter_internal(p, handle);  /* print job was not closed */

	memcpy(&r_u->handle, &q_u->handle, sizeof(r_u->handle));

	result = delete_printer_handle(p, handle);

	update_c_setprinter(False);

	return result;
}

/*******************************************************************
 * static function to lookup the version id corresponding to an
 * long architecture string
 ******************************************************************/

static int get_version_id (char * arch)
{
	int i;
	struct table_node archi_table[]= {
 
	        {"Windows 4.0",          "WIN40",       0 },
	        {"Windows NT x86",       "W32X86",      2 },
	        {"Windows NT R4000",     "W32MIPS",     2 },	
	        {"Windows NT Alpha_AXP", "W32ALPHA",    2 },
	        {"Windows NT PowerPC",   "W32PPC",      2 },
		{"Windows IA64",         "IA64",        3 },
		{"Windows x64",          "x64",         3 },
	        {NULL,                   "",            -1 }
	};
 
	for (i=0; archi_table[i].long_archi != NULL; i++)
	{
		if (strcmp(arch, archi_table[i].long_archi) == 0)
			return (archi_table[i].version);
        }
	
	return -1;
}

/********************************************************************
 * _spoolss_deleteprinterdriver
 ********************************************************************/

WERROR _spoolss_deleteprinterdriver(pipes_struct *p, SPOOL_Q_DELETEPRINTERDRIVER *q_u, SPOOL_R_DELETEPRINTERDRIVER *r_u)
{
	fstring				driver;
	fstring				arch;
	NT_PRINTER_DRIVER_INFO_LEVEL	info;
	NT_PRINTER_DRIVER_INFO_LEVEL	info_win2k;
	int				version;
	struct current_user		user;
	WERROR				status;
	WERROR				status_win2k = WERR_ACCESS_DENIED;
	SE_PRIV                         se_printop = SE_PRINT_OPERATOR;	
	
	get_current_user(&user, p);
	 
	/* if the user is not root, doesn't have SE_PRINT_OPERATOR privilege,
	   and not a printer admin, then fail */
			
	if ( (user.ut.uid != 0) 
		&& !user_has_privileges(user.nt_user_token, &se_printop ) 
		&& !token_contains_name_in_list( uidtoname(user.ut.uid), 
		    NULL, user.nt_user_token, lp_printer_admin(-1)) ) 
	{
		return WERR_ACCESS_DENIED;
	}

	unistr2_to_ascii(driver, &q_u->driver, sizeof(driver)-1 );
	unistr2_to_ascii(arch,   &q_u->arch,   sizeof(arch)-1   );
	
	/* check that we have a valid driver name first */
	
	if ((version=get_version_id(arch)) == -1) 
		return WERR_INVALID_ENVIRONMENT;
				
	ZERO_STRUCT(info);
	ZERO_STRUCT(info_win2k);
	
	if (!W_ERROR_IS_OK(get_a_printer_driver(&info, 3, driver, arch, version))) 
	{
		/* try for Win2k driver if "Windows NT x86" */
		
		if ( version == 2 ) {
			version = 3;
			if (!W_ERROR_IS_OK(get_a_printer_driver(&info, 3, driver, arch, version))) {
				status = WERR_UNKNOWN_PRINTER_DRIVER;
				goto done;
			}
		}
		/* otherwise it was a failure */
		else {
			status = WERR_UNKNOWN_PRINTER_DRIVER;
			goto done;
		}
		
	}
	
	if (printer_driver_in_use(info.info_3)) {
		status = WERR_PRINTER_DRIVER_IN_USE;
		goto done;
	}
	
	if ( version == 2 )
	{		
		if (W_ERROR_IS_OK(get_a_printer_driver(&info_win2k, 3, driver, arch, 3)))
		{
			/* if we get to here, we now have 2 driver info structures to remove */
			/* remove the Win2k driver first*/
		
			status_win2k = delete_printer_driver(info_win2k.info_3, &user, 3, False );
			free_a_printer_driver( info_win2k, 3 );
		
			/* this should not have failed---if it did, report to client */
			if ( !W_ERROR_IS_OK(status_win2k) )
			{
				status = status_win2k;
				goto done;
			}
		}
	}
	
	status = delete_printer_driver(info.info_3, &user, version, False);
	
	/* if at least one of the deletes succeeded return OK */
	
	if ( W_ERROR_IS_OK(status) || W_ERROR_IS_OK(status_win2k) )
		status = WERR_OK;
	
done:
	free_a_printer_driver( info, 3 );

	return status;
}

/********************************************************************
 * spoolss_deleteprinterdriverex
 ********************************************************************/

WERROR _spoolss_deleteprinterdriverex(pipes_struct *p, SPOOL_Q_DELETEPRINTERDRIVEREX *q_u, SPOOL_R_DELETEPRINTERDRIVEREX *r_u)
{
	fstring				driver;
	fstring				arch;
	NT_PRINTER_DRIVER_INFO_LEVEL	info;
	NT_PRINTER_DRIVER_INFO_LEVEL	info_win2k;
	int				version;
	uint32				flags = q_u->delete_flags;
	BOOL				delete_files;
	struct current_user		user;
	WERROR				status;
	WERROR				status_win2k = WERR_ACCESS_DENIED;
	SE_PRIV                         se_printop = SE_PRINT_OPERATOR;	
	
	get_current_user(&user, p);
	
	/* if the user is not root, doesn't have SE_PRINT_OPERATOR privilege,
	   and not a printer admin, then fail */
			
	if ( (user.ut.uid != 0) 
		&& !user_has_privileges(user.nt_user_token, &se_printop ) 
		&& !token_contains_name_in_list( uidtoname(user.ut.uid), 
		    NULL, user.nt_user_token, lp_printer_admin(-1)) ) 
	{
		return WERR_ACCESS_DENIED;
	}
	
	unistr2_to_ascii(driver, &q_u->driver, sizeof(driver)-1 );
	unistr2_to_ascii(arch,   &q_u->arch,   sizeof(arch)-1   );

	/* check that we have a valid driver name first */
	if ((version=get_version_id(arch)) == -1) {
		/* this is what NT returns */
		return WERR_INVALID_ENVIRONMENT;
	}
	
	if ( flags & DPD_DELETE_SPECIFIC_VERSION )
		version = q_u->version;
		
	ZERO_STRUCT(info);
	ZERO_STRUCT(info_win2k);
		
	status = get_a_printer_driver(&info, 3, driver, arch, version);
	
	if ( !W_ERROR_IS_OK(status) ) 
	{
		/* 
		 * if the client asked for a specific version, 
		 * or this is something other than Windows NT x86,
		 * then we've failed 
		 */
		
		if ( (flags&DPD_DELETE_SPECIFIC_VERSION) || (version !=2) )
			goto done;
			
		/* try for Win2k driver if "Windows NT x86" */
		
		version = 3;
		if (!W_ERROR_IS_OK(get_a_printer_driver(&info, 3, driver, arch, version))) {
			status = WERR_UNKNOWN_PRINTER_DRIVER;
			goto done;
		}
	}
		
	if ( printer_driver_in_use(info.info_3) ) {
		status = WERR_PRINTER_DRIVER_IN_USE;
		goto done;
	}
	
	/* 
	 * we have a couple of cases to consider. 
	 * (1) Are any files in use?  If so and DPD_DELTE_ALL_FILE is set,
	 *     then the delete should fail if **any** files overlap with 
	 *     other drivers 
	 * (2) If DPD_DELTE_UNUSED_FILES is sert, then delete all
	 *     non-overlapping files 
	 * (3) If neither DPD_DELTE_ALL_FILE nor DPD_DELTE_ALL_FILES
	 *     is set, the do not delete any files
	 * Refer to MSDN docs on DeletePrinterDriverEx() for details.
	 */
	
	delete_files = flags & (DPD_DELETE_ALL_FILES|DPD_DELETE_UNUSED_FILES);
	
	/* fail if any files are in use and DPD_DELETE_ALL_FILES is set */
		
	if ( delete_files && printer_driver_files_in_use(info.info_3) & (flags&DPD_DELETE_ALL_FILES) ) {
		/* no idea of the correct error here */
		status = WERR_ACCESS_DENIED;	
		goto done;
	}

			
	/* also check for W32X86/3 if necessary; maybe we already have? */
		
	if ( (version == 2) && ((flags&DPD_DELETE_SPECIFIC_VERSION) != DPD_DELETE_SPECIFIC_VERSION)  ) {
		if (W_ERROR_IS_OK(get_a_printer_driver(&info_win2k, 3, driver, arch, 3))) 
		{
			
			if ( delete_files && printer_driver_files_in_use(info_win2k.info_3) & (flags&DPD_DELETE_ALL_FILES) ) {
				/* no idea of the correct error here */
				free_a_printer_driver( info_win2k, 3 );
				status = WERR_ACCESS_DENIED;	
				goto done;
			}
		
			/* if we get to here, we now have 2 driver info structures to remove */
			/* remove the Win2k driver first*/
		
			status_win2k = delete_printer_driver(info_win2k.info_3, &user, 3, delete_files);
			free_a_printer_driver( info_win2k, 3 );
				
			/* this should not have failed---if it did, report to client */
				
			if ( !W_ERROR_IS_OK(status_win2k) )
				goto done;
		}
	}

	status = delete_printer_driver(info.info_3, &user, version, delete_files);

	if ( W_ERROR_IS_OK(status) || W_ERROR_IS_OK(status_win2k) )
		status = WERR_OK;
done:
	free_a_printer_driver( info, 3 );
	
	return status;
}


/****************************************************************************
 Internal routine for retreiving printerdata
 ***************************************************************************/

static WERROR get_printer_dataex( TALLOC_CTX *ctx, NT_PRINTER_INFO_LEVEL *printer, 
                                  const char *key, const char *value, uint32 *type, uint8 **data, 
				  uint32 *needed, uint32 in_size  )
{
	REGISTRY_VALUE 		*val;
	uint32			size;
	int			data_len;
	
	if ( !(val = get_printer_data( printer->info_2, key, value)) )
		return WERR_BADFILE;
	
	*type = regval_type( val );

	DEBUG(5,("get_printer_dataex: allocating %d\n", in_size));

	size = regval_size( val );
	
	/* copy the min(in_size, len) */
	
	if ( in_size ) {
		data_len = (size > in_size) ? in_size : size*sizeof(uint8);
		
		/* special case for 0 length values */
		if ( data_len ) {
			if ( (*data  = (uint8 *)TALLOC_MEMDUP(ctx, regval_data_p(val), data_len)) == NULL )
				return WERR_NOMEM;
		}
		else {
			if ( (*data  = (uint8 *)TALLOC_ZERO(ctx, in_size)) == NULL )
				return WERR_NOMEM;
		}
	}
	else
		*data = NULL;

	*needed = size;
	
	DEBUG(5,("get_printer_dataex: copy done\n"));

	return WERR_OK;
}

/****************************************************************************
 Internal routine for removing printerdata
 ***************************************************************************/

static WERROR delete_printer_dataex( NT_PRINTER_INFO_LEVEL *printer, const char *key, const char *value )
{
	return delete_printer_data( printer->info_2, key, value );
}

/****************************************************************************
 Internal routine for storing printerdata
 ***************************************************************************/

WERROR set_printer_dataex( NT_PRINTER_INFO_LEVEL *printer, const char *key, const char *value, 
                                  uint32 type, uint8 *data, int real_len  )
{
	/* the registry objects enforce uniqueness based on value name */

	return add_printer_data( printer->info_2, key, value, type, data, real_len );
}

/********************************************************************
 GetPrinterData on a printer server Handle.
********************************************************************/

static WERROR getprinterdata_printer_server(TALLOC_CTX *ctx, fstring value, uint32 *type, uint8 **data, uint32 *needed, uint32 in_size)
{		
	int i;
	
	DEBUG(8,("getprinterdata_printer_server:%s\n", value));
		
	if (!StrCaseCmp(value, "W3SvcInstalled")) {
		*type = REG_DWORD;
		if ( !(*data = TALLOC_ARRAY(ctx, uint8, sizeof(uint32) )) )
			return WERR_NOMEM;
		*needed = 0x4;
		return WERR_OK;
	}

	if (!StrCaseCmp(value, "BeepEnabled")) {
		*type = REG_DWORD;
		if ( !(*data = TALLOC_ARRAY(ctx, uint8, sizeof(uint32) )) )
			return WERR_NOMEM;
		SIVAL(*data, 0, 0x00);
		*needed = 0x4;			
		return WERR_OK;
	}

	if (!StrCaseCmp(value, "EventLog")) {
		*type = REG_DWORD;
		if ( !(*data = TALLOC_ARRAY(ctx, uint8, sizeof(uint32) )) )
			return WERR_NOMEM;
		/* formally was 0x1b */
		SIVAL(*data, 0, 0x0);
		*needed = 0x4;			
		return WERR_OK;
	}

	if (!StrCaseCmp(value, "NetPopup")) {
		*type = REG_DWORD;
		if ( !(*data = TALLOC_ARRAY(ctx, uint8, sizeof(uint32) )) )
			return WERR_NOMEM;
		SIVAL(*data, 0, 0x00);
		*needed = 0x4;
		return WERR_OK;
	}

	if (!StrCaseCmp(value, "MajorVersion")) {
		*type = REG_DWORD;
		if ( !(*data = TALLOC_ARRAY(ctx, uint8, sizeof(uint32) )) )
			return WERR_NOMEM;

		/* Windows NT 4.0 seems to not allow uploading of drivers
		   to a server that reports 0x3 as the MajorVersion.
		   need to investigate more how Win2k gets around this .
		   -- jerry */

		if ( RA_WINNT == get_remote_arch() )
			SIVAL(*data, 0, 2);
		else
			SIVAL(*data, 0, 3);
		
		*needed = 0x4;
		return WERR_OK;
	}

	if (!StrCaseCmp(value, "MinorVersion")) {
		*type = REG_DWORD;
		if ( !(*data = TALLOC_ARRAY(ctx, uint8, sizeof(uint32) )) )
			return WERR_NOMEM;
		SIVAL(*data, 0, 0);
		*needed = 0x4;
		return WERR_OK;
	}

	/* REG_BINARY
	 *  uint32 size	 	 = 0x114
	 *  uint32 major	 = 5
	 *  uint32 minor	 = [0|1]
	 *  uint32 build 	 = [2195|2600]
	 *  extra unicode string = e.g. "Service Pack 3"
	 */
	if (!StrCaseCmp(value, "OSVersion")) {
		*type = REG_BINARY;
		*needed = 0x114;

		if ( !(*data = TALLOC_ZERO_ARRAY(ctx, uint8, (*needed > in_size) ? *needed:in_size )) )
			return WERR_NOMEM;

		SIVAL(*data, 0, *needed);	/* size */
		SIVAL(*data, 4, 5);		/* Windows 2000 == 5.0 */
		SIVAL(*data, 8, 0);
		SIVAL(*data, 12, 2195);		/* build */
		
		/* leave extra string empty */
		
		return WERR_OK;
	}


   	if (!StrCaseCmp(value, "DefaultSpoolDirectory")) {
		const char *string="C:\\PRINTERS";
		*type = REG_SZ;
		*needed = 2*(strlen(string)+1);		
		if((*data  = (uint8 *)TALLOC(ctx, (*needed > in_size) ? *needed:in_size )) == NULL)
			return WERR_NOMEM;
		memset(*data, 0, (*needed > in_size) ? *needed:in_size);
		
		/* it's done by hand ready to go on the wire */
		for (i=0; i<strlen(string); i++) {
			(*data)[2*i]=string[i];
			(*data)[2*i+1]='\0';
		}			
		return WERR_OK;
	}

	if (!StrCaseCmp(value, "Architecture")) {			
		const char *string="Windows NT x86";
		*type = REG_SZ;
		*needed = 2*(strlen(string)+1);	
		if((*data  = (uint8 *)TALLOC(ctx, (*needed > in_size) ? *needed:in_size )) == NULL)
			return WERR_NOMEM;
		memset(*data, 0, (*needed > in_size) ? *needed:in_size);
		for (i=0; i<strlen(string); i++) {
			(*data)[2*i]=string[i];
			(*data)[2*i+1]='\0';
		}			
		return WERR_OK;
	}

	if (!StrCaseCmp(value, "DsPresent")) {
		*type = REG_DWORD;
		if ( !(*data = TALLOC_ARRAY(ctx, uint8, sizeof(uint32) )) )
			return WERR_NOMEM;

		/* only show the publish check box if we are a 
		   memeber of a AD domain */

		if ( lp_security() == SEC_ADS )
			SIVAL(*data, 0, 0x01);
		else
			SIVAL(*data, 0, 0x00);

		*needed = 0x4;
		return WERR_OK;
	}

	if (!StrCaseCmp(value, "DNSMachineName")) {			
		pstring hostname;
		
		if (!get_mydnsfullname(hostname))
			return WERR_BADFILE;
		*type = REG_SZ;
		*needed = 2*(strlen(hostname)+1);	
		if((*data  = (uint8 *)TALLOC(ctx, (*needed > in_size) ? *needed:in_size )) == NULL)
			return WERR_NOMEM;
		memset(*data, 0, (*needed > in_size) ? *needed:in_size);
		for (i=0; i<strlen(hostname); i++) {
			(*data)[2*i]=hostname[i];
			(*data)[2*i+1]='\0';
		}			
		return WERR_OK;
	}


	return WERR_BADFILE;
}

/********************************************************************
 * spoolss_getprinterdata
 ********************************************************************/

WERROR _spoolss_getprinterdata(pipes_struct *p, SPOOL_Q_GETPRINTERDATA *q_u, SPOOL_R_GETPRINTERDATA *r_u)
{
	POLICY_HND 	*handle = &q_u->handle;
	UNISTR2 	*valuename = &q_u->valuename;
	uint32 		in_size = q_u->size;
	uint32 		*type = &r_u->type;
	uint32 		*out_size = &r_u->size;
	uint8 		**data = &r_u->data;
	uint32 		*needed = &r_u->needed;
	WERROR 		status;
	fstring 	value;
	Printer_entry 	*Printer = find_printer_index_by_hnd(p, handle);
	NT_PRINTER_INFO_LEVEL 	*printer = NULL;
	int		snum = 0;
	
	/*
	 * Reminder: when it's a string, the length is in BYTES
	 * even if UNICODE is negociated.
	 *
	 * JFM, 4/19/1999
	 */

	*out_size = in_size;

	/* in case of problem, return some default values */
	
	*needed = 0;
	*type   = 0;
	
	DEBUG(4,("_spoolss_getprinterdata\n"));
	
	if ( !Printer ) {
		DEBUG(2,("_spoolss_getprinterdata: Invalid handle (%s:%u:%u).\n", OUR_HANDLE(handle)));
		status = WERR_BADFID;
		goto done;
	}
	
	unistr2_to_ascii(value, valuename, sizeof(value)-1);
	
	if ( Printer->printer_type == SPLHND_SERVER )
		status = getprinterdata_printer_server( p->mem_ctx, value, type, data, needed, *out_size );
	else
	{
		if ( !get_printer_snum(p,handle, &snum) ) {
			status = WERR_BADFID;
			goto done;
		}

		status = get_a_printer(Printer, &printer, 2, lp_servicename(snum));
		if ( !W_ERROR_IS_OK(status) )
			goto done;

		/* XP sends this and wants to change id value from the PRINTER_INFO_0 */

		if ( strequal(value, "ChangeId") ) {
			*type = REG_DWORD;
			*needed = sizeof(uint32);
			if ( (*data = (uint8*)TALLOC(p->mem_ctx, sizeof(uint32))) == NULL) {
				status = WERR_NOMEM;
				goto done;
			}
			SIVAL( *data, 0, printer->info_2->changeid );
			status = WERR_OK;
		}
		else
			status = get_printer_dataex( p->mem_ctx, printer, SPOOL_PRINTERDATA_KEY, value, type, data, needed, *out_size );
	}

	if (*needed > *out_size)
		status = WERR_MORE_DATA;
	
done:
	if ( !W_ERROR_IS_OK(status) ) 
	{
		DEBUG(5, ("error %d: allocating %d\n", W_ERROR_V(status),*out_size));
		
		/* reply this param doesn't exist */
		
		if ( *out_size ) {
			if((*data=(uint8 *)TALLOC_ZERO_ARRAY(p->mem_ctx, uint8, *out_size)) == NULL) {
				if ( printer ) 
					free_a_printer( &printer, 2 );
				return WERR_NOMEM;
		} 
		} 
		else {
			*data = NULL;
		}
	}
	
	/* cleanup & exit */

	if ( printer )
		free_a_printer( &printer, 2 );
	
	return status;
}

/*********************************************************
 Connect to the client machine.
**********************************************************/

static BOOL spoolss_connect_to_client(struct rpc_pipe_client **pp_pipe,
			struct in_addr *client_ip, const char *remote_machine)
{
	NTSTATUS ret;
	struct cli_state *the_cli;
	struct in_addr rm_addr;

	if ( is_zero_ip(*client_ip) ) {
		if ( !resolve_name( remote_machine, &rm_addr, 0x20) ) {
			DEBUG(2,("spoolss_connect_to_client: Can't resolve address for %s\n", remote_machine));
			return False;
		}

		if ( ismyip( rm_addr )) {
			DEBUG(0,("spoolss_connect_to_client: Machine %s is one of our addresses. Cannot add to ourselves.\n", remote_machine));
			return False;
		}
	} else {
		rm_addr.s_addr = client_ip->s_addr;
		DEBUG(5,("spoolss_connect_to_client: Using address %s (no name resolution necessary)\n",
			inet_ntoa(*client_ip) ));
	}

	/* setup the connection */

	ret = cli_full_connection( &the_cli, global_myname(), remote_machine, 
		&rm_addr, 0, "IPC$", "IPC",
		"", /* username */
		"", /* domain */
		"", /* password */
		0, lp_client_signing(), NULL );

	if ( !NT_STATUS_IS_OK( ret ) ) {
		DEBUG(2,("spoolss_connect_to_client: connection to [%s] failed!\n", 
			remote_machine ));
		return False;
	}	
		
	if ( the_cli->protocol != PROTOCOL_NT1 ) {
		DEBUG(0,("spoolss_connect_to_client: machine %s didn't negotiate NT protocol.\n", remote_machine));
		cli_shutdown(the_cli);
		return False;
	}
    
	/*
	 * Ok - we have an anonymous connection to the IPC$ share.
	 * Now start the NT Domain stuff :-).
	 */

	if ( !(*pp_pipe = cli_rpc_pipe_open_noauth(the_cli, PI_SPOOLSS, &ret)) ) {
		DEBUG(2,("spoolss_connect_to_client: unable to open the spoolss pipe on machine %s. Error was : %s.\n",
			remote_machine, nt_errstr(ret)));
		cli_shutdown(the_cli);
		return False;
	} 

	/* make sure to save the cli_state pointer.  Keep its own talloc_ctx */

	(*pp_pipe)->cli = the_cli;

	return True;
}

/***************************************************************************
 Connect to the client.
****************************************************************************/

static BOOL srv_spoolss_replyopenprinter(int snum, const char *printer, 
					uint32 localprinter, uint32 type, 
					POLICY_HND *handle, struct in_addr *client_ip)
{
	WERROR result;

	/*
	 * If it's the first connection, contact the client
	 * and connect to the IPC$ share anonymously
	 */
	if (smb_connections==0) {
		fstring unix_printer;

		fstrcpy(unix_printer, printer+2); /* the +2 is to strip the leading 2 backslashs */

		if ( !spoolss_connect_to_client( &notify_cli_pipe, client_ip, unix_printer ))
			return False;
			
		message_register(MSG_PRINTER_NOTIFY2, receive_notify2_message_list);
		/* Tell the connections db we're now interested in printer
		 * notify messages. */
		register_message_flags( True, FLAG_MSG_PRINT_NOTIFY );
	}

	/* 
	 * Tell the specific printing tdb we want messages for this printer
	 * by registering our PID.
	 */

	if (!print_notify_register_pid(snum))
		DEBUG(0,("print_notify_register_pid: Failed to register our pid for printer %s\n", printer ));

	smb_connections++;

	result = rpccli_spoolss_reply_open_printer(notify_cli_pipe, notify_cli_pipe->cli->mem_ctx, printer, localprinter, 
			type, handle);
			
	if (!W_ERROR_IS_OK(result))
		DEBUG(5,("srv_spoolss_reply_open_printer: Client RPC returned [%s]\n",
			dos_errstr(result)));

	return (W_ERROR_IS_OK(result));	
}

/********************************************************************
 * _spoolss_rffpcnex
 * ReplyFindFirstPrinterChangeNotifyEx
 *
 * before replying OK: status=0 a rpc call is made to the workstation
 * asking ReplyOpenPrinter 
 *
 * in fact ReplyOpenPrinter is the changenotify equivalent on the spoolss pipe
 * called from api_spoolss_rffpcnex
 ********************************************************************/

WERROR _spoolss_rffpcnex(pipes_struct *p, SPOOL_Q_RFFPCNEX *q_u, SPOOL_R_RFFPCNEX *r_u)
{
	POLICY_HND *handle = &q_u->handle;
	uint32 flags = q_u->flags;
	uint32 options = q_u->options;
	UNISTR2 *localmachine = &q_u->localmachine;
	uint32 printerlocal = q_u->printerlocal;
	int snum = -1;
	SPOOL_NOTIFY_OPTION *option = q_u->option;
	struct in_addr client_ip;

	/* store the notify value in the printer struct */

	Printer_entry *Printer=find_printer_index_by_hnd(p, handle);

	if (!Printer) {
		DEBUG(2,("_spoolss_rffpcnex: Invalid handle (%s:%u:%u).\n", OUR_HANDLE(handle)));
		return WERR_BADFID;
	}

	Printer->notify.flags=flags;
	Printer->notify.options=options;
	Printer->notify.printerlocal=printerlocal;

	if (Printer->notify.option)
		free_spool_notify_option(&Printer->notify.option);

	Printer->notify.option=dup_spool_notify_option(option);

	unistr2_to_ascii(Printer->notify.localmachine, localmachine, 
		       sizeof(Printer->notify.localmachine)-1);

	/* Connect to the client machine and send a ReplyOpenPrinter */

	if ( Printer->printer_type == SPLHND_SERVER)
		snum = -1;
	else if ( (Printer->printer_type == SPLHND_PRINTER) &&
			!get_printer_snum(p, handle, &snum) )
		return WERR_BADFID;
		
	client_ip.s_addr = inet_addr(p->conn->client_address);

	if(!srv_spoolss_replyopenprinter(snum, Printer->notify.localmachine,
					Printer->notify.printerlocal, 1,
					&Printer->notify.client_hnd, &client_ip))
		return WERR_SERVER_UNAVAILABLE;

	Printer->notify.client_connected=True;

	return WERR_OK;
}

/*******************************************************************
 * fill a notify_info_data with the servername
 ********************************************************************/

void spoolss_notify_server_name(int snum, 
				       SPOOL_NOTIFY_INFO_DATA *data, 
				       print_queue_struct *queue,
				       NT_PRINTER_INFO_LEVEL *printer,
				       TALLOC_CTX *mem_ctx) 
{
	pstring temp;
	uint32 len;

	len = rpcstr_push(temp, printer->info_2->servername, sizeof(temp)-2, STR_TERMINATE);

	data->notify_data.data.length = len;
	data->notify_data.data.string = (uint16 *)TALLOC(mem_ctx, len);

	if (!data->notify_data.data.string) {
		data->notify_data.data.length = 0;
		return;
	}
	
	memcpy(data->notify_data.data.string, temp, len);
}

/*******************************************************************
 * fill a notify_info_data with the printername (not including the servername).
 ********************************************************************/

void spoolss_notify_printer_name(int snum, 
					SPOOL_NOTIFY_INFO_DATA *data, 
					print_queue_struct *queue,
					NT_PRINTER_INFO_LEVEL *printer,
					TALLOC_CTX *mem_ctx)
{
	pstring temp;
	uint32 len;
		
	/* the notify name should not contain the \\server\ part */
	char *p = strrchr(printer->info_2->printername, '\\');

	if (!p) {
		p = printer->info_2->printername;
	} else {
		p++;
	}

	len = rpcstr_push(temp, p, sizeof(temp)-2, STR_TERMINATE);

	data->notify_data.data.length = len;
	data->notify_data.data.string = (uint16 *)TALLOC(mem_ctx, len);
	
	if (!data->notify_data.data.string) {
		data->notify_data.data.length = 0;
		return;
	}
	
	memcpy(data->notify_data.data.string, temp, len);
}

/*******************************************************************
 * fill a notify_info_data with the servicename
 ********************************************************************/

void spoolss_notify_share_name(int snum, 
				      SPOOL_NOTIFY_INFO_DATA *data, 
				      print_queue_struct *queue,
				      NT_PRINTER_INFO_LEVEL *printer,
				      TALLOC_CTX *mem_ctx)
{
	pstring temp;
	uint32 len;

	len = rpcstr_push(temp, lp_servicename(snum), sizeof(temp)-2, STR_TERMINATE);

	data->notify_data.data.length = len;
	data->notify_data.data.string = (uint16 *)TALLOC(mem_ctx, len);
	
	if (!data->notify_data.data.string) {
		data->notify_data.data.length = 0;
		return;
	}
	
	memcpy(data->notify_data.data.string, temp, len);
}

/*******************************************************************
 * fill a notify_info_data with the port name
 ********************************************************************/

void spoolss_notify_port_name(int snum, 
				     SPOOL_NOTIFY_INFO_DATA *data, 
				     print_queue_struct *queue,
				     NT_PRINTER_INFO_LEVEL *printer,
				     TALLOC_CTX *mem_ctx)
{
	pstring temp;
	uint32 len;

	/* even if it's strange, that's consistant in all the code */

	len = rpcstr_push(temp, printer->info_2->portname, sizeof(temp)-2, STR_TERMINATE);

	data->notify_data.data.length = len;
	data->notify_data.data.string = (uint16 *)TALLOC(mem_ctx, len);
	
	if (!data->notify_data.data.string) {
		data->notify_data.data.length = 0;
		return;
	}
	
	memcpy(data->notify_data.data.string, temp, len);
}

/*******************************************************************
 * fill a notify_info_data with the printername
 * but it doesn't exist, have to see what to do
 ********************************************************************/

void spoolss_notify_driver_name(int snum, 
				       SPOOL_NOTIFY_INFO_DATA *data,
				       print_queue_struct *queue,
				       NT_PRINTER_INFO_LEVEL *printer,
				       TALLOC_CTX *mem_ctx)
{
	pstring temp;
	uint32 len;

	len = rpcstr_push(temp, printer->info_2->drivername, sizeof(temp)-2, STR_TERMINATE);

	data->notify_data.data.length = len;
	data->notify_data.data.string = (uint16 *)TALLOC(mem_ctx, len);
	
	if (!data->notify_data.data.string) {
		data->notify_data.data.length = 0;
		return;
	}
	
	memcpy(data->notify_data.data.string, temp, len);
}

/*******************************************************************
 * fill a notify_info_data with the comment
 ********************************************************************/

void spoolss_notify_comment(int snum, 
				   SPOOL_NOTIFY_INFO_DATA *data,
				   print_queue_struct *queue,
				   NT_PRINTER_INFO_LEVEL *printer,
				   TALLOC_CTX *mem_ctx)
{
	pstring temp;
	uint32 len;

	if (*printer->info_2->comment == '\0')
		len = rpcstr_push(temp, lp_comment(snum), sizeof(temp)-2, STR_TERMINATE);
	else
		len = rpcstr_push(temp, printer->info_2->comment, sizeof(temp)-2, STR_TERMINATE);

	data->notify_data.data.length = len;
	data->notify_data.data.string = (uint16 *)TALLOC(mem_ctx, len);
	
	if (!data->notify_data.data.string) {
		data->notify_data.data.length = 0;
		return;
	}
	
	memcpy(data->notify_data.data.string, temp, len);
}

/*******************************************************************
 * fill a notify_info_data with the comment
 * location = "Room 1, floor 2, building 3"
 ********************************************************************/

void spoolss_notify_location(int snum, 
				    SPOOL_NOTIFY_INFO_DATA *data,
				    print_queue_struct *queue,
				    NT_PRINTER_INFO_LEVEL *printer,
				    TALLOC_CTX *mem_ctx)
{
	pstring temp;
	uint32 len;

	len = rpcstr_push(temp, printer->info_2->location,sizeof(temp)-2, STR_TERMINATE);

	data->notify_data.data.length = len;
	data->notify_data.data.string = (uint16 *)TALLOC(mem_ctx, len);
	
	if (!data->notify_data.data.string) {
		data->notify_data.data.length = 0;
		return;
	}
	
	memcpy(data->notify_data.data.string, temp, len);
}

/*******************************************************************
 * fill a notify_info_data with the device mode
 * jfm:xxxx don't to it for know but that's a real problem !!!
 ********************************************************************/

static void spoolss_notify_devmode(int snum, 
				   SPOOL_NOTIFY_INFO_DATA *data,
				   print_queue_struct *queue,
				   NT_PRINTER_INFO_LEVEL *printer,
				   TALLOC_CTX *mem_ctx)
{
	/* for a dummy implementation we have to zero the fields */
	data->notify_data.data.length = 0;
	data->notify_data.data.string = NULL;
}

/*******************************************************************
 * fill a notify_info_data with the separator file name
 ********************************************************************/

void spoolss_notify_sepfile(int snum, 
				   SPOOL_NOTIFY_INFO_DATA *data, 
				   print_queue_struct *queue,
				   NT_PRINTER_INFO_LEVEL *printer,
				   TALLOC_CTX *mem_ctx)
{
	pstring temp;
	uint32 len;

	len = rpcstr_push(temp, printer->info_2->sepfile, sizeof(temp)-2, STR_TERMINATE);

	data->notify_data.data.length = len;
	data->notify_data.data.string = (uint16 *)TALLOC(mem_ctx, len);
	
	if (!data->notify_data.data.string) {
		data->notify_data.data.length = 0;
		return;
	}
	
	memcpy(data->notify_data.data.string, temp, len);
}

/*******************************************************************
 * fill a notify_info_data with the print processor
 * jfm:xxxx return always winprint to indicate we don't do anything to it
 ********************************************************************/

void spoolss_notify_print_processor(int snum, 
					   SPOOL_NOTIFY_INFO_DATA *data,
					   print_queue_struct *queue,
					   NT_PRINTER_INFO_LEVEL *printer,
					   TALLOC_CTX *mem_ctx)
{
	pstring temp;
	uint32 len;

	len = rpcstr_push(temp,  printer->info_2->printprocessor, sizeof(temp)-2, STR_TERMINATE);

	data->notify_data.data.length = len;
	data->notify_data.data.string = (uint16 *)TALLOC(mem_ctx, len);
	
	if (!data->notify_data.data.string) {
		data->notify_data.data.length = 0;
		return;
	}
	
	memcpy(data->notify_data.data.string, temp, len);
}

/*******************************************************************
 * fill a notify_info_data with the print processor options
 * jfm:xxxx send an empty string
 ********************************************************************/

void spoolss_notify_parameters(int snum, 
				      SPOOL_NOTIFY_INFO_DATA *data,
				      print_queue_struct *queue,
				      NT_PRINTER_INFO_LEVEL *printer,
				      TALLOC_CTX *mem_ctx)
{
	pstring temp;
	uint32 len;

	len = rpcstr_push(temp,  printer->info_2->parameters, sizeof(temp)-2, STR_TERMINATE);

	data->notify_data.data.length = len;
	data->notify_data.data.string = (uint16 *)TALLOC(mem_ctx, len);
	
	if (!data->notify_data.data.string) {
		data->notify_data.data.length = 0;
		return;
	}
	
	memcpy(data->notify_data.data.string, temp, len);
}

/*******************************************************************
 * fill a notify_info_data with the data type
 * jfm:xxxx always send RAW as data type
 ********************************************************************/

void spoolss_notify_datatype(int snum, 
				    SPOOL_NOTIFY_INFO_DATA *data,
				    print_queue_struct *queue,
				    NT_PRINTER_INFO_LEVEL *printer,
				    TALLOC_CTX *mem_ctx)
{
	pstring temp;
	uint32 len;

	len = rpcstr_push(temp, printer->info_2->datatype, sizeof(pstring)-2, STR_TERMINATE);

	data->notify_data.data.length = len;
	data->notify_data.data.string = (uint16 *)TALLOC(mem_ctx, len);
	
	if (!data->notify_data.data.string) {
		data->notify_data.data.length = 0;
		return;
	}
	
	memcpy(data->notify_data.data.string, temp, len);
}

/*******************************************************************
 * fill a notify_info_data with the security descriptor
 * jfm:xxxx send an null pointer to say no security desc
 * have to implement security before !
 ********************************************************************/

static void spoolss_notify_security_desc(int snum, 
					 SPOOL_NOTIFY_INFO_DATA *data,
					 print_queue_struct *queue,
					 NT_PRINTER_INFO_LEVEL *printer,
					 TALLOC_CTX *mem_ctx)
{
	data->notify_data.sd.size = printer->info_2->secdesc_buf->len;
	data->notify_data.sd.desc = dup_sec_desc( mem_ctx, printer->info_2->secdesc_buf->sec ) ;
}

/*******************************************************************
 * fill a notify_info_data with the attributes
 * jfm:xxxx a samba printer is always shared
 ********************************************************************/

void spoolss_notify_attributes(int snum, 
				      SPOOL_NOTIFY_INFO_DATA *data,
				      print_queue_struct *queue,
				      NT_PRINTER_INFO_LEVEL *printer,
				      TALLOC_CTX *mem_ctx)
{
	data->notify_data.value[0] = printer->info_2->attributes;
	data->notify_data.value[1] = 0;
}

/*******************************************************************
 * fill a notify_info_data with the priority
 ********************************************************************/

static void spoolss_notify_priority(int snum, 
				    SPOOL_NOTIFY_INFO_DATA *data,
				    print_queue_struct *queue,
				    NT_PRINTER_INFO_LEVEL *printer,
				    TALLOC_CTX *mem_ctx)
{
	data->notify_data.value[0] = printer->info_2->priority;
	data->notify_data.value[1] = 0;
}

/*******************************************************************
 * fill a notify_info_data with the default priority
 ********************************************************************/

static void spoolss_notify_default_priority(int snum, 
					    SPOOL_NOTIFY_INFO_DATA *data,
					    print_queue_struct *queue,
					    NT_PRINTER_INFO_LEVEL *printer,
					    TALLOC_CTX *mem_ctx)
{
	data->notify_data.value[0] = printer->info_2->default_priority;
	data->notify_data.value[1] = 0;
}

/*******************************************************************
 * fill a notify_info_data with the start time
 ********************************************************************/

static void spoolss_notify_start_time(int snum, 
				      SPOOL_NOTIFY_INFO_DATA *data,
				      print_queue_struct *queue,
				      NT_PRINTER_INFO_LEVEL *printer,
				      TALLOC_CTX *mem_ctx)
{
	data->notify_data.value[0] = printer->info_2->starttime;
	data->notify_data.value[1] = 0;
}

/*******************************************************************
 * fill a notify_info_data with the until time
 ********************************************************************/

static void spoolss_notify_until_time(int snum, 
				      SPOOL_NOTIFY_INFO_DATA *data,
				      print_queue_struct *queue,
				      NT_PRINTER_INFO_LEVEL *printer,
				      TALLOC_CTX *mem_ctx)
{
	data->notify_data.value[0] = printer->info_2->untiltime;
	data->notify_data.value[1] = 0;
}

/*******************************************************************
 * fill a notify_info_data with the status
 ********************************************************************/

static void spoolss_notify_status(int snum, 
				  SPOOL_NOTIFY_INFO_DATA *data,
				  print_queue_struct *queue,
				  NT_PRINTER_INFO_LEVEL *printer,
				  TALLOC_CTX *mem_ctx)
{
	print_status_struct status;

	print_queue_length(snum, &status);
	data->notify_data.value[0]=(uint32) status.status;
	data->notify_data.value[1] = 0;
}

/*******************************************************************
 * fill a notify_info_data with the number of jobs queued
 ********************************************************************/

void spoolss_notify_cjobs(int snum, 
				 SPOOL_NOTIFY_INFO_DATA *data,
				 print_queue_struct *queue,
				 NT_PRINTER_INFO_LEVEL *printer, 
				 TALLOC_CTX *mem_ctx)
{
	data->notify_data.value[0] = print_queue_length(snum, NULL);
	data->notify_data.value[1] = 0;
}

/*******************************************************************
 * fill a notify_info_data with the average ppm
 ********************************************************************/

static void spoolss_notify_average_ppm(int snum, 
				       SPOOL_NOTIFY_INFO_DATA *data,
				       print_queue_struct *queue,
				       NT_PRINTER_INFO_LEVEL *printer,
				       TALLOC_CTX *mem_ctx)
{
	/* always respond 8 pages per minutes */
	/* a little hard ! */
	data->notify_data.value[0] = printer->info_2->averageppm;
	data->notify_data.value[1] = 0;
}

/*******************************************************************
 * fill a notify_info_data with username
 ********************************************************************/

static void spoolss_notify_username(int snum, 
				    SPOOL_NOTIFY_INFO_DATA *data,
				    print_queue_struct *queue,
				    NT_PRINTER_INFO_LEVEL *printer,
				    TALLOC_CTX *mem_ctx)
{
	pstring temp;
	uint32 len;

	len = rpcstr_push(temp, queue->fs_user, sizeof(temp)-2, STR_TERMINATE);

	data->notify_data.data.length = len;
	data->notify_data.data.string = (uint16 *)TALLOC(mem_ctx, len);
	
	if (!data->notify_data.data.string) {
		data->notify_data.data.length = 0;
		return;
	}
	
	memcpy(data->notify_data.data.string, temp, len);
}

/*******************************************************************
 * fill a notify_info_data with job status
 ********************************************************************/

static void spoolss_notify_job_status(int snum, 
				      SPOOL_NOTIFY_INFO_DATA *data,
				      print_queue_struct *queue,
				      NT_PRINTER_INFO_LEVEL *printer,
				      TALLOC_CTX *mem_ctx)
{
	data->notify_data.value[0]=nt_printj_status(queue->status);
	data->notify_data.value[1] = 0;
}

/*******************************************************************
 * fill a notify_info_data with job name
 ********************************************************************/

static void spoolss_notify_job_name(int snum, 
				    SPOOL_NOTIFY_INFO_DATA *data,
				    print_queue_struct *queue,
				    NT_PRINTER_INFO_LEVEL *printer,
				    TALLOC_CTX *mem_ctx)
{
	pstring temp;
	uint32 len;

	len = rpcstr_push(temp, queue->fs_file, sizeof(temp)-2, STR_TERMINATE);

	data->notify_data.data.length = len;
	data->notify_data.data.string = (uint16 *)TALLOC(mem_ctx, len);
	
	if (!data->notify_data.data.string) {
		data->notify_data.data.length = 0;
		return;
	}
	
	memcpy(data->notify_data.data.string, temp, len);
}

/*******************************************************************
 * fill a notify_info_data with job status
 ********************************************************************/

static void spoolss_notify_job_status_string(int snum, 
					     SPOOL_NOTIFY_INFO_DATA *data,
					     print_queue_struct *queue,
					     NT_PRINTER_INFO_LEVEL *printer, 
					     TALLOC_CTX *mem_ctx)
{
	/*
	 * Now we're returning job status codes we just return a "" here. JRA.
	 */

	const char *p = "";
	pstring temp;
	uint32 len;

#if 0 /* NO LONGER NEEDED - JRA. 02/22/2001 */
	p = "unknown";

	switch (queue->status) {
	case LPQ_QUEUED:
		p = "Queued";
		break;
	case LPQ_PAUSED:
		p = "";    /* NT provides the paused string */
		break;
	case LPQ_SPOOLING:
		p = "Spooling";
		break;
	case LPQ_PRINTING:
		p = "Printing";
		break;
	}
#endif /* NO LONGER NEEDED. */

	len = rpcstr_push(temp, p, sizeof(temp) - 2, STR_TERMINATE);

	data->notify_data.data.length = len;
	data->notify_data.data.string = (uint16 *)TALLOC(mem_ctx, len);
	
	if (!data->notify_data.data.string) {
		data->notify_data.data.length = 0;
		return;
	}
	
	memcpy(data->notify_data.data.string, temp, len);
}

/*******************************************************************
 * fill a notify_info_data with job time
 ********************************************************************/

static void spoolss_notify_job_time(int snum, 
				    SPOOL_NOTIFY_INFO_DATA *data,
				    print_queue_struct *queue,
				    NT_PRINTER_INFO_LEVEL *printer,
				    TALLOC_CTX *mem_ctx)
{
	data->notify_data.value[0]=0x0;
	data->notify_data.value[1]=0;
}

/*******************************************************************
 * fill a notify_info_data with job size
 ********************************************************************/

static void spoolss_notify_job_size(int snum, 
				    SPOOL_NOTIFY_INFO_DATA *data,
				    print_queue_struct *queue,
				    NT_PRINTER_INFO_LEVEL *printer,
				    TALLOC_CTX *mem_ctx)
{
	data->notify_data.value[0]=queue->size;
	data->notify_data.value[1]=0;
}

/*******************************************************************
 * fill a notify_info_data with page info
 ********************************************************************/
static void spoolss_notify_total_pages(int snum,
				SPOOL_NOTIFY_INFO_DATA *data,
				print_queue_struct *queue,
				NT_PRINTER_INFO_LEVEL *printer,
				TALLOC_CTX *mem_ctx)
{
	data->notify_data.value[0]=queue->page_count;
	data->notify_data.value[1]=0;
}

/*******************************************************************
 * fill a notify_info_data with pages printed info.
 ********************************************************************/
static void spoolss_notify_pages_printed(int snum,
				SPOOL_NOTIFY_INFO_DATA *data,
				print_queue_struct *queue,
				NT_PRINTER_INFO_LEVEL *printer,
				TALLOC_CTX *mem_ctx)
{
	data->notify_data.value[0]=0;  /* Add code when back-end tracks this */
	data->notify_data.value[1]=0;
}

/*******************************************************************
 Fill a notify_info_data with job position.
 ********************************************************************/

static void spoolss_notify_job_position(int snum, 
					SPOOL_NOTIFY_INFO_DATA *data,
					print_queue_struct *queue,
					NT_PRINTER_INFO_LEVEL *printer,
					TALLOC_CTX *mem_ctx)
{
	data->notify_data.value[0]=queue->job;
	data->notify_data.value[1]=0;
}

/*******************************************************************
 Fill a notify_info_data with submitted time.
 ********************************************************************/

static void spoolss_notify_submitted_time(int snum, 
					  SPOOL_NOTIFY_INFO_DATA *data,
					  print_queue_struct *queue,
					  NT_PRINTER_INFO_LEVEL *printer,
					  TALLOC_CTX *mem_ctx)
{
	struct tm *t;
	uint32 len;
	SYSTEMTIME st;
	char *p;

	t=gmtime(&queue->time);

	len = sizeof(SYSTEMTIME);

	data->notify_data.data.length = len;
	data->notify_data.data.string = (uint16 *)TALLOC(mem_ctx, len);

	if (!data->notify_data.data.string) {
		data->notify_data.data.length = 0;
		return;
	}
	
	make_systemtime(&st, t);

	/*
	 * Systemtime must be linearized as a set of UINT16's. 
	 * Fix from Benjamin (Bj) Kuit bj@it.uts.edu.au
	 */

	p = (char *)data->notify_data.data.string;
	SSVAL(p, 0, st.year);
	SSVAL(p, 2, st.month);
	SSVAL(p, 4, st.dayofweek);
	SSVAL(p, 6, st.day);
	SSVAL(p, 8, st.hour);
        SSVAL(p, 10, st.minute);
	SSVAL(p, 12, st.second);
	SSVAL(p, 14, st.milliseconds);
}

struct s_notify_info_data_table
{
	uint16 type;
	uint16 field;
	const char *name;
	uint32 size;
	void (*fn) (int snum, SPOOL_NOTIFY_INFO_DATA *data,
		    print_queue_struct *queue,
		    NT_PRINTER_INFO_LEVEL *printer, TALLOC_CTX *mem_ctx);
};

/* A table describing the various print notification constants and
   whether the notification data is a pointer to a variable sized
   buffer, a one value uint32 or a two value uint32. */

static const struct s_notify_info_data_table notify_info_data_table[] =
{
{ PRINTER_NOTIFY_TYPE, PRINTER_NOTIFY_SERVER_NAME,         "PRINTER_NOTIFY_SERVER_NAME",         NOTIFY_STRING,   spoolss_notify_server_name },
{ PRINTER_NOTIFY_TYPE, PRINTER_NOTIFY_PRINTER_NAME,        "PRINTER_NOTIFY_PRINTER_NAME",        NOTIFY_STRING,   spoolss_notify_printer_name },
{ PRINTER_NOTIFY_TYPE, PRINTER_NOTIFY_SHARE_NAME,          "PRINTER_NOTIFY_SHARE_NAME",          NOTIFY_STRING,   spoolss_notify_share_name },
{ PRINTER_NOTIFY_TYPE, PRINTER_NOTIFY_PORT_NAME,           "PRINTER_NOTIFY_PORT_NAME",           NOTIFY_STRING,   spoolss_notify_port_name },
{ PRINTER_NOTIFY_TYPE, PRINTER_NOTIFY_DRIVER_NAME,         "PRINTER_NOTIFY_DRIVER_NAME",         NOTIFY_STRING,   spoolss_notify_driver_name },
{ PRINTER_NOTIFY_TYPE, PRINTER_NOTIFY_COMMENT,             "PRINTER_NOTIFY_COMMENT",             NOTIFY_STRING,   spoolss_notify_comment },
{ PRINTER_NOTIFY_TYPE, PRINTER_NOTIFY_LOCATION,            "PRINTER_NOTIFY_LOCATION",            NOTIFY_STRING,   spoolss_notify_location },
{ PRINTER_NOTIFY_TYPE, PRINTER_NOTIFY_DEVMODE,             "PRINTER_NOTIFY_DEVMODE",             NOTIFY_POINTER,   spoolss_notify_devmode },
{ PRINTER_NOTIFY_TYPE, PRINTER_NOTIFY_SEPFILE,             "PRINTER_NOTIFY_SEPFILE",             NOTIFY_STRING,   spoolss_notify_sepfile },
{ PRINTER_NOTIFY_TYPE, PRINTER_NOTIFY_PRINT_PROCESSOR,     "PRINTER_NOTIFY_PRINT_PROCESSOR",     NOTIFY_STRING,   spoolss_notify_print_processor },
{ PRINTER_NOTIFY_TYPE, PRINTER_NOTIFY_PARAMETERS,          "PRINTER_NOTIFY_PARAMETERS",          NOTIFY_STRING,   spoolss_notify_parameters },
{ PRINTER_NOTIFY_TYPE, PRINTER_NOTIFY_DATATYPE,            "PRINTER_NOTIFY_DATATYPE",            NOTIFY_STRING,   spoolss_notify_datatype },
{ PRINTER_NOTIFY_TYPE, PRINTER_NOTIFY_SECURITY_DESCRIPTOR, "PRINTER_NOTIFY_SECURITY_DESCRIPTOR", NOTIFY_SECDESC,   spoolss_notify_security_desc },
{ PRINTER_NOTIFY_TYPE, PRINTER_NOTIFY_ATTRIBUTES,          "PRINTER_NOTIFY_ATTRIBUTES",          NOTIFY_ONE_VALUE, spoolss_notify_attributes },
{ PRINTER_NOTIFY_TYPE, PRINTER_NOTIFY_PRIORITY,            "PRINTER_NOTIFY_PRIORITY",            NOTIFY_ONE_VALUE, spoolss_notify_priority },
{ PRINTER_NOTIFY_TYPE, PRINTER_NOTIFY_DEFAULT_PRIORITY,    "PRINTER_NOTIFY_DEFAULT_PRIORITY",    NOTIFY_ONE_VALUE, spoolss_notify_default_priority },
{ PRINTER_NOTIFY_TYPE, PRINTER_NOTIFY_START_TIME,          "PRINTER_NOTIFY_START_TIME",          NOTIFY_ONE_VALUE, spoolss_notify_start_time },
{ PRINTER_NOTIFY_TYPE, PRINTER_NOTIFY_UNTIL_TIME,          "PRINTER_NOTIFY_UNTIL_TIME",          NOTIFY_ONE_VALUE, spoolss_notify_until_time },
{ PRINTER_NOTIFY_TYPE, PRINTER_NOTIFY_STATUS,              "PRINTER_NOTIFY_STATUS",              NOTIFY_ONE_VALUE, spoolss_notify_status },
{ PRINTER_NOTIFY_TYPE, PRINTER_NOTIFY_STATUS_STRING,       "PRINTER_NOTIFY_STATUS_STRING",       NOTIFY_POINTER,   NULL },
{ PRINTER_NOTIFY_TYPE, PRINTER_NOTIFY_CJOBS,               "PRINTER_NOTIFY_CJOBS",               NOTIFY_ONE_VALUE, spoolss_notify_cjobs },
{ PRINTER_NOTIFY_TYPE, PRINTER_NOTIFY_AVERAGE_PPM,         "PRINTER_NOTIFY_AVERAGE_PPM",         NOTIFY_ONE_VALUE, spoolss_notify_average_ppm },
{ PRINTER_NOTIFY_TYPE, PRINTER_NOTIFY_TOTAL_PAGES,         "PRINTER_NOTIFY_TOTAL_PAGES",         NOTIFY_POINTER,   NULL },
{ PRINTER_NOTIFY_TYPE, PRINTER_NOTIFY_PAGES_PRINTED,       "PRINTER_NOTIFY_PAGES_PRINTED",       NOTIFY_POINTER,   NULL },
{ PRINTER_NOTIFY_TYPE, PRINTER_NOTIFY_TOTAL_BYTES,         "PRINTER_NOTIFY_TOTAL_BYTES",         NOTIFY_POINTER,   NULL },
{ PRINTER_NOTIFY_TYPE, PRINTER_NOTIFY_BYTES_PRINTED,       "PRINTER_NOTIFY_BYTES_PRINTED",       NOTIFY_POINTER,   NULL },
{ JOB_NOTIFY_TYPE,     JOB_NOTIFY_PRINTER_NAME,            "JOB_NOTIFY_PRINTER_NAME",            NOTIFY_STRING,   spoolss_notify_printer_name },
{ JOB_NOTIFY_TYPE,     JOB_NOTIFY_MACHINE_NAME,            "JOB_NOTIFY_MACHINE_NAME",            NOTIFY_STRING,   spoolss_notify_server_name },
{ JOB_NOTIFY_TYPE,     JOB_NOTIFY_PORT_NAME,               "JOB_NOTIFY_PORT_NAME",               NOTIFY_STRING,   spoolss_notify_port_name },
{ JOB_NOTIFY_TYPE,     JOB_NOTIFY_USER_NAME,               "JOB_NOTIFY_USER_NAME",               NOTIFY_STRING,   spoolss_notify_username },
{ JOB_NOTIFY_TYPE,     JOB_NOTIFY_NOTIFY_NAME,             "JOB_NOTIFY_NOTIFY_NAME",             NOTIFY_STRING,   spoolss_notify_username },
{ JOB_NOTIFY_TYPE,     JOB_NOTIFY_DATATYPE,                "JOB_NOTIFY_DATATYPE",                NOTIFY_STRING,   spoolss_notify_datatype },
{ JOB_NOTIFY_TYPE,     JOB_NOTIFY_PRINT_PROCESSOR,         "JOB_NOTIFY_PRINT_PROCESSOR",         NOTIFY_STRING,   spoolss_notify_print_processor },
{ JOB_NOTIFY_TYPE,     JOB_NOTIFY_PARAMETERS,              "JOB_NOTIFY_PARAMETERS",              NOTIFY_STRING,   spoolss_notify_parameters },
{ JOB_NOTIFY_TYPE,     JOB_NOTIFY_DRIVER_NAME,             "JOB_NOTIFY_DRIVER_NAME",             NOTIFY_STRING,   spoolss_notify_driver_name },
{ JOB_NOTIFY_TYPE,     JOB_NOTIFY_DEVMODE,                 "JOB_NOTIFY_DEVMODE",                 NOTIFY_POINTER,   spoolss_notify_devmode },
{ JOB_NOTIFY_TYPE,     JOB_NOTIFY_STATUS,                  "JOB_NOTIFY_STATUS",                  NOTIFY_ONE_VALUE, spoolss_notify_job_status },
{ JOB_NOTIFY_TYPE,     JOB_NOTIFY_STATUS_STRING,           "JOB_NOTIFY_STATUS_STRING",           NOTIFY_STRING,   spoolss_notify_job_status_string },
{ JOB_NOTIFY_TYPE,     JOB_NOTIFY_SECURITY_DESCRIPTOR,     "JOB_NOTIFY_SECURITY_DESCRIPTOR",     NOTIFY_POINTER,   NULL },
{ JOB_NOTIFY_TYPE,     JOB_NOTIFY_DOCUMENT,                "JOB_NOTIFY_DOCUMENT",                NOTIFY_STRING,   spoolss_notify_job_name },
{ JOB_NOTIFY_TYPE,     JOB_NOTIFY_PRIORITY,                "JOB_NOTIFY_PRIORITY",                NOTIFY_ONE_VALUE, spoolss_notify_priority },
{ JOB_NOTIFY_TYPE,     JOB_NOTIFY_POSITION,                "JOB_NOTIFY_POSITION",                NOTIFY_ONE_VALUE, spoolss_notify_job_position },
{ JOB_NOTIFY_TYPE,     JOB_NOTIFY_SUBMITTED,               "JOB_NOTIFY_SUBMITTED",               NOTIFY_POINTER,   spoolss_notify_submitted_time },
{ JOB_NOTIFY_TYPE,     JOB_NOTIFY_START_TIME,              "JOB_NOTIFY_START_TIME",              NOTIFY_ONE_VALUE, spoolss_notify_start_time },
{ JOB_NOTIFY_TYPE,     JOB_NOTIFY_UNTIL_TIME,              "JOB_NOTIFY_UNTIL_TIME",              NOTIFY_ONE_VALUE, spoolss_notify_until_time },
{ JOB_NOTIFY_TYPE,     JOB_NOTIFY_TIME,                    "JOB_NOTIFY_TIME",                    NOTIFY_ONE_VALUE, spoolss_notify_job_time },
{ JOB_NOTIFY_TYPE,     JOB_NOTIFY_TOTAL_PAGES,             "JOB_NOTIFY_TOTAL_PAGES",             NOTIFY_ONE_VALUE, spoolss_notify_total_pages },
{ JOB_NOTIFY_TYPE,     JOB_NOTIFY_PAGES_PRINTED,           "JOB_NOTIFY_PAGES_PRINTED",           NOTIFY_ONE_VALUE, spoolss_notify_pages_printed },
{ JOB_NOTIFY_TYPE,     JOB_NOTIFY_TOTAL_BYTES,             "JOB_NOTIFY_TOTAL_BYTES",             NOTIFY_ONE_VALUE, spoolss_notify_job_size },
{ PRINT_TABLE_END, 0x0, NULL, 0x0, NULL },
};

/*******************************************************************
 Return the size of info_data structure.
********************************************************************/

static uint32 size_of_notify_info_data(uint16 type, uint16 field)
{
	int i=0;

	for (i = 0; i < (sizeof(notify_info_data_table)/sizeof(struct s_notify_info_data_table)); i++) {
		if ( (notify_info_data_table[i].type == type)
			&& (notify_info_data_table[i].field == field) ) {
			switch(notify_info_data_table[i].size) {
				case NOTIFY_ONE_VALUE:
				case NOTIFY_TWO_VALUE:
					return 1;
				case NOTIFY_STRING:
					return 2;

				/* The only pointer notify data I have seen on
				   the wire is the submitted time and this has
				   the notify size set to 4. -tpot */

				case NOTIFY_POINTER:
					return 4;
					
				case NOTIFY_SECDESC:
					return 5;
			}
		}
	}

	DEBUG(5, ("invalid notify data type %d/%d\n", type, field));

	return 0;
}

/*******************************************************************
 Return the type of notify_info_data.
********************************************************************/

static uint32 type_of_notify_info_data(uint16 type, uint16 field)
{
	uint32 i=0;

	for (i = 0; i < (sizeof(notify_info_data_table)/sizeof(struct s_notify_info_data_table)); i++) {
		if (notify_info_data_table[i].type == type &&
		    notify_info_data_table[i].field == field)
			return notify_info_data_table[i].size;
	}

	return 0;
}

/****************************************************************************
****************************************************************************/

static BOOL search_notify(uint16 type, uint16 field, int *value)
{	
	int i;

	for (i = 0; notify_info_data_table[i].type != PRINT_TABLE_END; i++) {
		if (notify_info_data_table[i].type == type &&
		    notify_info_data_table[i].field == field &&
		    notify_info_data_table[i].fn != NULL) {
			*value = i;
			return True;
		}
	}
	
	return False;	
}

/****************************************************************************
****************************************************************************/

void construct_info_data(SPOOL_NOTIFY_INFO_DATA *info_data, uint16 type, uint16 field, int id)
{
	info_data->type     = type;
	info_data->field    = field;
	info_data->reserved = 0;

	info_data->size     = size_of_notify_info_data(type, field);
	info_data->enc_type = type_of_notify_info_data(type, field);

	info_data->id = id;
}

/*******************************************************************
 *
 * fill a notify_info struct with info asked
 *
 ********************************************************************/

static BOOL construct_notify_printer_info(Printer_entry *print_hnd, SPOOL_NOTIFY_INFO *info, int
					  snum, SPOOL_NOTIFY_OPTION_TYPE
					  *option_type, uint32 id,
					  TALLOC_CTX *mem_ctx) 
{
	int field_num,j;
	uint16 type;
	uint16 field;

	SPOOL_NOTIFY_INFO_DATA *current_data;
	NT_PRINTER_INFO_LEVEL *printer = NULL;
	print_queue_struct *queue=NULL;

	type=option_type->type;

	DEBUG(4,("construct_notify_printer_info: Notify type: [%s], number of notify info: [%d] on printer: [%s]\n",
		(option_type->type==PRINTER_NOTIFY_TYPE?"PRINTER_NOTIFY_TYPE":"JOB_NOTIFY_TYPE"),
		option_type->count, lp_servicename(snum)));
	
	if (!W_ERROR_IS_OK(get_a_printer(print_hnd, &printer, 2, lp_const_servicename(snum))))
		return False;

	for(field_num=0; field_num<option_type->count; field_num++) {
		field = option_type->fields[field_num];
		
		DEBUG(4,("construct_notify_printer_info: notify [%d]: type [%x], field [%x]\n", field_num, type, field));

		if (!search_notify(type, field, &j) )
			continue;

		if((info->data=SMB_REALLOC_ARRAY(info->data, SPOOL_NOTIFY_INFO_DATA, info->count+1)) == NULL) {
			DEBUG(2,("construct_notify_printer_info: failed to enlarge buffer info->data!\n"));
			free_a_printer(&printer, 2);
			return False;
		}

		current_data = &info->data[info->count];

		construct_info_data(current_data, type, field, id);

		DEBUG(10,("construct_notify_printer_info: calling [%s]  snum=%d  printername=[%s])\n",
				notify_info_data_table[j].name, snum, printer->info_2->printername ));

		notify_info_data_table[j].fn(snum, current_data, queue,
					     printer, mem_ctx);

		info->count++;
	}

	free_a_printer(&printer, 2);
	return True;
}

/*******************************************************************
 *
 * fill a notify_info struct with info asked
 *
 ********************************************************************/

static BOOL construct_notify_jobs_info(print_queue_struct *queue,
				       SPOOL_NOTIFY_INFO *info,
				       NT_PRINTER_INFO_LEVEL *printer,
				       int snum, SPOOL_NOTIFY_OPTION_TYPE
				       *option_type, uint32 id,
				       TALLOC_CTX *mem_ctx) 
{
	int field_num,j;
	uint16 type;
	uint16 field;

	SPOOL_NOTIFY_INFO_DATA *current_data;
	
	DEBUG(4,("construct_notify_jobs_info\n"));
	
	type = option_type->type;

	DEBUGADD(4,("Notify type: [%s], number of notify info: [%d]\n",
		(option_type->type==PRINTER_NOTIFY_TYPE?"PRINTER_NOTIFY_TYPE":"JOB_NOTIFY_TYPE"),
		option_type->count));

	for(field_num=0; field_num<option_type->count; field_num++) {
		field = option_type->fields[field_num];

		if (!search_notify(type, field, &j) )
			continue;

		if((info->data=SMB_REALLOC_ARRAY(info->data, SPOOL_NOTIFY_INFO_DATA, info->count+1)) == NULL) {
			DEBUG(2,("construct_notify_jobs_info: failed to enlarg buffer info->data!\n"));
			return False;
		}

		current_data=&(info->data[info->count]);

		construct_info_data(current_data, type, field, id);
		notify_info_data_table[j].fn(snum, current_data, queue,
					     printer, mem_ctx);
		info->count++;
	}

	return True;
}

/*
 * JFM: The enumeration is not that simple, it's even non obvious.
 *
 * let's take an example: I want to monitor the PRINTER SERVER for
 * the printer's name and the number of jobs currently queued.
 * So in the NOTIFY_OPTION, I have one NOTIFY_OPTION_TYPE structure.
 * Its type is PRINTER_NOTIFY_TYPE and it has 2 fields NAME and CJOBS.
 *
 * I have 3 printers on the back of my server.
 *
 * Now the response is a NOTIFY_INFO structure, with 6 NOTIFY_INFO_DATA
 * structures.
 *   Number	Data			Id
 *	1	printer 1 name		1
 *	2	printer 1 cjob		1
 *	3	printer 2 name		2
 *	4	printer 2 cjob		2
 *	5	printer 3 name		3
 *	6	printer 3 name		3
 *
 * that's the print server case, the printer case is even worse.
 */

/*******************************************************************
 *
 * enumerate all printers on the printserver
 * fill a notify_info struct with info asked
 *
 ********************************************************************/

static WERROR printserver_notify_info(pipes_struct *p, POLICY_HND *hnd, 
				      SPOOL_NOTIFY_INFO *info,
				      TALLOC_CTX *mem_ctx)
{
	int snum;
	Printer_entry *Printer=find_printer_index_by_hnd(p, hnd);
	int n_services=lp_numservices();
	int i;
	SPOOL_NOTIFY_OPTION *option;
	SPOOL_NOTIFY_OPTION_TYPE *option_type;

	DEBUG(4,("printserver_notify_info\n"));
	
	if (!Printer)
		return WERR_BADFID;

	option=Printer->notify.option;
	info->version=2;
	info->data=NULL;
	info->count=0;

	/* a bug in xp sp2 rc2 causes it to send a fnpcn request without 
	   sending a ffpcn() request first */

	if ( !option )
		return WERR_BADFID;

	for (i=0; i<option->count; i++) {
		option_type=&(option->ctr.type[i]);
		
		if (option_type->type!=PRINTER_NOTIFY_TYPE)
			continue;
		
		for (snum=0; snum<n_services; snum++)
		{
			if ( lp_browseable(snum) && lp_snum_ok(snum) && lp_print_ok(snum) )
				construct_notify_printer_info ( Printer, info, snum, option_type, snum, mem_ctx );
		}
	}
			
#if 0			
	/*
	 * Debugging information, don't delete.
	 */

	DEBUG(1,("dumping the NOTIFY_INFO\n"));
	DEBUGADD(1,("info->version:[%d], info->flags:[%d], info->count:[%d]\n", info->version, info->flags, info->count));
	DEBUGADD(1,("num\ttype\tfield\tres\tid\tsize\tenc_type\n"));
	
	for (i=0; i<info->count; i++) {
		DEBUGADD(1,("[%d]\t[%d]\t[%d]\t[%d]\t[%d]\t[%d]\t[%d]\n",
		i, info->data[i].type, info->data[i].field, info->data[i].reserved,
		info->data[i].id, info->data[i].size, info->data[i].enc_type));
	}
#endif
	
	return WERR_OK;
}

/*******************************************************************
 *
 * fill a notify_info struct with info asked
 *
 ********************************************************************/

static WERROR printer_notify_info(pipes_struct *p, POLICY_HND *hnd, SPOOL_NOTIFY_INFO *info,
				  TALLOC_CTX *mem_ctx)
{
	int snum;
	Printer_entry *Printer=find_printer_index_by_hnd(p, hnd);
	int i;
	uint32 id;
	SPOOL_NOTIFY_OPTION *option;
	SPOOL_NOTIFY_OPTION_TYPE *option_type;
	int count,j;
	print_queue_struct *queue=NULL;
	print_status_struct status;
	
	DEBUG(4,("printer_notify_info\n"));

	if (!Printer)
		return WERR_BADFID;

	option=Printer->notify.option;
	id = 0x0;
	info->version=2;
	info->data=NULL;
	info->count=0;

	/* a bug in xp sp2 rc2 causes it to send a fnpcn request without 
	   sending a ffpcn() request first */

	if ( !option )
		return WERR_BADFID;

	get_printer_snum(p, hnd, &snum);

	for (i=0; i<option->count; i++) {
		option_type=&option->ctr.type[i];
		
		switch ( option_type->type ) {
		case PRINTER_NOTIFY_TYPE:
			if(construct_notify_printer_info(Printer, info, snum, 
							 option_type, id,
							 mem_ctx))  
				id--;
			break;
			
		case JOB_NOTIFY_TYPE: {
			NT_PRINTER_INFO_LEVEL *printer = NULL;

			count = print_queue_status(snum, &queue, &status);

			if (!W_ERROR_IS_OK(get_a_printer(Printer, &printer, 2, lp_const_servicename(snum))))
				goto done;

			for (j=0; j<count; j++) {
				construct_notify_jobs_info(&queue[j], info,
							   printer, snum,
							   option_type,
							   queue[j].job,
							   mem_ctx); 
			}

			free_a_printer(&printer, 2);
			
		done:
			SAFE_FREE(queue);
			break;
		}
		}
	}
	
	/*
	 * Debugging information, don't delete.
	 */
	/*
	DEBUG(1,("dumping the NOTIFY_INFO\n"));
	DEBUGADD(1,("info->version:[%d], info->flags:[%d], info->count:[%d]\n", info->version, info->flags, info->count));
	DEBUGADD(1,("num\ttype\tfield\tres\tid\tsize\tenc_type\n"));
	
	for (i=0; i<info->count; i++) {
		DEBUGADD(1,("[%d]\t[%d]\t[%d]\t[%d]\t[%d]\t[%d]\t[%d]\n",
		i, info->data[i].type, info->data[i].field, info->data[i].reserved,
		info->data[i].id, info->data[i].size, info->data[i].enc_type));
	}
	*/
	return WERR_OK;
}

/********************************************************************
 * spoolss_rfnpcnex
 ********************************************************************/

WERROR _spoolss_rfnpcnex( pipes_struct *p, SPOOL_Q_RFNPCNEX *q_u, SPOOL_R_RFNPCNEX *r_u)
{
	POLICY_HND *handle = &q_u->handle;
	SPOOL_NOTIFY_INFO *info = &r_u->info;

	Printer_entry *Printer=find_printer_index_by_hnd(p, handle);
	WERROR result = WERR_BADFID;

	/* we always have a NOTIFY_INFO struct */
	r_u->info_ptr=0x1;

	if (!Printer) {
		DEBUG(2,("_spoolss_rfnpcnex: Invalid handle (%s:%u:%u).\n",
			 OUR_HANDLE(handle)));
		goto done;
	}

	DEBUG(4,("Printer type %x\n",Printer->printer_type));

	/*
	 * 	We are now using the change value, and 
	 *	I should check for PRINTER_NOTIFY_OPTIONS_REFRESH but as
	 *	I don't have a global notification system, I'm sending back all the
	 *	informations even when _NOTHING_ has changed.
	 */

	/* We need to keep track of the change value to send back in 
           RRPCN replies otherwise our updates are ignored. */

	Printer->notify.fnpcn = True;

	if (Printer->notify.client_connected) {
		DEBUG(10,("_spoolss_rfnpcnex: Saving change value in request [%x]\n", q_u->change));
		Printer->notify.change = q_u->change;
	}

	/* just ignore the SPOOL_NOTIFY_OPTION */
	
	switch (Printer->printer_type) {
		case SPLHND_SERVER:
			result = printserver_notify_info(p, handle, info, p->mem_ctx);
			break;
			
		case SPLHND_PRINTER:
			result = printer_notify_info(p, handle, info, p->mem_ctx);
			break;
	}
	
	Printer->notify.fnpcn = False;
	
done:
	return result;
}

/********************************************************************
 * construct_printer_info_0
 * fill a printer_info_0 struct
 ********************************************************************/

static BOOL construct_printer_info_0(Printer_entry *print_hnd, PRINTER_INFO_0 *printer, int snum)
{
	pstring chaine;
	int count;
	NT_PRINTER_INFO_LEVEL *ntprinter = NULL;
	counter_printer_0 *session_counter;
	uint32 global_counter;
	struct tm *t;
	time_t setuptime;
	print_status_struct status;
	
	if (!W_ERROR_IS_OK(get_a_printer(print_hnd, &ntprinter, 2, lp_const_servicename(snum))))
		return False;

	count = print_queue_length(snum, &status);

	/* check if we already have a counter for this printer */	
	for(session_counter = counter_list; session_counter; session_counter = session_counter->next) {
		if (session_counter->snum == snum)
			break;
	}

	/* it's the first time, add it to the list */
	if (session_counter==NULL) {
		if((session_counter=SMB_MALLOC_P(counter_printer_0)) == NULL) {
			free_a_printer(&ntprinter, 2);
			return False;
		}
		ZERO_STRUCTP(session_counter);
		session_counter->snum=snum;
		session_counter->counter=0;
		DLIST_ADD(counter_list, session_counter);
	}
	
	/* increment it */
	session_counter->counter++;
	
	/* JFM:
	 * the global_counter should be stored in a TDB as it's common to all the clients
	 * and should be zeroed on samba startup
	 */
	global_counter=session_counter->counter;
	
	pstrcpy(chaine,ntprinter->info_2->printername);

	init_unistr(&printer->printername, chaine);
	
	slprintf(chaine,sizeof(chaine)-1,"\\\\%s", get_server_name(print_hnd));
	init_unistr(&printer->servername, chaine);
	
	printer->cjobs = count;
	printer->total_jobs = 0;
	printer->total_bytes = 0;

	setuptime = (time_t)ntprinter->info_2->setuptime;
	t=gmtime(&setuptime);

	printer->year = t->tm_year+1900;
	printer->month = t->tm_mon+1;
	printer->dayofweek = t->tm_wday;
	printer->day = t->tm_mday;
	printer->hour = t->tm_hour;
	printer->minute = t->tm_min;
	printer->second = t->tm_sec;
	printer->milliseconds = 0;

	printer->global_counter = global_counter;
	printer->total_pages = 0;
	
	/* in 2.2 we reported ourselves as 0x0004 and 0x0565 */
	printer->major_version = 0x0005; 	/* NT 5 */
	printer->build_version = 0x0893; 	/* build 2195 */
	
	printer->unknown7 = 0x1;
	printer->unknown8 = 0x0;
	printer->unknown9 = 0x0;
	printer->session_counter = session_counter->counter;
	printer->unknown11 = 0x0;
	printer->printer_errors = 0x0;		/* number of print failure */
	printer->unknown13 = 0x0;
	printer->unknown14 = 0x1;
	printer->unknown15 = 0x024a;		/* 586 Pentium ? */
	printer->unknown16 =  0x0;
	printer->change_id = ntprinter->info_2->changeid; /* ChangeID in milliseconds*/
	printer->unknown18 =  0x0;
	printer->status = nt_printq_status(status.status);
	printer->unknown20 =  0x0;
	printer->c_setprinter = get_c_setprinter(); /* monotonically increasing sum of delta printer counts */
	printer->unknown22 = 0x0;
	printer->unknown23 = 0x6; 		/* 6  ???*/
	printer->unknown24 = 0; 		/* unknown 24 to 26 are always 0 */
	printer->unknown25 = 0;
	printer->unknown26 = 0;
	printer->unknown27 = 0;
	printer->unknown28 = 0;
	printer->unknown29 = 0;
	
	free_a_printer(&ntprinter,2);
	return (True);	
}

/********************************************************************
 * construct_printer_info_1
 * fill a printer_info_1 struct
 ********************************************************************/
static BOOL construct_printer_info_1(Printer_entry *print_hnd, uint32 flags, PRINTER_INFO_1 *printer, int snum)
{
	pstring chaine;
	pstring chaine2;
	NT_PRINTER_INFO_LEVEL *ntprinter = NULL;

	if (!W_ERROR_IS_OK(get_a_printer(print_hnd, &ntprinter, 2, lp_const_servicename(snum))))
		return False;

	printer->flags=flags;

	if (*ntprinter->info_2->comment == '\0') {
		init_unistr(&printer->comment, lp_comment(snum));
		slprintf(chaine,sizeof(chaine)-1,"%s,%s,%s", ntprinter->info_2->printername,
			ntprinter->info_2->drivername, lp_comment(snum));
	}
	else {
		init_unistr(&printer->comment, ntprinter->info_2->comment); /* saved comment. */
		slprintf(chaine,sizeof(chaine)-1,"%s,%s,%s", ntprinter->info_2->printername,
			ntprinter->info_2->drivername, ntprinter->info_2->comment);
	}
		
	slprintf(chaine2,sizeof(chaine)-1,"%s", ntprinter->info_2->printername);

	init_unistr(&printer->description, chaine);
	init_unistr(&printer->name, chaine2);	
	
	free_a_printer(&ntprinter,2);

	return True;
}

/****************************************************************************
 Free a DEVMODE struct.
****************************************************************************/

static void free_dev_mode(DEVICEMODE *dev)
{
	if (dev == NULL)
		return;

	SAFE_FREE(dev->dev_private);
	SAFE_FREE(dev);	
}


/****************************************************************************
 Convert an NT_DEVICEMODE to a DEVICEMODE structure.  Both pointers 
 should be valid upon entry
****************************************************************************/

static BOOL convert_nt_devicemode( DEVICEMODE *devmode, NT_DEVICEMODE *ntdevmode )
{
	if ( !devmode || !ntdevmode )
		return False;
		
	init_unistr(&devmode->devicename, ntdevmode->devicename);

	init_unistr(&devmode->formname, ntdevmode->formname);

	devmode->specversion      = ntdevmode->specversion;
	devmode->driverversion    = ntdevmode->driverversion;
	devmode->size             = ntdevmode->size;
	devmode->driverextra      = ntdevmode->driverextra;
	devmode->fields           = ntdevmode->fields;
				
	devmode->orientation      = ntdevmode->orientation;	
	devmode->papersize        = ntdevmode->papersize;
	devmode->paperlength      = ntdevmode->paperlength;
	devmode->paperwidth       = ntdevmode->paperwidth;
	devmode->scale            = ntdevmode->scale;
	devmode->copies           = ntdevmode->copies;
	devmode->defaultsource    = ntdevmode->defaultsource;
	devmode->printquality     = ntdevmode->printquality;
	devmode->color            = ntdevmode->color;
	devmode->duplex           = ntdevmode->duplex;
	devmode->yresolution      = ntdevmode->yresolution;
	devmode->ttoption         = ntdevmode->ttoption;
	devmode->collate          = ntdevmode->collate;
	devmode->icmmethod        = ntdevmode->icmmethod;
	devmode->icmintent        = ntdevmode->icmintent;
	devmode->mediatype        = ntdevmode->mediatype;
	devmode->dithertype       = ntdevmode->dithertype;

	if (ntdevmode->nt_dev_private != NULL) {
		if ((devmode->dev_private=(uint8 *)memdup(ntdevmode->nt_dev_private, ntdevmode->driverextra)) == NULL)
			return False;
	}
	
	return True;
}

/****************************************************************************
 Create a DEVMODE struct. Returns malloced memory.
****************************************************************************/

DEVICEMODE *construct_dev_mode(int snum)
{
	NT_PRINTER_INFO_LEVEL 	*printer = NULL;
	DEVICEMODE 		*devmode = NULL;
	
	DEBUG(7,("construct_dev_mode\n"));
	
	DEBUGADD(8,("getting printer characteristics\n"));

	if (!W_ERROR_IS_OK(get_a_printer(NULL, &printer, 2, lp_const_servicename(snum)))) 
		return NULL;

	if ( !printer->info_2->devmode ) {
		DEBUG(5, ("BONG! There was no device mode!\n"));
		goto done;
	}

	if ((devmode = SMB_MALLOC_P(DEVICEMODE)) == NULL) {
		DEBUG(2,("construct_dev_mode: malloc fail.\n"));
		goto done;
	}

	ZERO_STRUCTP(devmode);	
	
	DEBUGADD(8,("loading DEVICEMODE\n"));

	if ( !convert_nt_devicemode( devmode, printer->info_2->devmode ) ) {
		free_dev_mode( devmode );
		devmode = NULL;
	}

done:
	free_a_printer(&printer,2);

	return devmode;
}

/********************************************************************
 * construct_printer_info_2
 * fill a printer_info_2 struct
 ********************************************************************/

static BOOL construct_printer_info_2(Printer_entry *print_hnd, PRINTER_INFO_2 *printer, int snum)
{
	int count;
	NT_PRINTER_INFO_LEVEL *ntprinter = NULL;

	print_status_struct status;

	if (!W_ERROR_IS_OK(get_a_printer(print_hnd, &ntprinter, 2, lp_const_servicename(snum))))
		return False;
		
	count = print_queue_length(snum, &status);

	init_unistr(&printer->servername, ntprinter->info_2->servername); /* servername*/
	init_unistr(&printer->printername, ntprinter->info_2->printername);				/* printername*/
	init_unistr(&printer->sharename, lp_servicename(snum));			/* sharename */
	init_unistr(&printer->portname, ntprinter->info_2->portname);			/* port */	
	init_unistr(&printer->drivername, ntprinter->info_2->drivername);	/* drivername */

	if (*ntprinter->info_2->comment == '\0')
		init_unistr(&printer->comment, lp_comment(snum));			/* comment */	
	else
		init_unistr(&printer->comment, ntprinter->info_2->comment); /* saved comment. */

	init_unistr(&printer->location, ntprinter->info_2->location);		/* location */	
	init_unistr(&printer->sepfile, ntprinter->info_2->sepfile);		/* separator file */
	init_unistr(&printer->printprocessor, ntprinter->info_2->printprocessor);/* print processor */
	init_unistr(&printer->datatype, ntprinter->info_2->datatype);		/* datatype */	
	init_unistr(&printer->parameters, ntprinter->info_2->parameters);	/* parameters (of print processor) */	

	printer->attributes = ntprinter->info_2->attributes;

	printer->priority = ntprinter->info_2->priority;				/* priority */	
	printer->defaultpriority = ntprinter->info_2->default_priority;		/* default priority */
	printer->starttime = ntprinter->info_2->starttime;			/* starttime */
	printer->untiltime = ntprinter->info_2->untiltime;			/* untiltime */
	printer->status = nt_printq_status(status.status);			/* status */
	printer->cjobs = count;							/* jobs */
	printer->averageppm = ntprinter->info_2->averageppm;			/* average pages per minute */
			
	if ( !(printer->devmode = construct_dev_mode(snum)) )
		DEBUG(8, ("Returning NULL Devicemode!\n"));

	printer->secdesc = NULL;

	if ( ntprinter->info_2->secdesc_buf 
		&& ntprinter->info_2->secdesc_buf->len != 0 ) 
	{
		/* don't use talloc_steal() here unless you do a deep steal of all 
		   the SEC_DESC members */

		printer->secdesc = dup_sec_desc( get_talloc_ctx(), 
			ntprinter->info_2->secdesc_buf->sec );
	}

	free_a_printer(&ntprinter, 2);

	return True;
}

/********************************************************************
 * construct_printer_info_3
 * fill a printer_info_3 struct
 ********************************************************************/

static BOOL construct_printer_info_3(Printer_entry *print_hnd, PRINTER_INFO_3 **pp_printer, int snum)
{
	NT_PRINTER_INFO_LEVEL *ntprinter = NULL;
	PRINTER_INFO_3 *printer = NULL;

	if (!W_ERROR_IS_OK(get_a_printer(print_hnd, &ntprinter, 2, lp_const_servicename(snum))))
		return False;

	*pp_printer = NULL;
	if ((printer = SMB_MALLOC_P(PRINTER_INFO_3)) == NULL) {
		DEBUG(2,("construct_printer_info_3: malloc fail.\n"));
		free_a_printer(&ntprinter, 2);
		return False;
	}

	ZERO_STRUCTP(printer);
	
	/* These are the components of the SD we are returning. */

	printer->flags = 0x4; 

	if (ntprinter->info_2->secdesc_buf && ntprinter->info_2->secdesc_buf->len != 0) {
		/* don't use talloc_steal() here unless you do a deep steal of all 
		   the SEC_DESC members */

		printer->secdesc = dup_sec_desc( get_talloc_ctx(), 
			ntprinter->info_2->secdesc_buf->sec );
	}

	free_a_printer(&ntprinter, 2);

	*pp_printer = printer;
	return True;
}

/********************************************************************
 * construct_printer_info_4
 * fill a printer_info_4 struct
 ********************************************************************/

static BOOL construct_printer_info_4(Printer_entry *print_hnd, PRINTER_INFO_4 *printer, int snum)
{
	NT_PRINTER_INFO_LEVEL *ntprinter = NULL;

	if (!W_ERROR_IS_OK(get_a_printer(print_hnd, &ntprinter, 2, lp_const_servicename(snum))))
		return False;
		
	init_unistr(&printer->printername, ntprinter->info_2->printername);				/* printername*/
	init_unistr(&printer->servername, ntprinter->info_2->servername); /* servername*/
	printer->attributes = ntprinter->info_2->attributes;

	free_a_printer(&ntprinter, 2);
	return True;
}

/********************************************************************
 * construct_printer_info_5
 * fill a printer_info_5 struct
 ********************************************************************/

static BOOL construct_printer_info_5(Printer_entry *print_hnd, PRINTER_INFO_5 *printer, int snum)
{
	NT_PRINTER_INFO_LEVEL *ntprinter = NULL;

	if (!W_ERROR_IS_OK(get_a_printer(print_hnd, &ntprinter, 2, lp_const_servicename(snum))))
		return False;
		
	init_unistr(&printer->printername, ntprinter->info_2->printername);
	init_unistr(&printer->portname, ntprinter->info_2->portname); 
	printer->attributes = ntprinter->info_2->attributes;

	/* these two are not used by NT+ according to MSDN */

	printer->device_not_selected_timeout = 0x0;  /* have seen 0x3a98 */
	printer->transmission_retry_timeout  = 0x0;  /* have seen 0xafc8 */

	free_a_printer(&ntprinter, 2);

	return True;
}

/********************************************************************
 * construct_printer_info_7
 * fill a printer_info_7 struct
 ********************************************************************/

static BOOL construct_printer_info_7(Printer_entry *print_hnd, PRINTER_INFO_7 *printer, int snum)
{
	char *guid_str = NULL;
	struct uuid guid; 
	
	if (is_printer_published(print_hnd, snum, &guid)) {
		asprintf(&guid_str, "{%s}", smb_uuid_string_static(guid));
		strupper_m(guid_str);
		init_unistr(&printer->guid, guid_str);
		printer->action = SPOOL_DS_PUBLISH;
	} else {
		init_unistr(&printer->guid, "");
		printer->action = SPOOL_DS_UNPUBLISH;
	}

	return True;
}

/********************************************************************
 Spoolss_enumprinters.
********************************************************************/

static WERROR enum_all_printers_info_1(uint32 flags, RPC_BUFFER *buffer, uint32 offered, uint32 *needed, uint32 *returned)
{
	int snum;
	int i;
	int n_services=lp_numservices();
	PRINTER_INFO_1 *printers=NULL;
	PRINTER_INFO_1 current_prt;
	WERROR result = WERR_OK;
	
	DEBUG(4,("enum_all_printers_info_1\n"));	

	for (snum=0; snum<n_services; snum++) {
		if (lp_browseable(snum) && lp_snum_ok(snum) && lp_print_ok(snum) ) {
			DEBUG(4,("Found a printer in smb.conf: %s[%x]\n", lp_servicename(snum), snum));

			if (construct_printer_info_1(NULL, flags, &current_prt, snum)) {
				if((printers=SMB_REALLOC_ARRAY(printers, PRINTER_INFO_1, *returned +1)) == NULL) {
					DEBUG(2,("enum_all_printers_info_1: failed to enlarge printers buffer!\n"));
					*returned=0;
					return WERR_NOMEM;
				}
				DEBUG(4,("ReAlloced memory for [%d] PRINTER_INFO_1\n", *returned));		

				memcpy(&printers[*returned], &current_prt, sizeof(PRINTER_INFO_1));
				(*returned)++;
			}
		}
	}
		
	/* check the required size. */	
	for (i=0; i<*returned; i++)
		(*needed) += spoolss_size_printer_info_1(&printers[i]);

	if (*needed > offered) {
		result = WERR_INSUFFICIENT_BUFFER;
		goto out;
	}

	if (!rpcbuf_alloc_size(buffer, *needed)) {
		result = WERR_NOMEM;
		goto out;
	}

	/* fill the buffer with the structures */
	for (i=0; i<*returned; i++)
		smb_io_printer_info_1("", buffer, &printers[i], 0);	

out:
	/* clear memory */

	SAFE_FREE(printers);

	if ( !W_ERROR_IS_OK(result) )
		*returned = 0;

	return result;
}

/********************************************************************
 enum_all_printers_info_1_local.
*********************************************************************/

static WERROR enum_all_printers_info_1_local(RPC_BUFFER *buffer, uint32 offered, uint32 *needed, uint32 *returned)
{
	DEBUG(4,("enum_all_printers_info_1_local\n"));	
	
	return enum_all_printers_info_1(PRINTER_ENUM_ICON8, buffer, offered, needed, returned);
}

/********************************************************************
 enum_all_printers_info_1_name.
*********************************************************************/

static WERROR enum_all_printers_info_1_name(fstring name, RPC_BUFFER *buffer, uint32 offered, uint32 *needed, uint32 *returned)
{
	char *s = name;
	
	DEBUG(4,("enum_all_printers_info_1_name\n"));	
	
	if ((name[0] == '\\') && (name[1] == '\\'))
		s = name + 2;
		
	if (is_myname_or_ipaddr(s)) {
		return enum_all_printers_info_1(PRINTER_ENUM_ICON8, buffer, offered, needed, returned);
	}
	else
		return WERR_INVALID_NAME;
}

#if 0 	/* JERRY -- disabled for now.  Don't think this is used, tested, or correct */
/********************************************************************
 enum_all_printers_info_1_remote.
*********************************************************************/

static WERROR enum_all_printers_info_1_remote(fstring name, RPC_BUFFER *buffer, uint32 offered, uint32 *needed, uint32 *returned)
{
	PRINTER_INFO_1 *printer;
	fstring printername;
	fstring desc;
	fstring comment;
	DEBUG(4,("enum_all_printers_info_1_remote\n"));	
	WERROR result = WERR_OK;

	/* JFM: currently it's more a place holder than anything else.
	 * In the spooler world there is a notion of server registration.
	 * the print servers are registered on the PDC (in the same domain)
	 *
	 * We should have a TDB here. The registration is done thru an 
	 * undocumented RPC call.
	 */
	
	if((printer=SMB_MALLOC_P(PRINTER_INFO_1)) == NULL)
		return WERR_NOMEM;

	*returned=1;
	
	slprintf(printername, sizeof(printername)-1,"Windows NT Remote Printers!!\\\\%s", name);		
	slprintf(desc, sizeof(desc)-1,"%s", name);
	slprintf(comment, sizeof(comment)-1, "Logged on Domain");

	init_unistr(&printer->description, desc);
	init_unistr(&printer->name, printername);	
	init_unistr(&printer->comment, comment);
	printer->flags=PRINTER_ENUM_ICON3|PRINTER_ENUM_CONTAINER;
		
	/* check the required size. */	
	*needed += spoolss_size_printer_info_1(printer);

	if (*needed > offered) {
		result = WERR_INSUFFICIENT_BUFFER;
		goto out;
	}

	if (!rpcbuf_alloc_size(buffer, *needed)) {
		result = WERR_NOMEM;
		goto out;
	}

	/* fill the buffer with the structures */
	smb_io_printer_info_1("", buffer, printer, 0);	

out:
	/* clear memory */
	SAFE_FREE(printer);

	if ( !W_ERROR_IS_OK(result) )
		*returned = 0;

	return result;
}

#endif

/********************************************************************
 enum_all_printers_info_1_network.
*********************************************************************/

static WERROR enum_all_printers_info_1_network(fstring name, RPC_BUFFER *buffer, uint32 offered, uint32 *needed, uint32 *returned)
{
	char *s = name;

	DEBUG(4,("enum_all_printers_info_1_network\n"));	
	
	/* If we respond to a enum_printers level 1 on our name with flags
	   set to PRINTER_ENUM_REMOTE with a list of printers then these
	   printers incorrectly appear in the APW browse list.
	   Specifically the printers for the server appear at the workgroup
	   level where all the other servers in the domain are
	   listed. Windows responds to this call with a
	   WERR_CAN_NOT_COMPLETE so we should do the same. */ 

	if (name[0] == '\\' && name[1] == '\\')
		 s = name + 2;

	if (is_myname_or_ipaddr(s))
		 return WERR_CAN_NOT_COMPLETE;

	return enum_all_printers_info_1(PRINTER_ENUM_UNKNOWN_8, buffer, offered, needed, returned);
}

/********************************************************************
 * api_spoolss_enumprinters
 *
 * called from api_spoolss_enumprinters (see this to understand)
 ********************************************************************/

static WERROR enum_all_printers_info_2(RPC_BUFFER *buffer, uint32 offered, uint32 *needed, uint32 *returned)
{
	int snum;
	int i;
	int n_services=lp_numservices();
	PRINTER_INFO_2 *printers=NULL;
	PRINTER_INFO_2 current_prt;
	WERROR result = WERR_OK;

	*returned = 0;

	for (snum=0; snum<n_services; snum++) {
		if (lp_browseable(snum) && lp_snum_ok(snum) && lp_print_ok(snum) ) {
			DEBUG(4,("Found a printer in smb.conf: %s[%x]\n", lp_servicename(snum), snum));
				
			if (construct_printer_info_2(NULL, &current_prt, snum)) {
				if ( !(printers=SMB_REALLOC_ARRAY(printers, PRINTER_INFO_2, *returned +1)) ) {
					DEBUG(2,("enum_all_printers_info_2: failed to enlarge printers buffer!\n"));
					*returned = 0;
					return WERR_NOMEM;
				}

				DEBUG(4,("ReAlloced memory for [%d] PRINTER_INFO_2\n", *returned + 1));		

				memcpy(&printers[*returned], &current_prt, sizeof(PRINTER_INFO_2));

				(*returned)++;
			}
		}
	}
	
	/* check the required size. */	
	for (i=0; i<*returned; i++) 
		(*needed) += spoolss_size_printer_info_2(&printers[i]);
	
	if (*needed > offered) {
		result = WERR_INSUFFICIENT_BUFFER;
		goto out;
	}

	if (!rpcbuf_alloc_size(buffer, *needed)) {
		result = WERR_NOMEM;
		goto out;
	}

	/* fill the buffer with the structures */
	for (i=0; i<*returned; i++)
		smb_io_printer_info_2("", buffer, &(printers[i]), 0);	
	
out:
	/* clear memory */

	for (i=0; i<*returned; i++) 
		free_devmode(printers[i].devmode);

	SAFE_FREE(printers);

	if ( !W_ERROR_IS_OK(result) )
		*returned = 0;

	return result;
}

/********************************************************************
 * handle enumeration of printers at level 1
 ********************************************************************/

static WERROR enumprinters_level1( uint32 flags, fstring name,
			         RPC_BUFFER *buffer, uint32 offered,
			         uint32 *needed, uint32 *returned)
{
	/* Not all the flags are equals */

	if (flags & PRINTER_ENUM_LOCAL)
		return enum_all_printers_info_1_local(buffer, offered, needed, returned);

	if (flags & PRINTER_ENUM_NAME)
		return enum_all_printers_info_1_name(name, buffer, offered, needed, returned);

#if 0	/* JERRY - disabled for now */
	if (flags & PRINTER_ENUM_REMOTE)
		return enum_all_printers_info_1_remote(name, buffer, offered, needed, returned);
#endif

	if (flags & PRINTER_ENUM_NETWORK)
		return enum_all_printers_info_1_network(name, buffer, offered, needed, returned);

	return WERR_OK; /* NT4sp5 does that */
}

/********************************************************************
 * handle enumeration of printers at level 2
 ********************************************************************/

static WERROR enumprinters_level2( uint32 flags, fstring servername,
			         RPC_BUFFER *buffer, uint32 offered,
			         uint32 *needed, uint32 *returned)
{
	char *s = servername;

	if (flags & PRINTER_ENUM_LOCAL) {
			return enum_all_printers_info_2(buffer, offered, needed, returned);
	}

	if (flags & PRINTER_ENUM_NAME) {
		if ((servername[0] == '\\') && (servername[1] == '\\'))
			s = servername + 2;
		if (is_myname_or_ipaddr(s))
			return enum_all_printers_info_2(buffer, offered, needed, returned);
		else
			return WERR_INVALID_NAME;
	}

	if (flags & PRINTER_ENUM_REMOTE)
		return WERR_UNKNOWN_LEVEL;

	return WERR_OK;
}

/********************************************************************
 * handle enumeration of printers at level 5
 ********************************************************************/

static WERROR enumprinters_level5( uint32 flags, fstring servername,
			         RPC_BUFFER *buffer, uint32 offered,
			         uint32 *needed, uint32 *returned)
{
/*	return enum_all_printers_info_5(buffer, offered, needed, returned);*/
	return WERR_OK;
}

/********************************************************************
 * api_spoolss_enumprinters
 *
 * called from api_spoolss_enumprinters (see this to understand)
 ********************************************************************/

WERROR _spoolss_enumprinters( pipes_struct *p, SPOOL_Q_ENUMPRINTERS *q_u, SPOOL_R_ENUMPRINTERS *r_u)
{
	uint32 flags = q_u->flags;
	UNISTR2 *servername = &q_u->servername;
	uint32 level = q_u->level;
	RPC_BUFFER *buffer = NULL;
	uint32 offered = q_u->offered;
	uint32 *needed = &r_u->needed;
	uint32 *returned = &r_u->returned;

	fstring name;
	
	/* that's an [in out] buffer */

	if (!q_u->buffer && (offered!=0)) {
		return WERR_INVALID_PARAM;
	}

	rpcbuf_move(q_u->buffer, &r_u->buffer);
	buffer = r_u->buffer;

	DEBUG(4,("_spoolss_enumprinters\n"));

	*needed=0;
	*returned=0;
	
	/*
	 * Level 1:
	 *	    flags==PRINTER_ENUM_NAME
	 *	     if name=="" then enumerates all printers
	 *	     if name!="" then enumerate the printer
	 *	    flags==PRINTER_ENUM_REMOTE
	 *	    name is NULL, enumerate printers
	 * Level 2: name!="" enumerates printers, name can't be NULL
	 * Level 3: doesn't exist
	 * Level 4: does a local registry lookup
	 * Level 5: same as Level 2
	 */

	unistr2_to_ascii(name, servername, sizeof(name)-1);
	strupper_m(name);

	switch (level) {
	case 1:
		return enumprinters_level1(flags, name, buffer, offered, needed, returned);
	case 2:
		return enumprinters_level2(flags, name, buffer, offered, needed, returned);
	case 5:
		return enumprinters_level5(flags, name, buffer, offered, needed, returned);
	case 3:
	case 4:
		break;
	}
	return WERR_UNKNOWN_LEVEL;
}

/****************************************************************************
****************************************************************************/

static WERROR getprinter_level_0(Printer_entry *print_hnd, int snum, RPC_BUFFER *buffer, uint32 offered, uint32 *needed)
{
	PRINTER_INFO_0 *printer=NULL;
	WERROR result = WERR_OK;

	if((printer=SMB_MALLOC_P(PRINTER_INFO_0)) == NULL)
		return WERR_NOMEM;

	construct_printer_info_0(print_hnd, printer, snum);
	
	/* check the required size. */	
	*needed += spoolss_size_printer_info_0(printer);

	if (*needed > offered) {
		result = WERR_INSUFFICIENT_BUFFER;
		goto out;
	}

	if (!rpcbuf_alloc_size(buffer, *needed)) {
		result = WERR_NOMEM;
		goto out;
	}

	/* fill the buffer with the structures */
	smb_io_printer_info_0("", buffer, printer, 0);	
	
out:
	/* clear memory */

	SAFE_FREE(printer);

	return result;
}

/****************************************************************************
****************************************************************************/

static WERROR getprinter_level_1(Printer_entry *print_hnd, int snum, RPC_BUFFER *buffer, uint32 offered, uint32 *needed)
{
	PRINTER_INFO_1 *printer=NULL;
	WERROR result = WERR_OK;

	if((printer=SMB_MALLOC_P(PRINTER_INFO_1)) == NULL)
		return WERR_NOMEM;

	construct_printer_info_1(print_hnd, PRINTER_ENUM_ICON8, printer, snum);
	
	/* check the required size. */	
	*needed += spoolss_size_printer_info_1(printer);

	if (*needed > offered) {
		result = WERR_INSUFFICIENT_BUFFER;
		goto out;
	}

	if (!rpcbuf_alloc_size(buffer, *needed)) {
		result = WERR_NOMEM;
		goto out;
	}

	/* fill the buffer with the structures */
	smb_io_printer_info_1("", buffer, printer, 0);	
	
out:
	/* clear memory */
	SAFE_FREE(printer);

	return result;	
}

/****************************************************************************
****************************************************************************/

static WERROR getprinter_level_2(Printer_entry *print_hnd, int snum, RPC_BUFFER *buffer, uint32 offered, uint32 *needed)
{
	PRINTER_INFO_2 *printer=NULL;
	WERROR result = WERR_OK;

	if((printer=SMB_MALLOC_P(PRINTER_INFO_2))==NULL)
		return WERR_NOMEM;
	
	construct_printer_info_2(print_hnd, printer, snum);
	
	/* check the required size. */	
	*needed += spoolss_size_printer_info_2(printer);
	
	if (*needed > offered) {
		result = WERR_INSUFFICIENT_BUFFER;
		goto out;
	}

	if (!rpcbuf_alloc_size(buffer, *needed)) {
		result = WERR_NOMEM;
		goto out;
	}

	/* fill the buffer with the structures */
	if (!smb_io_printer_info_2("", buffer, printer, 0)) 
		result = WERR_NOMEM;
	
out:
	/* clear memory */
	free_printer_info_2(printer);

	return result;	
}

/****************************************************************************
****************************************************************************/

static WERROR getprinter_level_3(Printer_entry *print_hnd, int snum, RPC_BUFFER *buffer, uint32 offered, uint32 *needed)
{
	PRINTER_INFO_3 *printer=NULL;
	WERROR result = WERR_OK;

	if (!construct_printer_info_3(print_hnd, &printer, snum))
		return WERR_NOMEM;
	
	/* check the required size. */	
	*needed += spoolss_size_printer_info_3(printer);

	if (*needed > offered) {
		result = WERR_INSUFFICIENT_BUFFER;
		goto out;
	}

	if (!rpcbuf_alloc_size(buffer, *needed)) {
		result = WERR_NOMEM;
		goto out;
	}

	/* fill the buffer with the structures */
	smb_io_printer_info_3("", buffer, printer, 0);	
	
out:
	/* clear memory */
	free_printer_info_3(printer);
	
	return result;	
}

/****************************************************************************
****************************************************************************/

static WERROR getprinter_level_4(Printer_entry *print_hnd, int snum, RPC_BUFFER *buffer, uint32 offered, uint32 *needed)
{
	PRINTER_INFO_4 *printer=NULL;
	WERROR result = WERR_OK;

	if((printer=SMB_MALLOC_P(PRINTER_INFO_4))==NULL)
		return WERR_NOMEM;

	if (!construct_printer_info_4(print_hnd, printer, snum)) {
		SAFE_FREE(printer);
		return WERR_NOMEM;
	}
	
	/* check the required size. */	
	*needed += spoolss_size_printer_info_4(printer);

	if (*needed > offered) {
		result = WERR_INSUFFICIENT_BUFFER;
		goto out;
	}

	if (!rpcbuf_alloc_size(buffer, *needed)) {
		result = WERR_NOMEM;
		goto out;
	}

	/* fill the buffer with the structures */
	smb_io_printer_info_4("", buffer, printer, 0);	
	
out:
	/* clear memory */
	free_printer_info_4(printer);
	
	return result;	
}

/****************************************************************************
****************************************************************************/

static WERROR getprinter_level_5(Printer_entry *print_hnd, int snum, RPC_BUFFER *buffer, uint32 offered, uint32 *needed)
{
	PRINTER_INFO_5 *printer=NULL;
	WERROR result = WERR_OK;

	if((printer=SMB_MALLOC_P(PRINTER_INFO_5))==NULL)
		return WERR_NOMEM;

	if (!construct_printer_info_5(print_hnd, printer, snum)) {
		free_printer_info_5(printer);
		return WERR_NOMEM;
	}
	
	/* check the required size. */	
	*needed += spoolss_size_printer_info_5(printer);

	if (*needed > offered) {
		result = WERR_INSUFFICIENT_BUFFER;
		goto out;
	}

	if (!rpcbuf_alloc_size(buffer, *needed)) {
		result = WERR_NOMEM;
		goto out;
	}

	/* fill the buffer with the structures */
	smb_io_printer_info_5("", buffer, printer, 0);	
	
out:
	/* clear memory */
	free_printer_info_5(printer);
	
	return result;	
}

static WERROR getprinter_level_7(Printer_entry *print_hnd, int snum, RPC_BUFFER *buffer, uint32 offered, uint32 *needed)
{
	PRINTER_INFO_7 *printer=NULL;
	WERROR result = WERR_OK;

	if((printer=SMB_MALLOC_P(PRINTER_INFO_7))==NULL)
		return WERR_NOMEM;

	if (!construct_printer_info_7(print_hnd, printer, snum))
		return WERR_NOMEM;
	
	/* check the required size. */	
	*needed += spoolss_size_printer_info_7(printer);

	if (*needed > offered) {
		result = WERR_INSUFFICIENT_BUFFER;
		goto out;
	}

	if (!rpcbuf_alloc_size(buffer, *needed)) {
		result = WERR_NOMEM;
		goto out;

	}

	/* fill the buffer with the structures */
	smb_io_printer_info_7("", buffer, printer, 0);	
	
out:
	/* clear memory */
	free_printer_info_7(printer);
	
	return result;	
}

/****************************************************************************
****************************************************************************/

WERROR _spoolss_getprinter(pipes_struct *p, SPOOL_Q_GETPRINTER *q_u, SPOOL_R_GETPRINTER *r_u)
{
	POLICY_HND *handle = &q_u->handle;
	uint32 level = q_u->level;
	RPC_BUFFER *buffer = NULL;
	uint32 offered = q_u->offered;
	uint32 *needed = &r_u->needed;
	Printer_entry *Printer=find_printer_index_by_hnd(p, handle);

	int snum;

	/* that's an [in out] buffer */

	if (!q_u->buffer && (offered!=0)) {
		return WERR_INVALID_PARAM;
	}

	rpcbuf_move(q_u->buffer, &r_u->buffer);
	buffer = r_u->buffer;

	*needed=0;

	if (!get_printer_snum(p, handle, &snum))
		return WERR_BADFID;

	switch (level) {
	case 0:
		return getprinter_level_0(Printer, snum, buffer, offered, needed);
	case 1:
		return getprinter_level_1(Printer, snum, buffer, offered, needed);
	case 2:		
		return getprinter_level_2(Printer, snum, buffer, offered, needed);
	case 3:		
		return getprinter_level_3(Printer, snum, buffer, offered, needed);
	case 4:		
		return getprinter_level_4(Printer, snum, buffer, offered, needed);
	case 5:		
		return getprinter_level_5(Printer, snum, buffer, offered, needed);
	case 7:
		return getprinter_level_7(Printer, snum, buffer, offered, needed);
	}
	return WERR_UNKNOWN_LEVEL;
}	
		
/********************************************************************
 * fill a DRIVER_INFO_1 struct
 ********************************************************************/

static void fill_printer_driver_info_1(DRIVER_INFO_1 *info, NT_PRINTER_DRIVER_INFO_LEVEL driver, fstring servername, fstring architecture)
{
	init_unistr( &info->name, driver.info_3->name);
}

/********************************************************************
 * construct_printer_driver_info_1
 ********************************************************************/

static WERROR construct_printer_driver_info_1(DRIVER_INFO_1 *info, int snum, fstring servername, fstring architecture, uint32 version)
{	
	NT_PRINTER_INFO_LEVEL *printer = NULL;
	NT_PRINTER_DRIVER_INFO_LEVEL driver;

	ZERO_STRUCT(driver);

	if (!W_ERROR_IS_OK(get_a_printer(NULL, &printer, 2, lp_const_servicename(snum))))
		return WERR_INVALID_PRINTER_NAME;

	if (!W_ERROR_IS_OK(get_a_printer_driver(&driver, 3, printer->info_2->drivername, architecture, version))) {
		free_a_printer(&printer, 2);
		return WERR_UNKNOWN_PRINTER_DRIVER;
	}

	fill_printer_driver_info_1(info, driver, servername, architecture);

	free_a_printer(&printer,2);

	return WERR_OK;
}

/********************************************************************
 * construct_printer_driver_info_2
 * fill a printer_info_2 struct
 ********************************************************************/

static void fill_printer_driver_info_2(DRIVER_INFO_2 *info, NT_PRINTER_DRIVER_INFO_LEVEL driver, fstring servername)
{
	pstring temp;

	info->version=driver.info_3->cversion;

	init_unistr( &info->name, driver.info_3->name );
	init_unistr( &info->architecture, driver.info_3->environment );


    if (strlen(driver.info_3->driverpath)) {
		slprintf(temp, sizeof(temp)-1, "\\\\%s%s", servername, driver.info_3->driverpath);
		init_unistr( &info->driverpath, temp );
    } else
        init_unistr( &info->driverpath, "" );

	if (strlen(driver.info_3->datafile)) {
		slprintf(temp, sizeof(temp)-1, "\\\\%s%s", servername, driver.info_3->datafile);
		init_unistr( &info->datafile, temp );
	} else
		init_unistr( &info->datafile, "" );
	
	if (strlen(driver.info_3->configfile)) {
		slprintf(temp, sizeof(temp)-1, "\\\\%s%s", servername, driver.info_3->configfile);
		init_unistr( &info->configfile, temp );	
	} else
		init_unistr( &info->configfile, "" );
}

/********************************************************************
 * construct_printer_driver_info_2
 * fill a printer_info_2 struct
 ********************************************************************/

static WERROR construct_printer_driver_info_2(DRIVER_INFO_2 *info, int snum, fstring servername, fstring architecture, uint32 version)
{
	NT_PRINTER_INFO_LEVEL *printer = NULL;
	NT_PRINTER_DRIVER_INFO_LEVEL driver;

	ZERO_STRUCT(printer);
	ZERO_STRUCT(driver);

	if (!W_ERROR_IS_OK(get_a_printer(NULL, &printer, 2, lp_const_servicename(snum))))
		return WERR_INVALID_PRINTER_NAME;

	if (!W_ERROR_IS_OK(get_a_printer_driver(&driver, 3, printer->info_2->drivername, architecture, version))) {
		free_a_printer(&printer, 2);
		return WERR_UNKNOWN_PRINTER_DRIVER;
	}

	fill_printer_driver_info_2(info, driver, servername);

	free_a_printer(&printer,2);

	return WERR_OK;
}

/********************************************************************
 * copy a strings array and convert to UNICODE
 *
 * convert an array of ascii string to a UNICODE string
 ********************************************************************/

static uint32 init_unistr_array(uint16 **uni_array, fstring *char_array, const char *servername)
{
	int i=0;
	int j=0;
	const char *v;
	pstring line;

	DEBUG(6,("init_unistr_array\n"));
	*uni_array=NULL;

	while (True) 
	{
		if ( !char_array )
			v = "";
		else 
		{
			v = char_array[i];
			if (!v) 
				v = ""; /* hack to handle null lists */
		}
		
		/* hack to allow this to be used in places other than when generating 
		   the list of dependent files */
		   
		if ( servername )
			slprintf( line, sizeof(line)-1, "\\\\%s%s", servername, v );
		else
			pstrcpy( line, v );
			
		DEBUGADD(6,("%d:%s:%lu\n", i, line, (unsigned long)strlen(line)));

		/* add one extra unit16 for the second terminating NULL */
		
		if ( (*uni_array=SMB_REALLOC_ARRAY(*uni_array, uint16, j+1+strlen(line)+2)) == NULL ) {
			DEBUG(2,("init_unistr_array: Realloc error\n" ));
			return 0;
		}

		if ( !strlen(v) ) 
			break;
		
		j += (rpcstr_push((*uni_array+j), line, sizeof(uint16)*strlen(line)+2, STR_TERMINATE) / sizeof(uint16));
		i++;
	}
	
	if (*uni_array) {
		/* special case for ""; we need to add both NULL's here */
		if (!j)
			(*uni_array)[j++]=0x0000;	
		(*uni_array)[j]=0x0000;
	}
	
	DEBUGADD(6,("last one:done\n"));

	/* return size of array in uint16's */
		
	return j+1;
}

/********************************************************************
 * construct_printer_info_3
 * fill a printer_info_3 struct
 ********************************************************************/

static void fill_printer_driver_info_3(DRIVER_INFO_3 *info, NT_PRINTER_DRIVER_INFO_LEVEL driver, fstring servername)
{
	pstring temp;

	ZERO_STRUCTP(info);

	info->version=driver.info_3->cversion;

	init_unistr( &info->name, driver.info_3->name );	
	init_unistr( &info->architecture, driver.info_3->environment );

	if (strlen(driver.info_3->driverpath)) {
		slprintf(temp, sizeof(temp)-1, "\\\\%s%s", servername, driver.info_3->driverpath);		
		init_unistr( &info->driverpath, temp );
	} else
		init_unistr( &info->driverpath, "" );
    
	if (strlen(driver.info_3->datafile)) {
		slprintf(temp, sizeof(temp)-1, "\\\\%s%s", servername, driver.info_3->datafile);
		init_unistr( &info->datafile, temp );
	} else
		init_unistr( &info->datafile, "" );

	if (strlen(driver.info_3->configfile)) {
		slprintf(temp, sizeof(temp)-1, "\\\\%s%s", servername, driver.info_3->configfile);
		init_unistr( &info->configfile, temp );	
	} else
		init_unistr( &info->configfile, "" );

	if (strlen(driver.info_3->helpfile)) {
		slprintf(temp, sizeof(temp)-1, "\\\\%s%s", servername, driver.info_3->helpfile);
		init_unistr( &info->helpfile, temp );
	} else
		init_unistr( &info->helpfile, "" );

	init_unistr( &info->monitorname, driver.info_3->monitorname );
	init_unistr( &info->defaultdatatype, driver.info_3->defaultdatatype );

	info->dependentfiles=NULL;
	init_unistr_array(&info->dependentfiles, driver.info_3->dependentfiles, servername);
}

/********************************************************************
 * construct_printer_info_3
 * fill a printer_info_3 struct
 ********************************************************************/

static WERROR construct_printer_driver_info_3(DRIVER_INFO_3 *info, int snum, fstring servername, fstring architecture, uint32 version)
{	
	NT_PRINTER_INFO_LEVEL *printer = NULL;
	NT_PRINTER_DRIVER_INFO_LEVEL driver;
	WERROR status;
	ZERO_STRUCT(driver);

	status=get_a_printer(NULL, &printer, 2, lp_const_servicename(snum) );
	DEBUG(8,("construct_printer_driver_info_3: status: %s\n", dos_errstr(status)));
	if (!W_ERROR_IS_OK(status))
		return WERR_INVALID_PRINTER_NAME;

	status=get_a_printer_driver(&driver, 3, printer->info_2->drivername, architecture, version);	
	DEBUG(8,("construct_printer_driver_info_3: status: %s\n", dos_errstr(status)));

#if 0	/* JERRY */

	/* 
	 * I put this code in during testing.  Helpful when commenting out the 
	 * support for DRIVER_INFO_6 in regards to win2k.  Not needed in general
	 * as win2k always queries the driver using an infor level of 6.
	 * I've left it in (but ifdef'd out) because I'll probably
	 * use it in experimentation again in the future.   --jerry 22/01/2002
	 */

	if (!W_ERROR_IS_OK(status)) {
		/*
		 * Is this a W2k client ?
		 */
		if (version == 3) {
			/* Yes - try again with a WinNT driver. */
			version = 2;
			status=get_a_printer_driver(&driver, 3, printer->info_2->drivername, architecture, version);	
			DEBUG(8,("construct_printer_driver_info_3: status: %s\n", dos_errstr(status)));
		}
#endif

		if (!W_ERROR_IS_OK(status)) {
			free_a_printer(&printer,2);
			return WERR_UNKNOWN_PRINTER_DRIVER;
		}
		
#if 0	/* JERRY */
	}
#endif
	

	fill_printer_driver_info_3(info, driver, servername);

	free_a_printer(&printer,2);

	return WERR_OK;
}

/********************************************************************
 * construct_printer_info_6
 * fill a printer_info_6 struct - we know that driver is really level 3. This sucks. JRA.
 ********************************************************************/

static void fill_printer_driver_info_6(DRIVER_INFO_6 *info, NT_PRINTER_DRIVER_INFO_LEVEL driver, fstring servername)
{
	pstring temp;
	fstring nullstr;

	ZERO_STRUCTP(info);
	memset(&nullstr, '\0', sizeof(fstring));

	info->version=driver.info_3->cversion;

	init_unistr( &info->name, driver.info_3->name );	
	init_unistr( &info->architecture, driver.info_3->environment );

	if (strlen(driver.info_3->driverpath)) {
		slprintf(temp, sizeof(temp)-1, "\\\\%s%s", servername, driver.info_3->driverpath);		
		init_unistr( &info->driverpath, temp );
	} else
		init_unistr( &info->driverpath, "" );

	if (strlen(driver.info_3->datafile)) {
		slprintf(temp, sizeof(temp)-1, "\\\\%s%s", servername, driver.info_3->datafile);
		init_unistr( &info->datafile, temp );
	} else
		init_unistr( &info->datafile, "" );

	if (strlen(driver.info_3->configfile)) {
		slprintf(temp, sizeof(temp)-1, "\\\\%s%s", servername, driver.info_3->configfile);
		init_unistr( &info->configfile, temp );	
	} else
		init_unistr( &info->configfile, "" );

	if (strlen(driver.info_3->helpfile)) {
		slprintf(temp, sizeof(temp)-1, "\\\\%s%s", servername, driver.info_3->helpfile);
		init_unistr( &info->helpfile, temp );
	} else
		init_unistr( &info->helpfile, "" );
	
	init_unistr( &info->monitorname, driver.info_3->monitorname );
	init_unistr( &info->defaultdatatype, driver.info_3->defaultdatatype );

	info->dependentfiles = NULL;
	init_unistr_array( &info->dependentfiles, driver.info_3->dependentfiles, servername );

	info->previousdrivernames=NULL;
	init_unistr_array(&info->previousdrivernames, &nullstr, servername);

	info->driver_date.low=0;
	info->driver_date.high=0;

	info->padding=0;
	info->driver_version_low=0;
	info->driver_version_high=0;

	init_unistr( &info->mfgname, "");
	init_unistr( &info->oem_url, "");
	init_unistr( &info->hardware_id, "");
	init_unistr( &info->provider, "");
}

/********************************************************************
 * construct_printer_info_6
 * fill a printer_info_6 struct
 ********************************************************************/

static WERROR construct_printer_driver_info_6(DRIVER_INFO_6 *info, int snum, 
              fstring servername, fstring architecture, uint32 version)
{	
	NT_PRINTER_INFO_LEVEL 		*printer = NULL;
	NT_PRINTER_DRIVER_INFO_LEVEL 	driver;
	WERROR 				status;
	
	ZERO_STRUCT(driver);

	status=get_a_printer(NULL, &printer, 2, lp_const_servicename(snum) );
	
	DEBUG(8,("construct_printer_driver_info_6: status: %s\n", dos_errstr(status)));
	
	if (!W_ERROR_IS_OK(status))
		return WERR_INVALID_PRINTER_NAME;

	status = get_a_printer_driver(&driver, 3, printer->info_2->drivername, architecture, version);
		
	DEBUG(8,("construct_printer_driver_info_6: status: %s\n", dos_errstr(status)));
	
	if (!W_ERROR_IS_OK(status)) 
	{
		/*
		 * Is this a W2k client ?
		 */

		if (version < 3) {
			free_a_printer(&printer,2);
			return WERR_UNKNOWN_PRINTER_DRIVER;
		}

		/* Yes - try again with a WinNT driver. */
		version = 2;
		status=get_a_printer_driver(&driver, 3, printer->info_2->drivername, architecture, version);	
		DEBUG(8,("construct_printer_driver_info_6: status: %s\n", dos_errstr(status)));
		if (!W_ERROR_IS_OK(status)) {
			free_a_printer(&printer,2);
			return WERR_UNKNOWN_PRINTER_DRIVER;
		}
	}

	fill_printer_driver_info_6(info, driver, servername);

	free_a_printer(&printer,2);
	free_a_printer_driver(driver, 3);

	return WERR_OK;
}

/****************************************************************************
****************************************************************************/

static void free_printer_driver_info_3(DRIVER_INFO_3 *info)
{
	SAFE_FREE(info->dependentfiles);
}

/****************************************************************************
****************************************************************************/

static void free_printer_driver_info_6(DRIVER_INFO_6 *info)
{
	SAFE_FREE(info->dependentfiles);
}

/****************************************************************************
****************************************************************************/

static WERROR getprinterdriver2_level1(fstring servername, fstring architecture, uint32 version, int snum, RPC_BUFFER *buffer, uint32 offered, uint32 *needed)
{
	DRIVER_INFO_1 *info=NULL;
	WERROR result;
	
	if((info=SMB_MALLOC_P(DRIVER_INFO_1)) == NULL)
		return WERR_NOMEM;
	
	result = construct_printer_driver_info_1(info, snum, servername, architecture, version);
	if (!W_ERROR_IS_OK(result)) 
		goto out;

	/* check the required size. */	
	*needed += spoolss_size_printer_driver_info_1(info);

	if (*needed > offered) {
		result = WERR_INSUFFICIENT_BUFFER;
		goto out;
	}

	if (!rpcbuf_alloc_size(buffer, *needed)) {
		result = WERR_NOMEM;
		goto out;
	}

	/* fill the buffer with the structures */
	smb_io_printer_driver_info_1("", buffer, info, 0);	

out:
	/* clear memory */
	SAFE_FREE(info);

	return result;
}

/****************************************************************************
****************************************************************************/

static WERROR getprinterdriver2_level2(fstring servername, fstring architecture, uint32 version, int snum, RPC_BUFFER *buffer, uint32 offered, uint32 *needed)
{
	DRIVER_INFO_2 *info=NULL;
	WERROR result;
	
	if((info=SMB_MALLOC_P(DRIVER_INFO_2)) == NULL)
		return WERR_NOMEM;
	
	result = construct_printer_driver_info_2(info, snum, servername, architecture, version);
	if (!W_ERROR_IS_OK(result)) 
		goto out;

	/* check the required size. */	
	*needed += spoolss_size_printer_driver_info_2(info);

	if (*needed > offered) {
		result = WERR_INSUFFICIENT_BUFFER;
		goto out;
	}
	
	if (!rpcbuf_alloc_size(buffer, *needed)) {
		result = WERR_NOMEM;
		goto out;
	}

	/* fill the buffer with the structures */
	smb_io_printer_driver_info_2("", buffer, info, 0);	

out:
	/* clear memory */
	SAFE_FREE(info);

	return result;
}

/****************************************************************************
****************************************************************************/

static WERROR getprinterdriver2_level3(fstring servername, fstring architecture, uint32 version, int snum, RPC_BUFFER *buffer, uint32 offered, uint32 *needed)
{
	DRIVER_INFO_3 info;
	WERROR result;

	ZERO_STRUCT(info);

	result = construct_printer_driver_info_3(&info, snum, servername, architecture, version);
	if (!W_ERROR_IS_OK(result))
		goto out;

	/* check the required size. */	
	*needed += spoolss_size_printer_driver_info_3(&info);

	if (*needed > offered) {
		result = WERR_INSUFFICIENT_BUFFER;
		goto out;
	}

	if (!rpcbuf_alloc_size(buffer, *needed)) {
		result = WERR_NOMEM;
		goto out;
	}

	/* fill the buffer with the structures */
	smb_io_printer_driver_info_3("", buffer, &info, 0);

out:
	free_printer_driver_info_3(&info);

	return result;
}

/****************************************************************************
****************************************************************************/

static WERROR getprinterdriver2_level6(fstring servername, fstring architecture, uint32 version, int snum, RPC_BUFFER *buffer, uint32 offered, uint32 *needed)
{
	DRIVER_INFO_6 info;
	WERROR result;

	ZERO_STRUCT(info);

	result = construct_printer_driver_info_6(&info, snum, servername, architecture, version);
	if (!W_ERROR_IS_OK(result)) 
		goto out;

	/* check the required size. */	
	*needed += spoolss_size_printer_driver_info_6(&info);

	if (*needed > offered) {
		result = WERR_INSUFFICIENT_BUFFER;
		goto out;
	}
	
	if (!rpcbuf_alloc_size(buffer, *needed)) {
		result = WERR_NOMEM;
		goto out;
	}

	/* fill the buffer with the structures */
	smb_io_printer_driver_info_6("", buffer, &info, 0);

out:
	free_printer_driver_info_6(&info);

	return result;
}

/****************************************************************************
****************************************************************************/

WERROR _spoolss_getprinterdriver2(pipes_struct *p, SPOOL_Q_GETPRINTERDRIVER2 *q_u, SPOOL_R_GETPRINTERDRIVER2 *r_u)
{
	POLICY_HND *handle = &q_u->handle;
	UNISTR2 *uni_arch = &q_u->architecture;
	uint32 level = q_u->level;
	uint32 clientmajorversion = q_u->clientmajorversion;
	RPC_BUFFER *buffer = NULL;
	uint32 offered = q_u->offered;
	uint32 *needed = &r_u->needed;
	uint32 *servermajorversion = &r_u->servermajorversion;
	uint32 *serverminorversion = &r_u->serverminorversion;
	Printer_entry *printer;

	fstring servername;
	fstring architecture;
	int snum;

	/* that's an [in out] buffer */

	if (!q_u->buffer && (offered!=0)) {
		return WERR_INVALID_PARAM;
	}

	rpcbuf_move(q_u->buffer, &r_u->buffer);
	buffer = r_u->buffer;

	DEBUG(4,("_spoolss_getprinterdriver2\n"));

	if ( !(printer = find_printer_index_by_hnd( p, handle )) ) {
		DEBUG(0,("_spoolss_getprinterdriver2: invalid printer handle!\n"));
		return WERR_INVALID_PRINTER_NAME;
	}

	*needed = 0;
	*servermajorversion = 0;
	*serverminorversion = 0;

	fstrcpy(servername, get_server_name( printer ));
	unistr2_to_ascii(architecture, uni_arch, sizeof(architecture)-1);

	if (!get_printer_snum(p, handle, &snum))
		return WERR_BADFID;

	switch (level) {
	case 1:
		return getprinterdriver2_level1(servername, architecture, clientmajorversion, snum, buffer, offered, needed);
	case 2:
		return getprinterdriver2_level2(servername, architecture, clientmajorversion, snum, buffer, offered, needed);
	case 3:
		return getprinterdriver2_level3(servername, architecture, clientmajorversion, snum, buffer, offered, needed);
	case 6:
		return getprinterdriver2_level6(servername, architecture, clientmajorversion, snum, buffer, offered, needed);
#if 0	/* JERRY */
	case 101: 
		/* apparently this call is the equivalent of 
		   EnumPrinterDataEx() for the DsDriver key */
		break;
#endif
	}

	return WERR_UNKNOWN_LEVEL;
}

/****************************************************************************
****************************************************************************/

WERROR _spoolss_startpageprinter(pipes_struct *p, SPOOL_Q_STARTPAGEPRINTER *q_u, SPOOL_R_STARTPAGEPRINTER *r_u)
{
	POLICY_HND *handle = &q_u->handle;

	Printer_entry *Printer = find_printer_index_by_hnd(p, handle);

	if (!Printer) {
		DEBUG(3,("Error in startpageprinter printer handle\n"));
		return WERR_BADFID;
	}

	Printer->page_started=True;
	return WERR_OK;
}

/****************************************************************************
****************************************************************************/

WERROR _spoolss_endpageprinter(pipes_struct *p, SPOOL_Q_ENDPAGEPRINTER *q_u, SPOOL_R_ENDPAGEPRINTER *r_u)
{
	POLICY_HND *handle = &q_u->handle;
	int snum;

	Printer_entry *Printer = find_printer_index_by_hnd(p, handle);

	if (!Printer) {
		DEBUG(2,("_spoolss_endpageprinter: Invalid handle (%s:%u:%u).\n",OUR_HANDLE(handle)));
		return WERR_BADFID;
	}
	
	if (!get_printer_snum(p, handle, &snum))
		return WERR_BADFID;

	Printer->page_started=False;
	print_job_endpage(snum, Printer->jobid);

	return WERR_OK;
}

/********************************************************************
 * api_spoolss_getprinter
 * called from the spoolss dispatcher
 *
 ********************************************************************/

WERROR _spoolss_startdocprinter(pipes_struct *p, SPOOL_Q_STARTDOCPRINTER *q_u, SPOOL_R_STARTDOCPRINTER *r_u)
{
	POLICY_HND *handle = &q_u->handle;
	DOC_INFO *docinfo = &q_u->doc_info_container.docinfo;
	uint32 *jobid = &r_u->jobid;

	DOC_INFO_1 *info_1 = &docinfo->doc_info_1;
	int snum;
	pstring jobname;
	fstring datatype;
	Printer_entry *Printer = find_printer_index_by_hnd(p, handle);
	struct current_user user;

	if (!Printer) {
		DEBUG(2,("_spoolss_startdocprinter: Invalid handle (%s:%u:%u)\n", OUR_HANDLE(handle)));
		return WERR_BADFID;
	}

	get_current_user(&user, p);

	/*
	 * a nice thing with NT is it doesn't listen to what you tell it.
	 * when asked to send _only_ RAW datas, it tries to send datas
	 * in EMF format.
	 *
	 * So I add checks like in NT Server ...
	 */
	
	if (info_1->p_datatype != 0) {
		unistr2_to_ascii(datatype, &info_1->datatype, sizeof(datatype));
		if (strcmp(datatype, "RAW") != 0) {
			(*jobid)=0;
			return WERR_INVALID_DATATYPE;
		}		
	}		
	
	/* get the share number of the printer */
	if (!get_printer_snum(p, handle, &snum)) {
		return WERR_BADFID;
	}

	unistr2_to_ascii(jobname, &info_1->docname, sizeof(jobname));
	
	Printer->jobid = print_job_start(&user, snum, jobname, Printer->nt_devmode);

	/* An error occured in print_job_start() so return an appropriate
	   NT error code. */

	if (Printer->jobid == -1) {
		return map_werror_from_unix(errno);
	}
	
	Printer->document_started=True;
	(*jobid) = Printer->jobid;

	return WERR_OK;
}

/********************************************************************
 * api_spoolss_getprinter
 * called from the spoolss dispatcher
 *
 ********************************************************************/

WERROR _spoolss_enddocprinter(pipes_struct *p, SPOOL_Q_ENDDOCPRINTER *q_u, SPOOL_R_ENDDOCPRINTER *r_u)
{
	POLICY_HND *handle = &q_u->handle;

	return _spoolss_enddocprinter_internal(p, handle);
}

/****************************************************************************
****************************************************************************/

WERROR _spoolss_writeprinter(pipes_struct *p, SPOOL_Q_WRITEPRINTER *q_u, SPOOL_R_WRITEPRINTER *r_u)
{
	POLICY_HND *handle = &q_u->handle;
	uint32 buffer_size = q_u->buffer_size;
	uint8 *buffer = q_u->buffer;
	uint32 *buffer_written = &q_u->buffer_size2;
	int snum;
	Printer_entry *Printer = find_printer_index_by_hnd(p, handle);
	
	if (!Printer) {
		DEBUG(2,("_spoolss_writeprinter: Invalid handle (%s:%u:%u)\n",OUR_HANDLE(handle)));
		r_u->buffer_written = q_u->buffer_size2;
		return WERR_BADFID;
	}

	if (!get_printer_snum(p, handle, &snum))
		return WERR_BADFID;

	(*buffer_written) = (uint32)print_job_write(snum, Printer->jobid, (const char *)buffer,
					(SMB_OFF_T)-1, (size_t)buffer_size);
	if (*buffer_written == (uint32)-1) {
		r_u->buffer_written = 0;
		if (errno == ENOSPC)
			return WERR_NO_SPOOL_SPACE;
		else
			return WERR_ACCESS_DENIED;
	}

	r_u->buffer_written = q_u->buffer_size2;

	return WERR_OK;
}

/********************************************************************
 * api_spoolss_getprinter
 * called from the spoolss dispatcher
 *
 ********************************************************************/

static WERROR control_printer(POLICY_HND *handle, uint32 command,
			      pipes_struct *p)
{
	struct current_user user;
	int snum;
	WERROR errcode = WERR_BADFUNC;
	Printer_entry *Printer = find_printer_index_by_hnd(p, handle);

	get_current_user(&user, p);

	if (!Printer) {
		DEBUG(2,("control_printer: Invalid handle (%s:%u:%u)\n", OUR_HANDLE(handle)));
		return WERR_BADFID;
	}

	if (!get_printer_snum(p, handle, &snum))
		return WERR_BADFID;

	switch (command) {
	case PRINTER_CONTROL_PAUSE:
		if (print_queue_pause(&user, snum, &errcode)) {
			errcode = WERR_OK;
		}
		break;
	case PRINTER_CONTROL_RESUME:
	case PRINTER_CONTROL_UNPAUSE:
		if (print_queue_resume(&user, snum, &errcode)) {
			errcode = WERR_OK;
		}
		break;
	case PRINTER_CONTROL_PURGE:
		if (print_queue_purge(&user, snum, &errcode)) {
			errcode = WERR_OK;
		}
		break;
	default:
		return WERR_UNKNOWN_LEVEL;
	}

	return errcode;
}

/********************************************************************
 * api_spoolss_abortprinter
 * From MSDN: "Deletes printer's spool file if printer is configured
 * for spooling"
 ********************************************************************/

WERROR _spoolss_abortprinter(pipes_struct *p, SPOOL_Q_ABORTPRINTER *q_u, SPOOL_R_ABORTPRINTER *r_u)
{
	POLICY_HND	*handle = &q_u->handle;
	Printer_entry 	*Printer = find_printer_index_by_hnd(p, handle);
	int		snum;
	struct 		current_user user;
	WERROR 		errcode = WERR_OK;
	
	if (!Printer) {
		DEBUG(2,("_spoolss_abortprinter: Invalid handle (%s:%u:%u)\n",OUR_HANDLE(handle)));
		return WERR_BADFID;
	}
	
	if (!get_printer_snum(p, handle, &snum))
		return WERR_BADFID;
	
	get_current_user( &user, p );	
	
	print_job_delete( &user, snum, Printer->jobid, &errcode );	
	
	return errcode;
}

/********************************************************************
 * called by spoolss_api_setprinter
 * when updating a printer description
 ********************************************************************/

static WERROR update_printer_sec(POLICY_HND *handle, uint32 level,
				 const SPOOL_PRINTER_INFO_LEVEL *info,
				 pipes_struct *p, SEC_DESC_BUF *secdesc_ctr)
{
	SEC_DESC_BUF *new_secdesc_ctr = NULL, *old_secdesc_ctr = NULL;
	WERROR result;
	int snum;

	Printer_entry *Printer = find_printer_index_by_hnd(p, handle);

	if (!Printer || !get_printer_snum(p, handle, &snum)) {
		DEBUG(2,("update_printer_sec: Invalid handle (%s:%u:%u)\n",
			 OUR_HANDLE(handle)));

		result = WERR_BADFID;
		goto done;
	}
	
	/* Check the user has permissions to change the security
	   descriptor.  By experimentation with two NT machines, the user
	   requires Full Access to the printer to change security
	   information. */

	if ( Printer->access_granted != PRINTER_ACCESS_ADMINISTER ) {
		DEBUG(4,("update_printer_sec: updated denied by printer permissions\n"));
		result = WERR_ACCESS_DENIED;
		goto done;
	}

	/* NT seems to like setting the security descriptor even though
	   nothing may have actually changed. */

	nt_printing_getsec(p->mem_ctx, Printer->sharename, &old_secdesc_ctr);

	if (DEBUGLEVEL >= 10) {
		SEC_ACL *the_acl;
		int i;

		the_acl = old_secdesc_ctr->sec->dacl;
		DEBUG(10, ("old_secdesc_ctr for %s has %d aces:\n", 
			   PRINTERNAME(snum), the_acl->num_aces));

		for (i = 0; i < the_acl->num_aces; i++) {
			fstring sid_str;

			sid_to_string(sid_str, &the_acl->ace[i].trustee);

			DEBUG(10, ("%s 0x%08x\n", sid_str, 
				  the_acl->ace[i].info.mask));
		}

		the_acl = secdesc_ctr->sec->dacl;

		if (the_acl) {
			DEBUG(10, ("secdesc_ctr for %s has %d aces:\n", 
				   PRINTERNAME(snum), the_acl->num_aces));

			for (i = 0; i < the_acl->num_aces; i++) {
				fstring sid_str;
				
				sid_to_string(sid_str, &the_acl->ace[i].trustee);
				
				DEBUG(10, ("%s 0x%08x\n", sid_str, 
					   the_acl->ace[i].info.mask));
			}
		} else {
			DEBUG(10, ("dacl for secdesc_ctr is NULL\n"));
		}
	}

	new_secdesc_ctr = sec_desc_merge(p->mem_ctx, secdesc_ctr, old_secdesc_ctr);
	if (!new_secdesc_ctr) {
		result = WERR_NOMEM;
		goto done;
	}

	if (sec_desc_equal(new_secdesc_ctr->sec, old_secdesc_ctr->sec)) {
		result = WERR_OK;
		goto done;
	}

	result = nt_printing_setsec(Printer->sharename, new_secdesc_ctr);

 done:

	return result;
}

/********************************************************************
 Canonicalize printer info from a client

 ATTN: It does not matter what we set the servername to hear 
 since we do the necessary work in get_a_printer() to set it to 
 the correct value based on what the client sent in the 
 _spoolss_open_printer_ex().
 ********************************************************************/

static BOOL check_printer_ok(NT_PRINTER_INFO_LEVEL_2 *info, int snum)
{
	fstring printername;
	const char *p;
	
	DEBUG(5,("check_printer_ok: servername=%s printername=%s sharename=%s "
		"portname=%s drivername=%s comment=%s location=%s\n",
		info->servername, info->printername, info->sharename, 
		info->portname, info->drivername, info->comment, info->location));

	/* we force some elements to "correct" values */
	slprintf(info->servername, sizeof(info->servername)-1, "\\\\%s", global_myname());
	fstrcpy(info->sharename, lp_servicename(snum));
	
	/* check to see if we allow printername != sharename */

	if ( lp_force_printername(snum) ) {
		slprintf(info->printername, sizeof(info->printername)-1, "\\\\%s\\%s",
			global_myname(), info->sharename );
	} else {

		/* make sure printername is in \\server\printername format */
	
		fstrcpy( printername, info->printername );
		p = printername;
		if ( printername[0] == '\\' && printername[1] == '\\' ) {
			if ( (p = strchr_m( &printername[2], '\\' )) != NULL )
				p++;
		}
		
		slprintf(info->printername, sizeof(info->printername)-1, "\\\\%s\\%s",
			 global_myname(), p );
	}

	info->attributes |= PRINTER_ATTRIBUTE_SAMBA;
	info->attributes &= ~PRINTER_ATTRIBUTE_NOT_SAMBA;
	
	
	
	return True;
}

/****************************************************************************
****************************************************************************/

WERROR add_port_hook(NT_USER_TOKEN *token, const char *portname, const char *uri )
{
	char *cmd = lp_addport_cmd();
	pstring command;
	int ret;
	int fd;
	SE_PRIV se_printop = SE_PRINT_OPERATOR;
	BOOL is_print_op = False;

	if ( !*cmd ) {
		return WERR_ACCESS_DENIED;
	}
		
	slprintf(command, sizeof(command)-1, "%s \"%s\" \"%s\"", cmd, portname, uri );

	if ( token )
		is_print_op = user_has_privileges( token, &se_printop );

	DEBUG(10,("Running [%s]\n", command));

	/********* BEGIN SePrintOperatorPrivilege **********/

	if ( is_print_op )
		become_root();
	
	ret = smbrun(command, &fd);

	if ( is_print_op )
		unbecome_root();

	/********* END SePrintOperatorPrivilege **********/

	DEBUGADD(10,("returned [%d]\n", ret));

	if ( ret != 0 ) {
		if (fd != -1)
			close(fd);
		return WERR_ACCESS_DENIED;
	}
	
	return WERR_OK;
}

/****************************************************************************
****************************************************************************/

BOOL add_printer_hook(NT_USER_TOKEN *token, NT_PRINTER_INFO_LEVEL *printer)
{
	char *cmd = lp_addprinter_cmd();
	char **qlines;
	pstring command;
	int numlines;
	int ret;
	int fd;
	fstring remote_machine = "%m";
	SE_PRIV se_printop = SE_PRINT_OPERATOR;
	BOOL is_print_op = False;

	standard_sub_basic(current_user_info.smb_name, remote_machine,sizeof(remote_machine));
	
	slprintf(command, sizeof(command)-1, "%s \"%s\" \"%s\" \"%s\" \"%s\" \"%s\" \"%s\" \"%s\"",
			cmd, printer->info_2->printername, printer->info_2->sharename,
			printer->info_2->portname, printer->info_2->drivername,
			printer->info_2->location, printer->info_2->comment, remote_machine);

	if ( token )
		is_print_op = user_has_privileges( token, &se_printop );

	DEBUG(10,("Running [%s]\n", command));

	/********* BEGIN SePrintOperatorPrivilege **********/

	if ( is_print_op )
		become_root();
	
	if ( (ret = smbrun(command, &fd)) == 0 ) {
		/* Tell everyone we updated smb.conf. */
		message_send_all(conn_tdb_ctx(), MSG_SMB_CONF_UPDATED, NULL, 0, False, NULL);
	}

	if ( is_print_op )
		unbecome_root();

	/********* END SePrintOperatorPrivilege **********/

	DEBUGADD(10,("returned [%d]\n", ret));

	if ( ret != 0 ) {
		if (fd != -1)
			close(fd);
		return False;
	}

	/* reload our services immediately */
	reload_services( False );

	numlines = 0;
	/* Get lines and convert them back to dos-codepage */
	qlines = fd_lines_load(fd, &numlines, 0);
	DEBUGADD(10,("Lines returned = [%d]\n", numlines));
	close(fd);

	/* Set the portname to what the script says the portname should be. */
	/* but don't require anything to be return from the script exit a good error code */

	if (numlines) {
		/* Set the portname to what the script says the portname should be. */
		strncpy(printer->info_2->portname, qlines[0], sizeof(printer->info_2->portname));
		DEBUGADD(6,("Line[0] = [%s]\n", qlines[0]));
	}

	file_lines_free(qlines);
	return True;
}


/********************************************************************
 * Called by spoolss_api_setprinter
 * when updating a printer description.
 ********************************************************************/

static WERROR update_printer(pipes_struct *p, POLICY_HND *handle, uint32 level,
                           const SPOOL_PRINTER_INFO_LEVEL *info,
                           DEVICEMODE *devmode)
{
	int snum;
	NT_PRINTER_INFO_LEVEL *printer = NULL, *old_printer = NULL;
	Printer_entry *Printer = find_printer_index_by_hnd(p, handle);
	WERROR result;
	UNISTR2 buffer;
	fstring asc_buffer;

	DEBUG(8,("update_printer\n"));

	result = WERR_OK;

	if (!Printer) {
		result = WERR_BADFID;
		goto done;
	}

	if (!get_printer_snum(p, handle, &snum)) {
		result = WERR_BADFID;
		goto done;
	}

	if (!W_ERROR_IS_OK(get_a_printer(Printer, &printer, 2, lp_const_servicename(snum))) ||
	    (!W_ERROR_IS_OK(get_a_printer(Printer, &old_printer, 2, lp_const_servicename(snum))))) {
		result = WERR_BADFID;
		goto done;
	}

	DEBUGADD(8,("Converting info_2 struct\n"));

	/*
	 * convert_printer_info converts the incoming
	 * info from the client and overwrites the info
	 * just read from the tdb in the pointer 'printer'.
	 */

	if (!convert_printer_info(info, printer, level)) {
		result =  WERR_NOMEM;
		goto done;
	}

	if (devmode) {
		/* we have a valid devmode
		   convert it and link it*/

		DEBUGADD(8,("update_printer: Converting the devicemode struct\n"));
		if (!convert_devicemode(printer->info_2->printername, devmode,
				&printer->info_2->devmode)) {
			result =  WERR_NOMEM;
			goto done;
		}
	}

	/* Do sanity check on the requested changes for Samba */

	if (!check_printer_ok(printer->info_2, snum)) {
		result = WERR_INVALID_PARAM;
		goto done;
	}

	/* FIXME!!! If the driver has changed we really should verify that 
	   it is installed before doing much else   --jerry */

	/* Check calling user has permission to update printer description */

	if (Printer->access_granted != PRINTER_ACCESS_ADMINISTER) {
		DEBUG(3, ("update_printer: printer property change denied by handle\n"));
		result = WERR_ACCESS_DENIED;
		goto done;
	}

	/* Call addprinter hook */
	/* Check changes to see if this is really needed */
	
	if ( *lp_addprinter_cmd() 
		&& (!strequal(printer->info_2->drivername, old_printer->info_2->drivername)
			|| !strequal(printer->info_2->comment, old_printer->info_2->comment)
			|| !strequal(printer->info_2->portname, old_printer->info_2->portname)
			|| !strequal(printer->info_2->location, old_printer->info_2->location)) )
	{
		/* add_printer_hook() will call reload_services() */

		if ( !add_printer_hook(p->pipe_user.nt_user_token, printer) ) {
			result = WERR_ACCESS_DENIED;
			goto done;
		}
	}
	
	/*
	 * When a *new* driver is bound to a printer, the drivername is used to
	 * lookup previously saved driver initialization info, which is then
	 * bound to the printer, simulating what happens in the Windows arch.
	 */
	if (!strequal(printer->info_2->drivername, old_printer->info_2->drivername))
	{
		if (!set_driver_init(printer, 2)) 
		{
			DEBUG(5,("update_printer: Error restoring driver initialization data for driver [%s]!\n",
				printer->info_2->drivername));
		}
		
		DEBUG(10,("update_printer: changing driver [%s]!  Sending event!\n",
			printer->info_2->drivername));
			
		notify_printer_driver(snum, printer->info_2->drivername);
	}

	/* 
	 * flag which changes actually occured.  This is a small subset of 
	 * all the possible changes.  We also have to update things in the 
	 * DsSpooler key.
	 */

	if (!strequal(printer->info_2->comment, old_printer->info_2->comment)) {
		init_unistr2( &buffer, printer->info_2->comment, UNI_STR_TERMINATE);
		set_printer_dataex( printer, SPOOL_DSSPOOLER_KEY, "description",
			REG_SZ, (uint8*)buffer.buffer, buffer.uni_str_len*2 );

		notify_printer_comment(snum, printer->info_2->comment);
	}

	if (!strequal(printer->info_2->sharename, old_printer->info_2->sharename)) {
		init_unistr2( &buffer, printer->info_2->sharename, UNI_STR_TERMINATE);
		set_printer_dataex( printer, SPOOL_DSSPOOLER_KEY, "shareName",
			REG_SZ, (uint8*)buffer.buffer, buffer.uni_str_len*2 );

		notify_printer_sharename(snum, printer->info_2->sharename);
	}

	if (!strequal(printer->info_2->printername, old_printer->info_2->printername)) {
		char *pname;
		
		if ( (pname = strchr_m( printer->info_2->printername+2, '\\' )) != NULL )
			pname++;
		else
			pname = printer->info_2->printername;
			

		init_unistr2( &buffer, pname, UNI_STR_TERMINATE);
		set_printer_dataex( printer, SPOOL_DSSPOOLER_KEY, "printerName",
			REG_SZ, (uint8*)buffer.buffer, buffer.uni_str_len*2 );

		notify_printer_printername( snum, pname );
	}
	
	if (!strequal(printer->info_2->portname, old_printer->info_2->portname)) {
		init_unistr2( &buffer, printer->info_2->portname, UNI_STR_TERMINATE);
		set_printer_dataex( printer, SPOOL_DSSPOOLER_KEY, "portName",
			REG_SZ, (uint8*)buffer.buffer, buffer.uni_str_len*2 );

		notify_printer_port(snum, printer->info_2->portname);
	}

	if (!strequal(printer->info_2->location, old_printer->info_2->location)) {
		init_unistr2( &buffer, printer->info_2->location, UNI_STR_TERMINATE);
		set_printer_dataex( printer, SPOOL_DSSPOOLER_KEY, "location",
			REG_SZ, (uint8*)buffer.buffer, buffer.uni_str_len*2 );

		notify_printer_location(snum, printer->info_2->location);
	}
	
	/* here we need to update some more DsSpooler keys */
	/* uNCName, serverName, shortServerName */
	
	init_unistr2( &buffer, global_myname(), UNI_STR_TERMINATE);
	set_printer_dataex( printer, SPOOL_DSSPOOLER_KEY, "serverName",
		REG_SZ, (uint8*)buffer.buffer, buffer.uni_str_len*2 );
	set_printer_dataex( printer, SPOOL_DSSPOOLER_KEY, "shortServerName",
		REG_SZ, (uint8*)buffer.buffer, buffer.uni_str_len*2 );

	slprintf( asc_buffer, sizeof(asc_buffer)-1, "\\\\%s\\%s",
                 global_myname(), printer->info_2->sharename );
	init_unistr2( &buffer, asc_buffer, UNI_STR_TERMINATE);
	set_printer_dataex( printer, SPOOL_DSSPOOLER_KEY, "uNCName",
		REG_SZ, (uint8*)buffer.buffer, buffer.uni_str_len*2 );

	/* Update printer info */
	result = mod_a_printer(printer, 2);

done:
	free_a_printer(&printer, 2);
	free_a_printer(&old_printer, 2);


	return result;
}

/****************************************************************************
****************************************************************************/
static WERROR publish_or_unpublish_printer(pipes_struct *p, POLICY_HND *handle,
				   const SPOOL_PRINTER_INFO_LEVEL *info)
{
#ifdef HAVE_ADS
	SPOOL_PRINTER_INFO_LEVEL_7 *info7 = info->info_7;
	int snum;
	Printer_entry *Printer;

	if ( lp_security() != SEC_ADS ) {
		return WERR_UNKNOWN_LEVEL;
	}

	Printer = find_printer_index_by_hnd(p, handle);

	DEBUG(5,("publish_or_unpublish_printer, action = %d\n",info7->action));

	if (!Printer)
		return WERR_BADFID;

	if (!get_printer_snum(p, handle, &snum))
		return WERR_BADFID;
	
	nt_printer_publish(Printer, snum, info7->action);
	
	return WERR_OK;
#else
	return WERR_UNKNOWN_LEVEL;
#endif
}
/****************************************************************************
****************************************************************************/

WERROR _spoolss_setprinter(pipes_struct *p, SPOOL_Q_SETPRINTER *q_u, SPOOL_R_SETPRINTER *r_u)
{
	POLICY_HND *handle = &q_u->handle;
	uint32 level = q_u->level;
	SPOOL_PRINTER_INFO_LEVEL *info = &q_u->info;
	DEVMODE_CTR devmode_ctr = q_u->devmode_ctr;
	SEC_DESC_BUF *secdesc_ctr = q_u->secdesc_ctr;
	uint32 command = q_u->command;
	WERROR result;

	Printer_entry *Printer = find_printer_index_by_hnd(p, handle);
	
	if (!Printer) {
		DEBUG(2,("_spoolss_setprinter: Invalid handle (%s:%u:%u)\n", OUR_HANDLE(handle)));
		return WERR_BADFID;
	}

	/* check the level */	
	switch (level) {
		case 0:
			return control_printer(handle, command, p);
		case 2:
			result = update_printer(p, handle, level, info, devmode_ctr.devmode);
			if (!W_ERROR_IS_OK(result)) 
				return result;
			if (secdesc_ctr)
				result = update_printer_sec(handle, level, info, p, secdesc_ctr);
			return result;
		case 3:
			return update_printer_sec(handle, level, info, p,
						  secdesc_ctr);
		case 7:
			return publish_or_unpublish_printer(p, handle, info);
		default:
			return WERR_UNKNOWN_LEVEL;
	}
}

/****************************************************************************
****************************************************************************/

WERROR _spoolss_fcpn(pipes_struct *p, SPOOL_Q_FCPN *q_u, SPOOL_R_FCPN *r_u)
{
	POLICY_HND *handle = &q_u->handle;
	Printer_entry *Printer= find_printer_index_by_hnd(p, handle);
	
	if (!Printer) {
		DEBUG(2,("_spoolss_fcpn: Invalid handle (%s:%u:%u)\n", OUR_HANDLE(handle)));
		return WERR_BADFID;
	}

	if (Printer->notify.client_connected==True) {
		int snum = -1;

		if ( Printer->printer_type == SPLHND_SERVER)
			snum = -1;
		else if ( (Printer->printer_type == SPLHND_PRINTER) &&
				!get_printer_snum(p, handle, &snum) )
			return WERR_BADFID;

		srv_spoolss_replycloseprinter(snum, &Printer->notify.client_hnd);
	}

	Printer->notify.flags=0;
	Printer->notify.options=0;
	Printer->notify.localmachine[0]='\0';
	Printer->notify.printerlocal=0;
	if (Printer->notify.option)
		free_spool_notify_option(&Printer->notify.option);
	Printer->notify.client_connected=False;

	return WERR_OK;
}

/****************************************************************************
****************************************************************************/

WERROR _spoolss_addjob(pipes_struct *p, SPOOL_Q_ADDJOB *q_u, SPOOL_R_ADDJOB *r_u)
{
	/* that's an [in out] buffer */

	if (!q_u->buffer && (q_u->offered!=0)) {
		return WERR_INVALID_PARAM;
	}

	rpcbuf_move(q_u->buffer, &r_u->buffer);

	r_u->needed = 0;
	return WERR_INVALID_PARAM; /* this is what a NT server
                                           returns for AddJob. AddJob
                                           must fail on non-local
                                           printers */
}

/****************************************************************************
****************************************************************************/

static void fill_job_info_1(JOB_INFO_1 *job_info, const print_queue_struct *queue,
                            int position, int snum, 
                            const NT_PRINTER_INFO_LEVEL *ntprinter)
{
	struct tm *t;
	
	t=gmtime(&queue->time);

	job_info->jobid=queue->job;	
	init_unistr(&job_info->printername, lp_servicename(snum));
	init_unistr(&job_info->machinename, ntprinter->info_2->servername);
	init_unistr(&job_info->username, queue->fs_user);
	init_unistr(&job_info->document, queue->fs_file);
	init_unistr(&job_info->datatype, "RAW");
	init_unistr(&job_info->text_status, "");
	job_info->status=nt_printj_status(queue->status);
	job_info->priority=queue->priority;
	job_info->position=position;
	job_info->totalpages=queue->page_count;
	job_info->pagesprinted=0;

	make_systemtime(&job_info->submitted, t);
}

/****************************************************************************
****************************************************************************/

static BOOL fill_job_info_2(JOB_INFO_2 *job_info, const print_queue_struct *queue,
                            int position, int snum, 
			    const NT_PRINTER_INFO_LEVEL *ntprinter,
			    DEVICEMODE *devmode)
{
	struct tm *t;

	t=gmtime(&queue->time);

	job_info->jobid=queue->job;
	
	init_unistr(&job_info->printername, ntprinter->info_2->printername);
	
	init_unistr(&job_info->machinename, ntprinter->info_2->servername);
	init_unistr(&job_info->username, queue->fs_user);
	init_unistr(&job_info->document, queue->fs_file);
	init_unistr(&job_info->notifyname, queue->fs_user);
	init_unistr(&job_info->datatype, "RAW");
	init_unistr(&job_info->printprocessor, "winprint");
	init_unistr(&job_info->parameters, "");
	init_unistr(&job_info->drivername, ntprinter->info_2->drivername);
	init_unistr(&job_info->text_status, "");
	
/* and here the security descriptor */

	job_info->status=nt_printj_status(queue->status);
	job_info->priority=queue->priority;
	job_info->position=position;
	job_info->starttime=0;
	job_info->untiltime=0;
	job_info->totalpages=queue->page_count;
	job_info->size=queue->size;
	make_systemtime(&(job_info->submitted), t);
	job_info->timeelapsed=0;
	job_info->pagesprinted=0;

	job_info->devmode = devmode;

	return (True);
}

/****************************************************************************
 Enumjobs at level 1.
****************************************************************************/

static WERROR enumjobs_level1(const print_queue_struct *queue, int snum,
                              const NT_PRINTER_INFO_LEVEL *ntprinter,
			      RPC_BUFFER *buffer, uint32 offered,
			      uint32 *needed, uint32 *returned)
{
	JOB_INFO_1 *info;
	int i;
	WERROR result = WERR_OK;
	
	info=SMB_MALLOC_ARRAY(JOB_INFO_1,*returned);
	if (info==NULL) {
		*returned=0;
		return WERR_NOMEM;
	}
	
	for (i=0; i<*returned; i++)
		fill_job_info_1( &info[i], &queue[i], i, snum, ntprinter );

	/* check the required size. */	
	for (i=0; i<*returned; i++)
		(*needed) += spoolss_size_job_info_1(&info[i]);

	if (*needed > offered) {
		result = WERR_INSUFFICIENT_BUFFER;
		goto out;
	}

	if (!rpcbuf_alloc_size(buffer, *needed)) {
		result = WERR_NOMEM;
		goto out;
	}

	/* fill the buffer with the structures */
	for (i=0; i<*returned; i++)
		smb_io_job_info_1("", buffer, &info[i], 0);	

out:
	/* clear memory */
	SAFE_FREE(info);

	if ( !W_ERROR_IS_OK(result) )
		*returned = 0;

	return result;
}

/****************************************************************************
 Enumjobs at level 2.
****************************************************************************/

static WERROR enumjobs_level2(const print_queue_struct *queue, int snum,
                              const NT_PRINTER_INFO_LEVEL *ntprinter,
			      RPC_BUFFER *buffer, uint32 offered,
			      uint32 *needed, uint32 *returned)
{
	JOB_INFO_2 *info = NULL;
	int i;
	WERROR result = WERR_OK;
	DEVICEMODE *devmode = NULL;
	
	if ( !(info = SMB_MALLOC_ARRAY(JOB_INFO_2,*returned)) ) {
		*returned=0;
		return WERR_NOMEM;
	}
		
	/* this should not be a failure condition if the devmode is NULL */
	
	devmode = construct_dev_mode(snum);

	for (i=0; i<*returned; i++)
		fill_job_info_2(&(info[i]), &queue[i], i, snum, ntprinter, devmode);

	/* check the required size. */	
	for (i=0; i<*returned; i++)
		(*needed) += spoolss_size_job_info_2(&info[i]);

	if (*needed > offered) {
		result = WERR_INSUFFICIENT_BUFFER;
		goto out;
	}

	if (!rpcbuf_alloc_size(buffer, *needed)) {
		result = WERR_NOMEM;
		goto out;
	}

	/* fill the buffer with the structures */
	for (i=0; i<*returned; i++)
		smb_io_job_info_2("", buffer, &info[i], 0);	

out:
	free_devmode(devmode);
	SAFE_FREE(info);

	if ( !W_ERROR_IS_OK(result) )
		*returned = 0;

	return result;

}

/****************************************************************************
 Enumjobs.
****************************************************************************/

WERROR _spoolss_enumjobs( pipes_struct *p, SPOOL_Q_ENUMJOBS *q_u, SPOOL_R_ENUMJOBS *r_u)
{	
	POLICY_HND *handle = &q_u->handle;
	uint32 level = q_u->level;
	RPC_BUFFER *buffer = NULL;
	uint32 offered = q_u->offered;
	uint32 *needed = &r_u->needed;
	uint32 *returned = &r_u->returned;
	WERROR wret;
	NT_PRINTER_INFO_LEVEL *ntprinter = NULL;
	int snum;
	print_status_struct prt_status;
	print_queue_struct *queue=NULL;

	/* that's an [in out] buffer */

	if (!q_u->buffer && (offered!=0)) {
		return WERR_INVALID_PARAM;
	}

	rpcbuf_move(q_u->buffer, &r_u->buffer);
	buffer = r_u->buffer;

	DEBUG(4,("_spoolss_enumjobs\n"));

	*needed=0;
	*returned=0;

	/* lookup the printer snum and tdb entry */
	
	if (!get_printer_snum(p, handle, &snum))
		return WERR_BADFID;

	wret = get_a_printer(NULL, &ntprinter, 2, lp_servicename(snum));
	if ( !W_ERROR_IS_OK(wret) )
		return wret;
	
	*returned = print_queue_status(snum, &queue, &prt_status);
	DEBUGADD(4,("count:[%d], status:[%d], [%s]\n", *returned, prt_status.status, prt_status.message));

	if (*returned == 0) {
		SAFE_FREE(queue);
		free_a_printer(&ntprinter, 2);
		return WERR_OK;
	}

	switch (level) {
	case 1:
		wret = enumjobs_level1(queue, snum, ntprinter, buffer, offered, needed, returned);
		break;
	case 2:
		wret = enumjobs_level2(queue, snum, ntprinter, buffer, offered, needed, returned);
		break;
	default:
		*returned=0;
		wret = WERR_UNKNOWN_LEVEL;
		break;
	}
	
	SAFE_FREE(queue);
	free_a_printer( &ntprinter, 2 );
	return wret;
}

/****************************************************************************
****************************************************************************/

WERROR _spoolss_schedulejob( pipes_struct *p, SPOOL_Q_SCHEDULEJOB *q_u, SPOOL_R_SCHEDULEJOB *r_u)
{
	return WERR_OK;
}

/****************************************************************************
****************************************************************************/

WERROR _spoolss_setjob(pipes_struct *p, SPOOL_Q_SETJOB *q_u, SPOOL_R_SETJOB *r_u)
{
	POLICY_HND *handle = &q_u->handle;
	uint32 jobid = q_u->jobid;
	uint32 command = q_u->command;

	struct current_user user;
	int snum;
	WERROR errcode = WERR_BADFUNC;
		
	if (!get_printer_snum(p, handle, &snum)) {
		return WERR_BADFID;
	}

	if (!print_job_exists(lp_const_servicename(snum), jobid)) {
		return WERR_INVALID_PRINTER_NAME;
	}

	get_current_user(&user, p);	

	switch (command) {
	case JOB_CONTROL_CANCEL:
	case JOB_CONTROL_DELETE:
		if (print_job_delete(&user, snum, jobid, &errcode)) {
			errcode = WERR_OK;
		}
		break;
	case JOB_CONTROL_PAUSE:
		if (print_job_pause(&user, snum, jobid, &errcode)) {
			errcode = WERR_OK;
		}		
		break;
	case JOB_CONTROL_RESTART:
	case JOB_CONTROL_RESUME:
		if (print_job_resume(&user, snum, jobid, &errcode)) {
			errcode = WERR_OK;
		}
		break;
	default:
		return WERR_UNKNOWN_LEVEL;
	}

	return errcode;
}

/****************************************************************************
 Enumerates all printer drivers at level 1.
****************************************************************************/

static WERROR enumprinterdrivers_level1(fstring servername, fstring architecture, RPC_BUFFER *buffer, uint32 offered, uint32 *needed, uint32 *returned)
{
	int i;
	int ndrivers;
	uint32 version;
	fstring *list = NULL;
	NT_PRINTER_DRIVER_INFO_LEVEL driver;
	DRIVER_INFO_1 *driver_info_1=NULL;
	WERROR result = WERR_OK;

	*returned=0;

	for (version=0; version<DRIVER_MAX_VERSION; version++) {
		list=NULL;
		ndrivers=get_ntdrivers(&list, architecture, version);
		DEBUGADD(4,("we have:[%d] drivers in environment [%s] and version [%d]\n", ndrivers, architecture, version));

		if(ndrivers == -1) {
			SAFE_FREE(driver_info_1);
			return WERR_NOMEM;
		}

		if(ndrivers != 0) {
			if((driver_info_1=SMB_REALLOC_ARRAY(driver_info_1, DRIVER_INFO_1, *returned+ndrivers )) == NULL) {
				DEBUG(0,("enumprinterdrivers_level1: failed to enlarge driver info buffer!\n"));
				SAFE_FREE(list);
				return WERR_NOMEM;
			}
		}

		for (i=0; i<ndrivers; i++) {
			WERROR status;
			DEBUGADD(5,("\tdriver: [%s]\n", list[i]));
			ZERO_STRUCT(driver);
			status = get_a_printer_driver(&driver, 3, list[i], 
						      architecture, version);
			if (!W_ERROR_IS_OK(status)) {
				SAFE_FREE(list);
				SAFE_FREE(driver_info_1);
				return status;
			}
			fill_printer_driver_info_1(&driver_info_1[*returned+i], driver, servername, architecture );		
			free_a_printer_driver(driver, 3);
		}	

		*returned+=ndrivers;
		SAFE_FREE(list);
	}
	
	/* check the required size. */
	for (i=0; i<*returned; i++) {
		DEBUGADD(6,("adding driver [%d]'s size\n",i));
		*needed += spoolss_size_printer_driver_info_1(&driver_info_1[i]);
	}

	if (*needed > offered) {
		result = WERR_INSUFFICIENT_BUFFER;
		goto out;
	}

	if (!rpcbuf_alloc_size(buffer, *needed)) {
		result = WERR_NOMEM;	
		goto out;
	}

	/* fill the buffer with the driver structures */
	for (i=0; i<*returned; i++) {
		DEBUGADD(6,("adding driver [%d] to buffer\n",i));
		smb_io_printer_driver_info_1("", buffer, &driver_info_1[i], 0);
	}

out:
	SAFE_FREE(driver_info_1);

	if ( !W_ERROR_IS_OK(result) )
		*returned = 0;

	return result;
}

/****************************************************************************
 Enumerates all printer drivers at level 2.
****************************************************************************/

static WERROR enumprinterdrivers_level2(fstring servername, fstring architecture, RPC_BUFFER *buffer, uint32 offered, uint32 *needed, uint32 *returned)
{
	int i;
	int ndrivers;
	uint32 version;
	fstring *list = NULL;
	NT_PRINTER_DRIVER_INFO_LEVEL driver;
	DRIVER_INFO_2 *driver_info_2=NULL;
	WERROR result = WERR_OK;

	*returned=0;

	for (version=0; version<DRIVER_MAX_VERSION; version++) {
		list=NULL;
		ndrivers=get_ntdrivers(&list, architecture, version);
		DEBUGADD(4,("we have:[%d] drivers in environment [%s] and version [%d]\n", ndrivers, architecture, version));

		if(ndrivers == -1) {
			SAFE_FREE(driver_info_2);
			return WERR_NOMEM;
		}

		if(ndrivers != 0) {
			if((driver_info_2=SMB_REALLOC_ARRAY(driver_info_2, DRIVER_INFO_2, *returned+ndrivers )) == NULL) {
				DEBUG(0,("enumprinterdrivers_level2: failed to enlarge driver info buffer!\n"));
				SAFE_FREE(list);
				return WERR_NOMEM;
			}
		}
		
		for (i=0; i<ndrivers; i++) {
			WERROR status;

			DEBUGADD(5,("\tdriver: [%s]\n", list[i]));
			ZERO_STRUCT(driver);
			status = get_a_printer_driver(&driver, 3, list[i], 
						      architecture, version);
			if (!W_ERROR_IS_OK(status)) {
				SAFE_FREE(list);
				SAFE_FREE(driver_info_2);
				return status;
			}
			fill_printer_driver_info_2(&driver_info_2[*returned+i], driver, servername);		
			free_a_printer_driver(driver, 3);
		}	

		*returned+=ndrivers;
		SAFE_FREE(list);
	}
	
	/* check the required size. */
	for (i=0; i<*returned; i++) {
		DEBUGADD(6,("adding driver [%d]'s size\n",i));
		*needed += spoolss_size_printer_driver_info_2(&(driver_info_2[i]));
	}

	if (*needed > offered) {
		result = WERR_INSUFFICIENT_BUFFER;
		goto out;
	}

	if (!rpcbuf_alloc_size(buffer, *needed)) {
		result = WERR_NOMEM;	
		goto out;
	}

	/* fill the buffer with the form structures */
	for (i=0; i<*returned; i++) {
		DEBUGADD(6,("adding driver [%d] to buffer\n",i));
		smb_io_printer_driver_info_2("", buffer, &(driver_info_2[i]), 0);
	}

out:
	SAFE_FREE(driver_info_2);

	if ( !W_ERROR_IS_OK(result) )
		*returned = 0;

	return result;
}

/****************************************************************************
 Enumerates all printer drivers at level 3.
****************************************************************************/

static WERROR enumprinterdrivers_level3(fstring servername, fstring architecture, RPC_BUFFER *buffer, uint32 offered, uint32 *needed, uint32 *returned)
{
	int i;
	int ndrivers;
	uint32 version;
	fstring *list = NULL;
	DRIVER_INFO_3 *driver_info_3=NULL;
	NT_PRINTER_DRIVER_INFO_LEVEL driver;
	WERROR result = WERR_OK;

	*returned=0;

	for (version=0; version<DRIVER_MAX_VERSION; version++) {
		list=NULL;
		ndrivers=get_ntdrivers(&list, architecture, version);
		DEBUGADD(4,("we have:[%d] drivers in environment [%s] and version [%d]\n", ndrivers, architecture, version));

		if(ndrivers == -1) {
			SAFE_FREE(driver_info_3);
			return WERR_NOMEM;
		}

		if(ndrivers != 0) {
			if((driver_info_3=SMB_REALLOC_ARRAY(driver_info_3, DRIVER_INFO_3, *returned+ndrivers )) == NULL) {
				DEBUG(0,("enumprinterdrivers_level3: failed to enlarge driver info buffer!\n"));
				SAFE_FREE(list);
				return WERR_NOMEM;
			}
		}

		for (i=0; i<ndrivers; i++) {
			WERROR status;

			DEBUGADD(5,("\tdriver: [%s]\n", list[i]));
			ZERO_STRUCT(driver);
			status = get_a_printer_driver(&driver, 3, list[i], 
						      architecture, version);
			if (!W_ERROR_IS_OK(status)) {
				SAFE_FREE(list);
				SAFE_FREE(driver_info_3);
				return status;
			}
			fill_printer_driver_info_3(&driver_info_3[*returned+i], driver, servername);		
			free_a_printer_driver(driver, 3);
		}	

		*returned+=ndrivers;
		SAFE_FREE(list);
	}

	/* check the required size. */
	for (i=0; i<*returned; i++) {
		DEBUGADD(6,("adding driver [%d]'s size\n",i));
		*needed += spoolss_size_printer_driver_info_3(&driver_info_3[i]);
	}

	if (*needed > offered) {
		result = WERR_INSUFFICIENT_BUFFER;
		goto out;
	}

	if (!rpcbuf_alloc_size(buffer, *needed)) {
		result = WERR_NOMEM;	
		goto out;
	}

	/* fill the buffer with the driver structures */
	for (i=0; i<*returned; i++) {
		DEBUGADD(6,("adding driver [%d] to buffer\n",i));
		smb_io_printer_driver_info_3("", buffer, &driver_info_3[i], 0);
	}

out:
	for (i=0; i<*returned; i++) {
		SAFE_FREE(driver_info_3[i].dependentfiles);
	}

	SAFE_FREE(driver_info_3);
	
	if ( !W_ERROR_IS_OK(result) )
		*returned = 0;

	return result;
}

/****************************************************************************
 Enumerates all printer drivers.
****************************************************************************/

WERROR _spoolss_enumprinterdrivers( pipes_struct *p, SPOOL_Q_ENUMPRINTERDRIVERS *q_u, SPOOL_R_ENUMPRINTERDRIVERS *r_u)
{
	uint32 level = q_u->level;
	RPC_BUFFER *buffer = NULL;
	uint32 offered = q_u->offered;
	uint32 *needed = &r_u->needed;
	uint32 *returned = &r_u->returned;

	fstring servername;
	fstring architecture;

	/* that's an [in out] buffer */

	if (!q_u->buffer && (offered!=0)) {
		return WERR_INVALID_PARAM;
	}

	rpcbuf_move(q_u->buffer, &r_u->buffer);
	buffer = r_u->buffer;

	DEBUG(4,("_spoolss_enumprinterdrivers\n"));
	
	*needed   = 0;
	*returned = 0;

	unistr2_to_ascii(architecture, &q_u->environment, sizeof(architecture)-1);
	unistr2_to_ascii(servername, &q_u->name, sizeof(servername)-1);

	if ( !is_myname_or_ipaddr( servername ) )
		return WERR_UNKNOWN_PRINTER_DRIVER;

	switch (level) {
	case 1:
		return enumprinterdrivers_level1(servername, architecture, buffer, offered, needed, returned);
	case 2:
		return enumprinterdrivers_level2(servername, architecture, buffer, offered, needed, returned);
	case 3:
		return enumprinterdrivers_level3(servername, architecture, buffer, offered, needed, returned);
	default:
		return WERR_UNKNOWN_LEVEL;
	}
}

/****************************************************************************
****************************************************************************/

static void fill_form_1(FORM_1 *form, nt_forms_struct *list)
{
	form->flag=list->flag;
	init_unistr(&form->name, list->name);
	form->width=list->width;
	form->length=list->length;
	form->left=list->left;
	form->top=list->top;
	form->right=list->right;
	form->bottom=list->bottom;	
}
	
/****************************************************************************
****************************************************************************/

WERROR _spoolss_enumforms(pipes_struct *p, SPOOL_Q_ENUMFORMS *q_u, SPOOL_R_ENUMFORMS *r_u)
{
	uint32 level = q_u->level;
	RPC_BUFFER *buffer = NULL;
	uint32 offered = q_u->offered;
	uint32 *needed = &r_u->needed;
	uint32 *numofforms = &r_u->numofforms;
	uint32 numbuiltinforms;

	nt_forms_struct *list=NULL;
	nt_forms_struct *builtinlist=NULL;
	FORM_1 *forms_1;
	int buffer_size=0;
	int i;

	/* that's an [in out] buffer */

	if (!q_u->buffer && (offered!=0) ) {
		return WERR_INVALID_PARAM;
	}

	rpcbuf_move(q_u->buffer, &r_u->buffer);
	buffer = r_u->buffer;

	DEBUG(4,("_spoolss_enumforms\n"));
	DEBUGADD(5,("Offered buffer size [%d]\n", offered));
	DEBUGADD(5,("Info level [%d]\n",          level));

	numbuiltinforms = get_builtin_ntforms(&builtinlist);
	DEBUGADD(5,("Number of builtin forms [%d]\n",     numbuiltinforms));
	*numofforms = get_ntforms(&list);
	DEBUGADD(5,("Number of user forms [%d]\n",     *numofforms));
	*numofforms += numbuiltinforms;

	if (*numofforms == 0) {
		SAFE_FREE(builtinlist);
		SAFE_FREE(list);
		return WERR_NO_MORE_ITEMS;
	}

	switch (level) {
	case 1:
		if ((forms_1=SMB_MALLOC_ARRAY(FORM_1, *numofforms)) == NULL) {
			SAFE_FREE(builtinlist);
			SAFE_FREE(list);
			*numofforms=0;
			return WERR_NOMEM;
		}

		/* construct the list of form structures */
		for (i=0; i<numbuiltinforms; i++) {
			DEBUGADD(6,("Filling form number [%d]\n",i));
			fill_form_1(&forms_1[i], &builtinlist[i]);
		}
		
		SAFE_FREE(builtinlist);

		for (; i<*numofforms; i++) {
			DEBUGADD(6,("Filling form number [%d]\n",i));
			fill_form_1(&forms_1[i], &list[i-numbuiltinforms]);
		}
		
		SAFE_FREE(list);

		/* check the required size. */
		for (i=0; i<numbuiltinforms; i++) {
			DEBUGADD(6,("adding form [%d]'s size\n",i));
			buffer_size += spoolss_size_form_1(&forms_1[i]);
		}
		for (; i<*numofforms; i++) {
			DEBUGADD(6,("adding form [%d]'s size\n",i));
			buffer_size += spoolss_size_form_1(&forms_1[i]);
		}

		*needed=buffer_size;		
		
		if (*needed > offered) {
			SAFE_FREE(forms_1);
			*numofforms=0;
			return WERR_INSUFFICIENT_BUFFER;
		}
	
		if (!rpcbuf_alloc_size(buffer, buffer_size)){
			SAFE_FREE(forms_1);
			*numofforms=0;
			return WERR_NOMEM;
		}

		/* fill the buffer with the form structures */
		for (i=0; i<numbuiltinforms; i++) {
			DEBUGADD(6,("adding form [%d] to buffer\n",i));
			smb_io_form_1("", buffer, &forms_1[i], 0);
		}
		for (; i<*numofforms; i++) {
			DEBUGADD(6,("adding form [%d] to buffer\n",i));
			smb_io_form_1("", buffer, &forms_1[i], 0);
		}

		SAFE_FREE(forms_1);

		return WERR_OK;
			
	default:
		SAFE_FREE(list);
		SAFE_FREE(builtinlist);
		return WERR_UNKNOWN_LEVEL;
	}
}

/****************************************************************************
****************************************************************************/

WERROR _spoolss_getform(pipes_struct *p, SPOOL_Q_GETFORM *q_u, SPOOL_R_GETFORM *r_u)
{
	uint32 level = q_u->level;
	UNISTR2 *uni_formname = &q_u->formname;
	RPC_BUFFER *buffer = NULL;
	uint32 offered = q_u->offered;
	uint32 *needed = &r_u->needed;

	nt_forms_struct *list=NULL;
	nt_forms_struct builtin_form;
	BOOL foundBuiltin;
	FORM_1 form_1;
	fstring form_name;
	int buffer_size=0;
	int numofforms=0, i=0;

	/* that's an [in out] buffer */

	if (!q_u->buffer && (offered!=0)) {
		return WERR_INVALID_PARAM;
	}

	rpcbuf_move(q_u->buffer, &r_u->buffer);
	buffer = r_u->buffer;

	unistr2_to_ascii(form_name, uni_formname, sizeof(form_name)-1);

	DEBUG(4,("_spoolss_getform\n"));
	DEBUGADD(5,("Offered buffer size [%d]\n", offered));
	DEBUGADD(5,("Info level [%d]\n",          level));

	foundBuiltin = get_a_builtin_ntform(uni_formname,&builtin_form);
	if (!foundBuiltin) {
		numofforms = get_ntforms(&list);
		DEBUGADD(5,("Number of forms [%d]\n",     numofforms));

		if (numofforms == 0)
			return WERR_BADFID;
	}

	switch (level) {
	case 1:
		if (foundBuiltin) {
			fill_form_1(&form_1, &builtin_form);
		} else {

			/* Check if the requested name is in the list of form structures */
			for (i=0; i<numofforms; i++) {

				DEBUG(4,("_spoolss_getform: checking form %s (want %s)\n", list[i].name, form_name));

				if (strequal(form_name, list[i].name)) {
					DEBUGADD(6,("Found form %s number [%d]\n", form_name, i));
					fill_form_1(&form_1, &list[i]);
					break;
				}
			}
			
			SAFE_FREE(list);
			if (i == numofforms) {
				return WERR_BADFID;
			}
		}
		/* check the required size. */

		*needed=spoolss_size_form_1(&form_1);
		
		if (*needed > offered) 
			return WERR_INSUFFICIENT_BUFFER;

		if (!rpcbuf_alloc_size(buffer, buffer_size))
			return WERR_NOMEM;

		/* fill the buffer with the form structures */
		DEBUGADD(6,("adding form %s [%d] to buffer\n", form_name, i));
		smb_io_form_1("", buffer, &form_1, 0);

		return WERR_OK;
			
	default:
		SAFE_FREE(list);
		return WERR_UNKNOWN_LEVEL;
	}
}

/****************************************************************************
****************************************************************************/

static void fill_port_1(PORT_INFO_1 *port, const char *name)
{
	init_unistr(&port->port_name, name);
}

/****************************************************************************
 TODO: This probably needs distinguish between TCP/IP and Local ports 
 somehow.
****************************************************************************/

static void fill_port_2(PORT_INFO_2 *port, const char *name)
{
	init_unistr(&port->port_name, name);
	init_unistr(&port->monitor_name, "Local Monitor");
	init_unistr(&port->description, SPL_LOCAL_PORT );
	port->port_type=PORT_TYPE_WRITE;
	port->reserved=0x0;	
}


/****************************************************************************
 wrapper around the enumer ports command
****************************************************************************/

WERROR enumports_hook( int *count, char ***lines )
{
	char *cmd = lp_enumports_cmd();
	char **qlines;
	pstring command;
	int numlines;
	int ret;
	int fd;

	*count = 0;
	*lines = NULL;

	/* if no hook then just fill in the default port */
	
	if ( !*cmd ) {
		qlines = SMB_MALLOC_ARRAY( char*, 2 );
		qlines[0] = SMB_STRDUP( SAMBA_PRINTER_PORT_NAME );
		qlines[1] = NULL;
		numlines = 1;
	}
	else {
		/* we have a valid enumport command */
		
		slprintf(command, sizeof(command)-1, "%s \"%d\"", cmd, 1);

		DEBUG(10,("Running [%s]\n", command));
		ret = smbrun(command, &fd);
		DEBUG(10,("Returned [%d]\n", ret));
		if (ret != 0) {
			if (fd != -1) {
				close(fd);
			}
			return WERR_ACCESS_DENIED;
		}

		numlines = 0;
		qlines = fd_lines_load(fd, &numlines, 0);
		DEBUGADD(10,("Lines returned = [%d]\n", numlines));
		close(fd);
	}
	
	*count = numlines;
	*lines = qlines;

	return WERR_OK;
}

/****************************************************************************
 enumports level 1.
****************************************************************************/

static WERROR enumports_level_1(RPC_BUFFER *buffer, uint32 offered, uint32 *needed, uint32 *returned)
{
	PORT_INFO_1 *ports=NULL;
	int i=0;
	WERROR result = WERR_OK;
	char **qlines = NULL;
	int numlines = 0;

	result = enumports_hook( &numlines, &qlines );
	if (!W_ERROR_IS_OK(result)) {
		file_lines_free(qlines);
		return result;
	}
	
	if(numlines) {
		if((ports=SMB_MALLOC_ARRAY( PORT_INFO_1, numlines )) == NULL) {
			DEBUG(10,("Returning WERR_NOMEM [%s]\n", 
				  dos_errstr(WERR_NOMEM)));
			file_lines_free(qlines);
			return WERR_NOMEM;
		}

		for (i=0; i<numlines; i++) {
			DEBUG(6,("Filling port number [%d] with port [%s]\n", i, qlines[i]));
			fill_port_1(&ports[i], qlines[i]);
		}
	}
	file_lines_free(qlines);

	*returned = numlines;

	/* check the required size. */
	for (i=0; i<*returned; i++) {
		DEBUGADD(6,("adding port [%d]'s size\n", i));
		*needed += spoolss_size_port_info_1(&ports[i]);
	}
		
	if (*needed > offered) {
		result = WERR_INSUFFICIENT_BUFFER;
		goto out;
	}

	if (!rpcbuf_alloc_size(buffer, *needed)) {
		result = WERR_NOMEM;
		goto out;
	}

	/* fill the buffer with the ports structures */
	for (i=0; i<*returned; i++) {
		DEBUGADD(6,("adding port [%d] to buffer\n", i));
		smb_io_port_1("", buffer, &ports[i], 0);
	}

out:
	SAFE_FREE(ports);

	if ( !W_ERROR_IS_OK(result) )
		*returned = 0;

	return result;
}

/****************************************************************************
 enumports level 2.
****************************************************************************/

static WERROR enumports_level_2(RPC_BUFFER *buffer, uint32 offered, uint32 *needed, uint32 *returned)
{
	PORT_INFO_2 *ports=NULL;
	int i=0;
	WERROR result = WERR_OK;
	char **qlines = NULL;
	int numlines = 0;

	result = enumports_hook( &numlines, &qlines );
	if ( !W_ERROR_IS_OK(result)) {
		file_lines_free(qlines);
		return result;
	}
	
	if(numlines) {
		if((ports=SMB_MALLOC_ARRAY( PORT_INFO_2, numlines)) == NULL) {
			file_lines_free(qlines);
			return WERR_NOMEM;
		}

		for (i=0; i<numlines; i++) {
			DEBUG(6,("Filling port number [%d] with port [%s]\n", i, qlines[i]));
			fill_port_2(&(ports[i]), qlines[i]);
		}
	}

	file_lines_free(qlines);

	*returned = numlines;

	/* check the required size. */
	for (i=0; i<*returned; i++) {
		DEBUGADD(6,("adding port [%d]'s size\n", i));
		*needed += spoolss_size_port_info_2(&ports[i]);
	}
		
	if (*needed > offered) {
		result = WERR_INSUFFICIENT_BUFFER;
		goto out;
	}

	if (!rpcbuf_alloc_size(buffer, *needed)) {
		result = WERR_NOMEM;
		goto out;
	}

	/* fill the buffer with the ports structures */
	for (i=0; i<*returned; i++) {
		DEBUGADD(6,("adding port [%d] to buffer\n", i));
		smb_io_port_2("", buffer, &ports[i], 0);
	}

out:
	SAFE_FREE(ports);

	if ( !W_ERROR_IS_OK(result) )
		*returned = 0;

	return result;
}

/****************************************************************************
 enumports.
****************************************************************************/

WERROR _spoolss_enumports( pipes_struct *p, SPOOL_Q_ENUMPORTS *q_u, SPOOL_R_ENUMPORTS *r_u)
{
	uint32 level = q_u->level;
	RPC_BUFFER *buffer = NULL;
	uint32 offered = q_u->offered;
	uint32 *needed = &r_u->needed;
	uint32 *returned = &r_u->returned;

	/* that's an [in out] buffer */

	if (!q_u->buffer && (offered!=0)) {
		return WERR_INVALID_PARAM;
	}

	rpcbuf_move(q_u->buffer, &r_u->buffer);
	buffer = r_u->buffer;

	DEBUG(4,("_spoolss_enumports\n"));
	
	*returned=0;
	*needed=0;
	
	switch (level) {
	case 1:
		return enumports_level_1(buffer, offered, needed, returned);
	case 2:
		return enumports_level_2(buffer, offered, needed, returned);
	default:
		return WERR_UNKNOWN_LEVEL;
	}
}

/****************************************************************************
****************************************************************************/

static WERROR spoolss_addprinterex_level_2( pipes_struct *p, const UNISTR2 *uni_srv_name,
				const SPOOL_PRINTER_INFO_LEVEL *info,
				DEVICEMODE *devmode, SEC_DESC_BUF *sec_desc_buf,
				uint32 user_switch, const SPOOL_USER_CTR *user,
				POLICY_HND *handle)
{
	NT_PRINTER_INFO_LEVEL *printer = NULL;
	fstring	name;
	int	snum;
	WERROR err = WERR_OK;

	if ( !(printer = TALLOC_ZERO_P(NULL, NT_PRINTER_INFO_LEVEL)) ) {
		DEBUG(0,("spoolss_addprinterex_level_2: malloc fail.\n"));
		return WERR_NOMEM;
	}

	/* convert from UNICODE to ASCII - this allocates the info_2 struct inside *printer.*/
	if (!convert_printer_info(info, printer, 2)) {
		free_a_printer(&printer, 2);
		return WERR_NOMEM;
	}

	/* check to see if the printer already exists */

	if ((snum = print_queue_snum(printer->info_2->sharename)) != -1) {
		DEBUG(5, ("spoolss_addprinterex_level_2: Attempted to add a printer named [%s] when one already existed!\n", 
			printer->info_2->sharename));
		free_a_printer(&printer, 2);
		return WERR_PRINTER_ALREADY_EXISTS;
	}
	
	/* FIXME!!!  smbd should check to see if the driver is installed before
	   trying to add a printer like this  --jerry */

	if (*lp_addprinter_cmd() ) {
		if ( !add_printer_hook(p->pipe_user.nt_user_token, printer) ) {
			free_a_printer(&printer,2);
			return WERR_ACCESS_DENIED;
		}
	} else {
		DEBUG(0,("spoolss_addprinterex_level_2: add printer for printer %s called and no"
			"smb.conf parameter \"addprinter command\" is defined. This"
			"parameter must exist for this call to succeed\n",
			printer->info_2->sharename ));
	}

	/* use our primary netbios name since get_a_printer() will convert 
	   it to what the client expects on a case by case basis */

	slprintf(name, sizeof(name)-1, "\\\\%s\\%s", global_myname(),
             printer->info_2->sharename);

	
	if ((snum = print_queue_snum(printer->info_2->sharename)) == -1) {
		free_a_printer(&printer,2);
		return WERR_ACCESS_DENIED;
	}

	/* you must be a printer admin to add a new printer */
	if (!print_access_check(NULL, snum, PRINTER_ACCESS_ADMINISTER)) {
		free_a_printer(&printer,2);
		return WERR_ACCESS_DENIED;		
	}
	
	/*
	 * Do sanity check on the requested changes for Samba.
	 */

	if (!check_printer_ok(printer->info_2, snum)) {
		free_a_printer(&printer,2);
		return WERR_INVALID_PARAM;
	}

	/*
	 * When a printer is created, the drivername bound to the printer is used
	 * to lookup previously saved driver initialization info, which is then 
	 * bound to the new printer, simulating what happens in the Windows arch.
	 */

	if (!devmode)
	{
		set_driver_init(printer, 2);
	}
	else 
	{
		/* A valid devmode was included, convert and link it
		*/
		DEBUGADD(10, ("spoolss_addprinterex_level_2: devmode included, converting\n"));

		if (!convert_devicemode(printer->info_2->printername, devmode,
				&printer->info_2->devmode))
			return  WERR_NOMEM;
	}

	/* write the ASCII on disk */
	err = mod_a_printer(printer, 2);
	if (!W_ERROR_IS_OK(err)) {
		free_a_printer(&printer,2);
		return err;
	}

	if (!open_printer_hnd(p, handle, name, PRINTER_ACCESS_ADMINISTER)) {
		/* Handle open failed - remove addition. */
		del_a_printer(printer->info_2->sharename);
		free_a_printer(&printer,2);
		return WERR_ACCESS_DENIED;
	}

	update_c_setprinter(False);
	free_a_printer(&printer,2);

	return WERR_OK;
}

/****************************************************************************
****************************************************************************/

WERROR _spoolss_addprinterex( pipes_struct *p, SPOOL_Q_ADDPRINTEREX *q_u, SPOOL_R_ADDPRINTEREX *r_u)
{
	UNISTR2 *uni_srv_name = q_u->server_name;
	uint32 level = q_u->level;
	SPOOL_PRINTER_INFO_LEVEL *info = &q_u->info;
	DEVICEMODE *devmode = q_u->devmode_ctr.devmode;
	SEC_DESC_BUF *sdb = q_u->secdesc_ctr;
	uint32 user_switch = q_u->user_switch;
	SPOOL_USER_CTR *user = &q_u->user_ctr;
	POLICY_HND *handle = &r_u->handle;

	switch (level) {
		case 1:
			/* we don't handle yet */
			/* but I know what to do ... */
			return WERR_UNKNOWN_LEVEL;
		case 2:
			return spoolss_addprinterex_level_2(p, uni_srv_name, info,
							    devmode, sdb,
							    user_switch, user, handle);
		default:
			return WERR_UNKNOWN_LEVEL;
	}
}

/****************************************************************************
****************************************************************************/

WERROR _spoolss_addprinterdriver(pipes_struct *p, SPOOL_Q_ADDPRINTERDRIVER *q_u, SPOOL_R_ADDPRINTERDRIVER *r_u)
{
	uint32 level = q_u->level;
	SPOOL_PRINTER_DRIVER_INFO_LEVEL *info = &q_u->info;
	WERROR err = WERR_OK;
	NT_PRINTER_DRIVER_INFO_LEVEL driver;
	struct current_user user;
	fstring driver_name;
	uint32 version;

	ZERO_STRUCT(driver);

	get_current_user(&user, p);
	
	if (!convert_printer_driver_info(info, &driver, level)) {
		err = WERR_NOMEM;
		goto done;
	}

	DEBUG(5,("Cleaning driver's information\n"));
	err = clean_up_driver_struct(driver, level, &user);
	if (!W_ERROR_IS_OK(err))
		goto done;

	DEBUG(5,("Moving driver to final destination\n"));
	if( !W_ERROR_IS_OK(err = move_driver_to_download_area(driver, level, &user, &err)) ) {
		goto done;
	}

	if (add_a_printer_driver(driver, level)!=0) {
		err = WERR_ACCESS_DENIED;
		goto done;
	}

	/* 
	 * I think this is where he DrvUpgradePrinter() hook would be
	 * be called in a driver's interface DLL on a Windows NT 4.0/2k
	 * server.  Right now, we just need to send ourselves a message
	 * to update each printer bound to this driver.   --jerry	
	 */
	 
	if (!srv_spoolss_drv_upgrade_printer(driver_name)) {
		DEBUG(0,("_spoolss_addprinterdriver: Failed to send message about upgrading driver [%s]!\n",
			driver_name));
	}

	/*
	 * Based on the version (e.g. driver destination dir: 0=9x,2=Nt/2k,3=2k/Xp),
	 * decide if the driver init data should be deleted. The rules are:
	 *  1) never delete init data if it is a 9x driver, they don't use it anyway
	 *  2) delete init data only if there is no 2k/Xp driver
	 *  3) always delete init data
	 * The generalized rule is always use init data from the highest order driver.
	 * It is necessary to follow the driver install by an initialization step to
	 * finish off this process.
	*/
	if (level == 3)
		version = driver.info_3->cversion;
	else if (level == 6)
		version = driver.info_6->version;
	else
		version = -1;
	switch (version) {
		/*
		 * 9x printer driver - never delete init data
		*/
		case 0: 
			DEBUG(10,("_spoolss_addprinterdriver: init data not deleted for 9x driver [%s]\n",
					driver_name));
			break;
		
		/*
		 * Nt or 2k (compatiblity mode) printer driver - only delete init data if
		 * there is no 2k/Xp driver init data for this driver name.
		*/
		case 2:
		{
			NT_PRINTER_DRIVER_INFO_LEVEL driver1;

			if (!W_ERROR_IS_OK(get_a_printer_driver(&driver1, 3, driver_name, "Windows NT x86", 3))) {
				/*
				 * No 2k/Xp driver found, delete init data (if any) for the new Nt driver.
				*/
				if (!del_driver_init(driver_name))
					DEBUG(6,("_spoolss_addprinterdriver: del_driver_init(%s) Nt failed!\n", driver_name));
			} else {
				/*
				 * a 2k/Xp driver was found, don't delete init data because Nt driver will use it.
				*/
				free_a_printer_driver(driver1,3);
				DEBUG(10,("_spoolss_addprinterdriver: init data not deleted for Nt driver [%s]\n", 
						driver_name));
			}
		}
		break;

		/*
		 * 2k or Xp printer driver - always delete init data
		*/
		case 3:	
			if (!del_driver_init(driver_name))
				DEBUG(6,("_spoolss_addprinterdriver: del_driver_init(%s) 2k/Xp failed!\n", driver_name));
			break;

		default:
			DEBUG(0,("_spoolss_addprinterdriver: invalid level=%d\n", level));
			break;
 	}

	
done:
	free_a_printer_driver(driver, level);
	return err;
}

/********************************************************************
 * spoolss_addprinterdriverex
 ********************************************************************/

WERROR _spoolss_addprinterdriverex(pipes_struct *p, SPOOL_Q_ADDPRINTERDRIVEREX *q_u, SPOOL_R_ADDPRINTERDRIVEREX *r_u)
{
	SPOOL_Q_ADDPRINTERDRIVER q_u_local;
	SPOOL_R_ADDPRINTERDRIVER r_u_local;
	
	/* 
	 * we only support the semantics of AddPrinterDriver()
	 * i.e. only copy files that are newer than existing ones
	 */
	
	if ( q_u->copy_flags != APD_COPY_NEW_FILES )
		return WERR_ACCESS_DENIED;
	
	ZERO_STRUCT(q_u_local);
	ZERO_STRUCT(r_u_local);

	/* just pass the information off to _spoolss_addprinterdriver() */
	q_u_local.server_name_ptr = q_u->server_name_ptr;
	copy_unistr2(&q_u_local.server_name, &q_u->server_name);
	q_u_local.level = q_u->level;
	memcpy( &q_u_local.info, &q_u->info, sizeof(SPOOL_PRINTER_DRIVER_INFO_LEVEL) );
	
	return _spoolss_addprinterdriver( p, &q_u_local, &r_u_local );
}

/****************************************************************************
****************************************************************************/

static void fill_driverdir_1(DRIVER_DIRECTORY_1 *info, char *name)
{
	init_unistr(&info->name, name);
}

/****************************************************************************
****************************************************************************/

static WERROR getprinterdriverdir_level_1(UNISTR2 *name, UNISTR2 *uni_environment, RPC_BUFFER *buffer, uint32 offered, uint32 *needed)
{
	pstring path;
	pstring long_archi;
	fstring servername;
	char *pservername; 
	const char *short_archi;
	DRIVER_DIRECTORY_1 *info=NULL;
	WERROR result = WERR_OK;

	unistr2_to_ascii(servername, name, sizeof(servername)-1);
	unistr2_to_ascii(long_archi, uni_environment, sizeof(long_archi)-1);

	/* check for beginning double '\'s and that the server
	   long enough */

	pservername = servername;
	if ( *pservername == '\\' && strlen(servername)>2 ) {
		pservername += 2;
	} 
	
	if ( !is_myname_or_ipaddr( pservername ) )
		return WERR_INVALID_PARAM;

	if (!(short_archi = get_short_archi(long_archi)))
		return WERR_INVALID_ENVIRONMENT;

	if((info=SMB_MALLOC_P(DRIVER_DIRECTORY_1)) == NULL)
		return WERR_NOMEM;

	slprintf(path, sizeof(path)-1, "\\\\%s\\print$\\%s", pservername, short_archi);

	DEBUG(4,("printer driver directory: [%s]\n", path));

	fill_driverdir_1(info, path);
	
	*needed += spoolss_size_driverdir_info_1(info);

	if (*needed > offered) {
		result = WERR_INSUFFICIENT_BUFFER;
		goto out;
	}

	if (!rpcbuf_alloc_size(buffer, *needed)) {
		result = WERR_NOMEM;
		goto out;
	}

	smb_io_driverdir_1("", buffer, info, 0);

out:
	SAFE_FREE(info);
	
	return result;
}

/****************************************************************************
****************************************************************************/

WERROR _spoolss_getprinterdriverdirectory(pipes_struct *p, SPOOL_Q_GETPRINTERDRIVERDIR *q_u, SPOOL_R_GETPRINTERDRIVERDIR *r_u)
{
	UNISTR2 *name = &q_u->name;
	UNISTR2 *uni_environment = &q_u->environment;
	uint32 level = q_u->level;
	RPC_BUFFER *buffer = NULL;
	uint32 offered = q_u->offered;
	uint32 *needed = &r_u->needed;

	/* that's an [in out] buffer */

	if (!q_u->buffer && (offered!=0)) {
		return WERR_INVALID_PARAM;
	}

	rpcbuf_move(q_u->buffer, &r_u->buffer);
	buffer = r_u->buffer;

	DEBUG(4,("_spoolss_getprinterdriverdirectory\n"));

	*needed=0;

	switch(level) {
	case 1:
		return getprinterdriverdir_level_1(name, uni_environment, buffer, offered, needed);
	default:
		return WERR_UNKNOWN_LEVEL;
	}
}
	
/****************************************************************************
****************************************************************************/

WERROR _spoolss_enumprinterdata(pipes_struct *p, SPOOL_Q_ENUMPRINTERDATA *q_u, SPOOL_R_ENUMPRINTERDATA *r_u)
{
	POLICY_HND *handle = &q_u->handle;
	uint32 idx 		 = q_u->index;
	uint32 in_value_len 	 = q_u->valuesize;
	uint32 in_data_len 	 = q_u->datasize;
	uint32 *out_max_value_len = &r_u->valuesize;
	uint16 **out_value 	 = &r_u->value;
	uint32 *out_value_len 	 = &r_u->realvaluesize;
	uint32 *out_type 	 = &r_u->type;
	uint32 *out_max_data_len = &r_u->datasize;
	uint8  **data_out 	 = &r_u->data;
	uint32 *out_data_len 	 = &r_u->realdatasize;

	NT_PRINTER_INFO_LEVEL *printer = NULL;
	
	uint32 		biggest_valuesize;
	uint32 		biggest_datasize;
	uint32 		data_len;
	Printer_entry 	*Printer = find_printer_index_by_hnd(p, handle);
	int 		snum;
	WERROR 		result;
	REGISTRY_VALUE	*val = NULL;
	NT_PRINTER_DATA *p_data;
	int		i, key_index, num_values;
	int		name_length;
	
	*out_type = 0;

	*out_max_data_len = 0;
	*data_out         = NULL;
	*out_data_len     = 0;

	DEBUG(5,("spoolss_enumprinterdata\n"));

	if (!Printer) {
		DEBUG(2,("_spoolss_enumprinterdata: Invalid handle (%s:%u:%u).\n", OUR_HANDLE(handle)));
		return WERR_BADFID;
	}

	if (!get_printer_snum(p,handle, &snum))
		return WERR_BADFID;
	
	result = get_a_printer(Printer, &printer, 2, lp_const_servicename(snum));
	if (!W_ERROR_IS_OK(result))
		return result;
		
	p_data = printer->info_2->data;	
	key_index = lookup_printerkey( p_data, SPOOL_PRINTERDATA_KEY );

	result = WERR_OK;

	/*
	 * The NT machine wants to know the biggest size of value and data
	 *
	 * cf: MSDN EnumPrinterData remark section
	 */
	 
	if ( !in_value_len && !in_data_len && (key_index != -1) ) 
	{
		DEBUGADD(6,("Activating NT mega-hack to find sizes\n"));

		biggest_valuesize = 0;
		biggest_datasize  = 0;
				
		num_values = regval_ctr_numvals( p_data->keys[key_index].values );
	
		for ( i=0; i<num_values; i++ )
		{
			val = regval_ctr_specific_value( p_data->keys[key_index].values, i );
			
			name_length = strlen(val->valuename);
			if ( strlen(val->valuename) > biggest_valuesize ) 
				biggest_valuesize = name_length;
				
			if ( val->size > biggest_datasize )
				biggest_datasize = val->size;
				
			DEBUG(6,("current values: [%d], [%d]\n", biggest_valuesize, 
				biggest_datasize));
		}

		/* the value is an UNICODE string but real_value_size is the length 
		   in bytes including the trailing 0 */
		   
		*out_value_len = 2 * (1+biggest_valuesize);
		*out_data_len  = biggest_datasize;

		DEBUG(6,("final values: [%d], [%d]\n", *out_value_len, *out_data_len));

		goto done;
	}
	
	/*
	 * the value len is wrong in NT sp3
	 * that's the number of bytes not the number of unicode chars
	 */
	
	if ( key_index != -1 )
		val = regval_ctr_specific_value( p_data->keys[key_index].values, idx );

	if ( !val ) 
	{

		/* out_value should default to "" or else NT4 has
		   problems unmarshalling the response */

		*out_max_value_len=(in_value_len/sizeof(uint16));
		
		if((*out_value=(uint16 *)TALLOC_ZERO(p->mem_ctx, in_value_len*sizeof(uint8))) == NULL)
		{
			result = WERR_NOMEM;
			goto done;
		}

		*out_value_len = (uint32)rpcstr_push((char *)*out_value, "", in_value_len, 0);

		/* the data is counted in bytes */
		
		*out_max_data_len = in_data_len;
		*out_data_len     = in_data_len;
		
		/* only allocate when given a non-zero data_len */
		
		if ( in_data_len && ((*data_out=(uint8 *)TALLOC_ZERO(p->mem_ctx, in_data_len*sizeof(uint8))) == NULL) )
		{
			result = WERR_NOMEM;
			goto done;
		}

		result = WERR_NO_MORE_ITEMS;
	}
	else 
	{
		/*
		 * the value is:
		 * - counted in bytes in the request
		 * - counted in UNICODE chars in the max reply
		 * - counted in bytes in the real size
		 *
		 * take a pause *before* coding not *during* coding
		 */
	
		/* name */
		*out_max_value_len=(in_value_len/sizeof(uint16));
		if ( (*out_value = (uint16 *)TALLOC_ZERO(p->mem_ctx, in_value_len*sizeof(uint8))) == NULL ) 
		{
			result = WERR_NOMEM;
			goto done;
		}
	
		*out_value_len = (uint32)rpcstr_push((char *)*out_value, regval_name(val), (size_t)in_value_len, 0);

		/* type */
		
		*out_type = regval_type( val );

		/* data - counted in bytes */

		*out_max_data_len = in_data_len;
		if ( in_data_len && (*data_out = (uint8 *)TALLOC_ZERO(p->mem_ctx, in_data_len*sizeof(uint8))) == NULL) 
		{
			result = WERR_NOMEM;
			goto done;
		}
		data_len = regval_size(val);
		if ( *data_out )
			memcpy( *data_out, regval_data_p(val), data_len );
		*out_data_len = data_len;
	}

done:
	free_a_printer(&printer, 2);
	return result;
}

/****************************************************************************
****************************************************************************/

WERROR _spoolss_setprinterdata( pipes_struct *p, SPOOL_Q_SETPRINTERDATA *q_u, SPOOL_R_SETPRINTERDATA *r_u)
{
	POLICY_HND 		*handle = &q_u->handle;
	UNISTR2 		*value = &q_u->value;
	uint32 			type = q_u->type;
	uint8 			*data = q_u->data;
	uint32 			real_len = q_u->real_len;

	NT_PRINTER_INFO_LEVEL 	*printer = NULL;
	int 			snum=0;
	WERROR 			status = WERR_OK;
	Printer_entry 		*Printer=find_printer_index_by_hnd(p, handle);
	fstring			valuename;
	
	DEBUG(5,("spoolss_setprinterdata\n"));

	if (!Printer) {
		DEBUG(2,("_spoolss_setprinterdata: Invalid handle (%s:%u:%u).\n", OUR_HANDLE(handle)));
		return WERR_BADFID;
	}

	if ( Printer->printer_type == SPLHND_SERVER ) {
		DEBUG(10,("_spoolss_setprinterdata: Not implemented for server handles yet\n"));
		return WERR_INVALID_PARAM;
	}

	if (!get_printer_snum(p,handle, &snum))
		return WERR_BADFID;

	/* 
	 * Access check : NT returns "access denied" if you make a 
	 * SetPrinterData call without the necessary privildge.
	 * we were originally returning OK if nothing changed
	 * which made Win2k issue **a lot** of SetPrinterData
	 * when connecting to a printer  --jerry
	 */

	if (Printer->access_granted != PRINTER_ACCESS_ADMINISTER) 
	{
		DEBUG(3, ("_spoolss_setprinterdata: change denied by handle access permissions\n"));
		status = WERR_ACCESS_DENIED;
		goto done;
	}

	status = get_a_printer(Printer, &printer, 2, lp_const_servicename(snum));
	if (!W_ERROR_IS_OK(status))
		return status;

	unistr2_to_ascii( valuename, value, sizeof(valuename)-1 );
	
	/*
	 * When client side code sets a magic printer data key, detect it and save
	 * the current printer data and the magic key's data (its the DEVMODE) for
	 * future printer/driver initializations.
	 */
	if ( (type == REG_BINARY) && strequal( valuename, PHANTOM_DEVMODE_KEY)) 
	{
		/* Set devmode and printer initialization info */
		status = save_driver_init( printer, 2, data, real_len );
	
		srv_spoolss_reset_printerdata( printer->info_2->drivername );
	}
	else 
	{
	status = set_printer_dataex( printer, SPOOL_PRINTERDATA_KEY, valuename, 
					type, data, real_len );
		if ( W_ERROR_IS_OK(status) )
			status = mod_a_printer(printer, 2);
	}

done:
	free_a_printer(&printer, 2);

	return status;
}

/****************************************************************************
****************************************************************************/

WERROR _spoolss_resetprinter(pipes_struct *p, SPOOL_Q_RESETPRINTER *q_u, SPOOL_R_RESETPRINTER *r_u)
{
	POLICY_HND 	*handle = &q_u->handle;
	Printer_entry 	*Printer=find_printer_index_by_hnd(p, handle);
	int 		snum;
	
	DEBUG(5,("_spoolss_resetprinter\n"));

	/*
	 * All we do is to check to see if the handle and queue is valid.
	 * This call really doesn't mean anything to us because we only
	 * support RAW printing.   --jerry
	 */
	 
	if (!Printer) {
		DEBUG(2,("_spoolss_resetprinter: Invalid handle (%s:%u:%u).\n", OUR_HANDLE(handle)));
		return WERR_BADFID;
	}

	if (!get_printer_snum(p,handle, &snum))
		return WERR_BADFID;


	/* blindly return success */	
	return WERR_OK;
}


/****************************************************************************
****************************************************************************/

WERROR _spoolss_deleteprinterdata(pipes_struct *p, SPOOL_Q_DELETEPRINTERDATA *q_u, SPOOL_R_DELETEPRINTERDATA *r_u)
{
	POLICY_HND 	*handle = &q_u->handle;
	UNISTR2 	*value = &q_u->valuename;

	NT_PRINTER_INFO_LEVEL 	*printer = NULL;
	int 		snum=0;
	WERROR 		status = WERR_OK;
	Printer_entry 	*Printer=find_printer_index_by_hnd(p, handle);
	pstring		valuename;
	
	DEBUG(5,("spoolss_deleteprinterdata\n"));
	
	if (!Printer) {
		DEBUG(2,("_spoolss_deleteprinterdata: Invalid handle (%s:%u:%u).\n", OUR_HANDLE(handle)));
		return WERR_BADFID;
	}

	if (!get_printer_snum(p, handle, &snum))
		return WERR_BADFID;

	if (Printer->access_granted != PRINTER_ACCESS_ADMINISTER) {
		DEBUG(3, ("_spoolss_deleteprinterdata: printer properties change denied by handle\n"));
		return WERR_ACCESS_DENIED;
	}

	status = get_a_printer(Printer, &printer, 2, lp_const_servicename(snum));
	if (!W_ERROR_IS_OK(status))
		return status;

	unistr2_to_ascii( valuename, value, sizeof(valuename)-1 );

	status = delete_printer_dataex( printer, SPOOL_PRINTERDATA_KEY, valuename );
	
	if ( W_ERROR_IS_OK(status) )
		mod_a_printer( printer, 2 );

	free_a_printer(&printer, 2);

	return status;
}

/****************************************************************************
****************************************************************************/

WERROR _spoolss_addform( pipes_struct *p, SPOOL_Q_ADDFORM *q_u, SPOOL_R_ADDFORM *r_u)
{
	POLICY_HND *handle = &q_u->handle;
	FORM *form = &q_u->form;
	nt_forms_struct tmpForm;
	int snum;
	WERROR status = WERR_OK;
	NT_PRINTER_INFO_LEVEL *printer = NULL;

	int count=0;
	nt_forms_struct *list=NULL;
	Printer_entry *Printer = find_printer_index_by_hnd(p, handle);

	DEBUG(5,("spoolss_addform\n"));

	if (!Printer) {
		DEBUG(2,("_spoolss_addform: Invalid handle (%s:%u:%u).\n", OUR_HANDLE(handle)));
		return WERR_BADFID;
	}
	
	
	/* forms can be added on printer of on the print server handle */
	
	if ( Printer->printer_type == SPLHND_PRINTER )
	{
		if (!get_printer_snum(p,handle, &snum))
	                return WERR_BADFID;
	 
		status = get_a_printer(Printer, &printer, 2, lp_const_servicename(snum));
        	if (!W_ERROR_IS_OK(status))
			goto done;
	}

	if ( !(Printer->access_granted & (PRINTER_ACCESS_ADMINISTER|SERVER_ACCESS_ADMINISTER)) ) {
		DEBUG(2,("_spoolss_addform: denied by handle permissions.\n"));
		status = WERR_ACCESS_DENIED;
		goto done;
	}
	
	/* can't add if builtin */
	
	if (get_a_builtin_ntform(&form->name,&tmpForm)) {
		status = WERR_ALREADY_EXISTS;
		goto done;
	}

	count = get_ntforms(&list);
	
	if(!add_a_form(&list, form, &count)) {
		status =  WERR_NOMEM;
		goto done;
	}
	
	write_ntforms(&list, count);
	
	/*
	 * ChangeID must always be set if this is a printer
	 */
	 
	if ( Printer->printer_type == SPLHND_PRINTER )
		status = mod_a_printer(printer, 2);
	
done:
	if ( printer )
		free_a_printer(&printer, 2);
	SAFE_FREE(list);

	return status;
}

/****************************************************************************
****************************************************************************/

WERROR _spoolss_deleteform( pipes_struct *p, SPOOL_Q_DELETEFORM *q_u, SPOOL_R_DELETEFORM *r_u)
{
	POLICY_HND *handle = &q_u->handle;
	UNISTR2 *form_name = &q_u->name;
	nt_forms_struct tmpForm;
	int count=0;
	nt_forms_struct *list=NULL;
	Printer_entry *Printer = find_printer_index_by_hnd(p, handle);
	int snum;
	WERROR status = WERR_OK;
	NT_PRINTER_INFO_LEVEL *printer = NULL;

	DEBUG(5,("spoolss_deleteform\n"));

	if (!Printer) {
		DEBUG(2,("_spoolss_deleteform: Invalid handle (%s:%u:%u).\n", OUR_HANDLE(handle)));
		return WERR_BADFID;
	}

	/* forms can be deleted on printer of on the print server handle */
	
	if ( Printer->printer_type == SPLHND_PRINTER )
	{
		if (!get_printer_snum(p,handle, &snum))
	                return WERR_BADFID;
	 
		status = get_a_printer(Printer, &printer, 2, lp_const_servicename(snum));
        	if (!W_ERROR_IS_OK(status))
			goto done;
	}

	if ( !(Printer->access_granted & (PRINTER_ACCESS_ADMINISTER|SERVER_ACCESS_ADMINISTER)) ) {
		DEBUG(2,("_spoolss_deleteform: denied by handle permissions.\n"));
		status = WERR_ACCESS_DENIED;
		goto done;
	}

	/* can't delete if builtin */
	
	if (get_a_builtin_ntform(form_name,&tmpForm)) {
		status = WERR_INVALID_PARAM;
		goto done;
	}

	count = get_ntforms(&list);
	
	if ( !delete_a_form(&list, form_name, &count, &status ))
		goto done;

	/*
	 * ChangeID must always be set if this is a printer
	 */
	 
	if ( Printer->printer_type == SPLHND_PRINTER )
		status = mod_a_printer(printer, 2);
	
done:
	if ( printer )
		free_a_printer(&printer, 2);
	SAFE_FREE(list);

	return status;
}

/****************************************************************************
****************************************************************************/

WERROR _spoolss_setform(pipes_struct *p, SPOOL_Q_SETFORM *q_u, SPOOL_R_SETFORM *r_u)
{
	POLICY_HND *handle = &q_u->handle;
	FORM *form = &q_u->form;
	nt_forms_struct tmpForm;
	int snum;
	WERROR status = WERR_OK;
	NT_PRINTER_INFO_LEVEL *printer = NULL;

	int count=0;
	nt_forms_struct *list=NULL;
	Printer_entry *Printer = find_printer_index_by_hnd(p, handle);

 	DEBUG(5,("spoolss_setform\n"));

	if (!Printer) {
		DEBUG(2,("_spoolss_setform: Invalid handle (%s:%u:%u).\n", OUR_HANDLE(handle)));
		return WERR_BADFID;
	}

	/* forms can be modified on printer of on the print server handle */
	
	if ( Printer->printer_type == SPLHND_PRINTER )
	{
		if (!get_printer_snum(p,handle, &snum))
	                return WERR_BADFID;
	 
		status = get_a_printer(Printer, &printer, 2, lp_const_servicename(snum));
        	if (!W_ERROR_IS_OK(status))
			goto done;
	}

	if ( !(Printer->access_granted & (PRINTER_ACCESS_ADMINISTER|SERVER_ACCESS_ADMINISTER)) ) {
		DEBUG(2,("_spoolss_setform: denied by handle permissions\n"));
		status = WERR_ACCESS_DENIED;
		goto done;
	}

	/* can't set if builtin */
	if (get_a_builtin_ntform(&form->name,&tmpForm)) {
		status = WERR_INVALID_PARAM;
		goto done;
	}

	count = get_ntforms(&list);
	update_a_form(&list, form, count);
	write_ntforms(&list, count);

	/*
	 * ChangeID must always be set if this is a printer
	 */
	 
	if ( Printer->printer_type == SPLHND_PRINTER )
		status = mod_a_printer(printer, 2);
	
	
done:
	if ( printer )
		free_a_printer(&printer, 2);
	SAFE_FREE(list);

	return status;
}

/****************************************************************************
 enumprintprocessors level 1.
****************************************************************************/

static WERROR enumprintprocessors_level_1(RPC_BUFFER *buffer, uint32 offered, uint32 *needed, uint32 *returned)
{
	PRINTPROCESSOR_1 *info_1=NULL;
	WERROR result = WERR_OK;
	
	if((info_1 = SMB_MALLOC_P(PRINTPROCESSOR_1)) == NULL)
		return WERR_NOMEM;

	(*returned) = 0x1;
	
	init_unistr(&info_1->name, "winprint");

	*needed += spoolss_size_printprocessor_info_1(info_1);

	if (*needed > offered) {
		result = WERR_INSUFFICIENT_BUFFER;
		goto out;
	}

	if (!rpcbuf_alloc_size(buffer, *needed)) {
		result = WERR_NOMEM;
		goto out;
	}

	smb_io_printprocessor_info_1("", buffer, info_1, 0);

out:
	SAFE_FREE(info_1);

	if ( !W_ERROR_IS_OK(result) )
		*returned = 0;

	return result;
}

/****************************************************************************
****************************************************************************/

WERROR _spoolss_enumprintprocessors(pipes_struct *p, SPOOL_Q_ENUMPRINTPROCESSORS *q_u, SPOOL_R_ENUMPRINTPROCESSORS *r_u)
{
	uint32 level = q_u->level;
	RPC_BUFFER *buffer = NULL;
	uint32 offered = q_u->offered;
	uint32 *needed = &r_u->needed;
	uint32 *returned = &r_u->returned;

	/* that's an [in out] buffer */

	if (!q_u->buffer && (offered!=0)) {
		return WERR_INVALID_PARAM;
	}

	rpcbuf_move(q_u->buffer, &r_u->buffer);
	buffer = r_u->buffer;

 	DEBUG(5,("spoolss_enumprintprocessors\n"));

	/*
	 * Enumerate the print processors ...
	 *
	 * Just reply with "winprint", to keep NT happy
	 * and I can use my nice printer checker.
	 */
	
	*returned=0;
	*needed=0;
	
	switch (level) {
	case 1:
		return enumprintprocessors_level_1(buffer, offered, needed, returned);
	default:
		return WERR_UNKNOWN_LEVEL;
	}
}

/****************************************************************************
 enumprintprocdatatypes level 1.
****************************************************************************/

static WERROR enumprintprocdatatypes_level_1(RPC_BUFFER *buffer, uint32 offered, uint32 *needed, uint32 *returned)
{
	PRINTPROCDATATYPE_1 *info_1=NULL;
	WERROR result = WERR_NOMEM;
	
	if((info_1 = SMB_MALLOC_P(PRINTPROCDATATYPE_1)) == NULL)
		return WERR_NOMEM;

	(*returned) = 0x1;
	
	init_unistr(&info_1->name, "RAW");

	*needed += spoolss_size_printprocdatatype_info_1(info_1);

	if (*needed > offered) {
		result = WERR_INSUFFICIENT_BUFFER;
		goto out;
	}

	if (!rpcbuf_alloc_size(buffer, *needed)) {
		result = WERR_NOMEM;
		goto out;
	}

	smb_io_printprocdatatype_info_1("", buffer, info_1, 0);

out:
	SAFE_FREE(info_1);

	if ( !W_ERROR_IS_OK(result) )
		*returned = 0;

	return result;
}

/****************************************************************************
****************************************************************************/

WERROR _spoolss_enumprintprocdatatypes(pipes_struct *p, SPOOL_Q_ENUMPRINTPROCDATATYPES *q_u, SPOOL_R_ENUMPRINTPROCDATATYPES *r_u)
{
	uint32 level = q_u->level;
	RPC_BUFFER *buffer = NULL;
	uint32 offered = q_u->offered;
	uint32 *needed = &r_u->needed;
	uint32 *returned = &r_u->returned;

	/* that's an [in out] buffer */

	if (!q_u->buffer && (offered!=0)) {
		return WERR_INVALID_PARAM;
	}

	rpcbuf_move(q_u->buffer, &r_u->buffer);
	buffer = r_u->buffer;

 	DEBUG(5,("_spoolss_enumprintprocdatatypes\n"));
	
	*returned=0;
	*needed=0;
	
	switch (level) {
	case 1:
		return enumprintprocdatatypes_level_1(buffer, offered, needed, returned);
	default:
		return WERR_UNKNOWN_LEVEL;
	}
}

/****************************************************************************
 enumprintmonitors level 1.
****************************************************************************/

static WERROR enumprintmonitors_level_1(RPC_BUFFER *buffer, uint32 offered, uint32 *needed, uint32 *returned)
{
	PRINTMONITOR_1 *info_1;
	WERROR result = WERR_OK;
	int i;
	
	if((info_1 = SMB_MALLOC_ARRAY(PRINTMONITOR_1, 2)) == NULL)
		return WERR_NOMEM;

	*returned = 2;
	
	init_unistr(&(info_1[0].name), SPL_LOCAL_PORT ); 
	init_unistr(&(info_1[1].name), SPL_TCPIP_PORT );

	for ( i=0; i<*returned; i++ ) {
		*needed += spoolss_size_printmonitor_info_1(&info_1[i]);
	}
	
	if (*needed > offered) {
		result = WERR_INSUFFICIENT_BUFFER;
		goto out;
	}

	if (!rpcbuf_alloc_size(buffer, *needed)) {
		result = WERR_NOMEM;
		goto out;
	}

	for ( i=0; i<*returned; i++ ) {
		smb_io_printmonitor_info_1("", buffer, &info_1[i], 0);
	}

out:
	SAFE_FREE(info_1);

	if ( !W_ERROR_IS_OK(result) )
		*returned = 0;

	return result;
}

/****************************************************************************
 enumprintmonitors level 2.
****************************************************************************/

static WERROR enumprintmonitors_level_2(RPC_BUFFER *buffer, uint32 offered, uint32 *needed, uint32 *returned)
{
	PRINTMONITOR_2 *info_2;
	WERROR result = WERR_OK;
	int i;
	
	if((info_2 = SMB_MALLOC_ARRAY(PRINTMONITOR_2, 2)) == NULL)
		return WERR_NOMEM;

	*returned = 2;
	
	init_unistr( &(info_2[0].name), SPL_LOCAL_PORT );
	init_unistr( &(info_2[0].environment), "Windows NT X86" );
	init_unistr( &(info_2[0].dll_name), "localmon.dll" );
	
	init_unistr( &(info_2[1].name), SPL_TCPIP_PORT );
	init_unistr( &(info_2[1].environment), "Windows NT X86" );
	init_unistr( &(info_2[1].dll_name), "tcpmon.dll" );

	for ( i=0; i<*returned; i++ ) {
		*needed += spoolss_size_printmonitor_info_2(&info_2[i]);
	}
	
	if (*needed > offered) {
		result = WERR_INSUFFICIENT_BUFFER;
		goto out;
	}

	if (!rpcbuf_alloc_size(buffer, *needed)) {
		result = WERR_NOMEM;
		goto out;
	}

	for ( i=0; i<*returned; i++ ) {
		smb_io_printmonitor_info_2("", buffer, &info_2[i], 0);
	}

out:
	SAFE_FREE(info_2);

	if ( !W_ERROR_IS_OK(result) )
		*returned = 0;
	
	return result;
}

/****************************************************************************
****************************************************************************/

WERROR _spoolss_enumprintmonitors(pipes_struct *p, SPOOL_Q_ENUMPRINTMONITORS *q_u, SPOOL_R_ENUMPRINTMONITORS *r_u)
{
	uint32 level = q_u->level;
	RPC_BUFFER *buffer = NULL;
	uint32 offered = q_u->offered;
	uint32 *needed = &r_u->needed;
	uint32 *returned = &r_u->returned;

	/* that's an [in out] buffer */

	if (!q_u->buffer && (offered!=0)) {
		return WERR_INVALID_PARAM;
	}

	rpcbuf_move(q_u->buffer, &r_u->buffer);
	buffer = r_u->buffer;

 	DEBUG(5,("spoolss_enumprintmonitors\n"));

	/*
	 * Enumerate the print monitors ...
	 *
	 * Just reply with "Local Port", to keep NT happy
	 * and I can use my nice printer checker.
	 */
	
	*returned=0;
	*needed=0;
	
	switch (level) {
	case 1:
		return enumprintmonitors_level_1(buffer, offered, needed, returned);
	case 2:
		return enumprintmonitors_level_2(buffer, offered, needed, returned);
	default:
		return WERR_UNKNOWN_LEVEL;
	}
}

/****************************************************************************
****************************************************************************/

static WERROR getjob_level_1(print_queue_struct **queue, int count, int snum,
                             NT_PRINTER_INFO_LEVEL *ntprinter,
                             uint32 jobid, RPC_BUFFER *buffer, uint32 offered, 
			     uint32 *needed)
{
	int i=0;
	BOOL found=False;
	JOB_INFO_1 *info_1=NULL;
	WERROR result = WERR_OK;

	info_1=SMB_MALLOC_P(JOB_INFO_1);

	if (info_1 == NULL) {
		return WERR_NOMEM;
	}
		
	for (i=0; i<count && found==False; i++) { 
		if ((*queue)[i].job==(int)jobid)
			found=True;
	}
	
	if (found==False) {
		SAFE_FREE(info_1);
		/* NT treats not found as bad param... yet another bad choice */
		return WERR_INVALID_PARAM;
	}
	
	fill_job_info_1( info_1, &((*queue)[i-1]), i, snum, ntprinter );
	
	*needed += spoolss_size_job_info_1(info_1);

	if (*needed > offered) {
		result = WERR_INSUFFICIENT_BUFFER;
		goto out;
	}

	if (!rpcbuf_alloc_size(buffer, *needed)) {
		result = WERR_NOMEM;
		goto out;
	}

	smb_io_job_info_1("", buffer, info_1, 0);

out:
	SAFE_FREE(info_1);

	return result;
}

/****************************************************************************
****************************************************************************/

static WERROR getjob_level_2(print_queue_struct **queue, int count, int snum, 
                             NT_PRINTER_INFO_LEVEL *ntprinter,
                             uint32 jobid, RPC_BUFFER *buffer, uint32 offered, 
			     uint32 *needed)
{
	int 		i = 0;
	BOOL 		found = False;
	JOB_INFO_2 	*info_2;
	WERROR 		result;
	DEVICEMODE 	*devmode = NULL;
	NT_DEVICEMODE	*nt_devmode = NULL;

	if ( !(info_2=SMB_MALLOC_P(JOB_INFO_2)) )
		return WERR_NOMEM;

	ZERO_STRUCTP(info_2);

	for ( i=0; i<count && found==False; i++ ) 
	{
		if ((*queue)[i].job == (int)jobid)
			found = True;
	}
	
	if ( !found ) {
		/* NT treats not found as bad param... yet another bad
		   choice */
		result = WERR_INVALID_PARAM;
		goto done;
	}
	
	/* 
	 * if the print job does not have a DEVMODE associated with it, 
	 * just use the one for the printer. A NULL devicemode is not
	 *  a failure condition
	 */
	 
	if ( !(nt_devmode=print_job_devmode( lp_const_servicename(snum), jobid )) )
		devmode = construct_dev_mode(snum);
	else {
		if ((devmode = SMB_MALLOC_P(DEVICEMODE)) != NULL) {
			ZERO_STRUCTP( devmode );
			convert_nt_devicemode( devmode, nt_devmode );
		}
	}
	
	fill_job_info_2(info_2, &((*queue)[i-1]), i, snum, ntprinter, devmode);
	
	*needed += spoolss_size_job_info_2(info_2);

	if (*needed > offered) {
		result = WERR_INSUFFICIENT_BUFFER;
		goto done;
	}

	if (!rpcbuf_alloc_size(buffer, *needed)) {
		result = WERR_NOMEM;
		goto done;
	}

	smb_io_job_info_2("", buffer, info_2, 0);

	result = WERR_OK;
	
 done:
	/* Cleanup allocated memory */

	free_job_info_2(info_2);	/* Also frees devmode */
	SAFE_FREE(info_2);

	return result;
}

/****************************************************************************
****************************************************************************/

WERROR _spoolss_getjob( pipes_struct *p, SPOOL_Q_GETJOB *q_u, SPOOL_R_GETJOB *r_u)
{
	POLICY_HND *handle = &q_u->handle;
	uint32 jobid = q_u->jobid;
	uint32 level = q_u->level;
	RPC_BUFFER *buffer = NULL;
	uint32 offered = q_u->offered;
	uint32 *needed = &r_u->needed;
	WERROR		wstatus = WERR_OK;
	NT_PRINTER_INFO_LEVEL *ntprinter = NULL;
	int snum;
	int count;
	print_queue_struct 	*queue = NULL;
	print_status_struct prt_status;

	/* that's an [in out] buffer */

	if (!q_u->buffer && (offered!=0)) {
		return WERR_INVALID_PARAM;
	}

	rpcbuf_move(q_u->buffer, &r_u->buffer);
	buffer = r_u->buffer;

	DEBUG(5,("spoolss_getjob\n"));
	
	*needed = 0;
	
	if (!get_printer_snum(p, handle, &snum))
		return WERR_BADFID;
	
	wstatus = get_a_printer(NULL, &ntprinter, 2, lp_servicename(snum));
	if ( !W_ERROR_IS_OK(wstatus) )
		return wstatus;
		
	count = print_queue_status(snum, &queue, &prt_status);
	
	DEBUGADD(4,("count:[%d], prt_status:[%d], [%s]\n",
	             count, prt_status.status, prt_status.message));
		
	switch ( level ) {
	case 1:
			wstatus = getjob_level_1(&queue, count, snum, ntprinter, jobid, 
				buffer, offered, needed);
			break;
	case 2:
			wstatus = getjob_level_2(&queue, count, snum, ntprinter, jobid, 
				buffer, offered, needed);
			break;
	default:
			wstatus = WERR_UNKNOWN_LEVEL;
			break;
	}
	
	SAFE_FREE(queue);
	free_a_printer( &ntprinter, 2 );
	
	return wstatus;
}

/********************************************************************
 spoolss_getprinterdataex
 
 From MSDN documentation of GetPrinterDataEx: pass request
 to GetPrinterData if key is "PrinterDriverData".
 ********************************************************************/

WERROR _spoolss_getprinterdataex(pipes_struct *p, SPOOL_Q_GETPRINTERDATAEX *q_u, SPOOL_R_GETPRINTERDATAEX *r_u)
{
	POLICY_HND	*handle = &q_u->handle;
	uint32 		in_size = q_u->size;
	uint32 		*type = &r_u->type;
	uint32 		*out_size = &r_u->size;
	uint8 		**data = &r_u->data;
	uint32 		*needed = &r_u->needed;
	fstring 	keyname, valuename;
	
	Printer_entry 	*Printer = find_printer_index_by_hnd(p, handle);
	
	NT_PRINTER_INFO_LEVEL 	*printer = NULL;
	int 			snum = 0;
	WERROR 			status = WERR_OK;

	DEBUG(4,("_spoolss_getprinterdataex\n"));

        unistr2_to_ascii(keyname, &q_u->keyname, sizeof(keyname) - 1);
        unistr2_to_ascii(valuename, &q_u->valuename, sizeof(valuename) - 1);
	
	DEBUG(10, ("_spoolss_getprinterdataex: key => [%s], value => [%s]\n", 
		keyname, valuename));

	/* in case of problem, return some default values */
	
	*needed   = 0;
	*type     = 0;
	*out_size = in_size;

	if (!Printer) {
		DEBUG(2,("_spoolss_getprinterdataex: Invalid handle (%s:%u:%u).\n", OUR_HANDLE(handle)));
		status = WERR_BADFID;
		goto done;
	}

	/* Is the handle to a printer or to the server? */

	if (Printer->printer_type == SPLHND_SERVER) {
		DEBUG(10,("_spoolss_getprinterdataex: Not implemented for server handles yet\n"));
		status = WERR_INVALID_PARAM;
		goto done;
	}
	
	if ( !get_printer_snum(p,handle, &snum) )
		return WERR_BADFID;

	status = get_a_printer(Printer, &printer, 2, lp_servicename(snum));
	if ( !W_ERROR_IS_OK(status) )
		goto done;

	/* check to see if the keyname is valid */
	if ( !strlen(keyname) ) {
		status = WERR_INVALID_PARAM;
		goto done;
	}
	
	if ( lookup_printerkey( printer->info_2->data, keyname ) == -1 ) {
		DEBUG(4,("_spoolss_getprinterdataex: Invalid keyname [%s]\n", keyname ));
		free_a_printer( &printer, 2 );
		status = WERR_BADFILE;
		goto done;
	}
	
	/* When given a new keyname, we should just create it */

	status = get_printer_dataex( p->mem_ctx, printer, keyname, valuename, type, data, needed, in_size );
	
	if (*needed > *out_size)
		status = WERR_MORE_DATA;

done:
	if ( !W_ERROR_IS_OK(status) ) 
	{
		DEBUG(5, ("error: allocating %d\n", *out_size));
		
		/* reply this param doesn't exist */
		
		if ( *out_size ) 
		{
			if( (*data=(uint8 *)TALLOC_ZERO(p->mem_ctx, *out_size*sizeof(uint8))) == NULL ) {
				status = WERR_NOMEM;
				goto done;
			}
		} 
		else {
			*data = NULL;
	}
	}
	
	if ( printer )
	free_a_printer( &printer, 2 );
	
	return status;
}

/********************************************************************
 * spoolss_setprinterdataex
 ********************************************************************/

WERROR _spoolss_setprinterdataex(pipes_struct *p, SPOOL_Q_SETPRINTERDATAEX *q_u, SPOOL_R_SETPRINTERDATAEX *r_u)
{
	POLICY_HND 		*handle = &q_u->handle; 
	uint32 			type = q_u->type;
	uint8 			*data = q_u->data;
	uint32 			real_len = q_u->real_len;

	NT_PRINTER_INFO_LEVEL 	*printer = NULL;
	int 			snum = 0;
	WERROR 			status = WERR_OK;
	Printer_entry 		*Printer = find_printer_index_by_hnd(p, handle);
	fstring			valuename;
	fstring			keyname;
	char			*oid_string;
	
	DEBUG(4,("_spoolss_setprinterdataex\n"));

        /* From MSDN documentation of SetPrinterDataEx: pass request to
           SetPrinterData if key is "PrinterDriverData" */

	if (!Printer) {
		DEBUG(2,("_spoolss_setprinterdataex: Invalid handle (%s:%u:%u).\n", OUR_HANDLE(handle)));
		return WERR_BADFID;
	}

	if ( Printer->printer_type == SPLHND_SERVER ) {
		DEBUG(10,("_spoolss_setprinterdataex: Not implemented for server handles yet\n"));
		return WERR_INVALID_PARAM;
	}

	if ( !get_printer_snum(p,handle, &snum) )
		return WERR_BADFID;

	/* 
	 * Access check : NT returns "access denied" if you make a 
	 * SetPrinterData call without the necessary privildge.
	 * we were originally returning OK if nothing changed
	 * which made Win2k issue **a lot** of SetPrinterData
	 * when connecting to a printer  --jerry
	 */

	if (Printer->access_granted != PRINTER_ACCESS_ADMINISTER) 
	{
		DEBUG(3, ("_spoolss_setprinterdataex: change denied by handle access permissions\n"));
		return WERR_ACCESS_DENIED;
	}

	status = get_a_printer(Printer, &printer, 2, lp_servicename(snum));
	if (!W_ERROR_IS_OK(status))
		return status;

        unistr2_to_ascii( valuename, &q_u->value, sizeof(valuename) - 1);
        unistr2_to_ascii( keyname, &q_u->key, sizeof(keyname) - 1);
	
	/* check for OID in valuename */
	
	if ( (oid_string = strchr( valuename, ',' )) != NULL )
	{
		*oid_string = '\0';
		oid_string++;
	}

	/* save the registry data */
	
	status = set_printer_dataex( printer, keyname, valuename, type, data, real_len ); 
	
	if ( W_ERROR_IS_OK(status) )
	{
		/* save the OID if one was specified */
		if ( oid_string ) {
			fstrcat( keyname, "\\" );
			fstrcat( keyname, SPOOL_OID_KEY );
		
			/* 
			 * I'm not checking the status here on purpose.  Don't know 
			 * if this is right, but I'm returning the status from the 
			 * previous set_printer_dataex() call.  I have no idea if 
			 * this is right.    --jerry
			 */
		 
			set_printer_dataex( printer, keyname, valuename, 
			                    REG_SZ, (void*)oid_string, strlen(oid_string)+1 );		
		}
	
		status = mod_a_printer(printer, 2);
	}
		
	free_a_printer(&printer, 2);

	return status;
}


/********************************************************************
 * spoolss_deleteprinterdataex
 ********************************************************************/

WERROR _spoolss_deleteprinterdataex(pipes_struct *p, SPOOL_Q_DELETEPRINTERDATAEX *q_u, SPOOL_R_DELETEPRINTERDATAEX *r_u)
{
	POLICY_HND 	*handle = &q_u->handle;
	UNISTR2 	*value = &q_u->valuename;
	UNISTR2 	*key = &q_u->keyname;

	NT_PRINTER_INFO_LEVEL 	*printer = NULL;
	int 		snum=0;
	WERROR 		status = WERR_OK;
	Printer_entry 	*Printer=find_printer_index_by_hnd(p, handle);
	pstring		valuename, keyname;
	
	DEBUG(5,("spoolss_deleteprinterdataex\n"));
	
	if (!Printer) {
		DEBUG(2,("_spoolss_deleteprinterdata: Invalid handle (%s:%u:%u).\n", OUR_HANDLE(handle)));
		return WERR_BADFID;
	}

	if (!get_printer_snum(p, handle, &snum))
		return WERR_BADFID;

	if (Printer->access_granted != PRINTER_ACCESS_ADMINISTER) {
		DEBUG(3, ("_spoolss_deleteprinterdataex: printer properties change denied by handle\n"));
		return WERR_ACCESS_DENIED;
	}

	status = get_a_printer(Printer, &printer, 2, lp_const_servicename(snum));
	if (!W_ERROR_IS_OK(status))
		return status;

	unistr2_to_ascii( valuename, value, sizeof(valuename)-1 );
	unistr2_to_ascii( keyname, key, sizeof(keyname)-1 );

	status = delete_printer_dataex( printer, keyname, valuename );

	if ( W_ERROR_IS_OK(status) )
		mod_a_printer( printer, 2 );
		
	free_a_printer(&printer, 2);

	return status;
}

/********************************************************************
 * spoolss_enumprinterkey
 ********************************************************************/


WERROR _spoolss_enumprinterkey(pipes_struct *p, SPOOL_Q_ENUMPRINTERKEY *q_u, SPOOL_R_ENUMPRINTERKEY *r_u)
{
	fstring 	key;
	fstring		*keynames = NULL;
	uint16  	*enumkeys = NULL;
	int		num_keys;
	int		printerkey_len;
	POLICY_HND	*handle = &q_u->handle;
	Printer_entry 	*Printer = find_printer_index_by_hnd(p, handle);
	NT_PRINTER_DATA	*data;
	NT_PRINTER_INFO_LEVEL 	*printer = NULL;
	int 		snum = 0;
	WERROR		status = WERR_BADFILE;
	
	
	DEBUG(4,("_spoolss_enumprinterkey\n"));

	if (!Printer) {
		DEBUG(2,("_spoolss_enumprinterkey: Invalid handle (%s:%u:%u).\n", OUR_HANDLE(handle)));
		return WERR_BADFID;
	}

	if ( !get_printer_snum(p,handle, &snum) )
		return WERR_BADFID;

	status = get_a_printer(Printer, &printer, 2, lp_const_servicename(snum));
	if (!W_ERROR_IS_OK(status))
		return status;
		
	/* get the list of subkey names */
	
	unistr2_to_ascii( key, &q_u->key, sizeof(key)-1 );
	data = printer->info_2->data;

	num_keys = get_printer_subkeys( data, key, &keynames );

	if ( num_keys == -1 ) {
		status = WERR_BADFILE;
		goto done;
	}

	printerkey_len = init_unistr_array( &enumkeys,  keynames, NULL );

	r_u->needed = printerkey_len*2;

	if ( q_u->size < r_u->needed ) {
		status = WERR_MORE_DATA;
		goto done;
	}

	if (!make_spoolss_buffer5(p->mem_ctx, &r_u->keys, printerkey_len, enumkeys)) {
		status = WERR_NOMEM;
		goto done;
	}
			
	status = WERR_OK;

	if ( q_u->size < r_u->needed ) 
		status = WERR_MORE_DATA;

done:
	free_a_printer( &printer, 2 );
	SAFE_FREE( keynames );
	
        return status;
}

/********************************************************************
 * spoolss_deleteprinterkey
 ********************************************************************/

WERROR _spoolss_deleteprinterkey(pipes_struct *p, SPOOL_Q_DELETEPRINTERKEY *q_u, SPOOL_R_DELETEPRINTERKEY *r_u)
{
	POLICY_HND		*handle = &q_u->handle;
	Printer_entry 		*Printer = find_printer_index_by_hnd(p, &q_u->handle);
	fstring 		key;
	NT_PRINTER_INFO_LEVEL 	*printer = NULL;
	int 			snum=0;
	WERROR			status;
	
	DEBUG(5,("spoolss_deleteprinterkey\n"));
	
	if (!Printer) {
		DEBUG(2,("_spoolss_deleteprinterkey: Invalid handle (%s:%u:%u).\n", OUR_HANDLE(handle)));
		return WERR_BADFID;
	}

	/* if keyname == NULL, return error */
	
	if ( !q_u->keyname.buffer )
		return WERR_INVALID_PARAM;
		
	if (!get_printer_snum(p, handle, &snum))
		return WERR_BADFID;

	if (Printer->access_granted != PRINTER_ACCESS_ADMINISTER) {
		DEBUG(3, ("_spoolss_deleteprinterkey: printer properties change denied by handle\n"));
		return WERR_ACCESS_DENIED;
	}

	status = get_a_printer(Printer, &printer, 2, lp_const_servicename(snum));
	if (!W_ERROR_IS_OK(status))
		return status;
	
	/* delete the key and all subneys */
	
        unistr2_to_ascii(key, &q_u->keyname, sizeof(key) - 1);
 
	status = delete_all_printer_data( printer->info_2, key );	

	if ( W_ERROR_IS_OK(status) )
		status = mod_a_printer(printer, 2);
	
	free_a_printer( &printer, 2 );
	
	return status;
}


/********************************************************************
 * spoolss_enumprinterdataex
 ********************************************************************/

WERROR _spoolss_enumprinterdataex(pipes_struct *p, SPOOL_Q_ENUMPRINTERDATAEX *q_u, SPOOL_R_ENUMPRINTERDATAEX *r_u)
{
	POLICY_HND	*handle = &q_u->handle; 
	uint32 		in_size = q_u->size;
	uint32 		num_entries, 
			needed;
	NT_PRINTER_INFO_LEVEL 	*printer = NULL;
	PRINTER_ENUM_VALUES	*enum_values = NULL;
	NT_PRINTER_DATA		*p_data;
	fstring 	key;
	Printer_entry 	*Printer = find_printer_index_by_hnd(p, handle);
	int 		snum;
	WERROR 		result;
	int		key_index;
	int		i;
	REGISTRY_VALUE	*val;
	char		*value_name;
	uint32		data_len;
	

	DEBUG(4,("_spoolss_enumprinterdataex\n"));

	if (!Printer) {
		DEBUG(2,("_spoolss_enumprinterdataex: Invalid handle (%s:%u:%u1<).\n", OUR_HANDLE(handle)));
		return WERR_BADFID;
	}

	/* 
	 * first check for a keyname of NULL or "".  Win2k seems to send 
	 * this a lot and we should send back WERR_INVALID_PARAM
	 * no need to spend time looking up the printer in this case.
	 * --jerry
	 */
	 
	unistr2_to_ascii(key, &q_u->key, sizeof(key) - 1);
	if ( !strlen(key) ) {
		result = WERR_INVALID_PARAM;
		goto done;
	}

	/* get the printer off of disk */
	
	if (!get_printer_snum(p,handle, &snum))
		return WERR_BADFID;
	
	ZERO_STRUCT(printer);
	result = get_a_printer(Printer, &printer, 2, lp_const_servicename(snum));
	if (!W_ERROR_IS_OK(result))
		return result;
	
	/* now look for a match on the key name */
	
	p_data = printer->info_2->data;
	
	unistr2_to_ascii(key, &q_u->key, sizeof(key) - 1);
	if ( (key_index = lookup_printerkey( p_data, key)) == -1  )
	{
		DEBUG(10,("_spoolss_enumprinterdataex: Unknown keyname [%s]\n", key));
		result = WERR_INVALID_PARAM;
		goto done;
	}
	
	result = WERR_OK;
	needed = 0;
	
	/* allocate the memory for the array of pointers -- if necessary */
	
	num_entries = regval_ctr_numvals( p_data->keys[key_index].values );
	if ( num_entries )
	{
		if ( (enum_values=TALLOC_ARRAY(p->mem_ctx, PRINTER_ENUM_VALUES, num_entries)) == NULL )
		{
			DEBUG(0,("_spoolss_enumprinterdataex: talloc() failed to allocate memory for [%lu] bytes!\n",
				(unsigned long)num_entries*sizeof(PRINTER_ENUM_VALUES)));
			result = WERR_NOMEM;
			goto done;
		}

		memset( enum_values, 0x0, num_entries*sizeof(PRINTER_ENUM_VALUES) );
	}
		
	/* 
	 * loop through all params and build the array to pass 
	 * back to the  client 
	 */
	 
	for ( i=0; i<num_entries; i++ )
	{
		/* lookup the registry value */
		
		val = regval_ctr_specific_value( p_data->keys[key_index].values, i );
		DEBUG(10,("retrieved value number [%d] [%s]\n", i, regval_name(val) ));

		/* copy the data */
		
		value_name = regval_name( val );
		init_unistr( &enum_values[i].valuename, value_name );
		enum_values[i].value_len = (strlen(value_name)+1) * 2;
		enum_values[i].type      = regval_type( val );
		
		data_len = regval_size( val );
		if ( data_len ) {
			if ( !(enum_values[i].data = TALLOC_MEMDUP(p->mem_ctx, regval_data_p(val), data_len)) ) 
			{
				DEBUG(0,("talloc_memdup failed to allocate memory [data_len=%d] for data!\n", 
					data_len ));
				result = WERR_NOMEM;
				goto done;
			}
		}
		enum_values[i].data_len = data_len;

		/* keep track of the size of the array in bytes */
		
		needed += spoolss_size_printer_enum_values(&enum_values[i]);
	}
	
	/* housekeeping information in the reply */
	
	r_u->needed 	= needed;
	r_u->returned 	= num_entries;

	if (needed > in_size) {
		result = WERR_MORE_DATA;
		goto done;
	}
		
	/* copy data into the reply */
	
	r_u->ctr.size        	= r_u->needed;
	r_u->ctr.size_of_array 	= r_u->returned;
	r_u->ctr.values 	= enum_values;
	
	
		
done:	
	if ( printer )
	free_a_printer(&printer, 2);

	return result;
}

/****************************************************************************
****************************************************************************/

static void fill_printprocessordirectory_1(PRINTPROCESSOR_DIRECTORY_1 *info, char *name)
{
	init_unistr(&info->name, name);
}

static WERROR getprintprocessordirectory_level_1(UNISTR2 *name, 
						 UNISTR2 *environment, 
						 RPC_BUFFER *buffer, 
						 uint32 offered, 
						 uint32 *needed)
{
	pstring path;
	pstring long_archi;
	PRINTPROCESSOR_DIRECTORY_1 *info=NULL;
	WERROR result = WERR_OK;

	unistr2_to_ascii(long_archi, environment, sizeof(long_archi)-1);

	if (!get_short_archi(long_archi))
		return WERR_INVALID_ENVIRONMENT;

	if((info=SMB_MALLOC_P(PRINTPROCESSOR_DIRECTORY_1)) == NULL)
		return WERR_NOMEM;

	pstrcpy(path, "C:\\WINNT\\System32\\spool\\PRTPROCS\\W32X86");

	fill_printprocessordirectory_1(info, path);
	
	*needed += spoolss_size_printprocessordirectory_info_1(info);

	if (*needed > offered) {
		result = WERR_INSUFFICIENT_BUFFER;
		goto out;
	}

	if (!rpcbuf_alloc_size(buffer, *needed)) {
		result = WERR_INSUFFICIENT_BUFFER;
		goto out;
	}

	smb_io_printprocessordirectory_1("", buffer, info, 0);

out:
	SAFE_FREE(info);
	
	return result;
}

WERROR _spoolss_getprintprocessordirectory(pipes_struct *p, SPOOL_Q_GETPRINTPROCESSORDIRECTORY *q_u, SPOOL_R_GETPRINTPROCESSORDIRECTORY *r_u)
{
	uint32 level = q_u->level;
	RPC_BUFFER *buffer = NULL;
	uint32 offered = q_u->offered;
	uint32 *needed = &r_u->needed;
	WERROR result;

	/* that's an [in out] buffer */

	if (!q_u->buffer && (offered!=0)) {
		return WERR_INVALID_PARAM;
	}

	rpcbuf_move(q_u->buffer, &r_u->buffer);
	buffer = r_u->buffer;

 	DEBUG(5,("_spoolss_getprintprocessordirectory\n"));
	
	*needed=0;

	switch(level) {
	case 1:
		result = getprintprocessordirectory_level_1
		  (&q_u->name, &q_u->environment, buffer, offered, needed);
		break;
	default:
		result = WERR_UNKNOWN_LEVEL;
	}

	return result;
}

/*******************************************************************
 Streams the monitor UI DLL name in UNICODE
*******************************************************************/

static WERROR xcvtcp_monitorui( NT_USER_TOKEN *token, RPC_BUFFER *in, 
                                RPC_BUFFER *out, uint32 *needed )
{
	const char *dllname = "tcpmonui.dll";
	
	*needed = (strlen(dllname)+1) * 2;
	
	if ( rpcbuf_get_size(out) < *needed ) {
		return WERR_INSUFFICIENT_BUFFER;		
	}
	
	if ( !make_monitorui_buf( out, dllname ) ) {
		return WERR_NOMEM;
	}
	
	return WERR_OK;
}

/*******************************************************************
 Create a new TCP/IP port
*******************************************************************/

static WERROR xcvtcp_addport( NT_USER_TOKEN *token, RPC_BUFFER *in, 
                              RPC_BUFFER *out, uint32 *needed )
{
	NT_PORT_DATA_1 port1;
	pstring device_uri;

	ZERO_STRUCT( port1 );

	/* convert to our internal port data structure */

	if ( !convert_port_data_1( &port1, in ) ) {
		return WERR_NOMEM;
	}

	/* create the device URI and call the add_port_hook() */

	switch ( port1.protocol ) {
	case PORT_PROTOCOL_DIRECT:
		pstr_sprintf( device_uri, "socket://%s:%d/", port1.hostaddr, port1.port );
		break;

	case PORT_PROTOCOL_LPR:
		pstr_sprintf( device_uri, "lpr://%s/%s", port1.hostaddr, port1.queue );
		break;
	
	default:
		return WERR_UNKNOWN_PORT;
	}

	return add_port_hook( token, port1.name, device_uri );
}

/*******************************************************************
*******************************************************************/

struct xcv_api_table xcvtcp_cmds[] = {
	{ "MonitorUI",	xcvtcp_monitorui },
	{ "AddPort",	xcvtcp_addport},
	{ NULL,		NULL }
};

static WERROR process_xcvtcp_command( NT_USER_TOKEN *token, const char *command, 
                                      RPC_BUFFER *inbuf, RPC_BUFFER *outbuf, 
                                      uint32 *needed )
{
	int i;
	
	DEBUG(10,("process_xcvtcp_command: Received command \"%s\"\n", command));
	
	for ( i=0; xcvtcp_cmds[i].name; i++ ) {
		if ( strcmp( command, xcvtcp_cmds[i].name ) == 0 )
			return xcvtcp_cmds[i].fn( token, inbuf, outbuf, needed );
	}
	
	return WERR_BADFUNC;
}

/*******************************************************************
*******************************************************************/
#if 0 	/* don't support management using the "Local Port" monitor */

static WERROR xcvlocal_monitorui( NT_USER_TOKEN *token, RPC_BUFFER *in, 
                                  RPC_BUFFER *out, uint32 *needed )
{
	const char *dllname = "localui.dll";
	
	*needed = (strlen(dllname)+1) * 2;
	
	if ( rpcbuf_get_size(out) < *needed ) {
		return WERR_INSUFFICIENT_BUFFER;		
	}
	
	if ( !make_monitorui_buf( out, dllname )) {
		return WERR_NOMEM;
	}
	
	return WERR_OK;
}

/*******************************************************************
*******************************************************************/

struct xcv_api_table xcvlocal_cmds[] = {
	{ "MonitorUI",	xcvlocal_monitorui },
	{ NULL,		NULL }
};
#else
struct xcv_api_table xcvlocal_cmds[] = {
	{ NULL,		NULL }
};
#endif



/*******************************************************************
*******************************************************************/

static WERROR process_xcvlocal_command( NT_USER_TOKEN *token, const char *command, 
                                        RPC_BUFFER *inbuf, RPC_BUFFER *outbuf, 
					uint32 *needed )
{
	int i;
	
	DEBUG(10,("process_xcvlocal_command: Received command \"%s\"\n", command));

	for ( i=0; xcvlocal_cmds[i].name; i++ ) {
		if ( strcmp( command, xcvlocal_cmds[i].name ) == 0 )
			return xcvlocal_cmds[i].fn( token, inbuf, outbuf , needed );
	}
	return WERR_BADFUNC;
}

/*******************************************************************
*******************************************************************/

WERROR _spoolss_xcvdataport(pipes_struct *p, SPOOL_Q_XCVDATAPORT *q_u, SPOOL_R_XCVDATAPORT *r_u)
{	
	Printer_entry *Printer = find_printer_index_by_hnd(p, &q_u->handle);
	fstring command;

	if (!Printer) {
		DEBUG(2,("_spoolss_xcvdataport: Invalid handle (%s:%u:%u).\n", OUR_HANDLE(&q_u->handle)));
		return WERR_BADFID;
	}

	/* Has to be a handle to the TCP/IP port monitor */
	
	if ( !(Printer->printer_type & (SPLHND_PORTMON_LOCAL|SPLHND_PORTMON_TCP)) ) {
		DEBUG(2,("_spoolss_xcvdataport: Call only valid for Port Monitors\n"));
		return WERR_BADFID;
	}
	
	/* requires administrative access to the server */
	
	if ( !(Printer->access_granted & SERVER_ACCESS_ADMINISTER) ) {
		DEBUG(2,("_spoolss_xcvdataport: denied by handle permissions.\n"));
		return WERR_ACCESS_DENIED;
	}

	/* Get the command name.  There's numerous commands supported by the 
	   TCPMON interface. */
	
	rpcstr_pull(command, q_u->dataname.buffer, sizeof(command), 
		q_u->dataname.uni_str_len*2, 0);
		
	/* Allocate the outgoing buffer */
	
	rpcbuf_init( &r_u->outdata, q_u->offered, p->mem_ctx );
	
	switch ( Printer->printer_type ) {
	case SPLHND_PORTMON_TCP:
		return process_xcvtcp_command( p->pipe_user.nt_user_token, command, 
			&q_u->indata, &r_u->outdata, &r_u->needed );
	case SPLHND_PORTMON_LOCAL:
		return process_xcvlocal_command( p->pipe_user.nt_user_token, command, 
			&q_u->indata, &r_u->outdata, &r_u->needed );
	}

	return WERR_INVALID_PRINT_MONITOR;
}


