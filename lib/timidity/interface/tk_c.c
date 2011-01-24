/*
    TiMidity++ -- MIDI to WAVE converter and player
    Copyright (C) 1999-2002 Masanao Izumo <mo@goice.co.jp>
    Copyright (C) 1995 Tuukka Toivonen <tt@cgs.fi>

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
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

*/

/*================================================================
 *
 * The Tcl/Tk interface for Timidity
 * written by Takashi Iwai (iwai@dragon.mm.t.u-tokyo.ac.jp)
 *
 * Most of the following codes are derived from both motif_ctl.c
 * and motif_pipe.c.  The communication between Tk program and
 * timidity is established by a pipe stream as in Motif interface.
 * On the contrast to motif, the stdin and stdout are assigned
 * as pipe i/o in Tk interface.
 *
 *================================================================*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdarg.h>
#ifndef NO_STRING_H
#include <string.h>
#else
#include <strings.h>
#endif
#include <sys/types.h>
#include <sys/ioctl.h>
#include <signal.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <tcl.h>
#include <tk.h>
#include <sys/wait.h>

#include "timidity.h"
#include "common.h"
#include "instrum.h"
#include "playmidi.h"
#include "readmidi.h"
#include "output.h"
#include "controls.h"
#include "miditrace.h"
#include "aq.h"

#ifndef TKPROGPATH
#define TKPROGPATH PKGLIBDIR "/tkmidity.tcl"
#endif /* TKPROGPATH */


static void ctl_refresh(void);
static void ctl_total_time(int tt);
static void ctl_master_volume(int mv);
static void ctl_file_name(char *name);
static void ctl_current_time(int secs, int v);
static void ctl_program(int ch, int val);
static void ctl_volume(int channel, int val);
static void ctl_expression(int channel, int val);
static void ctl_panning(int channel, int val);
static void ctl_sustain(int channel, int val);
static void ctl_pitch_bend(int channel, int val);
static void ctl_lyric(int lyricid);
static void ctl_reset(void);
static int ctl_open(int using_stdin, int using_stdout);
static void ctl_close(void);
static int ctl_read(int32 *valp);
static int cmsg(int type, int verbosity_level, char *fmt, ...);
static void ctl_pass_playing_list(int number_of_files, char *list_of_files[]);
static int ctl_blocking_read(int32 *valp);
static void ctl_note(int status, int ch, int note, int vel);
static void ctl_event(CtlEvent *e);

static void k_pipe_printf(char *fmt, ...);
static void k_pipe_puts(char *str);
static int k_pipe_gets(char *str, int maxlen);
static void k_pipe_open(void);
static void k_pipe_error(char *st);
static int k_pipe_read_ready(void);

static int AppInit(Tcl_Interp *interp);
static int ExitAll(ClientData clientData, Tcl_Interp *interp,
		       int argc, char *argv[]);
static int TraceCreate(ClientData clientData, Tcl_Interp *interp,
		       int argc, char *argv[]);
static int TraceUpdate(ClientData clientData, Tcl_Interp *interp,
		    int argc, char *argv[]);
static int TraceReset(ClientData clientData, Tcl_Interp *interp,
		    int argc, char *argv[]);
static void trace_volume(int ch, int val);
static void trace_panning(int ch, int val);
static void trace_prog_init(int ch);
static void trace_prog(int ch, int val);
static void trace_bank(int ch, int val);
static void trace_sustain(int ch, int val);

static void get_child(int sig);
static void shm_alloc(void);
static void shm_free(int sig);

static void start_panel(void);

#define MAX_TK_MIDI_CHANNELS	16

typedef struct {
	int reset_panel;
	int multi_part;

	int32 last_time, cur_time;

	char v_flags[MAX_TK_MIDI_CHANNELS];
	int16 cnote[MAX_TK_MIDI_CHANNELS];
	int16 cvel[MAX_TK_MIDI_CHANNELS];
	int16 ctotal[MAX_TK_MIDI_CHANNELS];

	char c_flags[MAX_TK_MIDI_CHANNELS];
	Channel channel[MAX_TK_MIDI_CHANNELS];
	int wait_reset;
} PanelInfo;


/**********************************************/

#define ctl tk_control_mode

