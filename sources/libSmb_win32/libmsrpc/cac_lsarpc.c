/* 
 *  Unix SMB/CIFS implementation.
 *  MS-RPC client library implementation (LSA pipe)
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

int cac_LsaOpenPolicy(CacServerHandle *hnd, TALLOC_CTX *mem_ctx, struct LsaOpenPolicy *op) {
   SMBCSRV *srv = NULL;
   POLICY_HND *policy = NULL;
   struct rpc_pipe_client *pipe_hnd = NULL;

   if(!hnd)
      return CAC_FAILURE;
         
   if(!hnd->_internal.ctx) {
      hnd->status = NT_STATUS_INVALID_HANDLE;
      return CAC_FAILURE;
   }

   if(!mem_ctx || !op) {
      hnd->status = NT_STATUS_INVALID_PARAMETER;
      return CAC_FAILURE;
   }

   op->out.pol = NULL;

   srv = cac_GetServer(hnd);
   if(!srv) {
      hnd->status = NT_STATUS_INVALID_CONNECTION;
      return CAC_FAILURE; 
   }

   /*see if there is already an active session on this pipe, if not then open one*/
   if(!hnd->_internal.pipes[PI_LSARPC]) {
      pipe_hnd = cli_rpc_pipe_open_noauth(&(srv->cli), PI_LSARPC, &(hnd->status));

      if(!pipe_hnd) {
         hnd->status = NT_STATUS_UNSUCCESSFUL;
         return CAC_FAILURE;
      }

      hnd->_internal.pipes[PI_LSARPC] = True;
   }

   pipe_hnd = cac_GetPipe(hnd, PI_LSARPC);
   if(!pipe_hnd) {
      hnd->status = NT_STATUS_INVALID_HANDLE;
      return CAC_FAILURE;
   }

   policy = TALLOC_P(mem_ctx, POLICY_HND);
   if(!policy) {
      errno = ENOMEM;
      hnd->status = NT_STATUS_NO_MEMORY;
      return CAC_FAILURE;
   }

   /*need to make sure that our nt status is good otherwise check might fail below*/
   hnd->status = NT_STATUS_OK;
   
   if(hnd->_internal.srv_level >= SRV_WIN_2K) { 

      /*try using open_policy2, if this fails try again in next block using open_policy, if that works then adjust hnd->_internal.srv_level*/

      /*we shouldn't need to modify the access mask to make it work here*/
      hnd->status = rpccli_lsa_open_policy2(pipe_hnd, mem_ctx, op->in.security_qos, op->in.access, policy);

   }

   if(hnd->_internal.srv_level < SRV_WIN_2K || !NT_STATUS_IS_OK(hnd->status)) {
      hnd->status = rpccli_lsa_open_policy(pipe_hnd, mem_ctx, op->in.security_qos, op->in.access, policy);

      if(hnd->_internal.srv_level > SRV_WIN_NT4 && NT_STATUS_IS_OK(hnd->status)) {
         /*change the server level to 1*/
         hnd->_internal.srv_level = SRV_WIN_NT4;
      }
      
   }

   if(!NT_STATUS_IS_OK(hnd->status)) {
      return CAC_FAILURE;
   }

   op->out.pol = policy;

   return CAC_SUCCESS;
}

int cac_LsaClosePolicy(CacServerHandle *hnd, TALLOC_CTX *mem_ctx, POLICY_HND *pol) {

   struct rpc_pipe_client *pipe_hnd = NULL;

   if(!hnd)
      return CAC_FAILURE;
   
   if(!pol)
      return CAC_SUCCESS; /*if the policy handle doesnt exist then it's already closed*/

   if(!hnd->_internal.ctx || !hnd->_internal.pipes[PI_LSARPC]) {
      hnd->status = NT_STATUS_INVALID_HANDLE;
      return CAC_FAILURE;
   }

   pipe_hnd = cac_GetPipe(hnd, PI_LSARPC);
   if(!pipe_hnd) {
      hnd->status = NT_STATUS_INVALID_HANDLE;
      return CAC_FAILURE;
   }

   hnd->status = rpccli_lsa_close(pipe_hnd, mem_ctx, pol);

   TALLOC_FREE(pol);

   if(!NT_STATUS_IS_OK(hnd->status))
      return CAC_FAILURE;

   return CAC_SUCCESS;
}

