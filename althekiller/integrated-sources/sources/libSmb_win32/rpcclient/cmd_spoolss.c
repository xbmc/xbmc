/*
   Unix SMB/CIFS implementation.
   RPC pipe client

   Copyright (C) Gerald Carter                2001-2005
   Copyright (C) Tim Potter                        2000
   Copyright (C) Andrew Tridgell              1992-1999
   Copyright (C) Luke Kenneth Casson Leighton 1996-1999
 
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
#include "rpcclient.h"

struct table_node {
	const char 	*long_archi;
	const char 	*short_archi;
	int	version;
};
 
/* The version int is used by getdrivers.  Note that
   all architecture strings that support mutliple
   versions must be grouped together since enumdrivers
   uses this property to prevent issuing multiple 
   enumdriver calls for the same arch */


static const struct table_node archi_table[]= {

	{"Windows 4.0",          "WIN40",	0 },
	{"Windows NT x86",       "W32X86",	2 },
	{"Windows NT x86",       "W32X86",	3 },
	{"Windows NT R4000",     "W32MIPS",	2 },
	{"Windows NT Alpha_AXP", "W32ALPHA",	2 },
	{"Windows NT PowerPC",   "W32PPC",	2 },
	{"Windows IA64",         "IA64",        3 },
	{"Windows x64",          "x64",         3 },
	{NULL,                   "",		-1 }
};

/**
 * @file
 *
 * rpcclient module for SPOOLSS rpc pipe.
 *
 * This generally just parses and checks command lines, and then calls
 * a cli_spoolss function.
 **/

/****************************************************************************
 function to do the mapping between the long architecture name and
 the short one.
****************************************************************************/

static const char *cmd_spoolss_get_short_archi(const char *long_archi)
{
        int i=-1;

        DEBUG(107,("Getting architecture dependant directory\n"));
        do {
                i++;
        } while ( (archi_table[i].long_archi!=NULL ) &&
                  StrCaseCmp(long_archi, archi_table[i].long_archi) );

        if (archi_table[i].long_archi==NULL) {
                DEBUGADD(10,("Unknown architecture [%s] !\n", long_archi));
                return NULL;
        }

	/* this might be client code - but shouldn't this be an fstrcpy etc? */


        DEBUGADD(108,("index: [%d]\n", i));
        DEBUGADD(108,("long architecture: [%s]\n", archi_table[i].long_archi));
        DEBUGADD(108,("short architecture: [%s]\n", archi_table[i].short_archi));

	return archi_table[i].short_archi;
}

/****************************************************************************
****************************************************************************/

static WERROR cmd_spoolss_open_printer_ex(struct rpc_pipe_client *cli, 
                                            TALLOC_CTX *mem_ctx,
                                            int argc, const char **argv)
{
	WERROR 	        werror;
	fstring		printername;
	fstring		servername, user;
	POLICY_HND	hnd;
	
	if (argc != 2) {
		printf("Usage: %s <printername>\n", argv[0]);
		return WERR_OK;
	}
	
	if (!cli)
            return WERR_GENERAL_FAILURE;

	slprintf(servername, sizeof(servername)-1, "\\\\%s", cli->cli->desthost);
	strupper_m(servername);
	fstrcpy(user, cli->user_name);
	fstrcpy(printername, argv[1]);

	/* Open the printer handle */

	werror = rpccli_spoolss_open_printer_ex(cli, mem_ctx, printername, 
					     "", PRINTER_ALL_ACCESS, 
					     servername, user, &hnd);

	if (W_ERROR_IS_OK(werror)) {
		printf("Printer %s opened successfully\n", printername);
		werror = rpccli_spoolss_close_printer(cli, mem_ctx, &hnd);

		if (!W_ERROR_IS_OK(werror)) {
			printf("Error closing printer handle! (%s)\n", 
				get_dos_error_msg(werror));
		}
	}

	return werror;
}


/****************************************************************************
****************************************************************************/

static void display_print_info_0(PRINTER_INFO_0 *i0)
{
	fstring name = "";
	fstring servername = "";

	if (!i0)
		return;

	rpcstr_pull(name, i0->printername.buffer, sizeof(name), -1, STR_TERMINATE);

	rpcstr_pull(servername, i0->servername.buffer, sizeof(servername), -1,STR_TERMINATE);
  
	printf("\tprintername:[%s]\n", name);
	printf("\tservername:[%s]\n", servername);
	printf("\tcjobs:[0x%x]\n", i0->cjobs);
	printf("\ttotal_jobs:[0x%x]\n", i0->total_jobs);
	
	printf("\t:date: [%d]-[%d]-[%d] (%d)\n", i0->year, i0->month, 
	       i0->day, i0->dayofweek);
	printf("\t:time: [%d]-[%d]-[%d]-[%d]\n", i0->hour, i0->minute, 
	       i0->second, i0->milliseconds);
	
	printf("\tglobal_counter:[0x%x]\n", i0->global_counter);
	printf("\ttotal_pages:[0x%x]\n", i0->total_pages);
	
	printf("\tmajorversion:[0x%x]\n", i0->major_version);
	printf("\tbuildversion:[0x%x]\n", i0->build_version);
	
	printf("\tunknown7:[0x%x]\n", i0->unknown7);
	printf("\tunknown8:[0x%x]\n", i0->unknown8);
	printf("\tunknown9:[0x%x]\n", i0->unknown9);
	printf("\tsession_counter:[0x%x]\n", i0->session_counter);
	printf("\tunknown11:[0x%x]\n", i0->unknown11);
	printf("\tprinter_errors:[0x%x]\n", i0->printer_errors);
	printf("\tunknown13:[0x%x]\n", i0->unknown13);
	printf("\tunknown14:[0x%x]\n", i0->unknown14);
	printf("\tunknown15:[0x%x]\n", i0->unknown15);
	printf("\tunknown16:[0x%x]\n", i0->unknown16);
	printf("\tchange_id:[0x%x]\n", i0->change_id);
	printf("\tunknown18:[0x%x]\n", i0->unknown18);
	printf("\tstatus:[0x%x]\n", i0->status);
	printf("\tunknown20:[0x%x]\n", i0->unknown20);
	printf("\tc_setprinter:[0x%x]\n", i0->c_setprinter);
	printf("\tunknown22:[0x%x]\n", i0->unknown22);
	printf("\tunknown23:[0x%x]\n", i0->unknown23);
	printf("\tunknown24:[0x%x]\n", i0->unknown24);
	printf("\tunknown25:[0x%x]\n", i0->unknown25);
	printf("\tunknown26:[0x%x]\n", i0->unknown26);
	printf("\tunknown27:[0x%x]\n", i0->unknown27);
	printf("\tunknown28:[0x%x]\n", i0->unknown28);
	printf("\tunknown29:[0x%x]\n", i0->unknown29);

	printf("\n");
}

/****************************************************************************
****************************************************************************/

static void display_print_info_1(PRINTER_INFO_1 *i1)
{
	fstring desc = "";
	fstring name = "";
	fstring comm = "";

	rpcstr_pull(desc, i1->description.buffer, sizeof(desc), -1,
		    STR_TERMINATE);

	rpcstr_pull(name, i1->name.buffer, sizeof(name), -1, STR_TERMINATE);
	rpcstr_pull(comm, i1->comment.buffer, sizeof(comm), -1, STR_TERMINATE);

	printf("\tflags:[0x%x]\n", i1->flags);
	printf("\tname:[%s]\n", name);
	printf("\tdescription:[%s]\n", desc);
	printf("\tcomment:[%s]\n", comm);

	printf("\n");
}

/****************************************************************************
****************************************************************************/

static void display_print_info_2(PRINTER_INFO_2 *i2)
{
	fstring servername = "";
	fstring printername = "";
	fstring sharename = "";
	fstring portname = "";
	fstring drivername = "";
	fstring comment = "";
	fstring location = "";
	fstring sepfile = "";
	fstring printprocessor = "";
	fstring datatype = "";
	fstring parameters = "";
	
	rpcstr_pull(servername, i2->servername.buffer,sizeof(servername), -1, STR_TERMINATE);
	rpcstr_pull(printername, i2->printername.buffer,sizeof(printername), -1, STR_TERMINATE);
	rpcstr_pull(sharename, i2->sharename.buffer,sizeof(sharename), -1, STR_TERMINATE);
	rpcstr_pull(portname, i2->portname.buffer,sizeof(portname), -1, STR_TERMINATE);
	rpcstr_pull(drivername, i2->drivername.buffer,sizeof(drivername), -1, STR_TERMINATE);
	rpcstr_pull(comment, i2->comment.buffer,sizeof(comment), -1, STR_TERMINATE);
	rpcstr_pull(location, i2->location.buffer,sizeof(location), -1, STR_TERMINATE);
	rpcstr_pull(sepfile, i2->sepfile.buffer,sizeof(sepfile), -1, STR_TERMINATE);
	rpcstr_pull(printprocessor, i2->printprocessor.buffer,sizeof(printprocessor), -1, STR_TERMINATE);
	rpcstr_pull(datatype, i2->datatype.buffer,sizeof(datatype), -1, STR_TERMINATE);
	rpcstr_pull(parameters, i2->parameters.buffer,sizeof(parameters), -1, STR_TERMINATE);

	printf("\tservername:[%s]\n", servername);
	printf("\tprintername:[%s]\n", printername);
	printf("\tsharename:[%s]\n", sharename);
	printf("\tportname:[%s]\n", portname);
	printf("\tdrivername:[%s]\n", drivername);
	printf("\tcomment:[%s]\n", comment);
	printf("\tlocation:[%s]\n", location);
	printf("\tsepfile:[%s]\n", sepfile);
	printf("\tprintprocessor:[%s]\n", printprocessor);
	printf("\tdatatype:[%s]\n", datatype);
	printf("\tparameters:[%s]\n", parameters);
	printf("\tattributes:[0x%x]\n", i2->attributes);
	printf("\tpriority:[0x%x]\n", i2->priority);
	printf("\tdefaultpriority:[0x%x]\n", i2->defaultpriority);
	printf("\tstarttime:[0x%x]\n", i2->starttime);
	printf("\tuntiltime:[0x%x]\n", i2->untiltime);
	printf("\tstatus:[0x%x]\n", i2->status);
	printf("\tcjobs:[0x%x]\n", i2->cjobs);
	printf("\taverageppm:[0x%x]\n", i2->averageppm);

	if (i2->secdesc) 
		display_sec_desc(i2->secdesc);

	printf("\n");
}

/****************************************************************************
****************************************************************************/

static void display_print_info_3(PRINTER_INFO_3 *i3)
{
	printf("\tflags:[0x%x]\n", i3->flags);

	display_sec_desc(i3->secdesc);

	printf("\n");
}

/****************************************************************************
****************************************************************************/

static void display_print_info_7(PRINTER_INFO_7 *i7)
{
	fstring guid = "";
	rpcstr_pull(guid, i7->guid.buffer,sizeof(guid), -1, STR_TERMINATE);
	printf("\tguid:[%s]\n", guid);
	printf("\taction:[0x%x]\n", i7->action);
}


