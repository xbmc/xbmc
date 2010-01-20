#include <glib.h>
#include <gtk/gtk.h>
#include "goom_config.h"

#include <SDL.h>
#include <SDL_thread.h>

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>

#include "goom_tools.h"
#include "goom.h"

#include "frame_rate_tester.h"
#include "gmtimer.h"

#include "pixeldoubler.h"
#include "sdl_pixeldoubler.h"

#include "readme.c"

#include "gtk-support.h"
#include "gtk-interface.h"

#include "sdl_goom.h"

static SdlGoom sdlGoom;

//#define BENCHMARK_X86
#ifdef BENCHMARK_X86
#include <stdint.h>
static uint64_t GetTick()
{
  uint64_t x;
  /* __asm__ volatile (".byte 0x0f, 0x31" : "=A" (x));*/
  asm volatile ("RDTSC" : "=A" (x));
  return x;
}
#endif

/**
 * Callback des parametres
 */
static void screen_size_changed (PluginParam *);
static void fps_limit_changed (PluginParam *);
static void pix_double_changed (PluginParam *);
static void hide_cursor_changed (PluginParam *);

/** 
 * Initialise les parametres de la version SDL
 */
static void init_parameters() {

	static char *resolutions[4] = {"320x240","400x300","640x480","800x600"};
    static struct ListVal reslist = {
        value:0,
        nbChoices:4,
        choices:resolutions
    };

	sdlGoom.screen_size = secure_param ();
	sdlGoom.screen_size.name = "Window's Size:";
	sdlGoom.screen_size.desc = "";
	sdlGoom.screen_size.type = PARAM_LISTVAL;
	sdlGoom.screen_size.param.slist = reslist;
	sdlGoom.screen_size.changed = screen_size_changed;
    set_list_param_value(&sdlGoom.screen_size, "400x300");

	sdlGoom.fps_limit = secure_param ();
	sdlGoom.fps_limit.name = "Frame Rate:";
	sdlGoom.fps_limit.desc = "";
	sdlGoom.fps_limit.type = PARAM_INTVAL;
	sdlGoom.fps_limit.param.ival.min = 1;
	sdlGoom.fps_limit.param.ival.max = 35;
	sdlGoom.fps_limit.param.ival.value = 30;
	sdlGoom.fps_limit.param.ival.step = 1;
	sdlGoom.fps_limit.changed = fps_limit_changed;

	sdlGoom.pix_double = secure_param ();
	sdlGoom.pix_double.name = "Double Pixel";
	sdlGoom.pix_double.type = PARAM_BOOLVAL;
	sdlGoom.pix_double.changed = pix_double_changed;
	BVAL(sdlGoom.pix_double) = 0;

	sdlGoom.hide_cursor = secure_param ();
	sdlGoom.hide_cursor.name = "Hide Cursor";
	sdlGoom.hide_cursor.type = PARAM_BOOLVAL;
	sdlGoom.hide_cursor.changed = hide_cursor_changed;
	BVAL(sdlGoom.hide_cursor) = 1;

	sdlGoom.display_fps = secure_param ();
	sdlGoom.display_fps.name = "Display FPS";
	sdlGoom.display_fps.type = PARAM_BOOLVAL;
	BVAL(sdlGoom.display_fps) = 0;

	sdlGoom.screen = plugin_parameters("Display", 7);
	sdlGoom.screen.params[0]=&sdlGoom.screen_size;
	sdlGoom.screen.params[1]=&sdlGoom.pix_double;
	sdlGoom.screen.params[2]=0;
	sdlGoom.screen.params[3]=&sdlGoom.fps_limit;
	sdlGoom.screen.params[4]=&sdlGoom.display_fps;
	sdlGoom.screen.params[5]=0;
	sdlGoom.screen.params[6]=&sdlGoom.hide_cursor;

	sdlGoom.config_win = 0;
	sdlGoom.screen_height = 300;
	sdlGoom.screen_width = 400;
	sdlGoom.doublepix = 0;
	sdlGoom.active = 1;
}

/*
 * Methodes utilitaires
 */
char *sdl_goom_set_doublepix (int dp);
static void apply_double ();


static int resx = 400;
static int resy = 300;
static int doublepix = 0;
static int doubledec = 0;

static int MAX_FRAMERATE = 32;
static double INTERPIX = 1000.0f / 32;

static SDL_Surface *surface = NULL;
static int is_fs = 0;

/* static void thread_func (); */

static void sdl_goom_init (int argc, char **argv);
static void sdl_goom_cleanup ();
static void sdl_goom_loop ();
static void sdl_goom_render_pcm (gint16 data[2][512], gchar *title);

