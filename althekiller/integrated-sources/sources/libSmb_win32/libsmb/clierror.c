/* 
   Unix SMB/CIFS implementation.
   client error handling routines
   Copyright (C) Andrew Tridgell 1994-1998
   Copyright (C) Jelmer Vernooij 2003
   Copyright (C) Jeremy Allison 2006
   
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

/*****************************************************
 RAP error codes - a small start but will be extended.

 XXX: Perhaps these should move into a common function because they're
 duplicated in clirap2.c

*******************************************************/

static const struct {
	int err;
	const char *message;
} rap_errmap[] = {
	{5,    "RAP5: User has insufficient privilege" },
	{50,   "RAP50: Not supported by server" },
	{65,   "RAP65: Access denied" },
	{86,   "RAP86: The specified password is invalid" },
	{2220, "RAP2220: Group does not exist" },
	{2221, "RAP2221: User does not exist" },
	{2226, "RAP2226: Operation only permitted on a Primary Domain Controller"  },
	{2237, "RAP2237: User is not in group" },
	{2242, "RAP2242: The password of this user has expired." },
	{2243, "RAP2243: The password of this user cannot change." },
	{2244, "RAP2244: This password cannot be used now (password history conflict)." },
	{2245, "RAP2245: The password is shorter than required." },
	{2246, "RAP2246: The password of this user is too recent to change."},

	/* these really shouldn't be here ... */
	{0x80, "Not listening on called name"},
	{0x81, "Not listening for calling name"},
	{0x82, "Called name not present"},
	{0x83, "Called name present, but insufficient resources"},

	{0, NULL}
};  

/****************************************************************************
 Return a description of an SMB error.
****************************************************************************/

static const char *cli_smb_errstr(struct cli_state *cli)
{
	return smb_dos_errstr(cli->inbuf);
}

/****************************************************************************
 Convert a socket error into an NTSTATUS.
****************************************************************************/

static NTSTATUS cli_smb_rw_error_to_ntstatus(struct cli_state *cli)
{
	switch(cli->smb_rw_error) {
		case READ_TIMEOUT:
			return NT_STATUS_IO_TIMEOUT;
		case READ_EOF:
			return NT_STATUS_END_OF_FILE;
		/* What we shoud really do for read/write errors is convert from errno. */
		/* FIXME. JRA. */
		case READ_ERROR:
			return NT_STATUS_INVALID_NETWORK_RESPONSE;
		case WRITE_ERROR:
			return NT_STATUS_UNEXPECTED_NETWORK_ERROR;
	        case READ_BAD_SIG:
			return NT_STATUS_INVALID_PARAMETER;
	        default:
			break;
	}
	return NT_STATUS_UNSUCCESSFUL;
}

/***************************************************************************
 Return an error message - either an NT error, SMB error or a RAP error.
 Note some of the NT errors are actually warnings or "informational" errors
 in which case they can be safely ignored.
****************************************************************************/

const char *cli_errstr(struct cli_state *cli)
{   
	static fstring cli_error_message;
	uint32 flgs2 = SVAL(cli->inbuf,smb_flg2), errnum;
	uint8 errclass;
	int i;

	if (!cli->initialised) {
		fstrcpy(cli_error_message, "[Programmer's error] cli_errstr called on unitialized cli_stat struct!\n");
		return cli_error_message;
	}

	/* Was it server socket error ? */
	if (cli->fd == -1 && cli->smb_rw_error) {
		switch(cli->smb_rw_error) {
			case READ_TIMEOUT:
				slprintf(cli_error_message, sizeof(cli_error_message) - 1,
					"Call timed out: server did not respond after %d milliseconds",
					cli->timeout);
				break;
			case READ_EOF:
				slprintf(cli_error_message, sizeof(cli_error_message) - 1,
					"Call returned zero bytes (EOF)" );
				break;
			case READ_ERROR:
				slprintf(cli_error_message, sizeof(cli_error_message) - 1,
					"Read error: %s", strerror(errno) );
				break;
			case WRITE_ERROR:
				slprintf(cli_error_message, sizeof(cli_error_message) - 1,
					"Write error: %s", strerror(errno) );
				break;
		        case READ_BAD_SIG:
				slprintf(cli_error_message, sizeof(cli_error_message) - 1,
					"Server packet had invalid SMB signature!");
				break;
		        default:
				slprintf(cli_error_message, sizeof(cli_error_message) - 1,
					"Unknown error code %d\n", cli->smb_rw_error );
				break;
		}
		return cli_error_message;
	}

	/* Case #1: RAP error */
	if (cli->rap_error) {
		for (i = 0; rap_errmap[i].message != NULL; i++) {
			if (rap_errmap[i].err == cli->rap_error) {
				return rap_errmap[i].message;
			}
		}

		slprintf(cli_error_message, sizeof(cli_error_message) - 1, "RAP code %d",
			cli->rap_error);

		return cli_error_message;
	}

	/* Case #2: 32-bit NT errors */
	if (flgs2 & FLAGS2_32_BIT_ERROR_CODES) {
		NTSTATUS status = NT_STATUS(IVAL(cli->inbuf,smb_rcls));

		return nt_errstr(status);
        }

	cli_dos_error(cli, &errclass, &errnum);

	/* Case #3: SMB error */

	return cli_smb_errstr(cli);
}


