/* 
 *  Unix SMB/CIFS implementation.
 *  MS-RPC client library implementation (SAMR pipe)
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
#include "libmsrpc_internal.h"

/*used by cac_SamGetNamesFromRids*/
#define SAMR_RID_UNKNOWN 8

#define SAMR_ENUM_MAX_SIZE 0xffff

/*not sure what this is.. taken from rpcclient/cmd_samr.c*/
#define SAMR_LOOKUP_FLAGS 0x000003e8

int cac_SamConnect(CacServerHandle *hnd, TALLOC_CTX *mem_ctx, struct SamConnect *op) {
   SMBCSRV *srv        = NULL;
   struct rpc_pipe_client *pipe_hnd = NULL;
   POLICY_HND *sam_out = NULL;

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
   if(!hnd->_internal.pipes[PI_SAMR]) {
      if(!(pipe_hnd = cli_rpc_pipe_open_noauth(&srv->cli, PI_SAMR, &(hnd->status)))) {
         return CAC_FAILURE;
      }

      hnd->_internal.pipes[PI_SAMR] = True;
   }

   pipe_hnd = cac_GetPipe(hnd, PI_SAMR);
   if(!pipe_hnd) {
      hnd->status = NT_STATUS_INVALID_HANDLE;
      return CAC_FAILURE;
   }

   sam_out = talloc(mem_ctx, POLICY_HND);
   if(!sam_out) {
      hnd->status = NT_STATUS_NO_MEMORY;
      return CAC_FAILURE;
   }

   if(hnd->_internal.srv_level >= SRV_WIN_2K_SP3) {
      hnd->status = rpccli_samr_connect4( pipe_hnd, mem_ctx, op->in.access, sam_out);
   }

   if(hnd->_internal.srv_level < SRV_WIN_2K_SP3 || !NT_STATUS_IS_OK(hnd->status)) {
      /*if sam_connect4 failed, the use sam_connect and lower srv_level*/

      hnd->status = rpccli_samr_connect( pipe_hnd, mem_ctx, op->in.access, sam_out);

      if(NT_STATUS_IS_OK(hnd->status) && hnd->_internal.srv_level > SRV_WIN_2K) {
         hnd->_internal.srv_level = SRV_WIN_2K;
      }
   }

   if(!NT_STATUS_IS_OK(hnd->status))
      return CAC_FAILURE;

   op->out.sam = sam_out;

   return CAC_SUCCESS;
}

int cac_SamClose(CacServerHandle *hnd, TALLOC_CTX *mem_ctx, POLICY_HND *sam) {
   struct rpc_pipe_client *pipe_hnd        = NULL;

   if(!hnd) 
      return CAC_FAILURE;

   if(!hnd->_internal.ctx || !hnd->_internal.pipes[PI_SAMR]) {
      hnd->status = NT_STATUS_INVALID_HANDLE;
      return CAC_FAILURE;
   }

   if(!sam || !mem_ctx) {
      hnd->status = NT_STATUS_INVALID_PARAMETER;
      return CAC_FAILURE;
   }

   pipe_hnd = cac_GetPipe(hnd, PI_SAMR);
   if(!pipe_hnd) {
      hnd->status = NT_STATUS_INVALID_HANDLE;
      return CAC_FAILURE;
   }
   
   hnd->status = rpccli_samr_close( pipe_hnd, mem_ctx, sam);
   
   if(!NT_STATUS_IS_OK(hnd->status))
      return CAC_FAILURE;

   return CAC_SUCCESS;
}

/*this is an internal function. Due to a circular dependency, it must be prototyped in libmsrpc.h (which I don't want to do)
 * cac_SamOpenDomain() is the only function that calls it, so I just put the definition here
 */
/*attempts to find the sid of the domain we are connected to*/
DOM_SID *cac_get_domain_sid(CacServerHandle *hnd, TALLOC_CTX *mem_ctx, uint32 des_access) {
   struct LsaOpenPolicy lop;
   struct LsaFetchSid fs;

   DOM_SID *sid;
   
   ZERO_STRUCT(lop);
   ZERO_STRUCT(fs);

   lop.in.access = des_access;
   lop.in.security_qos = True;

   if(!cac_LsaOpenPolicy(hnd, mem_ctx, &lop))
      return NULL;

   fs.in.pol = lop.out.pol;
   fs.in.info_class = CAC_DOMAIN_INFO;

   if(!cac_LsaFetchSid(hnd, mem_ctx, &fs))
      return NULL;
   
   cac_LsaClosePolicy(hnd, mem_ctx, lop.out.pol);

   if(!fs.out.domain_sid)
      return NULL;

   sid = talloc_memdup(mem_ctx, &(fs.out.domain_sid->sid), sizeof(DOM_SID));

   if(!sid) {
      hnd->status = NT_STATUS_NO_MEMORY;
   }

   return sid;

}

int cac_SamOpenDomain(CacServerHandle *hnd, TALLOC_CTX *mem_ctx, struct SamOpenDomain *op) {
   struct rpc_pipe_client *pipe_hnd = NULL;

   DOM_SID *sid_buf;
   POLICY_HND *sam_out;
   POLICY_HND *pol_out;

   struct SamLookupDomain sld;

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

   if(!op->in.sam) {
      /*use cac_SamConnect() since it does the session setup*/
      struct SamConnect sc;
      ZERO_STRUCT(sc);

      sc.in.access = op->in.access;

      if(!cac_SamConnect(hnd, mem_ctx, &sc)) {
         return CAC_FAILURE;
      }

      sam_out = sc.out.sam;
   }
   else {
      sam_out = op->in.sam;
   }

   if(!op->in.sid) {
      /*find the sid for the SAM's domain*/
      
      /*try using cac_SamLookupDomain() first*/
      ZERO_STRUCT(sld);

      sld.in.sam  = sam_out;
      sld.in.name = hnd->domain;

      if(cac_SamLookupDomain(hnd, mem_ctx, &sld)) {
         /*then we got the sid*/
         sid_buf = sld.out.sid;
      }
      else {
         /*try to get it from the LSA*/
         sid_buf = cac_get_domain_sid(hnd, mem_ctx, op->in.access);
      }
   }
   else {
      /*we already have the sid for the domain we want*/
      sid_buf = op->in.sid;
   }

   pipe_hnd = cac_GetPipe(hnd, PI_SAMR);
   if(!pipe_hnd) {
      hnd->status = NT_STATUS_INVALID_HANDLE;
      return CAC_FAILURE;
   }

   pol_out = talloc(mem_ctx, POLICY_HND);
   if(!pol_out) {
      hnd->status = NT_STATUS_NO_MEMORY;
      return CAC_FAILURE;
   }

   /*now open the domain*/
   hnd->status = rpccli_samr_open_domain( pipe_hnd, mem_ctx, sam_out, op->in.access, sid_buf, pol_out);

   if(!NT_STATUS_IS_OK(hnd->status))
      return CAC_FAILURE;

   op->out.sam     = sam_out;
   op->out.dom_hnd = pol_out;

   return CAC_SUCCESS;
}

int cac_SamOpenUser(CacServerHandle *hnd, TALLOC_CTX *mem_ctx, struct SamOpenUser *op) {
   struct rpc_pipe_client *pipe_hnd = NULL;

   uint32 *rid_buf = NULL;

   uint32 num_rids   = 0;
   uint32 *rid_types = NULL;

   POLICY_HND *user_out = NULL;

   if(!hnd) 
      return CAC_FAILURE;

   if(!hnd->_internal.ctx || !hnd->_internal.pipes[PI_SAMR]) {
      hnd->status = NT_STATUS_INVALID_HANDLE;
      return CAC_FAILURE;
   }

   if(!op || !op->in.dom_hnd || op->in.access == 0 || !mem_ctx) {
      hnd->status = NT_STATUS_INVALID_PARAMETER;
      return CAC_FAILURE;
   }

   if(op->in.rid == 0 && op->in.name == NULL) {
      hnd->status = NT_STATUS_INVALID_PARAMETER;
      return CAC_FAILURE;
   }

   pipe_hnd = cac_GetPipe(hnd, PI_SAMR);
   if(!pipe_hnd) {
      hnd->status = NT_STATUS_INVALID_HANDLE;
      return CAC_FAILURE;
   }

   if(op->in.rid == 0 && op->in.name) {
      /*lookup the name and then set rid_buf*/

      hnd->status = rpccli_samr_lookup_names( pipe_hnd, mem_ctx, op->in.dom_hnd, SAMR_LOOKUP_FLAGS, 1, (const char **)&op->in.name, 
            &num_rids, &rid_buf, &rid_types); 

      if(!NT_STATUS_IS_OK(hnd->status))
         return CAC_FAILURE;

      if(num_rids == 0 || rid_buf == NULL || rid_types[0] == SAMR_RID_UNKNOWN) {
         hnd->status = NT_STATUS_INVALID_PARAMETER;
         return CAC_FAILURE;
      }

      TALLOC_FREE(rid_types);

   }
   else {
      rid_buf = &op->in.rid;
   }

   user_out = talloc(mem_ctx, POLICY_HND);
   if(!user_out) {
      hnd->status = NT_STATUS_NO_MEMORY;
      return CAC_FAILURE;
   }

   hnd->status = rpccli_samr_open_user(pipe_hnd, mem_ctx, op->in.dom_hnd, op->in.access, *rid_buf, user_out);
   
   if(!NT_STATUS_IS_OK(hnd->status))
      return CAC_FAILURE;

   op->out.user_hnd = user_out;

   return CAC_SUCCESS;
}

int cac_SamCreateUser(CacServerHandle *hnd, TALLOC_CTX *mem_ctx, struct SamCreateUser *op) {
   struct rpc_pipe_client *pipe_hnd = NULL;

   POLICY_HND *user_out = NULL;
   uint32 rid_out;

   /**found in rpcclient/cmd_samr.c*/
   uint32 unknown = 0xe005000b;

   if(!hnd) 
      return CAC_FAILURE;

   if(!hnd->_internal.ctx || !hnd->_internal.pipes[PI_SAMR]) {
      hnd->status = NT_STATUS_INVALID_HANDLE;
      return CAC_FAILURE;
   }

   if(!op || !op->in.dom_hnd || !op->in.name || op->in.acb_mask == 0 || !mem_ctx) {
      hnd->status = NT_STATUS_INVALID_PARAMETER;
      return CAC_FAILURE;
   }

   pipe_hnd = cac_GetPipe(hnd, PI_SAMR);
   if(!pipe_hnd) {
      hnd->status = NT_STATUS_INVALID_HANDLE;
      return CAC_FAILURE;
   }

   user_out = talloc(mem_ctx, POLICY_HND);
   if(!user_out) {
      hnd->status = NT_STATUS_NO_MEMORY;
      return CAC_FAILURE;
   }

   hnd->status = rpccli_samr_create_dom_user( pipe_hnd, mem_ctx, op->in.dom_hnd, op->in.name, op->in.acb_mask, unknown, user_out, &rid_out);

   if(!NT_STATUS_IS_OK(hnd->status))
      return CAC_FAILURE;

   op->out.user_hnd = user_out;
   op->out.rid      = rid_out;

   return CAC_SUCCESS;
}