ControlMode ctl=
{
    "Tcl/Tk interface", 'k',
    1,0,0,
    0,
    ctl_open,
    ctl_close,
    ctl_pass_playing_list,
    ctl_read,
    cmsg,
    ctl_event
};

#define FLAG_NOTE_OFF	1
#define FLAG_NOTE_ON	2

#define FLAG_BANK	1
#define FLAG_PROG	2
#define FLAG_PAN	4
#define FLAG_SUST	8

static VOLATILE PanelInfo *Panel;
static VOLATILE int child_killed = 0;

static int shmid;	/* shared memory id */
static int semid;	/* semaphore id */

static int pipeAppli[2],pipePanel[2]; /* Pipe for communication with Tcl/Tk process   */
static int fpip_in, fpip_out;	/* in and out depends in which process we are */
static int child_pid;	               /* Pid for child process */

/***********************************************************************/
/* semaphore utilities                                                 */
/***********************************************************************/
static void semaphore_P(int sid)
{
    struct sembuf sb;

    sb.sem_num = 0;
    sb.sem_op  = -1;
    sb.sem_flg = 0;
    if(semop(sid, &sb, 1) == -1)
	perror("semop");
}

static void semaphore_V(int sid)
{
    struct sembuf sb;

    sb.sem_num = 0;
    sb.sem_op  = 1;
    sb.sem_flg = 0;
    if(semop(sid, &sb, 1) == -1)
	perror("semop");
}

/***********************************************************************/
/* Put controls on the pipe                                            */
/***********************************************************************/

static int cmsg(int type, int verbosity_level, char *fmt, ...)
{
	char local[2048];
#define TOO_LONG	2000

	va_list ap;
	if ((type==CMSG_TEXT || type==CMSG_INFO || type==CMSG_WARNING) &&
	    ctl.verbosity<verbosity_level)
		return 0;

	va_start(ap, fmt);
	if (strlen(fmt) > TOO_LONG)
		fmt[TOO_LONG] = 0;
	if (!ctl.opened) {
		vfprintf(stderr, fmt, ap);
		fprintf(stderr, "\n");
	} else if (type == CMSG_ERROR) {
		int32 val;
		vsnprintf(local, sizeof(local), fmt, ap);
		k_pipe_printf("CERR %d", type);
		k_pipe_puts(local);
		while (ctl_blocking_read(&val) != RC_NEXT)
			;
	} else {
		vsnprintf(local, sizeof(local), fmt, ap);
		k_pipe_printf("CMSG %d", type);
		k_pipe_puts(local);
	}
	va_end(ap);
	return 0;
}

static void ctl_refresh(void)
{
}

static void ctl_total_time(int tt)
{
	int centisecs=tt/(play_mode->rate/100);
	k_pipe_printf("TIME %d", centisecs);
	ctl_current_time(0, 0);
}

static void ctl_master_volume(int mv)
{
	k_pipe_printf("MVOL %d", mv);
}

static void ctl_file_name(char *name)
{
	k_pipe_printf("FILE %s", name);
}

static void ctl_current_time(int secs, int v)
{
    Panel->cur_time = secs;
}

static void ctl_channel_note(int ch, int note, int vel)
{
	if (vel == 0) {
		if (note == Panel->cnote[ch])
			Panel->v_flags[ch] = FLAG_NOTE_OFF;
		Panel->cvel[ch] = 0;
	} else if (vel > Panel->cvel[ch]) {
		Panel->cvel[ch] = vel;
		Panel->cnote[ch] = note;
		Panel->ctotal[ch] = vel * Panel->channel[ch].volume *
			Panel->channel[ch].expression / (127*127);
		Panel->v_flags[ch] = FLAG_NOTE_ON;
	}
}

static void ctl_note(int status, int ch, int note, int vel)
{
	if (!ctl.trace_playing)
		return;

	if (ch < 0 || ch >= MAX_TK_MIDI_CHANNELS) return;

	if (status != VOICE_ON)
		vel = 0;
	semaphore_P(semid);
	ctl_channel_note(ch, note, vel);
	semaphore_V(semid);
}

static void ctl_program(int ch, int val)
{
        if(ch >= MAX_TK_MIDI_CHANNELS)
		return;
	if (!ctl.trace_playing)
		return;
	if (ch < 0 || ch >= MAX_TK_MIDI_CHANNELS) return;
	if(channel[ch].special_sample)
	    val = channel[ch].special_sample;
	else
	    val += progbase;

	semaphore_P(semid);
	Panel->channel[ch].program = val;
	Panel->c_flags[ch] |= FLAG_PROG;
	semaphore_V(semid);
}

