/* 
   Unix SMB/CIFS implementation.
   ads (active directory) printer utility library
   Copyright (C) Jim McDonough <jmcd@us.ibm.com> 2002
   
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
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include "includes.h"

#ifdef HAVE_ADS

/*
  find a printer given the name and the hostname
    Note that results "res" may be allocated on return so that the
    results can be used.  It should be freed using ads_msgfree.
*/
ADS_STATUS ads_find_printer_on_server(ADS_STRUCT *ads, void **res,
				      const char *printer, const char *servername)
{
	ADS_STATUS status;
	char *srv_dn, **srv_cn, *s;
	const char *attrs[] = {"*", "nTSecurityDescriptor", NULL};

	status = ads_find_machine_acct(ads, res, servername);
	if (!ADS_ERR_OK(status)) {
		DEBUG(1, ("ads_find_printer_on_server: cannot find host %s in ads\n",
			  servername));
		return status;
	}
	if (ads_count_replies(ads, *res) != 1) {
		return ADS_ERROR(LDAP_NO_SUCH_OBJECT);
	}
	srv_dn = ldap_get_dn(ads->ld, *res);
	if (srv_dn == NULL) {
		return ADS_ERROR(LDAP_NO_MEMORY);
	}
	srv_cn = ldap_explode_dn(srv_dn, 1);
	if (srv_cn == NULL) {
		ldap_memfree(srv_dn);
		return ADS_ERROR(LDAP_INVALID_DN_SYNTAX);
	}
	ads_msgfree(ads, *res);

	asprintf(&s, "(cn=%s-%s)", srv_cn[0], printer);
	status = ads_search(ads, res, s, attrs);

	ldap_memfree(srv_dn);
	ldap_value_free(srv_cn);
	free(s);
	return status;	
}

ADS_STATUS ads_find_printers(ADS_STRUCT *ads, void **res)
{
	const char *ldap_expr;
	const char *attrs[] = { "objectClass", "printerName", "location", "driverName",
				"serverName", "description", NULL };

	/* For the moment only display all printers */

	ldap_expr = "(&(!(showInAdvancedViewOnly=TRUE))(uncName=*)"
		"(objectCategory=printQueue))";

	return ads_search(ads, res, ldap_expr, attrs);
}

/*
  modify a printer entry in the directory
*/
ADS_STATUS ads_mod_printer_entry(ADS_STRUCT *ads, char *prt_dn,
				 TALLOC_CTX *ctx, const ADS_MODLIST *mods)
{
	return ads_gen_mod(ads, prt_dn, *mods);
}

/*
  add a printer to the directory
*/
ADS_STATUS ads_add_printer_entry(ADS_STRUCT *ads, char *prt_dn,
					TALLOC_CTX *ctx, ADS_MODLIST *mods)
{
	ads_mod_str(ctx, mods, "objectClass", "printQueue");
	return ads_gen_add(ads, prt_dn, *mods);
}

/*
  map a REG_SZ to an ldap mod
*/
static BOOL map_sz(TALLOC_CTX *ctx, ADS_MODLIST *mods, 
			    const REGISTRY_VALUE *value)
{
	char *str_value = NULL;
	ADS_STATUS status;

	if (value->type != REG_SZ)
		return False;

	if (value->size && *((smb_ucs2_t *) value->data_p)) {
		pull_ucs2_talloc(ctx, &str_value, (const smb_ucs2_t *) value->data_p);
		status = ads_mod_str(ctx, mods, value->valuename, str_value);
		return ADS_ERR_OK(status);
	}
	return True;
		
}

/*
  map a REG_DWORD to an ldap mod
*/
static BOOL map_dword(TALLOC_CTX *ctx, ADS_MODLIST *mods, 
		      const REGISTRY_VALUE *value)
{
	char *str_value = NULL;
	ADS_STATUS status;

	if (value->type != REG_DWORD)
		return False;
	str_value = talloc_asprintf(ctx, "%d", *((uint32 *) value->data_p));
	if (!str_value) {
		return False;
	}
	status = ads_mod_str(ctx, mods, value->valuename, str_value);
	return ADS_ERR_OK(status);
}

/*
  map a boolean REG_BINARY to an ldap mod
*/
static BOOL map_bool(TALLOC_CTX *ctx, ADS_MODLIST *mods,
		     const REGISTRY_VALUE *value)
{
	char *str_value;
	ADS_STATUS status;

	if ((value->type != REG_BINARY) || (value->size != 1))
		return False;
	str_value =  talloc_asprintf(ctx, "%s", 
				     *(value->data_p) ? "TRUE" : "FALSE");
	if (!str_value) {
		return False;
	}
	status = ads_mod_str(ctx, mods, value->valuename, str_value);
	return ADS_ERR_OK(status);
}

