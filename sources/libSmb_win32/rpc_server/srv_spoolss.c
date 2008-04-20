/* 
 *  Unix SMB/CIFS implementation.
 *  RPC Pipe client / server routines
 *  Copyright (C) Andrew Tridgell              1992-2000,
 *  Copyright (C) Luke Kenneth Casson Leighton 1996-2000,
 *  Copyright (C) Jean François Micouleau      1998-2000,
 *  Copyright (C) Jeremy Allison                    2001,
 *  Copyright (C) Gerald Carter                2001-2002,
 *  Copyright (C) Jim McDonough <jmcd@us.ibm.com>   2003.
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

#include "includes.h"

#undef DBGC_CLASS
#define DBGC_CLASS DBGC_RPC_SRV

/********************************************************************
 * api_spoolss_open_printer_ex (rarely seen - older call)
 ********************************************************************/

static BOOL api_spoolss_open_printer(pipes_struct *p)
{
	SPOOL_Q_OPEN_PRINTER q_u;
	SPOOL_R_OPEN_PRINTER r_u;
	prs_struct *data = &p->in_data.data;
	prs_struct *rdata = &p->out_data.rdata;

	ZERO_STRUCT(q_u);
	ZERO_STRUCT(r_u);

	if (!spoolss_io_q_open_printer("", &q_u, data, 0)) {
		DEBUG(0,("spoolss_io_q_open_printer: unable to unmarshall SPOOL_Q_OPEN_PRINTER.\n"));
		return False;
	}

	r_u.status = _spoolss_open_printer( p, &q_u, &r_u);
	
	if (!spoolss_io_r_open_printer("",&r_u,rdata,0)){
		DEBUG(0,("spoolss_io_r_open_printer: unable to marshall SPOOL_R_OPEN_PRINTER.\n"));
		return False;
	}

	return True;
}


/********************************************************************
 * api_spoolss_open_printer_ex
 ********************************************************************/

static BOOL api_spoolss_open_printer_ex(pipes_struct *p)
{
	SPOOL_Q_OPEN_PRINTER_EX q_u;
	SPOOL_R_OPEN_PRINTER_EX r_u;
	prs_struct *data = &p->in_data.data;
	prs_struct *rdata = &p->out_data.rdata;

	ZERO_STRUCT(q_u);
	ZERO_STRUCT(r_u);

	if (!spoolss_io_q_open_printer_ex("", &q_u, data, 0)) {
		DEBUG(0,("spoolss_io_q_open_printer_ex: unable to unmarshall SPOOL_Q_OPEN_PRINTER_EX.\n"));
		return False;
	}

	r_u.status = _spoolss_open_printer_ex( p, &q_u, &r_u);

	if (!spoolss_io_r_open_printer_ex("",&r_u,rdata,0)){
		DEBUG(0,("spoolss_io_r_open_printer_ex: unable to marshall SPOOL_R_OPEN_PRINTER_EX.\n"));
		return False;
	}

	return True;
}

/********************************************************************
 * api_spoolss_getprinterdata
 *
 * called from the spoolss dispatcher
 ********************************************************************/

static BOOL api_spoolss_getprinterdata(pipes_struct *p)
{
	SPOOL_Q_GETPRINTERDATA q_u;
	SPOOL_R_GETPRINTERDATA r_u;
	prs_struct *data = &p->in_data.data;
	prs_struct *rdata = &p->out_data.rdata;

	ZERO_STRUCT(q_u);
	ZERO_STRUCT(r_u);

	/* read the stream and fill the struct */
	if (!spoolss_io_q_getprinterdata("", &q_u, data, 0)) {
		DEBUG(0,("spoolss_io_q_getprinterdata: unable to unmarshall SPOOL_Q_GETPRINTERDATA.\n"));
		return False;
	}
	
	r_u.status = _spoolss_getprinterdata( p, &q_u, &r_u);

	if (!spoolss_io_r_getprinterdata("", &r_u, rdata, 0)) {
		DEBUG(0,("spoolss_io_r_getprinterdata: unable to marshall SPOOL_R_GETPRINTERDATA.\n"));
		return False;
	}

	return True;
}

/********************************************************************
 * api_spoolss_deleteprinterdata
 *
 * called from the spoolss dispatcher
 ********************************************************************/

static BOOL api_spoolss_deleteprinterdata(pipes_struct *p)
{
	SPOOL_Q_DELETEPRINTERDATA q_u;
	SPOOL_R_DELETEPRINTERDATA r_u;
	prs_struct *data = &p->in_data.data;
	prs_struct *rdata = &p->out_data.rdata;

	ZERO_STRUCT(q_u);
	ZERO_STRUCT(r_u);

	/* read the stream and fill the struct */
	if (!spoolss_io_q_deleteprinterdata("", &q_u, data, 0)) {
		DEBUG(0,("spoolss_io_q_deleteprinterdata: unable to unmarshall SPOOL_Q_DELETEPRINTERDATA.\n"));
		return False;
	}
	
	r_u.status = _spoolss_deleteprinterdata( p, &q_u, &r_u );

	if (!spoolss_io_r_deleteprinterdata("", &r_u, rdata, 0)) {
		DEBUG(0,("spoolss_io_r_deleteprinterdata: unable to marshall SPOOL_R_DELETEPRINTERDATA.\n"));
		return False;
	}

	return True;
}

/********************************************************************
 * api_spoolss_closeprinter
 *
 * called from the spoolss dispatcher
 ********************************************************************/

static BOOL api_spoolss_closeprinter(pipes_struct *p)
{
	SPOOL_Q_CLOSEPRINTER q_u;
	SPOOL_R_CLOSEPRINTER r_u;
	prs_struct *data = &p->in_data.data;
	prs_struct *rdata = &p->out_data.rdata;

	ZERO_STRUCT(q_u);
	ZERO_STRUCT(r_u);

	if (!spoolss_io_q_closeprinter("", &q_u, data, 0)) {
		DEBUG(0,("spoolss_io_q_closeprinter: unable to unmarshall SPOOL_Q_CLOSEPRINTER.\n"));
		return False;
	}

	r_u.status = _spoolss_closeprinter(p, &q_u, &r_u);

	if (!spoolss_io_r_closeprinter("",&r_u,rdata,0)) {
		DEBUG(0,("spoolss_io_r_closeprinter: unable to marshall SPOOL_R_CLOSEPRINTER.\n"));
		return False;
	}

	return True;
}

/********************************************************************
 * api_spoolss_abortprinter
 *
 * called from the spoolss dispatcher
 ********************************************************************/

static BOOL api_spoolss_abortprinter(pipes_struct *p)
{
	SPOOL_Q_ABORTPRINTER q_u;
	SPOOL_R_ABORTPRINTER r_u;
	prs_struct *data = &p->in_data.data;
	prs_struct *rdata = &p->out_data.rdata;

	ZERO_STRUCT(q_u);
	ZERO_STRUCT(r_u);

	if (!spoolss_io_q_abortprinter("", &q_u, data, 0)) {
		DEBUG(0,("spoolss_io_q_abortprinter: unable to unmarshall SPOOL_Q_ABORTPRINTER.\n"));
		return False;
	}

	r_u.status = _spoolss_abortprinter(p, &q_u, &r_u);

	if (!spoolss_io_r_abortprinter("",&r_u,rdata,0)) {
		DEBUG(0,("spoolss_io_r_abortprinter: unable to marshall SPOOL_R_ABORTPRINTER.\n"));
		return False;
	}

	return True;
}

/********************************************************************
 * api_spoolss_deleteprinter
 *
 * called from the spoolss dispatcher
 ********************************************************************/

static BOOL api_spoolss_deleteprinter(pipes_struct *p)
{
	SPOOL_Q_DELETEPRINTER q_u;
	SPOOL_R_DELETEPRINTER r_u;
	prs_struct *data = &p->in_data.data;
	prs_struct *rdata = &p->out_data.rdata;

	ZERO_STRUCT(q_u);
	ZERO_STRUCT(r_u);

	if (!spoolss_io_q_deleteprinter("", &q_u, data, 0)) {
		DEBUG(0,("spoolss_io_q_deleteprinter: unable to unmarshall SPOOL_Q_DELETEPRINTER.\n"));
		return False;
	}

	r_u.status = _spoolss_deleteprinter(p, &q_u, &r_u);

	if (!spoolss_io_r_deleteprinter("",&r_u,rdata,0)) {
		DEBUG(0,("spoolss_io_r_deleteprinter: unable to marshall SPOOL_R_DELETEPRINTER.\n"));
		return False;
	}

	return True;
}


