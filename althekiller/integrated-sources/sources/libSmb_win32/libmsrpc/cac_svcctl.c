/* 
 *  Unix SMB/CIFS implementation.
 *  MS-RPC client library implementation (SVCCTL pipe)
 *  Copyright (C) Chris Nicholls              2005.
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

#include "libmsrpc.h"
#include "libsmb_internal.h"

#define WAIT_SLEEP_TIME 300000

int cac_SvcOpenScm(CacServerHandle *hnd, TALLOC_CTX *mem_ctx, struct SvcOpenScm *op) {
   SMBCSRV *srv        = NULL;
   struct rpc_pipe_client *pipe_hnd = NULL;
   WERROR err;

   POLICY_HND *scm_out = NULL;

   if(!hnd) 
      return CAC_FAILURE;

   if(!hnd->_internal.ctx) {
      hnd->status = NT_STATUS_INVALID_HANDLE;
      return CAC_FAILURE;
   }

   if(!op || op->in.access == 0 || !mem_ctx) {
      hnd->status = NT_STATUS_INVALID_PARAMETER;
      return CAC_FAILURE;
   }
   
   srv = cac_GetServer(hnd);
   if(!srv) {
      hnd->status = NT_STATUS_INVALID_CONNECTION;
      return CAC_FAILURE;
   }

   /*initialize for samr pipe if we have to*/
   if(!hnd->_internal.pipes[PI_SVCCTL]) {
      if(!(pipe_hnd = cli_rpc_pipe_open_noauth(&srv->cli, PI_SVCCTL, &(hnd->status)))) {
         hnd->status = NT_STATUS_UNSUCCESSFUL;
         return CAC_FAILURE;
      }

      hnd->_internal.pipes[PI_SVCCTL] = True;
   }

   scm_out = talloc(mem_ctx, POLICY_HND);
   if(!scm_out) {
      hnd->status = NT_STATUS_NO_MEMORY;
      return CAC_FAILURE;
   }

   err = rpccli_svcctl_open_scm( pipe_hnd, mem_ctx, scm_out, op->in.access);
   hnd->status = werror_to_ntstatus(err);

   if(!NT_STATUS_IS_OK(hnd->status))
      return CAC_FAILURE;

   op->out.scm_hnd = scm_out;

   return CAC_SUCCESS;
}

int cac_SvcClose(CacServerHandle *hnd, TALLOC_CTX *mem_ctx, POLICY_HND *scm_hnd) {
   struct rpc_pipe_client *pipe_hnd        = NULL;
   WERROR err;

   if(!hnd) 
      return CAC_FAILURE;

   if(!hnd->_internal.ctx) {
      hnd->status = NT_STATUS_INVALID_HANDLE;
      return CAC_FAILURE;
   }

   if(!scm_hnd || !mem_ctx) {
      hnd->status = NT_STATUS_INVALID_PARAMETER;
      return CAC_FAILURE;
   }

   pipe_hnd = cac_GetPipe(hnd, PI_SVCCTL);
   if(!pipe_hnd) {
      hnd->status = NT_STATUS_INVALID_HANDLE;
      return CAC_FAILURE;
   }

   err = rpccli_svcctl_close_service( pipe_hnd, mem_ctx, scm_hnd);
   hnd->status = werror_to_ntstatus(err);

   if(!NT_STATUS_IS_OK(hnd->status))
      return CAC_FAILURE;

   return CAC_SUCCESS;
}