/*
  map a REG_MULTI_SZ to an ldap mod
*/
static BOOL map_multi_sz(TALLOC_CTX *ctx, ADS_MODLIST *mods,
			 const REGISTRY_VALUE *value)
{
	char **str_values = NULL;
	smb_ucs2_t *cur_str = (smb_ucs2_t *) value->data_p;
        uint32 size = 0, num_vals = 0, i=0;
	ADS_STATUS status;

	if (value->type != REG_MULTI_SZ)
		return False;

	while(cur_str && *cur_str && (size < value->size)) {		
		size += 2 * (strlen_w(cur_str) + 1);
		cur_str += strlen_w(cur_str) + 1;
		num_vals++;
	};

	if (num_vals) {
		str_values = TALLOC_ARRAY(ctx, char *, num_vals + 1);
		if (!str_values) {
			return False;
		}
		memset(str_values, '\0', 
		       (num_vals + 1) * sizeof(char *));

		cur_str = (smb_ucs2_t *) value->data_p;
		for (i=0; i < num_vals; i++)
			cur_str += pull_ucs2_talloc(ctx, &str_values[i],
			                            cur_str);

		status = ads_mod_strlist(ctx, mods, value->valuename, 
					 (const char **) str_values);
		return ADS_ERR_OK(status);
	} 
	return True;
}

struct valmap_to_ads {
	const char *valname;
	BOOL (*fn)(TALLOC_CTX *, ADS_MODLIST *, const REGISTRY_VALUE *);
};

/*
  map a REG_SZ to an ldap mod
*/
static void map_regval_to_ads(TALLOC_CTX *ctx, ADS_MODLIST *mods, 
			      REGISTRY_VALUE *value)
{
	const struct valmap_to_ads map[] = {
		{SPOOL_REG_ASSETNUMBER, map_sz},
		{SPOOL_REG_BYTESPERMINUTE, map_dword},
		{SPOOL_REG_DEFAULTPRIORITY, map_dword},
		{SPOOL_REG_DESCRIPTION, map_sz},
		{SPOOL_REG_DRIVERNAME, map_sz},
		{SPOOL_REG_DRIVERVERSION, map_dword},
		{SPOOL_REG_FLAGS, map_dword},
		{SPOOL_REG_LOCATION, map_sz},
		{SPOOL_REG_OPERATINGSYSTEM, map_sz},
		{SPOOL_REG_OPERATINGSYSTEMHOTFIX, map_sz},
		{SPOOL_REG_OPERATINGSYSTEMSERVICEPACK, map_sz},
		{SPOOL_REG_OPERATINGSYSTEMVERSION, map_sz},
		{SPOOL_REG_PORTNAME, map_multi_sz},
		{SPOOL_REG_PRINTATTRIBUTES, map_dword},
		{SPOOL_REG_PRINTBINNAMES, map_multi_sz},
		{SPOOL_REG_PRINTCOLLATE, map_bool},
		{SPOOL_REG_PRINTCOLOR, map_bool},
		{SPOOL_REG_PRINTDUPLEXSUPPORTED, map_bool},
		{SPOOL_REG_PRINTENDTIME, map_dword},
		{SPOOL_REG_PRINTFORMNAME, map_sz},
		{SPOOL_REG_PRINTKEEPPRINTEDJOBS, map_bool},
		{SPOOL_REG_PRINTLANGUAGE, map_multi_sz},
		{SPOOL_REG_PRINTMACADDRESS, map_sz},
		{SPOOL_REG_PRINTMAXCOPIES, map_sz},
		{SPOOL_REG_PRINTMAXRESOLUTIONSUPPORTED, map_dword},
		{SPOOL_REG_PRINTMAXXEXTENT, map_dword},
		{SPOOL_REG_PRINTMAXYEXTENT, map_dword},
		{SPOOL_REG_PRINTMEDIAREADY, map_multi_sz},
		{SPOOL_REG_PRINTMEDIASUPPORTED, map_multi_sz},
		{SPOOL_REG_PRINTMEMORY, map_dword},
		{SPOOL_REG_PRINTMINXEXTENT, map_dword},
		{SPOOL_REG_PRINTMINYEXTENT, map_dword},
		{SPOOL_REG_PRINTNETWORKADDRESS, map_sz},
		{SPOOL_REG_PRINTNOTIFY, map_sz},
		{SPOOL_REG_PRINTNUMBERUP, map_dword},
		{SPOOL_REG_PRINTORIENTATIONSSUPPORTED, map_multi_sz},
		{SPOOL_REG_PRINTOWNER, map_sz},
		{SPOOL_REG_PRINTPAGESPERMINUTE, map_dword},
		{SPOOL_REG_PRINTRATE, map_dword},
		{SPOOL_REG_PRINTRATEUNIT, map_sz},
		{SPOOL_REG_PRINTSEPARATORFILE, map_sz},
		{SPOOL_REG_PRINTSHARENAME, map_sz},
		{SPOOL_REG_PRINTSPOOLING, map_sz},
		{SPOOL_REG_PRINTSTAPLINGSUPPORTED, map_bool},
		{SPOOL_REG_PRINTSTARTTIME, map_dword},
		{SPOOL_REG_PRINTSTATUS, map_sz},
		{SPOOL_REG_PRIORITY, map_dword},
		{SPOOL_REG_SERVERNAME, map_sz},
		{SPOOL_REG_SHORTSERVERNAME, map_sz},
		{SPOOL_REG_UNCNAME, map_sz},
		{SPOOL_REG_URL, map_sz},
		{SPOOL_REG_VERSIONNUMBER, map_dword},
		{NULL, NULL}
	};
	int i;

	for (i=0; map[i].valname; i++) {
		if (StrCaseCmp(map[i].valname, value->valuename) == 0) {
			if (!map[i].fn(ctx, mods, value)) {
				DEBUG(5, ("Add of value %s to modlist failed\n", value->valuename));
			} else {
				DEBUG(7, ("Mapped value %s\n", value->valuename));
			}
			
		}
	}
}