/********************************************************************
 * api_spoolss_deleteprinterdriver
 *
 * called from the spoolss dispatcher
 ********************************************************************/

static BOOL api_spoolss_deleteprinterdriver(pipes_struct *p)
{
	SPOOL_Q_DELETEPRINTERDRIVER q_u;
	SPOOL_R_DELETEPRINTERDRIVER r_u;
	prs_struct *data = &p->in_data.data;
	prs_struct *rdata = &p->out_data.rdata;

	ZERO_STRUCT(q_u);
	ZERO_STRUCT(r_u);

	if (!spoolss_io_q_deleteprinterdriver("", &q_u, data, 0)) {
		DEBUG(0,("spoolss_io_q_deleteprinterdriver: unable to unmarshall SPOOL_Q_DELETEPRINTERDRIVER.\n"));
		return False;
	}

	r_u.status = _spoolss_deleteprinterdriver(p, &q_u, &r_u);

	if (!spoolss_io_r_deleteprinterdriver("",&r_u,rdata,0)) {
		DEBUG(0,("spoolss_io_r_deleteprinter: unable to marshall SPOOL_R_DELETEPRINTER.\n"));
		return False;
	}

	return True;
}


/********************************************************************
 * api_spoolss_rffpcnex
 * ReplyFindFirstPrinterChangeNotifyEx
 ********************************************************************/

static BOOL api_spoolss_rffpcnex(pipes_struct *p)
{
	SPOOL_Q_RFFPCNEX q_u;
	SPOOL_R_RFFPCNEX r_u;
	prs_struct *data = &p->in_data.data;
	prs_struct *rdata = &p->out_data.rdata;

	ZERO_STRUCT(q_u);
	ZERO_STRUCT(r_u);

	if (!spoolss_io_q_rffpcnex("", &q_u, data, 0)) {
		DEBUG(0,("spoolss_io_q_rffpcnex: unable to unmarshall SPOOL_Q_RFFPCNEX.\n"));
		return False;
	}

	r_u.status = _spoolss_rffpcnex(p, &q_u, &r_u);

	if (!spoolss_io_r_rffpcnex("", &r_u, rdata, 0)) {
		DEBUG(0,("spoolss_io_r_rffpcnex: unable to marshall SPOOL_R_RFFPCNEX.\n"));
		return False;
	}

	return True;
}


/********************************************************************
 * api_spoolss_rfnpcnex
 * ReplyFindNextPrinterChangeNotifyEx
 * called from the spoolss dispatcher

 * Note - this is the *ONLY* function that breaks the RPC call
 * symmetry in all the other calls. We need to do this to fix
 * the massive memory allocation problem with thousands of jobs...
 * JRA.
 ********************************************************************/

static BOOL api_spoolss_rfnpcnex(pipes_struct *p)
{
	SPOOL_Q_RFNPCNEX q_u;
	SPOOL_R_RFNPCNEX r_u;
	prs_struct *data = &p->in_data.data;
	prs_struct *rdata = &p->out_data.rdata;

	ZERO_STRUCT(q_u);
	ZERO_STRUCT(r_u);

	if (!spoolss_io_q_rfnpcnex("", &q_u, data, 0)) {
		DEBUG(0,("spoolss_io_q_rfnpcnex: unable to unmarshall SPOOL_Q_RFNPCNEX.\n"));
		return False;
	}

	r_u.status = _spoolss_rfnpcnex(p, &q_u, &r_u);

	if (!spoolss_io_r_rfnpcnex("", &r_u, rdata, 0)) {
		SAFE_FREE(r_u.info.data);
		DEBUG(0,("spoolss_io_r_rfnpcnex: unable to marshall SPOOL_R_RFNPCNEX.\n"));
		return False;
	}

	SAFE_FREE(r_u.info.data);

	return True;
}


/********************************************************************
 * api_spoolss_enumprinters
 * called from the spoolss dispatcher
 *
 ********************************************************************/

static BOOL api_spoolss_enumprinters(pipes_struct *p)
{
	SPOOL_Q_ENUMPRINTERS q_u;
	SPOOL_R_ENUMPRINTERS r_u;
	prs_struct *data = &p->in_data.data;
	prs_struct *rdata = &p->out_data.rdata;

	ZERO_STRUCT(q_u);
	ZERO_STRUCT(r_u);

	if (!spoolss_io_q_enumprinters("", &q_u, data, 0)) {
		DEBUG(0,("spoolss_io_q_enumprinters: unable to unmarshall SPOOL_Q_ENUMPRINTERS.\n"));
		return False;
	}

	r_u.status = _spoolss_enumprinters( p, &q_u, &r_u);

	if (!spoolss_io_r_enumprinters("", &r_u, rdata, 0)) {
		DEBUG(0,("spoolss_io_r_enumprinters: unable to marshall SPOOL_R_ENUMPRINTERS.\n"));
		return False;
	}

	return True;
}

/********************************************************************
 * api_spoolss_getprinter
 * called from the spoolss dispatcher
 *
 ********************************************************************/

static BOOL api_spoolss_getprinter(pipes_struct *p)
{
	SPOOL_Q_GETPRINTER q_u;
	SPOOL_R_GETPRINTER r_u;
	prs_struct *data = &p->in_data.data;
	prs_struct *rdata = &p->out_data.rdata;

	ZERO_STRUCT(q_u);
	ZERO_STRUCT(r_u);

	if(!spoolss_io_q_getprinter("", &q_u, data, 0)) {
		DEBUG(0,("spoolss_io_q_getprinter: unable to unmarshall SPOOL_Q_GETPRINTER.\n"));
		return False;
	}

	r_u.status = _spoolss_getprinter(p, &q_u, &r_u);

	if(!spoolss_io_r_getprinter("",&r_u,rdata,0)) {
		DEBUG(0,("spoolss_io_r_getprinter: unable to marshall SPOOL_R_GETPRINTER.\n"));
		return False;
	}

	return True;
}

/********************************************************************
 * api_spoolss_getprinter
 * called from the spoolss dispatcher
 *
 ********************************************************************/

static BOOL api_spoolss_getprinterdriver2(pipes_struct *p)
{
	SPOOL_Q_GETPRINTERDRIVER2 q_u;
	SPOOL_R_GETPRINTERDRIVER2 r_u;
	prs_struct *data = &p->in_data.data;
	prs_struct *rdata = &p->out_data.rdata;

	ZERO_STRUCT(q_u);
	ZERO_STRUCT(r_u);

	if(!spoolss_io_q_getprinterdriver2("", &q_u, data, 0)) {
		DEBUG(0,("spoolss_io_q_getprinterdriver2: unable to unmarshall SPOOL_Q_GETPRINTERDRIVER2.\n"));
		return False;
	}

	r_u.status = _spoolss_getprinterdriver2(p, &q_u, &r_u);
	
	if(!spoolss_io_r_getprinterdriver2("",&r_u,rdata,0)) {
		DEBUG(0,("spoolss_io_r_getprinterdriver2: unable to marshall SPOOL_R_GETPRINTERDRIVER2.\n"));
		return False;
	}
	
	return True;
}

/********************************************************************
 * api_spoolss_getprinter
 * called from the spoolss dispatcher
 *
 ********************************************************************/

static BOOL api_spoolss_startpageprinter(pipes_struct *p)
{
	SPOOL_Q_STARTPAGEPRINTER q_u;
	SPOOL_R_STARTPAGEPRINTER r_u;
	prs_struct *data = &p->in_data.data;
	prs_struct *rdata = &p->out_data.rdata;

	ZERO_STRUCT(q_u);
	ZERO_STRUCT(r_u);

	if(!spoolss_io_q_startpageprinter("", &q_u, data, 0)) {
		DEBUG(0,("spoolss_io_q_startpageprinter: unable to unmarshall SPOOL_Q_STARTPAGEPRINTER.\n"));
		return False;
	}

	r_u.status = _spoolss_startpageprinter(p, &q_u, &r_u);

	if(!spoolss_io_r_startpageprinter("",&r_u,rdata,0)) {
		DEBUG(0,("spoolss_io_r_startpageprinter: unable to marshall SPOOL_R_STARTPAGEPRINTER.\n"));
		return False;
	}

	return True;
}