static Surface *gsurf2 = NULL;
static Surface gsurf;
static char *main_script = NULL;
static char *init_script = NULL;

static int     fini = 0;
static int     disable = 0;
static gint16  snd_data[2][512];

#include <signal.h>

void on_kill(int i) {
  fini    = 1;
  disable = 1;
}

int main (int argc, char **argv) {

	gtk_set_locale ();
	gtk_init (&argc, &argv);

	sdl_goom_init(argc, argv);
  signal(SIGQUIT, on_kill);
	sdl_goom_loop();
	sdl_goom_cleanup();
	return 0;
}

static char *load_file(const char *fname)
{
    FILE *f = fopen(fname, "rt");
    long size;
    char *sc;

    if (!f) {
        fprintf(stderr, "Could not load file %s\n", fname);
        return "";
    }
    
    fseek(f, 0L, SEEK_END);
    size = ftell(f);
    rewind(f);
    sc = (char*)malloc(size+1);
    fread(sc,1,size,f);
    sc[size] = 0;
    fclose(f);
    printf("%s loaded\n", fname);
    return sc;
}

static void display_help()
{
    printf("usage: goom2 <init_script> <main_script>\n");
}

static void check_arg(int argc, char *argv)
{
    static int has_init = 0;
    static int has_main = 0;

    if (argv[0] == '-') {
        if ((!strcmp(argv,"-h"))||(!strcmp(argv,"--help"))) {
            display_help();
            exit(0);
        }
    }
    else if (!has_init) {
        init_script = load_file(argv);
        has_init = 1;
    }
    else if (!has_main) {
        main_script = load_file(argv);
        has_main = 1;
    }
}

void sdl_goom_init (int argc, char **argv)
{
	gint16  data[2][512];
	int     i;
    int init_flags = SDL_INIT_VIDEO;

    for (i=1; i<argc; ++i) {
        check_arg(i,argv[i]);
    }

	init_parameters();

#ifdef VERBOSE
	printf ("--> INITIALIZING GOOM\n");
#endif

	fini = FALSE;
#ifdef THIS_MAKES_ATI_CARDS_TO_CRASH__linux__
  /* This Hack Allows Hardware Surface on Linux */
  setenv("SDL_VIDEODRIVER","dga",0);

  if (SDL_Init(init_flags) < 0) {
    printf(":-( Could not use DGA. Try using goom2 as root.\n");
    setenv("SDL_VIDEODRIVER","x11",1);
    if (SDL_Init(init_flags) < 0) {
      fprintf(stderr, "SDL initialisation error:  %s\n", SDL_GetError());
      exit(1);
    }
  }
  else {
    printf(":-) DGA Available !\n");
    SDL_WM_GrabInput(SDL_GRAB_ON);
  }
#else
  if ( SDL_Init(init_flags) < 0 ) {
    fprintf(stderr, "SDL initialisation error:  %s\n", SDL_GetError());
    exit(1);
  }
#endif
	surface = SDL_SetVideoMode (resx, resy, 32,
								SDL_RESIZABLE|SDL_SWSURFACE);
	SDL_WM_SetCaption ("What A Goom!!", NULL);
	SDL_ShowCursor (0);
	SDL_EnableKeyRepeat (0, 0);
  atexit(SDL_Quit);

	apply_double ();
	sdlGoom.plugin = goom_init (resx, resy);

    /*if (init_script != NULL) {
        gsl_ex(sdlGoom.plugin, init_script);
    }

    if (main_script != NULL) {
        goom_set_main_script(sdlGoom.plugin, main_script);
    }*/

	for (i = 0; i < 512; i++) {
		data[0][i] = 0;
		data[1][i] = 0;
	}

	framerate_tester_init ();
}

/* retourne x>>s , en testant le signe de x */
#define ShiftRight(_x,_s) ((_x<0) ? -(-_x>>_s) : (_x>>_s))

