/* 
 *  Unix SMB/CIFS implementation.
 *  MS-RPC client library implementation (WINREG pipe)
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


int cac_RegConnect(CacServerHandle *hnd, TALLOC_CTX *mem_ctx, struct RegConnect *op) {
   SMBCSRV *srv = NULL;
   struct rpc_pipe_client *pipe_hnd = NULL;
   POLICY_HND *key = NULL;
   WERROR err;

   if(!hnd) 
      return CAC_FAILURE;

   if(!hnd->_internal.ctx) {
      hnd->status = NT_STATUS_INVALID_HANDLE;
      return CAC_FAILURE;
   }

   if(!op || !op->in.root || !mem_ctx) {
      hnd->status = NT_STATUS_INVALID_PARAMETER;
      return CAC_FAILURE;
   }

   srv = cac_GetServer(hnd);
   if(!srv) {
      hnd->status = NT_STATUS_INVALID_CONNECTION;
      return CAC_FAILURE;
   }

   /*initialize for winreg pipe if we have to*/
   if(!hnd->_internal.pipes[PI_WINREG]) {
      if(!(pipe_hnd = cli_rpc_pipe_open_noauth(&srv->cli, PI_WINREG, &(hnd->status)))) {
         return CAC_FAILURE;
      }

      hnd->_internal.pipes[PI_WINREG] = True;
   }

   pipe_hnd = cac_GetPipe(hnd, PI_WINREG);
   if(!pipe_hnd) {
      hnd->status = NT_STATUS_INVALID_HANDLE;
      return CAC_FAILURE;
   }

   key = talloc(mem_ctx, POLICY_HND);
   if(!key) {
      hnd->status = NT_STATUS_NO_MEMORY;
      return CAC_FAILURE;
   }

   err = rpccli_reg_connect( pipe_hnd, mem_ctx, op->in.root, op->in.access, key);
   hnd->status = werror_to_ntstatus(err);

   if(!NT_STATUS_IS_OK(hnd->status)) {
      return CAC_FAILURE;
   }

   op->out.key = key;

   return CAC_SUCCESS;
}

int cac_RegClose(CacServerHandle *hnd, TALLOC_CTX *mem_ctx, POLICY_HND *key) {
   struct rpc_pipe_client *pipe_hnd = NULL;
   WERROR err;

   if(!hnd) 
      return CAC_FAILURE;

   if(!hnd->_internal.ctx || !hnd->_internal.pipes[PI_WINREG]) {
      hnd->status = NT_STATUS_INVALID_HANDLE;
      return CAC_FAILURE;
   }

   if(!key || !mem_ctx) {
      hnd->status = NT_STATUS_INVALID_PARAMETER;
      return CAC_FAILURE;
   }

   pipe_hnd = cac_GetPipe(hnd, PI_WINREG);
   if(!pipe_hnd) {
      hnd->status = NT_STATUS_INVALID_HANDLE;
      return CAC_FAILURE;
   }

   err = rpccli_reg_close(pipe_hnd, mem_ctx, key);
   hnd->status = werror_to_ntstatus(err);

   if(!NT_STATUS_IS_OK(hnd->status)) {
      return CAC_FAILURE;
   }

   return CAC_SUCCESS;
}

