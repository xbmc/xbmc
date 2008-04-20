/*
 *  Unix SMB/CIFS implementation.
 *  RPC Pipe client / server routines
 *  Copyright (C) Gerald Carter                   2005.
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
#include "rpc_client.h"

struct svc_state_msg {
	uint32 flag;
	const char *message;
};

static struct svc_state_msg state_msg_table[] = {
	{ SVCCTL_STOPPED,            "stopped" },
	{ SVCCTL_START_PENDING,      "start pending" },
	{ SVCCTL_STOP_PENDING,       "stop pending" },
	{ SVCCTL_RUNNING,            "running" },
	{ SVCCTL_CONTINUE_PENDING,   "resume pending" },
	{ SVCCTL_PAUSE_PENDING,      "pause pending" },
	{ SVCCTL_PAUSED,             "paused" },
	{ 0,                          NULL }
};
	

/********************************************************************
********************************************************************/
const char* svc_status_string( uint32 state )
{
	static fstring msg;
	int i;
	
	fstr_sprintf( msg, "Unknown State [%d]", state );
	
	for ( i=0; state_msg_table[i].message; i++ ) {
		if ( state_msg_table[i].flag == state ) {
			fstrcpy( msg, state_msg_table[i].message );
			break;	
		}
	}
	
	return msg;
}

/********************************************************************
********************************************************************/

WERROR rpccli_svcctl_open_scm(struct rpc_pipe_client *cli, TALLOC_CTX *mem_ctx, 
                              POLICY_HND *hSCM, uint32 access_desired )
{
	SVCCTL_Q_OPEN_SCMANAGER in;
	SVCCTL_R_OPEN_SCMANAGER out;
	prs_struct qbuf, rbuf;
	fstring server;
	
	ZERO_STRUCT(in);
	ZERO_STRUCT(out);
	
	/* leave the database name NULL to get the default service db */

	in.database = NULL;

	/* set the server name */

	if ( !(in.servername = TALLOC_P( mem_ctx, UNISTR2 )) )
		return WERR_NOMEM;
	fstr_sprintf( server, "\\\\%s", cli->cli->desthost );
	init_unistr2( in.servername, server, UNI_STR_TERMINATE );

	in.access = access_desired;
	
	CLI_DO_RPC_WERR( cli, mem_ctx, PI_SVCCTL, SVCCTL_OPEN_SCMANAGER_W, 
	            in, out, 
	            qbuf, rbuf,
	            svcctl_io_q_open_scmanager,
	            svcctl_io_r_open_scmanager, 
	            WERR_GENERAL_FAILURE );
	
	if ( !W_ERROR_IS_OK( out.status ) )
		return out.status;

	memcpy( hSCM, &out.handle, sizeof(POLICY_HND) );
	
	return out.status;
}

/********************************************************************
********************************************************************/

WERROR rpccli_svcctl_open_service( struct rpc_pipe_client *cli, TALLOC_CTX *mem_ctx, 
                                POLICY_HND *hSCM, POLICY_HND *hService, 
				const char *servicename, uint32 access_desired )
{
	SVCCTL_Q_OPEN_SERVICE in;
	SVCCTL_R_OPEN_SERVICE out;
	prs_struct qbuf, rbuf;
	
	ZERO_STRUCT(in);
	ZERO_STRUCT(out);
	
	memcpy( &in.handle, hSCM, sizeof(POLICY_HND) );
	init_unistr2( &in.servicename, servicename, UNI_STR_TERMINATE );
	in.access = access_desired;
	
	CLI_DO_RPC_WERR( cli, mem_ctx, PI_SVCCTL, SVCCTL_OPEN_SERVICE_W, 
	            in, out, 
	            qbuf, rbuf,
	            svcctl_io_q_open_service,
	            svcctl_io_r_open_service, 
	            WERR_GENERAL_FAILURE );
	
	if ( !W_ERROR_IS_OK( out.status ) )
		return out.status;

	memcpy( hService, &out.handle, sizeof(POLICY_HND) );
	
	return out.status;
}

