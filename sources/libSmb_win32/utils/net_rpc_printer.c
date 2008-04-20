/* 
   Samba Unix/Linux SMB client library 
   Distributed SMB/CIFS Server Management Utility 
   Copyright (C) 2004 Guenther Deschner (gd@samba.org)

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

struct table_node {
	const char *long_archi;
	const char *short_archi;
	int version;
};


/* support itanium as well */
static const struct table_node archi_table[]= {

	{"Windows 4.0",          "WIN40",	0 },
	{"Windows NT x86",       "W32X86",	2 },
	{"Windows NT x86",       "W32X86",	3 },
	{"Windows NT R4000",     "W32MIPS",	2 },
	{"Windows NT Alpha_AXP", "W32ALPHA",	2 },
	{"Windows NT PowerPC",   "W32PPC",	2 },
	{"Windows IA64",         "IA64",	3 },
	{"Windows x64",          "x64",		3 },
	{NULL,                   "",		-1 }
};


/**
 * This display-printdriver-functions was borrowed from rpcclient/cmd_spoolss.c.
 * It is here for debugging purpose and should be removed later on.
 **/

/****************************************************************************
 Printer info level 3 display function.
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

	d_printf ("Printer Driver Info 3:\n");
	d_printf ("\tVersion: [%x]\n", i1->version);
	d_printf ("\tDriver Name: [%s]\n",name);
	d_printf ("\tArchitecture: [%s]\n", architecture);
	d_printf ("\tDriver Path: [%s]\n", driverpath);
	d_printf ("\tDatafile: [%s]\n", datafile);
	d_printf ("\tConfigfile: [%s]\n", configfile);
	d_printf ("\tHelpfile: [%s]\n\n", helpfile);

	while (valid) {
		rpcstr_pull(dependentfiles, i1->dependentfiles+length, sizeof(dependentfiles), -1, STR_TERMINATE);
		
		length+=strlen(dependentfiles)+1;
		
		if (strlen(dependentfiles) > 0) {
			d_printf ("\tDependentfiles: [%s]\n", dependentfiles);
		} else {
			valid = False;
		}
	}
	
	printf ("\n");

	d_printf ("\tMonitorname: [%s]\n", monitorname);
	d_printf ("\tDefaultdatatype: [%s]\n\n", defaultdatatype);

	return;	
}

static void display_reg_value(const char *subkey, REGISTRY_VALUE value)
{
	pstring text;

	switch(value.type) {
	case REG_DWORD:
		d_printf("\t[%s:%s]: REG_DWORD: 0x%08x\n", subkey, value.valuename, 
		       *((uint32 *) value.data_p));
		break;

	case REG_SZ:
		rpcstr_pull(text, value.data_p, sizeof(text), value.size,
			    STR_TERMINATE);
		d_printf("\t[%s:%s]: REG_SZ: %s\n", subkey, value.valuename, text);
		break;

	case REG_BINARY: 
		d_printf("\t[%s:%s]: REG_BINARY: unknown length value not displayed\n", 
			 subkey, value.valuename);
		break;

	case REG_MULTI_SZ: {
		uint16 *curstr = (uint16 *) value.data_p;
		uint8 *start = value.data_p;
		d_printf("\t[%s:%s]: REG_MULTI_SZ:\n", subkey, value.valuename);
		while ((*curstr != 0) && 
		       ((uint8 *) curstr < start + value.size)) {
			rpcstr_pull(text, curstr, sizeof(text), -1, 
				    STR_TERMINATE);
			d_printf("%s\n", text);
			curstr += strlen(text) + 1;
		}
	}
	break;

	default:
		d_printf("\t%s: unknown type %d\n", value.valuename, value.type);
	}
	
}

/**
 * Copies ACLs, DOS-attributes and timestamps from one 
 * file or directory from one connected share to another connected share 
 *
 * @param mem_ctx		A talloc-context
 * @param cli_share_src		A connected cli_state 
 * @param cli_share_dst		A connected cli_state 
 * @param src_file		The source file-name
 * @param dst_file		The destination file-name
 * @param copy_acls		Whether to copy acls
 * @param copy_attrs		Whether to copy DOS attributes
 * @param copy_timestamps	Whether to preserve timestamps
 * @param is_file		Whether this file is a file or a dir
 *
 * @return Normal NTSTATUS return.
 **/ 

NTSTATUS net_copy_fileattr(TALLOC_CTX *mem_ctx,
		  struct cli_state *cli_share_src,
		  struct cli_state *cli_share_dst, 
		  const char *src_name, const char *dst_name,
		  BOOL copy_acls, BOOL copy_attrs,
		  BOOL copy_timestamps, BOOL is_file)
{
	NTSTATUS nt_status = NT_STATUS_UNSUCCESSFUL;
	int fnum_src = 0;
	int fnum_dst = 0;
	SEC_DESC *sd = NULL;
	uint16 attr;
	time_t f_atime, f_ctime, f_mtime;


	if (!copy_timestamps && !copy_acls && !copy_attrs)
		return NT_STATUS_OK;

	/* open file/dir on the originating server */

	DEBUGADD(3,("opening %s %s on originating server\n", 
		is_file?"file":"dir", src_name));

	fnum_src = cli_nt_create(cli_share_src, src_name, READ_CONTROL_ACCESS);
	if (fnum_src == -1) {
		DEBUGADD(0,("cannot open %s %s on originating server %s\n", 
			is_file?"file":"dir", src_name, cli_errstr(cli_share_src)));
		nt_status = cli_nt_error(cli_share_src);
		goto out;
	}


	if (copy_acls) {

		/* get the security descriptor */
		sd = cli_query_secdesc(cli_share_src, fnum_src, mem_ctx);
		if (!sd) {
			DEBUG(0,("failed to get security descriptor: %s\n",
				cli_errstr(cli_share_src)));
			nt_status = cli_nt_error(cli_share_src);
			goto out;
		}

		if (opt_verbose && DEBUGLEVEL >= 3)
			display_sec_desc(sd);
	}


	if (copy_attrs || copy_timestamps) {

		/* get file attributes */
		if (!cli_getattrE(cli_share_src, fnum_src, &attr, NULL, 
				 &f_ctime, &f_atime, &f_mtime)) {
			DEBUG(0,("failed to get file-attrs: %s\n", 
				cli_errstr(cli_share_src)));
			nt_status = cli_nt_error(cli_share_src);
			goto out;
		}
	}


	/* open the file/dir on the destination server */ 

	fnum_dst = cli_nt_create(cli_share_dst, dst_name, WRITE_DAC_ACCESS | WRITE_OWNER_ACCESS);
	if (fnum_dst == -1) {
		DEBUG(0,("failed to open %s on the destination server: %s: %s\n",
			is_file?"file":"dir", dst_name, cli_errstr(cli_share_dst)));
		nt_status = cli_nt_error(cli_share_dst);
		goto out;
	}

	if (copy_timestamps) {

		/* set timestamps */
		if (!cli_setattrE(cli_share_dst, fnum_dst, f_ctime, f_atime, f_mtime)) {
			DEBUG(0,("failed to set file-attrs (timestamps): %s\n",
				cli_errstr(cli_share_dst)));
			nt_status = cli_nt_error(cli_share_dst);
			goto out;
		}
	}

	if (copy_acls) {

		/* set acls */
		if (!cli_set_secdesc(cli_share_dst, fnum_dst, sd)) {
			DEBUG(0,("could not set secdesc on %s: %s\n",
				dst_name, cli_errstr(cli_share_dst)));
			nt_status = cli_nt_error(cli_share_dst);
			goto out;
		}
	}

	if (copy_attrs) {

		/* set attrs */
		if (!cli_setatr(cli_share_dst, dst_name, attr, 0)) {
			DEBUG(0,("failed to set file-attrs: %s\n",
				cli_errstr(cli_share_dst)));
			nt_status = cli_nt_error(cli_share_dst);
			goto out;
		}
	}


	/* closing files */

	if (!cli_close(cli_share_src, fnum_src)) {
		d_fprintf(stderr, "could not close %s on originating server: %s\n", 
			is_file?"file":"dir", cli_errstr(cli_share_src));
		nt_status = cli_nt_error(cli_share_src);
		goto out;
	}

	if (!cli_close(cli_share_dst, fnum_dst)) {
		d_fprintf(stderr, "could not close %s on destination server: %s\n", 
			is_file?"file":"dir", cli_errstr(cli_share_dst));
		nt_status = cli_nt_error(cli_share_dst);
		goto out;
	}


	nt_status = NT_STATUS_OK;

out:

	/* cleaning up */
	if (fnum_src)
		cli_close(cli_share_src, fnum_src);

	if (fnum_dst)
		cli_close(cli_share_dst, fnum_dst);

	return nt_status;
}

/**
 * Copy a file or directory from a connected share to another connected share 
 *
 * @param mem_ctx		A talloc-context
 * @param cli_share_src		A connected cli_state 
 * @param cli_share_dst		A connected cli_state 
 * @param src_file		The source file-name
 * @param dst_file		The destination file-name
 * @param copy_acls		Whether to copy acls
 * @param copy_attrs		Whether to copy DOS attributes
 * @param copy_timestamps	Whether to preserve timestamps
 * @param is_file		Whether this file is a file or a dir
 *
 * @return Normal NTSTATUS return.
 **/ 