int cac_SamDeleteUser(CacServerHandle *hnd, TALLOC_CTX *mem_ctx, POLICY_HND *user_hnd) {
   struct rpc_pipe_client *pipe_hnd = NULL;

   if(!hnd) 
      return CAC_FAILURE;

   if(!hnd->_internal.ctx || !hnd->_internal.pipes[PI_SAMR]) {
      hnd->status = NT_STATUS_INVALID_HANDLE;
      return CAC_FAILURE;
   }

   if(!user_hnd || !mem_ctx) {
      hnd->status = NT_STATUS_INVALID_PARAMETER;
      return CAC_FAILURE;
   }

   pipe_hnd = cac_GetPipe(hnd, PI_SAMR);
   if(!pipe_hnd) {
      hnd->status = NT_STATUS_INVALID_HANDLE;
      return CAC_FAILURE;
   }

   hnd->status = rpccli_samr_delete_dom_user( pipe_hnd, mem_ctx, user_hnd);

   if(!NT_STATUS_IS_OK(hnd->status))
      return CAC_FAILURE;

   return CAC_SUCCESS;
}

int cac_SamEnumUsers(CacServerHandle *hnd, TALLOC_CTX *mem_ctx, struct SamEnumUsers *op) {
   struct rpc_pipe_client *pipe_hnd = NULL;

   uint32 resume_idx_out = 0;
   char **names_out      = NULL;
   uint32 *rids_out      = NULL;
   uint32 num_users_out  = 0;

   if(!hnd) 
      return CAC_FAILURE;

   if(!hnd->_internal.ctx || !hnd->_internal.pipes[PI_SAMR]) {
      hnd->status = NT_STATUS_INVALID_HANDLE;
      return CAC_FAILURE;
   }

   if(!op || !op->in.dom_hnd || !mem_ctx) {
      hnd->status = NT_STATUS_INVALID_PARAMETER;
      return CAC_FAILURE;
   }

   /*this is a hack.. but is the only reliable way to know if everything has been enumerated*/
   if(op->out.done == True)
      return CAC_FAILURE;

   pipe_hnd = cac_GetPipe(hnd, PI_SAMR);
   if(!pipe_hnd) {
      hnd->status = NT_STATUS_INVALID_HANDLE;
      return CAC_FAILURE;
   }

   resume_idx_out = op->out.resume_idx;

   hnd->status = rpccli_samr_enum_dom_users( pipe_hnd, mem_ctx, op->in.dom_hnd, &resume_idx_out, op->in.acb_mask, SAMR_ENUM_MAX_SIZE, 
                                                &names_out, &rids_out, &num_users_out);


   if(NT_STATUS_IS_OK(hnd->status))
      op->out.done = True;

   /*if there are no more entries, the operation will return NT_STATUS_OK. 
    * We want to return failure if no results were returned*/
   if(!NT_STATUS_IS_OK(hnd->status) && NT_STATUS_V(hnd->status) != NT_STATUS_V(STATUS_MORE_ENTRIES))
      return CAC_FAILURE;

   op->out.resume_idx= resume_idx_out;
   op->out.num_users = num_users_out;
   op->out.rids      = rids_out;
   op->out.names     = names_out;

   return CAC_SUCCESS;
}

int cac_SamGetNamesFromRids(CacServerHandle *hnd, TALLOC_CTX *mem_ctx, struct SamGetNamesFromRids *op) {
   struct rpc_pipe_client *pipe_hnd = NULL;

   uint32 num_names_out;
   char **names_out;
   uint32 *name_types_out;


   uint32 i = 0;

   CacLookupRidsRecord *map_out;

   if(!hnd) 
      return CAC_FAILURE;

   if(!hnd->_internal.ctx || !hnd->_internal.pipes[PI_SAMR]) {
      hnd->status = NT_STATUS_INVALID_HANDLE;
      return CAC_FAILURE;
   }

   if(!op || !op->in.dom_hnd || !mem_ctx) {
      hnd->status = NT_STATUS_INVALID_PARAMETER;
      return CAC_FAILURE;
   }

   if(!op->in.rids && op->in.num_rids != 0) {
      hnd->status = NT_STATUS_INVALID_PARAMETER;
      return CAC_FAILURE;
   }

   if(op->in.num_rids == 0) {
      /*nothing to do*/
      op->out.num_names = 0;
      return CAC_SUCCESS;
   }

   pipe_hnd = cac_GetPipe(hnd, PI_SAMR);
   if(!pipe_hnd) {
      hnd->status = NT_STATUS_INVALID_HANDLE;
      return CAC_FAILURE;
   }

   hnd->status = rpccli_samr_lookup_rids( pipe_hnd, mem_ctx, op->in.dom_hnd, op->in.num_rids, op->in.rids, &num_names_out, &names_out, &name_types_out); 

   if(!NT_STATUS_IS_OK(hnd->status) && !NT_STATUS_EQUAL(hnd->status, STATUS_SOME_UNMAPPED))
      return CAC_FAILURE;

   map_out = TALLOC_ARRAY(mem_ctx, CacLookupRidsRecord, num_names_out);
   if(!map_out) {
      hnd->status = NT_STATUS_NO_MEMORY;
      return CAC_FAILURE;
   }

   for(i = 0; i < num_names_out; i++) {
      if(name_types_out[i] == SAMR_RID_UNKNOWN) {
         map_out[i].found = False;
         map_out[i].name  = NULL;
         map_out[i].type  = 0;
      }
      else {
         map_out[i].found = True;
         map_out[i].name = talloc_strdup(mem_ctx, names_out[i]);
         map_out[i].type = name_types_out[i];
      }
      map_out[i].rid = op->in.rids[i];
   }

   TALLOC_FREE(names_out);
   TALLOC_FREE(name_types_out);
   
   op->out.num_names = num_names_out;
   op->out.map       = map_out;
   
   if(NT_STATUS_EQUAL(hnd->status, STATUS_SOME_UNMAPPED))
      return CAC_PARTIAL_SUCCESS;

   return CAC_SUCCESS;
}

int cac_SamGetRidsFromNames(CacServerHandle *hnd, TALLOC_CTX *mem_ctx, struct SamGetRidsFromNames *op) {
   struct rpc_pipe_client *pipe_hnd = NULL;

   uint32 num_rids_out;
   uint32 *rids_out;
   uint32 *rid_types_out;

   uint32 i = 0;

   CacLookupRidsRecord *map_out;

   if(!hnd) 
      return CAC_FAILURE;

   if(!hnd->_internal.ctx || !hnd->_internal.pipes[PI_SAMR]) {
      hnd->status = NT_STATUS_INVALID_HANDLE;
      return CAC_FAILURE;
   }

   if(!op || !op->in.dom_hnd || !mem_ctx) {
      hnd->status = NT_STATUS_INVALID_PARAMETER;
      return CAC_FAILURE;
   }

   if(!op->in.names && op->in.num_names != 0) {
      hnd->status = NT_STATUS_INVALID_PARAMETER;
      return CAC_FAILURE;
   }

   if(op->in.num_names == 0) {
      /*then we don't have to do anything*/
      op->out.num_rids = 0;
      return CAC_SUCCESS;
   }

   pipe_hnd = cac_GetPipe(hnd, PI_SAMR);
   if(!pipe_hnd) {
      hnd->status = NT_STATUS_INVALID_HANDLE;
      return CAC_FAILURE;
   }

   hnd->status = rpccli_samr_lookup_names( pipe_hnd, mem_ctx, op->in.dom_hnd, SAMR_LOOKUP_FLAGS, op->in.num_names, (const char **)op->in.names, 
                                          &num_rids_out, &rids_out, &rid_types_out); 

   if(!NT_STATUS_IS_OK(hnd->status) && !NT_STATUS_EQUAL(hnd->status, STATUS_SOME_UNMAPPED))
      return CAC_FAILURE;

   map_out = TALLOC_ARRAY(mem_ctx, CacLookupRidsRecord, num_rids_out);
   if(!map_out) {
      hnd->status = NT_STATUS_NO_MEMORY;
      return CAC_FAILURE;
   }

   for(i = 0; i < num_rids_out; i++) {

      if(rid_types_out[i] == SAMR_RID_UNKNOWN) {
         map_out[i].found = False;
         map_out[i].rid   = 0;
         map_out[i].type  = 0;
      }
      else {
         map_out[i].found = True;
         map_out[i].rid   = rids_out[i];
         map_out[i].type  = rid_types_out[i];
      }

      map_out[i].name = talloc_strdup(mem_ctx, op->in.names[i]);
   }

   op->out.num_rids = num_rids_out;
   op->out.map      = map_out;

   TALLOC_FREE(rids_out);
   TALLOC_FREE(rid_types_out);
   
   if(NT_STATUS_EQUAL(hnd->status, STATUS_SOME_UNMAPPED))
      return CAC_PARTIAL_SUCCESS;

   return CAC_SUCCESS;
}


int cac_SamGetGroupsForUser(CacServerHandle *hnd, TALLOC_CTX *mem_ctx, struct SamGetGroupsForUser *op) {
   struct rpc_pipe_client *pipe_hnd = NULL;

   DOM_GID *groups = NULL;
   uint32 num_groups_out = 0;

   uint32 *rids_out = NULL;
   uint32 *attr_out = NULL;

   uint32 i;

   if(!hnd) 
      return CAC_FAILURE;

   if(!hnd->_internal.ctx || !hnd->_internal.pipes[PI_SAMR]) {
      hnd->status = NT_STATUS_INVALID_HANDLE;
      return CAC_FAILURE;
   }

   if(!op || !op->in.user_hnd || !mem_ctx) {
      hnd->status = NT_STATUS_INVALID_PARAMETER;
      return CAC_FAILURE;
   }

   pipe_hnd = cac_GetPipe(hnd, PI_SAMR);
   if(!pipe_hnd) {
      hnd->status = NT_STATUS_INVALID_HANDLE;
      return CAC_FAILURE;
   }

   hnd->status = rpccli_samr_query_usergroups(pipe_hnd, mem_ctx, op->in.user_hnd, &num_groups_out, &groups);

   if(!NT_STATUS_IS_OK(hnd->status))
      return CAC_FAILURE;


   rids_out = talloc_array(mem_ctx, uint32, num_groups_out);
   if(!rids_out) {
      hnd->status = NT_STATUS_NO_MEMORY;
      return CAC_FAILURE;
   }

   attr_out = talloc_array(mem_ctx, uint32, num_groups_out);
   if(!attr_out) {
      hnd->status = NT_STATUS_NO_MEMORY;
      return CAC_FAILURE;
   }
   
   for(i = 0; i < num_groups_out; i++) {
      rids_out[i] = groups[i].g_rid;
      attr_out[i] = groups[i].attr;
   }

   TALLOC_FREE(groups);

   op->out.num_groups = num_groups_out;
   op->out.rids = rids_out;
   op->out.attributes = attr_out;

   return CAC_SUCCESS;
}


