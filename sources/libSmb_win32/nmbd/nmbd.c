/*
   Unix SMB/CIFS implementation.
   NBT netbios routines and daemon - version 2
   Copyright (C) Andrew Tridgell 1994-1998
   Copyright (C) Jeremy Allison 1997-2002
   Copyright (C) Jelmer Vernooij 2002,2003 (Conversion to popt)
   
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

int ClientNMB       = -1;
int ClientDGRAM     = -1;
int global_nmb_port = -1;

extern BOOL rescan_listen_set;
extern struct in_addr loopback_ip;
extern BOOL global_in_nmbd;

extern BOOL override_logfile;

/* are we running as a daemon ? */
static BOOL is_daemon;

/* fork or run in foreground ? */
static BOOL Fork = True;

/* log to standard output ? */
static BOOL log_stdout;

/* have we found LanMan clients yet? */
BOOL found_lm_clients = False;

/* what server type are we currently */

time_t StartupTime = 0;

/**************************************************************************** **
 Handle a SIGTERM in band.
 **************************************************************************** */

static void terminate(void)
{
	DEBUG(0,("Got SIGTERM: going down...\n"));
  
	/* Write out wins.dat file if samba is a WINS server */
	wins_write_database(0,False);
  
	/* Remove all SELF registered names from WINS */
	release_wins_names();
  
	/* Announce all server entries as 0 time-to-live, 0 type. */
	announce_my_servers_removed();

	/* If there was an async dns child - kill it. */
	kill_async_dns_child();

	exit(0);
}

/**************************************************************************** **
 Handle a SHUTDOWN message from smbcontrol.
 **************************************************************************** */

static void nmbd_terminate(int msg_type, struct process_id src,
			   void *buf, size_t len)
{
	terminate();
}

/**************************************************************************** **
 Catch a SIGTERM signal.
 **************************************************************************** */

static SIG_ATOMIC_T got_sig_term;

static void sig_term(int sig)
{
	got_sig_term = 1;
	sys_select_signal(SIGTERM);
}

/**************************************************************************** **
 Catch a SIGHUP signal.
 **************************************************************************** */

static SIG_ATOMIC_T reload_after_sighup;

static void sig_hup(int sig)
{
	reload_after_sighup = 1;
	sys_select_signal(SIGHUP);
}

/**************************************************************************** **
 Possibly continue after a fault.
 **************************************************************************** */

static void fault_continue(void)
{
#if DUMP_CORE
	dump_core();
#endif
}

/**************************************************************************** **
 Expire old names from the namelist and server list.
 **************************************************************************** */

static void expire_names_and_servers(time_t t)
{
	static time_t lastrun = 0;
  
	if ( !lastrun )
		lastrun = t;
	if ( t < (lastrun + 5) )
		return;
	lastrun = t;

	/*
	 * Expire any timed out names on all the broadcast
	 * subnets and those registered with the WINS server.
	 * (nmbd_namelistdb.c)
	 */

	expire_names(t);

	/*
	 * Go through all the broadcast subnets and for each
	 * workgroup known on that subnet remove any expired
	 * server names. If a workgroup has an empty serverlist
	 * and has itself timed out then remove the workgroup.
	 * (nmbd_workgroupdb.c)
	 */

	expire_workgroups_and_servers(t);
}

/************************************************************************** **
 Reload the list of network interfaces.
 ************************************************************************** */