/****************************************************************************
 Return the 32-bit NT status code from the last packet.
****************************************************************************/

NTSTATUS cli_nt_error(struct cli_state *cli)
{
        int flgs2 = SVAL(cli->inbuf,smb_flg2);

	/* Deal with socket errors first. */
	if (cli->fd == -1 && cli->smb_rw_error) {
		return cli_smb_rw_error_to_ntstatus(cli);
	}

	if (!(flgs2 & FLAGS2_32_BIT_ERROR_CODES)) {
		int e_class  = CVAL(cli->inbuf,smb_rcls);
		int code  = SVAL(cli->inbuf,smb_err);
		return dos_to_ntstatus(e_class, code);
        }

        return NT_STATUS(IVAL(cli->inbuf,smb_rcls));
}


/****************************************************************************
 Return the DOS error from the last packet - an error class and an error
 code.
****************************************************************************/

void cli_dos_error(struct cli_state *cli, uint8 *eclass, uint32 *ecode)
{
	int  flgs2;

	if(!cli->initialised) {
		return;
	}

	/* Deal with socket errors first. */
	if (cli->fd == -1 && cli->smb_rw_error) {
		NTSTATUS status = cli_smb_rw_error_to_ntstatus(cli);
		ntstatus_to_dos( status, eclass, ecode);
		return;
	}

	flgs2 = SVAL(cli->inbuf,smb_flg2);

	if (flgs2 & FLAGS2_32_BIT_ERROR_CODES) {
		NTSTATUS ntstatus = NT_STATUS(IVAL(cli->inbuf, smb_rcls));
		ntstatus_to_dos(ntstatus, eclass, ecode);
                return;
        }

	*eclass  = CVAL(cli->inbuf,smb_rcls);
	*ecode  = SVAL(cli->inbuf,smb_err);
}

