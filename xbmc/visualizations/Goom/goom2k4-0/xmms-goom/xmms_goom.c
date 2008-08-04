#include <glib.h>
#include "goom_config.h"

#include <xmms/plugin.h>
#include <xmms/xmmsctrl.h>

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <signal.h>
#include <string.h>

static void plug_init (void);
static void plug_cleanup (void);
static void plug_render_pcm (gint16 data[2][512]);

static int fd_in, fd_out;
static pid_t goom_pid = -1;

static VisPlugin plug_vp = {
    NULL,
    NULL,
    0,
    "What A GOOM!! " VERSION,
    2,
    0,
    plug_init,/* init */
    plug_cleanup,/* cleanup */
    NULL,/* about */
    NULL,/* configure */
    NULL,/* disable_plugin */
    NULL,/* playback_start */
    NULL,/* playback_stop */
    plug_render_pcm, /* render_pcm */
    NULL /* render_freq */
};

VisPlugin *
get_vplugin_info (void)
{
    return &plug_vp;
}

static void
plug_init (void)
{
    int fd[2];
    pid_t pid;

    /* create a pipe */
    if (pipe(fd) < 0) {
        fprintf (stderr, "System Error\n");
        /* TODO: en gtk? */
        return;
    }
    fd_in  = fd[0];
    fd_out = fd[1];

    /* load an executable */
    pid = fork();

    /* todo look at the result */
    if (pid == 0) {
        dup2(fd_in, 0);

        execlp  ("goom2", "goom2", NULL, 0);
        fprintf (stderr, "Unable to load goom...\n"); /* TODO: Message en gtk
                                                         check the PATH */
        exit (1);
    }
    if (pid == -1) {
        /* erreur system : TODO -> dialog en gtk */
    }
    if (goom_pid != -1)
        kill (goom_pid, SIGQUIT);
    goom_pid = pid;
}

static void sendIntToGoom(int i) {
    write (fd_out, &i, sizeof(int));
}

static void
plug_cleanup (void)
{
    sendIntToGoom(2);
    kill (goom_pid, SIGQUIT);
    goom_pid = -1;
}

static void
plug_render_pcm (gint16 data[2][512])
{
    fd_set rfds;
    struct timeval tv;
    int retval;

    tv.tv_sec = 0;
    tv.tv_usec = 10000;

    FD_ZERO(&rfds);
    FD_SET(fd_out, &rfds);
    retval = select(fd_out+1, NULL, &rfds, NULL, &tv);
    if (retval) {
        /* send sound datas to goom */
        {
            sendIntToGoom(0);
            write (fd_out, &data[0][0], 512*sizeof(gint16)*2);
            fsync(fd_out);
        }

        /* send song title to goom */
        {
            static int spos = -1;
            int pos = xmms_remote_get_playlist_pos(plug_vp.xmms_session);
            char title[2048];
            if (spos != pos) {
                sendIntToGoom(1);
                strcpy(title, xmms_remote_get_playlist_title(plug_vp.xmms_session, pos));
                write (fd_out, &title[0], 2048);
                spos = pos;
            }
        }
    }
    else {
        usleep(100);
    }
}