int cac_SvcEnumServices(CacServerHandle *hnd, TALLOC_CTX *mem_ctx, struct SvcEnumServices *op) {
   struct rpc_pipe_client *pipe_hnd        = NULL;
   WERROR err;

   uint32 type_buf  = 0;
   uint32 state_buf = 0;

   uint32 num_svc_out = 0;

   ENUM_SERVICES_STATUS *svc_buf = NULL;

   if(!hnd) 
      return CAC_FAILURE;

   if(!hnd->_internal.ctx) {
      hnd->status = NT_STATUS_INVALID_HANDLE;
      return CAC_FAILURE;
   }

   if(!op || !op->in.scm_hnd || !mem_ctx) {
      hnd->status = NT_STATUS_INVALID_PARAMETER;
      return CAC_FAILURE;
   }

   pipe_hnd = cac_GetPipe(hnd, PI_SVCCTL);
   if(!pipe_hnd) {
      hnd->status = NT_STATUS_INVALID_HANDLE;
      return CAC_FAILURE;
   }

   type_buf = (op->in.type != 0) ? op->in.type : (SVCCTL_TYPE_DRIVER | SVCCTL_TYPE_WIN32);
   state_buf = (op->in.state != 0) ? op->in.state : SVCCTL_STATE_ALL;

   err = rpccli_svcctl_enumerate_services( pipe_hnd, mem_ctx, op->in.scm_hnd, type_buf, state_buf, &num_svc_out, &svc_buf);
   hnd->status = werror_to_ntstatus(err);

   if(!NT_STATUS_IS_OK(hnd->status))
      return CAC_FAILURE;

   op->out.services = cac_MakeServiceArray(mem_ctx, svc_buf, num_svc_out);

   if(!op->out.services) {
      hnd->status = NT_STATUS_NO_MEMORY;
      return CAC_FAILURE;
   }

   TALLOC_FREE(svc_buf);

   op->out.num_services = num_svc_out;

   return CAC_SUCCESS;
}

int cac_SvcOpenService(CacServerHandle *hnd, TALLOC_CTX *mem_ctx, struct SvcOpenService *op) {
   struct rpc_pipe_client *pipe_hnd        = NULL;
   WERROR err;

   POLICY_HND *svc_hnd_out = NULL;

   if(!hnd) 
      return CAC_FAILURE;

   if(!hnd->_internal.ctx) {
      hnd->status = NT_STATUS_INVALID_HANDLE;
      return CAC_FAILURE;
   }

   if(!op || !op->in.scm_hnd || !op->in.name || !mem_ctx) {
      hnd->status = NT_STATUS_INVALID_PARAMETER;
      return CAC_FAILURE;
   }

   pipe_hnd = cac_GetPipe(hnd, PI_SVCCTL);
   if(!pipe_hnd) {
      hnd->status = NT_STATUS_INVALID_HANDLE;
      return CAC_FAILURE;
   }

   svc_hnd_out = talloc(mem_ctx, POLICY_HND);
   if(!svc_hnd_out) {
      hnd->status = NT_STATUS_NO_MEMORY;
      return CAC_FAILURE;
   }

   err = rpccli_svcctl_open_service( pipe_hnd, mem_ctx, op->in.scm_hnd, svc_hnd_out, op->in.name, op->in.access);
   hnd->status = werror_to_ntstatus(err);

   if(!NT_STATUS_IS_OK(hnd->status))
      return CAC_FAILURE;

   op->out.svc_hnd = svc_hnd_out;

   return CAC_SUCCESS;
}

int cac_SvcControlService(CacServerHandle *hnd, TALLOC_CTX *mem_ctx, struct SvcControlService *op) {
   struct rpc_pipe_client *pipe_hnd        = NULL;
   WERROR err;

   SERVICE_STATUS status_out;

   if(!hnd) 
      return CAC_FAILURE;

   if(!hnd->_internal.ctx) {
      hnd->status = NT_STATUS_INVALID_HANDLE;
      return CAC_FAILURE;
   }

   if(!op || !op->in.svc_hnd || !mem_ctx) {
      hnd->status = NT_STATUS_INVALID_PARAMETER;
      return CAC_FAILURE;
   }

   if(op->in.control < SVCCTL_CONTROL_STOP || op->in.control > SVCCTL_CONTROL_SHUTDOWN) {
      hnd->status = NT_STATUS_INVALID_PARAMETER;
      return CAC_FAILURE;
   }

   pipe_hnd = cac_GetPipe(hnd, PI_SVCCTL);
   if(!pipe_hnd) {
      hnd->status = NT_STATUS_INVALID_HANDLE;
      return CAC_FAILURE;
   }

   err = rpccli_svcctl_control_service( pipe_hnd, mem_ctx, op->in.svc_hnd, op->in.control, &status_out);
   hnd->status = werror_to_ntstatus(err);

   if(!NT_STATUS_IS_OK(hnd->status))
      return CAC_FAILURE;

   return CAC_SUCCESS;
}