/* Return a UNIX errno from a NT status code */
static struct {
	NTSTATUS status;
	int error;
} nt_errno_map[] = {
        {NT_STATUS_ACCESS_VIOLATION, EACCES},
        {NT_STATUS_INVALID_HANDLE, EBADF},
        {NT_STATUS_ACCESS_DENIED, EACCES},
        {NT_STATUS_OBJECT_NAME_NOT_FOUND, ENOENT},
#ifdef _XBOX /* otherwise it get's mapped to a worse error */
        {NT_STATUS_OBJECT_PATH_NOT_FOUND, ENOENT},
#endif
        {NT_STATUS_SHARING_VIOLATION, EBUSY},
        {NT_STATUS_OBJECT_PATH_INVALID, ENOTDIR},
        {NT_STATUS_OBJECT_NAME_COLLISION, EEXIST},
        {NT_STATUS_PATH_NOT_COVERED, ENOENT},
	{NT_STATUS_UNSUCCESSFUL, EINVAL},
	{NT_STATUS_NOT_IMPLEMENTED, ENOSYS},
	{NT_STATUS_IN_PAGE_ERROR, EFAULT}, 
	{NT_STATUS_BAD_NETWORK_NAME, ENOENT},
#ifdef EDQUOT
	{NT_STATUS_PAGEFILE_QUOTA, EDQUOT},
	{NT_STATUS_QUOTA_EXCEEDED, EDQUOT},
	{NT_STATUS_REGISTRY_QUOTA_LIMIT, EDQUOT},
	{NT_STATUS_LICENSE_QUOTA_EXCEEDED, EDQUOT},
#endif
#ifdef ETIME
	{NT_STATUS_TIMER_NOT_CANCELED, ETIME},
#endif
	{NT_STATUS_INVALID_PARAMETER, EINVAL},
	{NT_STATUS_NO_SUCH_DEVICE, ENODEV},
	{NT_STATUS_NO_SUCH_FILE, ENOENT},
#ifdef ENODATA
	{NT_STATUS_END_OF_FILE, ENODATA}, 
#endif
#ifdef ENOMEDIUM
	{NT_STATUS_NO_MEDIA_IN_DEVICE, ENOMEDIUM}, 
	{NT_STATUS_NO_MEDIA, ENOMEDIUM},
#endif
	{NT_STATUS_NONEXISTENT_SECTOR, ESPIPE}, 
        {NT_STATUS_NO_MEMORY, ENOMEM},
	{NT_STATUS_CONFLICTING_ADDRESSES, EADDRINUSE},
	{NT_STATUS_NOT_MAPPED_VIEW, EINVAL},
	{NT_STATUS_UNABLE_TO_FREE_VM, EADDRINUSE},
	{NT_STATUS_ACCESS_DENIED, EACCES}, 
	{NT_STATUS_BUFFER_TOO_SMALL, ENOBUFS},
	{NT_STATUS_WRONG_PASSWORD, EACCES},
	{NT_STATUS_LOGON_FAILURE, EACCES},
	{NT_STATUS_INVALID_WORKSTATION, EACCES},
	{NT_STATUS_INVALID_LOGON_HOURS, EACCES},
	{NT_STATUS_PASSWORD_EXPIRED, EACCES},
	{NT_STATUS_ACCOUNT_DISABLED, EACCES},
	{NT_STATUS_DISK_FULL, ENOSPC},
	{NT_STATUS_INVALID_PIPE_STATE, EPIPE},
	{NT_STATUS_PIPE_BUSY, EPIPE},
	{NT_STATUS_PIPE_DISCONNECTED, EPIPE},
	{NT_STATUS_PIPE_NOT_AVAILABLE, ENOSYS},
	{NT_STATUS_FILE_IS_A_DIRECTORY, EISDIR},
	{NT_STATUS_NOT_SUPPORTED, ENOSYS},
	{NT_STATUS_NOT_A_DIRECTORY, ENOTDIR},
	{NT_STATUS_DIRECTORY_NOT_EMPTY, ENOTEMPTY},
	{NT_STATUS_NETWORK_UNREACHABLE, ENETUNREACH},
	{NT_STATUS_HOST_UNREACHABLE, EHOSTUNREACH},
	{NT_STATUS_CONNECTION_ABORTED, ECONNABORTED},
	{NT_STATUS_CONNECTION_REFUSED, ECONNREFUSED},
	{NT_STATUS_TOO_MANY_LINKS, EMLINK},
	{NT_STATUS_NETWORK_BUSY, EBUSY},
	{NT_STATUS_DEVICE_DOES_NOT_EXIST, ENODEV},
#ifdef ELIBACC
	{NT_STATUS_DLL_NOT_FOUND, ELIBACC},
#endif
	{NT_STATUS_PIPE_BROKEN, EPIPE},
	{NT_STATUS_REMOTE_NOT_LISTENING, ECONNREFUSED},
	{NT_STATUS_NETWORK_ACCESS_DENIED, EACCES},
	{NT_STATUS_TOO_MANY_OPENED_FILES, EMFILE},
#ifdef EPROTO
	{NT_STATUS_DEVICE_PROTOCOL_ERROR, EPROTO},
#endif
	{NT_STATUS_FLOAT_OVERFLOW, ERANGE},
	{NT_STATUS_FLOAT_UNDERFLOW, ERANGE},
	{NT_STATUS_INTEGER_OVERFLOW, ERANGE},
	{NT_STATUS_MEDIA_WRITE_PROTECTED, EROFS},
	{NT_STATUS_PIPE_CONNECTED, EISCONN},
	{NT_STATUS_MEMORY_NOT_ALLOCATED, EFAULT},
	{NT_STATUS_FLOAT_INEXACT_RESULT, ERANGE},
	{NT_STATUS_ILL_FORMED_PASSWORD, EACCES},
	{NT_STATUS_PASSWORD_RESTRICTION, EACCES},
	{NT_STATUS_ACCOUNT_RESTRICTION, EACCES},
	{NT_STATUS_PORT_CONNECTION_REFUSED, ECONNREFUSED},
	{NT_STATUS_NAME_TOO_LONG, ENAMETOOLONG},
	{NT_STATUS_REMOTE_DISCONNECT, ESHUTDOWN},
	{NT_STATUS_CONNECTION_DISCONNECTED, ECONNABORTED},
	{NT_STATUS_CONNECTION_RESET, ENETRESET},
#ifdef ENOTUNIQ
	{NT_STATUS_IP_ADDRESS_CONFLICT1, ENOTUNIQ},
	{NT_STATUS_IP_ADDRESS_CONFLICT2, ENOTUNIQ},
#endif
	{NT_STATUS_PORT_MESSAGE_TOO_LONG, EMSGSIZE},
	{NT_STATUS_PROTOCOL_UNREACHABLE, ENOPROTOOPT},
	{NT_STATUS_ADDRESS_ALREADY_EXISTS, EADDRINUSE},
	{NT_STATUS_PORT_UNREACHABLE, EHOSTUNREACH},
	{NT_STATUS_IO_TIMEOUT, ETIMEDOUT},
	{NT_STATUS_RETRY, EAGAIN},
#ifdef ENOTUNIQ
	{NT_STATUS_DUPLICATE_NAME, ENOTUNIQ},
#endif
#ifdef ECOMM
	{NT_STATUS_NET_WRITE_FAULT, ECOMM},
#endif

	{NT_STATUS(0), 0}
};

