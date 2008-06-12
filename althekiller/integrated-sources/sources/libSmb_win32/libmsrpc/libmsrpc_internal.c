/* 
 *  Unix SMB/CIFS implementation.
 *  MS-RPC client internal functions
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

/*used to get a struct rpc_pipe_client* to be passed into rpccli* calls*/
struct rpc_pipe_client *cac_GetPipe(CacServerHandle *hnd, int pi_idx) {
   SMBCSRV *srv = NULL;
   struct rpc_pipe_client *pipe_hnd = NULL;

   if(!hnd)
      return NULL;

   if(hnd->_internal.pipes[pi_idx] == False) {
      hnd->status = NT_STATUS_INVALID_HANDLE;
      return NULL;
   }

   srv = cac_GetServer(hnd);
   if(!srv) {
      hnd->status = NT_STATUS_INVALID_CONNECTION;
      return NULL;
   }

   pipe_hnd = srv->cli.pipe_list;

   while(pipe_hnd != NULL && pipe_hnd->pipe_idx != pi_idx)
      pipe_hnd = pipe_hnd->next;

   return pipe_hnd;
}

/*takes a string like HKEY_LOCAL_MACHINE\HARDWARE\ACPI and returns the reg_type code and then a pointer to the start of the path (HARDWARE)*/
int cac_ParseRegPath(char *path, uint32 *reg_type, char **key_name) {

   if(!path)
      return CAC_FAILURE;

   if(strncmp(path, "HKLM", 4) == 0) {
      *reg_type = HKEY_LOCAL_MACHINE;
      *key_name = (path[4] == '\\') ? path + 5 : NULL;
   }
   else if(strncmp(path, "HKEY_LOCAL_MACHINE", 18) == 0) {
      *reg_type = HKEY_LOCAL_MACHINE;
      *key_name = (path[18] == '\\') ? path + 19 : NULL;
   }
   else if(strncmp(path, "HKCR", 4) == 0) {
      *reg_type = HKEY_CLASSES_ROOT;
      *key_name = (path[4] == '\\') ? path + 5 : NULL;
   }
   else if(strncmp(path, "HKEY_CLASSES_ROOT", 17) == 0) {
      *reg_type = HKEY_CLASSES_ROOT;
      *key_name = (path[17] == '\\') ? path + 18 : NULL;
   }
   else if(strncmp(path, "HKU", 3) == 0) {
      *reg_type = HKEY_USERS;
      *key_name = (path[3] == '\\') ? path + 4 : NULL;
   }
   else if(strncmp(path, "HKEY_USERS", 10) == 0) {
      *reg_type = HKEY_USERS;
      *key_name = (path[10] == '\\') ? path + 11 : NULL;
   }
   else if(strncmp(path, "HKPD", 4) == 0) {
      *reg_type = HKEY_PERFORMANCE_DATA;
      *key_name = (path[4] == '\\') ? path + 5 : NULL;
   }
   else if(strncmp(path, "HKEY_PERFORMANCE_DATA", 21) == 0) {
      *reg_type = HKEY_PERFORMANCE_DATA;
      *key_name = (path[21] == '\\') ? path + 22 : NULL;
   }
   else {
      return CAC_FAILURE;
   }

   return CAC_SUCCESS;
}



