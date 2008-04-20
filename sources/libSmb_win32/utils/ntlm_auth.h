/* 
   Samba Unix/Linux NTLM authentication tool
   
   Copyright (C) 2001 Andrew Bartlett (abartlet@samba.org)

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

#include "utils/ntlm_auth_proto.h"
 
/* Some of the popt variables are needed in the diagnostics code */
extern const char *opt_username;
extern const char *opt_domain;
extern const char *opt_workstation;
extern const char *opt_password;

