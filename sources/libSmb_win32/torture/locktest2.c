/* 
   Unix SMB/CIFS implementation.
   byte range lock tester - with local filesystem support
   Copyright (C) Andrew Tridgell 1999
   
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

static fstring password;
static fstring username;
static int got_pass;
static int numops = 1000;
static BOOL showall;
static BOOL analyze;
static BOOL hide_unlock_fails;
static BOOL use_oplocks;

#define FILENAME "\\locktest.dat"
#define LOCKRANGE 100
#define LOCKBASE 0

/*
#define LOCKBASE (0x40000000 - 50)
*/

#define READ_PCT 50
#define LOCK_PCT 25
#define UNLOCK_PCT 65
#define RANGE_MULTIPLE 1

#define NSERVERS 2
#define NCONNECTIONS 2
#define NUMFSTYPES 2
#define NFILES 2
#define LOCK_TIMEOUT 0

#define FSTYPE_SMB 0
#define FSTYPE_NFS 1

struct record {
	char r1, r2;
	char conn, f, fstype;
	unsigned start, len;
	char needed;
};

static struct record *recorded;

static int try_open(struct cli_state *c, char *nfs, int fstype, const char *fname, int flags)
{
	pstring path;

	switch (fstype) {
	case FSTYPE_SMB:
		return cli_open(c, fname, flags, DENY_NONE);

	case FSTYPE_NFS:
		slprintf(path, sizeof(path), "%s%s", nfs, fname);
		pstring_sub(path,"\\", "/");
		return open(path, flags, 0666);
	}

	return -1;
}

static BOOL try_close(struct cli_state *c, int fstype, int fd)
{
	switch (fstype) {
	case FSTYPE_SMB:
		return cli_close(c, fd);

	case FSTYPE_NFS:
		return close(fd) == 0;
	}

	return False;
}

static BOOL try_lock(struct cli_state *c, int fstype, 
		     int fd, unsigned start, unsigned len,
		     enum brl_type op)
{
	struct flock lock;

	switch (fstype) {
	case FSTYPE_SMB:
		return cli_lock(c, fd, start, len, LOCK_TIMEOUT, op);

	case FSTYPE_NFS:
		lock.l_type = (op==READ_LOCK) ? F_RDLCK:F_WRLCK;
		lock.l_whence = SEEK_SET;
		lock.l_start = start;
		lock.l_len = len;
		lock.l_pid = getpid();
		return fcntl(fd,F_SETLK,&lock) == 0;
	}

	return False;
}

static BOOL try_unlock(struct cli_state *c, int fstype, 
		       int fd, unsigned start, unsigned len)
{
	struct flock lock;

	switch (fstype) {
	case FSTYPE_SMB:
		return cli_unlock(c, fd, start, len);

	case FSTYPE_NFS:
		lock.l_type = F_UNLCK;
		lock.l_whence = SEEK_SET;
		lock.l_start = start;
		lock.l_len = len;
		lock.l_pid = getpid();
		return fcntl(fd,F_SETLK,&lock) == 0;
	}

	return False;
}	

static void print_brl(SMB_DEV_T dev, SMB_INO_T ino, struct process_id pid, 
		      enum brl_type lock_type,
		      br_off start, br_off size)
{
	printf("%6d   %05x:%05x    %s  %.0f:%.0f(%.0f)\n", 
	       (int)procid_to_pid(&pid), (int)dev, (int)ino, 
	       lock_type==READ_LOCK?"R":"W",
	       (double)start, (double)start+size-1,(double)size);

}

/***************************************************** 
return a connection to a server
*******************************************************/
static struct cli_state *connect_one(char *share)
{
	struct cli_state *c;
	char *server_n;
	fstring server;
	fstring myname;
	static int count;
	NTSTATUS nt_status;

	fstrcpy(server,share+2);
	share = strchr_m(server,'\\');
	if (!share) return NULL;
	*share = 0;
	share++;

	server_n = server;
	
	if (!got_pass) {
		char *pass = getpass("Password: ");
		if (pass) {
			fstrcpy(password, pass);
		}
	}