int cac_LsaGetNamesFromSids(CacServerHandle *hnd, TALLOC_CTX *mem_ctx, struct LsaGetNamesFromSids *op) {
   struct rpc_pipe_client *pipe_hnd = NULL;

   int result   = -1;

   int i;

   /*buffers for outputs*/
   char **domains   = NULL;
   char **names     = NULL;
   uint32 *types    = NULL;

   CacSidInfo *sids_out   = NULL;
   DOM_SID *unknown_out   = NULL;
   int num_unknown         = 0;

   int num_sids;

   int found_idx;
   int unknown_idx;

   if(!hnd)
      return CAC_FAILURE;
      
   if(!hnd->_internal.ctx || !hnd->_internal.pipes[PI_LSARPC]) {
      hnd->status = NT_STATUS_INVALID_HANDLE;
      return CAC_FAILURE;
   }

   if(!mem_ctx || !op || !op->in.pol || !op->in.sids) {
      hnd->status = NT_STATUS_INVALID_PARAMETER;
      return CAC_FAILURE;
   }

   num_sids = op->in.num_sids;

   pipe_hnd = cac_GetPipe(hnd, PI_LSARPC);
   if(!pipe_hnd) {
      hnd->status = NT_STATUS_INVALID_HANDLE;
      return CAC_FAILURE;
   }



   /*now actually lookup the names*/
   hnd->status = rpccli_lsa_lookup_sids(pipe_hnd, mem_ctx, op->in.pol, op->in.num_sids,
                                       op->in.sids, &domains, &names, &types);

   if(NT_STATUS_IS_OK(hnd->status)) {
      /*this is the easy part, just make the out.sids array*/
      sids_out = TALLOC_ARRAY(mem_ctx, CacSidInfo, num_sids);
      if(!sids_out) {
         errno = ENOMEM;
         hnd->status = NT_STATUS_NO_MEMORY;
         return CAC_FAILURE;
      }

      for(i = 0; i < num_sids; i++) {
         sids_out[i].sid    = op->in.sids[i];
         sids_out[i].name   = names[i];
         sids_out[i].domain = domains[i];
      }

      result = CAC_SUCCESS;
   }
   else if(NT_STATUS_V(hnd->status) == NT_STATUS_V(STATUS_SOME_UNMAPPED)) {
      /*first find out how many couldn't be looked up*/

      for(i = 0; i < num_sids; i++) {
         if(names[i] == NULL) {
            num_unknown++;
         }
      }

      if( num_unknown >= num_sids) {
         hnd->status = NT_STATUS_UNSUCCESSFUL;
         return CAC_FAILURE;
      }

      sids_out = TALLOC_ARRAY(mem_ctx, CacSidInfo, (num_sids - num_unknown));
      if(!sids_out) {
         errno = ENOMEM;
         hnd->status = NT_STATUS_NO_MEMORY;
         return CAC_FAILURE;
      }

      unknown_out = TALLOC_ARRAY(mem_ctx, DOM_SID, num_unknown);
      if(!unknown_out) {
         errno = ENOMEM;
         hnd->status = NT_STATUS_NO_MEMORY;
         return CAC_FAILURE;
      }

      found_idx = unknown_idx = 0;

      /*now we can actually do the real work*/
      for(i = 0; i < num_sids; i++) {
         if(names[i] != NULL) {
            sids_out[found_idx].sid    = op->in.sids[i];
            sids_out[found_idx].name   = names[i];
            sids_out[found_idx].domain = domains[i];

            found_idx++;
         }
         else { /*then this one didnt work out*/
            unknown_out[unknown_idx] = op->in.sids[i];

            unknown_idx++;
         }
      }

      result = CAC_PARTIAL_SUCCESS;
   }
   else { /*then it failed for some reason*/
      return CAC_FAILURE;
   }

   op->out.num_found    = num_sids - num_unknown;
   op->out.sids         = sids_out;
   op->out.unknown      = unknown_out;

   return result;
   
}