int cac_SamOpenGroup(CacServerHandle *hnd, TALLOC_CTX *mem_ctx, struct SamOpenGroup *op) {
   struct rpc_pipe_client *pipe_hnd = NULL;

   POLICY_HND *group_hnd_out = NULL;

   if(!hnd) 
      return CAC_FAILURE;

   if(!hnd->_internal.ctx || !hnd->_internal.pipes[PI_SAMR]) {
      hnd->status = NT_STATUS_INVALID_HANDLE;
      return CAC_FAILURE;
   }

   if(!op || op->in.access == 0 || op->in.rid == 0 || !mem_ctx) {
      hnd->status = NT_STATUS_INVALID_PARAMETER;
      return CAC_FAILURE;
   }

   pipe_hnd = cac_GetPipe(hnd, PI_SAMR);
   if(!pipe_hnd) {
      hnd->status = NT_STATUS_INVALID_HANDLE;
      return CAC_FAILURE;
   }

   group_hnd_out = talloc(mem_ctx, POLICY_HND);
   if(!group_hnd_out) {
      hnd->status = NT_STATUS_NO_MEMORY;
      return CAC_FAILURE;
   }

   hnd->status = rpccli_samr_open_group( pipe_hnd, mem_ctx, op->in.dom_hnd, op->in.access, op->in.rid, group_hnd_out);

   if(!NT_STATUS_IS_OK(hnd->status))
      return CAC_FAILURE;

   op->out.group_hnd = group_hnd_out;

   return CAC_SUCCESS;
}

int cac_SamCreateGroup(CacServerHandle *hnd, TALLOC_CTX *mem_ctx, struct SamCreateGroup *op) {
   struct rpc_pipe_client *pipe_hnd = NULL;

   POLICY_HND *group_hnd_out = NULL;

   if(!hnd) 
      return CAC_FAILURE;

   if(!hnd->_internal.ctx || !hnd->_internal.pipes[PI_SAMR]) {
      hnd->status = NT_STATUS_INVALID_HANDLE;
      return CAC_FAILURE;
   }

   if(!op || !op->in.name || op->in.name[0] == '\0' || op->in.access == 0 || !mem_ctx) {
      hnd->status = NT_STATUS_INVALID_PARAMETER;
      return CAC_FAILURE;
   }

   pipe_hnd = cac_GetPipe(hnd, PI_SAMR);
   if(!pipe_hnd) {
      hnd->status = NT_STATUS_INVALID_HANDLE;
      return CAC_FAILURE;
   }

   group_hnd_out = talloc(mem_ctx, POLICY_HND);
   if(!group_hnd_out) {
      hnd->status = NT_STATUS_NO_MEMORY;
      return CAC_FAILURE;
   }

   hnd->status = rpccli_samr_create_dom_group( pipe_hnd, mem_ctx, op->in.dom_hnd, op->in.name, op->in.access, group_hnd_out);

   if(!NT_STATUS_IS_OK(hnd->status))
      return CAC_FAILURE;

   op->out.group_hnd = group_hnd_out;

   return CAC_SUCCESS;

}

int cac_SamDeleteGroup(CacServerHandle *hnd, TALLOC_CTX *mem_ctx, POLICY_HND *group_hnd) {
   struct rpc_pipe_client *pipe_hnd = NULL;

   if(!hnd) 
      return CAC_FAILURE;

   if(!hnd->_internal.ctx || !hnd->_internal.pipes[PI_SAMR]) {
      hnd->status = NT_STATUS_INVALID_HANDLE;
      return CAC_FAILURE;
   }

   if(!group_hnd || !mem_ctx) {
      hnd->status = NT_STATUS_INVALID_PARAMETER;
      return CAC_FAILURE;
   }

   pipe_hnd = cac_GetPipe(hnd, PI_SAMR);
   if(!pipe_hnd) {
      hnd->status = NT_STATUS_INVALID_HANDLE;
      return CAC_FAILURE;
   }

   hnd->status = rpccli_samr_delete_dom_group( pipe_hnd, mem_ctx, group_hnd);

   if(!NT_STATUS_IS_OK(hnd->status))
      return CAC_FAILURE;

   return CAC_SUCCESS;

}

int cac_SamGetGroupMembers(CacServerHandle *hnd, TALLOC_CTX *mem_ctx, struct SamGetGroupMembers *op) {
   struct rpc_pipe_client *pipe_hnd = NULL;

   uint32 num_mem_out;
   uint32 *rids_out;
   uint32 *attr_out;

   if(!hnd) 
      return CAC_FAILURE;

   if(!hnd->_internal.ctx || !hnd->_internal.pipes[PI_SAMR]) {
      hnd->status = NT_STATUS_INVALID_HANDLE;
      return CAC_FAILURE;
   }

   if(!op || !op->in.group_hnd || !mem_ctx) {
      hnd->status = NT_STATUS_INVALID_PARAMETER;
      return CAC_FAILURE;
   }

   pipe_hnd = cac_GetPipe(hnd, PI_SAMR);
   if(!pipe_hnd) {
      hnd->status = NT_STATUS_INVALID_HANDLE;
      return CAC_FAILURE;
   }

   hnd->status = rpccli_samr_query_groupmem( pipe_hnd, mem_ctx, op->in.group_hnd, &num_mem_out, &rids_out, &attr_out);

   if(!NT_STATUS_IS_OK(hnd->status))
      return CAC_FAILURE;

   op->out.num_members = num_mem_out;
   op->out.rids        = rids_out;
   op->out.attributes  = attr_out;

   return CAC_SUCCESS;
}


int cac_SamAddGroupMember(CacServerHandle *hnd, TALLOC_CTX *mem_ctx, struct SamAddGroupMember *op) {
   struct rpc_pipe_client *pipe_hnd = NULL;

   if(!hnd) 
      return CAC_FAILURE;

   if(!hnd->_internal.ctx || !hnd->_internal.pipes[PI_SAMR]) {
      hnd->status = NT_STATUS_INVALID_HANDLE;
      return CAC_FAILURE;
   }

   if(!op || !op->in.group_hnd || op->in.rid == 0 || !mem_ctx) {
      hnd->status = NT_STATUS_INVALID_PARAMETER;
      return CAC_FAILURE;
   }

   pipe_hnd = cac_GetPipe(hnd, PI_SAMR);
   if(!pipe_hnd) {
      hnd->status = NT_STATUS_INVALID_HANDLE;
      return CAC_FAILURE;
   }

   hnd->status = rpccli_samr_add_groupmem( pipe_hnd, mem_ctx, op->in.group_hnd, op->in.rid);

   if(!NT_STATUS_IS_OK(hnd->status))
      return CAC_FAILURE;

   return CAC_SUCCESS;
}

int cac_SamRemoveGroupMember(CacServerHandle *hnd, TALLOC_CTX *mem_ctx, struct SamRemoveGroupMember *op) {
   struct rpc_pipe_client *pipe_hnd = NULL;

   if(!hnd) 
      return CAC_FAILURE;

   if(!hnd->_internal.ctx || !hnd->_internal.pipes[PI_SAMR]) {
      hnd->status = NT_STATUS_INVALID_HANDLE;
      return CAC_FAILURE;
   }

   if(!op || !op->in.group_hnd || op->in.rid == 0 || !mem_ctx) {
      hnd->status = NT_STATUS_INVALID_PARAMETER;
      return CAC_FAILURE;
   }

   pipe_hnd = cac_GetPipe(hnd, PI_SAMR);
   if(!pipe_hnd) {
      hnd->status = NT_STATUS_INVALID_HANDLE;
      return CAC_FAILURE;
   }

   hnd->status = rpccli_samr_del_groupmem( pipe_hnd, mem_ctx, op->in.group_hnd, op->in.rid);

   if(!NT_STATUS_IS_OK(hnd->status))
      return CAC_FAILURE;

   return CAC_SUCCESS;
}

int cac_SamClearGroupMembers(CacServerHandle *hnd, TALLOC_CTX *mem_ctx, POLICY_HND *group_hnd) {
   struct rpc_pipe_client *pipe_hnd = NULL;

   int result = CAC_SUCCESS;

   int i = 0;

   uint32 num_mem = 0;
   uint32 *rid    = NULL;
   uint32 *attr   = NULL;

   NTSTATUS status;

   if(!hnd) 
      return CAC_FAILURE;

   if(!hnd->_internal.ctx || !hnd->_internal.pipes[PI_SAMR]) {
      hnd->status = NT_STATUS_INVALID_HANDLE;
      return CAC_FAILURE;
   }

   if(!group_hnd || !mem_ctx) {
      hnd->status = NT_STATUS_INVALID_PARAMETER;
      return CAC_FAILURE;
   }

   pipe_hnd = cac_GetPipe(hnd, PI_SAMR);
   if(!pipe_hnd) {
      hnd->status = NT_STATUS_INVALID_HANDLE;
      return CAC_FAILURE;
   }

   hnd->status = rpccli_samr_query_groupmem(pipe_hnd, mem_ctx, group_hnd, &num_mem, &rid, &attr);
   
   if(!NT_STATUS_IS_OK(hnd->status))
      return CAC_FAILURE;

   /*try to delete the users one by one*/
   for(i = 0; i < num_mem && NT_STATUS_IS_OK(hnd->status); i++) {
      hnd->status = rpccli_samr_del_groupmem(pipe_hnd, mem_ctx, group_hnd, rid[i]);
   }

   /*if not all members could be removed, then try to re-add the members that were already deleted*/
   if(!NT_STATUS_IS_OK(hnd->status)) {
      status = NT_STATUS_OK;

      for(i -= 1; i >= 0 && NT_STATUS_IS_OK(status); i--) {
         status = rpccli_samr_add_groupmem( pipe_hnd, mem_ctx, group_hnd, rid[i]);
      }

      /*we return with the NTSTATUS error that we got when trying to delete users*/
      if(!NT_STATUS_IS_OK(status))
         result = CAC_FAILURE;
   }

   TALLOC_FREE(attr);

   return result;
}