static void ctl_volume(int ch, int val)
{
        if(ch >= MAX_TK_MIDI_CHANNELS)
		return;
	if (!ctl.trace_playing)
		return;
	semaphore_P(semid);
	Panel->channel[ch].volume = val;
	ctl_channel_note(ch, Panel->cnote[ch], Panel->cvel[ch]);
	semaphore_V(semid);
}

static void ctl_expression(int ch, int val)
{
        if(ch >= MAX_TK_MIDI_CHANNELS)
		return;
	if (!ctl.trace_playing)
		return;
	semaphore_P(semid);
	Panel->channel[ch].expression = val;
	ctl_channel_note(ch, Panel->cnote[ch], Panel->cvel[ch]);
	semaphore_V(semid);
}

static void ctl_panning(int ch, int val)
{
        if(ch >= MAX_TK_MIDI_CHANNELS)
		return;
	if (!ctl.trace_playing)
		return;
	semaphore_P(semid);
	Panel->channel[ch].panning = val;
	Panel->c_flags[ch] |= FLAG_PAN;
	semaphore_V(semid);
}

static void ctl_sustain(int ch, int val)
{
        if(ch >= MAX_TK_MIDI_CHANNELS)
		return;
	if (!ctl.trace_playing)
		return;
	semaphore_P(semid);
	Panel->channel[ch].sustain = val;
	Panel->c_flags[ch] |= FLAG_SUST;
	semaphore_V(semid);
}

/*ARGSUSED*/
static void ctl_pitch_bend(int channel, int val)
{
/*
        if(ch >= MAX_TK_MIDI_CHANNELS)
		return;
	if (!ctl.trace_playing)
		return;
	semaphore_P(semid);
	Panel->channel[ch].pitch_bend = val;
	Panel->c_flags[ch] |= FLAG_BENDT;
	semaphore_V(semid);
*/
}

static void ctl_lyric(int lyricid)
{
    char *lyric;

    lyric = event2string(lyricid);
    if(lyric != NULL)
    {
	if(lyric[0] == ME_KARAOKE_LYRIC)
	{
	    if(lyric[1] == '/' || lyric[1] == '\\')
	    {
		k_pipe_printf("CMSG %d", CMSG_TEXT);
		k_pipe_puts("");
		k_pipe_printf("LYRC %d", CMSG_TEXT);
		k_pipe_printf("%s", lyric + 2);
	    }
	    else if(lyric[1] == '@')
	    {
		if(lyric[2] == 'L')
		{
		    k_pipe_printf("CMSG %d", CMSG_TEXT);
		    k_pipe_printf("Language: %s", lyric + 3);
		}
		else if(lyric[2] == 'T')
		{
		    k_pipe_printf("CMSG %d", CMSG_TEXT);
		    k_pipe_printf("Title: %s", lyric + 3);
		}
		else
		{
		    k_pipe_printf("CMSG %d", CMSG_TEXT);
		    k_pipe_printf("%s", lyric + 1);
		}
	    }
	    else
	    {
		k_pipe_printf("LYRC %d", CMSG_TEXT);
		k_pipe_printf("%s", lyric + 1);
	    }
	}
	else
	{
	    k_pipe_printf("CMSG %d", CMSG_TEXT);
	    k_pipe_printf("%s", lyric + 1);
	}
    }
}

static void ctl_reset(void)
{
	int i;

	if (!ctl.trace_playing)
	{
	    k_pipe_printf("RSET %d", ctl.trace_playing);
	    return;
	}

	Panel->wait_reset = 1;
	k_pipe_printf("RSET %d", ctl.trace_playing);

	while(Panel->wait_reset)
	    VOLATILE_TOUCH(Panel->wait_reset);

	if (!ctl.trace_playing)
		return;
	for (i = 0; i < MAX_TK_MIDI_CHANNELS; i++) {
		if(ISDRUMCHANNEL(i))
		    ctl_program(i, channel[i].bank);
		else
		    ctl_program(i, channel[i].program);
		ctl_volume(i, channel[i].volume);
		ctl_expression(i, channel[i].expression);
		ctl_panning(i, channel[i].panning);
		ctl_sustain(i, channel[i].sustain);
		if(channel[i].pitchbend == 0x2000 &&
		   channel[i].mod.val > 0)
		    ctl_pitch_bend(i, -1);
		else
		    ctl_pitch_bend(i, channel[i].pitchbend);
		ctl_channel_note(i, Panel->cnote[i], 0);
	}
}

