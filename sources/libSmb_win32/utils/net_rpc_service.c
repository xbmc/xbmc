/* 
   Samba Unix/Linux SMB client library 
   Distributed SMB/CIFS Server Management Utility 
   Copyright (C) Gerald (Jerry) Carter          2005

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */
 
#include "includes.h"
#include "utils/net.h"


/********************************************************************
********************************************************************/

static WERROR query_service_state(struct rpc_pipe_client *pipe_hnd,
				TALLOC_CTX *mem_ctx, 
				POLICY_HND *hSCM,
				const char *service,
				uint32 *state )
{
	POLICY_HND hService;
	SERVICE_STATUS service_status;
	WERROR result = WERR_GENERAL_FAILURE;
	
	/* now cycle until the status is actually 'watch_state' */
	
	result = rpccli_svcctl_open_service(pipe_hnd, mem_ctx, hSCM, &hService, 
		service, SC_RIGHT_SVC_QUERY_STATUS );

	if ( !W_ERROR_IS_OK(result) ) {
		d_fprintf(stderr, "Failed to open service.  [%s]\n", dos_errstr(result));
		return result;
	}

	result = rpccli_svcctl_query_status(pipe_hnd, mem_ctx, &hService, &service_status  );
	if ( W_ERROR_IS_OK(result) ) {
		*state = service_status.state;
	}
	
	rpccli_svcctl_close_service(pipe_hnd, mem_ctx, &hService );
	
	return result;
}

/********************************************************************
********************************************************************/

static WERROR watch_service_state(struct rpc_pipe_client *pipe_hnd,
				TALLOC_CTX *mem_ctx, 
				POLICY_HND *hSCM,
				const char *service, 
				uint32 watch_state,
				uint32 *final_state )
{
	uint32 i;
	uint32 state = 0;
	WERROR result = WERR_GENERAL_FAILURE;
	
	
	i = 0;
	while ( (state != watch_state ) && i<30 ) {
		/* get the status */

		result = query_service_state(pipe_hnd, mem_ctx, hSCM, service, &state  );
		if ( !W_ERROR_IS_OK(result) ) {
			break;
		}
		
		d_printf(".");
		i++;
		usleep( 100 );
	}
	d_printf("\n");
	
	*final_state = state;
	
	return result;
}

/********************************************************************
********************************************************************/

static WERROR control_service(struct rpc_pipe_client *pipe_hnd,
				TALLOC_CTX *mem_ctx, 
				POLICY_HND *hSCM,
				const char *service, 
				uint32 control,
				uint32 watch_state )
{
	POLICY_HND hService;
	WERROR result = WERR_GENERAL_FAILURE;
	SERVICE_STATUS service_status;
	uint32 state = 0;
	
	/* Open the Service */
	
	result = rpccli_svcctl_open_service(pipe_hnd, mem_ctx, hSCM, &hService, 
		service, (SC_RIGHT_SVC_STOP|SC_RIGHT_SVC_PAUSE_CONTINUE) );

	if ( !W_ERROR_IS_OK(result) ) {
		d_fprintf(stderr, "Failed to open service.  [%s]\n", dos_errstr(result));
		goto done;
	}
	
	/* get the status */

	result = rpccli_svcctl_control_service(pipe_hnd, mem_ctx, &hService, 
		control, &service_status  );
		
	if ( !W_ERROR_IS_OK(result) ) {
		d_fprintf(stderr, "Control service request failed.  [%s]\n", dos_errstr(result));
		goto done;
	}
	
	/* loop -- checking the state until we are where we want to be */
	
	result = watch_service_state(pipe_hnd, mem_ctx, hSCM, service, watch_state, &state );
		
	d_printf("%s service is %s.\n", service, svc_status_string(state));

done:	
	rpccli_svcctl_close_service(pipe_hnd, mem_ctx, &hService  );
		
	return result;
}	

/********************************************************************
********************************************************************/