int cac_SamSetGroupMembers(CacServerHandle *hnd, TALLOC_CTX *mem_ctx, struct SamSetGroupMembers *op) {
   struct rpc_pipe_client *pipe_hnd = NULL;

   uint32 i = 0;
   
   if(!hnd) 
      return CAC_FAILURE;

   if(!hnd->_internal.ctx || !hnd->_internal.pipes[PI_SAMR]) {
      hnd->status = NT_STATUS_INVALID_HANDLE;
      return CAC_FAILURE;
   }

   if(!op || !op->in.group_hnd || !mem_ctx) {
      hnd->status = NT_STATUS_INVALID_PARAMETER;
      return CAC_FAILURE;
   }

   pipe_hnd = cac_GetPipe(hnd, PI_SAMR);
   if(!pipe_hnd) {
      hnd->status = NT_STATUS_INVALID_HANDLE;
      return CAC_FAILURE;
   }

   /*use cac_SamClearGroupMembers() to clear them*/
   if(!cac_SamClearGroupMembers(hnd, mem_ctx, op->in.group_hnd))
      return CAC_FAILURE; /*hnd->status is already set*/


   for(i = 0; i < op->in.num_members && NT_STATUS_IS_OK(hnd->status); i++) {
      hnd->status = rpccli_samr_add_groupmem( pipe_hnd, mem_ctx, op->in.group_hnd, op->in.rids[i]);
   }

   if(!NT_STATUS_IS_OK(hnd->status))
      return CAC_FAILURE;

   return CAC_SUCCESS;

}

int cac_SamEnumGroups(CacServerHandle *hnd, TALLOC_CTX *mem_ctx, struct SamEnumGroups *op) {
   struct rpc_pipe_client *pipe_hnd = NULL;

   uint32 i              = 0;

   uint32 resume_idx_out = 0;
   char **names_out      = NULL;
   char **desc_out       = NULL;
   uint32 *rids_out      = NULL;
   uint32 num_groups_out = 0;

   struct acct_info *acct_buf = NULL;

   if(!hnd) 
      return CAC_FAILURE;

   if(!hnd->_internal.ctx || !hnd->_internal.pipes[PI_SAMR]) {
      hnd->status = NT_STATUS_INVALID_HANDLE;
      return CAC_FAILURE;
   }

   if(!op || !op->in.dom_hnd || !mem_ctx) {
      hnd->status = NT_STATUS_INVALID_PARAMETER;
      return CAC_FAILURE;
   }

   /*using this BOOL is the only reliable way to know that we are done*/
   if(op->out.done == True) /*we return failure so the call will break out of a loop*/
      return CAC_FAILURE;

   pipe_hnd = cac_GetPipe(hnd, PI_SAMR);
   if(!pipe_hnd) {
      hnd->status = NT_STATUS_INVALID_HANDLE;
      return CAC_FAILURE;
   }

   resume_idx_out = op->out.resume_idx;

   hnd->status = rpccli_samr_enum_dom_groups( pipe_hnd, mem_ctx, op->in.dom_hnd, &resume_idx_out, SAMR_ENUM_MAX_SIZE, 
                                                &acct_buf, &num_groups_out);


   if(NT_STATUS_IS_OK(hnd->status)) {
      op->out.done = True;
   }
   else if(NT_STATUS_V(hnd->status) != NT_STATUS_V(STATUS_MORE_ENTRIES)) {
      /*if there are no more entries, the operation will return NT_STATUS_OK. 
       * We want to return failure if no results were returned*/
      return CAC_FAILURE;
   }

   names_out = talloc_array(mem_ctx, char *, num_groups_out);
   if(!names_out) {
      hnd->status = NT_STATUS_NO_MEMORY;
      TALLOC_FREE(acct_buf);
      return CAC_FAILURE;
   }

   desc_out = talloc_array(mem_ctx, char *, num_groups_out);
   if(!desc_out) {
      hnd->status = NT_STATUS_NO_MEMORY;
      TALLOC_FREE(acct_buf);
      TALLOC_FREE(names_out);
      return CAC_FAILURE;
   }

   rids_out = talloc_array(mem_ctx, uint32, num_groups_out);
   if(!rids_out) {
      hnd->status = NT_STATUS_NO_MEMORY;
      TALLOC_FREE(acct_buf);
      TALLOC_FREE(names_out);
      TALLOC_FREE(desc_out);
      return CAC_FAILURE;
   }

   for(i = 0; i < num_groups_out; i++) {
      names_out[i] = talloc_strdup(mem_ctx, acct_buf[i].acct_name);
      desc_out[i]  = talloc_strdup(mem_ctx, acct_buf[i].acct_desc);
      rids_out[i]  = acct_buf[i].rid;

      if(!names_out[i] || !desc_out[i]) {
         hnd->status = NT_STATUS_NO_MEMORY;
         return CAC_FAILURE;
      }
   }

   op->out.resume_idx   = resume_idx_out;
   op->out.num_groups   = num_groups_out;
   op->out.rids         = rids_out;
   op->out.names        = names_out;
   op->out.descriptions = desc_out;

   return CAC_SUCCESS;
}

int cac_SamEnumAliases(CacServerHandle *hnd, TALLOC_CTX *mem_ctx, struct SamEnumAliases *op) {
   struct rpc_pipe_client *pipe_hnd = NULL;

   uint32 i              = 0;

   uint32 resume_idx_out = 0;
   char **names_out      = NULL;
   char **desc_out       = NULL;
   uint32 *rids_out      = NULL;
   uint32 num_als_out    = 0;

   struct acct_info *acct_buf = NULL;

   if(!hnd) 
      return CAC_FAILURE;

   if(!hnd->_internal.ctx || !hnd->_internal.pipes[PI_SAMR]) {
      hnd->status = NT_STATUS_INVALID_HANDLE;
      return CAC_FAILURE;
   }

   if(!op || !op->in.dom_hnd || !mem_ctx) {
      hnd->status = NT_STATUS_INVALID_PARAMETER;
      return CAC_FAILURE;
   }

   /*this is a hack.. but is the only reliable way to know if everything has been enumerated*/
   if(op->out.done == True) {
      return CAC_FAILURE;
   }

   pipe_hnd = cac_GetPipe(hnd, PI_SAMR);
   if(!pipe_hnd) {
      hnd->status = NT_STATUS_INVALID_HANDLE;
      return CAC_FAILURE;
   }

   resume_idx_out = op->out.resume_idx;

   hnd->status = rpccli_samr_enum_als_groups( pipe_hnd, mem_ctx, op->in.dom_hnd, &resume_idx_out, SAMR_ENUM_MAX_SIZE, 
                                               &acct_buf, &num_als_out);


   if(NT_STATUS_IS_OK(hnd->status))
      op->out.done = True;

   /*if there are no more entries, the operation will return NT_STATUS_OK. 
    * We want to return failure if no results were returned*/
   if(!NT_STATUS_IS_OK(hnd->status) && NT_STATUS_V(hnd->status) != NT_STATUS_V(STATUS_MORE_ENTRIES))
      return CAC_FAILURE;

   names_out = talloc_array(mem_ctx, char *, num_als_out);
   if(!names_out) {
      hnd->status = NT_STATUS_NO_MEMORY;
      TALLOC_FREE(acct_buf);
      return CAC_FAILURE;
   }

   desc_out = talloc_array(mem_ctx, char *, num_als_out);
   if(!desc_out) {
      hnd->status = NT_STATUS_NO_MEMORY;
      TALLOC_FREE(acct_buf);
      TALLOC_FREE(names_out);
      return CAC_FAILURE;
   }

   rids_out = talloc_array(mem_ctx, uint32, num_als_out);
   if(!rids_out) {
      hnd->status = NT_STATUS_NO_MEMORY;
      TALLOC_FREE(acct_buf);
      TALLOC_FREE(names_out);
      TALLOC_FREE(desc_out);
      return CAC_FAILURE;
   }

   for(i = 0; i < num_als_out; i++) {
      names_out[i] = talloc_strdup(mem_ctx, acct_buf[i].acct_name);
      desc_out[i]  = talloc_strdup(mem_ctx, acct_buf[i].acct_desc);
      rids_out[i]  = acct_buf[i].rid;

      if(!names_out[i] || !desc_out[i]) {
         hnd->status = NT_STATUS_NO_MEMORY;
         return CAC_FAILURE;
      }
   }

   op->out.resume_idx   = resume_idx_out;
   op->out.num_aliases  = num_als_out;
   op->out.rids         = rids_out;
   op->out.names        = names_out;
   op->out.descriptions = desc_out;

   return CAC_SUCCESS;
}

int cac_SamCreateAlias(CacServerHandle *hnd, TALLOC_CTX *mem_ctx, struct SamCreateAlias *op) {
   struct rpc_pipe_client *pipe_hnd = NULL;

   POLICY_HND *als_hnd_out = NULL;

   if(!hnd) 
      return CAC_FAILURE;

   if(!hnd->_internal.ctx || !hnd->_internal.pipes[PI_SAMR]) {
      hnd->status = NT_STATUS_INVALID_HANDLE;
      return CAC_FAILURE;
   }

   if(!op || !op->in.name || op->in.name[0] == '\0' || !mem_ctx) {
      hnd->status = NT_STATUS_INVALID_PARAMETER;
      return CAC_FAILURE;
   }

   pipe_hnd = cac_GetPipe(hnd, PI_SAMR);
   if(!pipe_hnd) {
      hnd->status = NT_STATUS_INVALID_HANDLE;
      return CAC_FAILURE;
   }

   als_hnd_out = talloc(mem_ctx, POLICY_HND);
   if(!als_hnd_out) {
      hnd->status = NT_STATUS_NO_MEMORY;
      return CAC_FAILURE;
   }

   hnd->status = rpccli_samr_create_dom_alias( pipe_hnd, mem_ctx, op->in.dom_hnd, op->in.name, als_hnd_out);

   if(!NT_STATUS_IS_OK(hnd->status))
      return CAC_FAILURE;

   op->out.alias_hnd = als_hnd_out;

   return CAC_SUCCESS;

}

int cac_SamOpenAlias(CacServerHandle *hnd, TALLOC_CTX *mem_ctx, struct SamOpenAlias *op) {
   struct rpc_pipe_client *pipe_hnd = NULL;

   POLICY_HND *als_hnd_out = NULL;

   if(!hnd) 
      return CAC_FAILURE;

   if(!hnd->_internal.ctx || !hnd->_internal.pipes[PI_SAMR]) {
      hnd->status = NT_STATUS_INVALID_HANDLE;
      return CAC_FAILURE;
   }

   if(!op || op->in.access == 0 || op->in.rid == 0 || !mem_ctx) {
      hnd->status = NT_STATUS_INVALID_PARAMETER;
      return CAC_FAILURE;
   }

   pipe_hnd = cac_GetPipe(hnd, PI_SAMR);
   if(!pipe_hnd) {
      hnd->status = NT_STATUS_INVALID_HANDLE;
      return CAC_FAILURE;
   }

   als_hnd_out = talloc(mem_ctx, POLICY_HND);
   if(!als_hnd_out) {
      hnd->status = NT_STATUS_NO_MEMORY;
      return CAC_FAILURE;
   }

   hnd->status = rpccli_samr_open_alias( pipe_hnd, mem_ctx, op->in.dom_hnd, op->in.access, op->in.rid, als_hnd_out);

   if(!NT_STATUS_IS_OK(hnd->status))
      return CAC_FAILURE;

   op->out.alias_hnd = als_hnd_out;

   return CAC_SUCCESS;
}

