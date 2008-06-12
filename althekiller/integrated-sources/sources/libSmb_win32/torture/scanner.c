/* 
   Unix SMB/CIFS implementation.
   SMB torture tester - scanning functions
   Copyright (C) Andrew Tridgell 2001
   
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

#define VERBOSE 0
#define OP_MIN 0
#define OP_MAX 20

/****************************************************************************
look for a partial hit
****************************************************************************/
static void trans2_check_hit(const char *format, int op, int level, NTSTATUS status)
{
	if (NT_STATUS_V(status) == NT_STATUS_V(NT_STATUS_INVALID_LEVEL) ||
	    NT_STATUS_V(status) == NT_STATUS_V(NT_STATUS_NOT_IMPLEMENTED) ||
	    NT_STATUS_V(status) == NT_STATUS_V(NT_STATUS_NOT_SUPPORTED) ||
	    NT_STATUS_V(status) == NT_STATUS_V(NT_STATUS_UNSUCCESSFUL) ||
	    NT_STATUS_V(status) == NT_STATUS_V(NT_STATUS_INVALID_INFO_CLASS)) {
		return;
	}
#if VERBOSE
	printf("possible %s hit op=%3d level=%5d status=%s\n",
	       format, op, level, nt_errstr(status));
#endif
}

/****************************************************************************
check for existance of a trans2 call
****************************************************************************/
static NTSTATUS try_trans2(struct cli_state *cli, 
			 int op,
			 char *param, char *data,
			 int param_len, int data_len,
			 int *rparam_len, int *rdata_len)
{
	uint16 setup = op;
	char *rparam=NULL, *rdata=NULL;

	if (!cli_send_trans(cli, SMBtrans2, 
                            NULL,                           /* name */
                            -1, 0,                          /* fid, flags */
                            &setup, 1, 0,                   /* setup, length, max */
                            param, param_len, 2,            /* param, length, max */
                            data, data_len, cli->max_xmit   /* data, length, max */
                           )) {
		return cli_nt_error(cli);
	}

	cli_receive_trans(cli, SMBtrans2,
			   &rparam, rparam_len,
			   &rdata, rdata_len);

	SAFE_FREE(rdata);
	SAFE_FREE(rparam);

	return cli_nt_error(cli);
}


static NTSTATUS try_trans2_len(struct cli_state *cli, 
			     const char *format,
			     int op, int level,
			     char *param, char *data,
			     int param_len, int *data_len,
			     int *rparam_len, int *rdata_len)
{
	NTSTATUS ret=NT_STATUS_OK;

	ret = try_trans2(cli, op, param, data, param_len,
			 sizeof(pstring), rparam_len, rdata_len);
#if VERBOSE 
	printf("op=%d level=%d ret=%s\n", op, level, nt_errstr(ret));
#endif
	if (!NT_STATUS_IS_OK(ret)) return ret;

	*data_len = 0;
	while (*data_len < sizeof(pstring)) {
		ret = try_trans2(cli, op, param, data, param_len,
				 *data_len, rparam_len, rdata_len);
		if (NT_STATUS_IS_OK(ret)) break;
		*data_len += 2;
	}
	if (NT_STATUS_IS_OK(ret)) {
		printf("found %s level=%d data_len=%d rparam_len=%d rdata_len=%d\n",
		       format, level, *data_len, *rparam_len, *rdata_len);
	} else {
		trans2_check_hit(format, op, level, ret);
	}
	return ret;
}