int cac_LsaGetSidsFromNames(CacServerHandle *hnd, TALLOC_CTX *mem_ctx, struct LsaGetSidsFromNames *op) {
   struct rpc_pipe_client *pipe_hnd = NULL;
   int result   = -1;

   int i;

   /*buffers for outputs*/
   DOM_SID *sids     = NULL;
   uint32 *types     = NULL;

   CacSidInfo *sids_out  = NULL;
   char **unknown_out     = NULL;
   int num_unknown        = 0;

   int num_names;

   int found_idx          = 0;
   int unknown_idx        = 0;

   if(!hnd)
      return CAC_FAILURE;

   if(!hnd->_internal.ctx || !hnd->_internal.pipes[PI_LSARPC]) {
      hnd->status = NT_STATUS_INVALID_HANDLE;
      return CAC_FAILURE;
   }

   if(!mem_ctx || !op || !op->in.pol || !op->in.names) {
      hnd->status = NT_STATUS_INVALID_PARAMETER;
      return CAC_FAILURE;
   }

   num_names = op->in.num_names;

   pipe_hnd = cac_GetPipe(hnd, PI_LSARPC);
   if(!pipe_hnd) {
      hnd->status = NT_STATUS_INVALID_HANDLE;
      return CAC_FAILURE;
   }


   /*now actually lookup the names*/
   hnd->status = rpccli_lsa_lookup_names( pipe_hnd, mem_ctx, op->in.pol, num_names,
                                          (const char **)op->in.names, NULL, &sids, &types);

   if(NT_STATUS_IS_OK(hnd->status)) {
      /*this is the easy part, just make the out.sids array*/
      sids_out = TALLOC_ARRAY(mem_ctx, CacSidInfo, num_names);
      if(!sids_out) {
         errno = ENOMEM;
         hnd->status = NT_STATUS_NO_MEMORY;
         return CAC_FAILURE;
      }

      for(i = 0; i < num_names; i++) {
         sids_out[i].sid    = sids[i];
         sids_out[i].name   = talloc_strdup(mem_ctx, op->in.names[i]);
         sids_out[i].domain = NULL;
      }
      
      result = CAC_SUCCESS;
   }
   else if(NT_STATUS_V(hnd->status) == NT_STATUS_V(STATUS_SOME_UNMAPPED)) {
      /*first find out how many couldn't be looked up*/

      for(i = 0; i < num_names; i++) {
         if(types[i] == SID_NAME_UNKNOWN) {
            num_unknown++;
         }
      }

      if( num_unknown >= num_names) {
         hnd->status = NT_STATUS_UNSUCCESSFUL;
         return CAC_FAILURE;
      }

      sids_out = TALLOC_ARRAY(mem_ctx, CacSidInfo, (num_names - num_unknown));
      if(!sids_out) {
         errno = ENOMEM;
         hnd->status = NT_STATUS_NO_MEMORY;
         return CAC_FAILURE;
      }

      unknown_out = TALLOC_ARRAY(mem_ctx, char *, num_unknown);
      if(!unknown_out) {
         errno = ENOMEM;
         hnd->status = NT_STATUS_NO_MEMORY;
         return CAC_FAILURE;
      }

      unknown_idx = found_idx = 0;

      /*now we can actually do the real work*/
      for(i = 0; i < num_names; i++) {
         if(types[i] != SID_NAME_UNKNOWN) {
            sids_out[found_idx].sid    = sids[i];
            sids_out[found_idx].name   = talloc_strdup(mem_ctx, op->in.names[i]);
            sids_out[found_idx].domain = NULL;

            found_idx++;
         }
         else { /*then this one didnt work out*/
            unknown_out[unknown_idx] = talloc_strdup(mem_ctx, op->in.names[i]);

            unknown_idx++;
         }
      }

      result = CAC_PARTIAL_SUCCESS;
   }
   else { /*then it failed for some reason*/
      return CAC_FAILURE;
   }

   op->out.num_found    = num_names - num_unknown;
   op->out.sids         = sids_out;
   op->out.unknown      = unknown_out;

   return result;
   
}

int cac_LsaFetchSid(CacServerHandle *hnd, TALLOC_CTX *mem_ctx, struct LsaFetchSid *op) {
   struct rpc_pipe_client *pipe_hnd = NULL;
   int result   = -1;

   if(!hnd)
      return CAC_FAILURE;
   
   if(!hnd->_internal.ctx || !hnd->_internal.pipes[PI_LSARPC] ) {
      hnd->status = NT_STATUS_INVALID_HANDLE;
      return CAC_FAILURE;
   }

   if(!mem_ctx || !op || !op->in.pol) {
      hnd->status = NT_STATUS_INVALID_PARAMETER;
      return CAC_FAILURE;
   }

   pipe_hnd = cac_GetPipe(hnd, PI_LSARPC);
   if(!pipe_hnd) {
      hnd->status = NT_STATUS_INVALID_HANDLE;
      return CAC_FAILURE;
   }

   op->out.local_sid  = NULL;
   op->out.domain_sid = NULL;

   if( (op->in.info_class & CAC_LOCAL_INFO) == CAC_LOCAL_INFO) {
      DOM_SID *local_sid = NULL;
      char *dom_name     = NULL;

      hnd->status = rpccli_lsa_query_info_policy( pipe_hnd, mem_ctx, op->in.pol, CAC_LOCAL_INFO, &dom_name, &local_sid);

      if(!NT_STATUS_IS_OK(hnd->status)) {
         result = CAC_FAILURE;
         goto domain;
      }

      op->out.local_sid = talloc(mem_ctx, CacSidInfo);
      if(!op->out.local_sid) {
         hnd->status = NT_STATUS_NO_MEMORY;
         result = CAC_FAILURE;
         goto domain;
      }

      op->out.local_sid->domain = dom_name;
      
      sid_copy(&op->out.local_sid->sid, local_sid);
      TALLOC_FREE(local_sid);
   }

domain:

   if( (op->in.info_class & CAC_DOMAIN_INFO) == CAC_DOMAIN_INFO) {
      DOM_SID *domain_sid;
      char *dom_name;

      hnd->status = rpccli_lsa_query_info_policy( pipe_hnd, mem_ctx, op->in.pol, CAC_DOMAIN_INFO, &dom_name, &domain_sid);
      if(!NT_STATUS_IS_OK(hnd->status)) {
         /*if we succeeded above, report partial success*/
         result = CAC_FAILURE;
         goto done;
      }
      else if(result == CAC_FAILURE) {
         /*if we failed above but succeded here then report partial success*/
         result = CAC_PARTIAL_SUCCESS;
      }

      op->out.domain_sid = talloc(mem_ctx, CacSidInfo);
      if(!op->out.domain_sid) {
         hnd->status = NT_STATUS_NO_MEMORY;
         result = CAC_FAILURE;
         goto done;
      }

      op->out.domain_sid->domain = dom_name;
      sid_copy(&op->out.domain_sid->sid, domain_sid);
      TALLOC_FREE(domain_sid);
   }
   
done:
   return result;
}