int cac_SamDeleteAlias(CacServerHandle *hnd, TALLOC_CTX *mem_ctx, POLICY_HND *alias_hnd) {
   struct rpc_pipe_client *pipe_hnd = NULL;

   if(!hnd) 
      return CAC_FAILURE;

   if(!hnd->_internal.ctx || !hnd->_internal.pipes[PI_SAMR]) {
      hnd->status = NT_STATUS_INVALID_HANDLE;
      return CAC_FAILURE;
   }

   if(!alias_hnd || !mem_ctx) {
      hnd->status = NT_STATUS_INVALID_PARAMETER;
      return CAC_FAILURE;
   }

   pipe_hnd = cac_GetPipe(hnd, PI_SAMR);
   if(!pipe_hnd) {
      hnd->status = NT_STATUS_INVALID_HANDLE;
      return CAC_FAILURE;
   }

   hnd->status = rpccli_samr_delete_dom_alias( pipe_hnd, mem_ctx, alias_hnd);

   if(!NT_STATUS_IS_OK(hnd->status))
      return CAC_FAILURE;

   return CAC_SUCCESS;

}

int cac_SamAddAliasMember(CacServerHandle *hnd, TALLOC_CTX *mem_ctx, struct SamAddAliasMember *op) {
   struct rpc_pipe_client *pipe_hnd = NULL;

   if(!hnd) 
      return CAC_FAILURE;

   if(!hnd->_internal.ctx || !hnd->_internal.pipes[PI_SAMR]) {
      hnd->status = NT_STATUS_INVALID_HANDLE;
      return CAC_FAILURE;
   }

   if(!op || !op->in.alias_hnd || !op->in.sid || !mem_ctx) {
      hnd->status = NT_STATUS_INVALID_PARAMETER;
      return CAC_FAILURE;
   }

   pipe_hnd = cac_GetPipe(hnd, PI_SAMR);
   if(!pipe_hnd) {
      hnd->status = NT_STATUS_INVALID_HANDLE;
      return CAC_FAILURE;
   }

   hnd->status = rpccli_samr_add_aliasmem( pipe_hnd, mem_ctx, op->in.alias_hnd, op->in.sid);

   if(!NT_STATUS_IS_OK(hnd->status))
      return CAC_FAILURE;

   return CAC_SUCCESS;
}

int cac_SamRemoveAliasMember(CacServerHandle *hnd, TALLOC_CTX *mem_ctx, struct SamRemoveAliasMember *op) {
   struct rpc_pipe_client *pipe_hnd = NULL;

   if(!hnd) 
      return CAC_FAILURE;

   if(!hnd->_internal.ctx || !hnd->_internal.pipes[PI_SAMR]) {
      hnd->status = NT_STATUS_INVALID_HANDLE;
      return CAC_FAILURE;
   }

   if(!op || !op->in.alias_hnd || !op->in.sid || !mem_ctx) {
      hnd->status = NT_STATUS_INVALID_PARAMETER;
      return CAC_FAILURE;
   }

   pipe_hnd = cac_GetPipe(hnd, PI_SAMR);
   if(!pipe_hnd) {
      hnd->status = NT_STATUS_INVALID_HANDLE;
      return CAC_FAILURE;
   }

   hnd->status = rpccli_samr_del_aliasmem( pipe_hnd, mem_ctx, op->in.alias_hnd, op->in.sid);

   if(!NT_STATUS_IS_OK(hnd->status))
      return CAC_FAILURE;

   return CAC_SUCCESS;
}

int cac_SamGetAliasMembers(CacServerHandle *hnd, TALLOC_CTX *mem_ctx, struct SamGetAliasMembers *op) {
   struct rpc_pipe_client *pipe_hnd = NULL;

   uint32 num_mem_out;
   DOM_SID *sids_out;

   if(!hnd) 
      return CAC_FAILURE;

   if(!hnd->_internal.ctx || !hnd->_internal.pipes[PI_SAMR]) {
      hnd->status = NT_STATUS_INVALID_HANDLE;
      return CAC_FAILURE;
   }

   if(!op || !op->in.alias_hnd || !mem_ctx) {
      hnd->status = NT_STATUS_INVALID_PARAMETER;
      return CAC_FAILURE;
   }

   pipe_hnd = cac_GetPipe(hnd, PI_SAMR);
   if(!pipe_hnd) {
      hnd->status = NT_STATUS_INVALID_HANDLE;
      return CAC_FAILURE;
   }

   hnd->status = rpccli_samr_query_aliasmem( pipe_hnd, mem_ctx, op->in.alias_hnd, &num_mem_out, &sids_out);

   if(!NT_STATUS_IS_OK(hnd->status))
      return CAC_FAILURE;

   op->out.num_members = num_mem_out;
   op->out.sids        = sids_out;

   return CAC_SUCCESS;
}

int cac_SamClearAliasMembers(CacServerHandle *hnd, TALLOC_CTX *mem_ctx, POLICY_HND *alias_hnd) {
   struct rpc_pipe_client *pipe_hnd = NULL;

   int result = CAC_SUCCESS;

   int i = 0;

   uint32 num_mem = 0;
   DOM_SID *sid   = NULL;

   NTSTATUS status;

   if(!hnd) 
      return CAC_FAILURE;

   if(!hnd->_internal.ctx || !hnd->_internal.pipes[PI_SAMR]) {
      hnd->status = NT_STATUS_INVALID_HANDLE;
      return CAC_FAILURE;
   }

   if(!alias_hnd || !mem_ctx) {
      hnd->status = NT_STATUS_INVALID_PARAMETER;
      return CAC_FAILURE;
   }

   pipe_hnd = cac_GetPipe(hnd, PI_SAMR);
   if(!pipe_hnd) {
      hnd->status = NT_STATUS_INVALID_HANDLE;
      return CAC_FAILURE;
   }

   hnd->status = rpccli_samr_query_aliasmem(pipe_hnd, mem_ctx, alias_hnd, &num_mem, &sid);
   
   if(!NT_STATUS_IS_OK(hnd->status))
      return CAC_FAILURE;

   /*try to delete the users one by one*/
   for(i = 0; i < num_mem && NT_STATUS_IS_OK(hnd->status); i++) {
      hnd->status = rpccli_samr_del_aliasmem(pipe_hnd, mem_ctx, alias_hnd, &sid[i]);
   }

   /*if not all members could be removed, then try to re-add the members that were already deleted*/
   if(!NT_STATUS_IS_OK(hnd->status)) {
      status = NT_STATUS_OK;

      for(i -= 1; i >= 0 && NT_STATUS_IS_OK(status); i--) {
         status = rpccli_samr_add_aliasmem( pipe_hnd, mem_ctx, alias_hnd, &sid[i]);
      }

      /*we return with the NTSTATUS error that we got when trying to delete users*/
      if(!NT_STATUS_IS_OK(status))
         result = CAC_FAILURE;
   }

   TALLOC_FREE(sid);
   return result;
}

int cac_SamSetAliasMembers(CacServerHandle *hnd, TALLOC_CTX *mem_ctx, struct SamSetAliasMembers *op) {
   struct rpc_pipe_client *pipe_hnd = NULL;

   uint32 i = 0;
   
   if(!hnd) 
      return CAC_FAILURE;

   if(!hnd->_internal.ctx || !hnd->_internal.pipes[PI_SAMR]) {
      hnd->status = NT_STATUS_INVALID_HANDLE;
      return CAC_FAILURE;
   }

   if(!op || !op->in.alias_hnd || !mem_ctx) {
      hnd->status = NT_STATUS_INVALID_PARAMETER;
      return CAC_FAILURE;
   }

   pipe_hnd = cac_GetPipe(hnd, PI_SAMR);
   if(!pipe_hnd) {
      hnd->status = NT_STATUS_INVALID_HANDLE;
      return CAC_FAILURE;
   }

   /*use cac_SamClearAliasMembers() to clear them*/
   if(!cac_SamClearAliasMembers(hnd, mem_ctx, op->in.alias_hnd))
      return CAC_FAILURE; /*hnd->status is already set*/


   for(i = 0; i < op->in.num_members && NT_STATUS_IS_OK(hnd->status); i++) {
      hnd->status = rpccli_samr_add_aliasmem( pipe_hnd, mem_ctx, op->in.alias_hnd, &(op->in.sids[i]));
   }

   if(!NT_STATUS_IS_OK(hnd->status))
      return CAC_FAILURE;

   return CAC_SUCCESS;

}

int cac_SamUserChangePasswd(CacServerHandle *hnd, TALLOC_CTX *mem_ctx, struct SamUserChangePasswd *op) {
   SMBCSRV *srv = NULL;
   struct rpc_pipe_client *pipe_hnd = NULL;

   if(!hnd) 
      return CAC_FAILURE;

   if(!hnd->_internal.ctx) {
      hnd->status = NT_STATUS_INVALID_HANDLE;
      return CAC_FAILURE;
   }

   if(!op || !op->in.username || !op->in.password || !op->in.new_password || !mem_ctx) {
      hnd->status = NT_STATUS_INVALID_PARAMETER;
      return CAC_FAILURE;
   }

   srv = cac_GetServer(hnd);
   if(!srv) {
      hnd->status = NT_STATUS_INVALID_CONNECTION;
      return CAC_FAILURE;
   }

   /*open a session on SAMR if we don't have one*/
   if(!hnd->_internal.pipes[PI_SAMR]) {
      if(!(pipe_hnd = cli_rpc_pipe_open_noauth(&srv->cli, PI_SAMR, &(hnd->status)))) {
         return CAC_FAILURE;
      }

      hnd->_internal.pipes[PI_SAMR] = True;
   }

   pipe_hnd = cac_GetPipe(hnd, PI_SAMR);
   if(!pipe_hnd) {
      hnd->status = NT_STATUS_INVALID_HANDLE;
      return CAC_FAILURE;
   }

   hnd->status = rpccli_samr_chgpasswd_user(pipe_hnd, mem_ctx, op->in.username, op->in.new_password, op->in.password);

   if(!NT_STATUS_IS_OK(hnd->status))
      return CAC_FAILURE;

   return CAC_SUCCESS;
}

