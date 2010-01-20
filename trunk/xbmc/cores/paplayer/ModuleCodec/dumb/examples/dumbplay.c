/*  _______         ____    __         ___    ___
 * \    _  \       \    /  \  /       \   \  /   /       '   '  '
 *  |  | \  \       |  |    ||         |   \/   |         .      .
 *  |  |  |  |      |  |    ||         ||\  /|  |
 *  |  |  |  |      |  |    ||         || \/ |  |         '  '  '
 *  |  |  |  |      |  |    ||         ||    |  |         .      .
 *  |  |_/  /        \  \__//          ||    |  |
 * /_______/ynamic    \____/niversal  /__\  /____\usic   /|  .  . ibliotheque
 *                                                      /  \
 *                                                     / .  \
 * dumbplay.c - Not-so-simple program to play         / / \  \
 *              music. It used to be simpler!        | <  /   \_
 *                                                   |  \/ /\   /
 * By entheh.                                         \_  /  > /
 *                                                      | \ / /
 * If this example does not explain everything you      |  ' /
 * need to know, or you write your own code and it       \__/
 * doesn't work, please refer to docs/howto.txt for
 * step-by-step instructions or docs/dumb.txt for a detailed description of
 * each of DUMB's functions.
 */

#include <stdlib.h>
#include <allegro.h>

#ifndef ALLEGRO_DOS
#include <string.h>
#endif

/* Note that your own programs should use <aldumb.h> not "aldumb.h". <> tells
 * the compiler to look in the compiler's default header directory, which is
 * where DUMB should be installed before you use it (make install does this).
 * Use "" when it is your own header file. This example uses "" because DUMB
 * might not have been installed yet when the makefile builds it.
 */
#include "aldumb.h"



/* Hook function to be called when the close button is clicked. If the
 * variable is set, the program will exit. Not needed in DOS.
 */
#ifndef ALLEGRO_DOS
	static volatile int closed = 0;
	static void closehook(void) { closed = 1; }
#else
#	define closed 0
#endif


/* Platform-specific code. We use GDI in Windows since there's no point in
 * using an accelerated graphics driver. If we can, we define our own YIELD()
 * function that allows the system to sleep when it has nothing to do. This
 * may now be part of Allegro, but I haven't kept up with it. You can ignore
 * this stuff.
 */
#ifdef ALLEGRO_WINDOWS
#	define GFX_DUMB_MODE GFX_GDI
#	include <winalleg.h>
#	define YIELD() Sleep(1)
#else
#	define GFX_DUMB_MODE GFX_AUTODETECT_WINDOWED
#	ifdef ALLEGRO_UNIX
#		include <sys/time.h>
		static void YIELD(void)
		{
			struct timeval tv;
			tv.tv_sec = 0;
			tv.tv_usec = 1;
			select(0, NULL, NULL, NULL, &tv);
		}
#	else
#		define YIELD() yield_timeslice()
#	endif
#endif



/* Playback flow callback functions. In DOS we don't set a graphics mode, so
 * we can print a message to the console when a callback is invoked. On other
 * platforms, we add a message to the window the program has created.
 */
#ifdef ALLEGRO_DOS
	static int loop_callback(void *data)
	{
		(void)data;
		printf("Music has looped.\n");
		return 0;
	}

	static int xm_speed_zero_callback(void *data)
	{
		(void)data;
		printf("Music has stopped.\n");
		return 0;
	}
#else
	static int gfx_half_width;

	static int loop_callback(void *data)
	{
		(void)data;
		if (gfx_half_width) {
			acquire_screen();
			textout_centre(screen, font, "Music has looped.", gfx_half_width, 36, 10);
			release_screen();
		}
		return 0;
	}

	static int xm_speed_zero_callback(void *data)
	{
		(void)data;
		if (gfx_half_width) {
			text_mode(0); /* In case this is overwriting "Music has looped." */
			acquire_screen();
			textout_centre(screen, font, "Music has stopped.", gfx_half_width, 36, 10);
			release_screen();
		}
		return 0;
	}
