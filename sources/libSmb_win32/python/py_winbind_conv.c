/* 
   Python wrappers for DCERPC/SMB client routines.

   Copyright (C) Tim Potter, 2002
   
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

#include "python/py_common.h"
#include "python/py_conv.h"

/* Convert a struct passwd to a dictionary */

static struct pyconv py_passwd[] = {
	{ "pw_name", PY_STRING, offsetof(struct winbindd_response, data.pw.pw_name) },
	{ "pw_passwd", PY_STRING, offsetof(struct winbindd_response, data.pw.pw_passwd) },
	{ "pw_uid", PY_UID, offsetof(struct winbindd_response, data.pw.pw_uid) },
	{ "pw_guid", PY_GID, offsetof(struct winbindd_response, data.pw.pw_gid) },
	{ "pw_gecos", PY_STRING, offsetof(struct winbindd_response, data.pw.pw_gecos) },
	{ "pw_dir", PY_STRING, offsetof(struct winbindd_response, data.pw.pw_dir) },
	{ "pw_shell", PY_STRING, offsetof(struct winbindd_response, data.pw.pw_shell) },
	{ NULL} 
};

BOOL py_from_winbind_passwd(PyObject **dict, struct winbindd_response *response)
{
	*dict = from_struct(response, py_passwd);
	return True;
}