/********************************************************************
 * api_spoolss_getprinter
 * called from the spoolss dispatcher
 *
 ********************************************************************/

static BOOL api_spoolss_endpageprinter(pipes_struct *p)
{
	SPOOL_Q_ENDPAGEPRINTER q_u;
	SPOOL_R_ENDPAGEPRINTER r_u;
	prs_struct *data = &p->in_data.data;
	prs_struct *rdata = &p->out_data.rdata;

	ZERO_STRUCT(q_u);
	ZERO_STRUCT(r_u);

	if(!spoolss_io_q_endpageprinter("", &q_u, data, 0)) {
		DEBUG(0,("spoolss_io_q_endpageprinter: unable to unmarshall SPOOL_Q_ENDPAGEPRINTER.\n"));
		return False;
	}

	r_u.status = _spoolss_endpageprinter(p, &q_u, &r_u);

	if(!spoolss_io_r_endpageprinter("",&r_u,rdata,0)) {
		DEBUG(0,("spoolss_io_r_endpageprinter: unable to marshall SPOOL_R_ENDPAGEPRINTER.\n"));
		return False;
	}

	return True;
}

/********************************************************************
********************************************************************/

static BOOL api_spoolss_startdocprinter(pipes_struct *p)
{
	SPOOL_Q_STARTDOCPRINTER q_u;
	SPOOL_R_STARTDOCPRINTER r_u;
	prs_struct *data = &p->in_data.data;
	prs_struct *rdata = &p->out_data.rdata;

	ZERO_STRUCT(q_u);
	ZERO_STRUCT(r_u);

	if(!spoolss_io_q_startdocprinter("", &q_u, data, 0)) {
		DEBUG(0,("spoolss_io_q_startdocprinter: unable to unmarshall SPOOL_Q_STARTDOCPRINTER.\n"));
		return False;
	}

	r_u.status = _spoolss_startdocprinter(p, &q_u, &r_u);

	if(!spoolss_io_r_startdocprinter("",&r_u,rdata,0)) {
		DEBUG(0,("spoolss_io_r_startdocprinter: unable to marshall SPOOL_R_STARTDOCPRINTER.\n"));
		return False;
	}

	return True;
}

/********************************************************************
********************************************************************/

static BOOL api_spoolss_enddocprinter(pipes_struct *p)
{
	SPOOL_Q_ENDDOCPRINTER q_u;
	SPOOL_R_ENDDOCPRINTER r_u;
	prs_struct *data = &p->in_data.data;
	prs_struct *rdata = &p->out_data.rdata;

	ZERO_STRUCT(q_u);
	ZERO_STRUCT(r_u);

	if(!spoolss_io_q_enddocprinter("", &q_u, data, 0)) {
		DEBUG(0,("spoolss_io_q_enddocprinter: unable to unmarshall SPOOL_Q_ENDDOCPRINTER.\n"));
		return False;
	}

	r_u.status = _spoolss_enddocprinter(p, &q_u, &r_u);

	if(!spoolss_io_r_enddocprinter("",&r_u,rdata,0)) {
		DEBUG(0,("spoolss_io_r_enddocprinter: unable to marshall SPOOL_R_ENDDOCPRINTER.\n"));
		return False;
	}

	return True;		
}

/********************************************************************
********************************************************************/

static BOOL api_spoolss_writeprinter(pipes_struct *p)
{
	SPOOL_Q_WRITEPRINTER q_u;
	SPOOL_R_WRITEPRINTER r_u;
	prs_struct *data = &p->in_data.data;
	prs_struct *rdata = &p->out_data.rdata;

	ZERO_STRUCT(q_u);
	ZERO_STRUCT(r_u);

	if(!spoolss_io_q_writeprinter("", &q_u, data, 0)) {
		DEBUG(0,("spoolss_io_q_writeprinter: unable to unmarshall SPOOL_Q_WRITEPRINTER.\n"));
		return False;
	}

	r_u.status = _spoolss_writeprinter(p, &q_u, &r_u);

	if(!spoolss_io_r_writeprinter("",&r_u,rdata,0)) {
		DEBUG(0,("spoolss_io_r_writeprinter: unable to marshall SPOOL_R_WRITEPRINTER.\n"));
		return False;
	}

	return True;
}

/****************************************************************************

****************************************************************************/

static BOOL api_spoolss_setprinter(pipes_struct *p)
{
	SPOOL_Q_SETPRINTER q_u;
	SPOOL_R_SETPRINTER r_u;
	prs_struct *data = &p->in_data.data;
	prs_struct *rdata = &p->out_data.rdata;

	ZERO_STRUCT(q_u);
	ZERO_STRUCT(r_u);

	if(!spoolss_io_q_setprinter("", &q_u, data, 0)) {
		DEBUG(0,("spoolss_io_q_setprinter: unable to unmarshall SPOOL_Q_SETPRINTER.\n"));
		return False;
	}
	
	r_u.status = _spoolss_setprinter(p, &q_u, &r_u);
	
	if(!spoolss_io_r_setprinter("",&r_u,rdata,0)) {
		DEBUG(0,("spoolss_io_r_setprinter: unable to marshall SPOOL_R_SETPRINTER.\n"));
		return False;
	}

	return True;
}

/****************************************************************************
****************************************************************************/

static BOOL api_spoolss_fcpn(pipes_struct *p)
{
	SPOOL_Q_FCPN q_u;
	SPOOL_R_FCPN r_u;
	prs_struct *data = &p->in_data.data;
	prs_struct *rdata = &p->out_data.rdata;

	ZERO_STRUCT(q_u);
	ZERO_STRUCT(r_u);

	if(!spoolss_io_q_fcpn("", &q_u, data, 0)) {
		DEBUG(0,("spoolss_io_q_fcpn: unable to unmarshall SPOOL_Q_FCPN.\n"));
		return False;
	}

	r_u.status = _spoolss_fcpn(p, &q_u, &r_u);

	if(!spoolss_io_r_fcpn("",&r_u,rdata,0)) {
		DEBUG(0,("spoolss_io_r_fcpn: unable to marshall SPOOL_R_FCPN.\n"));
		return False;
	}

	return True;
}

/****************************************************************************
****************************************************************************/

static BOOL api_spoolss_addjob(pipes_struct *p)
{
	SPOOL_Q_ADDJOB q_u;
	SPOOL_R_ADDJOB r_u;
	prs_struct *data = &p->in_data.data;
	prs_struct *rdata = &p->out_data.rdata;

	ZERO_STRUCT(q_u);
	ZERO_STRUCT(r_u);

	if(!spoolss_io_q_addjob("", &q_u, data, 0)) {
		DEBUG(0,("spoolss_io_q_addjob: unable to unmarshall SPOOL_Q_ADDJOB.\n"));
		return False;
	}

	r_u.status = _spoolss_addjob(p, &q_u, &r_u);
		
	if(!spoolss_io_r_addjob("",&r_u,rdata,0)) {
		DEBUG(0,("spoolss_io_r_addjob: unable to marshall SPOOL_R_ADDJOB.\n"));
		return False;
	}

	return True;		
}

/****************************************************************************
****************************************************************************/

static BOOL api_spoolss_enumjobs(pipes_struct *p)
{
	SPOOL_Q_ENUMJOBS q_u;
	SPOOL_R_ENUMJOBS r_u;
	prs_struct *data = &p->in_data.data;
	prs_struct *rdata = &p->out_data.rdata;

	ZERO_STRUCT(q_u);
	ZERO_STRUCT(r_u);

	if (!spoolss_io_q_enumjobs("", &q_u, data, 0)) {
		DEBUG(0,("spoolss_io_q_enumjobs: unable to unmarshall SPOOL_Q_ENUMJOBS.\n"));
		return False;
	}

	r_u.status = _spoolss_enumjobs(p, &q_u, &r_u);

	if (!spoolss_io_r_enumjobs("",&r_u,rdata,0)) {
		DEBUG(0,("spoolss_io_r_enumjobs: unable to marshall SPOOL_R_ENUMJOBS.\n"));
		return False;
	}

	return True;
}