static BOOL reload_interfaces(time_t t)
{
	static time_t lastt;
	int n;
	struct subnet_record *subrec;

	if (t && ((t - lastt) < NMBD_INTERFACES_RELOAD)) return False;
	lastt = t;

	if (!interfaces_changed()) return False;

	/* the list of probed interfaces has changed, we may need to add/remove
	   some subnets */
	load_interfaces();

	/* find any interfaces that need adding */
	for (n=iface_count() - 1; n >= 0; n--) {
		struct interface *iface = get_interface(n);

		if (!iface) {
			DEBUG(2,("reload_interfaces: failed to get interface %d\n", n));
			continue;
		}

		/*
		 * We don't want to add a loopback interface, in case
		 * someone has added 127.0.0.1 for smbd, nmbd needs to
		 * ignore it here. JRA.
		 */

		if (ip_equal(iface->ip, loopback_ip)) {
			DEBUG(2,("reload_interfaces: Ignoring loopback interface %s\n", inet_ntoa(iface->ip)));
			continue;
		}

		for (subrec=subnetlist; subrec; subrec=subrec->next) {
			if (ip_equal(iface->ip, subrec->myip) &&
			    ip_equal(iface->nmask, subrec->mask_ip)) break;
		}

		if (!subrec) {
			/* it wasn't found! add it */
			DEBUG(2,("Found new interface %s\n", 
				 inet_ntoa(iface->ip)));
			subrec = make_normal_subnet(iface);
			if (subrec)
				register_my_workgroup_one_subnet(subrec);
		}
	}

	/* find any interfaces that need deleting */
	for (subrec=subnetlist; subrec; subrec=subrec->next) {
		for (n=iface_count() - 1; n >= 0; n--) {
			struct interface *iface = get_interface(n);
			if (ip_equal(iface->ip, subrec->myip) &&
			    ip_equal(iface->nmask, subrec->mask_ip)) break;
		}
		if (n == -1) {
			/* oops, an interface has disapeared. This is
			 tricky, we don't dare actually free the
			 interface as it could be being used, so
			 instead we just wear the memory leak and
			 remove it from the list of interfaces without
			 freeing it */
			DEBUG(2,("Deleting dead interface %s\n", 
				 inet_ntoa(subrec->myip)));
			close_subnet(subrec);
		}
	}
	
	rescan_listen_set = True;

	/* We need to shutdown if there are no subnets... */
	if (FIRST_SUBNET == NULL) {
		DEBUG(0,("reload_interfaces: No subnets to listen to. Shutting down...\n"));
		return True;
	}
	return False;
}

/**************************************************************************** **
 Reload the services file.
 **************************************************************************** */

static BOOL reload_nmbd_services(BOOL test)
{
	BOOL ret;

	set_remote_machine_name("nmbd", False);

	if ( lp_loaded() ) {
		pstring fname;
		pstrcpy( fname,lp_configfile());
		if (file_exist(fname,NULL) && !strcsequal(fname,dyn_CONFIGFILE)) {
			pstrcpy(dyn_CONFIGFILE,fname);
			test = False;
		}
	}

	if ( test && !lp_file_list_changed() )
		return(True);

	ret = lp_load( dyn_CONFIGFILE, True , False, False, True);

	/* perhaps the config filename is now set */
	if ( !test ) {
		DEBUG( 3, ( "services not loaded\n" ) );
		reload_nmbd_services( True );
	}

	return(ret);
}

/**************************************************************************** **
 * React on 'smbcontrol nmbd reload-config' in the same way as to SIGHUP
 * We use buf here to return BOOL result to process() when reload_interfaces()
 * detects that there are no subnets.
 **************************************************************************** */

static void msg_reload_nmbd_services(int msg_type, struct process_id src,
				     void *buf, size_t len)
{
	write_browse_list( 0, True );
	dump_all_namelists();
	reload_nmbd_services( True );
	reopen_logs();
	
	if(buf) {
		/* We were called from process() */
		/* If reload_interfaces() returned True */
		/* we need to shutdown if there are no subnets... */
		/* pass this info back to process() */
		*((BOOL*)buf) = reload_interfaces(0);  
	}
}

static void msg_nmbd_send_packet(int msg_type, struct process_id src,
				 void *buf, size_t len)
{
	struct packet_struct *p = (struct packet_struct *)buf;
	struct subnet_record *subrec;
	struct in_addr *local_ip;

	DEBUG(10, ("Received send_packet from %d\n", procid_to_pid(&src)));

	if (len != sizeof(struct packet_struct)) {
		DEBUG(2, ("Discarding invalid packet length from %d\n",
			  procid_to_pid(&src)));
		return;
	}

	if ((p->packet_type != NMB_PACKET) &&
	    (p->packet_type != DGRAM_PACKET)) {
		DEBUG(2, ("Discarding invalid packet type from %d: %d\n",
			  procid_to_pid(&src), p->packet_type));
		return;
	}

	local_ip = iface_ip(p->ip);

	if (local_ip == NULL) {
		DEBUG(2, ("Could not find ip for packet from %d\n",
			  procid_to_pid(&src)));
		return;
	}

	subrec = FIRST_SUBNET;

	p->fd = (p->packet_type == NMB_PACKET) ?
		subrec->nmb_sock : subrec->dgram_sock;

	for (subrec = FIRST_SUBNET; subrec != NULL;
	     subrec = NEXT_SUBNET_EXCLUDING_UNICAST(subrec)) {
		if (ip_equal(*local_ip, subrec->myip)) {
			p->fd = (p->packet_type == NMB_PACKET) ?
				subrec->nmb_sock : subrec->dgram_sock;
			break;
		}
	}

	if (p->packet_type == DGRAM_PACKET) {
		p->port = 138;
		p->packet.dgram.header.source_ip.s_addr = local_ip->s_addr;
		p->packet.dgram.header.source_port = 138;
	}

	send_packet(p);
}