int cac_RegOpenKey(CacServerHandle *hnd, TALLOC_CTX *mem_ctx, struct RegOpenKey *op) {
   struct rpc_pipe_client *pipe_hnd = NULL;
   WERROR err;

   POLICY_HND *key_out;
   POLICY_HND *parent_key;

   char *key_name = NULL;
   uint32 reg_type = 0;

   struct RegConnect rc;

   if(!hnd) 
      return CAC_FAILURE;

   if(!hnd->_internal.ctx) {
      hnd->status = NT_STATUS_INVALID_HANDLE;
      return CAC_FAILURE;
   }

   if(!op || !op->in.name || !mem_ctx) {
      hnd->status = NT_STATUS_INVALID_PARAMETER;
      return CAC_FAILURE;
   }


   key_out = talloc(mem_ctx, POLICY_HND);
   if(!key_out) {
      hnd->status = NT_STATUS_NO_MEMORY;
      return CAC_FAILURE;
   }

   if(!op->in.parent_key) {
      /*then we need to connect to the registry*/
      if(!cac_ParseRegPath(op->in.name, &reg_type, &key_name)) {
         hnd->status = NT_STATUS_INVALID_PARAMETER;
         return CAC_FAILURE;
      }

      /*use cac_RegConnect because it handles the session setup*/
      ZERO_STRUCT(rc);

      rc.in.access = op->in.access;
      rc.in.root   = reg_type;

      if(!cac_RegConnect(hnd, mem_ctx, &rc)) {
         return CAC_FAILURE;
      }

      /**if they only specified the root key, return the key we just opened*/
      if(key_name == NULL) {
         op->out.key = rc.out.key;
         return CAC_SUCCESS;
      }

      parent_key = rc.out.key;
   }
   else {
      parent_key  = op->in.parent_key;
      key_name    = op->in.name;
   }

   pipe_hnd = cac_GetPipe(hnd, PI_WINREG);
   if(!pipe_hnd) {
      hnd->status = NT_STATUS_INVALID_HANDLE;
      return CAC_FAILURE;
   }

   err = rpccli_reg_open_entry( pipe_hnd, mem_ctx, parent_key, key_name, op->in.access, key_out);
   hnd->status = werror_to_ntstatus(err);

   if(!NT_STATUS_IS_OK(hnd->status)) {
      return CAC_FAILURE;
   }

   if(!op->in.parent_key) {
      /*then close the one that we opened above*/
      err = rpccli_reg_close( pipe_hnd, mem_ctx, parent_key);
      hnd->status = werror_to_ntstatus(err);

      if(!NT_STATUS_IS_OK(hnd->status)) {
         return CAC_FAILURE;
      }
   }

   op->out.key = key_out;

   return CAC_SUCCESS;
}

int cac_RegEnumKeys(CacServerHandle *hnd, TALLOC_CTX *mem_ctx, struct RegEnumKeys *op) {
   struct rpc_pipe_client *pipe_hnd = NULL;
   WERROR err;

   /*buffers for rpccli_reg_enum_key call*/
   fstring key_name_in;
   fstring class_name_in;

   /*output buffers*/
   char **key_names_out   = NULL;
   char **class_names_out = NULL;
   time_t *mod_times_out   = NULL;
   uint32 num_keys_out    = 0;
   uint32 resume_idx      = 0;

   if(!hnd) 
      return CAC_FAILURE;

   /*this is to avoid useless rpc calls, if the last call exhausted all the keys, then we don't need to go through everything again*/
   if(NT_STATUS_V(hnd->status) == NT_STATUS_V(NT_STATUS_GUIDS_EXHAUSTED))
      return CAC_FAILURE;

   if(!hnd->_internal.ctx || !hnd->_internal.pipes[PI_WINREG]) {
      hnd->status = NT_STATUS_INVALID_HANDLE;
      return CAC_FAILURE;
   }

   if(!op || op->in.max_keys == 0 || !mem_ctx) {
      hnd->status = NT_STATUS_INVALID_PARAMETER;
      return CAC_FAILURE;
   }

   pipe_hnd = cac_GetPipe(hnd, PI_WINREG);
   if(!pipe_hnd) {
      hnd->status = NT_STATUS_INVALID_HANDLE;
      return CAC_FAILURE;
   }

   /**the only way to know how many keys to expect is to assume max_keys keys will be found*/
   key_names_out = TALLOC_ARRAY(mem_ctx, char *, op->in.max_keys);
   if(!key_names_out) {
      hnd->status = NT_STATUS_NO_MEMORY;
      return CAC_FAILURE;
   }

   class_names_out = TALLOC_ARRAY(mem_ctx, char *, op->in.max_keys);
   if(!class_names_out) {
      hnd->status = NT_STATUS_NO_MEMORY;
      TALLOC_FREE(key_names_out);
      return CAC_FAILURE;
   }

   mod_times_out = TALLOC_ARRAY(mem_ctx, time_t, op->in.max_keys);
   if(!mod_times_out) {
      hnd->status = NT_STATUS_NO_MEMORY;
      TALLOC_FREE(key_names_out);
      TALLOC_FREE(class_names_out);

      return CAC_FAILURE;
   }
 
   resume_idx = op->out.resume_idx;

   do {
      err = rpccli_reg_enum_key( pipe_hnd, mem_ctx, op->in.key, resume_idx, key_name_in, class_name_in, &mod_times_out[num_keys_out]);
      hnd->status = werror_to_ntstatus(err);

      if(!NT_STATUS_IS_OK(hnd->status)) {
         /*don't increment any values*/
         break;
      }

      key_names_out[num_keys_out] = talloc_strdup(mem_ctx, key_name_in);

      class_names_out[num_keys_out] = talloc_strdup(mem_ctx, class_name_in);

      if(!key_names_out[num_keys_out] || !class_names_out[num_keys_out]) {
         hnd->status = NT_STATUS_NO_MEMORY;
         break;
      }
      
      resume_idx++;
      num_keys_out++;
   } while(num_keys_out < op->in.max_keys);

   if(CAC_OP_FAILED(hnd->status)) {
      op->out.num_keys = 0;
      return CAC_FAILURE;
   }

   op->out.resume_idx   = resume_idx;
   op->out.num_keys     = num_keys_out;
   op->out.key_names    = key_names_out;
   op->out.class_names  = class_names_out;
   op->out.mod_times    = mod_times_out;

   return CAC_SUCCESS;
}