int cac_LsaQueryInfoPolicy(CacServerHandle *hnd, TALLOC_CTX *mem_ctx, struct LsaQueryInfoPolicy *op) {
   struct rpc_pipe_client *pipe_hnd = NULL;

   char *domain_name    = NULL;
   char *dns_name       = NULL;
   char *forest_name     = NULL;
   struct uuid *domain_guid    = NULL;
   DOM_SID *domain_sid  = NULL;

   if(!hnd)
      return CAC_FAILURE;

   if(!hnd->_internal.ctx || !hnd->_internal.pipes[PI_LSARPC]) {
      hnd->status = NT_STATUS_INVALID_HANDLE;
      return CAC_FAILURE;
   }

   if(!op || !op->in.pol) {
      hnd->status = NT_STATUS_INVALID_PARAMETER;
      return CAC_FAILURE;
   }

   pipe_hnd = cac_GetPipe(hnd, PI_LSARPC);
   if(!pipe_hnd) {
      hnd->status = NT_STATUS_INVALID_HANDLE;
      return CAC_FAILURE;
   }

   /*only works if info_class parm is 12*/
   hnd->status = rpccli_lsa_query_info_policy2(pipe_hnd, mem_ctx, op->in.pol, 12,
                                             &domain_name, &dns_name, &forest_name, &domain_guid, &domain_sid);

   if(!NT_STATUS_IS_OK(hnd->status)) {
      return CAC_FAILURE;
   }
   
   op->out.domain_name    = domain_name;
   op->out.dns_name       = dns_name;
   op->out.forest_name    = forest_name;
   op->out.domain_guid    = domain_guid;
   op->out.domain_sid     = domain_sid;

   return CAC_SUCCESS;
}

int cac_LsaEnumSids(CacServerHandle *hnd, TALLOC_CTX *mem_ctx, struct LsaEnumSids *op) {
   struct rpc_pipe_client *pipe_hnd = NULL;

   uint32 num_sids;
   DOM_SID *sids;

   if(!hnd)
      return CAC_FAILURE;

   if(!hnd->_internal.ctx || !hnd->_internal.pipes[PI_LSARPC]) {
      hnd->status = NT_STATUS_INVALID_HANDLE;
      return CAC_FAILURE;
   }

   if(!op || !op->in.pol) {
      hnd->status = NT_STATUS_INVALID_PARAMETER;
      return CAC_FAILURE;
   }

   pipe_hnd = cac_GetPipe(hnd, PI_LSARPC);
   if(!pipe_hnd) {
      hnd->status = NT_STATUS_INVALID_HANDLE;
      return CAC_FAILURE;
   }

   hnd->status = rpccli_lsa_enum_sids(pipe_hnd, mem_ctx, op->in.pol, &(op->out.resume_idx), op->in.pref_max_sids, &num_sids, &sids);

   if(!NT_STATUS_IS_OK(hnd->status)) {
      return CAC_FAILURE;
   }

   op->out.num_sids  = num_sids;
   op->out.sids      = sids;

   return CAC_SUCCESS;

}