int cac_SamEnableUser(CacServerHandle *hnd, TALLOC_CTX *mem_ctx, POLICY_HND *user_hnd) {
   SMBCSRV *srv = NULL;
   struct rpc_pipe_client *pipe_hnd = NULL;

   SAM_USERINFO_CTR *ctr;

   if(!hnd) 
      return CAC_FAILURE;

   if(!hnd->_internal.ctx || !hnd->_internal.pipes[PI_SAMR]) {
      hnd->status = NT_STATUS_INVALID_HANDLE;
      return CAC_FAILURE;
   }

   if(!user_hnd || !mem_ctx) {
      hnd->status = NT_STATUS_INVALID_PARAMETER;
      return CAC_FAILURE;
   }

   srv = cac_GetServer(hnd);
   if(!srv) {
      hnd->status = NT_STATUS_INVALID_CONNECTION;
      return CAC_FAILURE;
   }

   pipe_hnd = cac_GetPipe(hnd, PI_SAMR);
   if(!pipe_hnd) {
      hnd->status = NT_STATUS_INVALID_HANDLE;
      return CAC_FAILURE;
   }

   /*info_level = 21 is the only level that I have found to work reliably. It would be nice if user_level = 10 worked.*/
   hnd->status = rpccli_samr_query_userinfo( pipe_hnd, mem_ctx, user_hnd, 0x10, &ctr);

   if(!NT_STATUS_IS_OK(hnd->status))
      return CAC_FAILURE;

   /**check the ACB mask*/
   if((ctr->info.id16->acb_info & ACB_DISABLED) == ACB_DISABLED) {
      /*toggle the disabled bit*/
      ctr->info.id16->acb_info ^= ACB_DISABLED;
   }
   else {
      /*the user is already enabled so just return success*/
      return CAC_SUCCESS;
   }

   /*now set the userinfo*/
   hnd->status = rpccli_samr_set_userinfo2( pipe_hnd, mem_ctx, user_hnd, 0x10, &(srv->cli.user_session_key), ctr);

   /*this will only work properly if we use set_userinfo2 - fail if it is not supported*/
   if(!NT_STATUS_IS_OK(hnd->status))
      return CAC_FAILURE;

   return CAC_SUCCESS;
}

int cac_SamDisableUser(CacServerHandle *hnd, TALLOC_CTX *mem_ctx, POLICY_HND *user_hnd) {
   SMBCSRV *srv = NULL;
   struct rpc_pipe_client *pipe_hnd = NULL;

   SAM_USERINFO_CTR *ctr;

   if(!hnd) 
      return CAC_FAILURE;

   if(!hnd->_internal.ctx || !hnd->_internal.pipes[PI_SAMR]) {
      hnd->status = NT_STATUS_INVALID_HANDLE;
      return CAC_FAILURE;
   }

   if(!user_hnd || !mem_ctx) {
      hnd->status = NT_STATUS_INVALID_PARAMETER;
      return CAC_FAILURE;
   }

   srv = cac_GetServer(hnd);
   if(!srv) {
      hnd->status = NT_STATUS_INVALID_CONNECTION;
      return CAC_FAILURE;
   }

   pipe_hnd = cac_GetPipe(hnd, PI_SAMR);
   if(!pipe_hnd) {
      hnd->status = NT_STATUS_INVALID_HANDLE;
      return CAC_FAILURE;
   }

   hnd->status = rpccli_samr_query_userinfo( pipe_hnd, mem_ctx, user_hnd, 0x10, &ctr);

   if(!NT_STATUS_IS_OK(hnd->status))
      return CAC_FAILURE;

   if((ctr->info.id16->acb_info & ACB_DISABLED) == ACB_DISABLED) {
      /*then the user is already disabled*/
      return CAC_SUCCESS;
   }

   /*toggle the disabled bit*/
   ctr->info.id16->acb_info ^= ACB_DISABLED;

   /*this will only work properly if we use set_userinfo2*/
   hnd->status = rpccli_samr_set_userinfo2( pipe_hnd, mem_ctx, user_hnd, 0x10, &(srv->cli.user_session_key), ctr);

   /*this will only work properly if we use set_userinfo2 fail if it is not supported*/
   if(!NT_STATUS_IS_OK(hnd->status))
      return CAC_FAILURE;

   return CAC_SUCCESS;
}

int cac_SamSetPassword(CacServerHandle *hnd, TALLOC_CTX *mem_ctx, struct SamSetPassword *op) {
   SMBCSRV *srv = NULL;
   struct rpc_pipe_client *pipe_hnd = NULL;

   SAM_USERINFO_CTR ctr;
   SAM_USER_INFO_24 info24;
   uint8 pw[516];

   if(!hnd) 
      return CAC_FAILURE;

   if(!hnd->_internal.ctx || !hnd->_internal.pipes[PI_SAMR]) {
      hnd->status = NT_STATUS_INVALID_HANDLE;
      return CAC_FAILURE;
   }

   if(!op->in.user_hnd || !op->in.password || !mem_ctx) {
      hnd->status = NT_STATUS_INVALID_PARAMETER;
      return CAC_FAILURE;
   }

   srv = cac_GetServer(hnd);
   if(!srv) {
      hnd->status = NT_STATUS_INVALID_CONNECTION;
      return CAC_FAILURE;
   }

   pipe_hnd = cac_GetPipe(hnd, PI_SAMR);
   if(!pipe_hnd) {
      hnd->status = NT_STATUS_INVALID_HANDLE;
      return CAC_FAILURE;
   }

   ZERO_STRUCT(ctr);
   ZERO_STRUCT(info24);

   encode_pw_buffer(pw, op->in.password, STR_UNICODE);

   init_sam_user_info24(&info24, (char *)pw, 24);

   ctr.switch_value = 24;
   ctr.info.id24 = &info24;

   hnd->status = rpccli_samr_set_userinfo( pipe_hnd, mem_ctx, op->in.user_hnd, 24, &(srv->cli.user_session_key), &ctr);

   if(!NT_STATUS_IS_OK(hnd->status))
      return CAC_FAILURE;

   return CAC_SUCCESS;
}

int cac_SamGetUserInfo(CacServerHandle *hnd, TALLOC_CTX *mem_ctx, struct SamGetUserInfo *op) {
   struct rpc_pipe_client *pipe_hnd = NULL;

   SAM_USERINFO_CTR *ctr;

   if(!hnd) 
      return CAC_FAILURE;

   if(!hnd->_internal.ctx || !hnd->_internal.pipes[PI_SAMR]) {
      hnd->status = NT_STATUS_INVALID_HANDLE;
      return CAC_FAILURE;
   }

   if(!op->in.user_hnd || !mem_ctx) {
      hnd->status = NT_STATUS_INVALID_PARAMETER;
      return CAC_FAILURE;
   }

   pipe_hnd = cac_GetPipe(hnd, PI_SAMR);
   if(!pipe_hnd) {
      hnd->status = NT_STATUS_INVALID_HANDLE;
      return CAC_FAILURE;
   }

   hnd->status = rpccli_samr_query_userinfo( pipe_hnd, mem_ctx, op->in.user_hnd, 21, &ctr);

   if(!NT_STATUS_IS_OK(hnd->status))
      return CAC_FAILURE;

   op->out.info = cac_MakeUserInfo(mem_ctx, ctr);

   if(!op->out.info) {
      hnd->status = NT_STATUS_NO_MEMORY;
      return CAC_FAILURE;
   }

   return CAC_SUCCESS;
}

int cac_SamSetUserInfo(CacServerHandle *hnd, TALLOC_CTX *mem_ctx, struct SamSetUserInfo *op) {
   SMBCSRV *srv = NULL;
   struct rpc_pipe_client *pipe_hnd = NULL;

   SAM_USERINFO_CTR *ctr;

   if(!hnd) 
      return CAC_FAILURE;

   if(!hnd->_internal.ctx || !hnd->_internal.pipes[PI_SAMR]) {
      hnd->status = NT_STATUS_INVALID_HANDLE;
      return CAC_FAILURE;
   }

   if(!op->in.user_hnd || !op->in.info || !mem_ctx) {
      hnd->status = NT_STATUS_INVALID_PARAMETER;
      return CAC_FAILURE;
   }

   ctr = cac_MakeUserInfoCtr(mem_ctx, op->in.info);
   if(!ctr) {
      hnd->status = NT_STATUS_NO_MEMORY;
      return CAC_FAILURE;
   }

   srv = cac_GetServer(hnd);
   if(!srv) {
      hnd->status = NT_STATUS_INVALID_CONNECTION;
      return CAC_FAILURE;
   }

   pipe_hnd = cac_GetPipe(hnd, PI_SAMR);
   if(!pipe_hnd) {
      hnd->status = NT_STATUS_INVALID_HANDLE;
      return CAC_FAILURE;
   }

   if(hnd->_internal.srv_level >= SRV_WIN_NT4) {
      hnd->status = rpccli_samr_set_userinfo2( pipe_hnd, mem_ctx, op->in.user_hnd, 21, &(srv->cli.user_session_key), ctr);
   }

   if(hnd->_internal.srv_level < SRV_WIN_NT4 || !NT_STATUS_IS_OK(hnd->status)) {
      hnd->status = rpccli_samr_set_userinfo( pipe_hnd, mem_ctx, op->in.user_hnd, 21, &(srv->cli.user_session_key), ctr);

      if(NT_STATUS_IS_OK(hnd->status) && hnd->_internal.srv_level > SRV_WIN_NT4) {
         hnd->_internal.srv_level = SRV_WIN_NT4;
      }
   }


   if(!NT_STATUS_IS_OK(hnd->status))
      return CAC_FAILURE;

   return CAC_SUCCESS;
}


int cac_SamGetUserInfoCtr(CacServerHandle *hnd, TALLOC_CTX *mem_ctx, struct SamGetUserInfoCtr *op) {
   struct rpc_pipe_client *pipe_hnd = NULL;

   SAM_USERINFO_CTR *ctr_out;

   if(!hnd) 
      return CAC_FAILURE;

   if(!hnd->_internal.ctx || !hnd->_internal.pipes[PI_SAMR]) {
      hnd->status = NT_STATUS_INVALID_HANDLE;
      return CAC_FAILURE;
   }

   if(!op->in.user_hnd || op->in.info_class == 0 || !mem_ctx) {
      hnd->status = NT_STATUS_INVALID_PARAMETER;
      return CAC_FAILURE;
   }

   pipe_hnd = cac_GetPipe(hnd, PI_SAMR);
   if(!pipe_hnd) {
      hnd->status = NT_STATUS_INVALID_HANDLE;
      return CAC_FAILURE;
   }

   hnd->status = rpccli_samr_query_userinfo( pipe_hnd, mem_ctx, op->in.user_hnd, op->in.info_class, &ctr_out);

   if(!NT_STATUS_IS_OK(hnd->status)) 
      return CAC_FAILURE;

   op->out.ctr = ctr_out;

   return CAC_SUCCESS;
}