/****************************************************************************
****************************************************************************/

static BOOL api_spoolss_schedulejob(pipes_struct *p)
{
	SPOOL_Q_SCHEDULEJOB q_u;
	SPOOL_R_SCHEDULEJOB r_u;
	prs_struct *data = &p->in_data.data;
	prs_struct *rdata = &p->out_data.rdata;

	ZERO_STRUCT(q_u);
	ZERO_STRUCT(r_u);

	if(!spoolss_io_q_schedulejob("", &q_u, data, 0)) {
		DEBUG(0,("spoolss_io_q_schedulejob: unable to unmarshall SPOOL_Q_SCHEDULEJOB.\n"));
		return False;
	}

	r_u.status = _spoolss_schedulejob(p, &q_u, &r_u);

	if(!spoolss_io_r_schedulejob("",&r_u,rdata,0)) {
		DEBUG(0,("spoolss_io_r_schedulejob: unable to marshall SPOOL_R_SCHEDULEJOB.\n"));
		return False;
	}

	return True;
}

/****************************************************************************
****************************************************************************/

static BOOL api_spoolss_setjob(pipes_struct *p)
{
	SPOOL_Q_SETJOB q_u;
	SPOOL_R_SETJOB r_u;
	prs_struct *data = &p->in_data.data;
	prs_struct *rdata = &p->out_data.rdata;

	ZERO_STRUCT(q_u);
	ZERO_STRUCT(r_u);

	if(!spoolss_io_q_setjob("", &q_u, data, 0)) {
		DEBUG(0,("spoolss_io_q_setjob: unable to unmarshall SPOOL_Q_SETJOB.\n"));
		return False;
	}

	r_u.status = _spoolss_setjob(p, &q_u, &r_u);

	if(!spoolss_io_r_setjob("",&r_u,rdata,0)) {
		DEBUG(0,("spoolss_io_r_setjob: unable to marshall SPOOL_R_SETJOB.\n"));
		return False;
	}

	return True;
}

/****************************************************************************
****************************************************************************/

static BOOL api_spoolss_enumprinterdrivers(pipes_struct *p)
{
	SPOOL_Q_ENUMPRINTERDRIVERS q_u;
	SPOOL_R_ENUMPRINTERDRIVERS r_u;
	prs_struct *data = &p->in_data.data;
	prs_struct *rdata = &p->out_data.rdata;

	ZERO_STRUCT(q_u);
	ZERO_STRUCT(r_u);

	if (!spoolss_io_q_enumprinterdrivers("", &q_u, data, 0)) {
		DEBUG(0,("spoolss_io_q_enumprinterdrivers: unable to unmarshall SPOOL_Q_ENUMPRINTERDRIVERS.\n"));
		return False;
	}

	r_u.status = _spoolss_enumprinterdrivers(p, &q_u, &r_u);

	if (!spoolss_io_r_enumprinterdrivers("",&r_u,rdata,0)) {
		DEBUG(0,("spoolss_io_r_enumprinterdrivers: unable to marshall SPOOL_R_ENUMPRINTERDRIVERS.\n"));
		return False;
	}

	return True;
}

/****************************************************************************
****************************************************************************/

static BOOL api_spoolss_getform(pipes_struct *p)
{
	SPOOL_Q_GETFORM q_u;
	SPOOL_R_GETFORM r_u;
	prs_struct *data = &p->in_data.data;
	prs_struct *rdata = &p->out_data.rdata;

	ZERO_STRUCT(q_u);
	ZERO_STRUCT(r_u);

	if (!spoolss_io_q_getform("", &q_u, data, 0)) {
		DEBUG(0,("spoolss_io_q_getform: unable to unmarshall SPOOL_Q_GETFORM.\n"));
		return False;
	}

	r_u.status = _spoolss_getform(p, &q_u, &r_u);

	if (!spoolss_io_r_getform("",&r_u,rdata,0)) {
		DEBUG(0,("spoolss_io_r_getform: unable to marshall SPOOL_R_GETFORM.\n"));
		return False;
	}

	return True;
}

/****************************************************************************
****************************************************************************/

static BOOL api_spoolss_enumforms(pipes_struct *p)
{
	SPOOL_Q_ENUMFORMS q_u;
	SPOOL_R_ENUMFORMS r_u;
	prs_struct *data = &p->in_data.data;
	prs_struct *rdata = &p->out_data.rdata;

	ZERO_STRUCT(q_u);
	ZERO_STRUCT(r_u);

	if (!spoolss_io_q_enumforms("", &q_u, data, 0)) {
		DEBUG(0,("spoolss_io_q_enumforms: unable to unmarshall SPOOL_Q_ENUMFORMS.\n"));
		return False;
	}

	r_u.status = _spoolss_enumforms(p, &q_u, &r_u);

	if (!spoolss_io_r_enumforms("",&r_u,rdata,0)) {
		DEBUG(0,("spoolss_io_r_enumforms: unable to marshall SPOOL_R_ENUMFORMS.\n"));
		return False;
	}

	return True;
}

/****************************************************************************
****************************************************************************/

static BOOL api_spoolss_enumports(pipes_struct *p)
{
	SPOOL_Q_ENUMPORTS q_u;
	SPOOL_R_ENUMPORTS r_u;
	prs_struct *data = &p->in_data.data;
	prs_struct *rdata = &p->out_data.rdata;

	ZERO_STRUCT(q_u);
	ZERO_STRUCT(r_u);

	if(!spoolss_io_q_enumports("", &q_u, data, 0)) {
		DEBUG(0,("spoolss_io_q_enumports: unable to unmarshall SPOOL_Q_ENUMPORTS.\n"));
		return False;
	}

	r_u.status = _spoolss_enumports(p, &q_u, &r_u);

	if (!spoolss_io_r_enumports("",&r_u,rdata,0)) {
		DEBUG(0,("spoolss_io_r_enumports: unable to marshall SPOOL_R_ENUMPORTS.\n"));
		return False;
	}

	return True;
}

/****************************************************************************
****************************************************************************/

static BOOL api_spoolss_addprinterex(pipes_struct *p)
{
	SPOOL_Q_ADDPRINTEREX q_u;
	SPOOL_R_ADDPRINTEREX r_u;
	prs_struct *data = &p->in_data.data;
	prs_struct *rdata = &p->out_data.rdata;
	
	ZERO_STRUCT(q_u);
	ZERO_STRUCT(r_u);
	
	if(!spoolss_io_q_addprinterex("", &q_u, data, 0)) {
		DEBUG(0,("spoolss_io_q_addprinterex: unable to unmarshall SPOOL_Q_ADDPRINTEREX.\n"));
		return False;
	}
	
	r_u.status = _spoolss_addprinterex(p, &q_u, &r_u);
				
	if(!spoolss_io_r_addprinterex("", &r_u, rdata, 0)) {
		DEBUG(0,("spoolss_io_r_addprinterex: unable to marshall SPOOL_R_ADDPRINTEREX.\n"));
		return False;
	}
	
	return True;
}

/****************************************************************************
****************************************************************************/

static BOOL api_spoolss_addprinterdriver(pipes_struct *p)
{
	SPOOL_Q_ADDPRINTERDRIVER q_u;
	SPOOL_R_ADDPRINTERDRIVER r_u;
	prs_struct *data = &p->in_data.data;
	prs_struct *rdata = &p->out_data.rdata;
	
	ZERO_STRUCT(q_u);
	ZERO_STRUCT(r_u);
	
	if(!spoolss_io_q_addprinterdriver("", &q_u, data, 0)) {
		DEBUG(0,("spoolss_io_q_addprinterdriver: unable to unmarshall SPOOL_Q_ADDPRINTERDRIVER.\n"));
		return False;
	}
	
	r_u.status = _spoolss_addprinterdriver(p, &q_u, &r_u);
				
	if(!spoolss_io_r_addprinterdriver("", &r_u, rdata, 0)) {
		DEBUG(0,("spoolss_io_r_addprinterdriver: unable to marshall SPOOL_R_ADDPRINTERDRIVER.\n"));
		return False;
	}
	
	return True;
}