/****************************************************************************
****************************************************************************/

static WERROR cmd_spoolss_enum_printers(struct rpc_pipe_client *cli, 
                                          TALLOC_CTX *mem_ctx,
                                          int argc, const char **argv)
{
	WERROR                  result;
	uint32			info_level = 1;
	PRINTER_INFO_CTR	ctr;
	uint32			i = 0, num_printers;
	fstring name;

	if (argc > 3) 
	{
		printf("Usage: %s [level] [name]\n", argv[0]);
		return WERR_OK;
	}

	if (argc >= 2)
		info_level = atoi(argv[1]);

	if (argc == 3)
		fstrcpy(name, argv[2]);
	else {
		slprintf(name, sizeof(name)-1, "\\\\%s", cli->cli->desthost);
		strupper_m(name);
	}

	ZERO_STRUCT(ctr);

	result = rpccli_spoolss_enum_printers(cli, mem_ctx, name, PRINTER_ENUM_LOCAL, 
		info_level, &num_printers, &ctr);

	if (W_ERROR_IS_OK(result)) {

		if (!num_printers) {
			printf ("No printers returned.\n");
			goto done;
		}
	
		for (i = 0; i < num_printers; i++) {
			switch(info_level) {
			case 0:
				display_print_info_0(&ctr.printers_0[i]);
				break;
			case 1:
				display_print_info_1(&ctr.printers_1[i]);
				break;
			case 2:
				display_print_info_2(&ctr.printers_2[i]);
				break;
			case 3:
				display_print_info_3(&ctr.printers_3[i]);
				break;
			default:
				printf("unknown info level %d\n", info_level);
				goto done;
			}
		}
	}
	done:

	return result;
}

/****************************************************************************
****************************************************************************/

static void display_port_info_1(PORT_INFO_1 *i1)
{
	fstring buffer;
	
	rpcstr_pull(buffer, i1->port_name.buffer, sizeof(buffer), -1, STR_TERMINATE);
	printf("\tPort Name:\t[%s]\n", buffer);
}

/****************************************************************************
****************************************************************************/

static void display_port_info_2(PORT_INFO_2 *i2)
{
	fstring buffer;
	
	rpcstr_pull(buffer, i2->port_name.buffer, sizeof(buffer), -1, STR_TERMINATE);
	printf("\tPort Name:\t[%s]\n", buffer);
	rpcstr_pull(buffer, i2->monitor_name.buffer, sizeof(buffer), -1, STR_TERMINATE);

	printf("\tMonitor Name:\t[%s]\n", buffer);
	rpcstr_pull(buffer, i2->description.buffer, sizeof(buffer), -1, STR_TERMINATE);

	printf("\tDescription:\t[%s]\n", buffer);
	printf("\tPort Type:\t" );
	if ( i2->port_type ) {
		int comma = 0; /* hack */
		printf( "[" );
		if ( i2->port_type & PORT_TYPE_READ ) {
			printf( "Read" );
			comma = 1;
		}
		if ( i2->port_type & PORT_TYPE_WRITE ) {
			printf( "%sWrite", comma ? ", " : "" );
			comma = 1;
		}
		/* These two have slightly different interpretations
		 on 95/98/ME but I'm disregarding that for now */
		if ( i2->port_type & PORT_TYPE_REDIRECTED ) {
			printf( "%sRedirected", comma ? ", " : "" );
			comma = 1;
		}
		if ( i2->port_type & PORT_TYPE_NET_ATTACHED ) {
			printf( "%sNet-Attached", comma ? ", " : "" );
		}
		printf( "]\n" );
	} else {
		printf( "[Unset]\n" );
	}
	printf("\tReserved:\t[%d]\n", i2->reserved);
	printf("\n");
}

/****************************************************************************
****************************************************************************/

static WERROR cmd_spoolss_enum_ports(struct rpc_pipe_client *cli, 
				       TALLOC_CTX *mem_ctx, int argc, 
				       const char **argv)
{
	WERROR         		result;
	uint32                  info_level = 1;
	PORT_INFO_CTR 		ctr;
	uint32 			returned;
	
	if (argc > 2) {
		printf("Usage: %s [level]\n", argv[0]);
		return WERR_OK;
	}
	
	if (argc == 2)
		info_level = atoi(argv[1]);

	/* Enumerate ports */

	ZERO_STRUCT(ctr);

	result = rpccli_spoolss_enum_ports(cli, mem_ctx, info_level, &returned, &ctr);

	if (W_ERROR_IS_OK(result)) {
		int i;

		for (i = 0; i < returned; i++) {
			switch (info_level) {
			case 1:
				display_port_info_1(&ctr.port.info_1[i]);
				break;
			case 2:
				display_port_info_2(&ctr.port.info_2[i]);
				break;
			default:
				printf("unknown info level %d\n", info_level);
				break;
			}
		}
	}
	
	return result;
}

/****************************************************************************
****************************************************************************/

static WERROR cmd_spoolss_setprinter(struct rpc_pipe_client *cli,
                                       TALLOC_CTX *mem_ctx,
                                       int argc, const char **argv)
{
	POLICY_HND 	pol;
	WERROR		result;
	uint32 		info_level = 2;
	BOOL 		opened_hnd = False;
	PRINTER_INFO_CTR ctr;
	fstring 	printername,
			servername,
			user,
			comment;

	if (argc == 1 || argc > 3) {
		printf("Usage: %s printername comment\n", argv[0]);

		return WERR_OK;
	}

	/* Open a printer handle */
	if (argc == 3) {
		fstrcpy(comment, argv[2]);
	}

	slprintf(servername, sizeof(servername)-1, "\\\\%s", cli->cli->desthost);
	strupper_m(servername);
	slprintf(printername, sizeof(servername)-1, "%s\\%s", servername, argv[1]);
	fstrcpy(user, cli->user_name);

	/* get a printer handle */
	result = rpccli_spoolss_open_printer_ex(cli, mem_ctx, printername, "", 
				PRINTER_ALL_ACCESS, servername,
				user, &pol);
				
	if (!W_ERROR_IS_OK(result))
		goto done;

	opened_hnd = True;

	/* Get printer info */
        result = rpccli_spoolss_getprinter(cli, mem_ctx, &pol, info_level, &ctr);

        if (!W_ERROR_IS_OK(result))
                goto done;


	/* Modify the comment. */
	init_unistr(&ctr.printers_2->comment, comment);
	ctr.printers_2->devmode = NULL;
	ctr.printers_2->secdesc = NULL;

	result = rpccli_spoolss_setprinter(cli, mem_ctx, &pol, info_level, &ctr, 0);
	if (W_ERROR_IS_OK(result))
		printf("Success in setting comment.\n");

 done:
	if (opened_hnd)
		rpccli_spoolss_close_printer(cli, mem_ctx, &pol);

	return result;
}

/****************************************************************************
****************************************************************************/

static WERROR cmd_spoolss_setprintername(struct rpc_pipe_client *cli,
                                       TALLOC_CTX *mem_ctx,
                                       int argc, const char **argv)
{
	POLICY_HND 	pol;
	WERROR		result;
	uint32 		info_level = 2;
	BOOL 		opened_hnd = False;
	PRINTER_INFO_CTR ctr;
	fstring 	printername,
			servername,
			user,
			new_printername;

	if (argc == 1 || argc > 3) {
		printf("Usage: %s printername new_printername\n", argv[0]);

		return WERR_OK;
	}

	/* Open a printer handle */
	if (argc == 3) {
		fstrcpy(new_printername, argv[2]);
	}

	slprintf(servername, sizeof(servername)-1, "\\\\%s", cli->cli->desthost);
	strupper_m(servername);
	slprintf(printername, sizeof(printername)-1, "%s\\%s", servername, argv[1]);
	fstrcpy(user, cli->user_name);

	/* get a printer handle */
	result = rpccli_spoolss_open_printer_ex(cli, mem_ctx, printername, "", 
				PRINTER_ALL_ACCESS, servername,
				user, &pol);
				
	if (!W_ERROR_IS_OK(result))
		goto done;

	opened_hnd = True;

	/* Get printer info */
        result = rpccli_spoolss_getprinter(cli, mem_ctx, &pol, info_level, &ctr);

        if (!W_ERROR_IS_OK(result))
                goto done;

	/* Modify the printername. */
	init_unistr(&ctr.printers_2->printername, new_printername);
	ctr.printers_2->devmode = NULL;
	ctr.printers_2->secdesc = NULL;

	result = rpccli_spoolss_setprinter(cli, mem_ctx, &pol, info_level, &ctr, 0);
	if (W_ERROR_IS_OK(result))
		printf("Success in setting printername.\n");

 done:
	if (opened_hnd)
		rpccli_spoolss_close_printer(cli, mem_ctx, &pol);

	return result;
}

/****************************************************************************
****************************************************************************/

static WERROR cmd_spoolss_getprinter(struct rpc_pipe_client *cli,
                                       TALLOC_CTX *mem_ctx,
                                       int argc, const char **argv)
{
	POLICY_HND 	pol;
	WERROR          result;
	uint32 		info_level = 1;
	BOOL 		opened_hnd = False;
	PRINTER_INFO_CTR ctr;
	fstring 	printername,
			servername,
			user;

	if (argc == 1 || argc > 3) {
		printf("Usage: %s <printername> [level]\n", argv[0]);
		return WERR_OK;
	}

	/* Open a printer handle */
	if (argc == 3) {
		info_level = atoi(argv[2]);
	}

	slprintf(servername, sizeof(servername)-1, "\\\\%s", cli->cli->desthost);
	strupper_m(servername);
	slprintf(printername, sizeof(printername)-1, "%s\\%s", servername, argv[1]);
	fstrcpy(user, cli->user_name);
	
	/* get a printer handle */

	result = rpccli_spoolss_open_printer_ex(cli, mem_ctx, printername, 
					     "", MAXIMUM_ALLOWED_ACCESS, 
					     servername, user, &pol);

	if (!W_ERROR_IS_OK(result))
		goto done;
 
	opened_hnd = True;

	/* Get printer info */

	result = rpccli_spoolss_getprinter(cli, mem_ctx, &pol, info_level, &ctr);

	if (!W_ERROR_IS_OK(result))
		goto done;

	/* Display printer info */

	switch (info_level) {
	case 0: 
		display_print_info_0(ctr.printers_0);
		break;
	case 1:
		display_print_info_1(ctr.printers_1);
		break;
	case 2:
		display_print_info_2(ctr.printers_2);
		break;
	case 3:
		display_print_info_3(ctr.printers_3);
		break;
	case 7:
		display_print_info_7(ctr.printers_7);
		break;
	default:
		printf("unknown info level %d\n", info_level);
		break;
	}

 done: 
	if (opened_hnd) 
		rpccli_spoolss_close_printer(cli, mem_ctx, &pol);

	return result;
}

/****************************************************************************
****************************************************************************/

