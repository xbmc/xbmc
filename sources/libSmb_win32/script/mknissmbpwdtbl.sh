#!/bin/sh
#
# Copyright (C) 1998 Benny Holmgren
#
# Creates smbpasswd table and smb group in NIS+
#

nistbladm \
    -D access=og=rmcd,nw= -c \
    -s : smbpasswd_tbl \
    	name=S,nogw=r \
    	uid=S,nogw=r \
		user_rid=S,nogw=r \
		smb_grpid=,nw+r \
		group_rid=,nw+r \
		acb=,nw+r \
		          \
    	lmpwd=C,nw=,g=r,o=rm \
    	ntpwd=C,nw=,g=r,o=rm \
		                     \
		logon_t=,nw+r \
		logoff_t=,nw+r \
		kick_t=,nw+r \
		pwdlset_t=,nw+r \
		pwdlchg_t=,nw+r \
		pwdmchg_t=,nw+r \
		                \
		full_name=,nw+r \
		home_dir=,nw+r \
		dir_drive=,nw+r \
		logon_script=,nw+r \
		profile_path=,nw+r \
		acct_desc=,nw+r \
		workstations=,nw+r \
		                   \
		hours=,nw+r \
	smbpasswd.org_dir.`nisdefaults -d`

nisgrpadm -c smb.`nisdefaults -d`

nischgrp smb.`nisdefaults -d` smbpasswd.org_dir.`nisdefaults -d`

