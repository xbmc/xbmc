/* 
   Unix SMB/CIFS implementation.
   randomised byte range lock tester
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

static fstring password[2];
static fstring username[2];
static int got_user;
static int got_pass;
static BOOL use_kerberos;
static int numops = 1000;
static BOOL showall;
static BOOL analyze;
static BOOL hide_unlock_fails;
static BOOL use_oplocks;
static unsigned lock_range = 100;
static unsigned lock_base = 0;
static unsigned min_length = 0;
static BOOL exact_error_codes;
static BOOL zero_zero;

#define FILENAME "\\locktest.dat"

#define READ_PCT 50
#define LOCK_PCT 45
#define UNLOCK_PCT 70
#define RANGE_MULTIPLE 1
#define NSERVERS 2
#define NCONNECTIONS 2
#define NFILES 2
#define LOCK_TIMEOUT 0

#define NASTY_POSIX_LOCK_HACK 0

enum lock_op {OP_LOCK, OP_UNLOCK, OP_REOPEN};

const char *lock_op_type(int op)
{
	if (op == WRITE_LOCK) return "write";
	else if (op == READ_LOCK) return "read";
	else return "other";
}

const char *lock_op_name(enum lock_op op)
{
	if (op == OP_LOCK) return "lock";
	else if (op == OP_UNLOCK) return "unlock";
	else return "reopen";
}

struct record {
	enum lock_op lock_op;
	enum brl_type lock_type;
	char conn, f;
	SMB_BIG_UINT start, len;
	char needed;
};

#define PRESETS 0

#if PRESETS
static struct record preset[] = {
{OP_LOCK, WRITE_LOCK, 0, 0, 2, 0, 1},
{OP_LOCK, WRITE_LOCK, 0, 0, 0, 0, 1},
{OP_LOCK, WRITE_LOCK, 0, 0, 3, 0, 1},
{OP_UNLOCK, 0       , 0, 0, 2, 0, 1},
{OP_REOPEN, 0, 0, 0, 0, 0, 1},

{OP_LOCK, READ_LOCK, 0, 0, 2, 0, 1},
{OP_LOCK, READ_LOCK, 0, 0, 1, 1, 1},
{OP_LOCK, WRITE_LOCK, 0, 0, 0, 0, 1},
{OP_REOPEN, 0, 0, 0, 0, 0, 1},

{OP_LOCK, READ_LOCK, 0, 0, 2, 0, 1},
{OP_LOCK, WRITE_LOCK, 0, 0, 3, 1, 1},
{OP_LOCK, WRITE_LOCK, 0, 0, 0, 0, 1},
{OP_REOPEN, 0, 0, 0, 0, 0, 1},

{OP_LOCK, READ_LOCK, 0, 0, 2, 0, 1},
{OP_LOCK, WRITE_LOCK, 0, 0, 1, 1, 1},
{OP_LOCK, WRITE_LOCK, 0, 0, 0, 0, 1},
{OP_REOPEN, 0, 0, 0, 0, 0, 1},

{OP_LOCK, WRITE_LOCK, 0, 0, 2, 0, 1},
{OP_LOCK, READ_LOCK, 0, 0, 1, 1, 1},
{OP_LOCK, WRITE_LOCK, 0, 0, 0, 0, 1},
{OP_REOPEN, 0, 0, 0, 0, 0, 1},

{OP_LOCK, WRITE_LOCK, 0, 0, 2, 0, 1},
{OP_LOCK, READ_LOCK, 0, 0, 3, 1, 1},
{OP_LOCK, WRITE_LOCK, 0, 0, 0, 0, 1},
{OP_REOPEN, 0, 0, 0, 0, 0, 1},

};
#endif

static struct record *recorded;

static void print_brl(SMB_DEV_T dev, SMB_INO_T ino, struct process_id pid, 
		      enum brl_type lock_type,
		      br_off start, br_off size)
{
#if NASTY_POSIX_LOCK_HACK
	{
		pstring cmd;
		static SMB_INO_T lastino;

		if (lastino != ino) {
			slprintf(cmd, sizeof(cmd), 
				 "egrep POSIX.*%u /proc/locks", (int)ino);
			system(cmd);
		}
		lastino = ino;
	}
#endif

	printf("%s   %05x:%05x    %s  %.0f:%.0f(%.0f)\n", 
	       procid_str_static(&pid), (int)dev, (int)ino, 
	       lock_type==READ_LOCK?"R":"W",
	       (double)start, (double)start+size-1,(double)size);

}


static void show_locks(void)
{
	brl_forall(print_brl);
	/* system("cat /proc/locks"); */
}