	slprintf(myname,sizeof(myname), "lock-%lu-%u", (unsigned long)getpid(), count++);

	nt_status = cli_full_connection(&c, myname, server_n, NULL, 0, share, "?????", 
					username, lp_workgroup(), password, 0,
					Undefined, NULL);

	if (!NT_STATUS_IS_OK(nt_status)) {
		DEBUG(0, ("cli_full_connection failed with error %s\n", nt_errstr(nt_status)));
		return NULL;
	}

	c->use_oplocks = use_oplocks;

	return c;
}


static void reconnect(struct cli_state *cli[NSERVERS][NCONNECTIONS], 
		      char *nfs[NSERVERS], 
		      int fnum[NSERVERS][NUMFSTYPES][NCONNECTIONS][NFILES],
		      char *share1, char *share2)
{
	int server, conn, f, fstype;
	char *share[2];
	share[0] = share1;
	share[1] = share2;

	fstype = FSTYPE_SMB;

	for (server=0;server<NSERVERS;server++)
	for (conn=0;conn<NCONNECTIONS;conn++) {
		if (cli[server][conn]) {
			for (f=0;f<NFILES;f++) {
				cli_close(cli[server][conn], fnum[server][fstype][conn][f]);
			}
			cli_ulogoff(cli[server][conn]);
			cli_shutdown(cli[server][conn]);
		}
		cli[server][conn] = connect_one(share[server]);
		if (!cli[server][conn]) {
			DEBUG(0,("Failed to connect to %s\n", share[server]));
			exit(1);
		}
	}
}



static BOOL test_one(struct cli_state *cli[NSERVERS][NCONNECTIONS], 
		     char *nfs[NSERVERS],
		     int fnum[NSERVERS][NUMFSTYPES][NCONNECTIONS][NFILES],
		     struct record *rec)
{
	unsigned conn = rec->conn;
	unsigned f = rec->f;
	unsigned fstype = rec->fstype;
	unsigned start = rec->start;
	unsigned len = rec->len;
	unsigned r1 = rec->r1;
	unsigned r2 = rec->r2;
	enum brl_type op;
	int server;
	BOOL ret[NSERVERS];

	if (r1 < READ_PCT) {
		op = READ_LOCK; 
	} else {
		op = WRITE_LOCK; 
	}

	if (r2 < LOCK_PCT) {
		/* set a lock */
		for (server=0;server<NSERVERS;server++) {
			ret[server] = try_lock(cli[server][conn], fstype,
					       fnum[server][fstype][conn][f],
					       start, len, op);
		}
		if (showall || ret[0] != ret[1]) {
			printf("lock   conn=%u fstype=%u f=%u range=%u:%u(%u) op=%s -> %u:%u\n",
			       conn, fstype, f, 
			       start, start+len-1, len,
			       op==READ_LOCK?"READ_LOCK":"WRITE_LOCK",
			       ret[0], ret[1]);
		}
		if (showall) brl_forall(print_brl);
		if (ret[0] != ret[1]) return False;
	} else if (r2 < LOCK_PCT+UNLOCK_PCT) {
		/* unset a lock */
		for (server=0;server<NSERVERS;server++) {
			ret[server] = try_unlock(cli[server][conn], fstype,
						 fnum[server][fstype][conn][f],
						 start, len);
		}
		if (showall || (!hide_unlock_fails && (ret[0] != ret[1]))) {
			printf("unlock conn=%u fstype=%u f=%u range=%u:%u(%u)       -> %u:%u\n",
			       conn, fstype, f, 
			       start, start+len-1, len,
			       ret[0], ret[1]);
		}
		if (showall) brl_forall(print_brl);
		if (!hide_unlock_fails && ret[0] != ret[1]) return False;
	} else {
		/* reopen the file */
		for (server=0;server<NSERVERS;server++) {
			try_close(cli[server][conn], fstype, fnum[server][fstype][conn][f]);
			fnum[server][fstype][conn][f] = try_open(cli[server][conn], nfs[server], fstype, FILENAME,
								 O_RDWR|O_CREAT);
			if (fnum[server][fstype][conn][f] == -1) {
				printf("failed to reopen on share1\n");
				return False;
			}
		}
		if (showall) {
			printf("reopen conn=%u fstype=%u f=%u\n",
			       conn, fstype, f);
			brl_forall(print_brl);
		}
	}
	return True;
}