static void display_reg_value(REGISTRY_VALUE value)
{
	pstring text;

	switch(value.type) {
	case REG_DWORD:
		printf("%s: REG_DWORD: 0x%08x\n", value.valuename, 
		       *((uint32 *) value.data_p));
		break;
	case REG_SZ:
		rpcstr_pull(text, value.data_p, sizeof(text), value.size,
			    STR_TERMINATE);
		printf("%s: REG_SZ: %s\n", value.valuename, text);
		break;
	case REG_BINARY: {
		char *hex = hex_encode(NULL, value.data_p, value.size);
		size_t i, len;
		printf("%s: REG_BINARY:", value.valuename);
		len = strlen(hex);
		for (i=0; i<len; i++) {
			if (hex[i] == '\0') {
				break;
			}
			if (i%40 == 0) {
				putchar('\n');
			}
			putchar(hex[i]);
		}
		TALLOC_FREE(hex);
		putchar('\n');
		break;
	}
	case REG_MULTI_SZ: {
		uint16 *curstr = (uint16 *) value.data_p;
		uint8 *start = value.data_p;
		printf("%s: REG_MULTI_SZ:\n", value.valuename);
		while (((uint8 *) curstr < start + value.size)) {
			rpcstr_pull(text, curstr, sizeof(text), -1, 
				    STR_TERMINATE);
			printf("  %s\n", *text != 0 ? text : "NULL");
			curstr += strlen(text) + 1;
		}
	}
	break;
	default:
		printf("%s: unknown type %d\n", value.valuename, value.type);
	}
	
}

/****************************************************************************
****************************************************************************/

static WERROR cmd_spoolss_getprinterdata(struct rpc_pipe_client *cli,
					   TALLOC_CTX *mem_ctx,
					   int argc, const char **argv)
{
	POLICY_HND 	pol;
	WERROR          result;
	BOOL 		opened_hnd = False;
	fstring 	printername,
			servername,
			user;
	const char *valuename;
	REGISTRY_VALUE value;

	if (argc != 3) {
		printf("Usage: %s <printername> <valuename>\n", argv[0]);
		printf("<printername> of . queries print server\n");
		return WERR_OK;
	}
	valuename = argv[2];

	/* Open a printer handle */

	slprintf(servername, sizeof(servername)-1, "\\\\%s", cli->cli->desthost);
	strupper_m(servername);
	if (strncmp(argv[1], ".", sizeof(".")) == 0)
		fstrcpy(printername, servername);
	else
		slprintf(printername, sizeof(servername)-1, "%s\\%s", 
			  servername, argv[1]);
	fstrcpy(user, cli->user_name);
	
	/* get a printer handle */

	result = rpccli_spoolss_open_printer_ex(cli, mem_ctx, printername, 
					     "", MAXIMUM_ALLOWED_ACCESS, 
					     servername, user, &pol);

	if (!W_ERROR_IS_OK(result))
		goto done;
 
	opened_hnd = True;

	/* Get printer info */

	result = rpccli_spoolss_getprinterdata(cli, mem_ctx, &pol, valuename, &value);

	if (!W_ERROR_IS_OK(result))
		goto done;

	/* Display printer data */

	fstrcpy(value.valuename, valuename);
	display_reg_value(value);
	

 done: 
	if (opened_hnd) 
		rpccli_spoolss_close_printer(cli, mem_ctx, &pol);

	return result;
}

/****************************************************************************
****************************************************************************/

static WERROR cmd_spoolss_getprinterdataex(struct rpc_pipe_client *cli,
					     TALLOC_CTX *mem_ctx,
					     int argc, const char **argv)
{
	POLICY_HND 	pol;
	WERROR          result;
	BOOL 		opened_hnd = False;
	fstring 	printername,
			servername,
			user;
	const char *valuename, *keyname;
	REGISTRY_VALUE value;

	if (argc != 4) {
		printf("Usage: %s <printername> <keyname> <valuename>\n", 
		       argv[0]);
		printf("<printername> of . queries print server\n");
		return WERR_OK;
	}
	valuename = argv[3];
	keyname = argv[2];

	/* Open a printer handle */

	slprintf(servername, sizeof(servername)-1, "\\\\%s", cli->cli->desthost);
	strupper_m(servername);
	if (strncmp(argv[1], ".", sizeof(".")) == 0)
		fstrcpy(printername, servername);
	else
		slprintf(printername, sizeof(printername)-1, "%s\\%s", 
			  servername, argv[1]);
	fstrcpy(user, cli->user_name);
	
	/* get a printer handle */

	result = rpccli_spoolss_open_printer_ex(cli, mem_ctx, printername, 
					     "", MAXIMUM_ALLOWED_ACCESS, 
					     servername, user, &pol);

	if (!W_ERROR_IS_OK(result))
		goto done;
 
	opened_hnd = True;

	/* Get printer info */

	result = rpccli_spoolss_getprinterdataex(cli, mem_ctx, &pol, keyname, 
		valuename, &value);

	if (!W_ERROR_IS_OK(result))
		goto done;

	/* Display printer data */

	fstrcpy(value.valuename, valuename);
	display_reg_value(value);
	

 done: 
	if (opened_hnd) 
		rpccli_spoolss_close_printer(cli, mem_ctx, &pol);

	return result;
}

/****************************************************************************
****************************************************************************/

static void display_print_driver_1(DRIVER_INFO_1 *i1)
{
	fstring name;
	if (i1 == NULL)
		return;

	rpcstr_pull(name, i1->name.buffer, sizeof(name), -1, STR_TERMINATE);

	printf ("Printer Driver Info 1:\n");
	printf ("\tDriver Name: [%s]\n\n", name);
	
	return;
}

/****************************************************************************
****************************************************************************/

static void display_print_driver_2(DRIVER_INFO_2 *i1)
{
	fstring name;
	fstring architecture;
	fstring driverpath;
	fstring datafile;
	fstring configfile;
	if (i1 == NULL)
		return;

	rpcstr_pull(name, i1->name.buffer, sizeof(name), -1, STR_TERMINATE);
	rpcstr_pull(architecture, i1->architecture.buffer, sizeof(architecture), -1, STR_TERMINATE);
	rpcstr_pull(driverpath, i1->driverpath.buffer, sizeof(driverpath), -1, STR_TERMINATE);
	rpcstr_pull(datafile, i1->datafile.buffer, sizeof(datafile), -1, STR_TERMINATE);
	rpcstr_pull(configfile, i1->configfile.buffer, sizeof(configfile), -1, STR_TERMINATE);

	printf ("Printer Driver Info 2:\n");
	printf ("\tVersion: [%x]\n", i1->version);
	printf ("\tDriver Name: [%s]\n", name);
	printf ("\tArchitecture: [%s]\n", architecture);
	printf ("\tDriver Path: [%s]\n", driverpath);
	printf ("\tDatafile: [%s]\n", datafile);
	printf ("\tConfigfile: [%s]\n\n", configfile);

	return;
}

/****************************************************************************
****************************************************************************/

static void display_print_driver_3(DRIVER_INFO_3 *i1)
{
	fstring name = "";
	fstring architecture = "";
	fstring driverpath = "";
	fstring datafile = "";
	fstring configfile = "";
	fstring helpfile = "";
	fstring dependentfiles = "";
	fstring monitorname = "";
	fstring defaultdatatype = "";
	
	int length=0;
	BOOL valid = True;
	
	if (i1 == NULL)
		return;

	rpcstr_pull(name, i1->name.buffer, sizeof(name), -1, STR_TERMINATE);
	rpcstr_pull(architecture, i1->architecture.buffer, sizeof(architecture), -1, STR_TERMINATE);
	rpcstr_pull(driverpath, i1->driverpath.buffer, sizeof(driverpath), -1, STR_TERMINATE);
	rpcstr_pull(datafile, i1->datafile.buffer, sizeof(datafile), -1, STR_TERMINATE);
	rpcstr_pull(configfile, i1->configfile.buffer, sizeof(configfile), -1, STR_TERMINATE);
	rpcstr_pull(helpfile, i1->helpfile.buffer, sizeof(helpfile), -1, STR_TERMINATE);
	rpcstr_pull(monitorname, i1->monitorname.buffer, sizeof(monitorname), -1, STR_TERMINATE);
	rpcstr_pull(defaultdatatype, i1->defaultdatatype.buffer, sizeof(defaultdatatype), -1, STR_TERMINATE);

	printf ("Printer Driver Info 3:\n");
	printf ("\tVersion: [%x]\n", i1->version);
	printf ("\tDriver Name: [%s]\n",name);
	printf ("\tArchitecture: [%s]\n", architecture);
	printf ("\tDriver Path: [%s]\n", driverpath);
	printf ("\tDatafile: [%s]\n", datafile);
	printf ("\tConfigfile: [%s]\n", configfile);
	printf ("\tHelpfile: [%s]\n\n", helpfile);

	while (valid)
	{
		rpcstr_pull(dependentfiles, i1->dependentfiles+length, sizeof(dependentfiles), -1, STR_TERMINATE);
		
		length+=strlen(dependentfiles)+1;
		
		if (strlen(dependentfiles) > 0)
		{
			printf ("\tDependentfiles: [%s]\n", dependentfiles);
		}
		else
		{
			valid = False;
		}
	}
	
	printf ("\n");

	printf ("\tMonitorname: [%s]\n", monitorname);
	printf ("\tDefaultdatatype: [%s]\n\n", defaultdatatype);

	return;	
}

/****************************************************************************
****************************************************************************/

static WERROR cmd_spoolss_getdriver(struct rpc_pipe_client *cli, 
                                      TALLOC_CTX *mem_ctx,
                                      int argc, const char **argv)
{
	POLICY_HND 	pol;
	WERROR          werror;
	uint32		info_level = 3;
	BOOL 		opened_hnd = False;
	PRINTER_DRIVER_CTR 	ctr;
	fstring 	printername, 
			servername, 
			user;
	uint32		i;
	BOOL		success = False;

	if ((argc == 1) || (argc > 3)) 
	{
		printf("Usage: %s <printername> [level]\n", argv[0]);
		return WERR_OK;
	}

	/* get the arguments need to open the printer handle */
	slprintf(servername, sizeof(servername)-1, "\\\\%s", cli->cli->desthost);
	strupper_m(servername);
	fstrcpy(user, cli->user_name);
	slprintf(printername, sizeof(servername)-1, "%s\\%s", servername, argv[1]);
	if (argc == 3)
		info_level = atoi(argv[2]);

	/* Open a printer handle */

	werror = rpccli_spoolss_open_printer_ex(cli, mem_ctx, printername, "", 
					     PRINTER_ACCESS_USE,
					     servername, user, &pol);

	if (!W_ERROR_IS_OK(werror)) {
		printf("Error opening printer handle for %s!\n", printername);
		return werror;
	}

	opened_hnd = True;

	/* loop through and print driver info level for each architecture */

	for (i=0; archi_table[i].long_archi!=NULL; i++) {

		werror = rpccli_spoolss_getprinterdriver( cli, mem_ctx, &pol, info_level, 
			archi_table[i].long_archi, archi_table[i].version,
			&ctr);

		if (!W_ERROR_IS_OK(werror))
			continue;
		
		/* need at least one success */
		
		success = True;
			
		printf ("\n[%s]\n", archi_table[i].long_archi);

		switch (info_level) {
		case 1:
			display_print_driver_1 (ctr.info1);
			break;
		case 2:
			display_print_driver_2 (ctr.info2);
			break;
		case 3:
			display_print_driver_3 (ctr.info3);
			break;
		default:
			printf("unknown info level %d\n", info_level);
			break;
		}
	}
	
	/* Cleanup */

	if (opened_hnd)
		rpccli_spoolss_close_printer (cli, mem_ctx, &pol);
	
	if ( success )
		werror = WERR_OK;
		
	return werror;
}