/****************************************************************************
check for existance of a trans2 call
****************************************************************************/
static BOOL scan_trans2(struct cli_state *cli, int op, int level, 
			int fnum, int dnum, const char *fname)
{
	int data_len = 0;
	int param_len = 0;
	int rparam_len, rdata_len;
	pstring param, data;
	NTSTATUS status;

	memset(data, 0, sizeof(data));
	data_len = 4;

	/* try with a info level only */
	param_len = 2;
	SSVAL(param, 0, level);
	status = try_trans2_len(cli, "void", op, level, param, data, param_len, &data_len, 
			    &rparam_len, &rdata_len);
	if (NT_STATUS_IS_OK(status)) return True;

	/* try with a file descriptor */
	param_len = 6;
	SSVAL(param, 0, fnum);
	SSVAL(param, 2, level);
	SSVAL(param, 4, 0);
	status = try_trans2_len(cli, "fnum", op, level, param, data, param_len, &data_len, 
				&rparam_len, &rdata_len);
	if (NT_STATUS_IS_OK(status)) return True;


	/* try with a notify style */
	param_len = 6;
	SSVAL(param, 0, dnum);
	SSVAL(param, 2, dnum);
	SSVAL(param, 4, level);
	status = try_trans2_len(cli, "notify", op, level, param, data, param_len, &data_len, 
				&rparam_len, &rdata_len);
	if (NT_STATUS_IS_OK(status)) return True;

	/* try with a file name */
	param_len = 6;
	SSVAL(param, 0, level);
	SSVAL(param, 2, 0);
	SSVAL(param, 4, 0);
	param_len += clistr_push(cli, &param[6], fname, -1, STR_TERMINATE);

	status = try_trans2_len(cli, "fname", op, level, param, data, param_len, &data_len, 
				&rparam_len, &rdata_len);
	if (NT_STATUS_IS_OK(status)) return True;

	/* try with a new file name */
	param_len = 6;
	SSVAL(param, 0, level);
	SSVAL(param, 2, 0);
	SSVAL(param, 4, 0);
	param_len += clistr_push(cli, &param[6], "\\newfile.dat", -1, STR_TERMINATE);

	status = try_trans2_len(cli, "newfile", op, level, param, data, param_len, &data_len, 
				&rparam_len, &rdata_len);
	cli_unlink(cli, "\\newfile.dat");
	cli_rmdir(cli, "\\newfile.dat");
	if (NT_STATUS_IS_OK(status)) return True;

	/* try dfs style  */
	cli_mkdir(cli, "\\testdir");
	param_len = 2;
	SSVAL(param, 0, level);
	param_len += clistr_push(cli, &param[2], "\\testdir", -1, STR_TERMINATE);

	status = try_trans2_len(cli, "dfs", op, level, param, data, param_len, &data_len, 
				&rparam_len, &rdata_len);
	cli_rmdir(cli, "\\testdir");
	if (NT_STATUS_IS_OK(status)) return True;

	return False;
}


BOOL torture_trans2_scan(int dummy)
{
	static struct cli_state *cli;
	int op, level;
	const char *fname = "\\scanner.dat";
	int fnum, dnum;

	printf("starting trans2 scan test\n");

	if (!torture_open_connection(&cli)) {
		return False;
	}

	fnum = cli_open(cli, fname, O_RDWR | O_CREAT | O_TRUNC, 
			 DENY_NONE);
	dnum = cli_open(cli, "\\", O_RDONLY, DENY_NONE);

	for (op=OP_MIN; op<=OP_MAX; op++) {
		printf("Scanning op=%d\n", op);
		for (level = 0; level <= 50; level++) {
			scan_trans2(cli, op, level, fnum, dnum, fname);
		}

		for (level = 0x100; level <= 0x130; level++) {
			scan_trans2(cli, op, level, fnum, dnum, fname);
		}

		for (level = 1000; level < 1050; level++) {
			scan_trans2(cli, op, level, fnum, dnum, fname);
		}
	}

	torture_close_connection(cli);

	printf("trans2 scan finished\n");
	return True;
}




/****************************************************************************
look for a partial hit
****************************************************************************/
static void nttrans_check_hit(const char *format, int op, int level, NTSTATUS status)
{
	if (NT_STATUS_V(status) == NT_STATUS_V(NT_STATUS_INVALID_LEVEL) ||
	    NT_STATUS_V(status) == NT_STATUS_V(NT_STATUS_NOT_IMPLEMENTED) ||
	    NT_STATUS_V(status) == NT_STATUS_V(NT_STATUS_NOT_SUPPORTED) ||
	    NT_STATUS_V(status) == NT_STATUS_V(NT_STATUS_UNSUCCESSFUL) ||
	    NT_STATUS_V(status) == NT_STATUS_V(NT_STATUS_INVALID_INFO_CLASS)) {
		return;
	}
#if VERBOSE
		printf("possible %s hit op=%3d level=%5d status=%s\n",
		       format, op, level, nt_errstr(status));
#endif
}

/****************************************************************************
check for existance of a nttrans call
****************************************************************************/
static NTSTATUS try_nttrans(struct cli_state *cli, 
			 int op,
			 char *param, char *data,
			 int param_len, int data_len,
			 int *rparam_len, int *rdata_len)
{
	char *rparam=NULL, *rdata=NULL;

	if (!cli_send_nt_trans(cli, op, 
			       0,   
			       NULL, 0, 0,
			       param, param_len, 2,            /* param, length, max */
			       data, data_len, cli->max_xmit   /* data, length, max */
                           )) {
		return cli_nt_error(cli);
	}

	cli_receive_nt_trans(cli,
			     &rparam, rparam_len,
			     &rdata, rdata_len);

	SAFE_FREE(rdata);
	SAFE_FREE(rparam);

	return cli_nt_error(cli);
}