/***********************************************************************/
/* OPEN THE CONNECTION                                                */
/***********************************************************************/
/*ARGSUSED*/
static int ctl_open(int using_stdin, int using_stdout)
{
	shm_alloc();
	k_pipe_open();

	if (child_pid == 0)
		start_panel();

	signal(SIGCHLD, get_child);
	signal(SIGTERM, shm_free);
	signal(SIGINT, shm_free);
	signal(SIGHUP, shm_free);

	ctl.opened=1;
	return 0;
}

/* Tells the window to disapear */
static void ctl_close(void)
{
	if (ctl.opened) {
		kill(child_pid, SIGTERM);
		shm_free(100);
		ctl.opened=0;
	}
}


/*
 * Read information coming from the window in a BLOCKING way
 */

/* commands are: PREV, NEXT, QUIT, STOP, LOAD, JUMP, VOLM */

static int ctl_blocking_read(int32 *valp)
{
	char buf[8192], *tok, *arg;
	int new_volume;
	int new_centiseconds;
	char *args[64], **files;
	int i=0, nfiles;

	k_pipe_gets(buf, sizeof(buf)-1);
	tok = strtok(buf, " ");

	for(;;)/* Loop after pause sleeping to treat other buttons! */
	{
		switch (*tok) {
		case 'V':
			if ((arg = strtok(NULL, " ")) != NULL) {
				new_volume = atoi(arg);
				*valp= new_volume - amplification ;
				return RC_CHANGE_VOLUME;
			}
			return RC_NONE;

		case 'J':
			if ((arg = strtok(NULL, " ")) != NULL) {
				new_centiseconds = atoi(arg);
				*valp= new_centiseconds*(play_mode->rate / 100) ;
				return RC_JUMP;
			}
			return RC_NONE;

		case 'Q':
			return RC_QUIT;

		case 'L':
			return RC_LOAD_FILE;

		case 'N':
			return RC_NEXT;

		case 'P':
			/*return RC_REALLY_PREVIOUS;*/
			return RC_PREVIOUS;

		case 'R':
			return RC_RESTART;

		case 'F':
			*valp=play_mode->rate;
			return RC_FORWARD;

		case 'B':
			*valp=play_mode->rate;
			return RC_BACK;

		case 'X':
			k_pipe_gets(buf, sizeof(buf)-1);
			args[i++] = strtok(buf, " ");
			while ((args[i++] = strtok(NULL, " ")) != NULL);
			nfiles = --i;
			files  = expand_file_archives(args, &nfiles);
			k_pipe_printf("ALST %d", nfiles);
			for (i=0;i<nfiles;i++)
				k_pipe_puts(files[i]);
			if(files != args)
			    free(files);
			return RC_NONE;

		case 'S':
			return RC_TOGGLE_PAUSE;

		default:
			fprintf(stderr,"UNKNOWN RC_MESSAGE `%s'\n", tok);
			return RC_NONE;
		}
	}

}

/*
 * Read information coming from the window in a non blocking way
 */
static int ctl_read(int32 *valp)
{
	int num;

	/* We don't wan't to lock on reading  */
	num=k_pipe_read_ready();

	if (num==0)
		return RC_NONE;

	return(ctl_blocking_read(valp));
}