int cac_SvcGetStatus(CacServerHandle *hnd, TALLOC_CTX *mem_ctx, struct SvcGetStatus *op) {
   struct rpc_pipe_client *pipe_hnd        = NULL;
   WERROR err;

   SERVICE_STATUS status_out;

   if(!hnd) 
      return CAC_FAILURE;

   if(!hnd->_internal.ctx) {
      hnd->status = NT_STATUS_INVALID_HANDLE;
      return CAC_FAILURE;
   }

   if(!op || !op->in.svc_hnd || !mem_ctx) {
      hnd->status = NT_STATUS_INVALID_PARAMETER;
      return CAC_FAILURE;
   }

   pipe_hnd = cac_GetPipe(hnd, PI_SVCCTL);
   if(!pipe_hnd) {
      hnd->status = NT_STATUS_INVALID_HANDLE;
      return CAC_FAILURE;
   }

   err = rpccli_svcctl_query_status( pipe_hnd, mem_ctx, op->in.svc_hnd, &status_out);
   hnd->status = werror_to_ntstatus(err);

   if(!NT_STATUS_IS_OK(hnd->status))
      return CAC_FAILURE;

   op->out.status = status_out;

   return CAC_SUCCESS;
}



/*Internal function - similar to code found in utils/net_rpc_service.c
 * Waits for a service to reach a specific state.
 * svc_hnd - Handle to the service
 * state   - the state we are waiting for
 * timeout - number of seconds to wait
 * returns CAC_FAILURE if the state is never reached
 *      or CAC_SUCCESS if the state is reached
 */
int cac_WaitForService(CacServerHandle *hnd, TALLOC_CTX *mem_ctx, POLICY_HND *svc_hnd, uint32 state, uint32 timeout, SERVICE_STATUS *status) {
   struct rpc_pipe_client *pipe_hnd = NULL;
   /*number of milliseconds we have spent*/
   uint32 time_spent = 0;
   WERROR err;

   if(!hnd || !mem_ctx || !svc_hnd || !status)
      return CAC_FAILURE;

   pipe_hnd = cac_GetPipe(hnd, PI_SVCCTL);
   if(!pipe_hnd) {
      hnd->status = NT_STATUS_INVALID_HANDLE;
      return CAC_FAILURE;
   }

   while(status->state != state && time_spent < (timeout * 1000000) && NT_STATUS_IS_OK(hnd->status)) {
      /*if this is the first call, then we _just_ got the status.. sleep now*/
      usleep(WAIT_SLEEP_TIME);
      time_spent += WAIT_SLEEP_TIME;

      err = rpccli_svcctl_query_status(pipe_hnd, mem_ctx, svc_hnd, status);
      hnd->status = werror_to_ntstatus(err);
   }

   if(status->state == state) 
      return CAC_SUCCESS;

   return CAC_FAILURE;
}

int cac_SvcStartService(CacServerHandle *hnd, TALLOC_CTX *mem_ctx, struct SvcStartService *op) {
   struct rpc_pipe_client *pipe_hnd        = NULL;
   WERROR err;

   SERVICE_STATUS status_buf;

   if(!hnd) 
      return CAC_FAILURE;

   if(!hnd->_internal.ctx) {
      hnd->status = NT_STATUS_INVALID_HANDLE;
      return CAC_FAILURE;
   }

   if(!op || !op->in.svc_hnd || !mem_ctx) {
      hnd->status = NT_STATUS_INVALID_PARAMETER;
      return CAC_FAILURE;
   }

   if(op->in.num_parms != 0 && op->in.parms == NULL) {
      hnd->status = NT_STATUS_INVALID_PARAMETER;
      return CAC_FAILURE;
   }

   pipe_hnd = cac_GetPipe(hnd, PI_SVCCTL);
   if(!pipe_hnd) {
      hnd->status = NT_STATUS_INVALID_HANDLE;
      return CAC_FAILURE;
   }

   err = rpccli_svcctl_start_service(pipe_hnd, mem_ctx, op->in.svc_hnd, (const char **)op->in.parms, op->in.num_parms);
   hnd->status = werror_to_ntstatus(err);

   if(!NT_STATUS_IS_OK(hnd->status))
      return CAC_FAILURE;

   if(op->in.timeout == 0)
      return CAC_SUCCESS;

   return cac_WaitForService(hnd, mem_ctx, op->in.svc_hnd, SVCCTL_RUNNING, op->in.timeout, &status_buf);
}