/****************************************************************************
****************************************************************************/

static BOOL api_spoolss_getprinterdriverdirectory(pipes_struct *p)
{
	SPOOL_Q_GETPRINTERDRIVERDIR q_u;
	SPOOL_R_GETPRINTERDRIVERDIR r_u;
	prs_struct *data = &p->in_data.data;
	prs_struct *rdata = &p->out_data.rdata;

	ZERO_STRUCT(q_u);
	ZERO_STRUCT(r_u);

	if(!spoolss_io_q_getprinterdriverdir("", &q_u, data, 0)) {
		DEBUG(0,("spoolss_io_q_getprinterdriverdir: unable to unmarshall SPOOL_Q_GETPRINTERDRIVERDIR.\n"));
		return False;
	}

	r_u.status = _spoolss_getprinterdriverdirectory(p, &q_u, &r_u);

	if(!spoolss_io_r_getprinterdriverdir("", &r_u, rdata, 0)) {
		DEBUG(0,("spoolss_io_r_getprinterdriverdir: unable to marshall SPOOL_R_GETPRINTERDRIVERDIR.\n"));
		return False;
	}

	return True;
}

/****************************************************************************
****************************************************************************/

static BOOL api_spoolss_enumprinterdata(pipes_struct *p)
{
	SPOOL_Q_ENUMPRINTERDATA q_u;
	SPOOL_R_ENUMPRINTERDATA r_u;
	prs_struct *data = &p->in_data.data;
	prs_struct *rdata = &p->out_data.rdata;
	
	ZERO_STRUCT(q_u);
	ZERO_STRUCT(r_u);
	
	if(!spoolss_io_q_enumprinterdata("", &q_u, data, 0)) {
		DEBUG(0,("spoolss_io_q_enumprinterdata: unable to unmarshall SPOOL_Q_ENUMPRINTERDATA.\n"));
		return False;
	}
	
	r_u.status = _spoolss_enumprinterdata(p, &q_u, &r_u);
				
	if(!spoolss_io_r_enumprinterdata("", &r_u, rdata, 0)) {
		DEBUG(0,("spoolss_io_r_enumprinterdata: unable to marshall SPOOL_R_ENUMPRINTERDATA.\n"));
		return False;
	}

	return True;
}

/****************************************************************************
****************************************************************************/

static BOOL api_spoolss_setprinterdata(pipes_struct *p)
{
	SPOOL_Q_SETPRINTERDATA q_u;
	SPOOL_R_SETPRINTERDATA r_u;
	prs_struct *data = &p->in_data.data;
	prs_struct *rdata = &p->out_data.rdata;
	
	ZERO_STRUCT(q_u);
	ZERO_STRUCT(r_u);
	
	if(!spoolss_io_q_setprinterdata("", &q_u, data, 0)) {
		DEBUG(0,("spoolss_io_q_setprinterdata: unable to unmarshall SPOOL_Q_SETPRINTERDATA.\n"));
		return False;
	}
	
	r_u.status = _spoolss_setprinterdata(p, &q_u, &r_u);
				
	if(!spoolss_io_r_setprinterdata("", &r_u, rdata, 0)) {
		DEBUG(0,("spoolss_io_r_setprinterdata: unable to marshall SPOOL_R_SETPRINTERDATA.\n"));
		return False;
	}

	return True;
}

/****************************************************************************
****************************************************************************/
static BOOL api_spoolss_reset_printer(pipes_struct *p)
{
	SPOOL_Q_RESETPRINTER q_u;
	SPOOL_R_RESETPRINTER r_u;
	prs_struct *data = &p->in_data.data;
	prs_struct *rdata = &p->out_data.rdata;

	ZERO_STRUCT(q_u);
	ZERO_STRUCT(r_u);

	if(!spoolss_io_q_resetprinter("", &q_u, data, 0)) {
		DEBUG(0,("spoolss_io_q_setprinterdata: unable to unmarshall SPOOL_Q_SETPRINTERDATA.\n"));
		return False;
	}
	
	r_u.status = _spoolss_resetprinter(p, &q_u, &r_u);

	if(!spoolss_io_r_resetprinter("", &r_u, rdata, 0)) {
		DEBUG(0,("spoolss_io_r_setprinterdata: unable to marshall SPOOL_R_RESETPRINTER.\n"));
		return False;
	}

	return True;
}

/****************************************************************************
****************************************************************************/
static BOOL api_spoolss_addform(pipes_struct *p)
{
	SPOOL_Q_ADDFORM q_u;
	SPOOL_R_ADDFORM r_u;
	prs_struct *data = &p->in_data.data;
	prs_struct *rdata = &p->out_data.rdata;

	ZERO_STRUCT(q_u);
	ZERO_STRUCT(r_u);
	
	if(!spoolss_io_q_addform("", &q_u, data, 0)) {
		DEBUG(0,("spoolss_io_q_addform: unable to unmarshall SPOOL_Q_ADDFORM.\n"));
		return False;
	}
	
	r_u.status = _spoolss_addform(p, &q_u, &r_u);
	
	if(!spoolss_io_r_addform("", &r_u, rdata, 0)) {
		DEBUG(0,("spoolss_io_r_addform: unable to marshall SPOOL_R_ADDFORM.\n"));
		return False;
	}

	return True;
}

/****************************************************************************
****************************************************************************/

static BOOL api_spoolss_deleteform(pipes_struct *p)
{
	SPOOL_Q_DELETEFORM q_u;
	SPOOL_R_DELETEFORM r_u;
	prs_struct *data = &p->in_data.data;
	prs_struct *rdata = &p->out_data.rdata;

	ZERO_STRUCT(q_u);
	ZERO_STRUCT(r_u);
	
	if(!spoolss_io_q_deleteform("", &q_u, data, 0)) {
		DEBUG(0,("spoolss_io_q_deleteform: unable to unmarshall SPOOL_Q_DELETEFORM.\n"));
		return False;
	}
	
	r_u.status = _spoolss_deleteform(p, &q_u, &r_u);
	
	if(!spoolss_io_r_deleteform("", &r_u, rdata, 0)) {
		DEBUG(0,("spoolss_io_r_deleteform: unable to marshall SPOOL_R_DELETEFORM.\n"));
		return False;
	}

	return True;
}

/****************************************************************************
****************************************************************************/

static BOOL api_spoolss_setform(pipes_struct *p)
{
	SPOOL_Q_SETFORM q_u;
	SPOOL_R_SETFORM r_u;
	prs_struct *data = &p->in_data.data;
	prs_struct *rdata = &p->out_data.rdata;

	ZERO_STRUCT(q_u);
	ZERO_STRUCT(r_u);
	
	if(!spoolss_io_q_setform("", &q_u, data, 0)) {
		DEBUG(0,("spoolss_io_q_setform: unable to unmarshall SPOOL_Q_SETFORM.\n"));
		return False;
	}
	
	r_u.status = _spoolss_setform(p, &q_u, &r_u);
				      
	if(!spoolss_io_r_setform("", &r_u, rdata, 0)) {
		DEBUG(0,("spoolss_io_r_setform: unable to marshall SPOOL_R_SETFORM.\n"));
		return False;
	}

	return True;
}

/****************************************************************************
****************************************************************************/

static BOOL api_spoolss_enumprintprocessors(pipes_struct *p)
{
	SPOOL_Q_ENUMPRINTPROCESSORS q_u;
	SPOOL_R_ENUMPRINTPROCESSORS r_u;
	prs_struct *data = &p->in_data.data;
	prs_struct *rdata = &p->out_data.rdata;

	ZERO_STRUCT(q_u);
	ZERO_STRUCT(r_u);
	
	if(!spoolss_io_q_enumprintprocessors("", &q_u, data, 0)) {
		DEBUG(0,("spoolss_io_q_enumprintprocessors: unable to unmarshall SPOOL_Q_ENUMPRINTPROCESSORS.\n"));
		return False;
	}
	
	r_u.status = _spoolss_enumprintprocessors(p, &q_u, &r_u);

	if(!spoolss_io_r_enumprintprocessors("", &r_u, rdata, 0)) {
		DEBUG(0,("spoolss_io_r_enumprintprocessors: unable to marshall SPOOL_R_ENUMPRINTPROCESSORS.\n"));
		return False;
	}
	
	return True;
}