/**************************************************************************** **
 The main select loop.
 **************************************************************************** */

static void process(void)
{
	BOOL run_election;
	BOOL no_subnets;

	while( True ) {
		time_t t = time(NULL);

		/* Check for internal messages */

		message_dispatch();

		/*
		 * Check all broadcast subnets to see if
		 * we need to run an election on any of them.
		 * (nmbd_elections.c)
		 */

		run_election = check_elections();

		/*
		 * Read incoming UDP packets.
		 * (nmbd_packets.c)
		 */

		if(listen_for_packets(run_election))
			return;

		/*
		 * Handle termination inband.
		 */

		if (got_sig_term) {
			got_sig_term = 0;
			terminate();
		}

		/*
		 * Process all incoming packets
		 * read above. This calls the success and
		 * failure functions registered when response
		 * packets arrrive, and also deals with request
		 * packets from other sources.
		 * (nmbd_packets.c)
		 */

		run_packet_queue();

		/*
		 * Run any elections - initiate becoming
		 * a local master browser if we have won.
		 * (nmbd_elections.c)
		 */

		run_elections(t);

		/*
		 * Send out any broadcast announcements
		 * of our server names. This also announces
		 * the workgroup name if we are a local
		 * master browser.
		 * (nmbd_sendannounce.c)
		 */

		announce_my_server_names(t);

		/*
		 * Send out any LanMan broadcast announcements
		 * of our server names.
		 * (nmbd_sendannounce.c)
		 */

		announce_my_lm_server_names(t);

		/*
		 * If we are a local master browser, periodically
		 * announce ourselves to the domain master browser.
		 * This also deals with syncronising the domain master
		 * browser server lists with ourselves as a local
		 * master browser.
		 * (nmbd_sendannounce.c)
		 */

		announce_myself_to_domain_master_browser(t);

		/*
		 * Fullfill any remote announce requests.
		 * (nmbd_sendannounce.c)
		 */

		announce_remote(t);

		/*
		 * Fullfill any remote browse sync announce requests.
		 * (nmbd_sendannounce.c)
		 */

		browse_sync_remote(t);

		/*
		 * Scan the broadcast subnets, and WINS client
		 * namelists and refresh any that need refreshing.
		 * (nmbd_mynames.c)
		 */

		refresh_my_names(t);

		/*
		 * Scan the subnet namelists and server lists and
		 * expire thos that have timed out.
		 * (nmbd.c)
		 */

		expire_names_and_servers(t);

		/*
		 * Write out a snapshot of our current browse list into
		 * the browse.dat file. This is used by smbd to service
		 * incoming NetServerEnum calls - used to synchronise
		 * browse lists over subnets.
		 * (nmbd_serverlistdb.c)
		 */

		write_browse_list(t, False);

		/*
		 * If we are a domain master browser, we have a list of
		 * local master browsers we should synchronise browse
		 * lists with (these are added by an incoming local
		 * master browser announcement packet). Expire any of
		 * these that are no longer current, and pull the server
		 * lists from each of these known local master browsers.
		 * (nmbd_browsesync.c)
		 */

		dmb_expire_and_sync_browser_lists(t);

		/*
		 * Check that there is a local master browser for our
		 * workgroup for all our broadcast subnets. If one
		 * is not found, start an election (which we ourselves
		 * may or may not participate in, depending on the
		 * setting of the 'local master' parameter.
		 * (nmbd_elections.c)
		 */

		check_master_browser_exists(t);

		/*
		 * If we are configured as a logon server, attempt to
		 * register the special NetBIOS names to become such
		 * (WORKGROUP<1c> name) on all broadcast subnets and
		 * with the WINS server (if used). If we are configured
		 * to become a domain master browser, attempt to register
		 * the special NetBIOS name (WORKGROUP<1b> name) to
		 * become such.
		 * (nmbd_become_dmb.c)
		 */

		add_domain_names(t);

		/*
		 * If we are a WINS server, do any timer dependent
		 * processing required.
		 * (nmbd_winsserver.c)
		 */

		initiate_wins_processing(t);

		/*
		 * If we are a domain master browser, attempt to contact the
		 * WINS server to get a list of all known WORKGROUPS/DOMAINS.
		 * This will only work to a Samba WINS server.
		 * (nmbd_browsesync.c)
		 */

		if (lp_enhanced_browsing())
			collect_all_workgroup_names_from_wins_server(t);

		/*
		 * Go through the response record queue and time out or re-transmit
		 * and expired entries.
		 * (nmbd_packets.c)
		 */

		retransmit_or_expire_response_records(t);

		/*
		 * check to see if any remote browse sync child processes have completed
		 */

		sync_check_completion();

		/*
		 * regularly sync with any other DMBs we know about 
		 */

		if (lp_enhanced_browsing())
			sync_all_dmbs(t);

		/*
		 * clear the unexpected packet queue 
		 */

		clear_unexpected(t);

		/*
		 * Reload the services file if we got a sighup.
		 */

		if(reload_after_sighup) {
			DEBUG( 0, ( "Got SIGHUP dumping debug info.\n" ) );
			msg_reload_nmbd_services(MSG_SMB_CONF_UPDATED,
						 pid_to_procid(0), (void*) &no_subnets, 0);
			if(no_subnets)
				return;
			reload_after_sighup = 0;
		}

		/* check for new network interfaces */

		if(reload_interfaces(t))
			return;

		/* free up temp memory */
			lp_TALLOC_FREE();
	}
}