/****************************************************************************
****************************************************************************/

static WERROR cmd_spoolss_enum_drivers(struct rpc_pipe_client *cli, 
                                         TALLOC_CTX *mem_ctx,
                                         int argc, const char **argv)
{
	WERROR werror = WERR_OK;
	uint32          info_level = 1;
	PRINTER_DRIVER_CTR 	ctr;
	uint32		i, j,
			returned;

	if (argc > 2) {
		printf("Usage: enumdrivers [level]\n");
		return WERR_OK;
	}

	if (argc == 2)
		info_level = atoi(argv[1]);


	/* loop through and print driver info level for each architecture */
	for (i=0; archi_table[i].long_archi!=NULL; i++) {
		/* check to see if we already asked for this architecture string */

		if ( i>0 && strequal(archi_table[i].long_archi, archi_table[i-1].long_archi) )
			continue;

		werror = rpccli_spoolss_enumprinterdrivers(
			cli, mem_ctx, info_level, 
			archi_table[i].long_archi, &returned, &ctr);

		if (W_ERROR_V(werror) == W_ERROR_V(WERR_INVALID_ENVIRONMENT)) {
			printf ("Server does not support environment [%s]\n", 
				archi_table[i].long_archi);
			werror = WERR_OK;
			continue;
		}

		if (returned == 0)
			continue;
			
		if (!W_ERROR_IS_OK(werror)) {
			printf ("Error getting driver for environment [%s] - %d\n",
				archi_table[i].long_archi, W_ERROR_V(werror));
			continue;
		}
		
		printf ("\n[%s]\n", archi_table[i].long_archi);
		switch (info_level) 
		{
			
		case 1:
			for (j=0; j < returned; j++) {
				display_print_driver_1 (&ctr.info1[j]);
			}
			break;
		case 2:
			for (j=0; j < returned; j++) {
				display_print_driver_2 (&ctr.info2[j]);
			}
			break;
		case 3:
			for (j=0; j < returned; j++) {
				display_print_driver_3 (&ctr.info3[j]);
			}
			break;
		default:
			printf("unknown info level %d\n", info_level);
			return WERR_UNKNOWN_LEVEL;
		}
	}
	
	return werror;
}

/****************************************************************************
****************************************************************************/

static void display_printdriverdir_1(DRIVER_DIRECTORY_1 *i1)
{
        fstring name;
        if (i1 == NULL)
                return;
 
	rpcstr_pull(name, i1->name.buffer, sizeof(name), -1, STR_TERMINATE);
 
	printf ("\tDirectory Name:[%s]\n", name);
}

/****************************************************************************
****************************************************************************/

static WERROR cmd_spoolss_getdriverdir(struct rpc_pipe_client *cli, 
                                         TALLOC_CTX *mem_ctx,
                                         int argc, const char **argv)
{
	WERROR result;
	fstring			env;
	DRIVER_DIRECTORY_CTR	ctr;

	if (argc > 2) {
		printf("Usage: %s [environment]\n", argv[0]);
		return WERR_OK;
	}

	/* Get the arguments need to open the printer handle */

	if (argc == 2)
		fstrcpy (env, argv[1]);
	else
		fstrcpy (env, "Windows NT x86");

	/* Get the directory.  Only use Info level 1 */

	result = rpccli_spoolss_getprinterdriverdir(cli, mem_ctx, 1, env, &ctr);

	if (W_ERROR_IS_OK(result))
		display_printdriverdir_1(ctr.info1);

	return result;
}

/****************************************************************************
****************************************************************************/

void set_drv_info_3_env (DRIVER_INFO_3 *info, const char *arch)
{

	int i;
	
	for (i=0; archi_table[i].long_archi != NULL; i++) 
	{
		if (strcmp(arch, archi_table[i].short_archi) == 0)
		{
			info->version = archi_table[i].version;
			init_unistr (&info->architecture, archi_table[i].long_archi);
			break;
		}
	}
	
	if (archi_table[i].long_archi == NULL)
	{
		DEBUG(0, ("set_drv_info_3_env: Unknown arch [%s]\n", arch));
	}
	
	return;
}


/**************************************************************************
 wrapper for strtok to get the next parameter from a delimited list.
 Needed to handle the empty parameter string denoted by "NULL"
 *************************************************************************/
 
static char* get_driver_3_param (char* str, const char* delim, UNISTR* dest)
{
	char	*ptr;

	/* get the next token */
	ptr = strtok(str, delim);

	/* a string of 'NULL' is used to represent an empty
	   parameter because two consecutive delimiters
	   will not return an empty string.  See man strtok(3)
	   for details */
	if (ptr && (StrCaseCmp(ptr, "NULL") == 0))
		ptr = NULL;

	if (dest != NULL)
		init_unistr(dest, ptr);	

	return ptr;
}

/********************************************************************************
 fill in the members of a DRIVER_INFO_3 struct using a character 
 string in the form of
 	 <Long Printer Name>:<Driver File Name>:<Data File Name>:\
	     <Config File Name>:<Help File Name>:<Language Monitor Name>:\
	     <Default Data Type>:<Comma Separated list of Files> 
 *******************************************************************************/
static BOOL init_drv_info_3_members ( TALLOC_CTX *mem_ctx, DRIVER_INFO_3 *info, 
                                      char *args )
{
	char	*str, *str2;
	uint32	len, i;
	
	/* fill in the UNISTR fields */
	str = get_driver_3_param (args, ":", &info->name);
	str = get_driver_3_param (NULL, ":", &info->driverpath);
	str = get_driver_3_param (NULL, ":", &info->datafile);
	str = get_driver_3_param (NULL, ":", &info->configfile);
	str = get_driver_3_param (NULL, ":", &info->helpfile);
	str = get_driver_3_param (NULL, ":", &info->monitorname);
	str = get_driver_3_param (NULL, ":", &info->defaultdatatype);

	/* <Comma Separated List of Dependent Files> */
	str2 = get_driver_3_param (NULL, ":", NULL); /* save the beginning of the string */
	str = str2;			

	/* begin to strip out each filename */
	str = strtok(str, ",");		
	len = 0;
	while (str != NULL)
	{
		/* keep a cumlative count of the str lengths */
		len += strlen(str)+1;
		str = strtok(NULL, ",");
	}

	/* allocate the space; add one extra slot for a terminating NULL.
	   Each filename is NULL terminated and the end contains a double
	   NULL */
	if ((info->dependentfiles=TALLOC_ARRAY(mem_ctx, uint16, len+1)) == NULL)
	{
		DEBUG(0,("init_drv_info_3_members: Unable to malloc memory for dependenfiles\n"));
		return False;
	}
	for (i=0; i<len; i++)
	{
		SSVAL(&info->dependentfiles[i], 0, str2[i]);
	}
	info->dependentfiles[len] = '\0';

	return True;
}


/****************************************************************************
****************************************************************************/

static WERROR cmd_spoolss_addprinterdriver(struct rpc_pipe_client *cli, 
                                             TALLOC_CTX *mem_ctx,
                                             int argc, const char **argv)
{
	WERROR result;
	uint32                  level = 3;
	PRINTER_DRIVER_CTR	ctr;
	DRIVER_INFO_3		info3;
	const char		*arch;
	fstring			driver_name;
	char 			*driver_args;

	/* parse the command arguements */
	if (argc != 3 && argc != 4)
	{
		printf ("Usage: %s <Environment> \\\n", argv[0]);
		printf ("\t<Long Printer Name>:<Driver File Name>:<Data File Name>:\\\n");
    		printf ("\t<Config File Name>:<Help File Name>:<Language Monitor Name>:\\\n");
	    	printf ("\t<Default Data Type>:<Comma Separated list of Files> \\\n");
		printf ("\t[version]\n");

            return WERR_OK;
        }
		
	/* Fill in the DRIVER_INFO_3 struct */
	ZERO_STRUCT(info3);
	if (!(arch = cmd_spoolss_get_short_archi(argv[1])))
	{
		printf ("Error Unknown architechture [%s]\n", argv[1]);
		return WERR_INVALID_PARAM;
	}
	else
		set_drv_info_3_env(&info3, arch);

	driver_args = talloc_strdup( mem_ctx, argv[2] );
	if (!init_drv_info_3_members(mem_ctx, &info3, driver_args ))
	{
		printf ("Error Invalid parameter list - %s.\n", argv[2]);
		return WERR_INVALID_PARAM;
	}

	/* if printer driver version specified, override the default version
	 * used by the architecture.  This allows installation of Windows
	 * 2000 (version 3) printer drivers. */
	if (argc == 4)
	{
		info3.version = atoi(argv[3]);
	}


	ctr.info3 = &info3;
	result = rpccli_spoolss_addprinterdriver (cli, mem_ctx, level, &ctr);

	if (W_ERROR_IS_OK(result)) {
		rpcstr_pull(driver_name, info3.name.buffer, 
			    sizeof(driver_name), -1, STR_TERMINATE);
		printf ("Printer Driver %s successfully installed.\n",
			driver_name);
	}

	return result;
}


/****************************************************************************
****************************************************************************/

static WERROR cmd_spoolss_addprinterex(struct rpc_pipe_client *cli, 
                                         TALLOC_CTX *mem_ctx,
                                         int argc, const char **argv)
{
	WERROR result;
	uint32			level = 2;
	PRINTER_INFO_CTR	ctr;
	PRINTER_INFO_2		info2;
	fstring			servername;
	
	/* parse the command arguements */
	if (argc != 5)
	{
		printf ("Usage: %s <name> <shared name> <driver> <port>\n", argv[0]);
		return WERR_OK;
        }
	
        slprintf(servername, sizeof(servername)-1, "\\\\%s", cli->cli->desthost);
        strupper_m(servername);

	/* Fill in the DRIVER_INFO_2 struct */
	ZERO_STRUCT(info2);
	
	init_unistr( &info2.printername,	argv[1]);
	init_unistr( &info2.sharename, 		argv[2]);
	init_unistr( &info2.drivername,		argv[3]);
	init_unistr( &info2.portname,		argv[4]);
	init_unistr( &info2.comment,		"Created by rpcclient");
	init_unistr( &info2.printprocessor, 	"winprint");
	init_unistr( &info2.datatype,		"RAW");
	info2.devmode = 	NULL;
	info2.secdesc = 	NULL;
	info2.attributes 	= PRINTER_ATTRIBUTE_SHARED;
	info2.priority 		= 0;
	info2.defaultpriority	= 0;
	info2.starttime		= 0;
	info2.untiltime		= 0;
	
	/* These three fields must not be used by AddPrinter() 
	   as defined in the MS Platform SDK documentation..  
	   --jerry
	info2.status		= 0;
	info2.cjobs		= 0;
	info2.averageppm	= 0;
	*/

	ctr.printers_2 = &info2;
	result = rpccli_spoolss_addprinterex (cli, mem_ctx, level, &ctr);

	if (W_ERROR_IS_OK(result))
		printf ("Printer %s successfully installed.\n", argv[1]);

	return result;
}