/********************************************************************
********************************************************************/

WERROR rpccli_svcctl_close_service(struct rpc_pipe_client *cli, TALLOC_CTX *mem_ctx, POLICY_HND *hService )
{
	SVCCTL_Q_CLOSE_SERVICE in;
	SVCCTL_R_CLOSE_SERVICE out;
	prs_struct qbuf, rbuf;
	
	ZERO_STRUCT(in);
	ZERO_STRUCT(out);
	
	memcpy( &in.handle, hService, sizeof(POLICY_HND) );
	
	CLI_DO_RPC_WERR( cli, mem_ctx, PI_SVCCTL, SVCCTL_CLOSE_SERVICE, 
	            in, out, 
	            qbuf, rbuf,
	            svcctl_io_q_close_service,
	            svcctl_io_r_close_service, 
	            WERR_GENERAL_FAILURE );

	return out.status;
}

/*******************************************************************
*******************************************************************/

WERROR rpccli_svcctl_enumerate_services( struct rpc_pipe_client *cli, TALLOC_CTX *mem_ctx,
                                      POLICY_HND *hSCM, uint32 type, uint32 state, 
				      uint32 *returned, ENUM_SERVICES_STATUS **service_array  )
{
	SVCCTL_Q_ENUM_SERVICES_STATUS in;
	SVCCTL_R_ENUM_SERVICES_STATUS out;
	prs_struct qbuf, rbuf;
	uint32 resume = 0;
	ENUM_SERVICES_STATUS *services;
	int i;

	ZERO_STRUCT(in);
	ZERO_STRUCT(out);
	
	/* setup the request */
	
	memcpy( &in.handle, hSCM, sizeof(POLICY_HND) );
	
	in.type        = type;
	in.state       = state;
	in.resume      = &resume;
	
	/* first time is to get the buffer size */
	in.buffer_size = 0;

	CLI_DO_RPC_WERR( cli, mem_ctx, PI_SVCCTL, SVCCTL_ENUM_SERVICES_STATUS_W, 
	            in, out, 
	            qbuf, rbuf,
	            svcctl_io_q_enum_services_status,
	            svcctl_io_r_enum_services_status, 
	            WERR_GENERAL_FAILURE );

	/* second time with correct buffer size...should be ok */
	
	if ( W_ERROR_EQUAL( out.status, WERR_MORE_DATA ) ) {
		in.buffer_size = out.needed;

		CLI_DO_RPC_WERR( cli, mem_ctx, PI_SVCCTL, SVCCTL_ENUM_SERVICES_STATUS_W, 
		            in, out, 
		            qbuf, rbuf,
		            svcctl_io_q_enum_services_status,
		            svcctl_io_r_enum_services_status, 
		            WERR_GENERAL_FAILURE );
	}
	
	if ( !W_ERROR_IS_OK(out.status) ) 
		return out.status;
		
	/* pull out the data */
	if ( !(services = TALLOC_ARRAY( mem_ctx, ENUM_SERVICES_STATUS, out.returned )) ) 
		return WERR_NOMEM;
		
	for ( i=0; i<out.returned; i++ ) {
		svcctl_io_enum_services_status( "", &services[i], &out.buffer, 0 );
	}
	
	*service_array = services;
	*returned      = out.returned;
	
	return out.status;
}

/*******************************************************************
*******************************************************************/