RPC_DATA_BLOB *cac_MakeRpcDataBlob(TALLOC_CTX *mem_ctx, uint32 data_type, REG_VALUE_DATA data) {
   RPC_DATA_BLOB *blob = NULL;
   int i;
   uint32 size = 0;
   uint8 *multi = NULL;
   uint32 multi_idx = 0;

   blob = talloc(mem_ctx, RPC_DATA_BLOB);

   if(!blob) {
      errno = ENOMEM;
      return NULL;
   }

   switch(data_type) {
      case REG_SZ:
         init_rpc_blob_str(blob, data.reg_sz, strlen(data.reg_sz ) + 1);
         break;

      case REG_EXPAND_SZ:
         init_rpc_blob_str(blob, data.reg_expand_sz, strlen(data.reg_sz) + 1);
         break;

      case REG_BINARY:
         init_rpc_blob_bytes(blob, data.reg_binary.data, data.reg_binary.data_length);
         break;

      case REG_DWORD:
         init_rpc_blob_uint32(blob, data.reg_dword);
         break;
         
      case REG_DWORD_BE:
         init_rpc_blob_uint32(blob, data.reg_dword_be);
         break;
         
      case REG_MULTI_SZ:
         /*need to find the size*/
         for(i = 0; i < data.reg_multi_sz.num_strings; i++) {
            size += strlen(data.reg_multi_sz.strings[i]) + 1;
         }

         /**need a whole bunch of unicode strings in a row (seperated by null characters), with an extra null-character on the end*/

         multi = TALLOC_ZERO_ARRAY(mem_ctx, uint8, (size + 1)*2); /*size +1 for the extra null character*/
         if(!multi) {
            errno = ENOMEM;
            break;
         }
         
         /*do it using rpcstr_push()*/
         multi_idx = 0;
         for(i = 0; i < data.reg_multi_sz.num_strings; i++) {
            size_t len = strlen(data.reg_multi_sz.strings[i]) + 1;

            rpcstr_push((multi + multi_idx), data.reg_multi_sz.strings[i], len * 2, STR_TERMINATE);

            /* x2 becuase it is a uint8 buffer*/
            multi_idx += len * 2;
         }
            
         /*now initialize the buffer as binary data*/
         init_rpc_blob_bytes(blob, multi, (size + 1)*2);

         break;

      default:
         TALLOC_FREE(blob);
         blob = NULL;
         return NULL;
   }

   if(!(blob->buffer)) {
      TALLOC_FREE(blob);
      return NULL;
   }

   return blob;
}

/*turns a string in a uint16 array to a char array*/
char *cac_unistr_to_str(TALLOC_CTX *mem_ctx, uint16 *src, int num_bytes) {
   char *buf;
   
   int i = 0;

   uint32 str_len = 0;
   
   /*don't allocate more space than we need*/
   while( (str_len) < num_bytes/2 && src[str_len] != 0x0000)
      str_len++;

   /*need room for a '\0'*/
   str_len++;

   buf = talloc_array(mem_ctx, char, str_len);
   if(!buf) {
      return NULL;
   }

   for(i = 0; i < num_bytes/2; i++) {
      buf[i] = ((char *)src)[2*i];
   }

   buf[str_len - 1] = '\0';

   return buf;
}