WERROR get_remote_printer_publishing_data(struct rpc_pipe_client *cli, 
					  TALLOC_CTX *mem_ctx,
					  ADS_MODLIST *mods,
					  const char *printer)
{
	WERROR result;
	char *printername, *servername;
	REGVAL_CTR *dsdriver_ctr, *dsspooler_ctr;
	uint32 i;
	POLICY_HND pol;

	asprintf(&servername, "\\\\%s", cli->cli->desthost);
	asprintf(&printername, "%s\\%s", servername, printer);
	if (!servername || !printername) {
		DEBUG(3, ("Insufficient memory\n"));
		return WERR_NOMEM;
	}
	
	result = rpccli_spoolss_open_printer_ex(cli, mem_ctx, printername, 
					     "", MAXIMUM_ALLOWED_ACCESS, 
					     servername, cli->cli->user_name, &pol);
	if (!W_ERROR_IS_OK(result)) {
		DEBUG(3, ("Unable to open printer %s, error is %s.\n",
			  printername, dos_errstr(result)));
		return result;
	}
	
	if ( !(dsdriver_ctr = TALLOC_ZERO_P( mem_ctx, REGVAL_CTR )) ) 
		return WERR_NOMEM;

	result = rpccli_spoolss_enumprinterdataex(cli, mem_ctx, &pol, SPOOL_DSDRIVER_KEY, dsdriver_ctr);

	if (!W_ERROR_IS_OK(result)) {
		DEBUG(3, ("Unable to do enumdataex on %s, error is %s.\n",
			  printername, dos_errstr(result)));
	} else {
		uint32 num_values = regval_ctr_numvals( dsdriver_ctr );

		/* Have the data we need now, so start building */
		for (i=0; i < num_values; i++) {
			map_regval_to_ads(mem_ctx, mods, dsdriver_ctr->values[i]);
		}
	}
	
	if ( !(dsspooler_ctr = TALLOC_ZERO_P( mem_ctx, REGVAL_CTR )) )
		return WERR_NOMEM;

	result = rpccli_spoolss_enumprinterdataex(cli, mem_ctx, &pol, SPOOL_DSSPOOLER_KEY, dsspooler_ctr);

	if (!W_ERROR_IS_OK(result)) {
		DEBUG(3, ("Unable to do enumdataex on %s, error is %s.\n",
			  printername, dos_errstr(result)));
	} else {
		uint32 num_values = regval_ctr_numvals( dsspooler_ctr );

		for (i=0; i<num_values; i++) {
			map_regval_to_ads(mem_ctx, mods, dsspooler_ctr->values[i]);
		}
	}
	
	ads_mod_str(mem_ctx, mods, SPOOL_REG_PRINTERNAME, printer);

	TALLOC_FREE( dsdriver_ctr );
	TALLOC_FREE( dsspooler_ctr );

	rpccli_spoolss_close_printer(cli, mem_ctx, &pol);

	return result;
}

BOOL get_local_printer_publishing_data(TALLOC_CTX *mem_ctx,
				       ADS_MODLIST *mods,
				       NT_PRINTER_DATA *data)
{
	uint32 key,val;

	for (key=0; key < data->num_keys; key++) {
		REGVAL_CTR *ctr = data->keys[key].values;
		for (val=0; val < ctr->num_values; val++)
			map_regval_to_ads(mem_ctx, mods, ctr->values[val]);
	}
	return True;
}

#endif