/***************************************************** 
return a connection to a server
*******************************************************/
static struct cli_state *connect_one(char *share, int snum)
{
	struct cli_state *c;
	struct nmb_name called, calling;
	char *server_n;
	fstring server;
	struct in_addr ip;
	fstring myname;
	static int count;

	fstrcpy(server,share+2);
	share = strchr_m(server,'\\');
	if (!share) return NULL;
	*share = 0;
	share++;

	server_n = server;
	
        zero_ip(&ip);

	slprintf(myname,sizeof(myname), "lock-%lu-%u", (unsigned long)getpid(), count++);

	make_nmb_name(&calling, myname, 0x0);
	make_nmb_name(&called , server, 0x20);

 again:
        zero_ip(&ip);

	/* have to open a new connection */
	if (!(c=cli_initialise(NULL)) || !cli_connect(c, server_n, &ip)) {
		DEBUG(0,("Connection to %s failed\n", server_n));
		return NULL;
	}

	c->use_kerberos = use_kerberos;

	if (!cli_session_request(c, &calling, &called)) {
		DEBUG(0,("session request to %s failed\n", called.name));
		cli_shutdown(c);
		if (strcmp(called.name, "*SMBSERVER")) {
			make_nmb_name(&called , "*SMBSERVER", 0x20);
			goto again;
		}
		return NULL;
	}

	DEBUG(4,(" session request ok\n"));

	if (!cli_negprot(c)) {
		DEBUG(0,("protocol negotiation failed\n"));
		cli_shutdown(c);
		return NULL;
	}

	if (!got_pass) {
		char *pass = getpass("Password: ");
		if (pass) {
			fstrcpy(password[0], pass);
			fstrcpy(password[1], pass);
		}
	}

	if (got_pass == 1) {
		fstrcpy(password[1], password[0]);
		fstrcpy(username[1], username[0]);
	}

	if (!cli_session_setup(c, username[snum], 
			       password[snum], strlen(password[snum]),
			       password[snum], strlen(password[snum]),
			       lp_workgroup())) {
		DEBUG(0,("session setup failed: %s\n", cli_errstr(c)));
		return NULL;
	}

	/*
	 * These next two lines are needed to emulate
	 * old client behaviour for people who have
	 * scripts based on client output.
	 * QUESTION ? Do we want to have a 'client compatibility
	 * mode to turn these on/off ? JRA.
	 */

	if (*c->server_domain || *c->server_os || *c->server_type)
		DEBUG(1,("Domain=[%s] OS=[%s] Server=[%s]\n",
			c->server_domain,c->server_os,c->server_type));
	
	DEBUG(4,(" session setup ok\n"));

	if (!cli_send_tconX(c, share, "?????",
			    password[snum], strlen(password[snum])+1)) {
		DEBUG(0,("tree connect failed: %s\n", cli_errstr(c)));
		cli_shutdown(c);
		return NULL;
	}

	DEBUG(4,(" tconx ok\n"));

	c->use_oplocks = use_oplocks;

	return c;
}


static void reconnect(struct cli_state *cli[NSERVERS][NCONNECTIONS], int fnum[NSERVERS][NCONNECTIONS][NFILES],
		      char *share[NSERVERS])
{
	int server, conn, f;

	for (server=0;server<NSERVERS;server++)
	for (conn=0;conn<NCONNECTIONS;conn++) {
		if (cli[server][conn]) {
			for (f=0;f<NFILES;f++) {
				if (fnum[server][conn][f] != -1) {
					cli_close(cli[server][conn], fnum[server][conn][f]);
					fnum[server][conn][f] = -1;
				}
			}
			cli_ulogoff(cli[server][conn]);
			cli_shutdown(cli[server][conn]);
		}
		cli[server][conn] = connect_one(share[server], server);
		if (!cli[server][conn]) {
			DEBUG(0,("Failed to connect to %s\n", share[server]));
			exit(1);
		}
	}
}