REG_VALUE_DATA *cac_MakeRegValueData(TALLOC_CTX *mem_ctx, uint32 data_type, REGVAL_BUFFER buf) {
   REG_VALUE_DATA *data;

   uint32 i;

   /*all of the following used for MULTI_SZ data*/
   uint32 size       = 0;
   uint32 len        = 0;
   uint32 multi_idx  = 0;
   uint32 num_strings= 0;
   char **strings    = NULL;

   data = talloc(mem_ctx, REG_VALUE_DATA);
   if(!data) {
      errno = ENOMEM;
      return NULL;
   }

   switch (data_type) {
      case REG_SZ:
         data->reg_sz = cac_unistr_to_str(mem_ctx, buf.buffer, buf.buf_len);
         if(!data->reg_sz) {
            TALLOC_FREE(data);
            errno = ENOMEM;
            data = NULL;
         }
            
         break;

      case REG_EXPAND_SZ:
         data->reg_expand_sz = cac_unistr_to_str(mem_ctx, buf.buffer, buf.buf_len);

         if(!data->reg_expand_sz) {
            TALLOC_FREE(data);
            errno = ENOMEM;
            data = NULL;
         }
            
         break;

      case REG_BINARY:
         size = buf.buf_len;

         data->reg_binary.data_length = size;

         data->reg_binary.data = talloc_memdup(mem_ctx, buf.buffer, size);
         if(!data->reg_binary.data) {
            TALLOC_FREE(data);
            errno = ENOMEM;
            data = NULL;
         }
         break;

      case REG_DWORD:
         data->reg_dword = *((uint32 *)buf.buffer);
         break;

      case REG_DWORD_BE:
         data->reg_dword_be = *((uint32 *)buf.buffer);
         break;

      case REG_MULTI_SZ:
         size = buf.buf_len;

         /*find out how many strings there are. size is # of bytes and we want to work uint16*/
         for(i = 0; i < (size/2 - 1); i++) {
            if(buf.buffer[i] == 0x0000)
               num_strings++;

            /*buffer is suppsed to be terminated with \0\0, but it might not be*/
            if(buf.buffer[i] == 0x0000 && buf.buffer[i + 1] == 0x0000)
               break;
         }
         
         strings = talloc_array(mem_ctx, char *, num_strings);
         if(!strings) {
            errno = ENOMEM;
            TALLOC_FREE(data);
            break;
         }

         if(num_strings == 0) /*then our work here is done*/
            break;

         for(i = 0; i < num_strings; i++) {
            /*find out how many characters are in this string*/
            len = 0;
            /*make sure we don't go past the end of the buffer and keep looping until we have a uni \0*/
            while( multi_idx + len < size/2 && buf.buffer[multi_idx + len] != 0x0000)
               len++;

            /*stay aware of the \0\0*/
            len++;

            strings[i] = TALLOC_ZERO_ARRAY(mem_ctx, char, len);

            /*pull out the unicode string*/
            rpcstr_pull(strings[i], (buf.buffer + multi_idx) , len, -1, STR_TERMINATE);

            /*keep track of where we are in the bigger array*/
            multi_idx += len;
         }

         data->reg_multi_sz.num_strings = num_strings;
         data->reg_multi_sz.strings     = strings;

         break;

      default:
         TALLOC_FREE(data);
         data = NULL;
   }

   return data;
}

SAM_USERINFO_CTR *cac_MakeUserInfoCtr(TALLOC_CTX *mem_ctx, CacUserInfo *info) {
   SAM_USERINFO_CTR *ctr = NULL;

   /*the flags we are 'setting'- include/passdb.h*/
   uint32 flags = ACCT_USERNAME | ACCT_FULL_NAME | ACCT_PRIMARY_GID | ACCT_ADMIN_DESC | ACCT_DESCRIPTION |
                     ACCT_HOME_DIR | ACCT_HOME_DRIVE | ACCT_LOGON_SCRIPT | ACCT_PROFILE | ACCT_WORKSTATIONS |
                      ACCT_FLAGS;

   NTTIME logon_time;
   NTTIME logoff_time;
   NTTIME kickoff_time;
   NTTIME pass_last_set_time;
   NTTIME pass_can_change_time;
   NTTIME pass_must_change_time;

   UNISTR2 user_name;
   UNISTR2 full_name;
   UNISTR2 home_dir;
   UNISTR2 dir_drive;
   UNISTR2 log_scr;
   UNISTR2 prof_path;
   UNISTR2 desc;
   UNISTR2 wkstas;
   UNISTR2 mung_dial;
   UNISTR2 unk;

   ctr = talloc(mem_ctx, SAM_USERINFO_CTR);
   if(!ctr)
      return NULL;

   ZERO_STRUCTP(ctr->info.id23);

   ctr->info.id21 = talloc(mem_ctx, SAM_USER_INFO_21);
   if(!ctr->info.id21)
      return NULL;

   ctr->switch_value = 21;

   ZERO_STRUCTP(ctr->info.id21);

   unix_to_nt_time(&logon_time, info->logon_time);
   unix_to_nt_time(&logoff_time, info->logoff_time);
   unix_to_nt_time(&kickoff_time, info->kickoff_time);
   unix_to_nt_time(&pass_last_set_time, info->pass_last_set_time);
   unix_to_nt_time(&pass_can_change_time, info->pass_can_change_time);
   unix_to_nt_time(&pass_must_change_time, info->pass_must_change_time);

   /*initialize the strings*/
   init_unistr2(&user_name, info->username, UNI_STR_TERMINATE);
   init_unistr2(&full_name, info->full_name, UNI_STR_TERMINATE);
   init_unistr2(&home_dir, info->home_dir, UNI_STR_TERMINATE);
   init_unistr2(&dir_drive, info->home_drive, UNI_STR_TERMINATE);
   init_unistr2(&log_scr, info->logon_script, UNI_STR_TERMINATE);
   init_unistr2(&prof_path, info->profile_path, UNI_STR_TERMINATE);
   init_unistr2(&desc, info->description, UNI_STR_TERMINATE);
   init_unistr2(&wkstas, info->workstations, UNI_STR_TERMINATE);
   init_unistr2(&unk, "\0", UNI_STR_TERMINATE);
   init_unistr2(&mung_dial, info->dial, UNI_STR_TERMINATE);

   /*manually set passmustchange*/
   ctr->info.id21->passmustchange = (info->pass_must_change) ? 0x01 : 0x00;

   init_sam_user_info21W(ctr->info.id21,
                         &logon_time,
                         &logoff_time,
                         &kickoff_time,
                         &pass_last_set_time,
                         &pass_can_change_time,
                         &pass_must_change_time,
                         &user_name,
                         &full_name,
                         &home_dir,
                         &dir_drive,
                         &log_scr,
                         &prof_path,
                         &desc,
                         &wkstas,
                         &unk,
                         &mung_dial,
                         info->lm_password,
                         info->nt_password,
                         info->rid,
                         info->group_rid,
                         info->acb_mask,
                         flags,
                         168, /*logon divs*/
                         info->logon_hours,
                         info->bad_passwd_count,
                         info->logon_count);

   return ctr;
   
}