int cac_RegCreateKey(CacServerHandle *hnd, TALLOC_CTX *mem_ctx, struct RegCreateKey *op) {
   struct rpc_pipe_client *pipe_hnd = NULL;
   WERROR err;

   POLICY_HND *key_out;

   struct RegOpenKey rok;

   if(!hnd) 
      return CAC_FAILURE;

   if(!hnd->_internal.ctx) {
      hnd->status = NT_STATUS_INVALID_HANDLE;
      return CAC_FAILURE;
   }

   if(!op || !op->in.parent_key || !op->in.key_name || !mem_ctx) {
      hnd->status = NT_STATUS_INVALID_PARAMETER;
      return CAC_FAILURE;
   }

   /*first try to open the key - we use cac_RegOpenKey(). this doubles as a way to ensure the winreg pipe is initialized*/
   ZERO_STRUCT(rok);

   rok.in.name = op->in.key_name;
   rok.in.access = op->in.access;
   rok.in.parent_key = op->in.parent_key;

   if(cac_RegOpenKey(hnd, mem_ctx, &rok)) {
      /*then we got the key, return*/
      op->out.key = rok.out.key;
      return CAC_SUCCESS;
   }

   /*just be ultra-safe*/
   pipe_hnd = cac_GetPipe(hnd, PI_WINREG);
   if(!pipe_hnd) {
      hnd->status = NT_STATUS_INVALID_HANDLE;
      return CAC_FAILURE;
   }

   key_out = talloc(mem_ctx, POLICY_HND);
   if(!key_out) {
      hnd->status = NT_STATUS_NO_MEMORY;
      return CAC_FAILURE;
   }

   err = rpccli_reg_create_key_ex( pipe_hnd, mem_ctx, op->in.parent_key, op->in.key_name, op->in.class_name, op->in.access, key_out);
   hnd->status = werror_to_ntstatus(err);

   if(!NT_STATUS_IS_OK(hnd->status)) {
      return CAC_FAILURE;
   }

   op->out.key = key_out;

   return CAC_SUCCESS;

}

WERROR cac_delete_subkeys_recursive(struct rpc_pipe_client *pipe_hnd, TALLOC_CTX *mem_ctx, POLICY_HND *key) {
   /*NOTE: using cac functions might result in a big(ger) memory bloat, and would probably be far less efficient 
    * so we use the cli_reg functions directly*/

   WERROR err = WERR_OK;

   POLICY_HND subkey;
   fstring subkey_name;
   fstring class_buf;
   time_t mod_time_buf;

   int cur_key = 0;

   while(W_ERROR_IS_OK(err)) {
      err = rpccli_reg_enum_key( pipe_hnd, mem_ctx, key, cur_key, subkey_name, class_buf, &mod_time_buf); 

      if(!W_ERROR_IS_OK(err))
         break;

      /*try to open the key with full access*/
      err = rpccli_reg_open_entry(pipe_hnd, mem_ctx, key, subkey_name, REG_KEY_ALL, &subkey);

      if(!W_ERROR_IS_OK(err))
         break;

      err = cac_delete_subkeys_recursive(pipe_hnd, mem_ctx, &subkey);

      if(!W_ERROR_EQUAL(err,WERR_NO_MORE_ITEMS) && !W_ERROR_IS_OK(err))
         break;

      /*flush the key just to be safe*/
      rpccli_reg_flush_key(pipe_hnd, mem_ctx, key);
      
      /*close the key that we opened*/
      rpccli_reg_close(pipe_hnd, mem_ctx, &subkey);

      /*now we delete the subkey*/
      err = rpccli_reg_delete_key(pipe_hnd, mem_ctx, key, subkey_name);


      cur_key++;
   }


   return err;
}