static NTSTATUS rpc_service_list_internal(const DOM_SID *domain_sid,
					const char *domain_name, 
					struct cli_state *cli,
					struct rpc_pipe_client *pipe_hnd,
					TALLOC_CTX *mem_ctx, 
					int argc,
					const char **argv )
{
	POLICY_HND hSCM;
	ENUM_SERVICES_STATUS *services;
	WERROR result = WERR_GENERAL_FAILURE;
	fstring servicename;
	fstring displayname;
	uint32 num_services = 0;
	int i;
	
	if (argc != 0 ) {
		d_printf("Usage: net rpc service list\n");
		return NT_STATUS_OK;
	}

	result = rpccli_svcctl_open_scm(pipe_hnd, mem_ctx, &hSCM, SC_RIGHT_MGR_ENUMERATE_SERVICE  );
	if ( !W_ERROR_IS_OK(result) ) {
		d_fprintf(stderr, "Failed to open Service Control Manager.  [%s]\n", dos_errstr(result));
		return werror_to_ntstatus(result);
	}
	
	result = rpccli_svcctl_enumerate_services(pipe_hnd, mem_ctx, &hSCM, SVCCTL_TYPE_WIN32,
		SVCCTL_STATE_ALL, &num_services, &services );
	
	if ( !W_ERROR_IS_OK(result) ) {
		d_fprintf(stderr, "Failed to enumerate services.  [%s]\n", dos_errstr(result));
		goto done;
	}
	
	if ( num_services == 0 )
		d_printf("No services returned\n");
	
	for ( i=0; i<num_services; i++ ) {
		rpcstr_pull( servicename, services[i].servicename.buffer, sizeof(servicename), -1, STR_TERMINATE );
		rpcstr_pull( displayname, services[i].displayname.buffer, sizeof(displayname), -1, STR_TERMINATE );
		
		d_printf("%-20s    \"%s\"\n", servicename, displayname);
	}

done:	
	rpccli_svcctl_close_service(pipe_hnd, mem_ctx, &hSCM  );
		
	return werror_to_ntstatus(result);
}	

/********************************************************************
********************************************************************/

static NTSTATUS rpc_service_status_internal(const DOM_SID *domain_sid,
						const char *domain_name, 
						struct cli_state *cli,
						struct rpc_pipe_client *pipe_hnd,
						TALLOC_CTX *mem_ctx, 
						int argc,
						const char **argv )
{
	POLICY_HND hSCM, hService;
	WERROR result = WERR_GENERAL_FAILURE;
	fstring servicename;
	SERVICE_STATUS service_status;
	SERVICE_CONFIG config;
	fstring ascii_string;
	
	if (argc != 1 ) {
		d_printf("Usage: net rpc service status <service>\n");
		return NT_STATUS_OK;
	}

	fstrcpy( servicename, argv[0] );

	/* Open the Service Control Manager */
	
	result = rpccli_svcctl_open_scm(pipe_hnd, mem_ctx, &hSCM, SC_RIGHT_MGR_ENUMERATE_SERVICE  );
	if ( !W_ERROR_IS_OK(result) ) {
		d_fprintf(stderr, "Failed to open Service Control Manager.  [%s]\n", dos_errstr(result));
		return werror_to_ntstatus(result);
	}
	
	/* Open the Service */
	
	result = rpccli_svcctl_open_service(pipe_hnd, mem_ctx, &hSCM, &hService, servicename, 
		(SC_RIGHT_SVC_QUERY_STATUS|SC_RIGHT_SVC_QUERY_CONFIG) );

	if ( !W_ERROR_IS_OK(result) ) {
		d_fprintf(stderr, "Failed to open service.  [%s]\n", dos_errstr(result));
		goto done;
	}
	
	/* get the status */

	result = rpccli_svcctl_query_status(pipe_hnd, mem_ctx, &hService, &service_status  );
	if ( !W_ERROR_IS_OK(result) ) {
		d_fprintf(stderr, "Query status request failed.  [%s]\n", dos_errstr(result));
		goto done;
	}
	
	d_printf("%s service is %s.\n", servicename, svc_status_string(service_status.state));

	/* get the config */

	result = rpccli_svcctl_query_config(pipe_hnd, mem_ctx, &hService, &config  );
	if ( !W_ERROR_IS_OK(result) ) {
		d_fprintf(stderr, "Query config request failed.  [%s]\n", dos_errstr(result));
		goto done;
	}

	/* print out the configuration information for the service */

	d_printf("Configuration details:\n");
	d_printf("\tControls Accepted    = 0x%x\n", service_status.controls_accepted);
	d_printf("\tService Type         = 0x%x\n", config.service_type);
	d_printf("\tStart Type           = 0x%x\n", config.start_type);
	d_printf("\tError Control        = 0x%x\n", config.error_control);
	d_printf("\tTag ID               = 0x%x\n", config.tag_id);

	if ( config.executablepath ) {
		rpcstr_pull( ascii_string, config.executablepath->buffer, sizeof(ascii_string), -1, STR_TERMINATE );
		d_printf("\tExecutable Path      = %s\n", ascii_string);
	}

	if ( config.loadordergroup ) {
		rpcstr_pull( ascii_string, config.loadordergroup->buffer, sizeof(ascii_string), -1, STR_TERMINATE );
		d_printf("\tLoad Order Group     = %s\n", ascii_string);
	}

	if ( config.dependencies ) {
		rpcstr_pull( ascii_string, config.dependencies->buffer, sizeof(ascii_string), -1, STR_TERMINATE );
		d_printf("\tDependencies         = %s\n", ascii_string);
	}

	if ( config.startname ) {
		rpcstr_pull( ascii_string, config.startname->buffer, sizeof(ascii_string), -1, STR_TERMINATE );
		d_printf("\tStart Name           = %s\n", ascii_string);
	}

	if ( config.displayname ) {
		rpcstr_pull( ascii_string, config.displayname->buffer, sizeof(ascii_string), -1, STR_TERMINATE );
		d_printf("\tDisplay Name         = %s\n", ascii_string);
	}

done:	
	rpccli_svcctl_close_service(pipe_hnd, mem_ctx, &hService  );
	rpccli_svcctl_close_service(pipe_hnd, mem_ctx, &hSCM  );

	return werror_to_ntstatus(result);
}	