char *talloc_unistr2_to_ascii(TALLOC_CTX *mem_ctx, UNISTR2 str) {
   char *buf = NULL;

   if(!mem_ctx)
      return NULL;

   buf = talloc_array(mem_ctx, char, (str.uni_str_len + 1));
   if(!buf)
      return NULL;

   unistr2_to_ascii(buf, &str, str.uni_str_len + 1);

   return buf;
}

CacUserInfo *cac_MakeUserInfo(TALLOC_CTX *mem_ctx, SAM_USERINFO_CTR *ctr) {
   CacUserInfo *info = NULL;
   SAM_USER_INFO_21 *id21 = NULL;

   if(!ctr || ctr->switch_value != 21)
      return NULL;

   info = talloc(mem_ctx, CacUserInfo);
   if(!info)
      return NULL;

   id21 = ctr->info.id21;

   ZERO_STRUCTP(info);

   info->logon_time = nt_time_to_unix(&id21->logon_time);
   info->logoff_time = nt_time_to_unix(&id21->logoff_time);
   info->kickoff_time = nt_time_to_unix(&id21->kickoff_time);
   info->pass_last_set_time = nt_time_to_unix(&id21->pass_last_set_time);
   info->pass_can_change_time = nt_time_to_unix(&id21->pass_can_change_time);
   info->pass_must_change_time = nt_time_to_unix(&id21->pass_must_change_time);

   info->username = talloc_unistr2_to_ascii(mem_ctx, id21->uni_user_name);
   if(!info->username)
      return NULL;

   info->full_name = talloc_unistr2_to_ascii(mem_ctx, id21->uni_full_name);
   if(!info->full_name)
      return NULL;
   
   info->home_dir = talloc_unistr2_to_ascii(mem_ctx, id21->uni_home_dir);
   if(!info->home_dir)
      return NULL;
   
   info->home_drive = talloc_unistr2_to_ascii(mem_ctx, id21->uni_dir_drive);
   if(!info->home_drive)
      return NULL;

   info->logon_script = talloc_unistr2_to_ascii(mem_ctx, id21->uni_logon_script);
   if(!info->logon_script)
      return NULL;

   info->profile_path = talloc_unistr2_to_ascii(mem_ctx, id21->uni_profile_path);
   if(!info->profile_path)
      return NULL;
   
   info->description = talloc_unistr2_to_ascii(mem_ctx, id21->uni_acct_desc);
   if(!info->description)
      return NULL;
   
   info->workstations = talloc_unistr2_to_ascii(mem_ctx, id21->uni_workstations);
   if(!info->workstations)
      return NULL;

   info->dial = talloc_unistr2_to_ascii(mem_ctx, id21->uni_munged_dial);
   if(!info->dial)
      return NULL;

   info->rid = id21->user_rid;
   info->group_rid = id21->group_rid;
   info->acb_mask = id21->acb_info;
   info->bad_passwd_count = id21->bad_password_count;
   info->logon_count = id21->logon_count;

   memcpy(info->nt_password, id21->nt_pwd, 8);
   memcpy(info->lm_password, id21->lm_pwd, 8);
   
   info->logon_hours = talloc_memdup(mem_ctx, &(id21->logon_hrs), sizeof(LOGON_HRS));
   if(!info->logon_hours)
      return NULL;

   info->pass_must_change = (id21->passmustchange) ? True : False;

   return info;
}