/****************************************************************************
****************************************************************************/

static BOOL api_spoolss_addprintprocessor(pipes_struct *p)
{
	SPOOL_Q_ADDPRINTPROCESSOR q_u;
	SPOOL_R_ADDPRINTPROCESSOR r_u;
	prs_struct *data = &p->in_data.data;
	prs_struct *rdata = &p->out_data.rdata;

	ZERO_STRUCT(q_u);
	ZERO_STRUCT(r_u);
	
	if(!spoolss_io_q_addprintprocessor("", &q_u, data, 0)) {
		DEBUG(0,("spoolss_io_q_addprintprocessor: unable to unmarshall SPOOL_Q_ADDPRINTPROCESSOR.\n"));
		return False;
	}
	
	/* for now, just indicate success and ignore the add.  We'll
	   automatically set the winprint processor for printer
	   entries later.  Used to debug the LexMark Optra S 1855 PCL
	   driver --jerry */
	r_u.status = WERR_OK;

	if(!spoolss_io_r_addprintprocessor("", &r_u, rdata, 0)) {
		DEBUG(0,("spoolss_io_r_addprintprocessor: unable to marshall SPOOL_R_ADDPRINTPROCESSOR.\n"));
		return False;
	}
	
	return True;
}

/****************************************************************************
****************************************************************************/

static BOOL api_spoolss_enumprintprocdatatypes(pipes_struct *p)
{
	SPOOL_Q_ENUMPRINTPROCDATATYPES q_u;
	SPOOL_R_ENUMPRINTPROCDATATYPES r_u;
	prs_struct *data = &p->in_data.data;
	prs_struct *rdata = &p->out_data.rdata;

	ZERO_STRUCT(q_u);
	ZERO_STRUCT(r_u);
	
	if(!spoolss_io_q_enumprintprocdatatypes("", &q_u, data, 0)) {
		DEBUG(0,("spoolss_io_q_enumprintprocdatatypes: unable to unmarshall SPOOL_Q_ENUMPRINTPROCDATATYPES.\n"));
		return False;
	}
	
	r_u.status = _spoolss_enumprintprocdatatypes(p, &q_u, &r_u);

	if(!spoolss_io_r_enumprintprocdatatypes("", &r_u, rdata, 0)) {
		DEBUG(0,("spoolss_io_r_enumprintprocdatatypes: unable to marshall SPOOL_R_ENUMPRINTPROCDATATYPES.\n"));
		return False;
	}
	
	return True;
}

/****************************************************************************
****************************************************************************/

static BOOL api_spoolss_enumprintmonitors(pipes_struct *p)
{
	SPOOL_Q_ENUMPRINTMONITORS q_u;
	SPOOL_R_ENUMPRINTMONITORS r_u;
	prs_struct *data = &p->in_data.data;
	prs_struct *rdata = &p->out_data.rdata;

	ZERO_STRUCT(q_u);
	ZERO_STRUCT(r_u);
	
	if (!spoolss_io_q_enumprintmonitors("", &q_u, data, 0)) {
		DEBUG(0,("spoolss_io_q_enumprintmonitors: unable to unmarshall SPOOL_Q_ENUMPRINTMONITORS.\n"));
		return False;
	}
		
	r_u.status = _spoolss_enumprintmonitors(p, &q_u, &r_u);

	if (!spoolss_io_r_enumprintmonitors("", &r_u, rdata, 0)) {
		DEBUG(0,("spoolss_io_r_enumprintmonitors: unable to marshall SPOOL_R_ENUMPRINTMONITORS.\n"));
		return False;
	}
	
	return True;
}

/****************************************************************************
****************************************************************************/

static BOOL api_spoolss_getjob(pipes_struct *p)
{
	SPOOL_Q_GETJOB q_u;
	SPOOL_R_GETJOB r_u;
	prs_struct *data = &p->in_data.data;
	prs_struct *rdata = &p->out_data.rdata;
	
	ZERO_STRUCT(q_u);
	ZERO_STRUCT(r_u);
	
	if(!spoolss_io_q_getjob("", &q_u, data, 0)) {
		DEBUG(0,("spoolss_io_q_getjob: unable to unmarshall SPOOL_Q_GETJOB.\n"));
		return False;
	}

	r_u.status = _spoolss_getjob(p, &q_u, &r_u);
	
	if(!spoolss_io_r_getjob("",&r_u,rdata,0)) {
		DEBUG(0,("spoolss_io_r_getjob: unable to marshall SPOOL_R_GETJOB.\n"));
		return False;
	}
		
	return True;
}

/********************************************************************
 * api_spoolss_getprinterdataex
 *
 * called from the spoolss dispatcher
 ********************************************************************/

static BOOL api_spoolss_getprinterdataex(pipes_struct *p)
{
	SPOOL_Q_GETPRINTERDATAEX q_u;
	SPOOL_R_GETPRINTERDATAEX r_u;
	prs_struct *data = &p->in_data.data;
	prs_struct *rdata = &p->out_data.rdata;

	ZERO_STRUCT(q_u);
	ZERO_STRUCT(r_u);

	/* read the stream and fill the struct */
	if (!spoolss_io_q_getprinterdataex("", &q_u, data, 0)) {
		DEBUG(0,("spoolss_io_q_getprinterdataex: unable to unmarshall SPOOL_Q_GETPRINTERDATAEX.\n"));
		return False;
	}
	
	r_u.status = _spoolss_getprinterdataex( p, &q_u, &r_u);

	if (!spoolss_io_r_getprinterdataex("", &r_u, rdata, 0)) {
		DEBUG(0,("spoolss_io_r_getprinterdataex: unable to marshall SPOOL_R_GETPRINTERDATAEX.\n"));
		return False;
	}

	return True;
}

/****************************************************************************
****************************************************************************/

static BOOL api_spoolss_setprinterdataex(pipes_struct *p)
{
	SPOOL_Q_SETPRINTERDATAEX q_u;
	SPOOL_R_SETPRINTERDATAEX r_u;
	prs_struct *data = &p->in_data.data;
	prs_struct *rdata = &p->out_data.rdata;
	
	ZERO_STRUCT(q_u);
	ZERO_STRUCT(r_u);
	
	if(!spoolss_io_q_setprinterdataex("", &q_u, data, 0)) {
		DEBUG(0,("spoolss_io_q_setprinterdataex: unable to unmarshall SPOOL_Q_SETPRINTERDATAEX.\n"));
		return False;
	}
	
	r_u.status = _spoolss_setprinterdataex(p, &q_u, &r_u);
				
	if(!spoolss_io_r_setprinterdataex("", &r_u, rdata, 0)) {
		DEBUG(0,("spoolss_io_r_setprinterdataex: unable to marshall SPOOL_R_SETPRINTERDATAEX.\n"));
		return False;
	}

	return True;
}


/****************************************************************************
****************************************************************************/

static BOOL api_spoolss_enumprinterkey(pipes_struct *p)
{
	SPOOL_Q_ENUMPRINTERKEY q_u;
	SPOOL_R_ENUMPRINTERKEY r_u;
	prs_struct *data = &p->in_data.data;
	prs_struct *rdata = &p->out_data.rdata;
	
	ZERO_STRUCT(q_u);
	ZERO_STRUCT(r_u);
	
	if(!spoolss_io_q_enumprinterkey("", &q_u, data, 0)) {
		DEBUG(0,("spoolss_io_q_setprinterkey: unable to unmarshall SPOOL_Q_ENUMPRINTERKEY.\n"));
		return False;
	}
	
	r_u.status = _spoolss_enumprinterkey(p, &q_u, &r_u);
				
	if(!spoolss_io_r_enumprinterkey("", &r_u, rdata, 0)) {
		DEBUG(0,("spoolss_io_r_enumprinterkey: unable to marshall SPOOL_R_ENUMPRINTERKEY.\n"));
		return False;
	}

	return True;
}

/****************************************************************************
****************************************************************************/

