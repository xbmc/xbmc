/* 
   Unix SMB/Netbios implementation.
   Version 3.0
   MSDfs services for Samba
   Copyright (C) Shirish Kalele 2000

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

#ifndef _MSDFS_H
#define _MSDFS_H

#define REFERRAL_TTL 600

/* Flags used in trans2 Get Referral reply */
#define DFSREF_REFERRAL_SERVER 0x1
#define DFSREF_STORAGE_SERVER  0x2

/* Referral sizes */
#define VERSION2_REFERRAL_SIZE 0x16
#define VERSION3_REFERRAL_SIZE 0x22
#define REFERRAL_HEADER_SIZE 0x08

/* Maximum number of referrals for each Dfs volume */
#define MAX_REFERRAL_COUNT   256
#define MAX_MSDFS_JUNCTIONS 256

typedef struct _client_referral {
	uint32 proximity;
	uint32 ttl;
	pstring dfspath;
} CLIENT_DFS_REFERRAL;

struct referral {
	pstring alternate_path; /* contains the path referred */
	uint32 proximity;
	uint32 ttl; /* how long should client cache referral */
};

struct junction_map {
	pstring service_name;
	pstring volume_name;
	pstring comment;
	int referral_count;
	struct referral* referral_list;
};

struct dfs_path {
	pstring hostname;
	pstring servicename;
	pstring reqpath;
};

#define RESOLVE_DFSPATH(name, conn, inbuf, outbuf)           	\
{ if ((SVAL(inbuf,smb_flg2) & FLAGS2_DFS_PATHNAMES) &&       	\
      lp_host_msdfs() && lp_msdfs_root(SNUM(conn)) &&		\
      dfs_redirect(name, conn, False))				\
             return ERROR_BOTH(NT_STATUS_PATH_NOT_COVERED,	\
			       ERRSRV, ERRbadpath);; }		

#define RESOLVE_DFSPATH_WCARD(name, conn, inbuf, outbuf)        \
{ if ((SVAL(inbuf,smb_flg2) & FLAGS2_DFS_PATHNAMES) &&       	\
      lp_host_msdfs() && lp_msdfs_root(SNUM(conn)) &&		\
      dfs_redirect(name,conn, True))				\
             return ERROR_BOTH(NT_STATUS_PATH_NOT_COVERED,	\
			       ERRSRV, ERRbadpath);; }		

#define init_dfsroot(conn, inbuf, outbuf)                    	\
{ if (lp_msdfs_root(SNUM(conn)) && lp_host_msdfs()) {        	\
        DEBUG(2,("Serving %s as a Dfs root\n", 			\
		 lp_servicename(SNUM(conn)) )); 		\
	SSVAL(outbuf, smb_vwv2, SMB_SHARE_IN_DFS 		\
	      | SVAL(outbuf, smb_vwv2));   			\
} }

#endif /* _MSDFS_H */