static void
sdl_goom_loop()
{
	static double tnext = 0;
	static gint16 prev0 = 0;
	static int     i, j;
  gchar *ptitle = NULL;
  static char title[2048];

	if (tnext < 0.01)
		tnext = INTERPIX + SDL_GetTicks();

	while (!fini) {
		double t;
		sdl_goom_render_pcm (snd_data, ptitle);
    ptitle = NULL;

		/* load the sound data */
		{
			fd_set rfds;
			struct timeval tv;
			int retval;

			tv.tv_sec = 0;
			tv.tv_usec = 10;

			FD_ZERO(&rfds);
			FD_SET(0, &rfds);
			retval = select(1, &rfds, NULL, NULL, &tv);

			if (retval) {
        int type;
        read (0, &type, sizeof(int));
        switch (type) {
          case 0:
            read (0, snd_data, 512*2*2);
            break;
          case 1:
            read (0, title, 2048);
            ptitle = &title[0];
            break;
          case 2:
            fini = 1;
            disable = TRUE;
            break;
        }
      }
		}

		if (prev0 == snd_data[0][0]) {
			for (i = 0; i < 2; i++)
				for (j = 0; j < 512; j++)
					snd_data[i][j] = ShiftRight((snd_data[i][j] * 31),5);
		}
		prev0 = snd_data[0][0];

		t = SDL_GetTicks();
		if (t < tnext) {
			float t2s = (tnext-t);
			while (t2s>20) {
				usleep(20*1000);
				gtk_main_iteration_do(FALSE);
				t = SDL_GetTicks();
				t2s = tnext-t;
			}
			tnext += INTERPIX;
		}
		else {
			tnext = t+INTERPIX;
		}
		i = 0;
		while (gtk_main_iteration_do(FALSE) == TRUE) {
			if (i++ > 10)
				break;
		}
	}
	/*  else {
	 *    gtk_main_quit();
	 *  }
	 */
}

static void
sdl_goom_cleanup (void)
{
#ifdef VERBOSE
	printf ("--> CLEANUP GOOM\n");
#endif

	if (is_fs) {
		SDL_WM_ToggleFullScreen (surface);
	}
	SDL_Quit ();

	goom_close (sdlGoom.plugin);
	framerate_tester_close ();
}


/*===============================*/

static void apply_double () {

	if (gsurf2) surface_delete (&gsurf2);
	if (!doublepix)
		return;

	if (surface->format->BytesPerPixel == 4)
		doublepix = 2;
	else
		doublepix = 1;

	if (doublepix==2) {
		resx /= 2;
		resy /= 2;
		doubledec = 0;
	}
	else if (doublepix == 1) {
		doubledec = resx % 32;
		resx = resx - doubledec;
		resx /= 2;
		resy /= 2;
		doubledec /= 2;
		gsurf2 = surface_new (resx*2,resy*2);
	}

	gsurf.width = resx;
	gsurf.height = resy;
	gsurf.size = resx*resy;
}


static char * resize_win (int w, int h, int force) {
	static char s[256];
	if ((w != sdlGoom.screen_width)||(h != sdlGoom.screen_height)||force) {
		static SDL_Event e;
		e.resize.type = SDL_VIDEORESIZE;
		e.resize.w = w;
		e.resize.h = h;
		SDL_PushEvent (&e);
	}
	sprintf (s,"%dx%d",w,h);
	return s;
}