NTSTATUS net_copy_file(TALLOC_CTX *mem_ctx,
		       struct cli_state *cli_share_src,
		       struct cli_state *cli_share_dst, 
		       const char *src_name, const char *dst_name,
		       BOOL copy_acls, BOOL copy_attrs,
		       BOOL copy_timestamps, BOOL is_file)
{
	NTSTATUS nt_status = NT_STATUS_UNSUCCESSFUL;
	int fnum_src = 0;
	int fnum_dst = 0;
	static int io_bufsize = 64512;
	int read_size = io_bufsize;
	char *data = NULL;
	off_t start = 0;
	off_t nread = 0;


	if (!src_name || !dst_name)
		goto out;

	if (cli_share_src == NULL || cli_share_dst == NULL)
		goto out; 
		

	/* open on the originating server */
	DEBUGADD(3,("opening %s %s on originating server\n", 
		is_file ? "file":"dir", src_name));
	if (is_file)
		fnum_src = cli_open(cli_share_src, src_name, O_RDONLY, DENY_NONE);
	else
		fnum_src = cli_nt_create(cli_share_src, src_name, READ_CONTROL_ACCESS);

	if (fnum_src == -1) {
		DEBUGADD(0,("cannot open %s %s on originating server %s\n",
			is_file ? "file":"dir",
			src_name, cli_errstr(cli_share_src)));
		nt_status = cli_nt_error(cli_share_src);
		goto out;
	}


	if (is_file) {

		/* open file on the destination server */
		DEBUGADD(3,("opening file %s on destination server\n", dst_name));
		fnum_dst = cli_open(cli_share_dst, dst_name, 
				O_RDWR|O_CREAT|O_TRUNC, DENY_NONE);

		if (fnum_dst == -1) {
			DEBUGADD(1,("cannot create file %s on destination server: %s\n", 
				dst_name, cli_errstr(cli_share_dst)));
			nt_status = cli_nt_error(cli_share_dst);
			goto out;
		}

		/* allocate memory */
		if (!(data = (char *)SMB_MALLOC(read_size))) {
			d_fprintf(stderr, "malloc fail for size %d\n", read_size);
			nt_status = NT_STATUS_NO_MEMORY;
			goto out;
		}

	}


	if (opt_verbose) {

		d_printf("copying [\\\\%s\\%s%s] => [\\\\%s\\%s%s] "
			 "%s ACLs and %s DOS Attributes %s\n", 
			cli_share_src->desthost, cli_share_src->share, src_name,
			cli_share_dst->desthost, cli_share_dst->share, dst_name,
			copy_acls ?  "with" : "without", 
			copy_attrs ? "with" : "without",
			copy_timestamps ? "(preserving timestamps)" : "" );
	}


	while (is_file) {

		/* copying file */
		int n, ret;
		n = cli_read(cli_share_src, fnum_src, data, nread + start, 
				read_size);

		if (n <= 0)
			break;

		ret = cli_write(cli_share_dst, fnum_dst, 0, data, 
			nread + start, n);

		if (n != ret) {
			d_fprintf(stderr, "Error writing file: %s\n", 
				cli_errstr(cli_share_dst));
			nt_status = cli_nt_error(cli_share_dst);
			goto out;
		}

		nread += n;
	}


	if (!is_file && !cli_chkpath(cli_share_dst, dst_name)) {

		/* creating dir */
		DEBUGADD(3,("creating dir %s on the destination server\n", 
			dst_name));

		if (!cli_mkdir(cli_share_dst, dst_name)) {
			DEBUG(0,("cannot create directory %s: %s\n",
				dst_name, cli_errstr(cli_share_dst)));
			nt_status = NT_STATUS_NO_SUCH_FILE;
		}

		if (!cli_chkpath(cli_share_dst, dst_name)) {
			d_fprintf(stderr, "cannot check for directory %s: %s\n",
				dst_name, cli_errstr(cli_share_dst));
			goto out;
		}
	}


	/* closing files */
	if (!cli_close(cli_share_src, fnum_src)) {
		d_fprintf(stderr, "could not close file on originating server: %s\n", 
			cli_errstr(cli_share_src));
		nt_status = cli_nt_error(cli_share_src);
		goto out;
	}

	if (is_file && !cli_close(cli_share_dst, fnum_dst)) {
		d_fprintf(stderr, "could not close file on destination server: %s\n", 
			cli_errstr(cli_share_dst));
		nt_status = cli_nt_error(cli_share_dst);
		goto out;
	}

	/* possibly we have to copy some file-attributes / acls / sd */
	nt_status = net_copy_fileattr(mem_ctx, cli_share_src, cli_share_dst, 
				      src_name, dst_name, copy_acls, 
				      copy_attrs, copy_timestamps, is_file);
	if (!NT_STATUS_IS_OK(nt_status))
		goto out;


	nt_status = NT_STATUS_OK;

out:

	/* cleaning up */
	if (fnum_src)
		cli_close(cli_share_src, fnum_src);

	if (fnum_dst)
		cli_close(cli_share_dst, fnum_dst);

	SAFE_FREE(data);

	return nt_status;
}

/**
 * Copy a driverfile from on connected share to another connected share 
 * This silently assumes that a driver-file is picked up from 
 *
 *	\\src_server\print$\{arch}\{version}\file 
 *
 * and copied to
 *
 * 	\\dst_server\print$\{arch}\file 
 * 
 * to be added via setdriver-calls later.
 * @param mem_ctx		A talloc-context
 * @param cli_share_src		A cli_state connected to source print$-share
 * @param cli_share_dst		A cli_state connected to destination print$-share
 * @param file			The file-name to be copied 
 * @param short_archi		The name of the driver-architecture (short form)
 *
 * @return Normal NTSTATUS return.
 **/ 

static NTSTATUS net_copy_driverfile(TALLOC_CTX *mem_ctx,
				    struct cli_state *cli_share_src,
				    struct cli_state *cli_share_dst, 
				    char *file, const char *short_archi) {

	NTSTATUS nt_status = NT_STATUS_UNSUCCESSFUL;
	const char *p;
	char *src_name;
	char *dst_name;
	fstring version;
	fstring filename;
	fstring tok;

	/* scroll through the file until we have the part 
	   beyond archi_table.short_archi */
	p = file;
	while (next_token(&p, tok, "\\", sizeof(tok))) {
		if (strequal(tok, short_archi)) {
			next_token(&p, version, "\\", sizeof(version));
			next_token(&p, filename, "\\", sizeof(filename));
		}
	}

	/* build source file name */
	if (asprintf(&src_name, "\\%s\\%s\\%s", short_archi, version, filename) < 0 ) 
		return NT_STATUS_NO_MEMORY;


	/* create destination file name */
	if (asprintf(&dst_name, "\\%s\\%s", short_archi, filename) < 0 )
                return NT_STATUS_NO_MEMORY;


	/* finally copy the file */
	nt_status = net_copy_file(mem_ctx, cli_share_src, cli_share_dst, 
				  src_name, dst_name, False, False, False, True);
	if (!NT_STATUS_IS_OK(nt_status))
		goto out;

	nt_status = NT_STATUS_OK;

out:
	SAFE_FREE(src_name);
	SAFE_FREE(dst_name);

	return nt_status;
}

/**
 * Check for existing Architecture directory on a given server
 *
 * @param cli_share		A cli_state connected to a print$-share
 * @param short_archi		The Architecture for the print-driver
 *
 * @return Normal NTSTATUS return.
 **/

static NTSTATUS check_arch_dir(struct cli_state *cli_share, const char *short_archi)
{

	NTSTATUS nt_status = NT_STATUS_UNSUCCESSFUL;
	char *dir;

	if (asprintf(&dir, "\\%s", short_archi) < 0) {
		return NT_STATUS_NO_MEMORY;
	}

	DEBUG(10,("creating print-driver dir for architecture: %s\n", 
		short_archi));

	if (!cli_mkdir(cli_share, dir)) {
                DEBUG(1,("cannot create directory %s: %s\n",
                         dir, cli_errstr(cli_share)));
                nt_status = NT_STATUS_NO_SUCH_FILE;
        }

	if (!cli_chkpath(cli_share, dir)) {
		d_fprintf(stderr, "cannot check %s: %s\n", 
			dir, cli_errstr(cli_share));
		goto out;
	}

	nt_status = NT_STATUS_OK;

out:
	SAFE_FREE(dir);
	return nt_status;
}

/**
 * Copy a print-driver (level 3) from one connected print$-share to another 
 * connected print$-share
 *
 * @param mem_ctx		A talloc-context
 * @param cli_share_src		A cli_state connected to a print$-share
 * @param cli_share_dst		A cli_state connected to a print$-share
 * @param short_archi		The Architecture for the print-driver
 * @param i1			The DRIVER_INFO_3-struct
 *
 * @return Normal NTSTATUS return.
 **/

static NTSTATUS copy_print_driver_3(TALLOC_CTX *mem_ctx,
		    struct cli_state *cli_share_src, 
		    struct cli_state *cli_share_dst, 
		    const char *short_archi, DRIVER_INFO_3 *i1)
{
	NTSTATUS nt_status = NT_STATUS_UNSUCCESSFUL;
	int length = 0;
	BOOL valid = True;
	
	fstring name = "";
	fstring driverpath = "";
	fstring datafile = "";
	fstring configfile = "";
	fstring helpfile = "";
	fstring dependentfiles = "";
	
	if (i1 == NULL)
		return nt_status;

	rpcstr_pull(name, i1->name.buffer, sizeof(name), -1, STR_TERMINATE);
	rpcstr_pull(driverpath, i1->driverpath.buffer, sizeof(driverpath), -1, STR_TERMINATE);
	rpcstr_pull(datafile, i1->datafile.buffer, sizeof(datafile), -1, STR_TERMINATE);
	rpcstr_pull(configfile, i1->configfile.buffer, sizeof(configfile), -1, STR_TERMINATE);
	rpcstr_pull(helpfile, i1->helpfile.buffer, sizeof(helpfile), -1, STR_TERMINATE);


	if (opt_verbose)
		d_printf("copying driver: [%s], for architecture: [%s], version: [%d]\n", 
			  name, short_archi, i1->version);
	
	nt_status = net_copy_driverfile(mem_ctx, cli_share_src, cli_share_dst, 
		driverpath, short_archi);
	if (!NT_STATUS_IS_OK(nt_status))
		return nt_status;
		
	nt_status = net_copy_driverfile(mem_ctx, cli_share_src, cli_share_dst, 
		datafile, short_archi);
	if (!NT_STATUS_IS_OK(nt_status))
		return nt_status;
		
	nt_status = net_copy_driverfile(mem_ctx, cli_share_src, cli_share_dst, 
		configfile, short_archi);
	if (!NT_STATUS_IS_OK(nt_status))
		return nt_status;
		
	nt_status = net_copy_driverfile(mem_ctx, cli_share_src, cli_share_dst, 
		helpfile, short_archi);
	if (!NT_STATUS_IS_OK(nt_status))
		return nt_status;

	while (valid) {
		
		rpcstr_pull(dependentfiles, i1->dependentfiles+length, sizeof(dependentfiles), -1, STR_TERMINATE);
		length += strlen(dependentfiles)+1;
		
		if (strlen(dependentfiles) > 0) {

			nt_status = net_copy_driverfile(mem_ctx, 
					cli_share_src, cli_share_dst, 
					dependentfiles, short_archi);
			if (!NT_STATUS_IS_OK(nt_status))
				return nt_status;
		} else {
			valid = False;
		}
	}

	return NT_STATUS_OK;
}