WERROR rpccli_svcctl_query_status( struct rpc_pipe_client *cli, TALLOC_CTX *mem_ctx,
                                POLICY_HND *hService, SERVICE_STATUS *status )
{
	SVCCTL_Q_QUERY_STATUS in;
	SVCCTL_R_QUERY_STATUS out;
	prs_struct qbuf, rbuf;
	
	ZERO_STRUCT(in);
	ZERO_STRUCT(out);
	
	memcpy( &in.handle, hService, sizeof(POLICY_HND) );
	
	CLI_DO_RPC_WERR( cli, mem_ctx, PI_SVCCTL, SVCCTL_QUERY_STATUS, 
	            in, out, 
	            qbuf, rbuf,
	            svcctl_io_q_query_status,
	            svcctl_io_r_query_status, 
	            WERR_GENERAL_FAILURE );
	
	if ( !W_ERROR_IS_OK( out.status ) )
		return out.status;

	memcpy( status, &out.svc_status, sizeof(SERVICE_STATUS) );
	
	return out.status;
}

/*******************************************************************
*******************************************************************/

WERROR rpccli_svcctl_query_config(struct rpc_pipe_client *cli, TALLOC_CTX *mem_ctx,
                                POLICY_HND *hService, SERVICE_CONFIG *config )
{
	SVCCTL_Q_QUERY_SERVICE_CONFIG in;
	SVCCTL_R_QUERY_SERVICE_CONFIG out;
	prs_struct qbuf, rbuf;
	
	ZERO_STRUCT(in);
	ZERO_STRUCT(out);
	
	memcpy( &in.handle, hService, sizeof(POLICY_HND) );
	in.buffer_size = 0;
	
	
	CLI_DO_RPC_WERR( cli, mem_ctx, PI_SVCCTL, SVCCTL_QUERY_SERVICE_CONFIG_W, 
	            in, out, 
	            qbuf, rbuf,
	            svcctl_io_q_query_service_config,
	            svcctl_io_r_query_service_config, 
	            WERR_GENERAL_FAILURE );
	
	if ( W_ERROR_EQUAL( out.status, WERR_INSUFFICIENT_BUFFER ) ) {
		in.buffer_size = out.needed;

		CLI_DO_RPC_WERR( cli, mem_ctx, PI_SVCCTL, SVCCTL_QUERY_SERVICE_CONFIG_W,
		            in, out, 
		            qbuf, rbuf,
		            svcctl_io_q_query_service_config,
		            svcctl_io_r_query_service_config, 
		            WERR_GENERAL_FAILURE );
	}
	
	if ( !W_ERROR_IS_OK( out.status ) )
		return out.status;

	memcpy( config, &out.config, sizeof(SERVICE_CONFIG) );
	
	config->executablepath = TALLOC_ZERO_P( mem_ctx, UNISTR2 );
	config->loadordergroup = TALLOC_ZERO_P( mem_ctx, UNISTR2 );
	config->dependencies   = TALLOC_ZERO_P( mem_ctx, UNISTR2 );
	config->startname      = TALLOC_ZERO_P( mem_ctx, UNISTR2 );
	config->displayname    = TALLOC_ZERO_P( mem_ctx, UNISTR2 );
	
	if ( out.config.executablepath ) {
		config->executablepath = TALLOC_ZERO_P( mem_ctx, UNISTR2 );
		copy_unistr2( config->executablepath, out.config.executablepath );
	}

	if ( out.config.loadordergroup ) {
		config->loadordergroup = TALLOC_ZERO_P( mem_ctx, UNISTR2 );
		copy_unistr2( config->loadordergroup, out.config.loadordergroup );
	}

	if ( out.config.dependencies ) {
		config->dependencies = TALLOC_ZERO_P( mem_ctx, UNISTR2 );
		copy_unistr2( config->dependencies, out.config.dependencies );
	}

	if ( out.config.startname ) {
		config->startname = TALLOC_ZERO_P( mem_ctx, UNISTR2 );
		copy_unistr2( config->startname, out.config.startname );
	}

	if ( out.config.displayname ) {
		config->displayname = TALLOC_ZERO_P( mem_ctx, UNISTR2 );
		copy_unistr2( config->displayname, out.config.displayname );
	}
	
	return out.status;
}