static BOOL api_spoolss_enumprinterdataex(pipes_struct *p)
{
	SPOOL_Q_ENUMPRINTERDATAEX q_u;
	SPOOL_R_ENUMPRINTERDATAEX r_u;
	prs_struct *data = &p->in_data.data;
	prs_struct *rdata = &p->out_data.rdata;
	
	ZERO_STRUCT(q_u);
	ZERO_STRUCT(r_u);
	
	if(!spoolss_io_q_enumprinterdataex("", &q_u, data, 0)) {
		DEBUG(0,("spoolss_io_q_enumprinterdataex: unable to unmarshall SPOOL_Q_ENUMPRINTERDATAEX.\n"));
		return False;
	}
	
	r_u.status = _spoolss_enumprinterdataex(p, &q_u, &r_u);
				
	if(!spoolss_io_r_enumprinterdataex("", &r_u, rdata, 0)) {
		DEBUG(0,("spoolss_io_r_enumprinterdataex: unable to marshall SPOOL_R_ENUMPRINTERDATAEX.\n"));
		return False;
	}

	return True;
}

/****************************************************************************
****************************************************************************/

static BOOL api_spoolss_getprintprocessordirectory(pipes_struct *p)
{
	SPOOL_Q_GETPRINTPROCESSORDIRECTORY q_u;
	SPOOL_R_GETPRINTPROCESSORDIRECTORY r_u;
	prs_struct *data = &p->in_data.data;
	prs_struct *rdata = &p->out_data.rdata;
	
	ZERO_STRUCT(q_u);
	ZERO_STRUCT(r_u);
	
	if(!spoolss_io_q_getprintprocessordirectory("", &q_u, data, 0)) {
		DEBUG(0,("spoolss_io_q_getprintprocessordirectory: unable to unmarshall SPOOL_Q_GETPRINTPROCESSORDIRECTORY.\n"));
		return False;
	}
	
	r_u.status = _spoolss_getprintprocessordirectory(p, &q_u, &r_u);
				
	if(!spoolss_io_r_getprintprocessordirectory("", &r_u, rdata, 0)) {
		DEBUG(0,("spoolss_io_r_getprintprocessordirectory: unable to marshall SPOOL_R_GETPRINTPROCESSORDIRECTORY.\n"));
		return False;
	}
	
	return True;
}

/****************************************************************************
****************************************************************************/

static BOOL api_spoolss_deleteprinterdataex(pipes_struct *p)
{
	SPOOL_Q_DELETEPRINTERDATAEX q_u;
	SPOOL_R_DELETEPRINTERDATAEX r_u;
	prs_struct *data = &p->in_data.data;
	prs_struct *rdata = &p->out_data.rdata;
	
	ZERO_STRUCT(q_u);
	ZERO_STRUCT(r_u);
	
	if(!spoolss_io_q_deleteprinterdataex("", &q_u, data, 0)) {
		DEBUG(0,("spoolss_io_q_deleteprinterdataex: unable to unmarshall SPOOL_Q_DELETEPRINTERDATAEX.\n"));
		return False;
	}
	
	r_u.status = _spoolss_deleteprinterdataex(p, &q_u, &r_u);
				
	if(!spoolss_io_r_deleteprinterdataex("", &r_u, rdata, 0)) {
		DEBUG(0,("spoolss_io_r_deleteprinterdataex: unable to marshall SPOOL_R_DELETEPRINTERDATAEX.\n"));
		return False;
	}
	
	return True;
}

/****************************************************************************
****************************************************************************/

static BOOL api_spoolss_deleteprinterkey(pipes_struct *p)
{
	SPOOL_Q_DELETEPRINTERKEY q_u;
	SPOOL_R_DELETEPRINTERKEY r_u;
	prs_struct *data = &p->in_data.data;
	prs_struct *rdata = &p->out_data.rdata;
	
	ZERO_STRUCT(q_u);
	ZERO_STRUCT(r_u);
	
	if(!spoolss_io_q_deleteprinterkey("", &q_u, data, 0)) {
		DEBUG(0,("spoolss_io_q_deleteprinterkey: unable to unmarshall SPOOL_Q_DELETEPRINTERKEY.\n"));
		return False;
	}
	
	r_u.status = _spoolss_deleteprinterkey(p, &q_u, &r_u);
				
	if(!spoolss_io_r_deleteprinterkey("", &r_u, rdata, 0)) {
		DEBUG(0,("spoolss_io_r_deleteprinterkey: unable to marshall SPOOL_R_DELETEPRINTERKEY.\n"));
		return False;
	}
	
	return True;
}

/****************************************************************************
****************************************************************************/

static BOOL api_spoolss_addprinterdriverex(pipes_struct *p)
{
	SPOOL_Q_ADDPRINTERDRIVEREX q_u;
	SPOOL_R_ADDPRINTERDRIVEREX r_u;
	prs_struct *data = &p->in_data.data;
	prs_struct *rdata = &p->out_data.rdata;
	
	ZERO_STRUCT(q_u);
	ZERO_STRUCT(r_u);
	
	if(!spoolss_io_q_addprinterdriverex("", &q_u, data, 0)) {
		DEBUG(0,("spoolss_io_q_addprinterdriverex: unable to unmarshall SPOOL_Q_ADDPRINTERDRIVEREX.\n"));
		return False;
	}
	
	r_u.status = _spoolss_addprinterdriverex(p, &q_u, &r_u);
				
	if(!spoolss_io_r_addprinterdriverex("", &r_u, rdata, 0)) {
		DEBUG(0,("spoolss_io_r_addprinterdriverex: unable to marshall SPOOL_R_ADDPRINTERDRIVEREX.\n"));
		return False;
	}
	
	return True;
}

/****************************************************************************
****************************************************************************/

static BOOL api_spoolss_deleteprinterdriverex(pipes_struct *p)
{
	SPOOL_Q_DELETEPRINTERDRIVEREX q_u;
	SPOOL_R_DELETEPRINTERDRIVEREX r_u;
	prs_struct *data = &p->in_data.data;
	prs_struct *rdata = &p->out_data.rdata;
	
	ZERO_STRUCT(q_u);
	ZERO_STRUCT(r_u);
	
	if(!spoolss_io_q_deleteprinterdriverex("", &q_u, data, 0)) {
		DEBUG(0,("spoolss_io_q_deleteprinterdriverex: unable to unmarshall SPOOL_Q_DELETEPRINTERDRIVEREX.\n"));
		return False;
	}
	
	r_u.status = _spoolss_deleteprinterdriverex(p, &q_u, &r_u);
				
	if(!spoolss_io_r_deleteprinterdriverex("", &r_u, rdata, 0)) {
		DEBUG(0,("spoolss_io_r_deleteprinterdriverex: unable to marshall SPOOL_R_DELETEPRINTERDRIVEREX.\n"));
		return False;
	}
	
	return True;
}

/****************************************************************************
****************************************************************************/

static BOOL api_spoolss_xcvdataport(pipes_struct *p)
{
	SPOOL_Q_XCVDATAPORT q_u;
	SPOOL_R_XCVDATAPORT r_u;
	prs_struct *data = &p->in_data.data;
	prs_struct *rdata = &p->out_data.rdata;
	
	ZERO_STRUCT(q_u);
	ZERO_STRUCT(r_u);
	
	if(!spoolss_io_q_xcvdataport("", &q_u, data, 0)) {
		DEBUG(0,("spoolss_io_q_replyopenprinter: unable to unmarshall SPOOL_Q_XCVDATAPORT.\n"));
		return False;
	}
	
	r_u.status = _spoolss_xcvdataport(p, &q_u, &r_u);
				
	if(!spoolss_io_r_xcvdataport("", &r_u, rdata, 0)) {
		DEBUG(0,("spoolss_io_r_replyopenprinter: unable to marshall SPOOL_R_XCVDATAPORT.\n"));
		return False;
	}
	
	return True;
}