/**
 * net_spoolss-functions
 * =====================
 *
 * the net_spoolss-functions aim to simplify spoolss-client-functions
 * required during the migration-process wrt buffer-sizes, returned
 * error-codes, etc. 
 *
 * this greatly reduces the complexitiy of the migrate-functions.
 *
 **/

static BOOL net_spoolss_enum_printers(struct rpc_pipe_client *pipe_hnd,
					TALLOC_CTX *mem_ctx, 
					char *name,
					uint32 flags,
					uint32 level, 
					uint32 *num_printers,
					PRINTER_INFO_CTR *ctr)
{
	WERROR result;

	/* enum printers */
	result = rpccli_spoolss_enum_printers(pipe_hnd, mem_ctx, name, flags,
		level, num_printers, ctr);

	if (!W_ERROR_IS_OK(result)) {
		printf("cannot enum printers: %s\n", dos_errstr(result));
		return False;
	}

	return True;
}

static BOOL net_spoolss_open_printer_ex(struct rpc_pipe_client *pipe_hnd,
					TALLOC_CTX *mem_ctx,
					const char *printername,
					uint32 access_required, 
					const char *username,
					POLICY_HND *hnd)
{
	WERROR result;
	fstring servername, printername2;

	slprintf(servername, sizeof(servername)-1, "\\\\%s", pipe_hnd->cli->desthost);

	fstrcpy(printername2, servername);
	fstrcat(printername2, "\\");
	fstrcat(printername2, printername);

	DEBUG(10,("connecting to: %s as %s for %s and access: %x\n", 
		servername, username, printername2, access_required));

	/* open printer */
	result = rpccli_spoolss_open_printer_ex(pipe_hnd, mem_ctx, printername2,
			"", access_required,
			servername, username, hnd);

	/* be more verbose */
	if (W_ERROR_V(result) == W_ERROR_V(WERR_ACCESS_DENIED)) {
		d_fprintf(stderr, "no access to printer [%s] on [%s] for user [%s] granted\n", 
			printername2, servername, username);
		return False;
	}

	if (!W_ERROR_IS_OK(result)) {
		d_fprintf(stderr, "cannot open printer %s on server %s: %s\n", 
			printername2, servername, dos_errstr(result));
		return False;
	}

	DEBUG(2,("got printer handle for printer: %s, server: %s\n", 
		printername2, servername));

	return True;
}

static BOOL net_spoolss_getprinter(struct rpc_pipe_client *pipe_hnd,
				TALLOC_CTX *mem_ctx,
				POLICY_HND *hnd,
				uint32 level, 
				PRINTER_INFO_CTR *ctr)
{
	WERROR result;

	/* getprinter call */
	result = rpccli_spoolss_getprinter(pipe_hnd, mem_ctx, hnd, level, ctr);

	if (!W_ERROR_IS_OK(result)) {
		printf("cannot get printer-info: %s\n", dos_errstr(result));
		return False;
	}

	return True;
}

static BOOL net_spoolss_setprinter(struct rpc_pipe_client *pipe_hnd,
				TALLOC_CTX *mem_ctx,
				POLICY_HND *hnd,
				uint32 level, 
				PRINTER_INFO_CTR *ctr)
{
	WERROR result;

	/* setprinter call */
	result = rpccli_spoolss_setprinter(pipe_hnd, mem_ctx, hnd, level, ctr, 0);

	if (!W_ERROR_IS_OK(result)) {
		printf("cannot set printer-info: %s\n", dos_errstr(result));
		return False;
	}

	return True;
}


static BOOL net_spoolss_setprinterdata(struct rpc_pipe_client *pipe_hnd,
					TALLOC_CTX *mem_ctx,
					POLICY_HND *hnd,
					REGISTRY_VALUE *value)
{
	WERROR result;
	
	/* setprinterdata call */
	result = rpccli_spoolss_setprinterdata(pipe_hnd, mem_ctx, hnd, value);

	if (!W_ERROR_IS_OK(result)) {
		printf ("unable to set printerdata: %s\n", dos_errstr(result));
		return False;
	}

	return True;
}


static BOOL net_spoolss_enumprinterkey(struct rpc_pipe_client *pipe_hnd,
					TALLOC_CTX *mem_ctx,
					POLICY_HND *hnd,
					const char *keyname,
					uint16 **keylist)
{
	WERROR result;

	/* enumprinterkey call */
	result = rpccli_spoolss_enumprinterkey(pipe_hnd, mem_ctx, hnd, keyname, keylist, NULL);
		
	if (!W_ERROR_IS_OK(result)) {
		printf("enumprinterkey failed: %s\n", dos_errstr(result));
		return False;
	}
	
	return True;
}

static BOOL net_spoolss_enumprinterdataex(struct rpc_pipe_client *pipe_hnd,
					TALLOC_CTX *mem_ctx,
					uint32 offered, 
					POLICY_HND *hnd,
					const char *keyname, 
					REGVAL_CTR *ctr) 
{
	WERROR result;

	/* enumprinterdataex call */
	result = rpccli_spoolss_enumprinterdataex(pipe_hnd, mem_ctx, hnd, keyname, ctr);
			
	if (!W_ERROR_IS_OK(result)) {
		printf("enumprinterdataex failed: %s\n", dos_errstr(result));
		return False;
	}
	
	return True;
}


static BOOL net_spoolss_setprinterdataex(struct rpc_pipe_client *pipe_hnd,
					TALLOC_CTX *mem_ctx,
					POLICY_HND *hnd,
					char *keyname, 
					REGISTRY_VALUE *value)
{
	WERROR result;

	/* setprinterdataex call */
	result = rpccli_spoolss_setprinterdataex(pipe_hnd, mem_ctx, hnd, 
					      keyname, value);
	
	if (!W_ERROR_IS_OK(result)) {
		printf("could not set printerdataex: %s\n", dos_errstr(result));
		return False;
	}
	
	return True;
}

static BOOL net_spoolss_enumforms(struct rpc_pipe_client *pipe_hnd,
				TALLOC_CTX *mem_ctx,
				POLICY_HND *hnd,
				int level,
				uint32 *num_forms,
				FORM_1 **forms)
										       
{
	WERROR result;

	/* enumforms call */
	result = rpccli_spoolss_enumforms(pipe_hnd, mem_ctx, hnd, level, num_forms, forms);

	if (!W_ERROR_IS_OK(result)) {
		printf("could not enum forms: %s\n", dos_errstr(result));
		return False;
	}
	
	return True;
}

static BOOL net_spoolss_enumprinterdrivers (struct rpc_pipe_client *pipe_hnd,
					TALLOC_CTX *mem_ctx,
					uint32 level, const char *env,
					uint32 *num_drivers,
					PRINTER_DRIVER_CTR *ctr)
{
	WERROR result;

	/* enumprinterdrivers call */
	result = rpccli_spoolss_enumprinterdrivers(
			pipe_hnd, mem_ctx, level,
			env, num_drivers, ctr);

	if (!W_ERROR_IS_OK(result)) {
		printf("cannot enum drivers: %s\n", dos_errstr(result));
		return False;
	}

	return True;
}

static BOOL net_spoolss_getprinterdriver(struct rpc_pipe_client *pipe_hnd,
			     TALLOC_CTX *mem_ctx, 
			     POLICY_HND *hnd, uint32 level, 
			     const char *env, int version, 
			     PRINTER_DRIVER_CTR *ctr)
{
	WERROR result;
	
	/* getprinterdriver call */
	result = rpccli_spoolss_getprinterdriver(
			pipe_hnd, mem_ctx, hnd, level,
			env, version, ctr);

	if (!W_ERROR_IS_OK(result)) {
		DEBUG(1,("cannot get driver (for architecture: %s): %s\n", 
			env, dos_errstr(result)));
		if (W_ERROR_V(result) != W_ERROR_V(WERR_UNKNOWN_PRINTER_DRIVER) &&
		    W_ERROR_V(result) != W_ERROR_V(WERR_INVALID_ENVIRONMENT)) {
			printf("cannot get driver: %s\n", dos_errstr(result));
		}
		return False;
	}

	return True;
}


static BOOL net_spoolss_addprinterdriver(struct rpc_pipe_client *pipe_hnd,
			     TALLOC_CTX *mem_ctx, uint32 level,
			     PRINTER_DRIVER_CTR *ctr)
{
	WERROR result;

	/* addprinterdriver call */
	result = rpccli_spoolss_addprinterdriver(pipe_hnd, mem_ctx, level, ctr);

	/* be more verbose */
	if (W_ERROR_V(result) == W_ERROR_V(WERR_ACCESS_DENIED)) {
		printf("You are not allowed to add drivers\n");
		return False;
	}
	if (!W_ERROR_IS_OK(result)) {
		printf("cannot add driver: %s\n", dos_errstr(result));
		return False;
	}

	return True;
}

/**
 * abstraction function to get uint32 num_printers and PRINTER_INFO_CTR ctr 
 * for a single printer or for all printers depending on argc/argv 
 **/