int cac_RegDeleteKey(CacServerHandle *hnd, TALLOC_CTX *mem_ctx, struct RegDeleteKey *op) {
   struct rpc_pipe_client *pipe_hnd = NULL;
   WERROR err;

   if(!hnd) 
      return CAC_FAILURE;

   if(!hnd->_internal.ctx || !hnd->_internal.pipes[PI_WINREG]) {
      hnd->status = NT_STATUS_INVALID_HANDLE;
      return CAC_FAILURE;
   }

   if(!op || !op->in.parent_key || !op->in.name || !mem_ctx) {
      hnd->status = NT_STATUS_INVALID_PARAMETER;
      return CAC_FAILURE;
   }

   pipe_hnd = cac_GetPipe(hnd, PI_WINREG);
   if(!pipe_hnd) {
      hnd->status = NT_STATUS_INVALID_HANDLE;
      return CAC_FAILURE;
   }

   if(op->in.recursive) {
      /*first open the key, and then delete all of it's subkeys recursively*/
      struct RegOpenKey rok;
      ZERO_STRUCT(rok);

      rok.in.parent_key = op->in.parent_key;
      rok.in.name       = op->in.name;
      rok.in.access     = REG_KEY_ALL;

      if(!cac_RegOpenKey(hnd, mem_ctx, &rok))
         return CAC_FAILURE;

      err = cac_delete_subkeys_recursive(pipe_hnd, mem_ctx, rok.out.key);

      /*close the key that we opened*/
      cac_RegClose(hnd, mem_ctx, rok.out.key);

      hnd->status = werror_to_ntstatus(err);

      if(NT_STATUS_V(hnd->status) != NT_STATUS_V(NT_STATUS_GUIDS_EXHAUSTED) && !NT_STATUS_IS_OK(hnd->status))
         return CAC_FAILURE;

      /*now go on to actually delete the key*/
   }

   err = rpccli_reg_delete_key( pipe_hnd, mem_ctx, op->in.parent_key, op->in.name);
   hnd->status = werror_to_ntstatus(err);

   if(!NT_STATUS_IS_OK(hnd->status)) {
      return CAC_FAILURE;
   }

   return CAC_SUCCESS;
}

int cac_RegDeleteValue(CacServerHandle *hnd, TALLOC_CTX *mem_ctx, struct RegDeleteValue *op) {
   struct rpc_pipe_client *pipe_hnd = NULL;
   WERROR err;

   if(!hnd) 
      return CAC_FAILURE;

   if(!hnd->_internal.ctx || !hnd->_internal.pipes[PI_WINREG]) {
      hnd->status = NT_STATUS_INVALID_HANDLE;
      return CAC_FAILURE;
   }

   if(!op || !op->in.parent_key || !op->in.name || !mem_ctx) {
      hnd->status = NT_STATUS_INVALID_PARAMETER;
      return CAC_FAILURE;
   }

   pipe_hnd = cac_GetPipe(hnd, PI_WINREG);
   if(!pipe_hnd) {
      hnd->status = NT_STATUS_INVALID_HANDLE;
      return CAC_FAILURE;
   }

   err = rpccli_reg_delete_val( pipe_hnd, mem_ctx, op->in.parent_key, op->in.name);
   hnd->status = werror_to_ntstatus(err);

   if(!NT_STATUS_IS_OK(hnd->status)) {
      return CAC_FAILURE;
   }

   return CAC_SUCCESS;
}

#if 0
/* JRA - disabled until fix. */
/* This code is currently broken so disable it - it needs to handle the ERROR_MORE_DATA
   cleanly and resubmit the query. */

