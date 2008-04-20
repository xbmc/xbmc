/* 
   Unix SMB/CIFS implementation.
   SMB wrapper functions - frontend
   Copyright (C) Andrew Tridgell 1998
   
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

static void smbsh_usage(void)
{
	printf("smbsh [options]\n\n");
	printf(" -W workgroup\n");
	printf(" -U username\n");
	printf(" -P prefix\n");
	printf(" -R resolve order\n");
	printf(" -d debug level\n");
	printf(" -l logfile\n");
	printf(" -L libdir\n");
	exit(0);
}

int main(int argc, char *argv[])
{
	char *p, *u;
	const char *libd = dyn_LIBDIR;
	pstring line, wd;
	int opt;
	extern char *optarg;
	extern int optind;

	dbf = x_stdout;
	smbw_setup_shared();

	while ((opt = getopt(argc, argv, "W:U:R:d:P:l:hL:")) != EOF) {
		switch (opt) {
		case 'L':
			libd = optarg;
			break;
		case 'W':
			smbw_setshared("WORKGROUP", optarg);
			break;
		case 'l':
			smbw_setshared("LOGFILE", optarg);
			break;
		case 'P':
			smbw_setshared("PREFIX", optarg);
			break;
		case 'd':
			smbw_setshared("DEBUG", optarg);
			break;
		case 'U':
			p = strchr_m(optarg,'%');
			if (p) {
				*p=0;
				smbw_setshared("PASSWORD",p+1);
			}
			smbw_setshared("USER", optarg);
			break;
		case 'R':
			smbw_setshared("RESOLVE_ORDER",optarg);
			break;

		case 'h':
		default:
			smbsh_usage();
		}
	}


	if (!smbw_getshared("USER")) {
		printf("Username: ");
		fflush(stdout);
		u = fgets_slash(line, sizeof(line)-1, x_stdin);
		smbw_setshared("USER", u);
	}

	if (!smbw_getshared("PASSWORD")) {
		p = getpass("Password: ");
		smbw_setshared("PASSWORD", p);
	}

	setenv("PS1", "smbsh$ ", 1);

	sys_getwd(wd);

	slprintf(line,sizeof(line)-1,"PWD_%d", (int)getpid());

	smbw_setshared(line, wd);

	slprintf(line,sizeof(line)-1,"%s/smbwrapper.so", libd);
	setenv("LD_PRELOAD", line, 1);

	slprintf(line,sizeof(line)-1,"%s/smbwrapper.32.so", libd);

	if (file_exist(line, NULL)) {
		slprintf(line,sizeof(line)-1,"%s/smbwrapper.32.so:DEFAULT", libd);
		setenv("_RLD_LIST", line, 1);
		slprintf(line,sizeof(line)-1,"%s/smbwrapper.so:DEFAULT", libd);
		setenv("_RLDN32_LIST", line, 1);
	} else {
		slprintf(line,sizeof(line)-1,"%s/smbwrapper.so:DEFAULT", libd);
		setenv("_RLD_LIST", line, 1);
	}

	{
    	char *shellpath = getenv("SHELL");
		if(shellpath)
			execl(shellpath,"smbsh",NULL);
		else
			execl("/bin/sh","smbsh",NULL);
	}
	printf("launch failed!\n");
	return 1;
}	