static BOOL get_printer_info(struct rpc_pipe_client *pipe_hnd,
			TALLOC_CTX *mem_ctx, 
			int level,
			int argc,
			const char **argv, 
			uint32 *num_printers,
			PRINTER_INFO_CTR *ctr)
{

	POLICY_HND hnd;

	/* no arguments given, enumerate all printers */
	if (argc == 0) {

		if (!net_spoolss_enum_printers(pipe_hnd, mem_ctx, NULL, 
				PRINTER_ENUM_LOCAL|PRINTER_ENUM_SHARED, 
				level, num_printers, ctr)) 
			return False;

		goto out;
	}


	/* argument given, get a single printer by name */
	if (!net_spoolss_open_printer_ex(pipe_hnd, mem_ctx, argv[0],
			MAXIMUM_ALLOWED_ACCESS,	pipe_hnd->cli->user_name, &hnd)) 
		return False;

	if (!net_spoolss_getprinter(pipe_hnd, mem_ctx, &hnd, level, ctr)) {
		rpccli_spoolss_close_printer(pipe_hnd, mem_ctx, &hnd);
		return False;
	}

	rpccli_spoolss_close_printer(pipe_hnd, mem_ctx, &hnd);

	*num_printers = 1;

out:
	DEBUG(3,("got %d printers\n", *num_printers));

	return True;

}

/** 
 * List print-queues (including local printers that are not shared)
 *
 * All parameters are provided by the run_rpc_command function, except for
 * argc, argv which are passed through. 
 *
 * @param domain_sid The domain sid aquired from the remote server
 * @param cli A cli_state connected to the server.
 * @param mem_ctx Talloc context, destoyed on compleation of the function.
 * @param argc  Standard main() style argc
 * @param argv  Standard main() style argv.  Initial components are already
 *              stripped
 *
 * @return Normal NTSTATUS return.
 **/

NTSTATUS rpc_printer_list_internals(const DOM_SID *domain_sid,
					const char *domain_name, 
					struct cli_state *cli,
					struct rpc_pipe_client *pipe_hnd,
					TALLOC_CTX *mem_ctx, 
					int argc,
					const char **argv)
{
	NTSTATUS nt_status = NT_STATUS_UNSUCCESSFUL;
	uint32 i, num_printers; 
	uint32 level = 2;
	pstring printername, sharename;
	PRINTER_INFO_CTR ctr;

	printf("listing printers\n");

	if (!get_printer_info(pipe_hnd, mem_ctx, level, argc, argv, &num_printers, &ctr))
		return nt_status;

	for (i = 0; i < num_printers; i++) {

		/* do some initialization */
		rpcstr_pull(printername, ctr.printers_2[i].printername.buffer, 
			sizeof(printername), -1, STR_TERMINATE);
		rpcstr_pull(sharename, ctr.printers_2[i].sharename.buffer, 
			sizeof(sharename), -1, STR_TERMINATE);
		
		d_printf("printer %d: %s, shared as: %s\n", 
			i+1, printername, sharename);
	}

	return NT_STATUS_OK;
}

/** 
 * List printer-drivers from a server 
 *
 * All parameters are provided by the run_rpc_command function, except for
 * argc, argv which are passed through. 
 *
 * @param domain_sid The domain sid aquired from the remote server
 * @param cli A cli_state connected to the server.
 * @param mem_ctx Talloc context, destoyed on compleation of the function.
 * @param argc  Standard main() style argc
 * @param argv  Standard main() style argv.  Initial components are already
 *              stripped
 *
 * @return Normal NTSTATUS return.
 **/

NTSTATUS rpc_printer_driver_list_internals(const DOM_SID *domain_sid,
						const char *domain_name, 
						struct cli_state *cli,
						struct rpc_pipe_client *pipe_hnd,
						TALLOC_CTX *mem_ctx, 
						int argc,
						const char **argv)
{
	NTSTATUS nt_status = NT_STATUS_UNSUCCESSFUL;
	uint32 i;
	uint32 level = 3; 
	PRINTER_DRIVER_CTR drv_ctr_enum;
	int d;
	
	ZERO_STRUCT(drv_ctr_enum);

	printf("listing printer-drivers\n");

        for (i=0; archi_table[i].long_archi!=NULL; i++) {

		uint32 num_drivers;

		/* enum remote drivers */
		if (!net_spoolss_enumprinterdrivers(pipe_hnd, mem_ctx, level,
				archi_table[i].long_archi, 
				&num_drivers, &drv_ctr_enum)) {
										
			nt_status = NT_STATUS_UNSUCCESSFUL;
			goto done;
		}

		if (num_drivers == 0) {
			d_printf ("no drivers found on server for architecture: [%s].\n", 
				archi_table[i].long_archi);
			continue;
		} 
		
		d_printf("got %d printer-drivers for architecture: [%s]\n", 
			num_drivers, archi_table[i].long_archi);


		/* do something for all drivers for architecture */
		for (d = 0; d < num_drivers; d++) {
			display_print_driver_3(&(drv_ctr_enum.info3[d]));
		}
	}
	
	nt_status = NT_STATUS_OK;

done:
	return nt_status;

}

/** 
 * Publish print-queues with args-wrapper
 *
 * @param cli A cli_state connected to the server.
 * @param mem_ctx Talloc context, destoyed on compleation of the function.
 * @param argc  Standard main() style argc
 * @param argv  Standard main() style argv.  Initial components are already
 *              stripped
 * @param action
 *
 * @return Normal NTSTATUS return.
 **/

static NTSTATUS rpc_printer_publish_internals_args(struct rpc_pipe_client *pipe_hnd,
					TALLOC_CTX *mem_ctx, 
					int argc,
					const char **argv,
					uint32 action)
{
	NTSTATUS nt_status = NT_STATUS_UNSUCCESSFUL;
	uint32 i, num_printers; 
	uint32 level = 7;
	pstring printername, sharename;
	PRINTER_INFO_CTR ctr, ctr_pub;
	POLICY_HND hnd;
	BOOL got_hnd = False;
	WERROR result;
	const char *action_str;

	if (!get_printer_info(pipe_hnd, mem_ctx, 2, argc, argv, &num_printers, &ctr))
		return nt_status;

	for (i = 0; i < num_printers; i++) {

		/* do some initialization */
		rpcstr_pull(printername, ctr.printers_2[i].printername.buffer, 
			sizeof(printername), -1, STR_TERMINATE);
		rpcstr_pull(sharename, ctr.printers_2[i].sharename.buffer, 
			sizeof(sharename), -1, STR_TERMINATE);

		/* open printer handle */
		if (!net_spoolss_open_printer_ex(pipe_hnd, mem_ctx, sharename,
			PRINTER_ALL_ACCESS, pipe_hnd->cli->user_name, &hnd)) 
			goto done;

		got_hnd = True;

		/* check for existing dst printer */
		if (!net_spoolss_getprinter(pipe_hnd, mem_ctx, &hnd, level, &ctr_pub)) 
			goto done;

		/* check action and set string */
		switch (action) {
		case SPOOL_DS_PUBLISH:
			action_str = "published";
			break;
		case SPOOL_DS_UPDATE:
			action_str = "updated";
			break;
		case SPOOL_DS_UNPUBLISH:
			action_str = "unpublished";
			break;
		default:
			action_str = "unknown action";
			printf("unkown action: %d\n", action);
			break;
		}

		ctr_pub.printers_7->action = action;

		result = rpccli_spoolss_setprinter(pipe_hnd, mem_ctx, &hnd, level, &ctr_pub, 0);
		if (!W_ERROR_IS_OK(result) && (W_ERROR_V(result) != W_ERROR_V(WERR_IO_PENDING))) {
			printf("cannot set printer-info: %s\n", dos_errstr(result));
			goto done;
		}

		printf("successfully %s printer %s in Active Directory\n", action_str, sharename);
	}

	nt_status = NT_STATUS_OK;

done:
	if (got_hnd) 
		rpccli_spoolss_close_printer(pipe_hnd, mem_ctx, &hnd);
	
	return nt_status;
}

NTSTATUS rpc_printer_publish_publish_internals(const DOM_SID *domain_sid,
						const char *domain_name, 
						struct cli_state *cli,
						struct rpc_pipe_client *pipe_hnd,
						TALLOC_CTX *mem_ctx, 
						int argc,
						const char **argv)
{
	return rpc_printer_publish_internals_args(pipe_hnd, mem_ctx, argc, argv, SPOOL_DS_PUBLISH);
}

NTSTATUS rpc_printer_publish_unpublish_internals(const DOM_SID *domain_sid,
						const char *domain_name, 
						struct cli_state *cli,
						struct rpc_pipe_client *pipe_hnd,
						TALLOC_CTX *mem_ctx, 
						int argc,
						const char **argv)
{
	return rpc_printer_publish_internals_args(pipe_hnd, mem_ctx, argc, argv, SPOOL_DS_UNPUBLISH);
}

NTSTATUS rpc_printer_publish_update_internals(const DOM_SID *domain_sid,
						const char *domain_name, 
						struct cli_state *cli,
						struct rpc_pipe_client *pipe_hnd,
						TALLOC_CTX *mem_ctx, 
						int argc,
						const char **argv)
{
	return rpc_printer_publish_internals_args(pipe_hnd, mem_ctx, argc, argv, SPOOL_DS_UPDATE);
}

/** 
 * List print-queues w.r.t. their publishing state
 *
 * All parameters are provided by the run_rpc_command function, except for
 * argc, argv which are passed through. 
 *
 * @param domain_sid The domain sid aquired from the remote server
 * @param cli A cli_state connected to the server.
 * @param mem_ctx Talloc context, destoyed on compleation of the function.
 * @param argc  Standard main() style argc
 * @param argv  Standard main() style argv.  Initial components are already
 *              stripped
 *
 * @return Normal NTSTATUS return.
 **/

