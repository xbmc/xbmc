/* 
   Unix SMB/CIFS implementation.
   SAM module tester

   Copyright (C) Jelmer Vernooij 2002

   Most of this code was ripped off of rpcclient.
   Copyright (C) Tim Potter 2000-2001

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

struct samtest_state {
	SAM_CONTEXT *context;
	NT_USER_TOKEN *token;
};

struct cmd_set {
	char *name;
	NTSTATUS (*fn)(struct samtest_state *sam, TALLOC_CTX *mem_ctx, int argc, 
                       char **argv);
	char *description;
	char *usage;
};