static void ctl_pass_playing_list(int number_of_files, char *list_of_files[])
{
	int i=0;
	char local[1000];
	int command;
	int32 val;

	/* Pass the list to the interface */
	k_pipe_printf("LIST %d", number_of_files);
	for (i=0;i<number_of_files;i++)
		k_pipe_puts(list_of_files[i]);

	/* Ask the interface for a filename to play -> begin to play automatically */
	/*k_pipe_puts("NEXT");*/
	command = ctl_blocking_read(&val);

	/* Main Loop */
	for (;;)
	{
		if (command==RC_LOAD_FILE)
		{
			/* Read a LoadFile command */
			k_pipe_gets(local, sizeof(local)-1);
			command=play_midi_file(local);
		}
		else
		{
			if (command==RC_QUIT) {
				/* if really QUIT */
				k_pipe_gets(local, sizeof(local)-1);
				if (*local == 'Z')
					return;
				/* only stop playing..*/
			}
			if (command==RC_CHANGE_VOLUME) /* init volume */
				amplification += val;

			switch(command)
			{
			case RC_ERROR:
				k_pipe_puts("ERRR");
				break;
			case RC_NEXT:
				k_pipe_puts("NEXT");
				break;
			case RC_REALLY_PREVIOUS:
				k_pipe_puts("PREV");
				break;
			case RC_TUNE_END:
				k_pipe_puts("TEND");
				break;
			case RC_RESTART:
				k_pipe_puts("RSTA");
				break;
			}

			command = ctl_blocking_read(&val);
		}
	}
}


/* open pipe and fork child process */
static void k_pipe_open(void)
{
	int res;

	res = pipe(pipeAppli);
	if (res!=0) k_pipe_error("PIPE_APPLI CREATION");

	res = pipe(pipePanel);
	if (res!=0) k_pipe_error("PIPE_PANEL CREATION");

	if ((child_pid = fork()) == 0) {
		/*child*/
		close(pipePanel[1]);
		close(pipeAppli[0]);

		/* redirect to stdin/out */
		dup2(pipePanel[0], fileno(stdin));
		close(pipePanel[0]);
		dup2(pipeAppli[1], fileno(stdout));
		close(pipeAppli[1]);
	} else {
		close(pipePanel[0]);
		close(pipeAppli[1]);

		fpip_in= pipeAppli[0];
		fpip_out= pipePanel[1];
	}
}


#if defined(sgi)
#include <sys/time.h>
#endif

#if defined(SOLARIS)
#include <sys/filio.h>
#endif

static int k_pipe_read_ready(void)
{
#if defined(sgi)
    fd_set fds;
    int cnt;
    struct timeval timeout;

    FD_ZERO(&fds);
    FD_SET(fpip_in, &fds);
    timeout.tv_sec = timeout.tv_usec = 0;

    if((cnt = select(fpip_in + 1, &fds, NULL, NULL, &timeout)) < 0)
    {
	perror("select");
	return -1;
    }

    return cnt > 0 && FD_ISSET(fpip_in, &fds) != 0;
#else
    int num;

    if(ioctl(fpip_in,FIONREAD,&num) < 0) /* see how many chars in buffer. */
    {
	perror("ioctl: FIONREAD");
	return -1;
    }
    return num;
#endif
}


/***********************************************************************/
/* PIPE COMUNICATION                                                   */
/***********************************************************************/

static void k_pipe_error(char *st)
{
    fprintf(stderr,"CONNECTION PROBLEM WITH TCL/TK PROCESS IN %s BECAUSE:%s\n",
	    st,
	    strerror(errno));
    exit(1);
}


static void k_pipe_printf(char *fmt, ...)
{
	char buf[256];
	va_list ap;
	va_start(ap, fmt);
	vsnprintf(buf, sizeof(buf), fmt, ap);
	k_pipe_puts(buf);
}

static int line_strlen(char *str)
{
    int len;

    len = 0;
    while(*str && *str != '\r' && *str != '\n') {
	str++;
	len++;
    }
    return len;
}

static void k_pipe_puts(char *str)
{
	int len;
	char lf = '\n';
	len = line_strlen(str);
	write(fpip_out, str, len);
	write(fpip_out, &lf, 1);
}


int k_pipe_gets(char *str, int maxlen)
{
/* blocking reading */
	char *p;
	int len;

	/* at least 5 letters (4+\n) command */
	len = 0;
	for (p = str; len < maxlen - 1; p++) {
		read(fpip_in, p, 1);
		if (*p == '\n')
			break;
		len++;
	}
	*p = 0;
	return len;
}


/*----------------------------------------------------------------
 * shared memory handling
 *----------------------------------------------------------------*/

/*ARGSUSED*/
static void get_child(int sig)
{
	child_killed = 1;
}

