/* 
 *  Unix SMB/CIFS implementation.
 *  DOS error routines
 *  Copyright (C) Tim Potter 2002.
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

/* DOS error codes.  please read doserr.h */

#include "includes.h"

typedef const struct
{
	const char *dos_errstr;
	WERROR werror;
} werror_code_struct;

werror_code_struct dos_errs[] =
{
	{ "WERR_OK", WERR_OK },
	{ "WERR_GENERAL_FAILURE", WERR_GENERAL_FAILURE },
	{ "WERR_BADFILE", WERR_BADFILE },
	{ "WERR_ACCESS_DENIED", WERR_ACCESS_DENIED },
	{ "WERR_BADFID", WERR_BADFID },
	{ "WERR_BADFUNC", WERR_BADFUNC },
	{ "WERR_INSUFFICIENT_BUFFER", WERR_INSUFFICIENT_BUFFER },
	{ "WERR_NO_SUCH_SHARE", WERR_NO_SUCH_SHARE },
	{ "WERR_ALREADY_EXISTS", WERR_ALREADY_EXISTS },
	{ "WERR_INVALID_PARAM", WERR_INVALID_PARAM },
	{ "WERR_NOT_SUPPORTED", WERR_NOT_SUPPORTED },
	{ "WERR_BAD_PASSWORD", WERR_BAD_PASSWORD },
	{ "WERR_NOMEM", WERR_NOMEM },
	{ "WERR_INVALID_NAME", WERR_INVALID_NAME },
	{ "WERR_UNKNOWN_LEVEL", WERR_UNKNOWN_LEVEL },
	{ "WERR_OBJECT_PATH_INVALID", WERR_OBJECT_PATH_INVALID },
	{ "WERR_NO_MORE_ITEMS", WERR_NO_MORE_ITEMS },
	{ "WERR_MORE_DATA", WERR_MORE_DATA },
	{ "WERR_UNKNOWN_PRINTER_DRIVER", WERR_UNKNOWN_PRINTER_DRIVER },
	{ "WERR_INVALID_PRINTER_NAME", WERR_INVALID_PRINTER_NAME },
	{ "WERR_PRINTER_ALREADY_EXISTS", WERR_PRINTER_ALREADY_EXISTS },
	{ "WERR_INVALID_DATATYPE", WERR_INVALID_DATATYPE },
	{ "WERR_INVALID_ENVIRONMENT", WERR_INVALID_ENVIRONMENT },
	{ "WERR_INVALID_FORM_NAME", WERR_INVALID_FORM_NAME },
	{ "WERR_INVALID_FORM_SIZE", WERR_INVALID_FORM_SIZE },
	{ "WERR_BUF_TOO_SMALL", WERR_BUF_TOO_SMALL },
	{ "WERR_JOB_NOT_FOUND", WERR_JOB_NOT_FOUND },
	{ "WERR_DEST_NOT_FOUND", WERR_DEST_NOT_FOUND },
	{ "WERR_NOT_LOCAL_DOMAIN", WERR_NOT_LOCAL_DOMAIN },
	{ "WERR_PRINTER_DRIVER_IN_USE", WERR_PRINTER_DRIVER_IN_USE },
	{ "WERR_STATUS_MORE_ENTRIES  ", WERR_STATUS_MORE_ENTRIES },
	{ "WERR_DFS_NO_SUCH_VOL", WERR_DFS_NO_SUCH_VOL },
	{ "WERR_DFS_NO_SUCH_SHARE", WERR_DFS_NO_SUCH_SHARE },
	{ "WERR_DFS_NO_SUCH_SERVER", WERR_DFS_NO_SUCH_SERVER },
	{ "WERR_DFS_INTERNAL_ERROR", WERR_DFS_INTERNAL_ERROR },
	{ "WERR_DFS_CANT_CREATE_JUNCT", WERR_DFS_CANT_CREATE_JUNCT },
	{ "WERR_MACHINE_LOCKED", WERR_MACHINE_LOCKED },
	{ "WERR_NO_LOGON_SERVERS", WERR_NO_LOGON_SERVERS },
	{ "WERR_NO_SUCH_DOMAIN", WERR_NO_SUCH_DOMAIN },
	{ "WERR_INVALID_SECURITY_DESCRIPTOR", WERR_INVALID_SECURITY_DESCRIPTOR },
	{ "WERR_INVALID_OWNER", WERR_INVALID_OWNER },
	{ "WERR_SERVER_UNAVAILABLE", WERR_SERVER_UNAVAILABLE },
	{ "WERR_IO_PENDING", WERR_IO_PENDING },
	{ "WERR_INVALID_SERVICE_CONTROL", WERR_INVALID_SERVICE_CONTROL },
	{ "WERR_NET_NAME_NOT_FOUND", WERR_NET_NAME_NOT_FOUND },
	{ "WERR_REG_CORRUPT", WERR_REG_CORRUPT },
	{ "WERR_REG_IO_FAILURE", WERR_REG_IO_FAILURE },
	{ "WERR_REG_FILE_INVALID", WERR_REG_FILE_INVALID },
	{ "WERR_SERVICE_DISABLED", WERR_SERVICE_DISABLED },
	{ NULL, W_ERROR(0) }
};

/*****************************************************************************
 returns a DOS error message.  not amazingly helpful, but better than a number.
 *****************************************************************************/
const char *dos_errstr(WERROR werror)
{
        static pstring msg;
        int idx = 0;

	slprintf(msg, sizeof(msg), "DOS code 0x%08x", W_ERROR_V(werror));

	while (dos_errs[idx].dos_errstr != NULL) {
		if (W_ERROR_V(dos_errs[idx].werror) == 
                    W_ERROR_V(werror))
                        return dos_errs[idx].dos_errstr;
		idx++;
	}

        return msg;
}