/*******************************************************************
*******************************************************************/

WERROR rpccli_svcctl_start_service( struct rpc_pipe_client *cli, TALLOC_CTX *mem_ctx,
                                 POLICY_HND *hService,
                                 const char **parm_array, uint32 parmcount )
{
	SVCCTL_Q_START_SERVICE in;
	SVCCTL_R_START_SERVICE out;
	prs_struct qbuf, rbuf;
	
	ZERO_STRUCT(in);
	ZERO_STRUCT(out);
	
	memcpy( &in.handle, hService, sizeof(POLICY_HND) );
	
	in.parmcount  = 0;
	in.parameters = NULL;
	
	CLI_DO_RPC_WERR( cli, mem_ctx, PI_SVCCTL, SVCCTL_START_SERVICE_W,
	            in, out, 
	            qbuf, rbuf,
	            svcctl_io_q_start_service,
	            svcctl_io_r_start_service,
	            WERR_GENERAL_FAILURE );
	
	return out.status;
}

/*******************************************************************
*******************************************************************/

WERROR rpccli_svcctl_control_service( struct rpc_pipe_client *cli, TALLOC_CTX *mem_ctx,
                                   POLICY_HND *hService, uint32 control,
				   SERVICE_STATUS *status )
{
	SVCCTL_Q_CONTROL_SERVICE in;
	SVCCTL_R_CONTROL_SERVICE out;
	prs_struct qbuf, rbuf;
	
	ZERO_STRUCT(in);
	ZERO_STRUCT(out);
	
	memcpy( &in.handle, hService, sizeof(POLICY_HND) );
	in.control = control;
	
	CLI_DO_RPC_WERR( cli, mem_ctx, PI_SVCCTL, SVCCTL_CONTROL_SERVICE, 
	            in, out, 
	            qbuf, rbuf,
	            svcctl_io_q_control_service,
	            svcctl_io_r_control_service,
	            WERR_GENERAL_FAILURE );
	
	if ( !W_ERROR_IS_OK( out.status ) )
		return out.status;

	memcpy( status, &out.svc_status, sizeof(SERVICE_STATUS) );
	
	return out.status;
}


/*******************************************************************
*******************************************************************/

WERROR rpccli_svcctl_get_dispname( struct rpc_pipe_client *cli, TALLOC_CTX *mem_ctx, 
                                POLICY_HND *hService, fstring displayname )
{
	SVCCTL_Q_GET_DISPLAY_NAME in;
	SVCCTL_R_GET_DISPLAY_NAME out;
	prs_struct qbuf, rbuf;
	
	ZERO_STRUCT(in);
	ZERO_STRUCT(out);
	
	memcpy( &in.handle, hService, sizeof(POLICY_HND) );
	in.display_name_len = 0;
	
	CLI_DO_RPC_WERR( cli, mem_ctx, PI_SVCCTL, SVCCTL_GET_DISPLAY_NAME, 
	            in, out, 
	            qbuf, rbuf,
	            svcctl_io_q_get_display_name,
	            svcctl_io_r_get_display_name, 
	            WERR_GENERAL_FAILURE );
	
	/* second time with correct buffer size...should be ok */
	
	if ( W_ERROR_EQUAL( out.status, WERR_INSUFFICIENT_BUFFER ) ) {
		in.display_name_len = out.display_name_len;

		CLI_DO_RPC_WERR( cli, mem_ctx, PI_SVCCTL, SVCCTL_GET_DISPLAY_NAME, 
		            in, out, 
		            qbuf, rbuf,
		            svcctl_io_q_get_display_name,
		            svcctl_io_r_get_display_name, 
		            WERR_GENERAL_FAILURE );
	}

	if ( !W_ERROR_IS_OK( out.status ) )
		return out.status;

	rpcstr_pull( displayname, out.displayname.buffer, sizeof(displayname), -1, STR_TERMINATE );
	
	return out.status;
}