static void shm_alloc(void)
{
	shmid = shmget(IPC_PRIVATE, sizeof(PanelInfo),
		       IPC_CREAT|0600);
	if (shmid < 0) {
		fprintf(stderr, "can't allocate shared memory\n");
		exit(1);
	}

	semid = semget(IPC_PRIVATE, 1, IPC_CREAT|0600);
	if (semid < 0) {
	    perror("semget");
	    shmctl(shmid, IPC_RMID,NULL);
	    exit(1);
	}

	/* bin semaphore: only call once at first */
	semaphore_V(semid);

	Panel = (PanelInfo *)shmat(shmid, 0, 0);
	Panel->reset_panel = 0;
	Panel->multi_part = 0;
	Panel->wait_reset = 0;
}


static void shm_free(int sig)
{
	int status;
#if defined(HAVE_UNION_SEMUN)
	union semun dmy;
#else /* Solaris 2.x, BSDI, OSF/1, HPUX */
	void *dmy;
#endif

	kill(child_pid, SIGTERM);
	while(wait(&status) != child_pid)
	    ;
	memset(&dmy, 0, sizeof(dmy)); /* Shut compiler warning up :-) */
	semctl(semid, 0, IPC_RMID, dmy);
	shmctl(shmid, IPC_RMID, NULL);
	shmdt((char *)Panel);
	if (sig != 100)
		exit(0);
}

/*----------------------------------------------------------------
 * start Tk window panel
 *----------------------------------------------------------------*/

static void start_panel(void)
{
	char *argv[128];
	int argc;

	argc = 0;
	argv[argc++] = "-f";
	argv[argc++] = TKPROGPATH;

	if (ctl.trace_playing) {
		argv[argc++] = "-mode";
		argv[argc++] = "trace";
	}

	/* call Tk main routine */
	Tk_Main(argc, argv, AppInit);

	exit(0);
}


/*----------------------------------------------------------------
 * initialize Tcl application
 *----------------------------------------------------------------*/

static Tcl_Interp *my_interp;

static int AppInit(Tcl_Interp *interp)
{
	my_interp = interp;

	if (Tcl_Init(interp) == TCL_ERROR) {
		return TCL_ERROR;
	}
	if (Tk_Init(interp) == TCL_ERROR) {
		return TCL_ERROR;
	}

	Tcl_CreateCommand(interp, "TraceCreate", (Tcl_CmdProc*) TraceCreate,
			  (ClientData)NULL, (Tcl_CmdDeleteProc*)NULL);
	Tcl_CreateCommand(interp, "TraceUpdate", (Tcl_CmdProc*) TraceUpdate,
			  (ClientData)NULL, (Tcl_CmdDeleteProc*)NULL);
	Tcl_CreateCommand(interp, "TraceReset", (Tcl_CmdProc*) TraceReset,
			  (ClientData)NULL, (Tcl_CmdDeleteProc*)NULL);
	Tcl_CreateCommand(interp, "ExitAll", (Tcl_CmdProc*) ExitAll,
			  (ClientData)NULL, (Tcl_CmdDeleteProc*)NULL);
	Tcl_CreateCommand(interp, "TraceUpdate", (Tcl_CmdProc*) TraceUpdate,
			  (ClientData)NULL, (Tcl_CmdDeleteProc*)NULL);
	return TCL_OK;
}

/*ARGSUSED*/
static int ExitAll(ClientData clientData, Tcl_Interp *interp,
		   int argc, char *argv[])
{
	/* window is killed; kill the parent process, too */
	kill(getppid(), SIGTERM);
	for (;;)
		sleep(1000);
	return TCL_OK;
}

/* evaluate Tcl script */
static char *v_eval(char *fmt, ...)
{
	char buf[256];
	va_list ap;
	va_start(ap, fmt);
	vsnprintf(buf, sizeof(buf), fmt, ap);
	Tcl_Eval(my_interp, buf);
	va_end(ap);
	return my_interp->result;
}

static const char *v_get2(const char *v1, const char *v2)
{
	return Tcl_GetVar2(my_interp, v1, v2, 0);
}


/*----------------------------------------------------------------
 * update Tcl timer / trace window
 *----------------------------------------------------------------*/

#define FRAME_WIN	".body.trace"
#define CANVAS_WIN	FRAME_WIN ".c"

#define BAR_WID 20
#define BAR_HGT 130
#define WIN_WID (BAR_WID * 16)
#define WIN_HGT (BAR_HGT + 11 + 17)
#define BAR_HALF_HGT (WIN_HGT / 2 - 11 - 17)