static void close_files(struct cli_state *cli[NSERVERS][NCONNECTIONS], 
			char *nfs[NSERVERS],
			int fnum[NSERVERS][NUMFSTYPES][NCONNECTIONS][NFILES])
{
	int server, conn, f, fstype; 

	for (server=0;server<NSERVERS;server++)
	for (fstype=0;fstype<NUMFSTYPES;fstype++)
	for (conn=0;conn<NCONNECTIONS;conn++)
	for (f=0;f<NFILES;f++) {
		if (fnum[server][fstype][conn][f] != -1) {
			try_close(cli[server][conn], fstype, fnum[server][fstype][conn][f]);
			fnum[server][fstype][conn][f] = -1;
		}
	}
	for (server=0;server<NSERVERS;server++) {
		cli_unlink(cli[server][0], FILENAME);
	}
}

static void open_files(struct cli_state *cli[NSERVERS][NCONNECTIONS], 
		       char *nfs[NSERVERS],
		       int fnum[NSERVERS][NUMFSTYPES][NCONNECTIONS][NFILES])
{
	int server, fstype, conn, f; 

	for (server=0;server<NSERVERS;server++)
	for (fstype=0;fstype<NUMFSTYPES;fstype++)
	for (conn=0;conn<NCONNECTIONS;conn++)
	for (f=0;f<NFILES;f++) {
		fnum[server][fstype][conn][f] = try_open(cli[server][conn], nfs[server], fstype, FILENAME,
							 O_RDWR|O_CREAT);
		if (fnum[server][fstype][conn][f] == -1) {
			fprintf(stderr,"Failed to open fnum[%u][%u][%u][%u]\n",
				server, fstype, conn, f);
			exit(1);
		}
	}
}


static int retest(struct cli_state *cli[NSERVERS][NCONNECTIONS], 
		  char *nfs[NSERVERS],
		  int fnum[NSERVERS][NUMFSTYPES][NCONNECTIONS][NFILES],
		  int n)
{
	int i;
	printf("testing %u ...\n", n);
	for (i=0; i<n; i++) {
		if (i && i % 100 == 0) {
			printf("%u\n", i);
		}

		if (recorded[i].needed &&
		    !test_one(cli, nfs, fnum, &recorded[i])) return i;
	}
	return n;
}


/* each server has two connections open to it. Each connection has two file
   descriptors open on the file - 8 file descriptors in total 

   we then do random locking ops in tamdem on the 4 fnums from each
   server and ensure that the results match
 */
