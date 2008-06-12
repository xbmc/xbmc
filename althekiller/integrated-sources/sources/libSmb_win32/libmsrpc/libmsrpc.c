/* 
 *  Unix SMB/CIFS implementation.
 *  MS-RPC client library implementation
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
#include "libsmbclient.h"
#include "libsmb_internal.h"

/*this function is based on code found in smbc_init_context() (libsmb/libsmbclient.c)*/
void cac_Init(int debug) {
   if(debug < 0 || debug > 99)
      debug = 0;

   DEBUGLEVEL = debug;

   setup_logging("libmsrpc", True);
}

int cac_InitHandleMem(CacServerHandle *hnd) {
   hnd->username       = SMB_MALLOC_ARRAY(char, sizeof(fstring));
   if(!hnd->username)
      return CAC_FAILURE;

   hnd->username[0] = '\0';

   hnd->domain         = SMB_MALLOC_ARRAY(char, sizeof(fstring));
   if(!hnd->domain)
      return CAC_FAILURE;
   
   hnd->domain[0] = '\0';

   hnd->netbios_name   = SMB_MALLOC_ARRAY(char, sizeof(fstring));
   if(!hnd->netbios_name)
      return CAC_FAILURE;

   hnd->netbios_name[0] = '\0';

   hnd->password       = SMB_MALLOC_ARRAY(char, sizeof(fstring));
   if(!hnd->password)
      return CAC_FAILURE;

   hnd->password[0] = '\0';

   hnd->server         = SMB_MALLOC_ARRAY(char, sizeof(fstring));
   if(!hnd->server)
      return CAC_FAILURE;

   hnd->server[0] = '\0';

   return CAC_SUCCESS;
}

CacServerHandle *cac_NewServerHandle(BOOL allocate_fields) {
   CacServerHandle * hnd;

   hnd = SMB_MALLOC_P(CacServerHandle);

   if(!hnd) {
      errno = ENOMEM;
      return NULL;
   }
   
   ZERO_STRUCTP(hnd);

   if(allocate_fields == True) {
      if(!cac_InitHandleMem(hnd)) {
         SAFE_FREE(hnd);
         return NULL;
      }
   }

   hnd->_internal.ctx = smbc_new_context();
   if(!hnd->_internal.ctx) {
      cac_FreeHandle(hnd);
      return NULL;
   }

   hnd->_internal.ctx->callbacks.auth_fn = cac_GetAuthDataFn;
   
   /*add defaults*/
   hnd->debug = 0;

   /*start at the highest and it will fall down after trying the functions*/
   hnd->_internal.srv_level = SRV_WIN_2K3;

   hnd->_internal.user_supplied_ctx = False;

   return hnd;
}

int cac_InitHandleData(CacServerHandle *hnd) {
   /*store any automatically initialized values*/
   if(!hnd->netbios_name) {
      hnd->netbios_name = SMB_STRDUP(hnd->_internal.ctx->netbios_name);
   }
   else if(hnd->netbios_name[0] == '\0') {
      strncpy(hnd->netbios_name, hnd->_internal.ctx->netbios_name, sizeof(fstring));
   }

   if(!hnd->username) {
      hnd->username = SMB_STRDUP(hnd->_internal.ctx->user);
   }
   else if(hnd->username[0] == '\0') {
      strncpy(hnd->username, hnd->_internal.ctx->user, sizeof(fstring));
   }

   if(!hnd->domain) {
      hnd->domain = SMB_STRDUP(hnd->_internal.ctx->workgroup);
   }
   else if(hnd->domain[0] == '\0') {
      strncpy(hnd->domain, hnd->_internal.ctx->workgroup, sizeof(fstring));
   }

   return CAC_SUCCESS;
}

void cac_SetAuthDataFn(CacServerHandle *hnd, smbc_get_auth_data_fn auth_fn) {
   hnd->_internal.ctx->callbacks.auth_fn = auth_fn;
}

void cac_SetSmbcContext(CacServerHandle *hnd, SMBCCTX *ctx) {

   SAFE_FREE(hnd->_internal.ctx);

   hnd->_internal.user_supplied_ctx = True;

   hnd->_internal.ctx = ctx;

   /*_try_ to avoid any problems that might occur if cac_Connect() isn't called*/
   /*cac_InitHandleData(hnd);*/
}

/*used internally*/
SMBCSRV *cac_GetServer(CacServerHandle *hnd) {
   SMBCSRV *srv;

   if(!hnd || !hnd->_internal.ctx) {
      return NULL;
   }

   srv = smbc_attr_server(hnd->_internal.ctx, hnd->server, "IPC$", hnd->domain, hnd->username, hnd->password, NULL);
   if(!srv) {
      hnd->status=NT_STATUS_UNSUCCESSFUL;
      DEBUG(1, ("cac_GetServer: Could not find server connection.\n"));
   }

   return srv;
}