/****************************************************************************
****************************************************************************/

static WERROR cmd_spoolss_setdriver(struct rpc_pipe_client *cli, 
                                      TALLOC_CTX *mem_ctx,
                                      int argc, const char **argv)
{
	POLICY_HND		pol;
	WERROR                  result;
	uint32			level = 2;
	BOOL			opened_hnd = False;
	PRINTER_INFO_CTR	ctr;
	PRINTER_INFO_2		info2;
	fstring			servername,
				printername,
				user;
	
	/* parse the command arguements */
	if (argc != 3)
	{
		printf ("Usage: %s <printer> <driver>\n", argv[0]);
		return WERR_OK;
        }

	slprintf(servername, sizeof(servername)-1, "\\\\%s", cli->cli->desthost);
	strupper_m(servername);
	slprintf(printername, sizeof(printername)-1, "%s\\%s", servername, argv[1]);
	fstrcpy(user, cli->user_name);

	/* Get a printer handle */

	result = rpccli_spoolss_open_printer_ex(cli, mem_ctx, printername, "", 
					     PRINTER_ALL_ACCESS,
					     servername, user, &pol);

	if (!W_ERROR_IS_OK(result))
		goto done;

	opened_hnd = True;

	/* Get printer info */

	ZERO_STRUCT (info2);
	ctr.printers_2 = &info2;

	result = rpccli_spoolss_getprinter(cli, mem_ctx, &pol, level, &ctr);

	if (!W_ERROR_IS_OK(result)) {
		printf ("Unable to retrieve printer information!\n");
		goto done;
	}

	/* Set the printer driver */

	init_unistr(&ctr.printers_2->drivername, argv[2]);

	result = rpccli_spoolss_setprinter(cli, mem_ctx, &pol, level, &ctr, 0);

	if (!W_ERROR_IS_OK(result)) {
		printf("SetPrinter call failed!\n");
		goto done;;
	}

	printf("Succesfully set %s to driver %s.\n", argv[1], argv[2]);

done:
	/* Cleanup */

	if (opened_hnd)
		rpccli_spoolss_close_printer(cli, mem_ctx, &pol);

	return result;
}


/****************************************************************************
****************************************************************************/

static WERROR cmd_spoolss_deletedriverex(struct rpc_pipe_client *cli, 
                                         TALLOC_CTX *mem_ctx,
                                         int argc, const char **argv)
{
	WERROR result, ret = WERR_UNKNOWN_PRINTER_DRIVER;
 
	int   i;
	int vers = -1;
 
	const char *arch = NULL;
 
	/* parse the command arguements */
	if (argc < 2 || argc > 4) {
		printf ("Usage: %s <driver> [arch] [version]\n", argv[0]);
		return WERR_OK;
	}

	if (argc >= 3)
		arch = argv[2];
	if (argc == 4)
		vers = atoi (argv[3]);
 
 
	/* delete the driver for all architectures */
	for (i=0; archi_table[i].long_archi; i++) {

		if (arch &&  !strequal( archi_table[i].long_archi, arch)) 
			continue;

		if (vers >= 0 && archi_table[i].version != vers)
			continue;

		/* make the call to remove the driver */
		result = rpccli_spoolss_deleteprinterdriverex(
			cli, mem_ctx, archi_table[i].long_archi, argv[1], archi_table[i].version); 

		if ( !W_ERROR_IS_OK(result) ) 
		{
			if ( !W_ERROR_EQUAL(result, WERR_UNKNOWN_PRINTER_DRIVER) ) {
				printf ("Failed to remove driver %s for arch [%s] (version: %d): %s\n", 
					argv[1], archi_table[i].long_archi, archi_table[i].version, dos_errstr(result));
			}
		} 
		else 
		{
			printf ("Driver %s and files removed for arch [%s] (version: %d).\n", argv[1], 
			archi_table[i].long_archi, archi_table[i].version);
			ret = WERR_OK;
		}
	}
  
	return ret;
}


/****************************************************************************
****************************************************************************/

static WERROR cmd_spoolss_deletedriver(struct rpc_pipe_client *cli, 
                                         TALLOC_CTX *mem_ctx,
                                         int argc, const char **argv)
{
	WERROR result = WERR_OK;
	fstring			servername;
	int			i;
	
	/* parse the command arguements */
	if (argc != 2) {
		printf ("Usage: %s <driver>\n", argv[0]);
		return WERR_OK;
        }

	slprintf(servername, sizeof(servername)-1, "\\\\%s", cli->cli->desthost);
	strupper_m(servername);

	/* delete the driver for all architectures */
	for (i=0; archi_table[i].long_archi; i++) {
		/* make the call to remove the driver */
		result = rpccli_spoolss_deleteprinterdriver(
			cli, mem_ctx, archi_table[i].long_archi, argv[1]);

		if ( !W_ERROR_IS_OK(result) ) {
			if ( !W_ERROR_EQUAL(result, WERR_UNKNOWN_PRINTER_DRIVER) ) {
				printf ("Failed to remove driver %s for arch [%s] - error 0x%x!\n", 
					argv[1], archi_table[i].long_archi, 
					W_ERROR_V(result));
			}
		} else {
			printf ("Driver %s removed for arch [%s].\n", argv[1], 
				archi_table[i].long_archi);
		}
	}
		
	return result;
}

/****************************************************************************
****************************************************************************/

static WERROR cmd_spoolss_getprintprocdir(struct rpc_pipe_client *cli, 
					    TALLOC_CTX *mem_ctx,
					    int argc, const char **argv)
{
	WERROR result;
	char *servername = NULL, *environment = NULL;
	fstring procdir;
	
	/* parse the command arguements */
	if (argc > 2) {
		printf ("Usage: %s [environment]\n", argv[0]);
		return WERR_OK;
        }

	if (asprintf(&servername, "\\\\%s", cli->cli->desthost) < 0)
		return WERR_NOMEM;
	strupper_m(servername);

	if (asprintf(&environment, "%s", (argc == 2) ? argv[1] : 
		     PRINTER_DRIVER_ARCHITECTURE) < 0) {
		SAFE_FREE(servername);
		return WERR_NOMEM;
	}

	result = rpccli_spoolss_getprintprocessordirectory(
		cli, mem_ctx, servername, environment, procdir);

	if (W_ERROR_IS_OK(result))
		printf("%s\n", procdir);

	SAFE_FREE(servername);
	SAFE_FREE(environment);

	return result;
}

/****************************************************************************
****************************************************************************/

static WERROR cmd_spoolss_addform(struct rpc_pipe_client *cli, TALLOC_CTX *mem_ctx,
				    int argc, const char **argv)
{
	POLICY_HND handle;
	WERROR werror;
	char *servername = NULL, *printername = NULL;
	FORM form;
	BOOL got_handle = False;
	
	/* Parse the command arguements */

	if (argc != 3) {
		printf ("Usage: %s <printer> <formname>\n", argv[0]);
		return WERR_OK;
        }
	
	/* Get a printer handle */

	asprintf(&servername, "\\\\%s", cli->cli->desthost);
	strupper_m(servername);
	asprintf(&printername, "%s\\%s", servername, argv[1]);

	werror = rpccli_spoolss_open_printer_ex(cli, mem_ctx, printername, "", 
					     PRINTER_ALL_ACCESS, 
					     servername, cli->user_name, &handle);

	if (!W_ERROR_IS_OK(werror))
		goto done;

	got_handle = True;

	/* Dummy up some values for the form data */

	form.flags = FORM_USER;
	form.size_x = form.size_y = 100;
	form.left = 0;
	form.top = 10;
	form.right = 20;
	form.bottom = 30;

	init_unistr2(&form.name, argv[2], UNI_STR_TERMINATE);

	/* Add the form */


	werror = rpccli_spoolss_addform(cli, mem_ctx, &handle, 1, &form);

 done:
	if (got_handle)
		rpccli_spoolss_close_printer(cli, mem_ctx, &handle);

	SAFE_FREE(servername);
	SAFE_FREE(printername);

	return werror;
}

/****************************************************************************
****************************************************************************/

static WERROR cmd_spoolss_setform(struct rpc_pipe_client *cli, TALLOC_CTX *mem_ctx,
				    int argc, const char **argv)
{
	POLICY_HND handle;
	WERROR werror;
	char *servername = NULL, *printername = NULL;
	FORM form;
	BOOL got_handle = False;
	
	/* Parse the command arguements */

	if (argc != 3) {
		printf ("Usage: %s <printer> <formname>\n", argv[0]);
		return WERR_OK;
        }
	
	/* Get a printer handle */

	asprintf(&servername, "\\\\%s", cli->cli->desthost);
	strupper_m(servername);
	asprintf(&printername, "%s\\%s", servername, argv[1]);

	werror = rpccli_spoolss_open_printer_ex(
		cli, mem_ctx, printername, "", MAXIMUM_ALLOWED_ACCESS, 
		servername, cli->user_name, &handle);

	if (!W_ERROR_IS_OK(werror))
		goto done;

	got_handle = True;

	/* Dummy up some values for the form data */

	form.flags = FORM_PRINTER;
	form.size_x = form.size_y = 100;
	form.left = 0;
	form.top = 1000;
	form.right = 2000;
	form.bottom = 3000;

	init_unistr2(&form.name, argv[2], UNI_STR_TERMINATE);

	/* Set the form */

	werror = rpccli_spoolss_setform(cli, mem_ctx, &handle, 1, argv[2], &form);

 done:
	if (got_handle)
		rpccli_spoolss_close_printer(cli, mem_ctx, &handle);

	SAFE_FREE(servername);
	SAFE_FREE(printername);

	return werror;
}

/****************************************************************************
****************************************************************************/

static const char *get_form_flag(int form_flag)
{
	switch (form_flag) {
	case FORM_USER:
		return "FORM_USER";
	case FORM_BUILTIN:
		return "FORM_BUILTIN";
	case FORM_PRINTER:
		return "FORM_PRINTER";
	default:
		return "unknown";
	}
}

/****************************************************************************
****************************************************************************/

static void display_form(FORM_1 *form)
{
	fstring form_name = "";

	if (form->name.buffer)
		rpcstr_pull(form_name, form->name.buffer,
			    sizeof(form_name), -1, STR_TERMINATE);

	printf("%s\n" \
		"\tflag: %s (%d)\n" \
		"\twidth: %d, length: %d\n" \
		"\tleft: %d, right: %d, top: %d, bottom: %d\n\n", 
		form_name, get_form_flag(form->flag), form->flag,
		form->width, form->length, 
		form->left, form->right, 
		form->top, form->bottom);
}

/****************************************************************************
****************************************************************************/