static BOOL test_one(struct cli_state *cli[NSERVERS][NCONNECTIONS], 
		     int fnum[NSERVERS][NCONNECTIONS][NFILES],
		     struct record *rec)
{
	unsigned conn = rec->conn;
	unsigned f = rec->f;
	SMB_BIG_UINT start = rec->start;
	SMB_BIG_UINT len = rec->len;
	enum brl_type op = rec->lock_type;
	int server;
	BOOL ret[NSERVERS];
	NTSTATUS status[NSERVERS];

	switch (rec->lock_op) {
	case OP_LOCK:
		/* set a lock */
		for (server=0;server<NSERVERS;server++) {
			ret[server] = cli_lock64(cli[server][conn], 
						 fnum[server][conn][f],
						 start, len, LOCK_TIMEOUT, op);
			status[server] = cli_nt_error(cli[server][conn]);
			if (!exact_error_codes && 
			    NT_STATUS_EQUAL(status[server], 
					    NT_STATUS_FILE_LOCK_CONFLICT)) {
				status[server] = NT_STATUS_LOCK_NOT_GRANTED;
			}
		}
		if (showall || !NT_STATUS_EQUAL(status[0],status[1])) {
			printf("lock   conn=%u f=%u range=%.0f(%.0f) op=%s -> %s:%s\n",
			       conn, f, 
			       (double)start, (double)len,
			       op==READ_LOCK?"READ_LOCK":"WRITE_LOCK",
			       nt_errstr(status[0]), nt_errstr(status[1]));
		}
		if (showall || !NT_STATUS_EQUAL(status[0],status[1])) show_locks();
		if (!NT_STATUS_EQUAL(status[0],status[1])) return False;
		break;
		
	case OP_UNLOCK:
		/* unset a lock */
		for (server=0;server<NSERVERS;server++) {
			ret[server] = cli_unlock64(cli[server][conn], 
						   fnum[server][conn][f],
						   start, len);
			status[server] = cli_nt_error(cli[server][conn]);
		}
		if (showall || 
		    (!hide_unlock_fails && !NT_STATUS_EQUAL(status[0],status[1]))) {
			printf("unlock conn=%u f=%u range=%.0f(%.0f)       -> %s:%s\n",
			       conn, f, 
			       (double)start, (double)len,
			       nt_errstr(status[0]), nt_errstr(status[1]));
		}
		if (showall || !NT_STATUS_EQUAL(status[0],status[1])) show_locks();
		if (!hide_unlock_fails && !NT_STATUS_EQUAL(status[0],status[1])) 
			return False;
		break;

	case OP_REOPEN:
		/* reopen the file */
		for (server=0;server<NSERVERS;server++) {
			cli_close(cli[server][conn], fnum[server][conn][f]);
			fnum[server][conn][f] = -1;
		}
		for (server=0;server<NSERVERS;server++) {
			fnum[server][conn][f] = cli_open(cli[server][conn], FILENAME,
							 O_RDWR|O_CREAT,
							 DENY_NONE);
			if (fnum[server][conn][f] == -1) {
				printf("failed to reopen on share%d\n", server);
				return False;
			}
		}
		if (showall) {
			printf("reopen conn=%u f=%u\n",
			       conn, f);
			show_locks();
		}
		break;
	}

	return True;
}

static void close_files(struct cli_state *cli[NSERVERS][NCONNECTIONS], 
			int fnum[NSERVERS][NCONNECTIONS][NFILES])
{
	int server, conn, f; 

	for (server=0;server<NSERVERS;server++)
	for (conn=0;conn<NCONNECTIONS;conn++)
	for (f=0;f<NFILES;f++) {
		if (fnum[server][conn][f] != -1) {
			cli_close(cli[server][conn], fnum[server][conn][f]);
			fnum[server][conn][f] = -1;
		}
	}
	for (server=0;server<NSERVERS;server++) {
		cli_unlink(cli[server][0], FILENAME);
	}
}