int cac_LsaEnumAccountRights(CacServerHandle *hnd, TALLOC_CTX *mem_ctx, struct LsaEnumAccountRights *op) {
   struct rpc_pipe_client *pipe_hnd = NULL;

   uint32 count = 0;
   char **privs = NULL;

   if(!hnd)
      return CAC_FAILURE;

   if(!hnd->_internal.ctx || !hnd->_internal.pipes[PI_LSARPC]) {
      hnd->status = NT_STATUS_INVALID_HANDLE;
      return CAC_FAILURE;
   }

   if(!op->in.pol) {
      hnd->status = NT_STATUS_INVALID_PARAMETER;
      return CAC_FAILURE;
   }

   if(!op->in.name && !op->in.sid) {
      hnd->status = NT_STATUS_INVALID_PARAMETER;
      return CAC_FAILURE;
   }

   pipe_hnd = cac_GetPipe(hnd, PI_LSARPC);
   if(!pipe_hnd) {
      hnd->status = NT_STATUS_INVALID_HANDLE;
      return CAC_FAILURE;
   }

   if(op->in.name && !op->in.sid) {
      DOM_SID *user_sid = NULL;
      uint32 *type;

      /*lookup the SID*/
      hnd->status = rpccli_lsa_lookup_names( pipe_hnd, mem_ctx, op->in.pol, 1, (const char **)&(op->in.name), NULL, &user_sid, &type);

      if(!NT_STATUS_IS_OK(hnd->status))
         return CAC_FAILURE;

      op->in.sid = user_sid;
   }
   
   hnd->status = rpccli_lsa_enum_account_rights( pipe_hnd, mem_ctx, op->in.pol, op->in.sid,
                                                   &count, &privs);

   if(!NT_STATUS_IS_OK(hnd->status)) {
      return CAC_FAILURE;
   }

   op->out.num_privs = count;
   op->out.priv_names = privs;

   return CAC_SUCCESS;
}

int cac_LsaEnumTrustedDomains(CacServerHandle *hnd, TALLOC_CTX *mem_ctx, struct LsaEnumTrustedDomains *op) {
   struct rpc_pipe_client *pipe_hnd;
   
   uint32 num_domains;
   char **domain_names;
   DOM_SID *domain_sids;
   
   if(!hnd)
      return CAC_FAILURE;

   if(!hnd->_internal.ctx || !hnd->_internal.pipes[PI_LSARPC]) {
      hnd->status = NT_STATUS_INVALID_HANDLE;
      return CAC_FAILURE;
   }

   if(!op->in.pol) {
      hnd->status = NT_STATUS_INVALID_PARAMETER;
      return CAC_FAILURE;
   }

   pipe_hnd = cac_GetPipe(hnd, PI_LSARPC);
   if(!pipe_hnd) {
      hnd->status = NT_STATUS_INVALID_HANDLE;
      return CAC_FAILURE;
   }

   hnd->status = rpccli_lsa_enum_trust_dom( pipe_hnd, mem_ctx, op->in.pol, &(op->out.resume_idx), &num_domains, &domain_names, &domain_sids);

   if(!NT_STATUS_IS_OK(hnd->status)) {
      return CAC_FAILURE;
   }

   op->out.num_domains = num_domains;
   op->out.domain_names = domain_names;
   op->out.domain_sids  = domain_sids;

   return CAC_SUCCESS;
}

int cac_LsaOpenTrustedDomain(CacServerHandle *hnd, TALLOC_CTX *mem_ctx, struct LsaOpenTrustedDomain *op) {
   struct rpc_pipe_client *pipe_hnd = NULL;

   POLICY_HND *dom_pol = NULL;

   if(!hnd)
      return CAC_FAILURE;

   if(!hnd->_internal.ctx || !hnd->_internal.pipes[PI_LSARPC]) {
      hnd->status = NT_STATUS_INVALID_HANDLE;
      return CAC_FAILURE;
   }

   if(!op->in.pol || !op->in.access || !op->in.domain_sid) {
      hnd->status = NT_STATUS_INVALID_PARAMETER;
      return CAC_FAILURE;
   }

   pipe_hnd = cac_GetPipe(hnd, PI_LSARPC);
   if(!pipe_hnd) {
      hnd->status = NT_STATUS_INVALID_HANDLE;
      return CAC_FAILURE;
   }

   dom_pol = talloc(mem_ctx, POLICY_HND);
   if(!dom_pol) {
      hnd->status = NT_STATUS_NO_MEMORY;
      errno = ENOMEM;
      return CAC_FAILURE;
   }
   
   hnd->status = rpccli_lsa_open_trusted_domain( pipe_hnd, mem_ctx, op->in.pol, op->in.domain_sid, op->in.access, dom_pol);

   if(!NT_STATUS_IS_OK(hnd->status)) {
      return CAC_FAILURE;
   }

   op->out.domain_pol = dom_pol;

   return CAC_SUCCESS;
}