CacGroupInfo *cac_MakeGroupInfo(TALLOC_CTX *mem_ctx, GROUP_INFO_CTR *ctr) {
   CacGroupInfo *info = NULL;

   if(!mem_ctx || !ctr || ctr->switch_value1 != 1)
      return NULL;

   info = talloc(mem_ctx, CacGroupInfo);
   if(!info)
      return NULL;

   info->name = talloc_unistr2_to_ascii(mem_ctx, ctr->group.info1.uni_acct_name);
   if(!info->name)
      return NULL;

   info->description = talloc_unistr2_to_ascii(mem_ctx, ctr->group.info1.uni_acct_desc);
   if(!info->description)
      return NULL;

   info->num_members = ctr->group.info1.num_members;

   return info;
}

GROUP_INFO_CTR *cac_MakeGroupInfoCtr(TALLOC_CTX *mem_ctx, CacGroupInfo *info) {
   GROUP_INFO_CTR *ctr = NULL;

   if(!mem_ctx || !info)
      return NULL;

   ctr = talloc(mem_ctx, GROUP_INFO_CTR);
   if(!ctr)
      return NULL;

   ctr->switch_value1 = 1;

   init_samr_group_info1(&(ctr->group.info1), info->name, info->description, info->num_members);

   return ctr;
}

CacAliasInfo *cac_MakeAliasInfo(TALLOC_CTX *mem_ctx, ALIAS_INFO_CTR ctr) {
   CacGroupInfo *info = NULL;

   if(!mem_ctx || ctr.level != 1)
      return NULL;

   info = talloc(mem_ctx, CacAliasInfo);
   if(!info)
      return NULL;

   info->name = talloc_unistr2_to_ascii(mem_ctx, *(ctr.alias.info1.name.string));
   if(!info->name)
      return NULL;

   info->description = talloc_unistr2_to_ascii(mem_ctx, *(ctr.alias.info1.description.string));
   if(!info->name)
      return NULL;

   info->num_members = ctr.alias.info1.num_member;

   return info;
}

ALIAS_INFO_CTR *cac_MakeAliasInfoCtr(TALLOC_CTX *mem_ctx, CacAliasInfo *info) {
   ALIAS_INFO_CTR *ctr = NULL;

   if(!mem_ctx || !info)
      return NULL;

   ctr = talloc(mem_ctx, ALIAS_INFO_CTR);
   if(!ctr)
      return NULL;

   ctr->level = 1;

   init_samr_alias_info1(&(ctr->alias.info1), info->name, info->num_members, info->description);

   return ctr;
}