static void open_files(struct cli_state *cli[NSERVERS][NCONNECTIONS], 
		       int fnum[NSERVERS][NCONNECTIONS][NFILES])
{
	int server, conn, f; 

	for (server=0;server<NSERVERS;server++)
	for (conn=0;conn<NCONNECTIONS;conn++)
	for (f=0;f<NFILES;f++) {
		fnum[server][conn][f] = cli_open(cli[server][conn], FILENAME,
						 O_RDWR|O_CREAT,
						 DENY_NONE);
		if (fnum[server][conn][f] == -1) {
			fprintf(stderr,"Failed to open fnum[%u][%u][%u]\n",
				server, conn, f);
			exit(1);
		}
	}
}


static int retest(struct cli_state *cli[NSERVERS][NCONNECTIONS], 
		   int fnum[NSERVERS][NCONNECTIONS][NFILES],
		   int n)
{
	int i;
	printf("testing %u ...\n", n);
	for (i=0; i<n; i++) {
		if (i && i % 100 == 0) {
			printf("%u\n", i);
		}

		if (recorded[i].needed &&
		    !test_one(cli, fnum, &recorded[i])) return i;
	}
	return n;
}


/* each server has two connections open to it. Each connection has two file
   descriptors open on the file - 8 file descriptors in total 

   we then do random locking ops in tamdem on the 4 fnums from each
   server and ensure that the results match
 */
static void test_locks(char *share[NSERVERS])
{
	struct cli_state *cli[NSERVERS][NCONNECTIONS];
	int fnum[NSERVERS][NCONNECTIONS][NFILES];
	int n, i, n1, skip, r1, r2; 

	ZERO_STRUCT(fnum);
	ZERO_STRUCT(cli);

	recorded = SMB_MALLOC_ARRAY(struct record, numops);

	for (n=0; n<numops; n++) {
#if PRESETS
		if (n < sizeof(preset) / sizeof(preset[0])) {
			recorded[n] = preset[n];
		} else {
#endif
			recorded[n].conn = random() % NCONNECTIONS;
			recorded[n].f = random() % NFILES;
			recorded[n].start = lock_base + ((unsigned)random() % (lock_range-1));
			recorded[n].len =  min_length +
				random() % (lock_range-(recorded[n].start-lock_base));
			recorded[n].start *= RANGE_MULTIPLE;
			recorded[n].len *= RANGE_MULTIPLE;
			r1 = random() % 100;
			r2 = random() % 100;
			if (r1 < READ_PCT) {
				recorded[n].lock_type = READ_LOCK;
			} else {
				recorded[n].lock_type = WRITE_LOCK;
			}
			if (r2 < LOCK_PCT) {
				recorded[n].lock_op = OP_LOCK;
			} else if (r2 < UNLOCK_PCT) {
				recorded[n].lock_op = OP_UNLOCK;
			} else {
				recorded[n].lock_op = OP_REOPEN;
			}
			recorded[n].needed = True;
			if (!zero_zero && recorded[n].start==0 && recorded[n].len==0) {
				recorded[n].len = 1;
			}
#if PRESETS
		}
#endif
	}

	reconnect(cli, fnum, share);
	open_files(cli, fnum);
	n = retest(cli, fnum, numops);

	if (n == numops || !analyze) return;
	n++;

	skip = n/2;

	while (1) {
		n1 = n;

		close_files(cli, fnum);
		reconnect(cli, fnum, share);
		open_files(cli, fnum);

		for (i=0;i<n-skip;i+=skip) {
			int m, j;
			printf("excluding %d-%d\n", i, i+skip-1);
			for (j=i;j<i+skip;j++) {
				recorded[j].needed = False;
			}

			close_files(cli, fnum);
			open_files(cli, fnum);

			m = retest(cli, fnum, n);
			if (m == n) {
				for (j=i;j<i+skip;j++) {
					recorded[j].needed = True;
				}
			} else {
				if (i+(skip-1) < m) {
					memmove(&recorded[i], &recorded[i+skip],
						(m-(i+skip-1))*sizeof(recorded[0]));
				}
				n = m-(skip-1);
				i--;
			}
		}

		if (skip > 1) {
			skip = skip/2;
			printf("skip=%d\n", skip);
			continue;
		}

		if (n1 == n) break;
	}

	close_files(cli, fnum);
	reconnect(cli, fnum, share);
	open_files(cli, fnum);
	showall = True;
	n1 = retest(cli, fnum, n);
	if (n1 != n-1) {
		printf("ERROR - inconsistent result (%u %u)\n", n1, n);
	}
	close_files(cli, fnum);

	for (i=0;i<n;i++) {
		printf("{%s, %s, conn = %u, file = %u, start = %.0f, len = %.0f, %u},\n",
		       lock_op_name(recorded[i].lock_op),
		       lock_op_type(recorded[i].lock_type),
		       recorded[i].conn,
		       recorded[i].f,
		       (double)recorded[i].start,
		       (double)recorded[i].len,
		       recorded[i].needed);
	}	
}