/****************************************************************************
 The following mappings need tidying up and moving into libsmb/errormap.c...
****************************************************************************/

static int cli_errno_from_nt(NTSTATUS status)
{
	int i;
        DEBUG(10,("cli_errno_from_nt: 32 bit codes: code=%08x\n", NT_STATUS_V(status)));

        /* Status codes without this bit set are not errors */

        if (!(NT_STATUS_V(status) & 0xc0000000)) {
                return 0;
	}

	for (i=0;nt_errno_map[i].error;i++) {
		if (NT_STATUS_V(nt_errno_map[i].status) ==
		    NT_STATUS_V(status)) return nt_errno_map[i].error;
	}

        /* for all other cases - a default code */
        return EINVAL;
}

/* Return a UNIX errno appropriate for the error received in the last
   packet. */

int cli_errno(struct cli_state *cli)
{
	NTSTATUS status;

	if (cli_is_nt_error(cli)) {
		status = cli_nt_error(cli);
		return cli_errno_from_nt(status);
	}

        if (cli_is_dos_error(cli)) {
                uint8 eclass;
                uint32 ecode;

                cli_dos_error(cli, &eclass, &ecode);
		status = dos_to_ntstatus(eclass, ecode);
		return cli_errno_from_nt(status);
        }

	/* for other cases */
	return EINVAL;
}

/* Return true if the last packet was in error */

BOOL cli_is_error(struct cli_state *cli)
{
	uint32 flgs2 = SVAL(cli->inbuf,smb_flg2), rcls = 0;

	/* A socket error is always an error. */
	if (cli->fd == -1 && cli->smb_rw_error != 0) {
		return True;
	}

        if (flgs2 & FLAGS2_32_BIT_ERROR_CODES) {
                /* Return error is error bits are set */
                rcls = IVAL(cli->inbuf, smb_rcls);
                return (rcls & 0xF0000000) == 0xC0000000;
        }
                
        /* Return error if error class in non-zero */

        rcls = CVAL(cli->inbuf, smb_rcls);
        return rcls != 0;
}

/* Return true if the last error was an NT error */

BOOL cli_is_nt_error(struct cli_state *cli)
{
	uint32 flgs2 = SVAL(cli->inbuf,smb_flg2);

	/* A socket error is always an NT error. */
	if (cli->fd == -1 && cli->smb_rw_error != 0) {
		return True;
	}

        return cli_is_error(cli) && (flgs2 & FLAGS2_32_BIT_ERROR_CODES);
}

/* Return true if the last error was a DOS error */

BOOL cli_is_dos_error(struct cli_state *cli)
{
	uint32 flgs2 = SVAL(cli->inbuf,smb_flg2);

	/* A socket error is always a DOS error. */
	if (cli->fd == -1 && cli->smb_rw_error != 0) {
		return True;
	}

        return cli_is_error(cli) && !(flgs2 & FLAGS2_32_BIT_ERROR_CODES);
}

/* Return the last error always as an NTSTATUS. */

NTSTATUS cli_get_nt_error(struct cli_state *cli)
{
	if (cli_is_nt_error(cli)) {
		return cli_nt_error(cli);
	} else if (cli_is_dos_error(cli)) {
		uint32 ecode;
		uint8 eclass;
		cli_dos_error(cli, &eclass, &ecode);
		return dos_to_ntstatus(eclass, ecode);
	} else {
		/* Something went wrong, we don't know what. */
		return NT_STATUS_UNSUCCESSFUL;
	}
}
