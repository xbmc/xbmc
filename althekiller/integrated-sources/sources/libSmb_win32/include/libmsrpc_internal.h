/* 
 *  Unix SMB/CIFS implementation.
 *  MS-RPC client internal definitions
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

#ifndef LIBMSRPC_INTERNAL_H
#define LIBMSRPC_INTERNAL_H

#include "libmsrpc.h"

/*definitions*/

struct CacServerHandleInternal {
   /*stores the os type of the server*/
   uint16 srv_level;

   /*stores the initialized/active pipes*/
   BOOL pipes[PI_MAX_PIPES];

   /*underlying smbc context*/
   SMBCCTX *ctx;

   /*did the user supply this SMBCCTX?*/
   BOOL user_supplied_ctx;
};

/*used to get a struct rpc_pipe_client* to be passed into rpccli* calls*/

/*nessecary prototypes*/
BOOL rid_in_list(uint32 rid, uint32 *list, uint32 list_len);

int cac_ParseRegPath(char *path, uint32 *reg_type, char **key_name);

REG_VALUE_DATA *cac_MakeRegValueData(TALLOC_CTX *mem_ctx, uint32 data_type, REGVAL_BUFFER buf);

RPC_DATA_BLOB *cac_MakeRpcDataBlob(TALLOC_CTX *mem_ctx, uint32 data_type, REG_VALUE_DATA data);

SAM_USERINFO_CTR *cac_MakeUserInfoCtr(TALLOC_CTX *mem_ctx, CacUserInfo *info);

CacUserInfo *cac_MakeUserInfo(TALLOC_CTX *mem_ctx, SAM_USERINFO_CTR *ctr);
CacGroupInfo *cac_MakeGroupInfo(TALLOC_CTX *mem_ctx, GROUP_INFO_CTR *ctr);
GROUP_INFO_CTR *cac_MakeGroupInfoCtr(TALLOC_CTX *mem_ctx, CacGroupInfo *info);
CacAliasInfo *cac_MakeAliasInfo(TALLOC_CTX *mem_ctx, ALIAS_INFO_CTR ctr);
ALIAS_INFO_CTR *cac_MakeAliasInfoCtr(TALLOC_CTX *mem_ctx, CacAliasInfo *info);
CacDomainInfo *cac_MakeDomainInfo(TALLOC_CTX *mem_ctx, SAM_UNK_INFO_1 *info1, SAM_UNK_INFO_2 *info2, SAM_UNK_INFO_12 *info12);
CacService *cac_MakeServiceArray(TALLOC_CTX *mem_ctx, ENUM_SERVICES_STATUS *svc, uint32 num_services);
int cac_InitCacServiceConfig(TALLOC_CTX *mem_ctx, SERVICE_CONFIG *src, CacServiceConfig *dest);

/*moved to libmsrpc.h*/
/*struct rpc_pipe_client *cac_GetPipe(CacServerHandle *hnd, int pi_idx);*/

SMBCSRV *smbc_attr_server(SMBCCTX *context,
                          const char *server, const char *share, 
                          fstring workgroup,
                          fstring username, fstring password,
                          POLICY_HND *pol);


#endif /* LIBMSRPC_INTERNAL_H */