static void
sdl_goom_render_pcm (gint16 data[2][512], gchar *title)
{
#ifdef BENCHMARK_X86
  uint64_t t0,t1,t2;
#endif
	static char *msg_tab[] = {
		"What a GOOM! version " VERSION
		  "\n\n\n\n\n\n\n\n"
		  "an iOS sotfware production.\n"
		  "\n\n\n"
		  "http://www.ios-software.com/",
		goom_readme,
		goom_big_readme,
		"Copyright (c)2000-2004, by Jeko"
	};
	static int msg_pos = 0;
#define ENCORE_NUL_LOCK (32*200)
	static int encore_nul = 0;

	guint32 *buf;
	SDL_Surface *tmpsurf = NULL;

	/* static int spos = -1; */

	gchar  *message = NULL;

	/* TODO : Utiliser une commande dans le pipe *
	 * int     pos = xmms_remote_get_playlist_pos (jeko_vp.xmms_session);
	 */

	int forceMode = 0;

#define NBresoli 11
	static int resoli = 7;
	static int resolx[] = {320,320,400,400,512,512,640,640,640,800,800};
	static int resoly[] = {180,240,200,300,280,384,320,400,480,400,600};

	int i;
	SDL_Event event;

	/* Check for events */
	while (SDL_PollEvent (&event)) {	/* Loop until there are no events left on 
										 * the queue */
		switch (event.type) { /* Process the appropiate event type */
			case SDL_QUIT:
				{
					fini = 1;
					disable = TRUE;
				}
				break;

			case SDL_ACTIVEEVENT:
				if (event.active.state & SDL_APPACTIVE)
					sdlGoom.active = event.active.gain;
				break;

			case SDL_KEYDOWN: /* Handle a KEYDOWN event */
				if (event.key.keysym.sym == SDLK_TAB) {
					SDL_WM_ToggleFullScreen (surface);
					is_fs = !is_fs;
				}

				if (event.key.keysym.sym == SDLK_q) {
					fini = 1;
				}
                /*
                 * TODO : GERER TOUT CA AVEC XMMS REMOTE CTRL ? ou le pipe *
                 if (event.key.keysym.sym == SDLK_q) {
                 xmms_remote_quit (jeko_vp.xmms_session);
                 }

                 if (event.key.keysym.sym == SDLK_x)
                 xmms_remote_play (jeko_vp.xmms_session);
                 if (event.key.keysym.sym == SDLK_c)
                 xmms_remote_pause (jeko_vp.xmms_session);
                 if (event.key.keysym.sym == SDLK_v)
                 xmms_remote_stop (jeko_vp.xmms_session);
                 if (event.key.keysym.sym == SDLK_b)
                 xmms_remote_playlist_next (jeko_vp.xmms_session);
                 if (event.key.keysym.sym == SDLK_z)
                 xmms_remote_playlist_prev (jeko_vp.xmms_session);
                 */

				if (event.key.keysym.sym == SDLK_f) {
					BVAL(sdlGoom.display_fps) = !BVAL(sdlGoom.display_fps);
					sdlGoom.display_fps.change_listener(&sdlGoom.display_fps);
				}

				if ((event.key.keysym.sym == SDLK_KP_PLUS) && (resoli+1<NBresoli)) {
					resoli = resoli+1;
					resize_win (resolx[resoli],resoly[resoli],FALSE);
				}
				if ((event.key.keysym.sym == SDLK_KP_MINUS) && (resoli>0)) {
					resoli = resoli-1;
					resize_win (resolx[resoli],resoly[resoli],FALSE);
				}

				if (event.key.keysym.sym == SDLK_KP_MULTIPLY) {
					title = sdl_goom_set_doublepix (!doublepix);
				}
				if (event.key.keysym.sym == SDLK_ESCAPE) {
          if (is_fs) {
  					SDL_WM_ToggleFullScreen (surface);
	  				is_fs = !is_fs;
          }
          else if (sdlGoom.config_win == 0) {
						sdlGoom.config_win = create_config_window ();
						gtk_data_init (&sdlGoom);
						gtk_widget_show (sdlGoom.config_win);
						message = "";
					}
					else {
						message = "Configuration Window is Already Open";
					}
				}
				if (event.key.keysym.sym == SDLK_SPACE) {
					encore_nul = ENCORE_NUL_LOCK;
				}

				if (event.key.keysym.sym == SDLK_F1)
					forceMode = 1;
				if (event.key.keysym.sym == SDLK_F2)
					forceMode = 2;
				if (event.key.keysym.sym == SDLK_F3)
					forceMode = 3;
				if (event.key.keysym.sym == SDLK_F4)
					forceMode = 4;
				if (event.key.keysym.sym == SDLK_F5)
					forceMode = 5;
				if (event.key.keysym.sym == SDLK_F6)
					forceMode = 6;
				if (event.key.keysym.sym == SDLK_F7)
					forceMode = 7;
				if (event.key.keysym.sym == SDLK_F8)
					forceMode = 8;
				if (event.key.keysym.sym == SDLK_F9)
					forceMode = 9;
				if (event.key.keysym.sym == SDLK_F10)
					forceMode = 10;

				break;
			case SDL_VIDEORESIZE:
				resx = sdlGoom.screen_width = event.resize.w;
				resy = sdlGoom.screen_height = event.resize.h;
				sdlGoom.doublepix = doublepix;
				{
					static char s[512];
					sprintf (s,"%dx%d",resx,resy);
					title = s;
					set_list_param_value(&sdlGoom.screen_size, s);
					sdlGoom.screen_size.change_listener (&sdlGoom.screen_size);
				}
				surface = SDL_SetVideoMode (resx, resy, 32,
											SDL_RESIZABLE|SDL_SWSURFACE);
				apply_double();
				goom_set_resolution (sdlGoom.plugin,resx, resy);
				if (is_fs)
					SDL_WM_ToggleFullScreen (surface);
				break;
				/* default:	* Report an unhandled event */
				/* printf("I don't know what this event is!\n"); */
		}
	}

	for (i=0;i<512;i++)
		if (data[0][i]>2) {
			if (encore_nul > ENCORE_NUL_LOCK)
				encore_nul = 0;
			break;
		}

	if ((i == 512) && (!encore_nul))
		encore_nul = ENCORE_NUL_LOCK + 100;

	if (encore_nul == ENCORE_NUL_LOCK) {
		message = msg_tab[msg_pos];
		msg_pos ++;
		msg_pos %= 4;
	}

	if (encore_nul)
		encore_nul --;

	if (!sdlGoom.active) {
		return;
	}

	/*
	 *  TODO:
	 *  if (pos != spos) {
	 *    title = xmms_remote_get_playlist_title (jeko_vp.xmms_session, pos);
	 *    spos = pos;
	 *  }
	 */

#ifdef BENCHMARK_X86
  t0 = GetTick();
#endif
  if (doublepix == 0)
    goom_set_screenbuffer(sdlGoom.plugin, surface->pixels);

	buf = goom_update (sdlGoom.plugin, data, forceMode,
					   BVAL(sdlGoom.display_fps)?framerate_tester_getvalue ():-1,
					   title, message);

#ifdef BENCHMARK_X86
  t1 = GetTick();
#endif

	if (doublepix == 2) {
		gsurf.buf = buf;
		sdl_pixel_doubler (&gsurf,surface);
	} else if (doublepix == 1) {
		SDL_Rect rect;
		gsurf.buf = buf;
		pixel_doubler (&gsurf,gsurf2);
		tmpsurf =
		  SDL_CreateRGBSurfaceFrom (gsurf2->buf, resx*2, resy*2,
									32, resx*8,
									0x00ff0000, 0x0000ff00, 0x000000ff,
									0x00000000);
		rect.x = doubledec;
		rect.y = 0;
		rect.w = resx * 2;
		rect.h = resy * 2;
		SDL_BlitSurface (tmpsurf, NULL, surface, &rect);
		SDL_FreeSurface (tmpsurf);
	}
	else {
/*    tmpsurf =
      SDL_CreateRGBSurfaceFrom (buf, resx, resy, 32, resx * 4,
                                0x00ff0000, 0x0000ff00, 0x000000ff,
                                0x00000000);
    SDL_BlitSurface (tmpsurf, NULL, surface, NULL);
    SDL_FreeSurface (tmpsurf);
    SDL_LockSurface(surface);
    memcpy(surface->pixels, buf, resx * resy * 4);
    SDL_UnlockSurface(surface);
*/
	}
	SDL_Flip (surface);
#ifdef BENCHMARK_X86
  t2 = GetTick();

  t2 -= t1;
  t1 -= t0;
  {
    double ft1, ft2;
    static double min_t1 = 1000.0,
                  min_t2 = 1000.0;
    static double moy_t1 = 150.0;
    static double moy_t2 = 40.0;

    ft1 = (double)(t1 / sdlGoom.plugin->screen.size);
    ft2 = (double)(t2 / sdlGoom.plugin->screen.size);

    if (ft1 < min_t1)
      min_t1 = ft1;
    if (ft2 < min_t2)
      min_t2 = ft2;

    moy_t1 = ((moy_t1 * 15.0) + ft1) / 16.0;
    moy_t2 = ((moy_t2 * 15.0) + ft2) / 16.0;
    printf("UPDATE = %4.0f/%3.0f CPP ", moy_t1, min_t1);
    printf("DISPLAY = %4.0f/%3.0f CPP\n", moy_t2, min_t2);
  }
#endif

	framerate_tester_newframe ();
}