int cac_SvcStopService(CacServerHandle *hnd, TALLOC_CTX *mem_ctx, struct SvcStopService *op) {
   struct rpc_pipe_client *pipe_hnd        = NULL;
   WERROR err;

   SERVICE_STATUS status_out;

   if(!hnd) 
      return CAC_FAILURE;

   if(!hnd->_internal.ctx) {
      hnd->status = NT_STATUS_INVALID_HANDLE;
      return CAC_FAILURE;
   }

   if(!op || !op->in.svc_hnd || !mem_ctx) {
      hnd->status = NT_STATUS_INVALID_PARAMETER;
      return CAC_FAILURE;
   }

   pipe_hnd = cac_GetPipe(hnd, PI_SVCCTL);
   if(!pipe_hnd) {
      hnd->status = NT_STATUS_INVALID_HANDLE;
      return CAC_FAILURE;
   }

   err = rpccli_svcctl_control_service( pipe_hnd, mem_ctx, op->in.svc_hnd, SVCCTL_CONTROL_STOP, &status_out);
   hnd->status = werror_to_ntstatus(err);

   if(!NT_STATUS_IS_OK(hnd->status))
      return CAC_FAILURE;

   op->out.status = status_out;

   if(op->in.timeout == 0)
      return CAC_SUCCESS;

   return cac_WaitForService(hnd, mem_ctx, op->in.svc_hnd, SVCCTL_STOPPED, op->in.timeout, &op->out.status);
}

int cac_SvcPauseService(CacServerHandle *hnd, TALLOC_CTX *mem_ctx, struct SvcPauseService *op) {
   struct rpc_pipe_client *pipe_hnd        = NULL;
   WERROR err;

   SERVICE_STATUS status_out;

   if(!hnd) 
      return CAC_FAILURE;

   if(!hnd->_internal.ctx) {
      hnd->status = NT_STATUS_INVALID_HANDLE;
      return CAC_FAILURE;
   }

   if(!op || !op->in.svc_hnd || !mem_ctx) {
      hnd->status = NT_STATUS_INVALID_PARAMETER;
      return CAC_FAILURE;
   }

   pipe_hnd = cac_GetPipe(hnd, PI_SVCCTL);
   if(!pipe_hnd) {
      hnd->status = NT_STATUS_INVALID_HANDLE;
      return CAC_FAILURE;
   }

   err = rpccli_svcctl_control_service( pipe_hnd, mem_ctx, op->in.svc_hnd, SVCCTL_CONTROL_PAUSE, &status_out);
   hnd->status = werror_to_ntstatus(err);

   if(!NT_STATUS_IS_OK(hnd->status))
      return CAC_FAILURE;

   op->out.status = status_out;

   if(op->in.timeout == 0)
      return CAC_SUCCESS;

   return cac_WaitForService(hnd, mem_ctx, op->in.svc_hnd, SVCCTL_PAUSED, op->in.timeout, &op->out.status);
}