int cac_LsaQueryTrustedDomainInfo(CacServerHandle *hnd, TALLOC_CTX *mem_ctx, struct LsaQueryTrustedDomainInfo *op) {
   struct rpc_pipe_client *pipe_hnd = NULL;

   LSA_TRUSTED_DOMAIN_INFO *dom_info;

   if(!hnd)
      return CAC_FAILURE;

   if(!hnd->_internal.ctx || !hnd->_internal.pipes[PI_LSARPC]) {
      hnd->status = NT_STATUS_INVALID_HANDLE;
      return CAC_FAILURE;
   }

   if(!op->in.pol || !op->in.info_class) {
      hnd->status = NT_STATUS_INVALID_PARAMETER;
      return CAC_FAILURE;
   }

   if(!op->in.domain_sid && !op->in.domain_name) {
      hnd->status = NT_STATUS_INVALID_PARAMETER;
      return CAC_FAILURE;
   }

   pipe_hnd = cac_GetPipe(hnd, PI_LSARPC);
   if(!pipe_hnd) {
      hnd->status = NT_STATUS_INVALID_HANDLE;
      return CAC_FAILURE;
   }

   if(op->in.domain_sid) {
      hnd->status = rpccli_lsa_query_trusted_domain_info_by_sid( pipe_hnd, mem_ctx, op->in.pol, op->in.info_class, op->in.domain_sid, &dom_info);
   }
   else if(op->in.domain_name) {
      hnd->status = rpccli_lsa_query_trusted_domain_info_by_name( pipe_hnd, mem_ctx, op->in.pol, op->in.info_class, op->in.domain_name, &dom_info);
   }

   if(!NT_STATUS_IS_OK(hnd->status)) {
      return CAC_FAILURE;
   }

   op->out.info = dom_info;

   return CAC_SUCCESS;

}

int cac_LsaEnumPrivileges(CacServerHandle *hnd, TALLOC_CTX *mem_ctx, struct LsaEnumPrivileges *op) {
   struct rpc_pipe_client *pipe_hnd = NULL;

   uint32 num_privs;
   char **priv_names;
   uint32 *high_bits;
   uint32 *low_bits;

   if(!hnd) {
      return CAC_FAILURE;
   }

   if(!hnd->_internal.ctx || !hnd->_internal.pipes[PI_LSARPC]) {
      hnd->status = NT_STATUS_INVALID_HANDLE;
      return CAC_FAILURE;
   }

   if(!op || !op->in.pol) {
      hnd->status = NT_STATUS_INVALID_PARAMETER;
      return CAC_FAILURE;
   }

   pipe_hnd = cac_GetPipe(hnd, PI_LSARPC);
   if(!pipe_hnd) {
      hnd->status = NT_STATUS_INVALID_HANDLE;
      return CAC_FAILURE;
   }

   hnd->status = rpccli_lsa_enum_privilege(pipe_hnd, mem_ctx, op->in.pol, &(op->out.resume_idx), op->in.pref_max_privs,
                                             &num_privs, &priv_names, &high_bits, &low_bits);

   if(!NT_STATUS_IS_OK(hnd->status)) {
      return CAC_FAILURE;
   }

   op->out.num_privs  = num_privs;
   op->out.priv_names = priv_names;
   op->out.high_bits  = high_bits;
   op->out.low_bits   = low_bits;

   return CAC_SUCCESS;
}

int cac_LsaOpenAccount(CacServerHandle *hnd, TALLOC_CTX *mem_ctx, struct LsaOpenAccount *op) {
   struct rpc_pipe_client *pipe_hnd = NULL;

   POLICY_HND *user_pol = NULL;

   if(!hnd) {
      return CAC_FAILURE;
   }

   if(!hnd->_internal.ctx || !hnd->_internal.pipes[PI_LSARPC]) {
      hnd->status = NT_STATUS_INVALID_HANDLE;
      return CAC_FAILURE;
   }

   if(!op || !op->in.pol) {
      hnd->status = NT_STATUS_INVALID_PARAMETER;
      return CAC_FAILURE;
   }

   if(!op->in.sid && !op->in.name) {
      hnd->status = NT_STATUS_INVALID_PARAMETER;
      return CAC_FAILURE;
   }

   pipe_hnd = cac_GetPipe(hnd, PI_LSARPC);
   if(!pipe_hnd) {
      hnd->status = NT_STATUS_INVALID_HANDLE;
      return CAC_FAILURE;
   }

   /*look up the user's SID if we have to*/
   if(op->in.name && !op->in.sid) {
      DOM_SID *user_sid = NULL;
      uint32 *type;

      /*lookup the SID*/
      hnd->status = rpccli_lsa_lookup_names( pipe_hnd, mem_ctx, op->in.pol, 1, (const char **)&(op->in.name), NULL, &user_sid, &type);

      if(!NT_STATUS_IS_OK(hnd->status))
         return CAC_FAILURE;

      op->in.sid = user_sid;
   }

   user_pol = talloc(mem_ctx, POLICY_HND);
   if(!user_pol) {
      hnd->status = NT_STATUS_NO_MEMORY;
      return CAC_FAILURE;
   }

   hnd->status = rpccli_lsa_open_account(pipe_hnd, mem_ctx, op->in.pol, op->in.sid, op->in.access, user_pol);

   if(!NT_STATUS_IS_OK(hnd->status)) {
      TALLOC_FREE(user_pol);
      return CAC_FAILURE;
   }

   op->out.user = user_pol;

   return CAC_SUCCESS;
}