static WERROR cmd_spoolss_getform(struct rpc_pipe_client *cli, TALLOC_CTX *mem_ctx,
				    int argc, const char **argv)
{
	POLICY_HND handle;
	WERROR werror;
	char *servername = NULL, *printername = NULL;
	FORM_1 form;
	BOOL got_handle = False;
	
	/* Parse the command arguements */

	if (argc != 3) {
		printf ("Usage: %s <printer> <formname>\n", argv[0]);
		return WERR_OK;
        }
	
	/* Get a printer handle */

	asprintf(&servername, "\\\\%s", cli->cli->desthost);
	strupper_m(servername);
	asprintf(&printername, "%s\\%s", servername, argv[1]);

	werror = rpccli_spoolss_open_printer_ex(
		cli, mem_ctx, printername, "", MAXIMUM_ALLOWED_ACCESS, 
		servername, cli->user_name, &handle);

	if (!W_ERROR_IS_OK(werror))
		goto done;

	got_handle = True;

	/* Get the form */

	werror = rpccli_spoolss_getform(cli, mem_ctx, &handle, argv[2], 1, &form);

	if (!W_ERROR_IS_OK(werror))
		goto done;

	display_form(&form);

 done:
	if (got_handle)
		rpccli_spoolss_close_printer(cli, mem_ctx, &handle);

	SAFE_FREE(servername);
	SAFE_FREE(printername);

	return werror;
}

/****************************************************************************
****************************************************************************/

static WERROR cmd_spoolss_deleteform(struct rpc_pipe_client *cli, 
				       TALLOC_CTX *mem_ctx, int argc, 
				       const char **argv)
{
	POLICY_HND handle;
	WERROR werror;
	char *servername = NULL, *printername = NULL;
	BOOL got_handle = False;
	
	/* Parse the command arguements */

	if (argc != 3) {
		printf ("Usage: %s <printer> <formname>\n", argv[0]);
		return WERR_OK;
        }
	
	/* Get a printer handle */

	asprintf(&servername, "\\\\%s", cli->cli->desthost);
	strupper_m(servername);
	asprintf(&printername, "%s\\%s", servername, argv[1]);

	werror = rpccli_spoolss_open_printer_ex(
		cli, mem_ctx, printername, "", MAXIMUM_ALLOWED_ACCESS, 
		servername, cli->user_name, &handle);

	if (!W_ERROR_IS_OK(werror))
		goto done;

	got_handle = True;

	/* Delete the form */

	werror = rpccli_spoolss_deleteform(cli, mem_ctx, &handle, argv[2]);

 done:
	if (got_handle)
		rpccli_spoolss_close_printer(cli, mem_ctx, &handle);

	SAFE_FREE(servername);
	SAFE_FREE(printername);

	return werror;
}

/****************************************************************************
****************************************************************************/

static WERROR cmd_spoolss_enum_forms(struct rpc_pipe_client *cli, 
				       TALLOC_CTX *mem_ctx, int argc, 
				       const char **argv)
{
	POLICY_HND handle;
	WERROR werror;
	char *servername = NULL, *printername = NULL;
	BOOL got_handle = False;
	uint32 num_forms, level = 1, i;
	FORM_1 *forms;
	
	/* Parse the command arguements */

	if (argc != 2) {
		printf ("Usage: %s <printer>\n", argv[0]);
		return WERR_OK;
        }
	
	/* Get a printer handle */

	asprintf(&servername, "\\\\%s", cli->cli->desthost);
	strupper_m(servername);
	asprintf(&printername, "%s\\%s", servername, argv[1]);

	werror = rpccli_spoolss_open_printer_ex(
		cli, mem_ctx, printername, "", MAXIMUM_ALLOWED_ACCESS, 
		servername, cli->user_name, &handle);

	if (!W_ERROR_IS_OK(werror))
		goto done;

	got_handle = True;

	/* Enumerate forms */

	werror = rpccli_spoolss_enumforms(cli, mem_ctx, &handle, level, &num_forms, &forms);

	if (!W_ERROR_IS_OK(werror))
		goto done;

	/* Display output */

	for (i = 0; i < num_forms; i++) {

		display_form(&forms[i]);

	}

 done:
	if (got_handle)
		rpccli_spoolss_close_printer(cli, mem_ctx, &handle);

	SAFE_FREE(servername);
	SAFE_FREE(printername);

	return werror;
}

/****************************************************************************
****************************************************************************/

static WERROR cmd_spoolss_setprinterdata(struct rpc_pipe_client *cli,
					    TALLOC_CTX *mem_ctx,
					    int argc, const char **argv)
{
	WERROR result;
	fstring servername, printername, user;
	POLICY_HND pol;
	BOOL opened_hnd = False;
	PRINTER_INFO_CTR ctr;
	PRINTER_INFO_0 info;
	REGISTRY_VALUE value;

	/* parse the command arguements */
	if (argc < 5) {
		printf ("Usage: %s <printer> <string|binary|dword|multistring>"
			" <value> <data>\n",
			argv[0]);
		return WERR_INVALID_PARAM;
	}

	slprintf(servername, sizeof(servername)-1, "\\\\%s", cli->cli->desthost);
	strupper_m(servername);
	slprintf(printername, sizeof(servername)-1, "%s\\%s", servername, argv[1]);
	fstrcpy(user, cli->user_name);

	value.type = REG_NONE;

	if (strequal(argv[2], "string")) {
		value.type = REG_SZ;
	}

	if (strequal(argv[2], "binary")) {
		value.type = REG_BINARY;
	}

	if (strequal(argv[2], "dword")) {
		value.type = REG_DWORD;
	}

	if (strequal(argv[2], "multistring")) {
		value.type = REG_MULTI_SZ;
	}

	if (value.type == REG_NONE) {
		printf("Unknown data type: %s\n", argv[2]);
		return WERR_INVALID_PARAM;
	}

	/* get a printer handle */
	result = rpccli_spoolss_open_printer_ex(cli, mem_ctx, printername, "",
					     MAXIMUM_ALLOWED_ACCESS, servername, 
					     user, &pol);
	if (!W_ERROR_IS_OK(result))
		goto done;

	opened_hnd = True;

	ctr.printers_0 = &info;

        result = rpccli_spoolss_getprinter(cli, mem_ctx, &pol, 0, &ctr);

        if (!W_ERROR_IS_OK(result))
                goto done;
		
	printf("%s\n", timestring(True));
	printf("\tchange_id (before set)\t:[0x%x]\n", info.change_id);

	/* Set the printer data */
	
	fstrcpy(value.valuename, argv[3]);

	switch (value.type) {
	case REG_SZ: {
		UNISTR2 data;
		init_unistr2(&data, argv[4], UNI_STR_TERMINATE);
		value.size = data.uni_str_len * 2;
		value.data_p = TALLOC_MEMDUP(mem_ctx, data.buffer, value.size);
		break;
	}
	case REG_DWORD: {
		uint32 data = strtoul(argv[4], NULL, 10);
		value.size = sizeof(data);
		value.data_p = TALLOC_MEMDUP(mem_ctx, &data, sizeof(data));
		break;
	}
	case REG_BINARY: {
		DATA_BLOB data = strhex_to_data_blob(mem_ctx, argv[4]);
		value.data_p = data.data;
		value.size = data.length;
		break;
	}
	case REG_MULTI_SZ: {
		int i;
		size_t len = 0;
		char *p;

		for (i=4; i<argc; i++) {
			if (strcmp(argv[i], "NULL") == 0) {
				argv[i] = "";
			}
			len += strlen(argv[i])+1;
		}

		value.size = len*2;
		value.data_p = TALLOC_ARRAY(mem_ctx, unsigned char, value.size);
		if (value.data_p == NULL) {
			result = WERR_NOMEM;
			goto done;
		}

		p = (char *)value.data_p;
		len = value.size;
		for (i=4; i<argc; i++) {
			size_t l = (strlen(argv[i])+1)*2;
			rpcstr_push(p, argv[i], len, STR_TERMINATE);
			p += l;
			len -= l;
		}
		SMB_ASSERT(len == 0);
		break;
	}
	default:
		printf("Unknown data type: %s\n", argv[2]);
		result = WERR_INVALID_PARAM;
		goto done;
	}

	result = rpccli_spoolss_setprinterdata(cli, mem_ctx, &pol, &value);
		
	if (!W_ERROR_IS_OK(result)) {
		printf ("Unable to set [%s=%s]!\n", argv[3], argv[4]);
		goto done;
	}
	printf("\tSetPrinterData succeeded [%s: %s]\n", argv[3], argv[4]);
	
        result = rpccli_spoolss_getprinter(cli, mem_ctx, &pol, 0, &ctr);

        if (!W_ERROR_IS_OK(result))
                goto done;
		
	printf("%s\n", timestring(True));
	printf("\tchange_id (after set)\t:[0x%x]\n", info.change_id);

done:
	/* cleanup */
	if (opened_hnd)
		rpccli_spoolss_close_printer(cli, mem_ctx, &pol);

	return result;
}

/****************************************************************************
****************************************************************************/

static void display_job_info_1(JOB_INFO_1 *job)
{
	fstring username = "", document = "", text_status = "";

	rpcstr_pull(username, job->username.buffer,
		    sizeof(username), -1, STR_TERMINATE);

	rpcstr_pull(document, job->document.buffer,
		    sizeof(document), -1, STR_TERMINATE);

	rpcstr_pull(text_status, job->text_status.buffer,
		    sizeof(text_status), -1, STR_TERMINATE);

	printf("%d: jobid[%d]: %s %s %s %d/%d pages\n", job->position, job->jobid,
	       username, document, text_status, job->pagesprinted,
	       job->totalpages);
}

/****************************************************************************
****************************************************************************/

static void display_job_info_2(JOB_INFO_2 *job)
{
	fstring username = "", document = "", text_status = "";

	rpcstr_pull(username, job->username.buffer,
		    sizeof(username), -1, STR_TERMINATE);

	rpcstr_pull(document, job->document.buffer,
		    sizeof(document), -1, STR_TERMINATE);

	rpcstr_pull(text_status, job->text_status.buffer,
		    sizeof(text_status), -1, STR_TERMINATE);

	printf("%d: jobid[%d]: %s %s %s %d/%d pages, %d bytes\n", job->position, job->jobid,
	       username, document, text_status, job->pagesprinted,
	       job->totalpages, job->size);
}

/****************************************************************************
****************************************************************************/