int cac_SamSetUserInfoCtr(CacServerHandle *hnd, TALLOC_CTX *mem_ctx, struct SamSetUserInfoCtr *op) {
   SMBCSRV *srv = NULL;
   struct rpc_pipe_client *pipe_hnd = NULL;

   if(!hnd) 
      return CAC_FAILURE;

   if(!hnd->_internal.ctx || !hnd->_internal.pipes[PI_SAMR]) {
      hnd->status = NT_STATUS_INVALID_HANDLE;
      return CAC_FAILURE;
   }

   if(!op->in.user_hnd || !op->in.ctr || !mem_ctx) {
      hnd->status = NT_STATUS_INVALID_PARAMETER;
      return CAC_FAILURE;
   }

   srv = cac_GetServer(hnd);
   if(!srv) {
      hnd->status = NT_STATUS_INVALID_CONNECTION;
      return CAC_FAILURE;
   }

   pipe_hnd = cac_GetPipe(hnd, PI_SAMR);
   if(!pipe_hnd) {
      hnd->status = NT_STATUS_INVALID_HANDLE;
      return CAC_FAILURE;
   }


   hnd->status = rpccli_samr_set_userinfo( pipe_hnd, mem_ctx, op->in.user_hnd, op->in.ctr->switch_value, &(srv->cli.user_session_key), op->in.ctr);

   if(!NT_STATUS_IS_OK(hnd->status))
      return CAC_FAILURE;

   return CAC_SUCCESS;

}

int cac_SamRenameUser(CacServerHandle *hnd, TALLOC_CTX *mem_ctx, struct SamRenameUser *op) {
   SMBCSRV *srv = NULL;
   struct rpc_pipe_client *pipe_hnd = NULL;

   SAM_USERINFO_CTR ctr;
   SAM_USER_INFO_7 info7;

   if(!hnd) 
      return CAC_FAILURE;

   if(!hnd->_internal.ctx || !hnd->_internal.pipes[PI_SAMR]) {
      hnd->status = NT_STATUS_INVALID_HANDLE;
      return CAC_FAILURE;
   }

   if(!op->in.user_hnd || !op->in.new_name || op->in.new_name[0] == '\0' || !mem_ctx) {
      hnd->status = NT_STATUS_INVALID_PARAMETER;
      return CAC_FAILURE;
   }

   srv = cac_GetServer(hnd);
   if(!srv) {
      hnd->status = NT_STATUS_INVALID_CONNECTION;
      return CAC_FAILURE;
   }

   pipe_hnd = cac_GetPipe(hnd, PI_SAMR);
   if(!pipe_hnd) {
      hnd->status = NT_STATUS_INVALID_HANDLE;
      return CAC_FAILURE;
   }
   
   ZERO_STRUCT(ctr);
   ZERO_STRUCT(info7);

   init_sam_user_info7(&info7, op->in.new_name);
   
   ctr.switch_value = 7;
   ctr.info.id7 = &info7;

   hnd->status = rpccli_samr_set_userinfo( pipe_hnd, mem_ctx, op->in.user_hnd, 7, &(srv->cli.user_session_key), &ctr);

   if(!NT_STATUS_IS_OK(hnd->status))
      return CAC_FAILURE;

   return CAC_SUCCESS;
}


int cac_SamGetGroupInfo(CacServerHandle *hnd, TALLOC_CTX *mem_ctx, struct SamGetGroupInfo *op) {
   struct rpc_pipe_client *pipe_hnd = NULL;

   GROUP_INFO_CTR *ctr;

   if(!hnd) 
      return CAC_FAILURE;

   if(!hnd->_internal.ctx || !hnd->_internal.pipes[PI_SAMR]) {
      hnd->status = NT_STATUS_INVALID_HANDLE;
      return CAC_FAILURE;
   }

   if(!op->in.group_hnd || !mem_ctx) {
      hnd->status = NT_STATUS_INVALID_PARAMETER;
      return CAC_FAILURE;
   }

   pipe_hnd = cac_GetPipe(hnd, PI_SAMR);
   if(!pipe_hnd) {
      hnd->status = NT_STATUS_INVALID_HANDLE;
      return CAC_FAILURE;
   }


   /*get a GROUP_INFO_1 structure*/
   hnd->status = rpccli_samr_query_groupinfo( pipe_hnd, mem_ctx, op->in.group_hnd, 1, &ctr);

   if(!NT_STATUS_IS_OK(hnd->status))
      return CAC_FAILURE;

   op->out.info = cac_MakeGroupInfo(mem_ctx, ctr);
   if(!op->out.info) {
      hnd->status = NT_STATUS_NO_MEMORY;
      return CAC_FAILURE;
   }

   return CAC_SUCCESS;
}

int cac_SamSetGroupInfo(CacServerHandle *hnd, TALLOC_CTX *mem_ctx, struct SamSetGroupInfo *op) {
   struct rpc_pipe_client *pipe_hnd = NULL;

   GROUP_INFO_CTR *ctr = NULL;

   if(!hnd) 
      return CAC_FAILURE;

   if(!hnd->_internal.ctx || !hnd->_internal.pipes[PI_SAMR]) {
      hnd->status = NT_STATUS_INVALID_HANDLE;
      return CAC_FAILURE;
   }

   if(!op->in.group_hnd || !op->in.info || !mem_ctx) {
      hnd->status = NT_STATUS_INVALID_PARAMETER;
      return CAC_FAILURE;
   }

   ctr = cac_MakeGroupInfoCtr(mem_ctx, op->in.info);
   if(!ctr) {
      hnd->status = NT_STATUS_NO_MEMORY;
      return CAC_FAILURE;
   }

   pipe_hnd = cac_GetPipe(hnd, PI_SAMR);
   if(!pipe_hnd) {
      hnd->status = NT_STATUS_INVALID_HANDLE;
      return CAC_FAILURE;
   }

   hnd->status = rpccli_samr_set_groupinfo(pipe_hnd, mem_ctx, op->in.group_hnd, ctr);

   if(!NT_STATUS_IS_OK(hnd->status))
      return CAC_FAILURE;

   return CAC_SUCCESS;
}

int cac_SamRenameGroup(CacServerHandle *hnd, TALLOC_CTX *mem_ctx, struct SamRenameGroup *op) {
   struct rpc_pipe_client *pipe_hnd = NULL;

   GROUP_INFO_CTR ctr;

   if(!hnd) 
      return CAC_FAILURE;

   if(!hnd->_internal.ctx || !hnd->_internal.pipes[PI_SAMR]) {
      hnd->status = NT_STATUS_INVALID_HANDLE;
      return CAC_FAILURE;
   }

   if(!op->in.group_hnd || !op->in.new_name || op->in.new_name[0] == '\0' || !mem_ctx) {
      hnd->status = NT_STATUS_INVALID_PARAMETER;
      return CAC_FAILURE;
   }

   pipe_hnd = cac_GetPipe(hnd, PI_SAMR);
   if(!pipe_hnd) {
      hnd->status = NT_STATUS_INVALID_HANDLE;
      return CAC_FAILURE;
   }
   
   ZERO_STRUCT(ctr);

   init_samr_group_info2(&ctr.group.info2, op->in.new_name);
   ctr.switch_value1 = 2;
   
   hnd->status = rpccli_samr_set_groupinfo( pipe_hnd, mem_ctx, op->in.group_hnd, &ctr);

   if(!NT_STATUS_IS_OK(hnd->status))
      return CAC_FAILURE;

   return CAC_SUCCESS;
}

int cac_SamGetAliasInfo(CacServerHandle *hnd, TALLOC_CTX *mem_ctx, struct SamGetAliasInfo *op) {
   struct rpc_pipe_client *pipe_hnd = NULL;

   ALIAS_INFO_CTR ctr;

   if(!hnd) 
      return CAC_FAILURE;

   if(!hnd->_internal.ctx || !hnd->_internal.pipes[PI_SAMR]) {
      hnd->status = NT_STATUS_INVALID_HANDLE;
      return CAC_FAILURE;
   }

   if(!op->in.alias_hnd || !mem_ctx) {
      hnd->status = NT_STATUS_INVALID_PARAMETER;
      return CAC_FAILURE;
   }

   pipe_hnd = cac_GetPipe(hnd, PI_SAMR);
   if(!pipe_hnd) {
      hnd->status = NT_STATUS_INVALID_HANDLE;
      return CAC_FAILURE;
   }

   /*get a GROUP_INFO_1 structure*/
   hnd->status = rpccli_samr_query_alias_info( pipe_hnd, mem_ctx, op->in.alias_hnd, 1, &ctr);

   if(!NT_STATUS_IS_OK(hnd->status))
      return CAC_FAILURE;

   op->out.info = cac_MakeAliasInfo(mem_ctx, ctr);
   if(!op->out.info) {
      hnd->status = NT_STATUS_NO_MEMORY;
      return CAC_FAILURE;
   }

   return CAC_SUCCESS;

}

int cac_SamSetAliasInfo(CacServerHandle *hnd, TALLOC_CTX *mem_ctx, struct SamSetAliasInfo *op) {
   struct rpc_pipe_client *pipe_hnd = NULL;

   ALIAS_INFO_CTR *ctr = NULL;

   if(!hnd) 
      return CAC_FAILURE;

   if(!hnd->_internal.ctx || !hnd->_internal.pipes[PI_SAMR]) {
      hnd->status = NT_STATUS_INVALID_HANDLE;
      return CAC_FAILURE;
   }

   if(!op->in.alias_hnd || !op->in.info || !mem_ctx) {
      hnd->status = NT_STATUS_INVALID_PARAMETER;
      return CAC_FAILURE;
   }

   ctr = cac_MakeAliasInfoCtr(mem_ctx, op->in.info);
   if(!ctr) {
      hnd->status = NT_STATUS_NO_MEMORY;
      return CAC_FAILURE;
   }

   pipe_hnd = cac_GetPipe(hnd, PI_SAMR);
   if(!pipe_hnd) {
      hnd->status = NT_STATUS_INVALID_HANDLE;
      return CAC_FAILURE;
   }

   hnd->status = rpccli_samr_set_aliasinfo(pipe_hnd, mem_ctx, op->in.alias_hnd, ctr);

   if(!NT_STATUS_IS_OK(hnd->status))
      return CAC_FAILURE;

   return CAC_SUCCESS;
}