/********************************************************************
********************************************************************/

static NTSTATUS rpc_service_stop_internal(const DOM_SID *domain_sid,
					const char *domain_name, 
					struct cli_state *cli,
					struct rpc_pipe_client *pipe_hnd,
					TALLOC_CTX *mem_ctx, 
					int argc,
					const char **argv )
{
	POLICY_HND hSCM;
	WERROR result = WERR_GENERAL_FAILURE;
	fstring servicename;
	
	if (argc != 1 ) {
		d_printf("Usage: net rpc service status <service>\n");
		return NT_STATUS_OK;
	}

	fstrcpy( servicename, argv[0] );

	/* Open the Service Control Manager */
	
	result = rpccli_svcctl_open_scm(pipe_hnd, mem_ctx, &hSCM, SC_RIGHT_MGR_ENUMERATE_SERVICE  );
	if ( !W_ERROR_IS_OK(result) ) {
		d_fprintf(stderr, "Failed to open Service Control Manager.  [%s]\n", dos_errstr(result));
		return werror_to_ntstatus(result);
	}
	
	result = control_service(pipe_hnd, mem_ctx, &hSCM, servicename, 
		SVCCTL_CONTROL_STOP, SVCCTL_STOPPED );
		
	rpccli_svcctl_close_service(pipe_hnd, mem_ctx, &hSCM  );
		
	return werror_to_ntstatus(result);
}	

/********************************************************************
********************************************************************/

static NTSTATUS rpc_service_pause_internal(const DOM_SID *domain_sid,
					const char *domain_name, 
					struct cli_state *cli,
					struct rpc_pipe_client *pipe_hnd,
					TALLOC_CTX *mem_ctx, 
					int argc,
					const char **argv )
{
	POLICY_HND hSCM;
	WERROR result = WERR_GENERAL_FAILURE;
	fstring servicename;
	
	if (argc != 1 ) {
		d_printf("Usage: net rpc service status <service>\n");
		return NT_STATUS_OK;
	}

	fstrcpy( servicename, argv[0] );

	/* Open the Service Control Manager */
	
	result = rpccli_svcctl_open_scm(pipe_hnd, mem_ctx, &hSCM, SC_RIGHT_MGR_ENUMERATE_SERVICE  );
	if ( !W_ERROR_IS_OK(result) ) {
		d_fprintf(stderr, "Failed to open Service Control Manager.  [%s]\n", dos_errstr(result));
		return werror_to_ntstatus(result);
	}
	
	result = control_service(pipe_hnd, mem_ctx, &hSCM, servicename, 
		SVCCTL_CONTROL_PAUSE, SVCCTL_PAUSED );
		
	rpccli_svcctl_close_service(pipe_hnd, mem_ctx, &hSCM  );
		
	return werror_to_ntstatus(result);
}	

/********************************************************************
********************************************************************/

static NTSTATUS rpc_service_resume_internal(const DOM_SID *domain_sid,
					const char *domain_name, 
					struct cli_state *cli,
					struct rpc_pipe_client *pipe_hnd,
					TALLOC_CTX *mem_ctx, 
					int argc,
					const char **argv )
{
	POLICY_HND hSCM;
	WERROR result = WERR_GENERAL_FAILURE;
	fstring servicename;
	
	if (argc != 1 ) {
		d_printf("Usage: net rpc service status <service>\n");
		return NT_STATUS_OK;
	}

	fstrcpy( servicename, argv[0] );

	/* Open the Service Control Manager */
	
	result = rpccli_svcctl_open_scm(pipe_hnd, mem_ctx, &hSCM, SC_RIGHT_MGR_ENUMERATE_SERVICE  );
	if ( !W_ERROR_IS_OK(result) ) {
		d_fprintf(stderr, "Failed to open Service Control Manager.  [%s]\n", dos_errstr(result));
		return werror_to_ntstatus(result);
	}
	
	result = control_service(pipe_hnd, mem_ctx, &hSCM, servicename, 
		SVCCTL_CONTROL_CONTINUE, SVCCTL_RUNNING );
		
	rpccli_svcctl_close_service(pipe_hnd, mem_ctx, &hSCM  );
		
	return werror_to_ntstatus(result);
}	

/********************************************************************
********************************************************************/

