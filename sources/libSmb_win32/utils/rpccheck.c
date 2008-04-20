/* 
   Unix SMB/CIFS implementation.
   
   Copyright (C) Jean François Micouleau 2001
   
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

main()
{
	char filter[]="0123456789ABCDEF";

	char s[128];
	char d=0;
	int x=0;
	prs_struct ps;
	TALLOC_CTX *ctx;

	/* change that struct */
	SAMR_R_QUERY_USERINFO rpc_stub;
	
	ZERO_STRUCT(rpc_stub);

	setup_logging("", True);
	DEBUGLEVEL=10;

	ctx=talloc_init("main");
	if (!ctx) exit(1);

	prs_init(&ps, 1600, 4, ctx, MARSHALL);

	while (scanf("%s", s)!=-1) {
		if (strlen(s)==2 && strchr_m(filter, *s)!=NULL && strchr_m(filter, *(s+1))!=NULL) {
			d=strtol(s, NULL, 16);
			if(!prs_append_data(&ps, &d, 1))
				printf("error while reading data\n");
		}
	}
	
	prs_switch_type(&ps, UNMARSHALL);
	prs_set_offset(&ps, 0);
	
	/* change that call */	
	if(!samr_io_r_query_userinfo("", &rpc_stub, &ps, 0))
		printf("error while UNMARSHALLING the data\n");

	printf("\n");
}