int cac_RegQueryKeyInfo(CacServerHandle *hnd, TALLOC_CTX *mem_ctx, struct RegQueryKeyInfo *op) {
   struct rpc_pipe_client *pipe_hnd = NULL;
   WERROR err;

   char *class_name_out    = NULL;
   uint32 class_len        = 0;
   uint32 num_subkeys_out  = 0;
   uint32 long_subkey_out  = 0;
   uint32 long_class_out   = 0;
   uint32 num_values_out   = 0;
   uint32 long_value_out   = 0;
   uint32 long_data_out    = 0;
   uint32 secdesc_size     = 0;
   NTTIME mod_time;

   if(!hnd) 
      return CAC_FAILURE;

   if(!hnd->_internal.ctx || !hnd->_internal.pipes[PI_WINREG]) {
      hnd->status = NT_STATUS_INVALID_HANDLE;
      return CAC_FAILURE;
   }

   if(!op || !op->in.key || !mem_ctx) {
      hnd->status = NT_STATUS_INVALID_PARAMETER;
      return CAC_FAILURE;
   }

   pipe_hnd = cac_GetPipe(hnd, PI_WINREG);
   if(!pipe_hnd) {
      hnd->status = NT_STATUS_INVALID_HANDLE;
      return CAC_FAILURE;
   }

   err = rpccli_reg_query_key( pipe_hnd, mem_ctx, op->in.key,
                              class_name_out,
                              &class_len,
                              &num_subkeys_out,
                              &long_subkey_out,
                              &long_class_out,
                              &num_values_out,
                              &long_value_out,
                              &long_data_out,
                              &secdesc_size,
                              &mod_time);

   hnd->status = werror_to_ntstatus(err);

   if(!NT_STATUS_IS_OK(hnd->status))
      return CAC_FAILURE;

   if(!class_name_out) {
      op->out.class_name = talloc_strdup(mem_ctx, "");
   }
   else if(class_len != 0 && class_name_out[class_len - 1] != '\0') {
      /*then we need to add a '\0'*/
      op->out.class_name = talloc_size(mem_ctx, sizeof(char)*(class_len + 1));

      memcpy(op->out.class_name, class_name_out, class_len);

      op->out.class_name[class_len] = '\0';
   }
   else { /*then everything worked out fine in the function*/
      op->out.class_name = talloc_strdup(mem_ctx, class_name_out);
   }

   if(!op->out.class_name) {
      hnd->status = NT_STATUS_NO_MEMORY;
      return CAC_FAILURE;
   }

   op->out.num_subkeys        = num_subkeys_out;
   op->out.longest_subkey     = long_subkey_out;
   op->out.longest_class      = long_class_out;
   op->out.num_values         = num_values_out;
   op->out.longest_value_name = long_value_out;
   op->out.longest_value_data = long_data_out;
   op->out.security_desc_size = secdesc_size;
   op->out.last_write_time    = nt_time_to_unix(&mod_time);

   return CAC_FAILURE;
}
#endif

int cac_RegQueryValue(CacServerHandle *hnd, TALLOC_CTX *mem_ctx, struct RegQueryValue *op) {
   struct rpc_pipe_client *pipe_hnd = NULL;
   WERROR err;

   uint32 val_type;
   REGVAL_BUFFER buffer;
   REG_VALUE_DATA *data_out = NULL;

   if(!hnd) 
      return CAC_FAILURE;

   if(!hnd->_internal.ctx || !hnd->_internal.pipes[PI_WINREG]) {
      hnd->status = NT_STATUS_INVALID_HANDLE;
      return CAC_FAILURE;
   }

   if(!op || !op->in.key || !op->in.val_name || !mem_ctx) {
      hnd->status = NT_STATUS_INVALID_PARAMETER;
      return CAC_FAILURE;
   }

   pipe_hnd = cac_GetPipe(hnd, PI_WINREG);
   if(!pipe_hnd) {
      hnd->status = NT_STATUS_INVALID_HANDLE;
      return CAC_FAILURE;
   }

   err = rpccli_reg_query_value(pipe_hnd, mem_ctx, op->in.key, op->in.val_name, &val_type, &buffer);
   hnd->status = werror_to_ntstatus(err);

   if(!NT_STATUS_IS_OK(hnd->status))
      return CAC_FAILURE;

   data_out = cac_MakeRegValueData(mem_ctx, val_type, buffer);
   if(!data_out) {
      if(errno == ENOMEM)
         hnd->status = NT_STATUS_NO_MEMORY;
      else
         hnd->status = NT_STATUS_INVALID_PARAMETER;

      return CAC_FAILURE;
   }

   op->out.type = val_type;
   op->out.data = data_out;

   return CAC_SUCCESS;
}