int cac_Connect(CacServerHandle *hnd, const char *srv) {
   if(!hnd) {
      return CAC_FAILURE;
   }

   /*these values should be initialized by the user*/
   if(!hnd->server && !srv) {
      return CAC_FAILURE;
   }


   /*change the server name in the server handle if necessary*/
   if(srv && hnd->server && strcmp(hnd->server, srv) == 0) {
      SAFE_FREE(hnd->server);
      hnd->server = SMB_STRDUP(srv);
   }


   /*first see if the context has already been setup*/
   if( !(hnd->_internal.ctx->internal->_initialized) ) {
      hnd->_internal.ctx->debug = hnd->debug;

      /*initialize the context*/
      if(!smbc_init_context(hnd->_internal.ctx)) {
         return CAC_FAILURE;
      }
   }

   /*copy any uninitialized values out of the smbc context into the handle*/
   if(!cac_InitHandleData(hnd)) {
      return CAC_FAILURE;
   }

   DEBUG(3, ("cac_Connect: Username:     %s\n", hnd->username));
   DEBUG(3, ("cac_Connect: Domain:       %s\n", hnd->domain));
   DEBUG(3, ("cac_Connect: Netbios Name: %s\n", hnd->netbios_name));

   if(!cac_GetServer(hnd)) {
      return CAC_FAILURE;
   }
   
   return CAC_SUCCESS;
                                     
}


void cac_FreeHandle(CacServerHandle * hnd) {
   if(!hnd)
      return;

   /*only free the context if we created it*/
   if(!hnd->_internal.user_supplied_ctx) {
      smbc_free_context(hnd->_internal.ctx, True);
   }

   SAFE_FREE(hnd->netbios_name);
   SAFE_FREE(hnd->domain);
   SAFE_FREE(hnd->username);
   SAFE_FREE(hnd->password);
   SAFE_FREE(hnd->server);
   SAFE_FREE(hnd);

}

void cac_InitCacTime(CacTime *cactime, NTTIME nttime) {
   float high, low;
   uint32 sec;

   if(!cactime)
      return;

   ZERO_STRUCTP(cactime);

   /*this code is taken from display_time() found in rpcclient/cmd_samr.c*/
   if (nttime.high==0 && nttime.low==0)
		return;

	if (nttime.high==0x80000000 && nttime.low==0)
		return;

	high = 65536;	
	high = high/10000;
	high = high*65536;
	high = high/1000;
	high = high * (~nttime.high);

	low = ~nttime.low;	
	low = low/(1000*1000*10);

	sec=high+low;

	cactime->days=sec/(60*60*24);
	cactime->hours=(sec - (cactime->days*60*60*24)) / (60*60);
	cactime->minutes=(sec - (cactime->days*60*60*24) - (cactime->hours*60*60) ) / 60;
	cactime->seconds=sec - (cactime->days*60*60*24) - (cactime->hours*60*60) - (cactime->minutes*60);
}

void cac_GetAuthDataFn(const char * pServer,
                 const char * pShare,
                 char * pWorkgroup,
                 int maxLenWorkgroup,
                 char * pUsername,
                 int maxLenUsername,
                 char * pPassword,
                 int maxLenPassword)
    
{
    char temp[sizeof(fstring)];
    
    static char authUsername[sizeof(fstring)];
    static char authWorkgroup[sizeof(fstring)];
    static char authPassword[sizeof(fstring)];
    static char authSet = 0;

    char *pass = NULL;

    
    if (authSet)
    {
        strncpy(pWorkgroup, authWorkgroup, maxLenWorkgroup - 1);
        strncpy(pUsername, authUsername, maxLenUsername - 1);
        strncpy(pPassword, authPassword, maxLenPassword - 1);
    }
    else
    {
        d_printf("Domain: [%s] ", pWorkgroup);
        fgets(temp, sizeof(fstring), stdin);
        
        if (temp[strlen(temp) - 1] == '\n') /* A new line? */
        {
            temp[strlen(temp) - 1] = '\0';
        }
        

        if (temp[0] != '\0')
        {
            strncpy(pWorkgroup, temp, maxLenWorkgroup - 1);
            strncpy(authWorkgroup, temp, maxLenWorkgroup - 1);
        }
        
        d_printf("Username: [%s] ", pUsername);
        fgets(temp, sizeof(fstring), stdin);
        
        if (temp[strlen(temp) - 1] == '\n') /* A new line? */
        {
            temp[strlen(temp) - 1] = '\0';
        }
        
        if (temp[0] != '\0')
        {
            strncpy(pUsername, temp, maxLenUsername - 1);
            strncpy(authUsername, pUsername, maxLenUsername - 1);
        }
        
        pass = getpass("Password: ");
        if (pass)
            fstrcpy(temp, pass);
        if (temp[strlen(temp) - 1] == '\n') /* A new line? */
        {
            temp[strlen(temp) - 1] = '\0';
        }        
        if (temp[0] != '\0')
        {
            strncpy(pPassword, temp, maxLenPassword - 1);
            strncpy(authPassword, pPassword, maxLenPassword - 1);
        }        
        authSet = 1;
    }
}