int cac_SvcContinueService(CacServerHandle *hnd, TALLOC_CTX *mem_ctx, struct SvcContinueService *op) {
   struct rpc_pipe_client *pipe_hnd        = NULL;
   WERROR err;

   SERVICE_STATUS status_out;

   if(!hnd) 
      return CAC_FAILURE;

   if(!hnd->_internal.ctx) {
      hnd->status = NT_STATUS_INVALID_HANDLE;
      return CAC_FAILURE;
   }

   if(!op || !op->in.svc_hnd || !mem_ctx) {
      hnd->status = NT_STATUS_INVALID_PARAMETER;
      return CAC_FAILURE;
   }

   pipe_hnd = cac_GetPipe(hnd, PI_SVCCTL);
   if(!pipe_hnd) {
      hnd->status = NT_STATUS_INVALID_HANDLE;
      return CAC_FAILURE;
   }

   err = rpccli_svcctl_control_service( pipe_hnd, mem_ctx, op->in.svc_hnd, SVCCTL_CONTROL_CONTINUE, &status_out);
   hnd->status = werror_to_ntstatus(err);

   if(!NT_STATUS_IS_OK(hnd->status))
      return CAC_FAILURE;

   op->out.status = status_out;

   if(op->in.timeout == 0)
      return CAC_SUCCESS;

   return cac_WaitForService(hnd, mem_ctx, op->in.svc_hnd, SVCCTL_RUNNING, op->in.timeout, &op->out.status);
}

int cac_SvcGetDisplayName(CacServerHandle *hnd, TALLOC_CTX *mem_ctx, struct SvcGetDisplayName *op) {
   struct rpc_pipe_client *pipe_hnd        = NULL;
   WERROR err;

   fstring disp_name_out;

   if(!hnd) 
      return CAC_FAILURE;

   if(!hnd->_internal.ctx) {
      hnd->status = NT_STATUS_INVALID_HANDLE;
      return CAC_FAILURE;
   }

   if(!op || !op->in.svc_hnd || !mem_ctx) {
      hnd->status = NT_STATUS_INVALID_PARAMETER;
      return CAC_FAILURE;
   }

   pipe_hnd = cac_GetPipe(hnd, PI_SVCCTL);
   if(!pipe_hnd) {
      hnd->status = NT_STATUS_INVALID_HANDLE;
      return CAC_FAILURE;
   }

   err = rpccli_svcctl_get_dispname( pipe_hnd, mem_ctx, op->in.svc_hnd, disp_name_out);
   hnd->status = werror_to_ntstatus(err);

   if(!NT_STATUS_IS_OK(hnd->status))
      return CAC_FAILURE;

   op->out.display_name = talloc_strdup(mem_ctx, disp_name_out);

   if(!op->out.display_name) {
      hnd->status = NT_STATUS_NO_MEMORY;
      return CAC_FAILURE;
   }

   return CAC_SUCCESS;
}


int cac_SvcGetServiceConfig(CacServerHandle *hnd, TALLOC_CTX *mem_ctx, struct SvcGetServiceConfig *op) {
   struct rpc_pipe_client *pipe_hnd        = NULL;
   WERROR err;

   SERVICE_CONFIG config_out;
   
   if(!hnd) 
      return CAC_FAILURE;

   if(!hnd->_internal.ctx) {
      hnd->status = NT_STATUS_INVALID_HANDLE;
      return CAC_FAILURE;
   }

   if(!op || !op->in.svc_hnd || !mem_ctx) {
      hnd->status = NT_STATUS_INVALID_PARAMETER;
      return CAC_FAILURE;
   }

   pipe_hnd = cac_GetPipe(hnd, PI_SVCCTL);
   if(!pipe_hnd) {
      hnd->status = NT_STATUS_INVALID_HANDLE;
      return CAC_FAILURE;
   }

   err = rpccli_svcctl_query_config( pipe_hnd, mem_ctx, op->in.svc_hnd, &config_out);
   hnd->status = werror_to_ntstatus(err);

   if(!NT_STATUS_IS_OK(hnd->status))
      return CAC_FAILURE;

   if(!cac_InitCacServiceConfig(mem_ctx, &config_out, &op->out.config)) {
      hnd->status = NT_STATUS_NO_MEMORY;
      return CAC_FAILURE;
   }

   return CAC_SUCCESS;

}