int cac_RegEnumValues(CacServerHandle *hnd, TALLOC_CTX *mem_ctx, struct RegEnumValues *op) {
   struct rpc_pipe_client *pipe_hnd = NULL;
   WERROR err;

   /*buffers for rpccli_reg_enum_key call*/
   fstring val_name_buf;
   REGVAL_BUFFER val_buf;

   /*output buffers*/
   uint32 *types_out          = NULL;
   REG_VALUE_DATA **values_out = NULL;
   char **val_names_out       = NULL;
   uint32 num_values_out      = 0;
   uint32 resume_idx          = 0;

   if(!hnd) 
      return CAC_FAILURE;

   /*this is to avoid useless rpc calls, if the last call exhausted all the keys, then we don't need to go through everything again*/
   if(NT_STATUS_V(hnd->status) == NT_STATUS_V(NT_STATUS_GUIDS_EXHAUSTED))
      return CAC_FAILURE;

   if(!hnd->_internal.ctx || !hnd->_internal.pipes[PI_WINREG]) {
      hnd->status = NT_STATUS_INVALID_HANDLE;
      return CAC_FAILURE;
   }

   if(!op || !op->in.key || op->in.max_values == 0 || !mem_ctx) {
      hnd->status = NT_STATUS_INVALID_PARAMETER;
      return CAC_FAILURE;
   }

   pipe_hnd = cac_GetPipe(hnd, PI_WINREG);
   if(!pipe_hnd) {
      hnd->status = NT_STATUS_INVALID_HANDLE;
      return CAC_FAILURE;
   }

   /*we need to assume that the max number of values will be enumerated*/
   types_out = (uint32 *)talloc_array(mem_ctx, int, op->in.max_values);
   if(!types_out) {
      hnd->status = NT_STATUS_NO_MEMORY;
      return CAC_FAILURE;
   }

   values_out = talloc_array(mem_ctx, REG_VALUE_DATA *, op->in.max_values);
   if(!values_out) {
      TALLOC_FREE(types_out);
      hnd->status = NT_STATUS_NO_MEMORY;
      return CAC_FAILURE;
   }

   val_names_out = talloc_array(mem_ctx, char *, op->in.max_values);
   if(!val_names_out) {
      TALLOC_FREE(types_out);
      TALLOC_FREE(values_out);
      hnd->status = NT_STATUS_NO_MEMORY;
      return CAC_FAILURE;
   }

   resume_idx = op->out.resume_idx;
   do {
      ZERO_STRUCT(val_buf);

      err = rpccli_reg_enum_val(pipe_hnd, mem_ctx, op->in.key, resume_idx, val_name_buf, &types_out[num_values_out], &val_buf);
      hnd->status = werror_to_ntstatus(err);

      if(!NT_STATUS_IS_OK(hnd->status))
         break;

      values_out[num_values_out] = cac_MakeRegValueData(mem_ctx, types_out[num_values_out], val_buf);
      val_names_out[num_values_out] = talloc_strdup(mem_ctx, val_name_buf);

      if(!val_names_out[num_values_out] || !values_out[num_values_out]) {
         hnd->status = NT_STATUS_NO_MEMORY;
         break;
      }

      num_values_out++;
      resume_idx++;
   } while(num_values_out < op->in.max_values);

   if(CAC_OP_FAILED(hnd->status))
      return CAC_FAILURE;

   op->out.types        = types_out;
   op->out.num_values   = num_values_out;
   op->out.value_names  = val_names_out;
   op->out.values       = values_out;
   op->out.resume_idx   = resume_idx;

   return CAC_SUCCESS;
}

int cac_RegSetValue(CacServerHandle *hnd, TALLOC_CTX *mem_ctx, struct RegSetValue *op) {
   struct rpc_pipe_client *pipe_hnd = NULL;
   WERROR err;

   RPC_DATA_BLOB *buffer;

   if(!hnd) 
      return CAC_FAILURE;

   if(!hnd->_internal.ctx || !hnd->_internal.pipes[PI_WINREG]) {
      hnd->status = NT_STATUS_INVALID_HANDLE;
      return CAC_FAILURE;
   }

   if(!op || !op->in.key || !op->in.val_name || !mem_ctx) {
      hnd->status = NT_STATUS_INVALID_PARAMETER;
      return CAC_FAILURE;
   }

   pipe_hnd = cac_GetPipe(hnd, PI_WINREG);
   if(!pipe_hnd) {
      hnd->status = NT_STATUS_INVALID_HANDLE;
      return CAC_FAILURE;
   }

   buffer = cac_MakeRpcDataBlob(mem_ctx, op->in.type, op->in.value);

   if(!buffer) {
      if(errno == ENOMEM)
         hnd->status = NT_STATUS_NO_MEMORY;
      else
         hnd->status = NT_STATUS_INVALID_PARAMETER;

      return CAC_FAILURE;
   }

   err = rpccli_reg_set_val(pipe_hnd, mem_ctx, op->in.key, op->in.val_name, op->in.type, buffer);
   hnd->status = werror_to_ntstatus(err);
   
   if(!NT_STATUS_IS_OK(hnd->status))
      return CAC_FAILURE;

   /*flush*/
   err = rpccli_reg_flush_key(pipe_hnd, mem_ctx, op->in.key);
   hnd->status = werror_to_ntstatus(err);

   if(!NT_STATUS_IS_OK(hnd->status))
      return CAC_FAILURE;

   return CAC_SUCCESS;
}