/**************************************************************************** **
 Open the socket communication.
 **************************************************************************** */

static BOOL open_sockets(BOOL isdaemon, int port)
{
	/*
	 * The sockets opened here will be used to receive broadcast
	 * packets *only*. Interface specific sockets are opened in
	 * make_subnet() in namedbsubnet.c. Thus we bind to the
	 * address "0.0.0.0". The parameter 'socket address' is
	 * now deprecated.
	 */

	if ( isdaemon )
		ClientNMB = open_socket_in(SOCK_DGRAM, port,
					   0, interpret_addr(lp_socket_address()),
					   True);
	else
		ClientNMB = 0;
  
	ClientDGRAM = open_socket_in(SOCK_DGRAM, DGRAM_PORT,
					   3, interpret_addr(lp_socket_address()),
					   True);

	if ( ClientNMB == -1 )
		return( False );

	/* we are never interested in SIGPIPE */
	BlockSignals(True,SIGPIPE);

	set_socket_options( ClientNMB,   "SO_BROADCAST" );
	set_socket_options( ClientDGRAM, "SO_BROADCAST" );

	/* Ensure we're non-blocking. */
	set_blocking( ClientNMB, False);
	set_blocking( ClientDGRAM, False);

	DEBUG( 3, ( "open_sockets: Broadcast sockets opened.\n" ) );
	return( True );
}