char *sdl_goom_set_doublepix (int dp) {
	if (doublepix && dp) return " ";
	if (!doublepix && !dp) return " ";

	doublepix = dp;
	BVAL(sdlGoom.pix_double) = dp;
	sdlGoom.pix_double.change_listener(&sdlGoom.pix_double);
	if (doublepix)
		return resize_win (resx,resy,TRUE);
	else
		return resize_win (resx*2,resy*2,TRUE);
}

void sdl_goom_set_fps (int fps) {
	MAX_FRAMERATE = fps;
	INTERPIX = 1000.0 / MAX_FRAMERATE;
}

void pix_double_changed (PluginParam *p) {
	sdl_goom_set_doublepix (BVAL(*p));
}

void screen_size_changed (PluginParam *p) {
	int i;
	static struct Resol { char*name; int x; int y; } res[4] = {
		{"320x240", 320, 240},
		{"400x300", 400, 300},
		{"640x480", 640, 480},
		{"800x600", 800, 600}};

		for (i=4;i--;) {
			if (!strcmp(LVAL(*p),res[i].name))
				resize_win (res[i].x,res[i].y,FALSE);
		}
}

void fps_limit_changed (PluginParam *p) {
	MAX_FRAMERATE = IVAL(*p);
	INTERPIX = 1000.0 / MAX_FRAMERATE;
}

void hide_cursor_changed (PluginParam *p) {
	SDL_ShowCursor(!BVAL(*p));
}