int cac_LsaAddPrivileges(CacServerHandle *hnd, TALLOC_CTX *mem_ctx, struct LsaAddPrivileges *op) {
   struct rpc_pipe_client *pipe_hnd = NULL;

   DOM_SID *user_sid = NULL;
   uint32  *type     = NULL;

   if(!hnd) {
      return CAC_FAILURE;
   }

   if(!hnd->_internal.ctx || !hnd->_internal.pipes[PI_LSARPC]) {
      hnd->status = NT_STATUS_INVALID_HANDLE;
      return CAC_FAILURE;
   }

   if(!op || !op->in.pol || !op->in.priv_names) {
      hnd->status = NT_STATUS_INVALID_PARAMETER;
      return CAC_FAILURE;
   }

   if(!op->in.sid && !op->in.name) {
      hnd->status = NT_STATUS_INVALID_PARAMETER;
      return CAC_FAILURE;
   }

   pipe_hnd = cac_GetPipe(hnd, PI_LSARPC);
   if(!pipe_hnd) {
      hnd->status = NT_STATUS_INVALID_HANDLE;
      return CAC_FAILURE;
   }

   if(op->in.name && !op->in.sid) {
      /*lookup the SID*/
      hnd->status = rpccli_lsa_lookup_names( pipe_hnd, mem_ctx, op->in.pol, 1, (const char **)&(op->in.name), NULL, &user_sid, &type);

      if(!NT_STATUS_IS_OK(hnd->status))
         return CAC_FAILURE;

      op->in.sid = user_sid;
   }

   hnd->status = rpccli_lsa_add_account_rights( pipe_hnd, mem_ctx, op->in.pol, *(op->in.sid), op->in.num_privs, (const char **)op->in.priv_names);

   if(!NT_STATUS_IS_OK(hnd->status)) {
      return CAC_FAILURE;
   }

   return CAC_SUCCESS;
}

int cac_LsaRemovePrivileges(CacServerHandle *hnd, TALLOC_CTX *mem_ctx, struct LsaRemovePrivileges *op) {
   struct rpc_pipe_client *pipe_hnd = NULL;

   DOM_SID *user_sid = NULL;
   uint32  *type     = NULL;

   if(!hnd) {
      return CAC_FAILURE;
   }

   if(!hnd->_internal.ctx || !hnd->_internal.pipes[PI_LSARPC]) {
      hnd->status = NT_STATUS_INVALID_HANDLE;
      return CAC_FAILURE;
   }

   if(!op || !op->in.pol || !op->in.priv_names) {
      hnd->status = NT_STATUS_INVALID_PARAMETER;
      return CAC_FAILURE;
   }

   if(!op->in.sid && !op->in.name) {
      hnd->status = NT_STATUS_INVALID_PARAMETER;
      return CAC_FAILURE;
   }

   pipe_hnd = cac_GetPipe(hnd, PI_LSARPC);
   if(!pipe_hnd) {
      hnd->status = NT_STATUS_INVALID_HANDLE;
      return CAC_FAILURE;
   }

   if(op->in.name && !op->in.sid) {
      /*lookup the SID*/
      hnd->status = rpccli_lsa_lookup_names( pipe_hnd, mem_ctx, op->in.pol, 1, (const char **)&(op->in.name), NULL, &user_sid, &type);

      if(!NT_STATUS_IS_OK(hnd->status))
         return CAC_FAILURE;

      op->in.sid = user_sid;
   }

   hnd->status = rpccli_lsa_remove_account_rights( pipe_hnd, mem_ctx, op->in.pol, *(op->in.sid), False, op->in.num_privs, (const char **)op->in.priv_names);

   if(!NT_STATUS_IS_OK(hnd->status)) {
      return CAC_FAILURE;
   }

   return CAC_SUCCESS;
}