static void test_locks(char *share1, char *share2, char *nfspath1, char *nfspath2)
{
	struct cli_state *cli[NSERVERS][NCONNECTIONS];
	char *nfs[NSERVERS];
	int fnum[NSERVERS][NUMFSTYPES][NCONNECTIONS][NFILES];
	int n, i, n1; 

	nfs[0] = nfspath1;
	nfs[1] = nfspath2;

	ZERO_STRUCT(fnum);
	ZERO_STRUCT(cli);

	recorded = SMB_MALLOC_ARRAY(struct record, numops);

	for (n=0; n<numops; n++) {
		recorded[n].conn = random() % NCONNECTIONS;
		recorded[n].fstype = random() % NUMFSTYPES;
		recorded[n].f = random() % NFILES;
		recorded[n].start = LOCKBASE + ((unsigned)random() % (LOCKRANGE-1));
		recorded[n].len = 1 + 
			random() % (LOCKRANGE-(recorded[n].start-LOCKBASE));
		recorded[n].start *= RANGE_MULTIPLE;
		recorded[n].len *= RANGE_MULTIPLE;
		recorded[n].r1 = random() % 100;
		recorded[n].r2 = random() % 100;
		recorded[n].needed = True;
	}

	reconnect(cli, nfs, fnum, share1, share2);
	open_files(cli, nfs, fnum);
	n = retest(cli, nfs, fnum, numops);

	if (n == numops || !analyze) return;
	n++;

	while (1) {
		n1 = n;

		close_files(cli, nfs, fnum);
		reconnect(cli, nfs, fnum, share1, share2);
		open_files(cli, nfs, fnum);

		for (i=0;i<n-1;i++) {
			int m;
			recorded[i].needed = False;

			close_files(cli, nfs, fnum);
			open_files(cli, nfs, fnum);

			m = retest(cli, nfs, fnum, n);
			if (m == n) {
				recorded[i].needed = True;
			} else {
				if (i < m) {
					memmove(&recorded[i], &recorded[i+1],
						(m-i)*sizeof(recorded[0]));
				}
				n = m;
				i--;
			}
		}

		if (n1 == n) break;
	}

	close_files(cli, nfs, fnum);
	reconnect(cli, nfs, fnum, share1, share2);
	open_files(cli, nfs, fnum);
	showall = True;
	n1 = retest(cli, nfs, fnum, n);
	if (n1 != n-1) {
		printf("ERROR - inconsistent result (%u %u)\n", n1, n);
	}
	close_files(cli, nfs, fnum);

	for (i=0;i<n;i++) {
		printf("{%u, %u, %u, %u, %u, %u, %u, %u},\n",
		       recorded[i].r1,
		       recorded[i].r2,
		       recorded[i].conn,
		       recorded[i].fstype,
		       recorded[i].f,
		       recorded[i].start,
		       recorded[i].len,
		       recorded[i].needed);
	}	
}



static void usage(void)
{
	printf(
"Usage:\n\
  locktest //server1/share1 //server2/share2 /path1 /path2 [options..]\n\
  options:\n\
        -U user%%pass\n\
        -s seed\n\
        -o numops\n\
        -u          hide unlock fails\n\
        -a          (show all ops)\n\
        -O          use oplocks\n\
");
}

/****************************************************************************
  main program
****************************************************************************/
 int main(int argc,char *argv[])
{
	char *share1, *share2, *nfspath1, *nfspath2;
	extern char *optarg;
	extern int optind;
	int opt;
	char *p;
	int seed;

	setlinebuf(stdout);

	dbf = x_stderr;

	if (argc < 5 || argv[1][0] == '-') {
		usage();
		exit(1);
	}

	share1 = argv[1];
	share2 = argv[2];
	nfspath1 = argv[3];
	nfspath2 = argv[4];

	all_string_sub(share1,"/","\\",0);
	all_string_sub(share2,"/","\\",0);

	setup_logging(argv[0],True);

	argc -= 4;
	argv += 4;

	lp_load(dyn_CONFIGFILE,True,False,False,True);
	load_interfaces();

	if (getenv("USER")) {
		fstrcpy(username,getenv("USER"));
	}

	seed = time(NULL);

	while ((opt = getopt(argc, argv, "U:s:ho:aAW:O")) != EOF) {
		switch (opt) {
		case 'U':
			fstrcpy(username,optarg);
			p = strchr_m(username,'%');
			if (p) {
				*p = 0;
				fstrcpy(password, p+1);
				got_pass = 1;
			}
			break;
		case 's':
			seed = atoi(optarg);
			break;
		case 'u':
			hide_unlock_fails = True;
			break;
		case 'o':
			numops = atoi(optarg);
			break;
		case 'O':
			use_oplocks = True;
			break;
		case 'a':
			showall = True;
			break;
		case 'A':
			analyze = True;
			break;
		case 'h':
			usage();
			exit(1);
		default:
			printf("Unknown option %c (%d)\n", (char)opt, opt);
			exit(1);
		}
	}

	argc -= optind;
	argv += optind;

	DEBUG(0,("seed=%u\n", seed));
	srandom(seed);

	locking_init(1);
	test_locks(share1, share2, nfspath1, nfspath2);

	return(0);
}