NTSTATUS rpc_printer_publish_list_internals(const DOM_SID *domain_sid,
						const char *domain_name, 
						struct cli_state *cli,
						struct rpc_pipe_client *pipe_hnd,
						TALLOC_CTX *mem_ctx, 
						int argc,
						const char **argv)
{
	NTSTATUS nt_status = NT_STATUS_UNSUCCESSFUL;
	uint32 i, num_printers; 
	uint32 level = 7;
	pstring printername, sharename;
	pstring guid;
	PRINTER_INFO_CTR ctr, ctr_pub;
	POLICY_HND hnd;
	BOOL got_hnd = False;
	int state;

	if (!get_printer_info(pipe_hnd, mem_ctx, 2, argc, argv, &num_printers, &ctr))
		return nt_status;

	for (i = 0; i < num_printers; i++) {

		ZERO_STRUCT(ctr_pub);

		/* do some initialization */
		rpcstr_pull(printername, ctr.printers_2[i].printername.buffer, 
			sizeof(printername), -1, STR_TERMINATE);
		rpcstr_pull(sharename, ctr.printers_2[i].sharename.buffer, 
			sizeof(sharename), -1, STR_TERMINATE);

		/* open printer handle */
		if (!net_spoolss_open_printer_ex(pipe_hnd, mem_ctx, sharename,
			PRINTER_ALL_ACCESS, cli->user_name, &hnd)) 
			goto done;

		got_hnd = True;

		/* check for existing dst printer */
		if (!net_spoolss_getprinter(pipe_hnd, mem_ctx, &hnd, level, &ctr_pub)) 
			goto done;

		rpcstr_pull(guid, ctr_pub.printers_7->guid.buffer, sizeof(guid), -1, STR_TERMINATE);

		state = ctr_pub.printers_7->action;
		switch (state) {
			case SPOOL_DS_PUBLISH:
				printf("printer [%s] is published", sharename);
				if (opt_verbose)
					printf(", guid: %s", guid);
				printf("\n");
				break;
			case SPOOL_DS_UNPUBLISH:
				printf("printer [%s] is unpublished\n", sharename);
				break;
			case SPOOL_DS_UPDATE:
				printf("printer [%s] is currently updating\n", sharename);
				break;
			default:
				printf("unkown state: %d\n", state);
				break;
		}
	}

	nt_status = NT_STATUS_OK;

done:
	if (got_hnd) 
		rpccli_spoolss_close_printer(pipe_hnd, mem_ctx, &hnd);
	
	return nt_status;
}

/** 
 * Migrate Printer-ACLs from a source server to the destination server
 *
 * All parameters are provided by the run_rpc_command function, except for
 * argc, argv which are passed through. 
 *
 * @param domain_sid The domain sid aquired from the remote server
 * @param cli A cli_state connected to the server.
 * @param mem_ctx Talloc context, destoyed on compleation of the function.
 * @param argc  Standard main() style argc
 * @param argv  Standard main() style argv.  Initial components are already
 *              stripped
 *
 * @return Normal NTSTATUS return.
 **/

NTSTATUS rpc_printer_migrate_security_internals(const DOM_SID *domain_sid,
						const char *domain_name, 
						struct cli_state *cli,
						struct rpc_pipe_client *pipe_hnd,
						TALLOC_CTX *mem_ctx, 
						int argc,
						const char **argv)
{
	/* TODO: what now, info2 or info3 ? 
	   convince jerry that we should add clientside setacls level 3 at least
	*/
	NTSTATUS nt_status = NT_STATUS_UNSUCCESSFUL;
	uint32 i = 0;
	uint32 num_printers;
	uint32 level = 2;
	pstring printername = "", sharename = "";
	BOOL got_hnd_src = False;
	BOOL got_hnd_dst = False;
	struct rpc_pipe_client *pipe_hnd_dst = NULL;
	POLICY_HND hnd_src, hnd_dst;
	PRINTER_INFO_CTR ctr_src, ctr_dst, ctr_enum;
	struct cli_state *cli_dst = NULL;

	ZERO_STRUCT(ctr_src);

	DEBUG(3,("copying printer ACLs\n"));

	/* connect destination PI_SPOOLSS */
	nt_status = connect_dst_pipe(&cli_dst, &pipe_hnd_dst, PI_SPOOLSS);
	if (!NT_STATUS_IS_OK(nt_status))
		return nt_status;


	/* enum source printers */
	if (!get_printer_info(pipe_hnd, mem_ctx, level, argc, argv, &num_printers, &ctr_enum)) {
		nt_status = NT_STATUS_UNSUCCESSFUL;
		goto done;
	}

	if (!num_printers) {
		printf ("no printers found on server.\n");
		nt_status = NT_STATUS_OK;
		goto done;
	} 
	
	/* do something for all printers */
	for (i = 0; i < num_printers; i++) {

		/* do some initialization */
		rpcstr_pull(printername, ctr_enum.printers_2[i].printername.buffer, 
			sizeof(printername), -1, STR_TERMINATE);
		rpcstr_pull(sharename, ctr_enum.printers_2[i].sharename.buffer, 
			sizeof(sharename), -1, STR_TERMINATE);
		/* we can reset NT_STATUS here because we do not 
		   get any real NT_STATUS-codes anymore from now on */
		nt_status = NT_STATUS_UNSUCCESSFUL;
		
		d_printf("migrating printer ACLs for:     [%s] / [%s]\n", 
			printername, sharename);

		/* according to msdn you have specify these access-rights 
		   to see the security descriptor
			- READ_CONTROL (DACL)
			- ACCESS_SYSTEM_SECURITY (SACL)
		*/

		/* open src printer handle */
		if (!net_spoolss_open_printer_ex(pipe_hnd, mem_ctx, sharename,
			MAXIMUM_ALLOWED_ACCESS, cli->user_name, &hnd_src)) 
			goto done;

		got_hnd_src = True;

		/* open dst printer handle */
		if (!net_spoolss_open_printer_ex(pipe_hnd_dst, mem_ctx, sharename,
			PRINTER_ALL_ACCESS, cli_dst->user_name, &hnd_dst)) 
			goto done;

		got_hnd_dst = True;

		/* check for existing dst printer */
		if (!net_spoolss_getprinter(pipe_hnd_dst, mem_ctx, &hnd_dst, level, &ctr_dst)) 
			goto done;

		/* check for existing src printer */
		if (!net_spoolss_getprinter(pipe_hnd, mem_ctx, &hnd_src, 3, &ctr_src)) 
			goto done;

		/* Copy Security Descriptor */

		/* copy secdesc (info level 2) */
		ctr_dst.printers_2->devmode = NULL; 
		ctr_dst.printers_2->secdesc = dup_sec_desc(mem_ctx, ctr_src.printers_3->secdesc);

		if (opt_verbose)
			display_sec_desc(ctr_dst.printers_2->secdesc);
		
		if (!net_spoolss_setprinter(pipe_hnd_dst, mem_ctx, &hnd_dst, 2, &ctr_dst)) 
			goto done;
		
		DEBUGADD(1,("\tSetPrinter of SECDESC succeeded\n"));


		/* close printer handles here */
		if (got_hnd_src) {
			rpccli_spoolss_close_printer(pipe_hnd, mem_ctx, &hnd_src);
			got_hnd_src = False;
		}

		if (got_hnd_dst) {
			rpccli_spoolss_close_printer(pipe_hnd_dst, mem_ctx, &hnd_dst);
			got_hnd_dst = False;
		}

	}
	
	nt_status = NT_STATUS_OK;

done:

	if (got_hnd_src) {
		rpccli_spoolss_close_printer(pipe_hnd, mem_ctx, &hnd_src);
	}

	if (got_hnd_dst) {
		rpccli_spoolss_close_printer(pipe_hnd_dst, mem_ctx, &hnd_dst);
	}

	if (cli_dst) {
		cli_shutdown(cli_dst);
	}
	return nt_status;
}

/** 
 * Migrate printer-forms from a src server to the dst server
 *
 * All parameters are provided by the run_rpc_command function, except for
 * argc, argv which are passed through. 
 *
 * @param domain_sid The domain sid aquired from the remote server
 * @param cli A cli_state connected to the server.
 * @param mem_ctx Talloc context, destoyed on compleation of the function.
 * @param argc  Standard main() style argc
 * @param argv  Standard main() style argv.  Initial components are already
 *              stripped
 *
 * @return Normal NTSTATUS return.
 **/