/**************************************************************************** **
 main program
 **************************************************************************** */
 int main(int argc, const char *argv[])
{
	pstring logfile;
	static BOOL opt_interactive;
	poptContext pc;
	static char *p_lmhosts = dyn_LMHOSTSFILE;
	static BOOL no_process_group = False;
	struct poptOption long_options[] = {
	POPT_AUTOHELP
	{"daemon", 'D', POPT_ARG_VAL, &is_daemon, True, "Become a daemon(default)" },
	{"interactive", 'i', POPT_ARG_VAL, &opt_interactive, True, "Run interactive (not a daemon)" },
	{"foreground", 'F', POPT_ARG_VAL, &Fork, False, "Run daemon in foreground (for daemontools & etc)" },
	{"no-process-group", 0, POPT_ARG_VAL, &no_process_group, True, "Don't create a new process group" },
	{"log-stdout", 'S', POPT_ARG_VAL, &log_stdout, True, "Log to stdout" },
	{"hosts", 'H', POPT_ARG_STRING, &p_lmhosts, 'H', "Load a netbios hosts file"},
	{"port", 'p', POPT_ARG_INT, &global_nmb_port, NMB_PORT, "Listen on the specified port" },
	POPT_COMMON_SAMBA
	{ NULL }
	};

	load_case_tables();

	global_nmb_port = NMB_PORT;

	pc = poptGetContext("nmbd", argc, argv, long_options, 0);
	while (poptGetNextOpt(pc) != -1) {};
	poptFreeContext(pc);

	global_in_nmbd = True;
	
	StartupTime = time(NULL);
	
	sys_srandom(time(NULL) ^ sys_getpid());
	
	if (!override_logfile) {
		slprintf(logfile, sizeof(logfile)-1, "%s/log.nmbd", dyn_LOGFILEBASE);
		lp_set_logfile(logfile);
	}
	
	fault_setup((void (*)(void *))fault_continue );
	dump_core_setup("nmbd");
	
	/* POSIX demands that signals are inherited. If the invoking process has
	 * these signals masked, we will have problems, as we won't receive them. */
	BlockSignals(False, SIGHUP);
	BlockSignals(False, SIGUSR1);
	BlockSignals(False, SIGTERM);
	
	CatchSignal( SIGHUP,  SIGNAL_CAST sig_hup );
	CatchSignal( SIGTERM, SIGNAL_CAST sig_term );
	
#if defined(SIGFPE)
	/* we are never interested in SIGFPE */
	BlockSignals(True,SIGFPE);
#endif

	/* We no longer use USR2... */
#if defined(SIGUSR2)
	BlockSignals(True, SIGUSR2);
#endif

	if ( opt_interactive ) {
		Fork = False;
		log_stdout = True;
	}

	if ( log_stdout && Fork ) {
		DEBUG(0,("ERROR: Can't log to stdout (-S) unless daemon is in foreground (-F) or interactive (-i)\n"));
		exit(1);
	}

	setup_logging( argv[0], log_stdout );

	reopen_logs();

	DEBUG( 0, ( "Netbios nameserver version %s started.\n", SAMBA_VERSION_STRING) );
	DEBUGADD( 0, ( "%s\n", COPYRIGHT_STARTUP_MESSAGE ) );

	if ( !reload_nmbd_services(False) )
		return(-1);

	if(!init_names())
		return -1;

	reload_nmbd_services( True );

	if (strequal(lp_workgroup(),"*")) {
		DEBUG(0,("ERROR: a workgroup name of * is no longer supported\n"));
		exit(1);
	}

	set_samba_nb_type();

	if (!is_daemon && !is_a_socket(0)) {
		DEBUG(0,("standard input is not a socket, assuming -D option\n"));
		is_daemon = True;
	}
  
	if (is_daemon && !opt_interactive) {
		DEBUG( 2, ( "Becoming a daemon.\n" ) );
		become_daemon(Fork, no_process_group);
	}

#if HAVE_SETPGID
	/*
	 * If we're interactive we want to set our own process group for 
	 * signal management.
	 */
	if (opt_interactive && !no_process_group)
		setpgid( (pid_t)0, (pid_t)0 );
#endif

#ifndef SYNC_DNS
	/* Setup the async dns. We do it here so it doesn't have all the other
		stuff initialised and thus chewing memory and sockets */
	if(lp_we_are_a_wins_server() && lp_dns_proxy()) {
		start_async_dns();
	}
#endif

	if (!directory_exist(lp_lockdir(), NULL)) {
		mkdir(lp_lockdir(), 0755);
	}

	pidfile_create("nmbd");
	message_init();
	message_register(MSG_FORCE_ELECTION, nmbd_message_election);
#if 0
	/* Until winsrepl is done. */
	message_register(MSG_WINS_NEW_ENTRY, nmbd_wins_new_entry);
#endif
	message_register(MSG_SHUTDOWN, nmbd_terminate);
	message_register(MSG_SMB_CONF_UPDATED, msg_reload_nmbd_services);
	message_register(MSG_SEND_PACKET, msg_nmbd_send_packet);

	TimeInit();

	DEBUG( 3, ( "Opening sockets %d\n", global_nmb_port ) );

	if ( !open_sockets( is_daemon, global_nmb_port ) ) {
		kill_async_dns_child();
		return 1;
	}

	/* Determine all the IP addresses we have. */
	load_interfaces();

	/* Create an nmbd subnet record for each of the above. */
	if( False == create_subnets() ) {
		DEBUG(0,("ERROR: Failed when creating subnet lists. Exiting.\n"));
		kill_async_dns_child();
		exit(1);
	}

	/* Load in any static local names. */ 
	load_lmhosts_file(p_lmhosts);
	DEBUG(3,("Loaded hosts file %s\n", p_lmhosts));

	/* If we are acting as a WINS server, initialise data structures. */
	if( !initialise_wins() ) {
		DEBUG( 0, ( "nmbd: Failed when initialising WINS server.\n" ) );
		kill_async_dns_child();
		exit(1);
	}

	/* 
	 * Register nmbd primary workgroup and nmbd names on all
	 * the broadcast subnets, and on the WINS server (if specified).
	 * Also initiate the startup of our primary workgroup (start
	 * elections if we are setup as being able to be a local
	 * master browser.
	 */

	if( False == register_my_workgroup_and_names() ) {
		DEBUG(0,("ERROR: Failed when creating my my workgroup. Exiting.\n"));
		kill_async_dns_child();
		exit(1);
	}

	/* We can only take signals in the select. */
	BlockSignals( True, SIGTERM );

	process();

	if (dbf)
		x_fclose(dbf);
	kill_async_dns_child();
	return(0);
}