/*ARGSUSED*/
static int TraceCreate(ClientData clientData, Tcl_Interp *interp,
		       int argc, char *argv[])
{
	int i;
	v_eval("frame %s -bg black", FRAME_WIN);
	v_eval("canvas %s -width %d -height %d -bd 0 -bg black "
	       "-highlightthickness 0",
	       CANVAS_WIN, WIN_WID, WIN_HGT);
	v_eval("pack %s -side top -fill x", CANVAS_WIN);
	for (i = 0; i < 32; i++) {
		char *color;
		v_eval("%s create text 0 0 -anchor n -fill white -text 00 "
		       "-tags prog%d", CANVAS_WIN, i);
		v_eval("%s create poly 0 0 0 0 0 0 -fill yellow -tags pos%d",
		       CANVAS_WIN, i);
		color = (ISDRUMCHANNEL(i) || i == 25) ? "red" : "green";
		v_eval("%s create rect 0 0 0 0 -fill %s -tags bar%d "
		       "-outline \"\"", CANVAS_WIN, color, i);
	}
	v_eval("set Stat(TimerId) -1");
	v_eval("TraceReset");
	return TCL_OK;
}

static void trace_bank(int ch, int val)
{
	v_eval("%s itemconfigure bar%d -fill %s",
	       CANVAS_WIN, ch,
	       (val == 128 ? "red" : "green"));
}

static void trace_prog(int ch, int val)
{
	v_eval("%s itemconfigure prog%d -text %02X",
	       CANVAS_WIN, ch, val);
}

static void trace_sustain(int ch, int val)
{
	v_eval("%s itemconfigure prog%d -fill %s",
	       CANVAS_WIN, ch,
	       (val == 127 ? "green" : "white"));
}

static void trace_prog_init(int ch)
{
	int item, yofs, bar, x, y;

	item = ch;
	yofs = 0;
	bar = Panel->multi_part ? BAR_HALF_HGT : BAR_HGT;
	if (ch >= 16) {
		ch -= 16;
		yofs = WIN_HGT / 2;
		if (!Panel->multi_part)
			yofs = -500;
	}
	x = ch * BAR_WID + BAR_WID/2;
	y = bar + 11 + yofs;
	v_eval("%s coords prog%d %d %d", CANVAS_WIN, item, x, y);
}

static void trace_volume(int ch, int val)
{
	int item, bar, yofs, x1, y1, x2, y2;
	item = ch;
	yofs = 0;
	bar = Panel->multi_part ? BAR_HALF_HGT : BAR_HGT;
	if (ch >= 16) {
		yofs = WIN_HGT / 2;
		ch -= 16;
		if (!Panel->multi_part)
			yofs = -500;
	}
	x1 = ch * BAR_WID;
	y1 = bar - 1 + yofs;
	x2 = x1 + BAR_WID - 1;
	y2 = y1 - bar * val / 127;
	v_eval("%s coords bar%d %d %d %d %d", CANVAS_WIN,
	       item, x1, y1, x2, y2);
}

static void trace_panning(int ch, int val)
{
	int item, bar, yofs;
	int x, ap, bp;

	if (val < 0) {
		v_eval("%s coords pos%d -1 0 -1 0 -1 0", CANVAS_WIN, ch);
		return;
	}

	item = ch;
	yofs = 0;
	bar = Panel->multi_part ? BAR_HALF_HGT : BAR_HGT;
	if (ch >= 16) {
		yofs = WIN_HGT / 2;
		ch -= 16;
		if (!Panel->multi_part)
			yofs = -500;
	}

	x = BAR_WID * ch;
	ap = BAR_WID * val / 127;
	bp = BAR_WID - ap - 1;
	v_eval("%s coords pos%d %d %d %d %d %d %d", CANVAS_WIN, item,
	       ap + x, bar + 5 + yofs,
	       bp + x, bar + 1 + yofs,
	       bp + x, bar + 9 + yofs);
}

/*ARGSUSED*/
static int TraceReset(ClientData clientData, Tcl_Interp *interp,
			   int argc, char *argv[])
{
	int i;

	semaphore_P(semid);
	for (i = 0; i < 32; i++) {
		trace_volume(i, 0);
		trace_panning(i, -1);
		trace_prog_init(i);
		trace_prog(i, 0);
		trace_sustain(i, 0);
		Panel->ctotal[i] = 0;
		Panel->cvel[i] = 0;
		Panel->v_flags[i] = 0;
		Panel->c_flags[i] = 0;
	}
	semaphore_V(semid);
	Panel->wait_reset = 0;
	return TCL_OK;
}