int cac_RegGetVersion(CacServerHandle *hnd, TALLOC_CTX *mem_ctx, struct RegGetVersion *op) {
   struct rpc_pipe_client *pipe_hnd = NULL;
   WERROR err;

   uint32 version_out;

   if(!hnd) 
      return CAC_FAILURE;

   if(!hnd->_internal.ctx || !hnd->_internal.pipes[PI_WINREG]) {
      hnd->status = NT_STATUS_INVALID_HANDLE;
      return CAC_FAILURE;
   }

   if(!op || !op->in.key || !mem_ctx) {
      hnd->status = NT_STATUS_INVALID_PARAMETER;
      return CAC_FAILURE;
   }

   pipe_hnd = cac_GetPipe(hnd, PI_WINREG);
   if(!pipe_hnd) {
      hnd->status = NT_STATUS_INVALID_HANDLE;
      return CAC_FAILURE;
   }

   err = rpccli_reg_getversion( pipe_hnd, mem_ctx, op->in.key, &version_out);
   hnd->status = werror_to_ntstatus(err);

   if(!NT_STATUS_IS_OK(hnd->status))
      return CAC_FAILURE;

   op->out.version = version_out;

   return CAC_SUCCESS;
}

int cac_RegGetKeySecurity(CacServerHandle *hnd, TALLOC_CTX *mem_ctx, struct RegGetKeySecurity *op) {
   struct rpc_pipe_client *pipe_hnd = NULL;
   WERROR err;

   uint32 buf_size;
   SEC_DESC_BUF buf;

   ZERO_STRUCT(buf);

   if(!hnd) 
      return CAC_FAILURE;

   if(!hnd->_internal.ctx || !hnd->_internal.pipes[PI_WINREG]) {
      hnd->status = NT_STATUS_INVALID_HANDLE;
      return CAC_FAILURE;
   }

   if(!op || !op->in.key || op->in.info_type == 0 || !mem_ctx) {
      hnd->status = NT_STATUS_INVALID_PARAMETER;
      return CAC_FAILURE;
   }

   pipe_hnd = cac_GetPipe(hnd, PI_WINREG);
   if(!pipe_hnd) {
      hnd->status = NT_STATUS_INVALID_HANDLE;
      return CAC_FAILURE;
   }

   err = rpccli_reg_get_key_sec(pipe_hnd, mem_ctx, op->in.key, op->in.info_type, &buf_size, &buf);
   hnd->status = werror_to_ntstatus(err);


   if(!NT_STATUS_IS_OK(hnd->status)) {
      return CAC_FAILURE;
   }

   op->out.size = buf.len;
   op->out.descriptor = dup_sec_desc(mem_ctx, buf.sec);

   if (op->out.descriptor == NULL) {
	   return CAC_FAILURE;
   }

   return CAC_SUCCESS;
}

int cac_RegSetKeySecurity(CacServerHandle *hnd, TALLOC_CTX *mem_ctx, struct RegSetKeySecurity *op) {
   struct rpc_pipe_client *pipe_hnd = NULL;
   WERROR err;

   if(!hnd) 
      return CAC_FAILURE;

   if(!hnd->_internal.ctx || !hnd->_internal.pipes[PI_WINREG]) {
      hnd->status = NT_STATUS_INVALID_HANDLE;
      return CAC_FAILURE;
   }

   if(!op || !op->in.key || op->in.info_type == 0 || op->in.size == 0 || !op->in.descriptor || !mem_ctx) {
      hnd->status = NT_STATUS_INVALID_PARAMETER;
      return CAC_FAILURE;
   }

   pipe_hnd = cac_GetPipe(hnd, PI_WINREG);
   if(!pipe_hnd) {
      hnd->status = NT_STATUS_INVALID_HANDLE;
      return CAC_FAILURE;
   }

   err = rpccli_reg_set_key_sec(pipe_hnd, mem_ctx, op->in.key, op->in.info_type, op->in.size, op->in.descriptor);
   hnd->status = werror_to_ntstatus(err);


   if(!NT_STATUS_IS_OK(hnd->status)) {
      return CAC_FAILURE;
   }

   return CAC_SUCCESS;
}