NTSTATUS rpc_printer_migrate_forms_internals(const DOM_SID *domain_sid,
						const char *domain_name, 
						struct cli_state *cli,
						struct rpc_pipe_client *pipe_hnd,
						TALLOC_CTX *mem_ctx, 
						int argc,
						const char **argv)
{
	NTSTATUS nt_status = NT_STATUS_UNSUCCESSFUL;
	WERROR result;
	uint32 i, f;
	uint32 num_printers;
	uint32 level = 1;
	pstring printername = "", sharename = "";
	BOOL got_hnd_src = False;
	BOOL got_hnd_dst = False;
	struct rpc_pipe_client *pipe_hnd_dst = NULL;
	POLICY_HND hnd_src, hnd_dst;
	PRINTER_INFO_CTR ctr_enum, ctr_dst;
	uint32 num_forms;
	FORM_1 *forms;
	struct cli_state *cli_dst = NULL;
	
	ZERO_STRUCT(ctr_enum);

	DEBUG(3,("copying forms\n"));
	
	/* connect destination PI_SPOOLSS */
	nt_status = connect_dst_pipe(&cli_dst, &pipe_hnd_dst, PI_SPOOLSS);
	if (!NT_STATUS_IS_OK(nt_status))
		return nt_status;
	

	/* enum src printers */
	if (!get_printer_info(pipe_hnd, mem_ctx, 2, argc, argv, &num_printers, &ctr_enum)) {
		nt_status = NT_STATUS_UNSUCCESSFUL;
		goto done;
	}

	if (!num_printers) {
		printf ("no printers found on server.\n");
		nt_status = NT_STATUS_OK;
		goto done;
	} 
	

	/* do something for all printers */
	for (i = 0; i < num_printers; i++) {

		/* do some initialization */
		rpcstr_pull(printername, ctr_enum.printers_2[i].printername.buffer, 
			sizeof(printername), -1, STR_TERMINATE);
		rpcstr_pull(sharename, ctr_enum.printers_2[i].sharename.buffer, 
			sizeof(sharename), -1, STR_TERMINATE);
		/* we can reset NT_STATUS here because we do not 
		   get any real NT_STATUS-codes anymore from now on */
		nt_status = NT_STATUS_UNSUCCESSFUL;
		
		d_printf("migrating printer forms for:    [%s] / [%s]\n", 
			printername, sharename);


		/* open src printer handle */
		if (!net_spoolss_open_printer_ex(pipe_hnd, mem_ctx, sharename,
			MAXIMUM_ALLOWED_ACCESS, cli->user_name, &hnd_src)) 
			goto done;

		got_hnd_src = True;


		/* open dst printer handle */
		if (!net_spoolss_open_printer_ex(pipe_hnd_dst, mem_ctx, sharename,
			PRINTER_ALL_ACCESS, cli->user_name, &hnd_dst)) 
			goto done;

		got_hnd_dst = True;


		/* check for existing dst printer */
		if (!net_spoolss_getprinter(pipe_hnd_dst, mem_ctx, &hnd_dst, level, &ctr_dst)) 
			goto done;

		/* finally migrate forms */
		if (!net_spoolss_enumforms(pipe_hnd, mem_ctx, &hnd_src, level, &num_forms, &forms))
			goto done;

		DEBUG(1,("got %d forms for printer\n", num_forms));


		for (f = 0; f < num_forms; f++) {

			FORM form;
			fstring form_name;
			
			/* only migrate FORM_PRINTER types, according to jerry 
			   FORM_BUILTIN-types are hard-coded in samba */
			if (forms[f].flag != FORM_PRINTER)
				continue;

			if (forms[f].name.buffer)
				rpcstr_pull(form_name, forms[f].name.buffer,
					sizeof(form_name), -1, STR_TERMINATE);

			if (opt_verbose)
				d_printf("\tmigrating form # %d [%s] of type [%d]\n", 
					f, form_name, forms[f].flag);

			/* is there a more elegant way to do that ? */
			form.flags 	= FORM_PRINTER;
			form.size_x	= forms[f].width;
			form.size_y	= forms[f].length;
			form.left	= forms[f].left;
			form.top	= forms[f].top;
			form.right	= forms[f].right;
			form.bottom	= forms[f].bottom;
			
			init_unistr2(&form.name, form_name, UNI_STR_TERMINATE);

			/* FIXME: there might be something wrong with samba's 
			   builtin-forms */
			result = rpccli_spoolss_addform(pipe_hnd_dst, mem_ctx, 
				&hnd_dst, 1, &form);
			if (!W_ERROR_IS_OK(result)) {
				d_printf("\tAddForm form %d: [%s] refused.\n", 
					f, form_name);
				continue;
			}
	
			DEBUGADD(1,("\tAddForm of [%s] succeeded\n", form_name));
		}


		/* close printer handles here */
		if (got_hnd_src) {
			rpccli_spoolss_close_printer(pipe_hnd, mem_ctx, &hnd_src);
			got_hnd_src = False;
		}

		if (got_hnd_dst) {
			rpccli_spoolss_close_printer(pipe_hnd_dst, mem_ctx, &hnd_dst);
			got_hnd_dst = False;
		}
	}

	nt_status = NT_STATUS_OK;

done:

	if (got_hnd_src)
		rpccli_spoolss_close_printer(pipe_hnd, mem_ctx, &hnd_src);

	if (got_hnd_dst)
		rpccli_spoolss_close_printer(pipe_hnd_dst, mem_ctx, &hnd_dst);

	if (cli_dst) {
		cli_shutdown(cli_dst);
	}
	return nt_status;
}

/** 
 * Migrate printer-drivers from a src server to the dst server
 *
 * All parameters are provided by the run_rpc_command function, except for
 * argc, argv which are passed through. 
 *
 * @param domain_sid The domain sid aquired from the remote server
 * @param cli A cli_state connected to the server.
 * @param mem_ctx Talloc context, destoyed on compleation of the function.
 * @param argc  Standard main() style argc
 * @param argv  Standard main() style argv.  Initial components are already
 *              stripped
 *
 * @return Normal NTSTATUS return.
 **/

NTSTATUS rpc_printer_migrate_drivers_internals(const DOM_SID *domain_sid,
						const char *domain_name, 
						struct cli_state *cli,
						struct rpc_pipe_client *pipe_hnd,
						TALLOC_CTX *mem_ctx, 
						int argc,
						const char **argv)
{
	NTSTATUS nt_status = NT_STATUS_UNSUCCESSFUL;
	uint32 i, p;
	uint32 num_printers;
	uint32 level = 3; 
	pstring printername = "", sharename = "";
	BOOL got_hnd_src = False;
	BOOL got_hnd_dst = False;
	BOOL got_src_driver_share = False;
	BOOL got_dst_driver_share = False;
	struct rpc_pipe_client *pipe_hnd_dst = NULL;
	POLICY_HND hnd_src, hnd_dst;
	PRINTER_DRIVER_CTR drv_ctr_src, drv_ctr_dst;
	PRINTER_INFO_CTR info_ctr_enum, info_ctr_dst;
	struct cli_state *cli_dst = NULL;
	struct cli_state *cli_share_src = NULL;
	struct cli_state *cli_share_dst = NULL;
	fstring drivername = "";
	
	ZERO_STRUCT(drv_ctr_src);
	ZERO_STRUCT(drv_ctr_dst);
	ZERO_STRUCT(info_ctr_enum);
	ZERO_STRUCT(info_ctr_dst);


	DEBUG(3,("copying printer-drivers\n"));

	nt_status = connect_dst_pipe(&cli_dst, &pipe_hnd_dst, PI_SPOOLSS);
	if (!NT_STATUS_IS_OK(nt_status))
		return nt_status;
	

	/* open print$-share on the src server */
	nt_status = connect_to_service(&cli_share_src, &cli->dest_ip, 
			cli->desthost, "print$", "A:");
	if (!NT_STATUS_IS_OK(nt_status)) 
		goto done;

	got_src_driver_share = True;


	/* open print$-share on the dst server */
	nt_status = connect_to_service(&cli_share_dst, &cli_dst->dest_ip, 
			cli_dst->desthost, "print$", "A:");
	if (!NT_STATUS_IS_OK(nt_status)) 
		return nt_status;

	got_dst_driver_share = True;


	/* enum src printers */
	if (!get_printer_info(pipe_hnd, mem_ctx, 2, argc, argv, &num_printers, &info_ctr_enum)) {
		nt_status = NT_STATUS_UNSUCCESSFUL;
		goto done;
	}

	if (num_printers == 0) {
		printf ("no printers found on server.\n");
		nt_status = NT_STATUS_OK;
		goto done;
	} 
	

	/* do something for all printers */
	for (p = 0; p < num_printers; p++) {

		/* do some initialization */
		rpcstr_pull(printername, info_ctr_enum.printers_2[p].printername.buffer, 
			sizeof(printername), -1, STR_TERMINATE);
		rpcstr_pull(sharename, info_ctr_enum.printers_2[p].sharename.buffer, 
			sizeof(sharename), -1, STR_TERMINATE);
		/* we can reset NT_STATUS here because we do not 
		   get any real NT_STATUS-codes anymore from now on */
		nt_status = NT_STATUS_UNSUCCESSFUL;

		d_printf("migrating printer driver for:   [%s] / [%s]\n", 
			printername, sharename);

		/* open dst printer handle */
		if (!net_spoolss_open_printer_ex(pipe_hnd_dst, mem_ctx, sharename,
			PRINTER_ALL_ACCESS, cli->user_name, &hnd_dst)) 
			goto done;
			
		got_hnd_dst = True;

		/* check for existing dst printer */
		if (!net_spoolss_getprinter(pipe_hnd_dst, mem_ctx, &hnd_dst, 2, &info_ctr_dst)) 
			goto done;


		/* open src printer handle */
		if (!net_spoolss_open_printer_ex(pipe_hnd, mem_ctx, sharename,
			MAXIMUM_ALLOWED_ACCESS, pipe_hnd->cli->user_name, &hnd_src)) 
			goto done;

		got_hnd_src = True;


		/* in a first step call getdriver for each shared printer (per arch)
		   to get a list of all files that have to be copied */
		   
	        for (i=0; archi_table[i].long_archi!=NULL; i++) {

			/* getdriver src */
			if (!net_spoolss_getprinterdriver(pipe_hnd, mem_ctx, &hnd_src, 
					level, archi_table[i].long_archi, 
					archi_table[i].version, &drv_ctr_src)) 
				continue;

			rpcstr_pull(drivername, drv_ctr_src.info3->name.buffer, 
					sizeof(drivername), -1, STR_TERMINATE);

			if (opt_verbose)
				display_print_driver_3(drv_ctr_src.info3);


			/* check arch dir */
			nt_status = check_arch_dir(cli_share_dst, archi_table[i].short_archi);
			if (!NT_STATUS_IS_OK(nt_status))
				goto done;


			/* copy driver-files */
			nt_status = copy_print_driver_3(mem_ctx, cli_share_src, cli_share_dst, 
							archi_table[i].short_archi, 
							drv_ctr_src.info3);
			if (!NT_STATUS_IS_OK(nt_status))
				goto done;


			/* adddriver dst */
			if (!net_spoolss_addprinterdriver(pipe_hnd_dst, mem_ctx, level, &drv_ctr_src)) { 
				nt_status = NT_STATUS_UNSUCCESSFUL;
				goto done;
			}
				
			DEBUGADD(1,("Sucessfully added driver [%s] for printer [%s]\n", 
				drivername, printername));

		}

		if (strlen(drivername) == 0) {
			DEBUGADD(1,("Did not get driver for printer %s\n",
				    printername));
			goto done;
		}

		/* setdriver dst */
		init_unistr(&info_ctr_dst.printers_2->drivername, drivername);
		
		if (!net_spoolss_setprinter(pipe_hnd_dst, mem_ctx, &hnd_dst, 2, &info_ctr_dst)) { 
			nt_status = NT_STATUS_UNSUCCESSFUL;
			goto done;
		}

		DEBUGADD(1,("Sucessfully set driver %s for printer %s\n", 
			drivername, printername));

		/* close dst */
		if (got_hnd_dst) {
			rpccli_spoolss_close_printer(pipe_hnd_dst, mem_ctx, &hnd_dst);
			got_hnd_dst = False;
		}

		/* close src */
		if (got_hnd_src) {
			rpccli_spoolss_close_printer(pipe_hnd, mem_ctx, &hnd_src);
			got_hnd_src = False;
		}
	}

	nt_status = NT_STATUS_OK;

done:

	if (got_hnd_src)
		rpccli_spoolss_close_printer(pipe_hnd, mem_ctx, &hnd_src);

	if (got_hnd_dst)
		rpccli_spoolss_close_printer(pipe_hnd_dst, mem_ctx, &hnd_dst);

	if (cli_dst) {
		cli_shutdown(cli_dst);
	}

	if (got_src_driver_share)
		cli_shutdown(cli_share_src);

	if (got_dst_driver_share)
		cli_shutdown(cli_share_dst);

	return nt_status;

}