CacDomainInfo *cac_MakeDomainInfo(TALLOC_CTX *mem_ctx, SAM_UNK_INFO_1 *info1, SAM_UNK_INFO_2 *info2, SAM_UNK_INFO_12 *info12) {
   CacDomainInfo *info = NULL;

   if(!mem_ctx || !info1 || !info2 || !info12)
      return NULL;

   info = talloc(mem_ctx, CacDomainInfo);
   if(!info)
      return NULL;

   info->min_pass_length = info1->min_length_password;
   info->pass_history    = info1->password_history;

   cac_InitCacTime(&(info->expire), info1->expire);
   cac_InitCacTime(&(info->min_pass_age), info1->min_passwordage);

   info->server_role       = info2->server_role;
   info->num_users         = info2->num_domain_usrs;
   info->num_domain_groups = info2->num_domain_grps;
   info->num_local_groups  = info2->num_local_grps;

   /*if these have been ZERO'd out we need to know. uni_str_len will be 0*/
   if(info2->uni_comment.uni_str_len == 0) {
      info->comment = talloc_strdup(mem_ctx, "\0");
   }
   else {
      info->comment = talloc_unistr2_to_ascii(mem_ctx, info2->uni_comment);
   }

   if(info2->uni_domain.uni_str_len == 0) {
      info->domain_name = talloc_strdup(mem_ctx, "\0");
   }
   else {
      info->domain_name = talloc_unistr2_to_ascii(mem_ctx, info2->uni_domain);
   }

   if(info2->uni_server.uni_str_len == 0) {
      info->server_name = talloc_strdup(mem_ctx, "\0");
   }
   else {
      info->server_name = talloc_unistr2_to_ascii(mem_ctx, info2->uni_server);
   }


   cac_InitCacTime(&(info->lockout_duration), info12->duration);
   cac_InitCacTime(&(info->lockout_reset), info12->reset_count);
   info->num_bad_attempts = info12->bad_attempt_lockout;

   return info;
}

char *cac_unistr_ascii(TALLOC_CTX *mem_ctx, UNISTR src) {
   char *buf;
   uint32 len;

   if(!mem_ctx || !src.buffer)
      return NULL;

   len = unistrlen(src.buffer) + 1;

   buf = TALLOC_ZERO_ARRAY(mem_ctx, char, len);
   if(!buf)
      return NULL;

   rpcstr_pull(buf, src.buffer, len, -1, STR_TERMINATE);

   return buf;
}

CacService *cac_MakeServiceArray(TALLOC_CTX *mem_ctx, ENUM_SERVICES_STATUS *svc, uint32 num_services) {
   int i;
   CacService *services = NULL;

   if(!mem_ctx || !svc)
      return NULL;

   services = TALLOC_ZERO_ARRAY(mem_ctx, CacService, num_services);
   if(!services)
      return NULL;

   for(i = 0; i < num_services; i++) {
      services[i].service_name = cac_unistr_ascii(mem_ctx, svc[i].servicename);
      services[i].display_name = cac_unistr_ascii(mem_ctx, svc[i].displayname);

      if(!services[i].service_name || !services[i].display_name)
         return NULL;

      services[i].status = svc[i].status;
   }

   return services;
}

int cac_InitCacServiceConfig(TALLOC_CTX *mem_ctx, SERVICE_CONFIG *src, CacServiceConfig *dest) {
   if(!src || !dest)
      return CAC_FAILURE;

   dest->exe_path = talloc_unistr2_to_ascii(mem_ctx, *src->executablepath);
   if(!dest->exe_path)
      return CAC_FAILURE;

   dest->load_order_group = talloc_unistr2_to_ascii(mem_ctx, *src->loadordergroup);
   if(!dest->load_order_group)
      return CAC_FAILURE;

   dest->dependencies = talloc_unistr2_to_ascii(mem_ctx, *src->dependencies);
   if(!dest->dependencies)
      return CAC_FAILURE;

   dest->start_name = talloc_unistr2_to_ascii(mem_ctx, *src->startname);
   if(!dest->start_name)
      return CAC_FAILURE;

   dest->display_name = talloc_unistr2_to_ascii(mem_ctx, *src->displayname);
   if(!dest->display_name)
      return CAC_FAILURE;

   dest->type = src->service_type;
   dest->start_type = src->start_type;
   dest->error_control = src->error_control;
   dest->tag_id = src->tag_id;

   return CAC_SUCCESS;
}