static void usage(void)
{
	printf(
"Usage:\n\
  locktest //server1/share1 //server2/share2 [options..]\n\
  options:\n\
        -U user%%pass        (may be specified twice)\n\
        -k               use kerberos\n\
        -s seed\n\
        -o numops\n\
        -u          hide unlock fails\n\
        -a          (show all ops)\n\
        -A          analyse for minimal ops\n\
        -O          use oplocks\n\
        -E          enable exact error code checking\n\
        -Z          enable the zero/zero lock\n\
        -R range    set lock range\n\
        -B base     set lock base\n\
        -M min      set min lock length\n\
");
}

/****************************************************************************
  main program
****************************************************************************/
 int main(int argc,char *argv[])
{
	char *share[NSERVERS];
	extern char *optarg;
	extern int optind;
	int opt;
	char *p;
	int seed, server;

	setlinebuf(stdout);

	load_case_tables();

	dbf = x_stderr;

	if (argc < 3 || argv[1][0] == '-') {
		usage();
		exit(1);
	}

	setup_logging(argv[0],True);

	for (server=0;server<NSERVERS;server++) {
		share[server] = argv[1+server];
		all_string_sub(share[server],"/","\\",0);
	}

	argc -= NSERVERS;
	argv += NSERVERS;

	lp_load(dyn_CONFIGFILE,True,False,False,True);
	load_interfaces();

	if (getenv("USER")) {
		fstrcpy(username[0],getenv("USER"));
		fstrcpy(username[1],getenv("USER"));
	}

	seed = time(NULL);

	while ((opt = getopt(argc, argv, "U:s:ho:aAW:OkR:B:M:EZ")) != EOF) {
		switch (opt) {
		case 'k':
#ifdef HAVE_KRB5
			use_kerberos = True;
#else
			d_printf("No kerberos support compiled in\n");
			exit(1);
#endif
			break;
		case 'U':
			got_user = 1;
			if (got_pass == 2) {
				d_printf("Max of 2 usernames\n");
				exit(1);
			}
			fstrcpy(username[got_pass],optarg);
			p = strchr_m(username[got_pass],'%');
			if (p) {
				*p = 0;
				fstrcpy(password[got_pass], p+1);
				got_pass++;
			}
			break;
		case 'R':
			lock_range = strtol(optarg, NULL, 0);
			break;
		case 'B':
			lock_base = strtol(optarg, NULL, 0);
			break;
		case 'M':
			min_length = strtol(optarg, NULL, 0);
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
		case 'Z':
			zero_zero = True;
			break;
		case 'E':
			exact_error_codes = True;
			break;
		case 'h':
			usage();
			exit(1);
		default:
			printf("Unknown option %c (%d)\n", (char)opt, opt);
			exit(1);
		}
	}

	if(use_kerberos && !got_user) got_pass = True;

	argc -= optind;
	argv += optind;

	DEBUG(0,("seed=%u\n", seed));
	srandom(seed);

	test_locks(share);

	return(0);
}