int cac_LsaClearPrivileges(CacServerHandle *hnd, TALLOC_CTX *mem_ctx, struct LsaClearPrivileges *op) {
   struct rpc_pipe_client *pipe_hnd = NULL;

   DOM_SID *user_sid = NULL;
   uint32  *type     = NULL;

   if(!hnd) {
      return CAC_FAILURE;
   }

   if(!hnd->_internal.ctx || !hnd->_internal.pipes[PI_LSARPC]) {
      hnd->status = NT_STATUS_INVALID_HANDLE;
      return CAC_FAILURE;
   }

   if(!op || !op->in.pol) {
      hnd->status = NT_STATUS_INVALID_PARAMETER;
      return CAC_FAILURE;
   }

   if(!op->in.sid && !op->in.name) {
      hnd->status = NT_STATUS_INVALID_PARAMETER;
      return CAC_FAILURE;
   }

   pipe_hnd = cac_GetPipe(hnd, PI_LSARPC);
   if(!pipe_hnd) {
      hnd->status = NT_STATUS_INVALID_HANDLE;
      return CAC_FAILURE;
   }

   if(op->in.name && !op->in.sid) {
      /*lookup the SID*/
      hnd->status = rpccli_lsa_lookup_names( pipe_hnd, mem_ctx, op->in.pol, 1, (const char **)&(op->in.name), NULL, &user_sid, &type);

      if(!NT_STATUS_IS_OK(hnd->status))
         return CAC_FAILURE;

      op->in.sid = user_sid;
   }

   hnd->status = rpccli_lsa_remove_account_rights( pipe_hnd, mem_ctx, op->in.pol, *(op->in.sid), True, 0, NULL);

   if(!NT_STATUS_IS_OK(hnd->status)) {
      return CAC_FAILURE;
   }

   return CAC_SUCCESS;
}

int cac_LsaSetPrivileges(CacServerHandle *hnd, TALLOC_CTX *mem_ctx, struct LsaAddPrivileges *op) {
   struct rpc_pipe_client *pipe_hnd = NULL;

   DOM_SID *user_sid = NULL;
   uint32  *type     = NULL;

   if(!hnd) {
      return CAC_FAILURE;
   }

   if(!hnd->_internal.ctx || !hnd->_internal.pipes[PI_LSARPC]) {
      hnd->status = NT_STATUS_INVALID_HANDLE;
      return CAC_FAILURE;
   }

   if(!op || !op->in.pol || !op->in.priv_names) {
      hnd->status = NT_STATUS_INVALID_PARAMETER;
      return CAC_FAILURE;
   }

   if(!op->in.sid && !op->in.name) {
      hnd->status = NT_STATUS_INVALID_PARAMETER;
      return CAC_FAILURE;
   }

   pipe_hnd = cac_GetPipe(hnd, PI_LSARPC);
   if(!pipe_hnd) {
      return CAC_FAILURE;
   }

   if(op->in.name && !op->in.sid) {
      /*lookup the SID*/
      hnd->status = rpccli_lsa_lookup_names( pipe_hnd, mem_ctx, op->in.pol, 1, (const char **)&(op->in.name), NULL, &user_sid, &type);

      if(!NT_STATUS_IS_OK(hnd->status))
         return CAC_FAILURE;

      op->in.sid = user_sid;
   }

   /*first remove all privileges*/
   hnd->status = rpccli_lsa_remove_account_rights( pipe_hnd, mem_ctx, op->in.pol, *(op->in.sid), True, 0, NULL);

   if(!NT_STATUS_IS_OK(hnd->status)) {
      return CAC_FAILURE;
   }

   hnd->status = rpccli_lsa_add_account_rights( pipe_hnd, mem_ctx, op->in.pol, *(op->in.sid), op->in.num_privs, (const char **)op->in.priv_names);

   if(!NT_STATUS_IS_OK(hnd->status)) {
      return CAC_FAILURE;
   }

   return CAC_SUCCESS;
}

int cac_LsaGetSecurityObject(CacServerHandle *hnd, TALLOC_CTX *mem_ctx, struct LsaGetSecurityObject *op) {
   struct rpc_pipe_client *pipe_hnd = NULL;

   /*this is taken from rpcclient/cmd_lsarpc.c*/
   uint16 info_level = 4;

   SEC_DESC_BUF *sec_out = NULL;

   if(!hnd) {
      return CAC_FAILURE;
   }

   if(!hnd->_internal.ctx || !hnd->_internal.pipes[PI_LSARPC]) {
      hnd->status = NT_STATUS_INVALID_HANDLE;
      return CAC_FAILURE;
   }

   if(!op || !op->in.pol) {
      hnd->status = NT_STATUS_INVALID_PARAMETER;
      return CAC_FAILURE;
   }

   pipe_hnd = cac_GetPipe(hnd, PI_LSARPC);
   if(!pipe_hnd) {
      hnd->status = NT_STATUS_INVALID_HANDLE;
      return CAC_FAILURE;
   }

   hnd->status = rpccli_lsa_query_secobj( pipe_hnd, mem_ctx, op->in.pol, info_level, &sec_out);

   if(!NT_STATUS_IS_OK(hnd->status))
      return CAC_FAILURE;

   op->out.sec = sec_out;

   return CAC_FAILURE;
}