static WERROR cmd_spoolss_enum_jobs(struct rpc_pipe_client *cli, 
				      TALLOC_CTX *mem_ctx, int argc, 
				      const char **argv)
{
	WERROR result;
	uint32 level = 1, num_jobs, i;
	BOOL got_hnd = False;
	pstring printername;
	fstring servername, user;
	POLICY_HND hnd;
	JOB_INFO_CTR ctr;
	
	if (argc < 2 || argc > 3) {
		printf("Usage: %s printername [level]\n", argv[0]);
		return WERR_OK;
	}
	
	if (argc == 3)
		level = atoi(argv[2]);

	/* Open printer handle */

	slprintf(servername, sizeof(servername)-1, "\\\\%s", cli->cli->desthost);
	strupper_m(servername);
	fstrcpy(user, cli->user_name);
	slprintf(printername, sizeof(servername)-1, "\\\\%s\\", cli->cli->desthost);
	strupper_m(printername);
	pstrcat(printername, argv[1]);

	result = rpccli_spoolss_open_printer_ex(cli, mem_ctx, printername, 
					     "", MAXIMUM_ALLOWED_ACCESS, 
					     servername, user, &hnd);

	if (!W_ERROR_IS_OK(result))
		goto done;
 
	got_hnd = True;

	/* Enumerate ports */

	result = rpccli_spoolss_enumjobs(cli, mem_ctx, &hnd, level, 0, 1000,
		&num_jobs, &ctr);

	if (!W_ERROR_IS_OK(result))
		goto done;

	for (i = 0; i < num_jobs; i++) {
		switch(level) {
		case 1:
			display_job_info_1(&ctr.job.job_info_1[i]);
			break;
		case 2:
			display_job_info_2(&ctr.job.job_info_2[i]);
			break;
		default:
			d_printf("unknown info level %d\n", level);
			break;
		}
	}
	
done:
	if (got_hnd)
		rpccli_spoolss_close_printer(cli, mem_ctx, &hnd);

	return result;
}

/****************************************************************************
****************************************************************************/

static WERROR cmd_spoolss_enum_data( struct rpc_pipe_client *cli, 
				       TALLOC_CTX *mem_ctx, int argc, 
				       const char **argv)
{
	WERROR result;
	uint32 i=0, val_needed, data_needed;
	BOOL got_hnd = False;
	pstring printername;
	fstring servername, user;
	POLICY_HND hnd;

	if (argc != 2) {
		printf("Usage: %s printername\n", argv[0]);
		return WERR_OK;
	}
	
	/* Open printer handle */

	slprintf(servername, sizeof(servername)-1, "\\\\%s", cli->cli->desthost);
	strupper_m(servername);
	fstrcpy(user, cli->user_name);
	slprintf(printername, sizeof(printername)-1, "\\\\%s\\", cli->cli->desthost);
	strupper_m(printername);
	pstrcat(printername, argv[1]);

	result = rpccli_spoolss_open_printer_ex(cli, mem_ctx, printername, 
					     "", MAXIMUM_ALLOWED_ACCESS, 
					     servername, user, &hnd);

	if (!W_ERROR_IS_OK(result))
		goto done;
 
	got_hnd = True;

	/* Enumerate data */

	result = rpccli_spoolss_enumprinterdata(cli, mem_ctx, &hnd, i, 0, 0,
					     &val_needed, &data_needed,
					     NULL);
	while (W_ERROR_IS_OK(result)) {
		REGISTRY_VALUE value;
		result = rpccli_spoolss_enumprinterdata(
			cli, mem_ctx, &hnd, i++, val_needed,
			data_needed, 0, 0, &value);
		if (W_ERROR_IS_OK(result))
			display_reg_value(value);
	}
	if (W_ERROR_V(result) == ERRnomoreitems)
		result = W_ERROR(ERRsuccess);

done:
	if (got_hnd)
		rpccli_spoolss_close_printer(cli, mem_ctx, &hnd);

	return result;
}

/****************************************************************************
****************************************************************************/

static WERROR cmd_spoolss_enum_data_ex( struct rpc_pipe_client *cli, 
					  TALLOC_CTX *mem_ctx, int argc, 
					  const char **argv)
{
	WERROR result;
	uint32 i;
	BOOL got_hnd = False;
	pstring printername;
	fstring servername, user;
	const char *keyname = NULL;
	POLICY_HND hnd;
	REGVAL_CTR *ctr = NULL;

	if (argc != 3) {
		printf("Usage: %s printername <keyname>\n", argv[0]);
		return WERR_OK;
	}
	
	keyname = argv[2];

	/* Open printer handle */

	slprintf(servername, sizeof(servername)-1, "\\\\%s", cli->cli->desthost);
	strupper_m(servername);
	fstrcpy(user, cli->user_name);
	slprintf(printername, sizeof(printername)-1, "\\\\%s\\", cli->cli->desthost);
	strupper_m(printername);
	pstrcat(printername, argv[1]);

	result = rpccli_spoolss_open_printer_ex(cli, mem_ctx, printername, 
					     "", MAXIMUM_ALLOWED_ACCESS, 
					     servername, user, &hnd);

	if (!W_ERROR_IS_OK(result))
		goto done;
 
	got_hnd = True;

	/* Enumerate subkeys */

	if ( !(ctr = TALLOC_ZERO_P( mem_ctx, REGVAL_CTR )) ) 
		return WERR_NOMEM;

	result = rpccli_spoolss_enumprinterdataex(cli, mem_ctx, &hnd, keyname, ctr);

	if (!W_ERROR_IS_OK(result))
		goto done;

	for (i=0; i < ctr->num_values; i++) {
		display_reg_value(*(ctr->values[i]));
	}

	TALLOC_FREE( ctr );

done:
	if (got_hnd)
		rpccli_spoolss_close_printer(cli, mem_ctx, &hnd);

	return result;
}

/****************************************************************************
****************************************************************************/

static WERROR cmd_spoolss_enum_printerkey( struct rpc_pipe_client *cli, 
					     TALLOC_CTX *mem_ctx, int argc, 
					     const char **argv)
{
	WERROR result;
	BOOL got_hnd = False;
	pstring printername;
	fstring servername, user;
	const char *keyname = NULL;
	POLICY_HND hnd;
	uint16 *keylist = NULL, *curkey;

	if (argc < 2 || argc > 3) {
		printf("Usage: %s printername [keyname]\n", argv[0]);
		return WERR_OK;
	}
		
	if (argc == 3)
		keyname = argv[2];
	else
		keyname = "";

	/* Open printer handle */

	slprintf(servername, sizeof(servername)-1, "\\\\%s", cli->cli->desthost);
	strupper_m(servername);
	fstrcpy(user, cli->user_name);
	slprintf(printername, sizeof(printername)-1, "\\\\%s\\", cli->cli->desthost);
	strupper_m(printername);
	pstrcat(printername, argv[1]);

	result = rpccli_spoolss_open_printer_ex(cli, mem_ctx, printername, 
					     "", MAXIMUM_ALLOWED_ACCESS, 
					     servername, user, &hnd);

	if (!W_ERROR_IS_OK(result))
		goto done;
	 
	got_hnd = True;

	/* Enumerate subkeys */

	result = rpccli_spoolss_enumprinterkey(cli, mem_ctx, &hnd, keyname, &keylist, NULL);

	if (!W_ERROR_IS_OK(result))
		goto done;

	curkey = keylist;
	while (*curkey != 0) {
		pstring subkey;
		rpcstr_pull(subkey, curkey, sizeof(subkey), -1, 
			    STR_TERMINATE);
		printf("%s\n", subkey);
		curkey += strlen(subkey) + 1;
	}

done:

	SAFE_FREE(keylist);

	if (got_hnd)
		rpccli_spoolss_close_printer(cli, mem_ctx, &hnd);

	return result;
}

/****************************************************************************
****************************************************************************/

static WERROR cmd_spoolss_rffpcnex(struct rpc_pipe_client *cli, 
				     TALLOC_CTX *mem_ctx, int argc, 
				     const char **argv)
{
	fstring servername, printername;
	POLICY_HND hnd;
	BOOL got_hnd = False;
	WERROR result;
	SPOOL_NOTIFY_OPTION option;

	if (argc != 2) {
		printf("Usage: %s printername\n", argv[0]);
		result = WERR_OK;
		goto done;
	}

	/* Open printer */

	slprintf(servername, sizeof(servername) - 1, "\\\\%s", cli->cli->desthost);
	strupper_m(servername);

	slprintf(printername, sizeof(printername) - 1, "\\\\%s\\%s", cli->cli->desthost,
		 argv[1]);
	strupper_m(printername);

	result = rpccli_spoolss_open_printer_ex(
		cli, mem_ctx, printername, "", MAXIMUM_ALLOWED_ACCESS, 
		servername, cli->user_name, &hnd);

	if (!W_ERROR_IS_OK(result)) {
		printf("Error opening %s\n", argv[1]);
		goto done;
	}

	got_hnd = True;

	/* Create spool options */

	ZERO_STRUCT(option);

	option.version = 2;
	option.option_type_ptr = 1;
	option.count = option.ctr.count = 2;

	option.ctr.type = TALLOC_ARRAY(mem_ctx, SPOOL_NOTIFY_OPTION_TYPE, 2);
	if (option.ctr.type == NULL) {
		result = WERR_NOMEM;
		goto done;
	}

	ZERO_STRUCT(option.ctr.type[0]);
	option.ctr.type[0].type = PRINTER_NOTIFY_TYPE;
	option.ctr.type[0].count = option.ctr.type[0].count2 = 1;
	option.ctr.type[0].fields_ptr = 1;
	option.ctr.type[0].fields[0] = PRINTER_NOTIFY_SERVER_NAME;

	ZERO_STRUCT(option.ctr.type[1]);
	option.ctr.type[1].type = JOB_NOTIFY_TYPE;
	option.ctr.type[1].count = option.ctr.type[1].count2 = 1;
	option.ctr.type[1].fields_ptr = 1;
	option.ctr.type[1].fields[0] = JOB_NOTIFY_PRINTER_NAME;

	/* Send rffpcnex */

	slprintf(servername, sizeof(servername) - 1, "\\\\%s", myhostname());
	strupper_m(servername);

	result = rpccli_spoolss_rffpcnex(
		cli, mem_ctx, &hnd, 0, 0, servername, 123, &option);

	if (!W_ERROR_IS_OK(result)) {
		printf("Error rffpcnex %s\n", argv[1]);
		goto done;
	}

done:		
	if (got_hnd)
		rpccli_spoolss_close_printer(cli, mem_ctx, &hnd);

	return result;
}

/****************************************************************************
****************************************************************************/

static BOOL compare_printer( struct rpc_pipe_client *cli1, POLICY_HND *hnd1,
                             struct rpc_pipe_client *cli2, POLICY_HND *hnd2 )
{
	PRINTER_INFO_CTR ctr1, ctr2;
	WERROR werror;
	TALLOC_CTX *mem_ctx = talloc_init("compare_printer");

	printf("Retrieving printer propertiesfor %s...", cli1->cli->desthost);
	werror = rpccli_spoolss_getprinter( cli1, mem_ctx, hnd1, 2, &ctr1);
	if ( !W_ERROR_IS_OK(werror) ) {
		printf("failed (%s)\n", dos_errstr(werror));
		talloc_destroy(mem_ctx);
		return False;
	}
	printf("ok\n");

	printf("Retrieving printer properties for %s...", cli2->cli->desthost);
	werror = rpccli_spoolss_getprinter( cli2, mem_ctx, hnd2, 2, &ctr2);
	if ( !W_ERROR_IS_OK(werror) ) {
		printf("failed (%s)\n", dos_errstr(werror));
		talloc_destroy(mem_ctx);
		return False;
	}
	printf("ok\n");

	talloc_destroy(mem_ctx);

	return True;
}

/****************************************************************************
****************************************************************************/