/** 
 * Migrate printer-queues from a src to the dst server
 * (requires a working "addprinter command" to be installed for the local smbd)
 *
 * All parameters are provided by the run_rpc_command function, except for
 * argc, argv which are passed through. 
 *
 * @param domain_sid The domain sid aquired from the remote server
 * @param cli A cli_state connected to the server.
 * @param mem_ctx Talloc context, destoyed on compleation of the function.
 * @param argc  Standard main() style argc
 * @param argv  Standard main() style argv.  Initial components are already
 *              stripped
 *
 * @return Normal NTSTATUS return.
 **/

NTSTATUS rpc_printer_migrate_printers_internals(const DOM_SID *domain_sid,
						const char *domain_name, 
						struct cli_state *cli,
						struct rpc_pipe_client *pipe_hnd,
						TALLOC_CTX *mem_ctx, 
						int argc,
						const char **argv)
{
	WERROR result;
	NTSTATUS nt_status = NT_STATUS_UNSUCCESSFUL;
	uint32 i = 0, num_printers;
	uint32 level = 2;
	PRINTER_INFO_CTR ctr_src, ctr_dst, ctr_enum;
	struct cli_state *cli_dst = NULL;
	POLICY_HND hnd_dst, hnd_src;
	pstring printername, sharename;
	BOOL got_hnd_src = False;
	BOOL got_hnd_dst = False;
	struct rpc_pipe_client *pipe_hnd_dst = NULL;

	DEBUG(3,("copying printers\n"));

	/* connect destination PI_SPOOLSS */
	nt_status = connect_dst_pipe(&cli_dst, &pipe_hnd_dst, PI_SPOOLSS);
	if (!NT_STATUS_IS_OK(nt_status))
		return nt_status;

	/* enum printers */
	if (!get_printer_info(pipe_hnd, mem_ctx, level, argc, argv, &num_printers, &ctr_enum)) {
		nt_status = NT_STATUS_UNSUCCESSFUL;
		goto done;
	}

	if (!num_printers) {
		printf ("no printers found on server.\n");
		nt_status = NT_STATUS_OK;
		goto done;
	} 
	

	/* do something for all printers */
	for (i = 0; i < num_printers; i++) {

		/* do some initialization */
		rpcstr_pull(printername, ctr_enum.printers_2[i].printername.buffer, 
			sizeof(printername), -1, STR_TERMINATE);
		rpcstr_pull(sharename, ctr_enum.printers_2[i].sharename.buffer, 
			sizeof(sharename), -1, STR_TERMINATE);
		/* we can reset NT_STATUS here because we do not 
		   get any real NT_STATUS-codes anymore from now on */
		nt_status = NT_STATUS_UNSUCCESSFUL;
		
		d_printf("migrating printer queue for:    [%s] / [%s]\n", 
			printername, sharename);

		/* open dst printer handle */
		if (!net_spoolss_open_printer_ex(pipe_hnd_dst, mem_ctx, sharename, 
			PRINTER_ALL_ACCESS, cli->user_name, &hnd_dst)) {
			
			DEBUG(1,("could not open printer: %s\n", sharename));
		} else {
			got_hnd_dst = True;
		}

		/* check for existing dst printer */
		if (!net_spoolss_getprinter(pipe_hnd_dst, mem_ctx, &hnd_dst, level, &ctr_dst)) {
			printf ("could not get printer, creating printer.\n");
		} else {
			DEBUG(1,("printer already exists: %s\n", sharename));
			/* close printer handle here - dst only, not got src yet. */
			if (got_hnd_dst) {
				rpccli_spoolss_close_printer(pipe_hnd_dst, mem_ctx, &hnd_dst);
				got_hnd_dst = False;
			}
			continue;
		}

		/* now get again src printer ctr via getprinter, 
		   we first need a handle for that */

		/* open src printer handle */
		if (!net_spoolss_open_printer_ex(pipe_hnd, mem_ctx, sharename,
			MAXIMUM_ALLOWED_ACCESS, cli->user_name, &hnd_src)) 
			goto done;

		got_hnd_src = True;

		/* getprinter on the src server */
		if (!net_spoolss_getprinter(pipe_hnd, mem_ctx, &hnd_src, level, &ctr_src)) 
			goto done;

		/* copy each src printer to a dst printer 1:1, 
		   maybe some values have to be changed though */
		d_printf("creating printer: %s\n", printername);
		result = rpccli_spoolss_addprinterex (pipe_hnd_dst, mem_ctx, level, &ctr_src);

		if (W_ERROR_IS_OK(result))
			d_printf ("printer [%s] successfully added.\n", printername);
		else if (W_ERROR_V(result) == W_ERROR_V(WERR_PRINTER_ALREADY_EXISTS)) 
			d_fprintf (stderr, "printer [%s] already exists.\n", printername);
		else {
			d_fprintf (stderr, "could not create printer [%s]\n", printername);
			goto done;
		}

		/* close printer handles here */
		if (got_hnd_src) {
			rpccli_spoolss_close_printer(pipe_hnd, mem_ctx, &hnd_src);
			got_hnd_src = False;
		}

		if (got_hnd_dst) {
			rpccli_spoolss_close_printer(pipe_hnd_dst, mem_ctx, &hnd_dst);
			got_hnd_dst = False;
		}
	}

	nt_status = NT_STATUS_OK;

done:
	if (got_hnd_src)
		rpccli_spoolss_close_printer(pipe_hnd, mem_ctx, &hnd_src);

	if (got_hnd_dst)
		rpccli_spoolss_close_printer(pipe_hnd_dst, mem_ctx, &hnd_dst);

	if (cli_dst) {
		cli_shutdown(cli_dst);
	}
	return nt_status;
}

/** 
 * Migrate Printer-Settings from a src server to the dst server
 * (for this to work, printers and drivers already have to be migrated earlier)
 *
 * All parameters are provided by the run_rpc_command function, except for
 * argc, argv which are passed through. 
 *
 * @param domain_sid The domain sid aquired from the remote server
 * @param cli A cli_state connected to the server.
 * @param mem_ctx Talloc context, destoyed on compleation of the function.
 * @param argc  Standard main() style argc
 * @param argv  Standard main() style argv.  Initial components are already
 *              stripped
 *
 * @return Normal NTSTATUS return.
 **/