int cac_RegSaveKey(CacServerHandle *hnd, TALLOC_CTX *mem_ctx, struct RegSaveKey *op) {
   struct rpc_pipe_client *pipe_hnd = NULL;
   WERROR err;

   if(!hnd) 
      return CAC_FAILURE;

   if(!hnd->_internal.ctx || !hnd->_internal.pipes[PI_WINREG]) {
      hnd->status = NT_STATUS_INVALID_HANDLE;
      return CAC_FAILURE;
   }

   if(!op || !op->in.key || !op->in.filename || !mem_ctx) {
      hnd->status = NT_STATUS_INVALID_PARAMETER;
      return CAC_FAILURE;
   }

   pipe_hnd = cac_GetPipe(hnd, PI_WINREG);
   if(!pipe_hnd) {
      hnd->status = NT_STATUS_INVALID_HANDLE;
      return CAC_FAILURE;
   }

   err = rpccli_reg_save_key( pipe_hnd, mem_ctx, op->in.key, op->in.filename);
   hnd->status = werror_to_ntstatus(err);


   if(!NT_STATUS_IS_OK(hnd->status)) {
      return CAC_FAILURE;
   }

   return CAC_SUCCESS;
}

int cac_Shutdown(CacServerHandle *hnd, TALLOC_CTX *mem_ctx, struct Shutdown *op) {
   SMBCSRV *srv = NULL;
   struct rpc_pipe_client *pipe_hnd = NULL;

   char *msg;

   if(!hnd) 
      return CAC_FAILURE;

   if(!hnd->_internal.ctx) {
      hnd->status = NT_STATUS_INVALID_HANDLE;
      return CAC_FAILURE;
   }

   if(!op || !mem_ctx) {
      hnd->status = NT_STATUS_INVALID_PARAMETER;
      return CAC_FAILURE;
   }

   srv = cac_GetServer(hnd);
   if(!srv) {
      hnd->status = NT_STATUS_INVALID_HANDLE;
      return CAC_FAILURE;
   }

   /*initialize for winreg pipe if we have to*/
   if(!hnd->_internal.pipes[PI_SHUTDOWN]) {
      if(!(pipe_hnd = cli_rpc_pipe_open_noauth(&srv->cli, PI_SHUTDOWN, &(hnd->status)))) {
         return CAC_FAILURE;
      }

      hnd->_internal.pipes[PI_SHUTDOWN] = True;
   }

   pipe_hnd = cac_GetPipe(hnd, PI_SHUTDOWN);
   if(!pipe_hnd) {
      hnd->status = NT_STATUS_INVALID_HANDLE;
      return CAC_FAILURE;
   }

   msg = (op->in.message != NULL) ? op->in.message : talloc_strdup(mem_ctx, "");

   hnd->status = NT_STATUS_OK;

   if(hnd->_internal.srv_level > SRV_WIN_NT4) {
      hnd->status = rpccli_shutdown_init_ex( pipe_hnd, mem_ctx, msg, op->in.timeout, op->in.reboot, op->in.force, op->in.reason);
   }

   if(hnd->_internal.srv_level < SRV_WIN_2K || !NT_STATUS_IS_OK(hnd->status)) {
      hnd->status = rpccli_shutdown_init( pipe_hnd, mem_ctx, msg, op->in.timeout, op->in.reboot, op->in.force);

      hnd->_internal.srv_level = SRV_WIN_NT4;
   }

   if(!NT_STATUS_IS_OK(hnd->status)) {
      return CAC_FAILURE;
   }

   return CAC_SUCCESS;
}

int cac_AbortShutdown(CacServerHandle *hnd, TALLOC_CTX *mem_ctx) {
   struct rpc_pipe_client *pipe_hnd = NULL;

   if(!hnd) 
      return CAC_FAILURE;

   if(!hnd->_internal.ctx || !hnd->_internal.pipes[PI_SHUTDOWN]) {
      hnd->status = NT_STATUS_INVALID_HANDLE;
      return CAC_FAILURE;
   }

   pipe_hnd = cac_GetPipe(hnd, PI_SHUTDOWN);
   if(!pipe_hnd) {
      hnd->status = NT_STATUS_INVALID_HANDLE;
      return CAC_FAILURE;
   }

   hnd->status = rpccli_shutdown_abort(pipe_hnd, mem_ctx);

   if(!NT_STATUS_IS_OK(hnd->status))
      return CAC_FAILURE;

   return CAC_SUCCESS;
}