static NTSTATUS rpc_service_start_internal(const DOM_SID *domain_sid,
					const char *domain_name, 
					struct cli_state *cli,
					struct rpc_pipe_client *pipe_hnd,
					TALLOC_CTX *mem_ctx, 
					int argc,
					const char **argv )
{
	POLICY_HND hSCM, hService;
	WERROR result = WERR_GENERAL_FAILURE;
	fstring servicename;
	uint32 state = 0;
	
	if (argc != 1 ) {
		d_printf("Usage: net rpc service status <service>\n");
		return NT_STATUS_OK;
	}

	fstrcpy( servicename, argv[0] );

	/* Open the Service Control Manager */
	
	result = rpccli_svcctl_open_scm( pipe_hnd, mem_ctx, &hSCM, SC_RIGHT_MGR_ENUMERATE_SERVICE  );
	if ( !W_ERROR_IS_OK(result) ) {
		d_fprintf(stderr, "Failed to open Service Control Manager.  [%s]\n", dos_errstr(result));
		return werror_to_ntstatus(result);
	}
	
	/* Open the Service */
	
	result = rpccli_svcctl_open_service(pipe_hnd, mem_ctx, &hSCM, &hService, 
		servicename, SC_RIGHT_SVC_START );

	if ( !W_ERROR_IS_OK(result) ) {
		d_fprintf(stderr, "Failed to open service.  [%s]\n", dos_errstr(result));
		goto done;
	}
	
	/* get the status */

	result = rpccli_svcctl_start_service(pipe_hnd, mem_ctx, &hService, NULL, 0 );
	if ( !W_ERROR_IS_OK(result) ) {
		d_fprintf(stderr, "Query status request failed.  [%s]\n", dos_errstr(result));
		goto done;
	}
	
	result = watch_service_state(pipe_hnd, mem_ctx, &hSCM, servicename, SVCCTL_RUNNING, &state  );
	
	if ( W_ERROR_IS_OK(result) && (state == SVCCTL_RUNNING) )
		d_printf("Successfully started service: %s\n", servicename );
	else
		d_fprintf(stderr, "Failed to start service: %s [%s]\n", servicename, dos_errstr(result) );
	
done:	
	rpccli_svcctl_close_service(pipe_hnd, mem_ctx, &hService  );
	rpccli_svcctl_close_service(pipe_hnd, mem_ctx, &hSCM  );

	return werror_to_ntstatus(result);
}

/********************************************************************
********************************************************************/

static int rpc_service_list( int argc, const char **argv )
{
	return run_rpc_command( NULL, PI_SVCCTL, 0, 
		rpc_service_list_internal, argc, argv );
}

/********************************************************************
********************************************************************/

static int rpc_service_start( int argc, const char **argv )
{
	return run_rpc_command( NULL, PI_SVCCTL, 0, 
		rpc_service_start_internal, argc, argv );
}

/********************************************************************
********************************************************************/

static int rpc_service_stop( int argc, const char **argv )
{
	return run_rpc_command( NULL, PI_SVCCTL, 0, 
		rpc_service_stop_internal, argc, argv );
}

/********************************************************************
********************************************************************/

static int rpc_service_resume( int argc, const char **argv )
{
	return run_rpc_command( NULL, PI_SVCCTL, 0, 
		rpc_service_resume_internal, argc, argv );
}

/********************************************************************
********************************************************************/

static int rpc_service_pause( int argc, const char **argv )
{
	return run_rpc_command( NULL, PI_SVCCTL, 0, 
		rpc_service_pause_internal, argc, argv );
}

/********************************************************************
********************************************************************/

static int rpc_service_status( int argc, const char **argv )
{
	return run_rpc_command( NULL, PI_SVCCTL, 0, 
		rpc_service_status_internal, argc, argv );
}

/********************************************************************
********************************************************************/

static int net_help_service( int argc, const char **argv )
{
	d_printf("net rpc service list               View configured Win32 services\n");
	d_printf("net rpc service start <service>    Start a service\n");
	d_printf("net rpc service stop <service>     Stop a service\n");
	d_printf("net rpc service pause <service>    Pause a service\n");
	d_printf("net rpc service resume <service>   Resume a paused service\n");
	d_printf("net rpc service status <service>   View the current status of a service\n");
	
	return -1;
}

/********************************************************************
********************************************************************/

int net_rpc_service(int argc, const char **argv) 
{
	struct functable func[] = {
		{"list", rpc_service_list},
		{"start", rpc_service_start},
		{"stop", rpc_service_stop},
		{"pause", rpc_service_pause},
		{"resume", rpc_service_resume},
		{"status", rpc_service_status},
		{NULL, NULL}
	};
	
	if ( argc )
		return net_run_function( argc, argv, func, net_help_service );
		
	return net_help_service( argc, argv );
}