NTSTATUS rpc_printer_migrate_settings_internals(const DOM_SID *domain_sid,
						const char *domain_name, 
						struct cli_state *cli,
						struct rpc_pipe_client *pipe_hnd,
						TALLOC_CTX *mem_ctx, 
						int argc,
						const char **argv)
{

	/* FIXME: Here the nightmare begins */

	WERROR result;
	NTSTATUS nt_status = NT_STATUS_UNSUCCESSFUL;
	uint32 i = 0, p = 0, j = 0;
	uint32 num_printers, val_needed, data_needed;
	uint32 level = 2;
	pstring printername = "", sharename = "";
	BOOL got_hnd_src = False;
	BOOL got_hnd_dst = False;
	struct rpc_pipe_client *pipe_hnd_dst = NULL;
	POLICY_HND hnd_src, hnd_dst;
	PRINTER_INFO_CTR ctr_enum, ctr_dst, ctr_dst_publish;
	REGVAL_CTR *reg_ctr;
	struct cli_state *cli_dst = NULL;
	char *devicename = NULL, *unc_name = NULL, *url = NULL;
	fstring longname;

	uint16 *keylist = NULL, *curkey;

	ZERO_STRUCT(ctr_enum);

	DEBUG(3,("copying printer settings\n"));

	/* connect destination PI_SPOOLSS */
	nt_status = connect_dst_pipe(&cli_dst, &pipe_hnd_dst, PI_SPOOLSS);
	if (!NT_STATUS_IS_OK(nt_status))
		return nt_status;


	/* enum src printers */
	if (!get_printer_info(pipe_hnd, mem_ctx, level, argc, argv, &num_printers, &ctr_enum)) {
		nt_status = NT_STATUS_UNSUCCESSFUL;
		goto done;
	}

	if (!num_printers) {
		printf ("no printers found on server.\n");
		nt_status = NT_STATUS_OK;
		goto done;
	} 
	

	/* needed for dns-strings in regkeys */
	get_mydnsfullname(longname);
	
	/* do something for all printers */
	for (i = 0; i < num_printers; i++) {

		/* do some initialization */
		rpcstr_pull(printername, ctr_enum.printers_2[i].printername.buffer, 
			sizeof(printername), -1, STR_TERMINATE);
		rpcstr_pull(sharename, ctr_enum.printers_2[i].sharename.buffer, 
			sizeof(sharename), -1, STR_TERMINATE);
		
		/* we can reset NT_STATUS here because we do not 
		   get any real NT_STATUS-codes anymore from now on */
		nt_status = NT_STATUS_UNSUCCESSFUL;
		
		d_printf("migrating printer settings for: [%s] / [%s]\n", 
			printername, sharename);


		/* open src printer handle */
		if (!net_spoolss_open_printer_ex(pipe_hnd, mem_ctx, sharename,
			MAXIMUM_ALLOWED_ACCESS, cli->user_name, &hnd_src)) 
			goto done;

		got_hnd_src = True;


		/* open dst printer handle */
		if (!net_spoolss_open_printer_ex(pipe_hnd_dst, mem_ctx, sharename,
			PRINTER_ALL_ACCESS, cli_dst->user_name, &hnd_dst)) 
			goto done;

		got_hnd_dst = True;


		/* check for existing dst printer */
		if (!net_spoolss_getprinter(pipe_hnd_dst, mem_ctx, &hnd_dst, 
				level, &ctr_dst)) 
			goto done;


		/* STEP 1: COPY DEVICE-MODE and other 
			   PRINTER_INFO_2-attributes
		*/

		ctr_dst.printers_2 = &ctr_enum.printers_2[i];

		/* why is the port always disconnected when the printer 
		   is correctly installed (incl. driver ???) */
		init_unistr( &ctr_dst.printers_2->portname, SAMBA_PRINTER_PORT_NAME);

		/* check if printer is published */ 
		if (ctr_enum.printers_2[i].attributes & PRINTER_ATTRIBUTE_PUBLISHED) {

			/* check for existing dst printer */
			if (!net_spoolss_getprinter(pipe_hnd_dst, mem_ctx, &hnd_dst, 7, &ctr_dst_publish))
				goto done;

			ctr_dst_publish.printers_7->action = SPOOL_DS_PUBLISH;

			/* ignore False from setprinter due to WERR_IO_PENDING */
	 		net_spoolss_setprinter(pipe_hnd_dst, mem_ctx, &hnd_dst, 7, &ctr_dst_publish);

			DEBUG(3,("republished printer\n"));
		}

		if (ctr_enum.printers_2[i].devmode != NULL) {

			/* copy devmode (info level 2) */
			ctr_dst.printers_2->devmode =
				TALLOC_MEMDUP(mem_ctx,
					      ctr_enum.printers_2[i].devmode,
					      sizeof(DEVICEMODE));

			/* do not copy security descriptor (we have another
			 * command for that) */
			ctr_dst.printers_2->secdesc = NULL;

#if 0
			if (asprintf(&devicename, "\\\\%s\\%s", longname,
				     printername) < 0) {
				nt_status = NT_STATUS_NO_MEMORY;
				goto done;
			}

			init_unistr(&ctr_dst.printers_2->devmode->devicename,
				    devicename); 
#endif
			if (!net_spoolss_setprinter(pipe_hnd_dst, mem_ctx, &hnd_dst,
						    level, &ctr_dst)) 
				goto done;
		
			DEBUGADD(1,("\tSetPrinter of DEVICEMODE succeeded\n"));
		}

		/* STEP 2: COPY REGISTRY VALUES */
	
		/* please keep in mind that samba parse_spools gives horribly 
		   crippled results when used to rpccli_spoolss_enumprinterdataex 
		   a win2k3-server.  (Bugzilla #1851)
		   FIXME: IIRC I've seen it too on a win2k-server 
		*/

		/* enumerate data on src handle */
		result = rpccli_spoolss_enumprinterdata(pipe_hnd, mem_ctx, &hnd_src, p, 0, 0,
			&val_needed, &data_needed, NULL);

		/* loop for all printerdata of "PrinterDriverData" */
		while (W_ERROR_IS_OK(result)) {
			
			REGISTRY_VALUE value;
			
			result = rpccli_spoolss_enumprinterdata(
				pipe_hnd, mem_ctx, &hnd_src, p++, val_needed,
				data_needed, 0, 0, &value);

			/* loop for all reg_keys */
			if (W_ERROR_IS_OK(result)) {

				/* display_value */
				if (opt_verbose) 
					display_reg_value(SPOOL_PRINTERDATA_KEY, value);

				/* set_value */
				if (!net_spoolss_setprinterdata(pipe_hnd_dst, mem_ctx, 
								&hnd_dst, &value)) 
					goto done;

				DEBUGADD(1,("\tSetPrinterData of [%s] succeeded\n", 
					value.valuename));
			}
		}
		
		/* STEP 3: COPY SUBKEY VALUES */

		/* here we need to enum all printer_keys and then work 
		   on the result with enum_printer_key_ex. nt4 does not
		   respond to enumprinterkey, win2k does, so continue 
		   in case of an error */

		if (!net_spoolss_enumprinterkey(pipe_hnd, mem_ctx, &hnd_src, "", &keylist)) {
			printf("got no key-data\n");
			continue;
		}


		/* work on a list of printer keys 
		   each key has to be enumerated to get all required
		   information.  information is then set via setprinterdataex-calls */ 

		if (keylist == NULL)
			continue;

		curkey = keylist;
		while (*curkey != 0) {

			pstring subkey;
			rpcstr_pull(subkey, curkey, sizeof(subkey), -1, STR_TERMINATE);

			curkey += strlen(subkey) + 1;

			if ( !(reg_ctr = TALLOC_ZERO_P( mem_ctx, REGVAL_CTR )) )
				return NT_STATUS_NO_MEMORY;

			/* enumerate all src subkeys */
			if (!net_spoolss_enumprinterdataex(pipe_hnd, mem_ctx, 0, 
							   &hnd_src, subkey, 
							   reg_ctr)) 
				goto done;

			for (j=0; j < reg_ctr->num_values; j++) {
			
				REGISTRY_VALUE value;
				UNISTR2 data;
			
				/* although samba replies with sane data in most cases we 
				   should try to avoid writing wrong registry data */
	
				if (strequal(reg_ctr->values[j]->valuename, SPOOL_REG_PORTNAME) || 
				    strequal(reg_ctr->values[j]->valuename, SPOOL_REG_UNCNAME) ||
				    strequal(reg_ctr->values[j]->valuename, SPOOL_REG_URL) ||
				    strequal(reg_ctr->values[j]->valuename, SPOOL_REG_SHORTSERVERNAME) ||
				    strequal(reg_ctr->values[j]->valuename, SPOOL_REG_SERVERNAME)) {

					if (strequal(reg_ctr->values[j]->valuename, SPOOL_REG_PORTNAME)) {
				
						/* although windows uses a multi-sz, we use a sz */
						init_unistr2(&data, SAMBA_PRINTER_PORT_NAME, UNI_STR_TERMINATE);
						fstrcpy(value.valuename, SPOOL_REG_PORTNAME);
					}
				
					if (strequal(reg_ctr->values[j]->valuename, SPOOL_REG_UNCNAME)) {
					
						if (asprintf(&unc_name, "\\\\%s\\%s", longname, sharename) < 0) {
							nt_status = NT_STATUS_NO_MEMORY;
							goto done;
						}
						init_unistr2(&data, unc_name, UNI_STR_TERMINATE);
						fstrcpy(value.valuename, SPOOL_REG_UNCNAME);
					}

					if (strequal(reg_ctr->values[j]->valuename, SPOOL_REG_URL)) {

						continue;

#if 0
						/* FIXME: should we really do that ??? */
						if (asprintf(&url, "http://%s:631/printers/%s", longname, sharename) < 0) {
							nt_status = NT_STATUS_NO_MEMORY;
							goto done;
						}
						init_unistr2(&data, url, UNI_STR_TERMINATE);
						fstrcpy(value.valuename, SPOOL_REG_URL);
#endif
					}

					if (strequal(reg_ctr->values[j]->valuename, SPOOL_REG_SERVERNAME)) {

						init_unistr2(&data, longname, UNI_STR_TERMINATE);
						fstrcpy(value.valuename, SPOOL_REG_SERVERNAME);
					}

					if (strequal(reg_ctr->values[j]->valuename, SPOOL_REG_SHORTSERVERNAME)) {

						init_unistr2(&data, global_myname(), UNI_STR_TERMINATE);
						fstrcpy(value.valuename, SPOOL_REG_SHORTSERVERNAME);
					}

					value.type = REG_SZ;
					value.size = data.uni_str_len * 2;
					value.data_p = TALLOC_MEMDUP(mem_ctx, data.buffer, value.size);

					if (opt_verbose) 
						display_reg_value(subkey, value);

					/* here we have to set all subkeys on the dst server */
					if (!net_spoolss_setprinterdataex(pipe_hnd_dst, mem_ctx, &hnd_dst, 
							subkey, &value)) 
						goto done;
							
				} else {

					if (opt_verbose) 
						display_reg_value(subkey, *(reg_ctr->values[j]));

					/* here we have to set all subkeys on the dst server */
					if (!net_spoolss_setprinterdataex(pipe_hnd_dst, mem_ctx, &hnd_dst, 
							subkey, reg_ctr->values[j])) 
						goto done;

				}

				DEBUGADD(1,("\tSetPrinterDataEx of key [%s\\%s] succeeded\n", 
						subkey, reg_ctr->values[j]->valuename));

			}

			TALLOC_FREE( reg_ctr );
		}

		safe_free(keylist);

		/* close printer handles here */
		if (got_hnd_src) {
			rpccli_spoolss_close_printer(pipe_hnd, mem_ctx, &hnd_src);
			got_hnd_src = False;
		}

		if (got_hnd_dst) {
			rpccli_spoolss_close_printer(pipe_hnd_dst, mem_ctx, &hnd_dst);
			got_hnd_dst = False;
		}

	}
	
	nt_status = NT_STATUS_OK;

done:
	SAFE_FREE(devicename);
	SAFE_FREE(url);
	SAFE_FREE(unc_name);

	if (got_hnd_src)
		rpccli_spoolss_close_printer(pipe_hnd, mem_ctx, &hnd_src);

	if (got_hnd_dst)
		rpccli_spoolss_close_printer(pipe_hnd_dst, mem_ctx, &hnd_dst);

	if (cli_dst) {
		cli_shutdown(cli_dst);
	}
	return nt_status;
}