static BOOL compare_printer_secdesc( struct rpc_pipe_client *cli1, POLICY_HND *hnd1,
                                     struct rpc_pipe_client *cli2, POLICY_HND *hnd2 )
{
	PRINTER_INFO_CTR ctr1, ctr2;
	WERROR werror;
	TALLOC_CTX *mem_ctx = talloc_init("compare_printer_secdesc");
	SEC_DESC *sd1, *sd2;
	BOOL result = True;


	printf("Retreiving printer security for %s...", cli1->cli->desthost);
	werror = rpccli_spoolss_getprinter( cli1, mem_ctx, hnd1, 3, &ctr1);
	if ( !W_ERROR_IS_OK(werror) ) {
		printf("failed (%s)\n", dos_errstr(werror));
		result = False;
		goto done;
	}
	printf("ok\n");

	printf("Retrieving printer security for %s...", cli2->cli->desthost);
	werror = rpccli_spoolss_getprinter( cli2, mem_ctx, hnd2, 3, &ctr2);
	if ( !W_ERROR_IS_OK(werror) ) {
		printf("failed (%s)\n", dos_errstr(werror));
		result = False;
		goto done;
	}
	printf("ok\n");
	

	printf("++ ");

	if ( (ctr1.printers_3 != ctr2.printers_3) && (!ctr1.printers_3 || !ctr2.printers_3) ) {
		printf("NULL PRINTER_INFO_3!\n");
		result = False;
		goto done;
	}
	
	sd1 = ctr1.printers_3->secdesc;
	sd2 = ctr2.printers_3->secdesc;
	
	if ( (sd1 != sd2) && ( !sd1 || !sd2 ) ) {
		printf("NULL secdesc!\n");
		result = False;
		goto done;
	}
	
	if ( (ctr1.printers_3->flags != ctr1.printers_3->flags ) || !sec_desc_equal( sd1, sd2 ) ) {
		printf("Security Descriptors *not* equal!\n");
		result = False;
		goto done;
	}
	
	printf("Security descriptors match\n");
	
done:
	talloc_destroy(mem_ctx);
	return result;
}


/****************************************************************************
****************************************************************************/

static WERROR cmd_spoolss_printercmp(struct rpc_pipe_client *cli, 
				     TALLOC_CTX *mem_ctx, int argc, 
				     const char **argv)
{
	fstring printername, servername1, servername2;
	pstring printername_path;
	struct cli_state *cli_server1 = cli->cli;
	struct cli_state *cli_server2 = NULL;
	struct rpc_pipe_client *cli2 = NULL;
	POLICY_HND hPrinter1, hPrinter2;
	NTSTATUS nt_status;
	WERROR werror;
	
	if ( argc != 3 )  {
		printf("Usage: %s <printer> <server>\n", argv[0]);
		return WERR_OK;
	}
	
	fstrcpy( printername, argv[1] );
	
	fstr_sprintf( servername1, cli->cli->desthost );
	fstrcpy( servername2, argv[2] );
	strupper_m( servername1 );
	strupper_m( servername2 );
	
	
	/* first get the connection to the remote server */
	
	nt_status = cli_full_connection(&cli_server2, global_myname(), servername2, 
					NULL, 0,
					"IPC$", "IPC",  
					cmdline_auth_info.username, 
					lp_workgroup(),
					cmdline_auth_info.password, 
					cmdline_auth_info.use_kerberos ? CLI_FULL_CONNECTION_USE_KERBEROS : 0,
					cmdline_auth_info.signing_state, NULL);
					
	if ( !NT_STATUS_IS_OK(nt_status) )
		return WERR_GENERAL_FAILURE;

	cli2 = cli_rpc_pipe_open_noauth(cli_server2, PI_SPOOLSS, &nt_status);
	if (!cli2) {
		printf("failed to open spoolss pipe on server %s (%s)\n",
			servername2, nt_errstr(nt_status));
		return WERR_GENERAL_FAILURE;
	}
					
	/* now open up both printers */

	pstr_sprintf( printername_path, "\\\\%s\\%s", servername1, printername );
	printf("Opening %s...", printername_path);
	werror = rpccli_spoolss_open_printer_ex( cli, mem_ctx, printername_path, 
		"", PRINTER_ALL_ACCESS, servername1, cli_server1->user_name, &hPrinter1);
	if ( !W_ERROR_IS_OK(werror) ) {
		printf("failed (%s)\n", dos_errstr(werror));
		goto done;
	}
	printf("ok\n");
	
	pstr_sprintf( printername_path, "\\\\%s\\%s", servername2, printername );
	printf("Opening %s...", printername_path);
	werror = rpccli_spoolss_open_printer_ex( cli2, mem_ctx, printername_path,  
		"", PRINTER_ALL_ACCESS, servername2, cli_server2->user_name, &hPrinter2 );
	if ( !W_ERROR_IS_OK(werror) ) {
		 printf("failed (%s)\n", dos_errstr(werror));
		goto done;
	}
	printf("ok\n");
	
	
	compare_printer( cli, &hPrinter1, cli2, &hPrinter2 );
	compare_printer_secdesc( cli, &hPrinter1, cli2, &hPrinter2 );
#if 0
	compare_printerdata( cli_server1, &hPrinter1, cli_server2, &hPrinter2 );
#endif


done:
	/* cleanup */

	printf("Closing printers...");	
	rpccli_spoolss_close_printer( cli, mem_ctx, &hPrinter1 );
	rpccli_spoolss_close_printer( cli2, mem_ctx, &hPrinter2 );
	printf("ok\n");
	
	/* close the second remote connection */
	
	cli_shutdown( cli_server2 );
	
	return WERR_OK;
}

/* List of commands exported by this module */
struct cmd_set spoolss_commands[] = {

	{ "SPOOLSS"  },

	{ "adddriver",		RPC_RTYPE_WERROR, NULL, cmd_spoolss_addprinterdriver,	PI_SPOOLSS, NULL, "Add a print driver",                  "" },
	{ "addprinter",		RPC_RTYPE_WERROR, NULL, cmd_spoolss_addprinterex,	PI_SPOOLSS, NULL, "Add a printer",                       "" },
	{ "deldriver",		RPC_RTYPE_WERROR, NULL, cmd_spoolss_deletedriver,	PI_SPOOLSS, NULL, "Delete a printer driver",             "" },
	{ "deldriverex",	RPC_RTYPE_WERROR, NULL, cmd_spoolss_deletedriverex,	PI_SPOOLSS, NULL, "Delete a printer driver with files",  "" },
	{ "enumdata",		RPC_RTYPE_WERROR, NULL, cmd_spoolss_enum_data,		PI_SPOOLSS, NULL, "Enumerate printer data",              "" },
	{ "enumdataex",		RPC_RTYPE_WERROR, NULL, cmd_spoolss_enum_data_ex,	PI_SPOOLSS, NULL, "Enumerate printer data for a key",    "" },
	{ "enumkey",		RPC_RTYPE_WERROR, NULL, cmd_spoolss_enum_printerkey,	PI_SPOOLSS, NULL, "Enumerate printer keys",              "" },
	{ "enumjobs",		RPC_RTYPE_WERROR, NULL, cmd_spoolss_enum_jobs,          PI_SPOOLSS, NULL, "Enumerate print jobs",                "" },
	{ "enumports", 		RPC_RTYPE_WERROR, NULL, cmd_spoolss_enum_ports, 	PI_SPOOLSS, NULL, "Enumerate printer ports",             "" },
	{ "enumdrivers", 	RPC_RTYPE_WERROR, NULL, cmd_spoolss_enum_drivers, 	PI_SPOOLSS, NULL, "Enumerate installed printer drivers", "" },
	{ "enumprinters", 	RPC_RTYPE_WERROR, NULL, cmd_spoolss_enum_printers, 	PI_SPOOLSS, NULL, "Enumerate printers",                  "" },
	{ "getdata",		RPC_RTYPE_WERROR, NULL, cmd_spoolss_getprinterdata,	PI_SPOOLSS, NULL, "Get print driver data",               "" },
	{ "getdataex",		RPC_RTYPE_WERROR, NULL, cmd_spoolss_getprinterdataex,	PI_SPOOLSS, NULL, "Get printer driver data with keyname", ""},
	{ "getdriver",		RPC_RTYPE_WERROR, NULL, cmd_spoolss_getdriver,		PI_SPOOLSS, NULL, "Get print driver information",        "" },
	{ "getdriverdir",	RPC_RTYPE_WERROR, NULL, cmd_spoolss_getdriverdir,	PI_SPOOLSS, NULL, "Get print driver upload directory",   "" },
	{ "getprinter", 	RPC_RTYPE_WERROR, NULL, cmd_spoolss_getprinter, 	PI_SPOOLSS, NULL, "Get printer info",                    "" },
	{ "openprinter",	RPC_RTYPE_WERROR, NULL, cmd_spoolss_open_printer_ex,	PI_SPOOLSS, NULL, "Open printer handle",                 "" },
	{ "setdriver", 		RPC_RTYPE_WERROR, NULL, cmd_spoolss_setdriver,		PI_SPOOLSS, NULL, "Set printer driver",                  "" },
	{ "getprintprocdir",	RPC_RTYPE_WERROR, NULL, cmd_spoolss_getprintprocdir,    PI_SPOOLSS, NULL, "Get print processor directory",       "" },
	{ "addform",		RPC_RTYPE_WERROR, NULL, cmd_spoolss_addform,            PI_SPOOLSS, NULL, "Add form",                            "" },
	{ "setform",		RPC_RTYPE_WERROR, NULL, cmd_spoolss_setform,            PI_SPOOLSS, NULL, "Set form",                            "" },
	{ "getform",		RPC_RTYPE_WERROR, NULL, cmd_spoolss_getform,            PI_SPOOLSS, NULL, "Get form",                            "" },
	{ "deleteform",		RPC_RTYPE_WERROR, NULL, cmd_spoolss_deleteform,         PI_SPOOLSS, NULL, "Delete form",                         "" },
	{ "enumforms",		RPC_RTYPE_WERROR, NULL, cmd_spoolss_enum_forms,         PI_SPOOLSS, NULL, "Enumerate forms",                     "" },
	{ "setprinter",		RPC_RTYPE_WERROR, NULL, cmd_spoolss_setprinter,         PI_SPOOLSS, NULL, "Set printer comment",                 "" },
	{ "setprintername",	RPC_RTYPE_WERROR, NULL, cmd_spoolss_setprintername,	PI_SPOOLSS, NULL, "Set printername",                 "" },
	{ "setprinterdata",	RPC_RTYPE_WERROR, NULL, cmd_spoolss_setprinterdata,     PI_SPOOLSS, NULL, "Set REG_SZ printer data",             "" },
	{ "rffpcnex",		RPC_RTYPE_WERROR, NULL, cmd_spoolss_rffpcnex,           PI_SPOOLSS, NULL, "Rffpcnex test", "" },
	{ "printercmp",		RPC_RTYPE_WERROR, NULL, cmd_spoolss_printercmp,         PI_SPOOLSS, NULL, "Printer comparison test", "" },

	{ NULL }
};