/*******************************************************************
\pipe\spoolss commands
********************************************************************/

  struct api_struct api_spoolss_cmds[] = 
    {
 {"SPOOLSS_OPENPRINTER",               SPOOLSS_OPENPRINTER,               api_spoolss_open_printer              },
 {"SPOOLSS_OPENPRINTEREX",             SPOOLSS_OPENPRINTEREX,             api_spoolss_open_printer_ex           },
 {"SPOOLSS_GETPRINTERDATA",            SPOOLSS_GETPRINTERDATA,            api_spoolss_getprinterdata            },
 {"SPOOLSS_CLOSEPRINTER",              SPOOLSS_CLOSEPRINTER,              api_spoolss_closeprinter              },
 {"SPOOLSS_DELETEPRINTER",             SPOOLSS_DELETEPRINTER,             api_spoolss_deleteprinter             },
 {"SPOOLSS_ABORTPRINTER",              SPOOLSS_ABORTPRINTER,              api_spoolss_abortprinter              },
 {"SPOOLSS_RFFPCNEX",                  SPOOLSS_RFFPCNEX,                  api_spoolss_rffpcnex                  },
 {"SPOOLSS_RFNPCNEX",                  SPOOLSS_RFNPCNEX,                  api_spoolss_rfnpcnex                  },
 {"SPOOLSS_ENUMPRINTERS",              SPOOLSS_ENUMPRINTERS,              api_spoolss_enumprinters              },
 {"SPOOLSS_GETPRINTER",                SPOOLSS_GETPRINTER,                api_spoolss_getprinter                },
 {"SPOOLSS_GETPRINTERDRIVER2",         SPOOLSS_GETPRINTERDRIVER2,         api_spoolss_getprinterdriver2         }, 
 {"SPOOLSS_STARTPAGEPRINTER",          SPOOLSS_STARTPAGEPRINTER,          api_spoolss_startpageprinter          },
 {"SPOOLSS_ENDPAGEPRINTER",            SPOOLSS_ENDPAGEPRINTER,            api_spoolss_endpageprinter            }, 
 {"SPOOLSS_STARTDOCPRINTER",           SPOOLSS_STARTDOCPRINTER,           api_spoolss_startdocprinter           },
 {"SPOOLSS_ENDDOCPRINTER",             SPOOLSS_ENDDOCPRINTER,             api_spoolss_enddocprinter             },
 {"SPOOLSS_WRITEPRINTER",              SPOOLSS_WRITEPRINTER,              api_spoolss_writeprinter              },
 {"SPOOLSS_SETPRINTER",                SPOOLSS_SETPRINTER,                api_spoolss_setprinter                },
 {"SPOOLSS_FCPN",                      SPOOLSS_FCPN,                      api_spoolss_fcpn		        },
 {"SPOOLSS_ADDJOB",                    SPOOLSS_ADDJOB,                    api_spoolss_addjob                    },
 {"SPOOLSS_ENUMJOBS",                  SPOOLSS_ENUMJOBS,                  api_spoolss_enumjobs                  },
 {"SPOOLSS_SCHEDULEJOB",               SPOOLSS_SCHEDULEJOB,               api_spoolss_schedulejob               },
 {"SPOOLSS_SETJOB",                    SPOOLSS_SETJOB,                    api_spoolss_setjob                    },
 {"SPOOLSS_ENUMFORMS",                 SPOOLSS_ENUMFORMS,                 api_spoolss_enumforms                 },
 {"SPOOLSS_ENUMPORTS",                 SPOOLSS_ENUMPORTS,                 api_spoolss_enumports                 },
 {"SPOOLSS_ENUMPRINTERDRIVERS",        SPOOLSS_ENUMPRINTERDRIVERS,        api_spoolss_enumprinterdrivers        },
 {"SPOOLSS_ADDPRINTEREX",              SPOOLSS_ADDPRINTEREX,              api_spoolss_addprinterex              },
 {"SPOOLSS_ADDPRINTERDRIVER",          SPOOLSS_ADDPRINTERDRIVER,          api_spoolss_addprinterdriver          },
 {"SPOOLSS_DELETEPRINTERDRIVER",       SPOOLSS_DELETEPRINTERDRIVER,       api_spoolss_deleteprinterdriver       },
 {"SPOOLSS_GETPRINTERDRIVERDIRECTORY", SPOOLSS_GETPRINTERDRIVERDIRECTORY, api_spoolss_getprinterdriverdirectory },
 {"SPOOLSS_ENUMPRINTERDATA",           SPOOLSS_ENUMPRINTERDATA,           api_spoolss_enumprinterdata           },
 {"SPOOLSS_SETPRINTERDATA",            SPOOLSS_SETPRINTERDATA,            api_spoolss_setprinterdata            },
 {"SPOOLSS_RESETPRINTER",              SPOOLSS_RESETPRINTER,              api_spoolss_reset_printer             },
 {"SPOOLSS_DELETEPRINTERDATA",         SPOOLSS_DELETEPRINTERDATA,         api_spoolss_deleteprinterdata         },
 {"SPOOLSS_ADDFORM",                   SPOOLSS_ADDFORM,                   api_spoolss_addform                   },
 {"SPOOLSS_DELETEFORM",                SPOOLSS_DELETEFORM,                api_spoolss_deleteform                },
 {"SPOOLSS_GETFORM",                   SPOOLSS_GETFORM,                   api_spoolss_getform                   },
 {"SPOOLSS_SETFORM",                   SPOOLSS_SETFORM,                   api_spoolss_setform                   },
 {"SPOOLSS_ADDPRINTPROCESSOR",         SPOOLSS_ADDPRINTPROCESSOR,         api_spoolss_addprintprocessor         },
 {"SPOOLSS_ENUMPRINTPROCESSORS",       SPOOLSS_ENUMPRINTPROCESSORS,       api_spoolss_enumprintprocessors       },
 {"SPOOLSS_ENUMMONITORS",              SPOOLSS_ENUMMONITORS,              api_spoolss_enumprintmonitors         },
 {"SPOOLSS_GETJOB",                    SPOOLSS_GETJOB,                    api_spoolss_getjob                    },
 {"SPOOLSS_ENUMPRINTPROCDATATYPES",    SPOOLSS_ENUMPRINTPROCDATATYPES,    api_spoolss_enumprintprocdatatypes    },
 {"SPOOLSS_GETPRINTERDATAEX",          SPOOLSS_GETPRINTERDATAEX,          api_spoolss_getprinterdataex          },
 {"SPOOLSS_SETPRINTERDATAEX",          SPOOLSS_SETPRINTERDATAEX,          api_spoolss_setprinterdataex          },
 {"SPOOLSS_DELETEPRINTERDATAEX",       SPOOLSS_DELETEPRINTERDATAEX,       api_spoolss_deleteprinterdataex       },
 {"SPOOLSS_ENUMPRINTERDATAEX",         SPOOLSS_ENUMPRINTERDATAEX,         api_spoolss_enumprinterdataex         },
 {"SPOOLSS_ENUMPRINTERKEY",            SPOOLSS_ENUMPRINTERKEY,            api_spoolss_enumprinterkey            },
 {"SPOOLSS_DELETEPRINTERKEY",          SPOOLSS_DELETEPRINTERKEY,          api_spoolss_deleteprinterkey          },
 {"SPOOLSS_GETPRINTPROCESSORDIRECTORY",SPOOLSS_GETPRINTPROCESSORDIRECTORY,api_spoolss_getprintprocessordirectory},
 {"SPOOLSS_ADDPRINTERDRIVEREX",        SPOOLSS_ADDPRINTERDRIVEREX,        api_spoolss_addprinterdriverex        },
 {"SPOOLSS_DELETEPRINTERDRIVEREX",     SPOOLSS_DELETEPRINTERDRIVEREX,     api_spoolss_deleteprinterdriverex     },
 {"SPOOLSS_XCVDATAPORT",               SPOOLSS_XCVDATAPORT,               api_spoolss_xcvdataport               },
};

void spoolss_get_pipe_fns( struct api_struct **fns, int *n_fns )
{
	*fns = api_spoolss_cmds;
	*n_fns = sizeof(api_spoolss_cmds) / sizeof(struct api_struct);
}

NTSTATUS rpc_spoolss_init(void)
{
  return rpc_pipe_register_commands(SMB_RPC_INTERFACE_VERSION, "spoolss", "spoolss", api_spoolss_cmds,
				    sizeof(api_spoolss_cmds) / sizeof(struct api_struct));
}