static NTSTATUS try_nttrans_len(struct cli_state *cli, 
			     const char *format,
			     int op, int level,
			     char *param, char *data,
			     int param_len, int *data_len,
			     int *rparam_len, int *rdata_len)
{
	NTSTATUS ret=NT_STATUS_OK;

	ret = try_nttrans(cli, op, param, data, param_len,
			 sizeof(pstring), rparam_len, rdata_len);
#if VERBOSE 
	printf("op=%d level=%d ret=%s\n", op, level, nt_errstr(ret));
#endif
	if (!NT_STATUS_IS_OK(ret)) return ret;

	*data_len = 0;
	while (*data_len < sizeof(pstring)) {
		ret = try_nttrans(cli, op, param, data, param_len,
				 *data_len, rparam_len, rdata_len);
		if (NT_STATUS_IS_OK(ret)) break;
		*data_len += 2;
	}
	if (NT_STATUS_IS_OK(ret)) {
		printf("found %s level=%d data_len=%d rparam_len=%d rdata_len=%d\n",
		       format, level, *data_len, *rparam_len, *rdata_len);
	} else {
		nttrans_check_hit(format, op, level, ret);
	}
	return ret;
}

/****************************************************************************
check for existance of a nttrans call
****************************************************************************/
static BOOL scan_nttrans(struct cli_state *cli, int op, int level, 
			int fnum, int dnum, const char *fname)
{
	int data_len = 0;
	int param_len = 0;
	int rparam_len, rdata_len;
	pstring param, data;
	NTSTATUS status;

	memset(data, 0, sizeof(data));
	data_len = 4;

	/* try with a info level only */
	param_len = 2;
	SSVAL(param, 0, level);
	status = try_nttrans_len(cli, "void", op, level, param, data, param_len, &data_len, 
			    &rparam_len, &rdata_len);
	if (NT_STATUS_IS_OK(status)) return True;

	/* try with a file descriptor */
	param_len = 6;
	SSVAL(param, 0, fnum);
	SSVAL(param, 2, level);
	SSVAL(param, 4, 0);
	status = try_nttrans_len(cli, "fnum", op, level, param, data, param_len, &data_len, 
				&rparam_len, &rdata_len);
	if (NT_STATUS_IS_OK(status)) return True;


	/* try with a notify style */
	param_len = 6;
	SSVAL(param, 0, dnum);
	SSVAL(param, 2, dnum);
	SSVAL(param, 4, level);
	status = try_nttrans_len(cli, "notify", op, level, param, data, param_len, &data_len, 
				&rparam_len, &rdata_len);
	if (NT_STATUS_IS_OK(status)) return True;

	/* try with a file name */
	param_len = 6;
	SSVAL(param, 0, level);
	SSVAL(param, 2, 0);
	SSVAL(param, 4, 0);
	param_len += clistr_push(cli, &param[6], fname, -1, STR_TERMINATE);

	status = try_nttrans_len(cli, "fname", op, level, param, data, param_len, &data_len, 
				&rparam_len, &rdata_len);
	if (NT_STATUS_IS_OK(status)) return True;

	/* try with a new file name */
	param_len = 6;
	SSVAL(param, 0, level);
	SSVAL(param, 2, 0);
	SSVAL(param, 4, 0);
	param_len += clistr_push(cli, &param[6], "\\newfile.dat", -1, STR_TERMINATE);

	status = try_nttrans_len(cli, "newfile", op, level, param, data, param_len, &data_len, 
				&rparam_len, &rdata_len);
	cli_unlink(cli, "\\newfile.dat");
	cli_rmdir(cli, "\\newfile.dat");
	if (NT_STATUS_IS_OK(status)) return True;

	/* try dfs style  */
	cli_mkdir(cli, "\\testdir");
	param_len = 2;
	SSVAL(param, 0, level);
	param_len += clistr_push(cli, &param[2], "\\testdir", -1, STR_TERMINATE);

	status = try_nttrans_len(cli, "dfs", op, level, param, data, param_len, &data_len, 
				&rparam_len, &rdata_len);
	cli_rmdir(cli, "\\testdir");
	if (NT_STATUS_IS_OK(status)) return True;

	return False;
}


BOOL torture_nttrans_scan(int dummy)
{
	static struct cli_state *cli;
	int op, level;
	const char *fname = "\\scanner.dat";
	int fnum, dnum;

	printf("starting nttrans scan test\n");

	if (!torture_open_connection(&cli)) {
		return False;
	}

	fnum = cli_open(cli, fname, O_RDWR | O_CREAT | O_TRUNC, 
			 DENY_NONE);
	dnum = cli_open(cli, "\\", O_RDONLY, DENY_NONE);

	for (op=OP_MIN; op<=OP_MAX; op++) {
		printf("Scanning op=%d\n", op);
		for (level = 0; level <= 50; level++) {
			scan_nttrans(cli, op, level, fnum, dnum, fname);
		}

		for (level = 0x100; level <= 0x130; level++) {
			scan_nttrans(cli, op, level, fnum, dnum, fname);
		}

		for (level = 1000; level < 1050; level++) {
			scan_nttrans(cli, op, level, fnum, dnum, fname);
		}
	}

	torture_close_connection(cli);

	printf("nttrans scan finished\n");
	return True;
}