int cac_SamGetDomainInfo(CacServerHandle *hnd, TALLOC_CTX *mem_ctx, struct SamGetDomainInfo *op) {
   struct rpc_pipe_client *pipe_hnd = NULL;

   SAM_UNK_CTR ctr;
   SAM_UNK_INFO_1 info1;
   SAM_UNK_INFO_2 info2;
   SAM_UNK_INFO_12 info12;

   /*use this to keep track of a failed call*/
   NTSTATUS status_buf = NT_STATUS_OK;

   uint16 fail_count = 0;


   if(!hnd) 
      return CAC_FAILURE;

   if(!hnd->_internal.ctx || !hnd->_internal.pipes[PI_SAMR]) {
      hnd->status = NT_STATUS_INVALID_HANDLE;
      return CAC_FAILURE;
   }

   if(!op->in.dom_hnd || !mem_ctx) {
      hnd->status = NT_STATUS_INVALID_PARAMETER;
      return CAC_FAILURE;
   }

   pipe_hnd = cac_GetPipe(hnd, PI_SAMR);
   if(!pipe_hnd) {
      hnd->status = NT_STATUS_INVALID_HANDLE;
      return CAC_FAILURE;
   }

   /*first try with info 1*/
   hnd->status = rpccli_samr_query_dom_info( pipe_hnd, mem_ctx, op->in.dom_hnd, 1, &ctr);

   if(NT_STATUS_IS_OK(hnd->status)) {
      /*then we buffer the SAM_UNK_INFO_1 structure*/
      info1 = ctr.info.inf1;
   }
   else {
      /*then the call failed, store the status and ZERO out the info structure*/
      ZERO_STRUCT(info1);
      status_buf = hnd->status;
      fail_count++;
   }

   /*try again for the next one*/
   hnd->status = rpccli_samr_query_dom_info( pipe_hnd, mem_ctx, op->in.dom_hnd, 2, &ctr);

   if(NT_STATUS_IS_OK(hnd->status)) {
      /*store the info*/
      info2 = ctr.info.inf2;
   }
   else {
      /*ZERO out the structure and store the bad status*/
      ZERO_STRUCT(info2);
      status_buf = hnd->status;
      fail_count++;
   }

   /*once more*/
   hnd->status = rpccli_samr_query_dom_info( pipe_hnd, mem_ctx, op->in.dom_hnd, 12, &ctr);

   if(NT_STATUS_IS_OK(hnd->status)) {
      info12 = ctr.info.inf12;
   }
   else {
      ZERO_STRUCT(info12);
      status_buf = hnd->status;
      fail_count++;
   }

   /*return failure if all 3 calls failed*/
   if(fail_count == 3)
      return CAC_FAILURE;

   op->out.info = cac_MakeDomainInfo(mem_ctx, &info1, &info2, &info12);

   if(!op->out.info) {
      hnd->status = NT_STATUS_NO_MEMORY;
      return CAC_FAILURE;
   }

   if(fail_count > 0) {
      hnd->status = status_buf;
      return CAC_PARTIAL_SUCCESS;
   }

   return CAC_SUCCESS;
}

int cac_SamGetDomainInfoCtr(CacServerHandle *hnd, TALLOC_CTX *mem_ctx, struct SamGetDomainInfoCtr *op) {
   struct rpc_pipe_client *pipe_hnd = NULL;

   SAM_UNK_CTR *ctr_out;

   if(!hnd) 
      return CAC_FAILURE;

   if(!hnd->_internal.ctx || !hnd->_internal.pipes[PI_SAMR]) {
      hnd->status = NT_STATUS_INVALID_HANDLE;
      return CAC_FAILURE;
   }

   if(!op->in.dom_hnd || op->in.info_class == 0 || !mem_ctx) {
      hnd->status = NT_STATUS_INVALID_PARAMETER;
      return CAC_FAILURE;
   }

   pipe_hnd = cac_GetPipe(hnd, PI_SAMR);
   if(!pipe_hnd) {
      hnd->status = NT_STATUS_INVALID_HANDLE;
      return CAC_FAILURE;
   }

   ctr_out = talloc(mem_ctx, SAM_UNK_CTR);
   if(!ctr_out) {
      hnd->status = NT_STATUS_NO_MEMORY;
      return CAC_FAILURE;
   }

   hnd->status = rpccli_samr_query_dom_info( pipe_hnd, mem_ctx, op->in.dom_hnd, op->in.info_class, ctr_out);

   if(!NT_STATUS_IS_OK(hnd->status))
      return CAC_FAILURE;

   op->out.info = ctr_out;

   return CAC_SUCCESS;
}

int cac_SamGetDisplayInfo(CacServerHandle *hnd, TALLOC_CTX *mem_ctx, struct SamGetDisplayInfo *op) {
   struct rpc_pipe_client *pipe_hnd = NULL;

   SAM_DISPINFO_CTR ctr_out;

   uint32 max_entries_buf = 0;
   uint32 max_size_buf    = 0;

   uint32 resume_idx_out;
   uint32 num_entries_out;

   if(!hnd) 
      return CAC_FAILURE;

   if(!hnd->_internal.ctx || !hnd->_internal.pipes[PI_SAMR]) {
      hnd->status = NT_STATUS_INVALID_HANDLE;
      return CAC_FAILURE;
   }

   if(!op->in.dom_hnd || op->in.info_class == 0 || !mem_ctx) {
      hnd->status = NT_STATUS_INVALID_PARAMETER;
      return CAC_FAILURE;
   }

   if(op->out.done == True) /*this is done so we can use the function as a loop condition*/
      return CAC_FAILURE;

   pipe_hnd = cac_GetPipe(hnd, PI_SAMR);
   if(!pipe_hnd) {
      hnd->status = NT_STATUS_INVALID_HANDLE;
      return CAC_FAILURE;
   }

   if(op->in.max_entries == 0 || op->in.max_size == 0) {
      get_query_dispinfo_params(op->out.loop_count, &max_entries_buf, &max_size_buf);
   }
   else {
      max_entries_buf = op->in.max_entries;
      max_size_buf    = op->in.max_size;
   }

   resume_idx_out = op->out.resume_idx;

   hnd->status = rpccli_samr_query_dispinfo( pipe_hnd, mem_ctx, op->in.dom_hnd, &resume_idx_out, op->in.info_class, 
                                             &num_entries_out, max_entries_buf, max_size_buf, &ctr_out);

   if(!NT_STATUS_IS_OK(hnd->status) && !NT_STATUS_EQUAL(hnd->status, STATUS_MORE_ENTRIES)) {
      /*be defensive, maybe they'll call again without zeroing the struct*/ 
      op->out.loop_count = 0;       
      op->out.resume_idx = 0;
      return CAC_FAILURE;
   }

   if(NT_STATUS_IS_OK(hnd->status)) {
      /*we want to quit once the function is called next. so it can be used in a loop*/
      op->out.done = True;
   }

   op->out.resume_idx  = resume_idx_out;
   op->out.num_entries = num_entries_out;
   op->out.ctr         = ctr_out;
   op->out.loop_count++;

   return CAC_SUCCESS;
}

int cac_SamLookupDomain(CacServerHandle *hnd, TALLOC_CTX *mem_ctx, struct SamLookupDomain *op) {
   struct rpc_pipe_client *pipe_hnd = NULL;

   DOM_SID *sid_out = NULL;
   
   if(!hnd) 
      return CAC_FAILURE;

   if(!hnd->_internal.ctx || !hnd->_internal.pipes[PI_SAMR]) {
      hnd->status = NT_STATUS_INVALID_HANDLE;
      return CAC_FAILURE;
   }

   if(!op->in.sam || !op->in.name || !mem_ctx) {
      hnd->status = NT_STATUS_INVALID_PARAMETER;
      return CAC_FAILURE;
   }

   pipe_hnd = cac_GetPipe(hnd, PI_SAMR);
   if(!pipe_hnd) {
      hnd->status = NT_STATUS_INVALID_HANDLE;
      return CAC_FAILURE;
   }

   sid_out = talloc(mem_ctx, DOM_SID);
   if(!sid_out) {
      hnd->status = NT_STATUS_NO_MEMORY;
      return CAC_FAILURE;
   }

   hnd->status = rpccli_samr_lookup_domain( pipe_hnd, mem_ctx, op->in.sam, op->in.name, sid_out);

   if(!NT_STATUS_IS_OK(hnd->status))
      return CAC_FAILURE;

   op->out.sid = sid_out;

   return CAC_SUCCESS;
}

int cac_SamGetSecurityObject(CacServerHandle *hnd, TALLOC_CTX *mem_ctx, struct SamGetSecurityObject *op) {
   struct rpc_pipe_client *pipe_hnd = NULL;

   /*this number taken from rpcclient/cmd_samr.c, I think it is the only supported level*/
   uint32 sec_info = DACL_SECURITY_INFORMATION;

   SEC_DESC_BUF *sec_out = NULL;

   if(!hnd) 
      return CAC_FAILURE;

   if(!hnd->_internal.ctx || !hnd->_internal.pipes[PI_SAMR]) {
      hnd->status = NT_STATUS_INVALID_HANDLE;
      return CAC_FAILURE;
   }

   if(!op->in.pol || !mem_ctx) {
      hnd->status = NT_STATUS_INVALID_PARAMETER;
      return CAC_FAILURE;
   }

   pipe_hnd = cac_GetPipe(hnd, PI_SAMR);
   if(!pipe_hnd) {
      hnd->status = NT_STATUS_INVALID_HANDLE;
      return CAC_FAILURE;
   }

   hnd->status = rpccli_samr_query_sec_obj(pipe_hnd, mem_ctx, op->in.pol, sec_info, mem_ctx, &sec_out);

   if(!NT_STATUS_IS_OK(hnd->status))
      return CAC_FAILURE;

   op->out.sec = sec_out;

   return CAC_SUCCESS;
}

int cac_SamFlush(CacServerHandle *hnd, TALLOC_CTX *mem_ctx, struct SamFlush *op) {
   struct SamOpenDomain od;

   if(!hnd) 
      return CAC_FAILURE;

   if(!hnd->_internal.ctx || !hnd->_internal.pipes[PI_SAMR]) {
      hnd->status = NT_STATUS_INVALID_HANDLE;
      return CAC_FAILURE;
   }

   if(!op || !op->in.dom_hnd || !mem_ctx) {
      hnd->status = NT_STATUS_INVALID_PARAMETER;
      return CAC_FAILURE;
   }

   if(!cac_SamClose(hnd, mem_ctx, op->in.dom_hnd))
      return CAC_FAILURE;

   ZERO_STRUCT(od);
   od.in.access = (op->in.access) ? op->in.access : MAXIMUM_ALLOWED_ACCESS;
   od.in.sid    = op->in.sid;

   if(!cac_SamOpenDomain(hnd, mem_ctx, &od))
      return CAC_FAILURE;

   /*this function does not use an output parameter to make it as convenient as possible to use*/
   *op->in.dom_hnd = *od.out.dom_hnd;

   TALLOC_FREE(od.out.dom_hnd);

   return CAC_SUCCESS;
}
