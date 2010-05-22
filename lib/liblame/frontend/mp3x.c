/* $Id: mp3x.c,v 1.25 2008/03/09 17:13:23 robert Exp $ */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdio.h>

#include "lame.h"
#include "machine.h"
#include "encoder.h"
#include "lame-analysis.h"
#include <gtk/gtk.h>
#include "parse.h"
#include "get_audio.h"
#include "gtkanal.h"
#include "lametime.h"

#include "main.h"
#include "console.h"





/************************************************************************
*
* main
*
* PURPOSE:  MPEG-1,2 Layer III encoder with GPSYCHO
* psychoacoustic model.
*
************************************************************************/
int
main(int argc, char **argv)
{
    char    mp3buffer[LAME_MAXMP3BUFFER];
    lame_global_flags *gf;
    char    outPath[PATH_MAX + 1];
    char    inPath[PATH_MAX + 1];
    int     ret;
    int     enc_delay = -1;
    int     enc_padding = -1;

    frontend_open_console();
    gf = lame_init();
    if (NULL == gf) {
        error_printf("fatal error during initialization\n");
        frontend_close_console();
        return 1;
    }
    lame_set_errorf(gf, &frontend_errorf);
    lame_set_debugf(gf, &frontend_debugf);
    lame_set_msgf(gf, &frontend_msgf);
    if (argc <= 1) {
        usage(stderr, argv[0]); /* no command-line args  */
        frontend_close_console();
        return -1;
    }
    ret = parse_args(gf, argc, argv, inPath, outPath, NULL, NULL);
    if (ret < 0) {
        frontend_close_console();
        return ret == -2 ? 0 : 1;
    }
    (void) lame_set_analysis(gf, 1);

    init_infile(gf, inPath, &enc_delay, &enc_padding);
    lame_init_params(gf);
    lame_print_config(gf);


    gtk_init(&argc, &argv);
    gtkcontrol(gf, inPath);

    lame_encode_flush(gf, mp3buffer, sizeof(mp3buffer));
    lame_close(gf);
    close_infile();
    frontend_close_console();
    return 0;
}