#endif



static void usage(const char *exename)
{
	allegro_message(
#ifdef ALLEGRO_WINDOWS
		"Usage:\n"
		"  At the command line: %s file\n"
		"  In Windows Explorer: drag a file on to this program's icon.\n"
#else
		"Usage: %s file\n"
#endif
		"This will play the music file specified.\n"
		"File formats supported: IT XM S3M MOD.\n"
		"You can control playback quality by editing dumb.ini.\n", exename);

	exit(1);
}



int main(int argc, const char *const *argv) /* I'm const-crazy! */
{
	DUH *duh;          /* Encapsulates the music file. */
	AL_DUH_PLAYER *dp; /* Holds the current playback state. */

	/* Initialise Allegro */
	if (allegro_init())
		return EXIT_FAILURE;

	/* Check that we have one argument (plus the executable name). */
	if (argc != 2)
		usage(argv[0]);

	/* Tell Allegro where to find configuration data. This means you can
	 * put any settings for Allegro in dumb.ini. See Allegro's
	 * documentation for more information.
	 */
	set_config_file("dumb.ini");

	/* Initialise Allegro's keyboard input. */
	if (install_keyboard()) {
		allegro_message("Failed to initialise keyboard driver!\n");
		return EXIT_FAILURE;
	}

	/* This function call is appropriate for a program that will play one
	 * sample or one audio stream at a time. If you have sound effects
	 * too, you may want to increase the parameter. See Allegro's
	 * documentation for details on what the parameter means. Note that
	 * newer versions of Allegro act as if set_volume_per_voice() was
	 * called with parameter 1 initially, while older versions behave as
	 * if -1 was passed, so you should call the function if you want
	 * consistent behaviour.
	 */
	set_volume_per_voice(0);

	/* Initialise Allegro's sound output system. */
	if (install_sound(DIGI_AUTODETECT, MIDI_NONE, NULL)) {
		allegro_message("Failed to initialise sound driver!\n%s\n", allegro_error);
		return EXIT_FAILURE;
	}

	/* dumb_exit() is a function defined by DUMB. This operation arranges
	 * for dumb_exit() to be called last thing before the program exits.
	 * dumb_exit() does a bit of cleaning up for you. atexit() is
	 * declared in stdlib.h.
	 */
	atexit(&dumb_exit);

	/* DUMB defines its own wrappers for file input. There is a struct
	 * called DUMBFILE that holds function pointers for the various file
	 * operations needed by DUMB. You can decide whether to use stdio
	 * FILE objects, Allegro's PACKFILEs or something else entirely. No
	 * wrapper is installed initially, so you must call this or
	 * dumb_register_stdfiles() or set up your own before trying to load
	 * modules by file name. (If you are using another method, such as
	 * loading an Allegro datafile with modules embedded in it, then DUMB
	 * never opens a file by file name so this doesn't apply.)
	 */
	dumb_register_packfiles();

	/* Load the module file into a DUH object. Quick and dirty: try the
	 * loader for each format until one succeeds. Note that 15-sample
	 * mods have no identifying features, so dumb_load_mod() may succeed
	 * on files that aren't mods at all. We therefore try that one last.
	 */
	duh = dumb_load_it(argv[1]);
	if (!duh) {
		duh = dumb_load_xm(argv[1]);
		if (!duh) {
			duh = dumb_load_s3m(argv[1]);
			if (!duh) {
				duh = dumb_load_mod(argv[1]);
				if (!duh) {
					allegro_message("Failed to load %s!\n", argv[1]);
					return EXIT_FAILURE;
				}
			}
		}
	}

	/* Read the quality values from the config file we told Allegro to
	 * use. You may want to hardcode these or provide a more elaborate
	 * interface via which the user can control them.
	 */
	dumb_resampling_quality = get_config_int("sound", "dumb_resampling_quality", 4);
	dumb_it_max_to_mix = get_config_int("sound", "dumb_it_max_to_mix", 128);

	/* If we're not in DOS, show a window and register our close hook
	 * function.
	 */
#	ifndef ALLEGRO_DOS
		{
			const char *fn = get_filename(argv[1]);
			gfx_half_width = strlen(fn);
			if (gfx_half_width < 22) gfx_half_width = 22;
			gfx_half_width = (gfx_half_width + 2) * 4;

			/* set_window_title() is not const-correct (yet). */
			set_window_title((char *)"DUMB Music Player");

			if (set_gfx_mode(GFX_DUMB_MODE, gfx_half_width*2, 80, 0, 0) == 0) {
				acquire_screen();
				textout_centre(screen, font, fn, gfx_half_width, 20, 14);
				textout_centre(screen, font, "Press any key to exit.", gfx_half_width, 52, 11);
				release_screen();
			} else
				gfx_half_width = 0;
		}

		/* Silly check to get around the fact that someone stupidly removed
		 * an old function from Allegro instead of deprecating it. The old
		 * function was put back a version later, but we may as well use the
		 * new one if it's there!
		 */
#		if ALLEGRO_VERSION*10000 + ALLEGRO_SUB_VERSION*100 + ALLEGRO_WIP_VERSION >= 40105
			set_close_button_callback(&closehook);
#		else
			set_window_close_hook(&closehook);
#		endif

#	endif

	/* We want to continue running if the user switches to another
	 * application.
	 */
	set_display_switch_mode(SWITCH_BACKGROUND);

	/* We have the music loaded, but it isn't playing yet. This starts it
	 * playing. We construct a second object, the AL_DUH_PLAYER, to
	 * represent the playing music. This means you can play the music
	 * twice at the same time should you want to!
	 *
	 * Specify the number of channels (2 for stereo), which 'signal' to
	 * play (always 0 for modules), the volume (1.0f for default), the
	 * buffer size (4096 generally works well) and the sampling frequency
	 * (ideally match the final output frequency Allegro is using). An
	 * Allegro audio stream will be started.
	 */
	dp = al_start_duh(duh, 2, 0, 1.0f,
		get_config_int("sound", "buffer_size", 4096),
		get_config_int("sound", "sound_freq", 44100));

	/* Register our callback functions so that they are called when the
	 * music loops or stops. See docs/howto.txt for more information.
	 * There is no threading issue: DUMB will only process playback
	 * in al_poll_duh(), which we call below.
	 */
	{
		DUH_SIGRENDERER *sr = al_duh_get_sigrenderer(dp);
		DUMB_IT_SIGRENDERER *itsr = duh_get_it_sigrenderer(sr);
		dumb_it_set_loop_callback(itsr, &loop_callback, NULL);
		dumb_it_set_xm_speed_zero_callback(itsr, &xm_speed_zero_callback, NULL);
	}

	/* Main loop. */
	for (;;) {
		/* Check for keys in the buffer. If we get one, discard it
		 * and exit the main loop.
		 */
		if (keypressed()) {
			readkey();
			break;
		}

		/* Poll the music. We exit the loop if al_poll_duh() has
		 * returned nonzero (music finished) or the window has been
		 * closed. al_poll_duh() might return nonzero if you have set
		 * up a callback that tells the music to stop.
		 */
		if (al_poll_duh(dp) || closed)
			break;

		/* Give other threads a look-in, or allow the processor to
		 * sleep for a bit. YIELD() is defined further up in this
		 * file.
		 */
		YIELD();
	}

	/* Remove the audio stream and deallocate the memory being used for
	 * the playback state. We set dp to NULL to emphasise that the object
	 * has gone.
	 */
	al_stop_duh(dp);
	dp = NULL;

	/* Free the DUH object containing the actual music data. */
	unload_duh(duh);
	duh = NULL;

	/* All done! */
	return EXIT_SUCCESS;
}
END_OF_MAIN();