#define DELTA_VEL	32

static void update_notes(void)
{
	int i, imax;
	semaphore_P(semid);
	imax = Panel->multi_part ? 32 : 16;
	for (i = 0; i < imax; i++) {
		if (Panel->v_flags[i]) {
			if (Panel->v_flags[i] == FLAG_NOTE_OFF) {
				Panel->ctotal[i] -= DELTA_VEL;
				if (Panel->ctotal[i] <= 0) {
					Panel->ctotal[i] = 0;
					Panel->v_flags[i] = 0;
				}
			} else {
				Panel->v_flags[i] = 0;
			}
			trace_volume(i, Panel->ctotal[i]);
		}

		if (Panel->c_flags[i]) {
			if (Panel->c_flags[i] & FLAG_PAN)
				trace_panning(i, Panel->channel[i].panning);
			if (Panel->c_flags[i] & FLAG_BANK)
				trace_bank(i, Panel->channel[i].bank);
			if (Panel->c_flags[i] & FLAG_PROG)
				trace_prog(i, Panel->channel[i].program);
			if (Panel->c_flags[i] & FLAG_SUST)
				trace_sustain(i, Panel->channel[i].sustain);
			Panel->c_flags[i] = 0;
		}
	}
	semaphore_V(semid);
}

/*ARGSUSED*/
static int TraceUpdate(ClientData clientData, Tcl_Interp *interp,
		    int argc, char *argv[])
{
	const char *playing = v_get2("Stat", "Playing");
	if (playing && *playing != '0') {
		if (Panel->reset_panel) {
			v_eval("TraceReset");
			Panel->reset_panel = 0;
		}
		if (Panel->last_time != Panel->cur_time) {
			v_eval("SetTime %d", Panel->cur_time);
			Panel->last_time = Panel->cur_time;
		}
		if (ctl.trace_playing)
			update_notes();
	}
	v_eval("set Stat(TimerId) [after 50 TraceUpdate]");
	return TCL_OK;
}

static void ctl_event(CtlEvent *e)
{
    switch(e->type)
    {
      case CTLE_NOW_LOADING:
	ctl_file_name((char *)e->v1);
	break;
      case CTLE_LOADING_DONE:
	break;
      case CTLE_PLAY_START:
	ctl_total_time((int)e->v1);
	break;
      case CTLE_PLAY_END:
	break;
      case CTLE_TEMPO:
	break;
      case CTLE_METRONOME:
	break;
      case CTLE_CURRENT_TIME:
	ctl_current_time((int)e->v1, (int)e->v2);
	break;
      case CTLE_NOTE:
	ctl_note((int)e->v1, (int)e->v2, (int)e->v3, (int)e->v4);
	break;
      case CTLE_MASTER_VOLUME:
	ctl_master_volume((int)e->v1);
	break;
      case CTLE_PROGRAM:
	ctl_program((int)e->v1, (int)e->v2);
	break;
      case CTLE_VOLUME:
	ctl_volume((int)e->v1, (int)e->v2);
	break;
      case CTLE_EXPRESSION:
	ctl_expression((int)e->v1, (int)e->v2);
	break;
      case CTLE_PANNING:
	ctl_panning((int)e->v1, (int)e->v2);
	break;
      case CTLE_SUSTAIN:
	ctl_sustain((int)e->v1, (int)e->v2);
	break;
      case CTLE_PITCH_BEND:
	ctl_pitch_bend((int)e->v1, (int)e->v2);
	break;
      case CTLE_MOD_WHEEL:
	ctl_pitch_bend((int)e->v1, e->v2 ? -1 : 0x2000);
	break;
      case CTLE_CHORUS_EFFECT:
	break;
      case CTLE_REVERB_EFFECT:
	break;
      case CTLE_LYRIC:
	ctl_lyric((int)e->v1);
	break;
      case CTLE_REFRESH:
	ctl_refresh();
	break;
      case CTLE_RESET:
	ctl_reset();
	break;
    }
}

/*
 * interface_<id>_loader();
 */
ControlMode *interface_k_loader(void)
{
    return &ctl;
}
