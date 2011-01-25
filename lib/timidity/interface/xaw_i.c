/*
    TiMidity++ -- MIDI to WAVE converter and player
    Copyright (C) 1999-2004 Masanao Izumo <iz@onicos.co.jp>
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
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA

    xaw_i.c - XAW Interface
        from Tomokazu Harada <harada@prince.pe.u-tokyo.ac.jp>
        modified by Yoshishige Arai <ryo2@on.rim.or.jp>
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifndef NO_STRING_H
#include <string.h>
#else
#include <strings.h>
#endif
#include <sys/types.h>
#include <pwd.h>
#include <sys/stat.h>
#include "xaw.h"
#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>
#include <X11/Xaw/Form.h>
#include <X11/Xaw/Box.h>
#include <X11/Xaw/Label.h>
#include <X11/Xaw/Dialog.h>
#include <X11/Xaw/AsciiText.h>

#ifdef OFFIX
#include <OffiX/DragAndDrop.h>
#include <OffiX/DragAndDropTypes.h>
#endif
#include <X11/Xaw/Toggle.h>
#include <X11/Xaw/MenuButton.h>
#include <X11/Xaw/SimpleMenu.h>
#include <X11/Xaw/SmeLine.h>
#include <X11/Xaw/SmeBSB.h>
#include <X11/Xaw/Scrollbar.h>
#include <X11/Xaw/Cardinals.h>
#include <X11/Xaw/Simple.h>
#include <X11/Xaw/List.h>
#include <X11/Xaw/Viewport.h>

#include "check.xbm"
#include "arrow.xbm"
#include "on.xbm"
#include "off.xbm"
#include "timidity.h"
#include "common.h"
#include "instrum.h"
#include "playmidi.h"
#include "readmidi.h"
#include "controls.h"
#include "timer.h"
#include "strtab.h"

#ifndef S_ISDIR
#define S_ISDIR(mode)   (((mode)&0xF000) == 0x4000)
#endif /* S_ISDIR */

#define TRACE_WIDTH 627			/* default height of trace_vport */
#define TRACEVPORT_WIDTH (TRACE_WIDTH+12)
#define TRACE_WIDTH_SHORT 388
#define TRACEV_OFS 22
#define TRACE_FOOT 16
#define TRACEH_OFS 0
#define BAR_SPACE 20
#define BAR_HEIGHT 16
static int VOLUME_LABEL_WIDTH = 80;
#define MAX_TRACE_HEIGHT 560
#if (MAX_XAW_MIDI_CHANNELS*BAR_SPACE+TRACE_FOOT+14 > MAX_TRACE_HEIGHT)
#define TRACE_HEIGHT MAX_TRACE_HEIGHT
#else
#define TRACE_HEIGHT (MAX_XAW_MIDI_CHANNELS*BAR_SPACE+TRACE_FOOT+14)
#endif
#define VOICES_NUM_OFS 6
#define TTITLE_OFS 120
#define FILEVPORT_HEIGHT 336
#define FILEVPORT_WIDTH 272

#define BARSCALE2 0.31111	/* velocity scale   (60-4)/180 */
#define BARSCALE3 0.28125	/* volume scale     (40-4)/128 */
#define BARSCALE4 0.25		/* expression scale (36-4)/128 */
#define BARSCALE5 0.385827	/* expression scale (53-4)/128 */
static int currwidth = 2;
static Dimension rotatewidth[] = {TRACE_WIDTH_SHORT, 592, TRACE_WIDTH+8};

typedef struct {
  int			col;	/* column number */
  char			**cap;	/* caption strings array */
  int			*w;  	/* column width array */
  int			*ofs;	/* column offset array */
} Tplane;
static int plane = 0;

#define TCOLUMN 9
#define T2COLUMN 10
static char *caption[TCOLUMN] =
{"ch","  vel"," vol","expr","prog","pan","pit"," instrument",
 "          keyboard"};
static char *caption2[T2COLUMN] =
{"ch","  vel"," vol","expr","prog","pan","bnk","reverb","chorus",
 "          keyboard"};

static int BARH_SPACE[TCOLUMN]= {22,60,40,36,36,36,30,106,304};
#define BARH_OFS0	(TRACEH_OFS)
#define BARH_OFS1	(BARH_OFS0+22)
#define BARH_OFS2	(BARH_OFS1+60)
#define BARH_OFS3	(BARH_OFS2+40)
#define BARH_OFS4	(BARH_OFS3+36)
#define BARH_OFS5	(BARH_OFS4+36)
#define BARH_OFS6	(BARH_OFS5+36)
#define BARH_OFS7	(BARH_OFS6+30)
#define BARH_OFS8	(BARH_OFS7+106)
static int bar0ofs[] = {BARH_OFS0,BARH_OFS1,BARH_OFS2,BARH_OFS3,
	  BARH_OFS4,BARH_OFS5,BARH_OFS6,BARH_OFS7,BARH_OFS8};

static int BARH2_SPACE[T2COLUMN]= {22,60,40,36,36,36,30,53,53,304};
#define BARH2_OFS0	(TRACEH_OFS)
#define BARH2_OFS1	(BARH2_OFS0+22)
#define BARH2_OFS2	(BARH2_OFS1+60)
#define BARH2_OFS3	(BARH2_OFS2+40)
#define BARH2_OFS4	(BARH2_OFS3+36)
#define BARH2_OFS5	(BARH2_OFS4+36)
#define BARH2_OFS6	(BARH2_OFS5+36)
#define BARH2_OFS7	(BARH2_OFS6+30)
#define BARH2_OFS8	(BARH2_OFS7+53)
#define BARH2_OFS9	(BARH2_OFS8+53)
static int bar1ofs[] = {BARH2_OFS0,BARH2_OFS1,BARH2_OFS2,BARH2_OFS3,
	  BARH2_OFS4,BARH2_OFS5,BARH2_OFS6,BARH2_OFS7,BARH2_OFS8,BARH2_OFS9};

static Tplane pl[] = {
  {TCOLUMN, caption, BARH_SPACE, bar0ofs},
  {T2COLUMN, caption2, BARH2_SPACE, bar1ofs},
};

#define CL_C 0	/* column 0 = channel */
#define CL_VE 1	/* column 1 = velocity */
#define CL_VO 2	/* column 2 = volume */
#define CL_EX 3	/* column 3 = expression */
#define CL_PR 4	/* column 4 = program */
#define CL_PA 5	/* column 5 = panning */
#define CL_PI 6	/* column 6 = pitch bend */
#define CL_IN 7	/* column 7 = instrument name */

#define CL_BA  6	/* column 6 = bank */
#define CL_RE  7	/* column 7 = reverb */
#define CL_CH  8	/* column 8 = chorus */

#define INST_NAME_SIZE 16
#define INIT_FLISTNUM MAX_DIRECTORY_ENTRY
static char *inst_name[MAX_XAW_MIDI_CHANNELS];
static int disp_inst_name_len = 13;
#define UNTITLED_STR "<No Title>"

typedef struct {
  int			id;
  String		name;
  Boolean		trap;
  Boolean		bmflag;
  Widget		widget;
} ButtonRec;

typedef struct {
  int			id;
  int			bit;
  Widget		widget;
} OptionRec;


typedef struct {
  char *dirname;
  char *basename;
} DirPath;

void a_print_text(Widget,char *);
static void drawProg(int,int,int,int,Boolean),drawPan(int,int,Boolean),
  draw1Chan(int,int,char),drawVol(int,int),drawExp(int,int),drawPitch(int,int),
  drawInstname(int,char *),drawDrumPart(int,int),drawBank(int,int),
  drawReverb(int,int),drawChorus(int,int),drawVoices(void),drawTitle(char *),
  quitCB(),playCB(),pauseCB(),stopCB(),prevCB(),nextCB(),
  optionsCB(),optionspopupCB(),optionscloseCB(),chorusCB(),optionsdestroyCB(),
  flistpopupCB(),flistcloseCB(),
  forwardCB(),backCB(),repeatCB(),randomCB(),menuCB(),sndspecCB(),
  volsetCB(),volupdownCB(),tuneslideCB(),filemenuCB(),
  fselectCB(),fdeleteCB(),fdelallCB(),backspaceCB(),aboutCB(),aboutcloseCB(),
#ifndef WIDGET_IS_LABEL_WIDGET
  deleteTextCB(),
#endif
  toggleMark(),popupLoad(),popdownLoad(),volupdownAction(),tuneslideAction(),
  tunesetAction(),a_readconfig(),a_saveconfig(),filemenuAction(),
  setDirAction(),setDirList(),
  drawBar(),redrawTrace(Boolean),completeDir(),ctl_channel_note(),
  drawKeyboardAll(),draw1Note(),redrawAction(),redrawCaption(),
  exchgWidth(),toggletrace(),
  checkRightAndPopupSubmenu(),popdownSubmenuCB(),popdownSubmenu(),leaveSubmenu(),
  addOneFile(int,int,char *,Boolean),
  flistMove(),closeParent(),createOptions(),createFlist();
static char *expandDir(),*strmatch();
static int configcmp();
static void pauseAction(),soundkeyAction(),speedAction(),voiceAction();
static Boolean IsTracePlaying(),IsEffectiveFile();
extern void a_pipe_write(char *);
extern int a_pipe_read(char *,int);
extern int a_pipe_nread(char *buf, int n);
static void initStatus(void);
static void xaw_vendor_setup(void);
static void safe_getcwd(char *cwd, int maxlen);

static Widget title_mb,title_sm,time_l,popup_load,popup_load_f,load_d,load_t;
static Widget load_vport,load_flist,cwd_l,load_info, lyric_t;
static Dimension lyric_height, base_height, text_height;
static GC gc,gcs,gct,gc_xcopy;
static Pixel bgcolor,menubcolor,textcolor,textbgcolor,text2bgcolor,buttonbgcolor,
  buttoncolor,togglecolor,tracecolor,volcolor,expcolor,pancolor,capcolor,rimcolor,
  boxcolor,suscolor,playcolor,revcolor,chocolor;
static Pixel black,white;
static Pixel barcol[MAX_XAW_MIDI_CHANNELS];

static Widget toplevel,m_box,base_f,file_mb,file_sm,bsb,
  quit_b,play_b,pause_b,stop_b,prev_b,next_b,fwd_b,back_b,
  random_b,repeat_b,b_box,v_box,t_box,vol_l0,vol_l,vol_bar,tune_l0,tune_l,tune_bar,
  trace_vport,trace,pbox,popup_opt,popup_optbox,popup_oclose,
  popup_about,popup_abox,popup_aok,about_lbl[5],
  file_vport,file_list,popup_file,popup_fbox,flist_cmdbox,
  popup_fplay,popup_fdelete,popup_fdelall,popup_fclose,
  modul_b,porta_b,nrpnv_b,chpressure_b,txtmeta_b,overlapv_b,reverb_b,chorus_b,
  modul_l,porta_l,nrpnv_l,chpressure_l,txtmeta_l,overlapv_l,reverb_l,chorus_l,
  modul_bb,porta_bb,nrpnv_bb,chpressure_bb,txtmeta_bb,overlapv_bb,reverb_bb,chorus_bb;
static Widget *psmenu = NULL;
static char local_buf[300];
static char window_title[300], *w_title;
int amplitude = DEFAULT_AMPLIFICATION;

#ifndef XAW_BITMAP_DIR
#define XAW_BITMAP_DIR PKGLIBDIR "/bitmaps"
#endif /* XAW_BITMAP_DIR */

String bitmapdir = XAW_BITMAP_DIR;
Boolean arrangetitle,savelist;
static char **current_flist = NULL;
static int voices = 0, last_voice = 0, voices_num_width;
static int maxentry_on_a_menu = 0,submenu_n = 0;
#define OPTIONS_WINDOW 1
#define FLIST_WINDOW 2
#define ABOUT_WINDOW 4
static int popup_shell_exist = 0;

typedef struct {
  Boolean		confirmexit;
  Boolean		repeat;
  Boolean		autostart;
  Boolean		autoexit;
  Boolean		hidetext;
  Boolean		shuffle;
  Boolean		disptrace;
  int			amplitude;
  int			extendopt;
  int			chorusopt;
} Config;

#define FLAG_NOTE_OFF   1
#define FLAG_NOTE_ON    2
#define FLAG_BANK       1
#define FLAG_PROG       2
#define FLAG_PROG_ON    4
#define FLAG_PAN        8
#define FLAG_SUST       16
#define FLAG_BENDT		32
typedef struct {
  int reset_panel;
  int multi_part;
  char v_flags[MAX_XAW_MIDI_CHANNELS];
  int16 cnote[MAX_XAW_MIDI_CHANNELS];
  int16 cvel[MAX_XAW_MIDI_CHANNELS];
  int16 ctotal[MAX_XAW_MIDI_CHANNELS];
  int16 bank[MAX_XAW_MIDI_CHANNELS];
  int16 reverb[MAX_XAW_MIDI_CHANNELS];  
  char c_flags[MAX_XAW_MIDI_CHANNELS];
  Channel channel[MAX_XAW_MIDI_CHANNELS];
  int is_drum[MAX_XAW_MIDI_CHANNELS];
} PanelInfo;

/* Default configuration to execute Xaw interface */
/* (confirmexit repeat autostart autoexit hidetext shuffle disptrace amplitude) */
Config Cfg = { False, False, True, False, False, False, True,
               DEFAULT_AMPLIFICATION, DEFAULT_OPTIONS, DEFAULT_CHORUS };
static PanelInfo *Panel;

typedef struct {
  int y;
  int l;
} KeyL;

typedef struct {
  KeyL k[3];
  int xofs;
  Pixel col;
} ThreeL;
static ThreeL *keyG;
#define KEY_NUM 111

#define MAXBITMAP 10
static char *bmfname[] = {
  "back.xbm","fwrd.xbm","next.xbm","pause.xbm","play.xbm","prev.xbm",
  "quit.xbm","stop.xbm","random.xbm","repeat.xbm",(char *)NULL
};
static char *iconname = "timidity.xbm";
#define BM_BACK			0
#define BM_FWRD			1
#define BM_NEXT			2
#define BM_PAUSE		3
#define BM_PLAY			4
#define BM_PREV			5
#define BM_QUIT			6
#define BM_STOP			7
#define BM_RANDOM		8
#define BM_REPEAT		9
static char *dotfile = NULL;

char *cfg_items[]= {"Tracing", "ConfirmExit", "Disp:file", "Disp:volume",
		"Disp:button", "RepeatPlay", "AutoStart", "Disp:text",
		"Disp:time", "Disp:trace", "CurVol", "ShufflePlay",
		"AutoExit", "CurFileMode", "ExtOptions", "ChorusOption","File"};
#define S_Tracing 0
#define S_ConfirmExit 1
#define S_DispFile 2
#define S_DispVolume 3 
#define S_DispButton 4
#define S_RepeatPlay 5
#define S_AutoStart 6
#define S_DispText 7
#define S_DispTime 8
#define S_DispTrace 9
#define S_CurVol 10
#define S_ShufflePlay 11
#define S_AutoExit 12
#define S_CurFileMode 13
#define S_ExtOptions 14
#define S_ChorusOption 15
#define S_MidiFile 16
#define CFGITEMSNUMBER 17

#define COMMON_BGCOLOR "gray67"
#define COMMANDBUTTON_COLOR "gray78"
#define TEXTBG_COLOR "gray82"

static Display *disp;
static int screen;
static Pixmap check_mark, arrow_mark, on_mark, off_mark, layer[2];

static int bm_height[MAXBITMAP], bm_width[MAXBITMAP],
  x_hot,y_hot, root_height, root_width;
static Pixmap bm_Pixmap[MAXBITMAP];
static int max_files, init_options = 0, init_chorus = 0;
static char basepath[PATH_MAX];
static String *dirlist = NULL, dirlist_top, *flist = NULL;
static Dimension trace_width, trace_height, menu_width;
static int total_time = 0, curr_time;
static float thumbj;
static XFontStruct *labelfont,*volumefont,*tracefont;
#ifdef I18N
static XFontSet ttitlefont;
static XFontStruct *ttitlefont0;
static char **ml;
static XFontStruct **fs_list;
#else
static XFontStruct *ttitlefont;
#define ttitlefont0 ttitlefont
#endif

static struct _app_resources {
  String bitmap_dir;
  Boolean arrange_title,save_list,gradient_bar;
  Dimension text_height,trace_width,trace_height,menu_width;
  Pixel common_fgcolor,common_bgcolor,menub_bgcolor,text_bgcolor,text2_bgcolor,
	toggle_fgcolor,button_fgcolor,button_bgcolor,
	velocity_color,drumvelocity_color,volume_color,expr_color,pan_color,
	trace_bgcolor,rim_color,box_color,caption_color,sus_color,white_key_color,black_key_color,play_color,
	rev_color,cho_color;
  XFontStruct *label_font,*volume_font,*trace_font;
  String more_text,file_text;
#ifdef I18N
  XFontSet text_font, ttitle_font;
#else
  XFontStruct *text_font, *ttitle_font;
#endif
} app_resources;

static XtResource xaw_resources[] ={
#define offset(entry) XtOffset(struct _app_resources*, entry)
  {"bitmapDir", "BitmapDir", XtRString, sizeof(String),
   offset(bitmap_dir), XtRString, XAW_BITMAP_DIR },
  {"arrangeTitle", "ArrangeTitle", XtRBoolean, sizeof(Boolean),
   offset(arrange_title), XtRImmediate, (XtPointer)False},
  {"saveList", "SaveList", XtRBoolean, sizeof(Boolean),
   offset(save_list), XtRImmediate, (XtPointer)True},
  {"gradientBar", "GradientBar", XtRBoolean, sizeof(Boolean),
   offset(gradient_bar), XtRImmediate, (XtPointer)False},
#ifdef WIDGET_IS_LABEL_WIDGET
  {"textLHeight", "TextLHeight", XtRShort, sizeof(short),
   offset(text_height), XtRImmediate, (XtPointer)30},
#else
  {"textHeight", "TextHeight", XtRShort, sizeof(short),
   offset(text_height), XtRImmediate, (XtPointer)120},
#endif
  {"traceWidth", "TraceWidth", XtRShort, sizeof(short),
   offset(trace_width), XtRImmediate, (XtPointer)TRACE_WIDTH},
  {"traceHeight", "TraceHeight", XtRShort, sizeof(short),
   offset(trace_height), XtRImmediate, (XtPointer)TRACE_HEIGHT},
  {"menuWidth", "MenuWidth", XtRShort, sizeof(Dimension),
   offset(menu_width), XtRImmediate, (XtPointer)200},
  {"foreground", XtCForeground, XtRPixel, sizeof(Pixel),
   offset(common_fgcolor), XtRString, "black"},
  {"background", XtCBackground, XtRPixel, sizeof(Pixel),
   offset(common_bgcolor), XtRString, COMMON_BGCOLOR},
  {"menubutton", "MenuButtonBackground", XtRPixel, sizeof(Pixel),
   offset(menub_bgcolor), XtRString, "#CCFF33"},
  {"textbackground", "TextBackground", XtRPixel, sizeof(Pixel),
   offset(text_bgcolor), XtRString, TEXTBG_COLOR},
  {"text2background", "Text2Background", XtRPixel, sizeof(Pixel),
   offset(text2_bgcolor), XtRString, "gray80"},
  {"toggleforeground", "ToggleForeground", XtRPixel, sizeof(Pixel),
   offset(toggle_fgcolor), XtRString, "MediumBlue"},
  {"buttonforeground", "ButtonForeground", XtRPixel, sizeof(Pixel),
   offset(button_fgcolor), XtRString, "blue"},
  {"buttonbackground", "ButtonBackground", XtRPixel, sizeof(Pixel),
   offset(button_bgcolor), XtRString, COMMANDBUTTON_COLOR},
  {"velforeground", "VelForeground", XtRPixel, sizeof(Pixel),
   offset(velocity_color), XtRString, "orange"},
  {"veldrumforeground", "VelDrumForeground", XtRPixel, sizeof(Pixel),
   offset(drumvelocity_color), XtRString, "red"},
  {"volforeground", "VolForeground", XtRPixel, sizeof(Pixel),
   offset(volume_color), XtRString, "LightPink"},
  {"expforeground", "ExpForeground", XtRPixel, sizeof(Pixel),
   offset(expr_color), XtRString, "aquamarine"},
  {"panforeground", "PanForeground", XtRPixel, sizeof(Pixel),
   offset(pan_color), XtRString, "blue"},
  {"tracebackground", "TraceBackground", XtRPixel, sizeof(Pixel),
   offset(trace_bgcolor), XtRString, "gray90"},
  {"rimcolor", "RimColor", XtRPixel, sizeof(Pixel),
   offset(rim_color), XtRString, "gray20"},
  {"boxcolor", "BoxColor", XtRPixel, sizeof(Pixel),
   offset(box_color), XtRString, "gray76"},
  {"captioncolor", "CaptionColor", XtRPixel, sizeof(Pixel),
   offset(caption_color), XtRString, "DarkSlateGrey"},
  {"sustainedkeycolor", "SustainedKeyColor", XtRPixel, sizeof(Pixel),
   offset(sus_color), XtRString, "red4"},
  {"whitekeycolor", "WhiteKeyColor", XtRPixel, sizeof(Pixel),
   offset(white_key_color), XtRString, "white"},
  {"blackkeycolor", "BlackKeyColor", XtRPixel, sizeof(Pixel),
   offset(black_key_color), XtRString, "black"},
  {"playingkeycolor", "PlayingKeyColor", XtRPixel, sizeof(Pixel),
   offset(play_color), XtRString, "maroon1"},
  {"reverbcolor", "ReverbColor", XtRPixel, sizeof(Pixel),
   offset(rev_color), XtRString, "PaleGoldenrod"},
  {"choruscolor", "ChorusColor", XtRPixel, sizeof(Pixel),
   offset(cho_color), XtRString, "yellow"},
  {"labelfont", "LabelFont", XtRFontStruct, sizeof(XFontStruct *),
   offset(label_font), XtRString, "-adobe-helvetica-bold-r-*-*-14-*-75-75-*-*-*-*"},
  {"volumefont", "VolumeFont", XtRFontStruct, sizeof(XFontStruct *),
   offset(volume_font), XtRString, "-adobe-helvetica-bold-r-*-*-12-*-75-75-*-*-*-*"},
#ifdef I18N
  {"textfontset", "TextFontSet", XtRFontSet, sizeof(XFontSet),
   offset(text_font), XtRString, "-*-*-medium-r-normal--14-*-*-*-*-*-*-*"},
  {"ttitlefont", "TtitleFont", XtRFontSet, sizeof(XFontSet),
   offset(ttitle_font), XtRString, "-*-fixed-medium-r-normal--14-*-*-*-*-*-*-*"},
#else
  {"textfont", XtCFont, XtRFontStruct, sizeof(XFontStruct *),
   offset(text_font), XtRString, "-*-fixed-medium-r-normal--14-*-*-*-*-*-*-*"},
  {"ttitlefont", "Ttitlefont", XtRFontStruct, sizeof(XFontStruct *),
   offset(ttitle_font), XtRString, "-adobe-helvetica-bold-o-*-*-14-*-75-75-*-*-*-*"},
#endif
  {"tracefont", "TraceFont", XtRFontStruct, sizeof(XFontStruct *),
   offset(trace_font), XtRString, "7x14"},
  {"labelfile", "LabelFile", XtRString, sizeof(String),
   offset(file_text), XtRString, "file..."},
#undef offset
};

enum {
    ID_LOAD = 100,
    ID_SAVECONFIG,
    ID_HIDETXT,
    ID_HIDETRACE,
    ID_SHUFFLE,
    ID_REPEAT,
    ID_AUTOSTART,
    ID_AUTOQUIT,
    ID_LINE,
    ID_FILELIST,
    ID_OPTIONS,
    ID_LINE2,
    ID_ABOUT,
    ID_QUIT,
};
#define IDS_SAVECONFIG  "101"
#define IDS_HIDETXT     "102"
#define IDS_HIDETRACE   "103"
#define IDS_SHUFFLE     "104"
#define IDS_REPEAT      "105"
#define IDS_AUTOSTART   "106"
#define IDS_AUTOQUIT    "107"
#define IDS_FILELIST    "109"
#define IDS_OPTIONS     "110"
#define IDS_ABOUT       "112"


static ButtonRec file_menu[] = {
  {ID_LOAD, "load", True, False},
  {ID_SAVECONFIG, "saveconfig", True, False},
  {ID_HIDETXT, "hidetext", True, False},
  {ID_HIDETRACE, "hidetrace", True, False},
  {ID_SHUFFLE, "shuffle", True, False},
  {ID_REPEAT, "repeat", True, False},
  {ID_AUTOSTART, "autostart", True, False},
  {ID_AUTOQUIT, "autoquit", True, False},
  {ID_LINE, "line", False, False},
  {ID_FILELIST, "filelist", True, False},
  {ID_OPTIONS, "modes", True, False},
  {ID_LINE2, "line2", False, False},
  {ID_ABOUT, "about", True, False},
  {ID_QUIT, "quit", True, False},
};

static OptionRec option_num[] = {
  {MODUL_N, MODUL_BIT},
  {PORTA_N, PORTA_BIT},
  {NRPNV_N, NRPNV_BIT},
  {REVERB_N, REVERB_BIT},
  {CHPRESSURE_N, CHPRESSURE_BIT},
  {OVERLAPV_N, OVERLAPV_BIT},
  {TXTMETA_N, TXTMETA_BIT},
};

static void offPauseButton(void) {
  Boolean s;

  XtVaGetValues(pause_b,XtNstate,&s,NULL);
  if (s == True) {
    s=False;
    XtVaSetValues(pause_b,XtNstate,&s,NULL);
    a_pipe_write("U");
  }
}

static void offPlayButton(void) {
  Boolean s;

  XtVaGetValues(play_b,XtNstate,&s,NULL);
  if (s == True) {
    s=False;
    XtVaSetValues(play_b,XtNstate,&s,NULL);
    a_pipe_write("T 0\n");
  }
}

static Boolean IsTracePlaying(void) {
  Boolean s;

  if (!ctl->trace_playing) return False;
  XtVaGetValues(play_b,XtNstate,&s,NULL);
  return(s);
}

static Boolean IsEffectiveFile(char *file) {
  char *p2;
  struct stat st;

  if((p2 = strrchr(file,'#')) != NULL)
    *p2 = '\0';
  if(stat(file, &st) != -1)
    if (st.st_mode & S_IFMT & (S_IFDIR|S_IFREG|S_IFLNK)) {
      if(p2) *p2 = '#';
      return(True);
    }
  return(False);
}

static Boolean onPlayOffPause(void) {
  Boolean s;
  Boolean play_on;

  play_on = False;
  XtVaGetValues(play_b,XtNstate,&s,NULL);
  if (s == False) {
    s=True;
    XtVaSetValues(play_b,XtNstate,&s,NULL);
    play_on = True;
  }
  offPauseButton();
  return(play_on);
}

/*ARGSUSED*/
static void chorusCB(Widget w,XtPointer id_data, XtPointer data) {
  Boolean s;
  char str[16];

  stopCB(NULL,NULL,NULL);
  XtVaGetValues(w,XtNstate,&s,NULL);
  XtVaSetValues(w,XtNbitmap,(s)? on_mark:off_mark,NULL);
  if(!s)
    sprintf(str, "C 0");
  else
    sprintf(str, "C %03d", (init_chorus)? init_chorus:1);
  a_pipe_write(str);
}

/*ARGSUSED*/
static void optionsCB(Widget w,XtPointer id_data, XtPointer data) {
  Boolean s;
  int i,flags;
  char str[16];

  stopCB(NULL,NULL,NULL);
  XtVaGetValues(w,XtNstate,&s,NULL);
  XtVaSetValues(w,XtNbitmap,(s)? on_mark:off_mark,NULL);
  flags = 0;
  for(i=0; i<MAX_OPTION_N; i++) {
    XtVaGetValues(option_num[i].widget,XtNstate,&s,NULL);
    flags |= ((s)? option_num[i].bit:0);
  }
  sprintf(str, "E %03d", flags);
  init_options = flags;
  a_pipe_write(str);
}

/*ARGSUSED*/
static void optionspopupCB(Widget w,XtPointer data,XtPointer dummy) {
  Dimension x,y;

  createOptions();
  XtVaGetValues(toplevel,XtNx,&x,XtNy,&y,NULL);
  XtVaSetValues(popup_opt,XtNx,x+TRACE_WIDTH_SHORT,XtNy,y,NULL);
  XtPopup(popup_opt,(XtGrabKind)XtGrabNone);
}

static void flistpopupCB(Widget w,XtPointer data,XtPointer dummy) {
  Dimension x,y,w1,h1;

  createFlist();
  XtVaGetValues(toplevel,XtNx,&x,XtNy,&y,NULL);
  XtVaSetValues(popup_file,XtNx,x+TRACE_WIDTH_SHORT,XtNy,y,NULL);
  XtPopup(popup_file,(XtGrabKind)XtGrabNone);
  XtVaGetValues(file_vport,XtNwidth,&w1,XtNheight,&h1,NULL);
  XtVaSetValues(file_vport,XtNheight,((h1>FILEVPORT_HEIGHT)? h1:FILEVPORT_HEIGHT),NULL);
}

static void aboutCB(Widget w,XtPointer data,XtPointer dummy) {
  char s[12],*p;
  int i;

  static char *info[]= {"TiMidity++ %s%s - Xaw interface",
                      "- MIDI to WAVE converter and player -",
                      "by Masanao Izumo and Tomokazu Harada",
                      "modified by Yoshishige Arai"," ",NULL};

  if(!(popup_shell_exist & ABOUT_WINDOW)) {
    popup_about= XtVaCreatePopupShell("popup_about",transientShellWidgetClass,
                                  toplevel,NULL);
    popup_abox= XtVaCreateManagedWidget("popup_abox",boxWidgetClass,popup_about,
                                  XtNwidth,320,XtNheight,120,
                                  XtNorientation,XtorientVertical,
                                  XtNbackground,bgcolor,NULL);
    for(i=0,p=info[0]; p!=NULL; p=info[++i]) {
      snprintf(s,sizeof(s),"about_lbl%d",i);
      snprintf(local_buf, sizeof(local_buf), p,
      		(strcmp(timidity_version, "current")) ? "version " : "",
      		timidity_version);
      about_lbl[i]= XtVaCreateManagedWidget(s,labelWidgetClass,popup_abox,
                                  XtNlabel,local_buf,XtNfont,((i)? labelfont:labelfont),
                                  XtNforeground,textcolor, XtNbackground,bgcolor,
                                  XtNwidth,320,XtNresize,False,
                                  XtNborderWidth,0,NULL);
    }
    popup_aok= XtVaCreateManagedWidget("OK",
                    commandWidgetClass,popup_abox,
                    XtNwidth,320,XtNresize,False,NULL);
    XtAddCallback(popup_aok,XtNcallback,aboutcloseCB,NULL);
    XtSetKeyboardFocus(popup_about, popup_abox);
    popup_shell_exist |= ABOUT_WINDOW;
  }
  XtVaSetValues(popup_about,XtNx,root_width/2 -160,XtNy,root_height/2 -60,NULL);
  XtPopup(popup_about,(XtGrabKind)XtGrabNonexclusive);
}

static void aboutcloseCB(Widget w,XtPointer data,XtPointer dummy) {
  XtPopdown(popup_about);
}

static void flistcloseCB(Widget w,XtPointer data,XtPointer dummy) {
  XtPopdown(popup_file);
}

/*ARGSUSED*/
static void optionscloseCB(Widget w,XtPointer data,XtPointer dummy) {
  XtPopdown(popup_opt);
}

/*ARGSUSED*/
static void optionsdestroyCB(Widget w,XtPointer data,XtPointer dummy) {
  popup_shell_exist &= (! OPTIONS_WINDOW);
}

/*ARGSUSED*/
static void quitCB(Widget w,XtPointer data,XtPointer dummy) {
  a_pipe_write("Q");
}

/*ARGSUSED*/
static void sndspecCB(Widget w,XtPointer data,XtPointer dummy) {
#ifdef SUPPORT_SOUNDSPEC
  a_pipe_write("g");
#endif
}

/*ARGSUSED*/
static void playCB(Widget w,XtPointer data,XtPointer dummy) {
  if(!max_files) return;
  onPlayOffPause();
  XtVaGetValues(title_mb,XtNlabel,&local_buf+2,NULL);
  a_pipe_write("P");
}

/*ARGSUSED*/
static void pauseAction(Widget w,XEvent *e,String *v,Cardinal *n) {
  Boolean s;

  XtVaGetValues(play_b,XtNstate,&s,NULL);
  if (e->type == KeyPress && s == True) {
    XtVaGetValues(pause_b,XtNstate,&s,NULL);
    s ^= True;
    XtVaSetValues(pause_b,XtNstate,&s,NULL);
    a_pipe_write("U");
  }
}

static void soundkeyAction(Widget w,XEvent *e,String *v,Cardinal *n) {
  a_pipe_write(*(int *)n == 0 ? "+\n":"-\n");
}

static void speedAction(Widget w,XEvent *e,String *v,Cardinal *n) {
  a_pipe_write(*(int *)n == 0 ? ">\n":"<\n");
}

static void voiceAction(Widget w,XEvent *e,String *v,Cardinal *n) {
  a_pipe_write(*(int *)n == 0 ? "O\n":"o\n");
}

static void randomAction(Widget w,XEvent *e,String *v,Cardinal *n) {
  Boolean s;
  XtVaGetValues(random_b,XtNstate,&s,NULL);
  s ^= True;
  if(!s) XtVaSetValues(random_b,XtNstate,s,NULL);
    randomCB(NULL,&s,NULL);
}

static void repeatAction(Widget w,XEvent *e,String *v,Cardinal *n) {
  Boolean s;
  XtVaGetValues(repeat_b,XtNstate,&s,NULL);
  s ^= True;
  if(!s) XtVaSetValues(repeat_b,XtNstate,s,NULL);
  repeatCB(NULL,&s,NULL);
}

/*ARGSUSED*/
static void pauseCB() {
  Boolean s;

  XtVaGetValues(play_b,XtNstate,&s,NULL);
  if (s == True) {
    a_pipe_write("U");
  } else {
    s = True;
    XtVaSetValues(pause_b,XtNstate,&s,NULL);
  }
}

/*ARGSUSED*/
static void stopCB(Widget w,XtPointer data,XtPointer dummy) {
  offPlayButton();
  offPauseButton();
  a_pipe_write("S");
  initStatus();
  window_title[0]='\0';
  redrawTrace(False);
}

/*ARGSUSED*/
static void nextCB(Widget w,XtPointer data,XtPointer dummy) {
  onPlayOffPause();
  a_pipe_write("N");
  initStatus();
  redrawTrace(True);
}

/*ARGSUSED*/
static void prevCB(Widget w,XtPointer data,XtPointer dummy) {
  onPlayOffPause();
  a_pipe_write("B");
  initStatus();
  redrawTrace(True);
}

/*ARGSUSED*/
static void forwardCB(Widget w,XtPointer data,XtPointer dummy) {
  if (onPlayOffPause()) a_pipe_write("P");
  a_pipe_write("f");
  initStatus();
}

/*ARGSUSED*/
static void backCB(Widget w,XtPointer data,XtPointer dummy) {
  if (onPlayOffPause()) a_pipe_write("P");
  a_pipe_write("b");
  initStatus();
}

/*ARGSUSED*/
static void repeatCB(Widget w,XtPointer data,XtPointer dummy) {
  Boolean s;
  Boolean *set= (Boolean *)data;

  if (set != NULL && *set) {
    s = *set;
    XtVaSetValues(repeat_b,XtNstate,set,NULL);
  } else {
    XtVaGetValues(repeat_b,XtNstate,&s,NULL);
  }
  if (s == True) {
    a_pipe_write("R 1");
  } else a_pipe_write("R 0");
  toggleMark(file_menu[ID_REPEAT-100].widget, file_menu[ID_REPEAT-100].id);
}

/*ARGSUSED*/
static void randomCB(Widget w,XtPointer data,XtPointer dummy) {
  Boolean s;
  Boolean *set= (Boolean *)data;

  onPlayOffPause();
  if (set != NULL && *set) {
    s = *set;
    XtVaSetValues(random_b,XtNstate,set,NULL);
  } else {
    XtVaGetValues(random_b,XtNstate,&s,NULL);
  }
  if (s == True) {
    onPlayOffPause();
    a_pipe_write("D 1");
  } else {
    offPlayButton();
    offPauseButton();
    a_pipe_write("D 2");
  }
  toggleMark(file_menu[ID_SHUFFLE-100].widget, file_menu[ID_SHUFFLE-100].id);
}

static void menuCB(Widget w,XtPointer data,XtPointer dummy) {
  onPlayOffPause();
  sprintf(local_buf,"L %d",((int)data)+1);
  a_pipe_write(local_buf);
}

static void setVolbar(int val) {
  char s[8];
  float thumb;

  amplitude = (val > MAXVOLUME)? MAXVOLUME:val;
  sprintf(s, "%d", val);
  XtVaSetValues(vol_l, XtNlabel, s, NULL);
  sprintf(s, "V %03d\n", val);
  a_pipe_write(s);
  thumb = (float)val / (float)MAXVOLUME;
  sprintf(s, "%d", val);
  XtVaSetValues(vol_l, XtNlabel, s, NULL);
  if (sizeof(thumb) > sizeof(XtArgVal)) {
    XtVaSetValues(vol_bar, XtNtopOfThumb, &thumb, NULL);
  } else {
    XtArgVal *l_thumb = (XtArgVal *) &thumb;
    XtVaSetValues(vol_bar, XtNtopOfThumb,*l_thumb, NULL);
  }
}

static void volsetCB(Widget w,XtPointer data,XtPointer call_data) {
  float percent = *(float*)call_data;
  int val = (float)(MAXVOLUME * percent);

  if (amplitude == val) return;
  setVolbar(val);
}

static void volupdownCB(Widget w,XtPointer data,XtPointer diff) {
  int i = ((int)diff > 0)? (-10):10;

  i += amplitude;
  setVolbar(i);
}

static void volupdownAction(Widget w,XEvent *e,String *v,Cardinal *n) {
  int i = atoi(*v);

  i += amplitude;
  setVolbar(i);
}

#if 0 /* Not used */
static void tunesetCB(Widget w,XtPointer data,XtPointer call_data)
{
  static int tmpval;
  char s[16];
  float percent = *(float*)call_data;
  int value = (float)(total_time * percent);
  float thumb, l_thumb;

  if (tmpval == value) return;
  if (curr_time > total_time) curr_time = total_time;
  curr_time = tmpval = value;
  l_thumb = thumb = (float)curr_time / (float)total_time;
  snprintf(s,sizeof(s), "%2d:%02d", tmpval / 60, tmpval % 60);
  XtVaSetValues(tune_l0, XtNlabel, s, NULL);
  sprintf(s, "T %d\n", tmpval);
  a_pipe_write(s);
}
#endif

static void tunesetAction(Widget w,XEvent *e,String *v,Cardinal *n) {
  static float tmpval;
  char s[16];
  int value;
  float l_thumb;

  XtVaGetValues(tune_bar, XtNtopOfThumb, &l_thumb, NULL);
  if (tmpval == l_thumb) return;
  tmpval = l_thumb;
  value = (int)(l_thumb * total_time);
  snprintf(s,sizeof(s), "%2d:%02d", curr_time / 60, curr_time % 60);
  XtVaSetValues(tune_l0, XtNlabel, s, NULL);
  XtVaSetValues(tune_bar, XtNtopOfThumb, &l_thumb, NULL);
  sprintf(s, "T %d\n", value);
  a_pipe_write(s);
}

static void tuneslideCB(Widget w,XtPointer data,XtPointer diff) {
  char s[16];

  sprintf(s, "T %d\n", curr_time+ (int)diff);
  a_pipe_write(s);
}

static void tuneslideAction(Widget w,XEvent *e,String *v,Cardinal *n) {
  char s[16];
  float l_thumb;

  XtVaGetValues(tune_bar, XtNtopOfThumb, &l_thumb, NULL);
  sprintf(s, "T %d\n", (int)(total_time * l_thumb));
  a_pipe_write(s);
}

/*ARGSUSED*/
static void resizeAction(Widget w,XEvent *e,String *v,Cardinal *n) {
  Dimension w1,w2,h1,h2;
  Position y1;
  int i,tmp,tmp2,tmp3;

  XtVaGetValues(toplevel,XtNwidth,&w1,XtNheight,&h1,NULL);
  w2 = w1 -8;
  if(w2>TRACEVPORT_WIDTH) w2 = TRACEVPORT_WIDTH;
  XtVaGetValues(lyric_t,XtNheight,&h2,NULL);
  XtResizeWidget(lyric_t,w2-2,h2,0);
  i= 0; tmp = 10;
  while (tmp > 0) {
    i++; tmp -= (int)(w2) / 36;
  }
  XtVaSetValues(lyric_t,XtNborderWidth,1,NULL);
  XtVaSetValues(b_box,XtNheight,i*40,NULL);
  XtResizeWidget(b_box,w2,i*40,0);

  if(ctl->trace_playing) {
    XtVaGetValues(trace_vport,XtNy,&y1,NULL);
    XtResizeWidget(trace_vport,w2,((h1-y1>TRACE_HEIGHT+12)? TRACE_HEIGHT+12:h1-y1),0);
  }
  XtVaGetValues(v_box,XtNheight,&h2,NULL);
  w2 = ((w1 < TRACE_WIDTH_SHORT)? w1:TRACE_WIDTH_SHORT);    /* new v_box width */
  tmp = XTextWidth(app_resources.volume_font,"Volume ",7)+8; /* vol_l width */
  XtVaSetValues(vol_l0,XtNwidth,tmp,NULL);
  XtVaSetValues(v_box,XtNwidth,w2,NULL);
  tmp2 = w2 -tmp - XTextWidth(app_resources.volume_font,"000",3) -38;
  tmp3 = w2 -XTextWidth(app_resources.volume_font,"/ 99:59",7)
    - XTextWidth(app_resources.volume_font,"000",3) -45;
  XtResizeWidget(v_box,w2,h2,0);
  XtVaGetValues(vol_bar,XtNheight,&h2,NULL);
  XtVaSetValues(vol_bar,XtNwidth,tmp2,NULL);
  XtVaSetValues(tune_bar,XtNwidth,tmp3,NULL);
  XtResizeWidget(vol_bar,tmp2,h2,0);
  XtResizeWidget(tune_bar,tmp3,h2,0);
  XSync(disp, False);
  usleep(10000);
}

#ifndef WIDGET_IS_LABEL_WIDGET
void a_print_text(Widget w, char *st) {
  XawTextPosition pos;
  XawTextBlock tb;

  st = strcat(st, "\n");
  pos = XawTextGetInsertionPoint(w);
  tb.firstPos = 0;
  tb.length = strlen(st);
  tb.ptr = st;
  tb.format = FMT8BIT;
  XawTextReplace(w, pos, pos, &tb);
  XawTextSetInsertionPoint(w, pos + tb.length);
}
#else
void a_print_text(Widget w, char *st) {
  XtVaSetValues(w,XtNlabel,st,NULL);
}
#endif /* !WIDGET_IS_LABEL_WIDGET */

/*ARGSUSED*/
static void popupLoad(Widget w,XtPointer client_data,XtPointer call_data) {
#define DIALOG_HEIGHT 400
  Position popup_x, popup_y, top_x, top_y;
  Dimension top_width;

  XtVaGetValues(toplevel, XtNx,&top_x,XtNy,&top_y,XtNwidth,&top_width,NULL);
  popup_x=top_x+ 20;
  popup_y=top_y+ 72;
  top_width += 100;
  if(popup_x+top_width > root_width) popup_x = root_width -top_width -20;
  if(popup_y+DIALOG_HEIGHT > root_height) popup_y = root_height -DIALOG_HEIGHT -20;

  XtVaSetValues(popup_load, XtNx,popup_x,XtNy,popup_y,XtNwidth,top_width,
                XtNheight,DIALOG_HEIGHT,NULL);
  XtRealizeWidget(popup_load);
  XtPopup(popup_load,(XtGrabKind)XtGrabNone);
  top_width -= 4;
  XtVaSetValues(load_vport,XtNwidth,top_width,NULL);
  XawTextSetInsertionPoint(load_t,(XawTextPosition)(strlen(basepath)));
}

static void popdownLoad(Widget w,XtPointer s,XtPointer data) {
  char *p, *p2;
  DirPath full;
  char local_buf[300],tmp[PATH_MAX];
#ifndef	ORIGINAL
  int	Aflag = 0; /* RAKK/HIOENS: adding All files in directory */
#endif	/*RAKK/HIOENS*/

  /* tricky way for both use of action and callback */
  if (s != NULL && data == NULL){
    if(*(char *)s == 'A') {
      snprintf(tmp,sizeof(tmp),"%s%c",basepath,'/');
      p = tmp;
#ifndef	ORIGINAL
      Aflag = 1;
#endif	/*RAKK/HIOENS*/
    } else {
      p = XawDialogGetValueString(load_d);
    }
    if (NULL != (p2 = expandDir(p, &full)))
      p = p2;
    if(IsEffectiveFile(p)) {
#ifndef	ORIGINAL
      if(Aflag == 1) strcat(p,"/");
#endif	/*RAKK/HIOENS*/
      snprintf(local_buf,sizeof(local_buf),"X %s\n",p);
      a_pipe_write(local_buf);
    }
  }
  XtPopdown(popup_load);
}

static void toggleMark(Widget w,int id) {
  file_menu[id-100].bmflag ^= True;
  XtVaSetValues(w,XtNleftBitmap,
                ((file_menu[id-100].bmflag)? check_mark:None),NULL);
}

static void filemenuAction(Widget w,XEvent *e,String *v,Cardinal *n) {
  int i;

  if(e == NULL)
    i= ID_HIDETXT;
  else
    i= atoi(*v);
  if(!(ctl->trace_playing) && i == ID_HIDETRACE) i= ID_HIDETXT;
  filemenuCB(file_menu[i-100].widget,&file_menu[i-100].id,NULL);
}

static void filemenuCB(Widget w,XtPointer id_data, XtPointer data) {
  int *id = (int *)id_data;
  Dimension w1,h1,w2,h2,tmp;

  switch (*id) {
    case ID_LOAD:
      popupLoad(w,NULL,NULL);
      break;
    case ID_AUTOSTART:
      toggleMark(w,*id);
      break;
    case ID_AUTOQUIT:
      toggleMark(w,*id);
      a_pipe_write("q");
      break;
    case ID_HIDETRACE:
      if(ctl->trace_playing) {
        XtVaGetValues(toplevel,XtNheight,&h1,XtNwidth,&w1,NULL);
        if(XtIsManaged(trace_vport)) {
          tmp = trace_height + (XtIsManaged(lyric_t) ? 0:lyric_height);
          XtUnmanageChild(trace_vport);
          XtMakeResizeRequest(toplevel,w1,base_height-tmp,&w2,&h2);
        } else {
          XtManageChild(trace_vport);
          XtVaSetValues(trace_vport,XtNfromVert,
                        (XtIsManaged(lyric_t) ? lyric_t:t_box),NULL);
          XtMakeResizeRequest(toplevel,w1,h1+trace_height,&w2,&h2);
          XtVaSetValues(trace_vport,XtNheight,trace_height,NULL);
        }
        toggleMark(w,*id);
      }
      break;
    case ID_HIDETXT:
      XtVaGetValues(toplevel,XtNheight,&h1,XtNwidth,&w1,NULL);
      if(XtIsManaged(lyric_t)) {
        if(ctl->trace_playing) {
          tmp = lyric_height + (XtIsManaged(trace_vport) ? 0:trace_height);
        } else {
          tmp = lyric_height;
        }
        XtUnmanageChild(lyric_t);
        if(ctl->trace_playing && XtIsManaged(trace_vport))
          XtVaSetValues(trace_vport,XtNfromVert,t_box,NULL);
        XtMakeResizeRequest(toplevel,w1,base_height-tmp,&w2,&h2);
      } else {
        XtManageChild(lyric_t);
        if(ctl->trace_playing && XtIsManaged(trace_vport)) {
          XtVaSetValues(trace_vport,XtNfromVert,lyric_t,NULL);
        }
        XtVaSetValues(lyric_t,XtNheight,lyric_height,NULL);
        XtMakeResizeRequest(toplevel,w1,h1+lyric_height,&w2,&h2);
      }
      toggleMark(w,*id);
      break;
    case ID_SAVECONFIG:
      a_saveconfig(dotfile);
      break;
    case ID_SHUFFLE:
      randomAction(NULL,NULL,NULL,NULL);
      break;
    case ID_REPEAT:
      repeatAction(NULL,NULL,NULL,NULL);
      break;
    case ID_OPTIONS:
      optionspopupCB(w,NULL,NULL);
      break;    
    case ID_FILELIST:
      flistpopupCB(w,NULL,NULL);
      break;    
    case ID_ABOUT:
      aboutCB(w,NULL,NULL);
      break;
    case ID_QUIT:
      quitCB(w,NULL,NULL);
      break;    
  }
}

#ifdef WIDGET_IS_LABEL_WIDGET
static void a_print_msg(Widget w)
{
    int i, msglen;
    a_pipe_nread((char *)&msglen, sizeof(int));
    while(msglen > 0)
    {
    i = msglen;
    if(i > sizeof(local_buf)-1)
        i = sizeof(local_buf)-1;
    a_pipe_nread(local_buf, i);
    local_buf[i] = '\0';
    XtVaSetValues(w,XtNlabel,local_buf,NULL);
    msglen -= i;
    }
}
#else
static void a_print_msg(Widget w)
{
    int i, msglen;
    XawTextPosition pos;
    XawTextBlock tb;

    tb.firstPos = 0;
    tb.ptr = local_buf;
    tb.format = FMT8BIT;
    pos = XawTextGetInsertionPoint(w);

    a_pipe_nread((char *)&msglen, sizeof(int));
    while(msglen > 0)
    {
        i = msglen;
        if(i > sizeof(local_buf))
            i = sizeof(local_buf);
        a_pipe_nread(local_buf, i);
        tb.length = i;
        XawTextReplace(w, pos, pos, &tb);
        pos += i;
        XawTextSetInsertionPoint(w, pos);
        msglen -= i;
    }
}
#endif /* WIDGET_IS_LABEL_WIDGET */

#define DELTA_VEL       32

static void ctl_channel_note(int ch, int note, int velocity) {
  
  if(!ctl->trace_playing) return;
  if (velocity == 0) {
    if (note == Panel->cnote[ch])     
      Panel->v_flags[ch] = FLAG_NOTE_OFF;
    Panel->cvel[ch] = 0;
  } else if (velocity > Panel->cvel[ch]) {
    Panel->cvel[ch] = velocity;
    Panel->cnote[ch] = note;
    Panel->ctotal[ch] = velocity * Panel->channel[ch].volume *
      Panel->channel[ch].expression / (127*127);
    Panel->v_flags[ch] = FLAG_NOTE_ON;
  }
}

/*ARGSUSED*/
static void handle_input(XtPointer data,int *source,XtInputId *id) {
  char s[16], c;
  int i=0, n, ch, note;
  float thumb;
  char **pp;

  a_pipe_read(local_buf,sizeof(local_buf));
  switch (local_buf[0]) {
  case 't' :
    curr_time = n = atoi(local_buf+2); i= n % 60; n /= 60;
    sprintf(s, "%d:%02d", n,i);
    XtVaSetValues(tune_l0, XtNlabel, s, NULL);
    if (total_time >0) {
      thumbj = (float)curr_time / (float)total_time;
      if (sizeof(thumbj) > sizeof(XtArgVal)) {
        XtVaSetValues(tune_bar,XtNtopOfThumb,&thumbj,NULL);
      } else {
        XtArgVal *l_thumbj = (XtArgVal *) &thumbj;
        XtVaSetValues(tune_bar,XtNtopOfThumb,*l_thumbj,NULL);
      }
    }
    break;
  case 'T' :
    n= atoi(local_buf+2);
    if(n > 0) {
      total_time = n;
      snprintf(s,sizeof(s), "/%2d:%02d", n/60, n%60);
      XtVaSetValues(tune_l,XtNlabel,s,NULL);
    }
    break;
  case 'E' :
    {
      char *name;
      name=strrchr(local_buf+2,' ');
      n= atoi(local_buf+2);
      if(popup_shell_exist & FLIST_WINDOW)
        XawListHighlight(file_list, n-1);
      if(name==NULL)
        break;
      name++;
      XtVaSetValues(title_mb,XtNlabel,name,NULL);
      snprintf(window_title, sizeof(window_title), "%s : %s", APP_CLASS, local_buf+2);
      XtVaSetValues(toplevel,XtNtitle,window_title,NULL);
      *window_title = '\0';
    }
    break;
  case 'e' :
    if (arrangetitle) {
      char *p= local_buf+2;
      if (!strcmp(p, "(null)")) p = UNTITLED_STR;
      snprintf(window_title, sizeof(window_title), "%s : %s", APP_CLASS, p);
      XtVaSetValues(toplevel,XtNtitle,window_title,NULL);
    }
    snprintf(window_title, sizeof(window_title), "%s", local_buf+2);
    break;
  case 'O' : offPlayButton();break;
  case 'L' :
    a_print_msg(lyric_t);
    break;
  case 'Q' : exit(0);
  case 'V':
    amplitude=atoi(local_buf+2);
    thumb = (float)amplitude / (float)MAXVOLUME;
    sprintf(s, "%d", amplitude);
    XtVaSetValues(vol_l, XtNlabel, s, NULL);
    if (sizeof(thumb) > sizeof(XtArgVal)) {
      XtVaSetValues(vol_bar, XtNtopOfThumb, &thumb, NULL);
    } else {
      XtArgVal *l_thumb = (XtArgVal *) &thumb;
      XtVaSetValues(vol_bar, XtNtopOfThumb,*l_thumb, NULL);
    }
    break;
  case 'v':
    c= *(local_buf+1);
    n= atoi(local_buf+2);
    if(c == 'L')
      voices = n;
    else
      last_voice = n;
    if(IsTracePlaying()) drawVoices();
    break;
  case 'g':
  case '\0' :
    break;
  case 'X':
    n=max_files;
    max_files+=atoi(local_buf+2);
    for (i=n;i<max_files;i++) {
      a_pipe_read(local_buf,sizeof(local_buf));
      addOneFile(max_files,i,local_buf,True);
    }
    if(popup_shell_exist & FLIST_WINDOW){
      Dimension h,w;
      XtVaGetValues(file_list,XtNwidth,&w,NULL);
      XawListChange(file_list,flist,max_files,w,True);
      /* to keep Viewport size */
      XtVaGetValues(file_vport,XtNwidth,&w,XtNheight,&h,NULL);
      XtVaSetValues(file_vport,XtNheight,((h>FILEVPORT_HEIGHT)? h:FILEVPORT_HEIGHT),NULL);
    }
    break;
  case 'Y':
    if(ctl->trace_playing) {
      ch= *(local_buf+1) - 'A';
      c= *(local_buf+2);
      note= (*(local_buf+3)-'0')*100 + (*(local_buf+4)-'0')*10 + *(local_buf+5)-'0';
      n= atoi(local_buf+6);
      if (c == '*' || c == '&') {
        Panel->c_flags[ch] |= FLAG_PROG_ON;
      } else {
        Panel->c_flags[ch] &= ~FLAG_PROG_ON; n= 0;
      }
      ctl_channel_note(ch, note, n);
      draw1Note(ch,note,c);
      draw1Chan(ch,Panel->ctotal[ch],c);
    }
    break;
  case 'I':
    if(IsTracePlaying()) {
      ch= *(local_buf+1) - 'A';
      strncpy(inst_name[ch], (char *)&local_buf[2], INST_NAME_SIZE);
      drawInstname(ch, inst_name[ch]);
    }
    break;
  case 'i':
    if(IsTracePlaying()) {
      ch= *(local_buf+1) - 'A';
      Panel->is_drum[ch]= *(local_buf+2) - 'A';
      drawDrumPart(ch, Panel->is_drum[ch]);
    }
    break;
  case 'P':
    if(IsTracePlaying()) {
      c= *(local_buf+1);
      ch= *(local_buf+2)-'A';
      n= atoi(local_buf+3);
      switch(c) {
      case  'A':        /* panning */
        Panel->channel[ch].panning = n;
        Panel->c_flags[ch] |= FLAG_PAN;
        drawPan(ch,n,True);
        break;
      case  'B':        /* pitch_bend */
        Panel->channel[ch].pitchbend = n;
        Panel->c_flags[ch] |= FLAG_BENDT;
        if (!plane) drawPitch(ch,n);
        break;
      case  'b':        /* tonebank */
        Panel->channel[ch].bank = n;
        if (plane) drawBank(ch,n);
        break;
      case  'r':        /* reverb */
        Panel->reverb[ch] = n;
        if (plane) drawReverb(ch,n);
        break;
      case  'c':        /* chorus */
        Panel->channel[ch].chorus_level = n;
        if (plane) drawChorus(ch,n);
        break;
      case  'S':        /* sustain */
        Panel->channel[ch].sustain = n;
        Panel->c_flags[ch] |= FLAG_SUST;
        break;
      case  'P':        /* program */
        Panel->channel[ch].program = n;
        Panel->c_flags[ch] |= FLAG_PROG;
        drawProg(ch,n,4,pl[plane].ofs[CL_PR],True);
        break;
      case  'E':        /* expression */
        Panel->channel[ch].expression = n;
        ctl_channel_note(ch, Panel->cnote[ch], Panel->cvel[ch]);
        drawExp(ch,n);
        break;
      case  'V':        /* volume */
        Panel->channel[ch].volume = n;
        ctl_channel_note(ch, Panel->cnote[ch], Panel->cvel[ch]);
        drawVol(ch,n);
        break;
      }
    }
    break;
  case 'R':
    redrawTrace(True);  break;
  case 'U':         /* update timer */
    if(ctl->trace_playing) {
      static double last_time = 0;
      double d, t;
      Bool need_flush;
      double delta_time;

      t = get_current_calender_time();
      d = t - last_time;
      if(d > 1)
        d = 1;
      delta_time = d / XAW_UPDATE_TIME;
      last_time = t;
      need_flush = False;
      for(i=0; i<MAX_XAW_MIDI_CHANNELS; i++)
        if (Panel->v_flags[i]) {
          if (Panel->v_flags[i] == FLAG_NOTE_OFF) {
            Panel->ctotal[i] -= DELTA_VEL * delta_time;
            if (Panel->ctotal[i] <= 0) {
              Panel->ctotal[i] = 0;
              Panel->v_flags[i] = 0;
            }
            draw1Chan(i,Panel->ctotal[i],'*');
            need_flush = True;
          } else {
            Panel->v_flags[i] = 0;
          }
        }
      if(need_flush)
        XFlush(XtDisplay(trace));
    }
    break;
  case 'm':
    n= atoi(local_buf+1);
    switch(n) {
    case GM_SYSTEM_MODE:
      sprintf(s,"%d:%02d / GM",total_time/60,total_time%60); break;
    case GS_SYSTEM_MODE:
      sprintf(s,"%d:%02d / GS",total_time/60,total_time%60); break;
    case XG_SYSTEM_MODE:
      sprintf(s,"%d:%02d / XG",total_time/60,total_time%60); break;
    default:
      sprintf(s,"%d:%02d",total_time/60,total_time%60); break;
    }
    XtVaSetValues(time_l,XtNlabel,s,NULL);
    break;
  case 's':
    n= atoi(local_buf+1);
    pp = current_flist;
    if(pp != NULL) {
      while(*pp != NULL) free(*pp++);
      free(current_flist);
    }
    current_flist = (char **)safe_malloc(sizeof(char *) * (n+1));
    if ('\0' != *dotfile) {
      FILE *fp;
      if (savelist) {
        if (NULL != (fp=fopen(dotfile, "a+"))) {
          for(i=0; i<n; i++) {
            a_pipe_read(local_buf,sizeof(local_buf));
            current_flist[i]=(char *)safe_malloc(sizeof(char)*(strlen(local_buf)+1));
            strcpy(current_flist[i], local_buf);
            fprintf(fp,"set %s %s\n",cfg_items[S_MidiFile],current_flist[i]);
          }
          fclose(fp);
        }
      } else
        for(i=0; i<n; i++) a_pipe_read(local_buf,sizeof(local_buf));
    }
    current_flist[n] = NULL;
    break;
  default : 
    fprintf(stderr,"Unkown message '%s' from CONTROL" NLS,local_buf);
  }
}


static int configcmp(char *s, int *num) {
  int i;
  char *p;
  for (i= 0; i < CFGITEMSNUMBER; i++) {
    if (0 == strncasecmp(s, cfg_items[i], strlen(cfg_items[i]))) {
      p = s + strlen(cfg_items[i]);
      while (*p == SPACE || *p == TAB) p++;
      if(i == S_MidiFile)
        *num = p - s;
      else
        *num = atoi(p);
      return i;
    }
  }
  return(-1);
}

static char *strmatch(char *s1, char *s2) {
  char *p = s1;

  while (*p != '\0' && *p == *s2++) p++;
  *p = '\0';
  return(s1);
}

/* Canonicalize by removing /. and /foo/.. if they appear. */
static char *canonicalize_path(char *path)
{
    char *o, *p, *target;
    int abspath;

    o = p = path;
    while(*p)
    {
	if(p[0] == '/' && p[1] == '/')
	    p++;
	else 
	    *o++ = *p++;
    }
    while(path < o-1 && path[o - path - 1] == '/')
	o--;
    path[o - path] = '\0';

    if((p = strchr(path, '/')) == NULL)
	return path;
    abspath = (p == path);

    o = target = p;
    while(*p)
    {
	if(*p != '/')
	    *o++ = *p++;
	else if(p[0] == '/' && p[1] == '.'
		&& (p[2]=='/' || p[2] == '\0'))
	{
	    /* If "/." is the entire filename, keep the "/".  Otherwise,
	       just delete the whole "/.".  */
	    if(o == target && p[2] == '\0')
		*o++ = *p;
	    p += 2;
	}
	else if(p[0] == '/' && p[1] == '.' && p[2] == '.'
		/* `/../' is the "superroot" on certain file systems.  */
		&& o != target
		&& (p[3]=='/' || p[3] == '\0'))
	{
	    while(o != target && (--o) && *o != '/')
		;
	    p += 3;
	    if(o == target && !abspath)
		o = target = p;
	}
	else
	    *o++ = *p++;
    }

    target[o - target] = '\0';
    if(!*path)
	strcpy(path, "/");
    return path;
}

static char *expandDir(char *path, DirPath *full) {
  static char tmp[PATH_MAX];
  static char newfull[PATH_MAX];
  char *p, *tail;

  p = path;
  if (path == NULL) {
    strcpy(tmp, "/");
    full->dirname = tmp;
    full->basename = NULL;
    strcpy(newfull, tmp); return newfull;
  } else if (*p != '~' && NULL == (tail = strrchr(path, '/'))) {
    p = tmp;
    strncpy(p, basepath, PATH_MAX - 1);
    full->dirname = p;
    while (*p++ != '\0') ;
    strncpy(p, path, PATH_MAX - (p - tmp) - 1);
    tmp[PATH_MAX-1] = '\0';
    snprintf(newfull,sizeof(newfull),"%s/%s", basepath, path);
    full->basename = p; return newfull;
  }
  if (*p  == '/') {
    strncpy(tmp, path, PATH_MAX - 1);
  } else {
    if (*p == '~') {
      struct passwd *pw;

      p++;
      if (*p == '/' || *p == '\0') {
        pw = getpwuid(getuid());
      } else {
        char buf[80], *bp = buf;

        while (*p != '/' && *p != '\0') *bp++ = *p++;
        *bp = '\0';
        pw = getpwnam(buf);
      }
      if (pw == NULL) {
        ctl->cmsg(CMSG_WARNING, VERB_NORMAL,
                  "something wrong with getting path."); return NULL;
      }
      while (*p == '/') p++;
      snprintf(tmp, sizeof(tmp), "%s/%s", pw->pw_dir, p);
    } else {    /* *p != '~' */
      snprintf(tmp, sizeof(tmp), "%s/%s", basepath, path);
    }
  }
  p = canonicalize_path(tmp);
  tail = strrchr(p, '/'); *tail++ = '\0';
  full->dirname = p;
  full->basename = tail;
  snprintf(newfull,sizeof(newfull),"%s/%s", p, tail);
  return newfull;
}

/*ARGSUSED*/
static void setDirAction(Widget w,XEvent *e,String *v,Cardinal *n) {
  char *p, *p2;
  struct stat st;
  DirPath full;
  XawListReturnStruct lrs;

  p = XawDialogGetValueString(load_d);
  if (NULL != (p2 = expandDir(p, &full)))
    p = p2;
  if(stat(p, &st) == -1) return;
  if(S_ISDIR(st.st_mode)) {
    strncpy(basepath,p,sizeof(basepath)-1);
    p = strrchr(basepath, '/');
    if (*(p+1) == '\0') *p = '\0';
    lrs.string = "";
    if(dirlist != NULL)
    {
	free(dirlist_top);
	free(dirlist);
	dirlist = NULL;
    }
    setDirList(load_flist, cwd_l, &lrs);
  }
}

/*
 * sort algorithm for DirList:
 * - directories before files
 */
static int dirlist_cmp (const void *p1, const void *p2)
{
    int i1, i2;
    char *s1, *s2;

    s1 = *((char **)p1);
    s2 = *((char **)p2);
    i1 = strlen (s1) - 1;
    i2 = strlen (s2) - 1;
    if (i1 >= 0 && i2 >= 0) {
    if (s1 [i1] == '/' && s2 [i2] != '/')
        return -1;
    if (s1 [i1] != '/' && s2 [i2] == '/')
        return  1;
    }
    return strcmp (s1, s2);
}

#ifndef	ORIGINAL
/* RAKK/HIOENS: Save a string on the heap. Addition for 'common.c' ? */
static  char  * strsav( char  * str ) {
    char  * tp = safe_malloc( strlen(str)+1 );
    strcpy(tp, str);
    return tp;
}
#endif	/* RAKK/HIOENS */

static void setDirList(Widget list, Widget label, XawListReturnStruct *lrs) {
  URL dirp;
  struct stat st;
  char currdir[PATH_MAX], filename[PATH_MAX];
  int i, d_num, f_num;

  snprintf(currdir, sizeof(currdir)-1, "%s/%s", basepath, lrs->string);
  canonicalize_path(currdir);
  if(stat(currdir, &st) == -1) return;
  if(!S_ISDIR(st.st_mode)) {
#ifdef	ORIGINAL
      XtVaSetValues(load_d,XtNvalue,currdir,NULL);
#else	/* RAKK/HIOENS */
      XtVaSetValues(load_d,XtNvalue,strsav(currdir),NULL);
#endif	/* ORIGINAL */
      return;
  }

  if (NULL != (dirp=url_dir_open(currdir))) {
    char *fullpath;
    MBlockList pool;
    StringTable strtab;
    init_mblock(&pool);

    if(dirlist != NULL)
    {
	free(dirlist_top);
	free(dirlist);
    }
    init_string_table(&strtab);
    i = 0; d_num = 0; f_num = 0;
    while (url_gets(dirp, filename, sizeof(filename)) != NULL) {
      fullpath = (char *)new_segment(&pool,strlen(currdir) +strlen(filename) +2);
      sprintf(fullpath, "%s/%s", currdir, filename);
      if(stat(fullpath, &st) == -1) continue;
      if(filename[0] == '.' && filename[1] == '\0') continue;
      if (currdir[0] == '/' && currdir[1] == '\0' && filename[0] == '.'
          && filename[1] == '.' && filename[2] == '\0') continue;
      if(S_ISDIR(st.st_mode)) {
        strcat(filename, "/"); d_num++;
      } else {
        f_num++;
      }
      put_string_table(&strtab, filename, strlen(filename));
      i++;
    }
    dirlist = (String *)make_string_array(&strtab);
    dirlist_top = (String)dirlist[0]; /* Marking for free() */
    qsort (dirlist, i, sizeof (char *), dirlist_cmp);
    snprintf(local_buf, sizeof(local_buf), "%d Directories, %d Files", d_num, f_num);
    XawListChange(list,dirlist,0,0,True);
  }
  else
    strcpy(local_buf, "Can't read directry");

  XtVaSetValues(load_info,XtNlabel,local_buf,NULL);
#ifdef	ORIGINAL
  XtVaSetValues(label,XtNlabel,currdir,NULL);
#else	/* RAKK/HIOENS */
  XtVaSetValues(label,XtNlabel,strsav(currdir),NULL);
#endif	/* RAKK/HIOENS */
  strcpy(basepath, currdir);
  if(currdir[strlen(currdir) - 1] != '/')
      strcat(currdir, "/");
#ifdef	ORIGINAL
  XtVaSetValues(load_d,XtNvalue,currdir,NULL);
#else	/* RAKK/HIOENS */
  XtVaSetValues(load_d,XtNvalue,strsav(currdir),NULL);
#endif	/* RAKK/HIOENS */
}

static int Red_depth,Green_depth,Blue_depth;
static int Red_sft,Green_sft,Blue_sft;
static int bitcount( int d )
{
    int rt=0;
    while( (d & 0x01)==0x01 ){
        d>>=1;
        rt++;
    }
    return(rt);
}
static int sftcount( int *mask )
{
    int rt=0;
    while( (*mask & 0x01)==0 ){
        (*mask)>>=1;
            rt++;
    }
    return(rt);
}
static int getdisplayinfo()
{
    XWindowAttributes xvi;
    XGetWindowAttributes( disp, XtWindow(trace), &xvi );
    if( 16 <= xvi.depth ){
	Red_depth=(xvi.visual)->red_mask;
	Green_depth=(xvi.visual)->green_mask;
	Blue_depth=(xvi.visual)->blue_mask;
	Red_sft=sftcount(&(Red_depth));
	Green_sft=sftcount(&(Green_depth));
	Blue_sft=sftcount(&(Blue_depth));
	Red_depth=bitcount(Red_depth);
	Green_depth=bitcount(Green_depth);
	Blue_depth=bitcount(Blue_depth);
    }
    return(xvi.depth);
}

static void drawBar(int ch,int len, int xofs, int column, Pixel color) {
  static Pixel column1color0;
  static GC gradient_gc[T2COLUMN];
  static Pixmap gradient_pixmap[T2COLUMN];
  static int gradient_set[T2COLUMN];
  static int depth,init=1;
  static XColor x_boxcolor;
  static XGCValues gv;
  int i;
  int col;
  XColor x_color;
  if( init ){
      for(i=0;i<T2COLUMN;i++) gradient_set[i]=0;
      depth=getdisplayinfo();
      if( 16 <= depth && app_resources.gradient_bar != 0 ){
	  x_boxcolor.pixel=boxcolor;
	  XQueryColor(disp,DefaultColormap(disp,0),&x_boxcolor);
	  gv.fill_style = FillTiled;
	  gv.fill_rule = WindingRule;
      }
      init=0;
  }
  if( 16 <= depth && app_resources.gradient_bar != 0 ){
      if( column < T2COLUMN ){
	  col=column;
	  if( column==1 ){
	      if( gradient_set[0]==0 ){
		  column1color0=color;
		  col=0;
	      }
	      else if(gradient_set[1]==0 && column1color0!=color){
		  col=1;
	      }
	      else{
		  if( column1color0==color ) col=0;
		  else col=1;
	      }
	  }
	  if( gradient_set[col]==0 ){
	      unsigned long pxl;
	      gradient_pixmap[col]=XCreatePixmap(disp,XtWindow(trace),BARH2_SPACE[column],1,
						    DefaultDepth(disp,screen));	  
	      x_color.pixel=color;
	      XQueryColor(disp,DefaultColormap(disp,0),&x_color);
	      for(i=0;i<BARH2_SPACE[column];i++){
		  int r,g,b;
		  r=(x_boxcolor.red)+(x_color.red-x_boxcolor.red)*i/BARH2_SPACE[column];
		  g=(x_boxcolor.green)+(x_color.green-x_boxcolor.green)*i/BARH2_SPACE[column];
		  b=(x_boxcolor.blue)+(x_color.blue-x_boxcolor.blue)*i/BARH2_SPACE[column];
		  if(r<0) r=0;
		  if(g<0) g=0;
		  if(b<0) b=0;
		  r >>= 8;
		  g >>= 8;
		  b >>= 8;
		  if(255<r) r=255;
		  if(255<g) g=255;
		  if(255<b) b=255;
		  pxl  = (r>>(8-Red_depth))<<Red_sft;
		  pxl |= (g>>(8-Green_depth))<<Green_sft;
		  pxl |= (b>>(8-Blue_depth))<<Blue_sft;
		  XSetForeground(disp, gct, pxl);
		  XDrawPoint(disp,gradient_pixmap[col],gct,i,0);
	      }
	      gv.tile = gradient_pixmap[col];
	      gradient_gc[col] = XCreateGC( disp,XtWindow(trace),GCFillStyle|GCFillRule|GCTile,&gv );
	      gradient_set[col]=1;
	  }
	  XSetForeground(disp, gct, boxcolor);
	  XFillRectangle(XtDisplay(trace),XtWindow(trace),gct,
			 xofs+len+2,TRACEV_OFS+BAR_SPACE*ch+2,
			 pl[plane].w[column] -len -4,BAR_HEIGHT);
	  gv.ts_x_origin=xofs+2 - BARH2_SPACE[column]+len;
	  XChangeGC(disp,gradient_gc[col],GCTileStipXOrigin,&gv);
	  XFillRectangle(XtDisplay(trace),XtWindow(trace),gradient_gc[col],
			 xofs+2,TRACEV_OFS+BAR_SPACE*ch+2,
			 len,BAR_HEIGHT);
      }
  }
  else{
      /* XSetForeground(disp, gct, bgcolor); */ /* ?? */
      XSetForeground(disp, gct, boxcolor);
      XFillRectangle(XtDisplay(trace),XtWindow(trace),gct,
		     xofs+len+2,TRACEV_OFS+BAR_SPACE*ch+2,
		     pl[plane].w[column] -len -4,BAR_HEIGHT);
      XSetForeground(disp, gct, color);
      XFillRectangle(XtDisplay(trace),XtWindow(trace),gct,
		     xofs+2,TRACEV_OFS+BAR_SPACE*ch+2,
		     len,BAR_HEIGHT);
  }
}

static void drawProg(int ch,int val,int column,int xofs, Boolean do_clean) {
  char s[4];

  if(do_clean) {
    XSetForeground(disp, gct, boxcolor);
    XFillRectangle(XtDisplay(trace),XtWindow(trace),gct,
                   xofs+2,TRACEV_OFS+BAR_SPACE*ch+2,
                   pl[plane].w[4]-4,BAR_HEIGHT);
  }
  XSetForeground(disp, gct, black);
  sprintf(s, "%3d", val);
  XDrawString(XtDisplay(trace), XtWindow(trace), gct,
              xofs+5,TRACEV_OFS+BAR_SPACE*ch+16,s,3);
}

static void drawPan(int ch,int val,Boolean setcolor) {
  int ap,bp;
  int x;
  static XPoint pp[3];

  if (val < 0) return;
  if (setcolor) {
    XSetForeground(disp, gct, boxcolor);
    XFillRectangle(XtDisplay(trace),XtWindow(trace),gct,
                   pl[plane].ofs[CL_PA]+2,TRACEV_OFS+BAR_SPACE*ch+2,
                   pl[plane].w[CL_PA]-4,BAR_HEIGHT);
    XSetForeground(disp, gct, pancolor);
  }
  x= pl[plane].ofs[CL_PA]+3;
  ap= 31 * val/127;
  bp= 31 -ap -1;
  pp[0].x= ap+ x; pp[0].y= 12 +BAR_SPACE*(ch+1);
  pp[1].x= bp+ x; pp[1].y= 8 +BAR_SPACE*(ch+1);
  pp[2].x= bp+ x; pp[2].y= 16 +BAR_SPACE*(ch+1);
  XFillPolygon(XtDisplay(trace),XtWindow(trace),gct,pp,3,
               (int)Nonconvex,(int)CoordModeOrigin);
}

static void draw1Chan(int ch,int val,char cmd) {
  if (cmd == '*' || cmd == '&')
    drawBar(ch, (int)(val*BARSCALE2), pl[plane].ofs[CL_VE], CL_VE, barcol[ch]);
}

static void drawVol(int ch,int val) {
  drawBar(ch, (int)(val*BARSCALE3), pl[plane].ofs[CL_VO], CL_VO, volcolor);
}

static void drawExp(int ch,int val) {
  drawBar(ch, (int)(val*BARSCALE4), pl[plane].ofs[CL_EX], CL_EX, expcolor);
}

static void drawReverb(int ch,int val) {
  drawBar(ch, (int)(val*BARSCALE5), pl[plane].ofs[CL_RE], CL_RE, revcolor);
}

static void drawChorus(int ch,int val) {
  drawBar(ch, (int)(val*BARSCALE5), pl[plane].ofs[CL_CH], CL_CH, chocolor);
}

static void drawPitch(int ch,int val) {
  char s[3];

  XSetForeground(disp, gct, boxcolor);
  XFillRectangle(XtDisplay(trace),XtWindow(trace),gct,
                 pl[plane].ofs[CL_PI]+2,TRACEV_OFS+BAR_SPACE*ch+2,
                 pl[plane].w[CL_PI] -4,BAR_HEIGHT);
  XSetForeground(disp, gct, barcol[9]);
  if (val != 0) {
    if (val<0) {
      sprintf(s, "=");
    } else {
      if (val == 0x2000) sprintf(s, "*");
      else if (val>0x3000) sprintf(s, ">>");
      else if (val>0x2000) sprintf(s, ">");
      else if (val>0x1000) sprintf(s, "<");
      else sprintf(s, "<<");
    }
    XDrawString(XtDisplay(trace), XtWindow(trace), gct,
                pl[plane].ofs[CL_PI]+4,TRACEV_OFS+BAR_SPACE*ch+16,s,strlen(s));
  }
}

static void drawInstname(int ch, char *name) {
  int len;
  if(!ctl->trace_playing) return;
  if(plane!=0) return;

  XSetForeground(disp, gct, boxcolor);
  XFillRectangle(XtDisplay(trace),XtWindow(trace),gct,
                 pl[plane].ofs[CL_IN]+2,TRACEV_OFS+BAR_SPACE*ch+2,
                 pl[plane].w[CL_IN] -4,BAR_HEIGHT);
  XSetForeground(disp, gct, ((Panel->is_drum[ch])? capcolor:black));
  len = strlen(name);
  XDrawString(XtDisplay(trace), XtWindow(trace), gct,
              pl[plane].ofs[CL_IN]+4,TRACEV_OFS+BAR_SPACE*ch+15,
              name,(len>disp_inst_name_len)? disp_inst_name_len:len);
}

static void drawDrumPart(int ch, int is_drum) {
  if(!ctl->trace_playing) return;
  if(plane!=0) return;

  if (is_drum) barcol[ch]=app_resources.drumvelocity_color;
  else         barcol[ch]=app_resources.velocity_color;
}

static void draw1Note(int ch,int note,int flag) {
  int i, j;
  XSegment dot[3];

  j = note -9;
  if (j<0) return;
  if (flag == '*') {
    XSetForeground(disp, gct, playcolor);
  } else if (flag == '&') {
    XSetForeground(disp, gct,
                   ((keyG[j].col == black)? suscolor:barcol[0]));
  }  else {
    XSetForeground(disp, gct, keyG[j].col);
  }
  for(i= 0; i<3; i++) {
    dot[i].x1 = keyG[j].xofs +i;
    dot[i].y1 = TRACEV_OFS+ keyG[j].k[i].y+ ch*BAR_SPACE;
    dot[i].x2 = dot[i].x1;
    dot[i].y2 = dot[i].y1 + keyG[j].k[i].l;
  }
  XDrawSegments(XtDisplay(trace),XtWindow(trace),gct,dot,3);
}

static void drawKeyboardAll(Display *disp,Drawable pix) {
  int i, j;
  XSegment dot[3];

  XSetForeground(disp, gc, tracecolor);
  XFillRectangle(disp,pix,gc,0,0,BARH_OFS8,BAR_SPACE);
  XSetForeground(disp, gc, boxcolor);
  XFillRectangle(disp,pix,gc,BARH_OFS8,0,TRACE_WIDTH-BARH_OFS8+1,BAR_SPACE);
  for(i= 0; i<KEY_NUM; i++) {
    XSetForeground(disp, gc, keyG[i].col);
    for(j= 0; j<3; j++) {
      dot[j].x1 = keyG[i].xofs +j;
      dot[j].y1 = keyG[i].k[j].y;
      dot[j].x2 = dot[j].x1;
      dot[j].y2 = dot[j].y1 + keyG[i].k[j].l;
    }
    XDrawSegments(disp,pix,gc,dot,3);
  }
}

static void drawBank(int ch,int val) {
  char s[4];

  XSetForeground(disp, gct, black);
  sprintf(s, "%3d", (int)val);
  XDrawString(disp,XtWindow(trace),gct,
              pl[plane].ofs[CL_BA],TRACEV_OFS+BAR_SPACE*ch+15, s,strlen(s));
}

#define VOICENUM_WIDTH 56
static void drawVoices(void) {
  XSetForeground(disp, gct, tracecolor);
  XFillRectangle(disp,XtWindow(trace),gct,voices_num_width +4,
                 MAX_XAW_MIDI_CHANNELS*BAR_SPACE+TRACEV_OFS+1,VOICENUM_WIDTH,TRACE_FOOT);  
  sprintf(local_buf, "%3d/%d", last_voice, voices);
  XSetForeground(disp, gct, capcolor);  
  XDrawString(disp, XtWindow(trace),gct,voices_num_width+6,
              MAX_XAW_MIDI_CHANNELS*BAR_SPACE+TRACEV_OFS+16,local_buf,strlen(local_buf));
}

static void drawTitle(char *str) {
  char *p = str;

  if(ctl->trace_playing) {
    if (!strcmp(p, "(null)")) p = UNTITLED_STR;
    XSetForeground(disp, gcs, capcolor);
#ifdef I18N
    XmbDrawString(XtDisplay(trace), XtWindow(trace),ttitlefont,gcs,
                  VOICES_NUM_OFS+TTITLE_OFS,
                  MAX_XAW_MIDI_CHANNELS*BAR_SPACE+TRACEV_OFS+ ttitlefont0->ascent +3,
                  p,strlen(p));
#else
    XDrawString(XtDisplay(trace), XtWindow(trace),gcs,
                VOICES_NUM_OFS+TTITLE_OFS,
                MAX_XAW_MIDI_CHANNELS*BAR_SPACE+TRACEV_OFS+ ttitlefont->ascent +3,
                p,strlen(p));
#endif
    }
}

static void toggletrace(Widget w,XEvent *e,String *v,Cardinal *n) {
  if((e->xbutton.button == 1) || e->type == KeyPress) {
    plane ^= 1;
    redrawTrace(True);
  }
}

/*ARGSUSED*/
static void exchgWidth(Widget w,XEvent *e,String *v,Cardinal *n) {
  Dimension w1,h1,w2,h2;

  XtVaGetValues(toplevel,XtNheight,&h1,NULL);
  ++currwidth;
  currwidth %= 3;       /* number of rotatewidth */
  w1 = rotatewidth[currwidth];
  XtMakeResizeRequest(toplevel,w1,h1,&w2,&h2);
  resizeAction(w,NULL,NULL,NULL);  
}

/*ARGSUSED*/
static void redrawAction(Widget w,XEvent *e,String *v,Cardinal *n) {
  if(e->xexpose.count == 0)
    redrawTrace(True);
}

/*ARGSUSED*/
static Boolean cursor_is_in = False;
static void redrawCaption(Widget w,XEvent *e,String *v,Cardinal *n) {
  char *p;
  int i;

  if(e->type == EnterNotify) {
    cursor_is_in = True;
    XSetForeground(disp, gct, capcolor);
  } else {
    cursor_is_in = False;
    XSetForeground(disp, gct, tracecolor);
  }
  XFillRectangle(disp,XtWindow(trace),gct, 0,0,TRACE_WIDTH,TRACEV_OFS);
  XSetBackground(disp, gct, (e->type == EnterNotify)? expcolor:tracecolor);
  XSetForeground(disp, gct, (e->type == EnterNotify)? tracecolor:capcolor);
  for(i=0; i<pl[plane].col; i++) {
    p = pl[plane].cap[i];
    XDrawString(disp,XtWindow(trace),gct,pl[plane].ofs[i]+4,16,p,strlen(p));
  }
}

static void redrawTrace(Boolean draw) {
  int i;
  /* Dimension w1, h1; */
  char s[3];

  if(!ctl->trace_playing) return;
  if(!XtIsRealized(trace)) return;

  /* XtVaGetValues(trace,XtNheight,&h1,XtNwidth,&w1,NULL); */
  /* XSetForeground(disp, gct, tracecolor); */
  /* XFillRectangle(disp,XtWindow(trace),gct, 0,0,w1,h1); */
  /* XSetForeground(disp, gct, boxcolor); */
  /* XFillRectangle(disp,XtWindow(trace),gct,
                 BARH_OFS8 -1,TRACEV_OFS, TRACE_WIDTH-BARH_OFS8+1,
                 BAR_SPACE*MAX_XAW_MIDI_CHANNELS); */
  for(i= 0; i<MAX_XAW_MIDI_CHANNELS; i++) {
    XGCValues gv;
    gv.tile = layer[plane];
    gv.ts_x_origin = 0;
    gv.ts_y_origin = TRACEV_OFS+i*BAR_SPACE;
    XChangeGC(disp,gc_xcopy,GCTile|GCTileStipXOrigin|GCTileStipYOrigin,&gv);
    XFillRectangle(disp,XtWindow(trace),gc_xcopy,
		   0, TRACEV_OFS+i*BAR_SPACE, TRACE_WIDTH,BAR_SPACE);
    /* XCopyArea(disp, layer[plane], XtWindow(trace), gct, 0,0,
              TRACE_WIDTH,BAR_SPACE, 0, TRACEV_OFS+i*BAR_SPACE); */
  }
  XSetForeground(disp, gct, capcolor);
  XDrawLine(disp,XtWindow(trace),gct,BARH_OFS0,TRACEV_OFS+BAR_SPACE*MAX_XAW_MIDI_CHANNELS,
            TRACE_WIDTH-1,TRACEV_OFS+BAR_SPACE*MAX_XAW_MIDI_CHANNELS);

  XSetForeground(disp, gct, black);
  for(i= 1; i<MAX_XAW_MIDI_CHANNELS+1; i++) {
    sprintf(s, "%2d", i);
    XDrawString(disp, XtWindow(trace), gct,
                pl[plane].ofs[CL_C]+2,TRACEV_OFS+BAR_SPACE*i-5,s,2);
  }

  if(cursor_is_in) {
    XSetForeground(disp, gct, capcolor);
    XFillRectangle(disp,XtWindow(trace),gct, 0,0,TRACE_WIDTH,TRACEV_OFS);
  }
  XSetForeground(disp, gct, (cursor_is_in)? tracecolor:capcolor);
  for(i=0; i<pl[plane].col; i++) {
    char *p;
    p = pl[plane].cap[i];
    XDrawString(disp,XtWindow(trace),gct,pl[plane].ofs[i]+4,16,p,strlen(p));
  }
  XSetForeground(disp, gct, tracecolor);
  XFillRectangle(disp,XtWindow(trace),gct,0,MAX_XAW_MIDI_CHANNELS*BAR_SPACE+TRACEV_OFS+1,
                 TRACE_WIDTH,TRACE_FOOT);
  XSetForeground(disp, gct, capcolor);  
  XDrawString(disp, XtWindow(trace),gct,VOICES_NUM_OFS,
              MAX_XAW_MIDI_CHANNELS*BAR_SPACE+TRACEV_OFS+16,"Voices",6);
  drawVoices();
  drawTitle(window_title);
  if(draw) {
    for(i=0; i<MAX_XAW_MIDI_CHANNELS; i++)
      if (Panel->ctotal[i] != 0 && Panel->c_flags[i] & FLAG_PROG_ON)
        draw1Chan(i,Panel->ctotal[i],'*');
    XSetForeground(disp, gct, pancolor);
    for(i=0; i<MAX_XAW_MIDI_CHANNELS; i++) {
      if (Panel->c_flags[i] & FLAG_PAN)
        drawPan(i,Panel->channel[i].panning,False);
    }
    XSetForeground(disp, gct, black);
    for(i=0; i<MAX_XAW_MIDI_CHANNELS; i++) {
      drawProg(i,Panel->channel[i].program,CL_PR,pl[plane].ofs[4],False);
      drawVol(i,Panel->channel[i].volume);
      drawExp(i,Panel->channel[i].expression);
      if (plane) {
        drawBank(i,Panel->channel[i].bank);
        drawReverb(i,Panel->reverb[i]);
        drawChorus(i,Panel->channel[i].chorus_level);
      } else {
        drawPitch(i,Panel->channel[i].pitchbend);
        drawInstname(i, inst_name[i]);
      }
    }
  }
}

static void initStatus(void) {
  int i;

  if(!ctl->trace_playing) return;
  for(i=0; i<MAX_XAW_MIDI_CHANNELS; i++) {
    Panel->channel[i].program= 0;
    Panel->channel[i].volume= 0;
    Panel->channel[i].sustain= 0;
    Panel->channel[i].expression= 0;
    Panel->channel[i].pitchbend= 0;
    Panel->channel[i].panning= -1;
    Panel->ctotal[i] = 0;
    Panel->cvel[i] = 0;
    Panel->bank[i] = 0;
    Panel->reverb[i] = 0;
    Panel->channel[i].chorus_level = 0;
    Panel->v_flags[i] = 0;
    Panel->c_flags[i] = 0;
    Panel->is_drum[i] = 0;
    *inst_name[i] = '\0';
  }
  last_voice = 0;
}

/*ARGSUSED*/
static void completeDir(Widget w,XEvent *e, XtPointer data)
{
  char *p;
  DirPath full;

  p = XawDialogGetValueString(load_d);
  if (!expandDir(p, &full))
    ctl->cmsg(CMSG_WARNING,VERB_NORMAL,"something wrong with getting path.");
  if(full.basename != NULL) {
    int len, match = 0;
    char filename[PATH_MAX], matchstr[PATH_MAX],*path = "/";
    char *fullpath;
    URL dirp;

    len = strlen(full.basename);
    if(strlen(full.dirname)) path = full.dirname;
    if (NULL != (dirp=url_dir_open(path))) {
      MBlockList pool;
      init_mblock(&pool);

      while (url_gets(dirp, filename, sizeof(filename)) != NULL) {
        if (!strncmp(full.basename, filename, len)) {
          struct stat st;

          fullpath = (char *)new_segment(&pool,
                            strlen(full.dirname) + strlen(filename) + 2);
          sprintf(fullpath, "%s/%s", full.dirname, filename);

          if (stat(fullpath, &st) == -1)
            continue;
          if (!match)
            strncpy(matchstr, filename, PATH_MAX - 1);
          else
            strmatch(matchstr, filename);
          match++;
          if(S_ISDIR(st.st_mode) && (!strcmp(filename,full.basename))) {
            strncpy(matchstr, filename, PATH_MAX - 1);
            strncat(matchstr, "/", PATH_MAX - strlen(matchstr) - 1);
            match = 1;
            break;
          }
        }
      }

      if (match) {
        sprintf(filename, "%s/%s", full.dirname, matchstr);
        XtVaSetValues(load_d,XtNvalue,filename,NULL);
      }
      url_close(dirp);
      reuse_mblock(&pool);
    }
  }
}

static char **dotfile_flist;
static int dot_nfile = 0;
#define SSIZE 256

static void a_readconfig (Config *Cfg) {
  char s[SSIZE];
  char *home, c = ' ', *p;
  FILE *fp;
  int i, k;

  if (NULL == (home=getenv("HOME"))) home=getenv("home");
  if (home != NULL) {
    dotfile = (char *)XtMalloc(sizeof(char) * PATH_MAX);
    snprintf(dotfile, PATH_MAX, "%s/%s", home, INITIAL_CONFIG);
    if (NULL != (fp=fopen(dotfile, "r"))) {
      dotfile_flist = (char **)safe_malloc(sizeof(char *) * INIT_FLISTNUM);
      while (c != EOF) {
        p = s;
        for(i=0;;i++) {
          c = fgetc(fp);
          if (c == LF || c == EOF || i > SSIZE) break;
          *p++ = c;
        }
        *p = (char)NULL;
        if (0 != strncasecmp(s, "set ", 4)) continue;
        switch (configcmp(s+4, &k)) {
        case S_RepeatPlay:
          Cfg->repeat = (Boolean)k; break;
        case S_AutoStart:
          Cfg->autostart = (Boolean)k; break;
        case S_AutoExit:
          Cfg->autoexit = (Boolean)k; break;
        case S_DispText:
          Cfg->hidetext = (Boolean)(k ^ 1);
          break;
        case S_ShufflePlay:
          Cfg->shuffle = (Boolean)k; break;
        case S_DispTrace:
          Cfg->disptrace = (Boolean)k; break;
        case S_CurVol:
          Cfg->amplitude = (int)k; break;
        case S_ExtOptions:
          Cfg->extendopt = (int)k; break;
        case S_ChorusOption:
          Cfg->chorusopt = (int)k; break;
        case S_MidiFile:
          if(savelist) {
            p = s+k+4;
            if(dot_nfile < INIT_FLISTNUM)
              if(IsEffectiveFile(p)) {
                dotfile_flist[dot_nfile]
                  = (char *)safe_malloc(sizeof(char)*(strlen(p) +1));
                strcpy(dotfile_flist[dot_nfile],p);
                dotfile_flist[++dot_nfile] = NULL;
              }
          }
          break;
        }
      }

      fclose(fp);
    }
  }
}

static void a_saveconfig (char *file) {
  FILE *fp;
  Boolean s1, s2;
  int i,flags, cflag;

  if ('\0' != *file) {
    if (NULL != (fp=fopen(file, "w"))) {
      XtVaGetValues(repeat_b,XtNstate,&s1,NULL);
      XtVaGetValues(random_b,XtNstate,&s2,NULL);
      fprintf(fp,"set %s %d\n",cfg_items[S_ConfirmExit],(int)Cfg.confirmexit);
      fprintf(fp,"set %s %d\n",cfg_items[S_RepeatPlay],(int)s1);
      fprintf(fp,"set %s %d\n",cfg_items[S_AutoStart],(int)file_menu[ID_AUTOSTART-100].bmflag);
      fprintf(fp,"set %s %d\n",cfg_items[S_DispText],(int)(file_menu[ID_HIDETXT-100].bmflag ^ TRUE));
      fprintf(fp,"set %s %d\n",cfg_items[S_ShufflePlay],(int)s2);
      fprintf(fp,"set %s %d\n",cfg_items[S_DispTrace],((int)file_menu[ID_HIDETRACE-100].bmflag ? 0:1));
      fprintf(fp,"set %s %d\n",cfg_items[S_CurVol],amplitude);
      fprintf(fp,"set %s %d\n",cfg_items[S_AutoExit],(int)file_menu[ID_AUTOQUIT-100].bmflag);
      if(popup_shell_exist & OPTIONS_WINDOW) {
        flags = 0; cflag = 0;
        for(i=0; i<MAX_OPTION_N; i++) {
          XtVaGetValues(option_num[i].widget,XtNstate,&s1,NULL);
          flags |= ((s1)? option_num[i].bit:0);
        }
        XtVaGetValues(chorus_b,XtNstate,&s1,NULL);
        if(s1) cflag = (init_chorus)? init_chorus:1;
      } else {
        flags = init_options; cflag = init_chorus;
      }
      fprintf(fp,"set %s %d\n",cfg_items[S_ExtOptions],flags);
      fprintf(fp,"set %s %d\n",cfg_items[S_ChorusOption],cflag);
      fclose(fp);
      a_pipe_write("s");
    } else {
      fprintf(stderr, "cannot open initializing file '%s'.\n", file);
    }
  }
}

#ifdef OFFIX
static void FileDropedHandler(Widget widget ,XtPointer data,XEvent *event,Boolean *b)
{
  char *filename;
  unsigned char *Data;
  unsigned long Size;
  char local_buffer[PATH_MAX];
  int i;
  static const int AcceptType[]={DndFile,DndFiles,DndDir,DndLink,DndExe,DndURL,
                 DndNotDnd};
  int Type;
  Type=DndDataType(event);
  for(i=0;AcceptType[i]!=DndNotDnd;i++){
    if(AcceptType[i]==Type)
      goto OK;
  }
  fprintf(stderr,"NOT ACCEPT\n");
  /*Not Acceptable,so Do Nothing*/
  return;
OK:
  DndGetData(&Data,&Size);
  if(Type==DndFiles){
    filename = Data;
    while (filename[0] != '\0'){
      snprintf(local_buffer,sizeof(local_buffer),"X %s\n",filename);
      a_pipe_write(local_buffer);
      filename = filename + strlen(filename) + 1;
    }       
  }
  else{
    snprintf(local_buffer,sizeof(local_buffer),"X %s%s\n",Data,(Type==DndDir)?"/":"");
    a_pipe_write(local_buffer);
  }
  return;
}
#endif

/*ARGSUSED*/
static void leaveSubmenu(Widget w, XEvent *e, String *v, Cardinal *n) {
  XLeaveWindowEvent *leave_event = (XLeaveWindowEvent *)e;
  Dimension height;

  XtVaGetValues(w,XtNheight,&height,NULL);
  if (leave_event->x <= 0 || leave_event->y <= 0 || leave_event->y >= height)
    XtPopdown(w);
}

/*ARGSUSED*/
static void checkRightAndPopupSubmenu(Widget w, XEvent *e, String *v, Cardinal *n) {
  XLeaveWindowEvent *leave_ev = (XLeaveWindowEvent *)e;
  Dimension nheight,height,width;
  Position x,y;
  int i;

  if(!maxentry_on_a_menu) return;

  if(e == NULL)
    i= *(int *)n;
  else
    i= atoi(*v);
  if(w != title_sm) {
    if(leave_ev->x <= 0 || leave_ev->y < 0) {
        XtPopdown(w); return;
    }
  } else {
    if(leave_ev->x <= 0 || leave_ev->y <= 0) return;
  }
  if(psmenu[i] == NULL) return;
  XtVaGetValues(psmenu[i],XtNheight,&height,NULL);
  
  /* neighbor menu height */
  XtVaGetValues((i>0)? psmenu[i-1]:title_sm,
                XtNwidth,&width,XtNheight,&nheight,XtNx,&x,XtNy,&y,NULL);
  if(leave_ev->x > 0 && leave_ev->y > nheight - 22) {
    XtVaSetValues(psmenu[i],XtNx,x+80,NULL);
    XtPopup(psmenu[i],XtGrabNone);  
    XtVaGetValues(psmenu[i],XtNheight,&height,NULL);
    XtVaSetValues(psmenu[i],XtNy,y +((height)? nheight-height:0),NULL);
  }
}

/*ARGSUSED*/
static void popdownSubmenuCB(Widget w,XtPointer data,XtPointer dummy) {
  int i = (int)data;

  if (i < 0) i = submenu_n -1;
  while(i >= 0) XtPopdown(psmenu[i--]);
}

/*ARGSUSED*/
static void popdownSubmenu(Widget w, XEvent *e, String *v, Cardinal *n) {
  int i = atoi(*v);

  while(i >= 0) XtPopdown(psmenu[i--]);
}

/*ARGSUSED*/
static void closeParent(Widget w, XEvent *e, String *v, Cardinal *n) {
  XtPopdown(XtParent(w));
}

/*ARGSUSED*/
static void flistMove(Widget w, XEvent *e, String *v, Cardinal *n) {
  int i = atoi(*v);
  XawListReturnStruct *lr;

  if(!max_files) return;
  lr= XawListShowCurrent(file_list);
  if(popup_shell_exist & FLIST_WINDOW) {
    if(lr != NULL && lr->list_index != XAW_LIST_NONE) {
      i += atoi(lr->string) -1;
      if(i<0) i= 0;
      if(i>=max_files) i= max_files -1;
      XawListHighlight(file_list, i);
    } else {
      XawListHighlight(file_list, (i>0)? max_files-1:0);
    }
  }
}

static int max_num = INIT_FLISTNUM;

static void addFlist(char *fname, int current_n) {
  char *p;

  if(max_num < current_n+1) {
    max_num += 64;
    flist = (String *)safe_realloc(flist,(max_num+1)*sizeof(String));
  }
  p = (char *)safe_malloc(sizeof(char) * (strlen(fname) +1));
  flist[current_n]= strcpy(p, fname);
  flist[current_n+1]= NULL;
}

static void addOneFile(int max_files,int curr_num,char *fname,Boolean update_flist) {
  static Dimension tmpi;
  static int menu_height;
  static Widget tmpw;
  static int j;
  char sbuf[256];

  if(curr_num == 0) {
    j = 0; tmpi = 0; menu_height = 0; tmpw = title_sm;
  }
  if(menu_height + tmpi*2 > root_height) {
    if(!maxentry_on_a_menu) {
      maxentry_on_a_menu = j = curr_num;
      XtAddCallback(title_sm,XtNpopdownCallback,popdownSubmenuCB,(XtPointer)(submenu_n -1));
    }
    if(j >= maxentry_on_a_menu) {
      if (psmenu == NULL)
        psmenu = (Widget *)safe_malloc(sizeof(Widget) * ((int)(max_files / curr_num)+ 2));
      else
        psmenu = (Widget *)safe_realloc((char *)psmenu, sizeof(Widget)*(submenu_n + 2));
      sprintf(sbuf, "morebox%d", submenu_n);
      pbox=XtVaCreateManagedWidget(sbuf,smeBSBObjectClass,tmpw,XtNlabel,"  More...",
                                   XtNbackground,textbgcolor,XtNforeground,capcolor,
                                   XtNrightBitmap,arrow_mark,
                                   XtNfont,app_resources.label_font, NULL);
      snprintf(sbuf,sizeof(sbuf),
              "<LeaveWindow>: unhighlight() checkRightAndPopupSubmenu(%d)",submenu_n);
      XtOverrideTranslations(tmpw, XtParseTranslationTable(sbuf));

      sprintf(sbuf, "psmenu%d", submenu_n);
      psmenu[submenu_n] = XtVaCreatePopupShell(sbuf,simpleMenuWidgetClass,title_sm,
                                   XtNforeground,textcolor, XtNbackground,textbgcolor,
                                   XtNbackingStore,NotUseful,XtNsaveUnder,False,
                                   XtNwidth,menu_width,
                                   NULL);
      snprintf(sbuf,sizeof(sbuf), "<BtnUp>: popdownSubmenu(%d) notify() unhighlight()\n\
        <EnterWindow>: unhighlight()\n\
        <LeaveWindow>: leaveSubmenu(%d) unhighlight()",submenu_n,submenu_n);
      XtOverrideTranslations(psmenu[submenu_n],XtParseTranslationTable(sbuf));
      tmpw = psmenu[submenu_n++]; psmenu[submenu_n] = NULL;
      j = 0;
    }
  }
  if(maxentry_on_a_menu) j++;
  bsb=XtVaCreateManagedWidget(fname,smeBSBObjectClass,tmpw,NULL);
  XtAddCallback(bsb,XtNcallback,menuCB,(XtPointer)curr_num);
  XtVaGetValues(bsb, XtNheight, &tmpi, NULL);
  if(!maxentry_on_a_menu)
    menu_height += tmpi;
  else
    psmenu[submenu_n] = NULL;
  if(update_flist)
    addFlist(fname, curr_num);
}

static void createOptions(void) {
  if(!(popup_shell_exist & OPTIONS_WINDOW)) {
    popup_opt= XtVaCreatePopupShell("popup_option",transientShellWidgetClass,
                                  toplevel,NULL);
    popup_optbox= XtVaCreateManagedWidget("popup_optbox",boxWidgetClass,popup_opt,
                                  XtNorientation,XtorientVertical,
                                  XtNwidth,220,XtNheight,280,
                                  XtNbackground,bgcolor,NULL);
    modul_bb=XtVaCreateManagedWidget("modul_box",boxWidgetClass,popup_optbox,
                                  XtNorientation,XtorientHorizontal,
                                  XtNforeground,boxcolor, XtNbackground,bgcolor,
                                  XtNborderWidth,0,NULL);
    modul_b=XtVaCreateManagedWidget("modul_button",toggleWidgetClass,modul_bb,
                                  XtNbitmap,off_mark,
                                  XtNforeground,togglecolor, XtNbackground,buttonbgcolor,
                                  NULL);
    modul_l=XtVaCreateManagedWidget("modul_lbl",labelWidgetClass,modul_bb,
                                  XtNforeground,textcolor, XtNbackground,bgcolor,
                                  XtNborderWidth,0,NULL);
    porta_bb=XtVaCreateManagedWidget("porta_box",boxWidgetClass,popup_optbox,
                                  XtNorientation,XtorientHorizontal,
                                  XtNforeground,boxcolor, XtNbackground,bgcolor,
                                  XtNborderWidth,0,NULL);
    porta_b=XtVaCreateManagedWidget("porta_button",toggleWidgetClass,porta_bb,
                                  XtNbitmap,off_mark,XtNfromVert,modul_b,
                                  XtNforeground,togglecolor, XtNbackground,buttonbgcolor,
                                  NULL);
    porta_l=XtVaCreateManagedWidget("porta_lbl",labelWidgetClass,porta_bb,
                                  XtNforeground,textcolor, XtNbackground,bgcolor,
                                  XtNborderWidth,0,NULL);
    nrpnv_bb=XtVaCreateManagedWidget("nrpnv_box",boxWidgetClass,popup_optbox,
                                  XtNorientation,XtorientHorizontal,
                                  XtNforeground,boxcolor, XtNbackground,bgcolor,
                                  XtNborderWidth,0,NULL);
    nrpnv_b=XtVaCreateManagedWidget("nrpnv_button",toggleWidgetClass,nrpnv_bb,
                                  XtNbitmap,off_mark,XtNfromVert,porta_b,
                                  XtNforeground,togglecolor, XtNbackground,buttonbgcolor,
                                  NULL);
    nrpnv_l=XtVaCreateManagedWidget("nrpnv_lbl",labelWidgetClass,nrpnv_bb,
                                  XtNforeground,textcolor, XtNbackground,bgcolor,
                                  XtNborderWidth,0,NULL);
    reverb_bb=XtVaCreateManagedWidget("reverb_box",boxWidgetClass,popup_optbox,
                                  XtNorientation,XtorientHorizontal,
                                  XtNforeground,boxcolor, XtNbackground,bgcolor,
                                  XtNborderWidth,0,NULL);
    reverb_b=XtVaCreateManagedWidget("reverb_button",toggleWidgetClass,reverb_bb,
                                  XtNbitmap,off_mark,XtNfromVert,nrpnv_b,
                                  XtNforeground,togglecolor, XtNbackground,buttonbgcolor,
                                  NULL);
    reverb_l=XtVaCreateManagedWidget("reverb_lbl",labelWidgetClass,reverb_bb,
                                  XtNforeground,textcolor, XtNbackground,bgcolor,
                                  XtNborderWidth,0,NULL);
    chorus_bb=XtVaCreateManagedWidget("chorus_box",boxWidgetClass,popup_optbox,
                                  XtNorientation,XtorientHorizontal,
                                  XtNforeground,boxcolor, XtNbackground,bgcolor,
                                  XtNborderWidth,0,NULL);
    chorus_b=XtVaCreateManagedWidget("chorus_button",toggleWidgetClass,chorus_bb,
                                  XtNbitmap,off_mark,XtNfromVert,reverb_b,
                                  XtNforeground,togglecolor, XtNbackground,buttonbgcolor,
                                  NULL);
    chorus_l=XtVaCreateManagedWidget("chorus_lbl",labelWidgetClass,chorus_bb,
                                  XtNforeground,textcolor, XtNbackground,bgcolor,
                                  XtNborderWidth,0,NULL);
    chpressure_bb=XtVaCreateManagedWidget("chpressure_box",boxWidgetClass,popup_optbox,
                                  XtNorientation,XtorientHorizontal,XtNfromVert,chorus_b,
                                  XtNforeground,boxcolor, XtNbackground,bgcolor,
                                  XtNborderWidth,0,NULL);
    chpressure_b=XtVaCreateManagedWidget("chpressure_button",toggleWidgetClass,chpressure_bb,
                                  XtNbitmap,off_mark,XtNfromVert,chorus_b,
                                  XtNforeground,togglecolor, XtNbackground,buttonbgcolor,
                                  NULL);
    chpressure_l=XtVaCreateManagedWidget("chpressure_lbl",labelWidgetClass,chpressure_bb,
                                  XtNforeground,textcolor, XtNbackground,bgcolor,
                                  XtNborderWidth,0,NULL);
    overlapv_bb=XtVaCreateManagedWidget("overlapvoice_box",boxWidgetClass,popup_optbox,
                                  XtNorientation,XtorientHorizontal,XtNfromVert,chpressure_b,
                                  XtNforeground,boxcolor, XtNbackground,bgcolor,
                                  XtNborderWidth,0,NULL);
    overlapv_b=XtVaCreateManagedWidget("overlapvoice_button",toggleWidgetClass,overlapv_bb,
                                  XtNbitmap,off_mark,XtNfromVert,chpressure_b,
                                  XtNforeground,togglecolor, XtNbackground,buttonbgcolor,
                                  NULL);
    overlapv_l=XtVaCreateManagedWidget("overlapv_lbl",labelWidgetClass,overlapv_bb,
                                  XtNforeground,textcolor, XtNbackground,bgcolor,
                                  XtNborderWidth,0,NULL);
    txtmeta_bb=XtVaCreateManagedWidget("txtmeta_box",boxWidgetClass,popup_optbox,
                                  XtNorientation,XtorientHorizontal,XtNfromVert,overlapv_b,
                                  XtNforeground,boxcolor, XtNbackground,bgcolor,
                                  XtNborderWidth,0,NULL);
    txtmeta_b=XtVaCreateManagedWidget("txtmeta_button",toggleWidgetClass,txtmeta_bb,
                                  XtNbitmap,off_mark,XtNfromVert,overlapv_b,
                                  XtNforeground,togglecolor, XtNbackground,buttonbgcolor,
                                  NULL);
    txtmeta_l=XtVaCreateManagedWidget("txtmeta_lbl",labelWidgetClass,txtmeta_bb,
                                  XtNforeground,textcolor, XtNbackground,bgcolor,
                                  XtNborderWidth,0,NULL);
    popup_oclose= XtVaCreateManagedWidget("closebutton",
                                  commandWidgetClass, popup_optbox,
                                  XtNfromVert,txtmeta_b,
                                  XtNwidth,80,XtNresize,False,NULL);
    XtAddCallback(popup_opt,XtNdestroyCallback,optionsdestroyCB,NULL);
    XtAddCallback(popup_oclose,XtNcallback,optionscloseCB,NULL);
    XtAddCallback(modul_b,XtNcallback,optionsCB,(XtPointer)&option_num[MODUL_N].id);
    XtAddCallback(porta_b,XtNcallback,optionsCB,(XtPointer)&option_num[PORTA_N].id);
    XtAddCallback(nrpnv_b,XtNcallback,optionsCB,(XtPointer)&option_num[NRPNV_N].id);
    XtAddCallback(reverb_b,XtNcallback,optionsCB,(XtPointer)&option_num[REVERB_N].id);
    XtAddCallback(chpressure_b,XtNcallback,optionsCB,(XtPointer)&option_num[CHPRESSURE_N].id);
    XtAddCallback(overlapv_b,XtNcallback,optionsCB,(XtPointer)&option_num[OVERLAPV_N].id);
    XtAddCallback(txtmeta_b,XtNcallback,optionsCB,(XtPointer)&option_num[TXTMETA_N].id);
    XtAddCallback(chorus_b,XtNcallback,chorusCB,NULL);
    option_num[MODUL_N].widget= modul_b;
    option_num[PORTA_N].widget= porta_b;
    option_num[NRPNV_N].widget= nrpnv_b;
    option_num[REVERB_N].widget= reverb_b;
    option_num[CHPRESSURE_N].widget= chpressure_b;
    option_num[OVERLAPV_N].widget= overlapv_b;
    option_num[TXTMETA_N].widget= txtmeta_b;
    XtSetKeyboardFocus(popup_opt, popup_optbox);
    if(init_options) {
      int i;
      for(i=0; i<MAX_OPTION_N; i++) {
        if(init_options & option_num[i].bit)
          XtVaSetValues(option_num[i].widget,XtNstate,True,XtNbitmap,on_mark,NULL);
      }
    }
    if(init_chorus)
      XtVaSetValues(chorus_b,XtNstate,True,XtNbitmap,on_mark,NULL);
    popup_shell_exist |= OPTIONS_WINDOW;
  }
}

/*ARGSUSED*/
static void fselectCB(Widget w,XtPointer data,XtPointer call_data) {
  XawListReturnStruct *lr = XawListShowCurrent(file_list);

  if(lr != NULL && lr->list_index != XAW_LIST_NONE) {
    onPlayOffPause();
    sprintf(local_buf,"L %d",atoi(lr->string));
    a_pipe_write(local_buf);
  }
}

/*ARGSUSED*/
static void fdeleteCB(Widget w,XtPointer data,XtPointer call_data) {
  XawListReturnStruct *lr = XawListShowCurrent(file_list);
  int i,n;
  char *p;

  if(lr == NULL || lr->list_index == XAW_LIST_NONE) return;
  stopCB(w,NULL,NULL);
  if(max_files==1) {
    fdelallCB(w,NULL,NULL); return;
  }
  n = atoi(lr->string) -1;
  sprintf(local_buf,"d %d",n);
  a_pipe_write(local_buf);
  --max_files;
  free(flist[n]);
  for(i= n; i<max_files; i++) {
    p= strchr(flist[i+1],'.');
    snprintf(local_buf,sizeof(local_buf),"%d%s",i+1,p);
    strncpy(flist[i+1],local_buf,strlen(flist[i+1]));
    flist[i]= flist[i+1];
  }
  flist[max_files]= NULL;
  if(popup_shell_exist & FLIST_WINDOW) {
    Dimension h,w;
    XawListChange(file_list,flist,max_files,360,True);
    /* keep Viewport size */
    XtVaGetValues(file_vport,XtNwidth,&w,XtNheight,&h,NULL);
    XtVaSetValues(file_vport,XtNheight,((h>FILEVPORT_HEIGHT)? h:FILEVPORT_HEIGHT),NULL);
    if(n>=max_files) --n;
    XawListHighlight(file_list,n);
  }
  if(psmenu != NULL) free(psmenu);
  XtDestroyWidget(title_sm); psmenu = NULL;
  maxentry_on_a_menu = 0,submenu_n = 0;
  title_sm=XtVaCreatePopupShell("title_simplemenu",simpleMenuWidgetClass,title_mb,
                                XtNforeground,textcolor, XtNbackground,textbgcolor,
                                XtNbackingStore,NotUseful, XtNsaveUnder,False, NULL);
  for(i= 0; i<max_files; i++)
    addOneFile(max_files,i,flist[i],False);
}

/*ARGSUSED*/
static void fdelallCB(Widget w,XtPointer data,XtPointer call_data) {
  int i;

  stopCB(w,NULL,NULL);
  a_pipe_write("A");
  for(i=0; i<max_files; i++) free(flist[i]);
  max_files= 0;
  flist[0]= NULL;
  if(popup_shell_exist & FLIST_WINDOW) {
    Dimension h,w;
    XawListChange(file_list,NULL,max_files,360,True);
    /* keep Viewport size */
    XtVaGetValues(file_vport,XtNwidth,&w,XtNheight,&h,NULL);
    XtVaSetValues(file_vport,XtNheight,((h>FILEVPORT_HEIGHT)? h:FILEVPORT_HEIGHT),NULL);
  }
  if(psmenu != NULL) free(psmenu);
  XtDestroyWidget(title_sm); psmenu = NULL;
  maxentry_on_a_menu = 0,submenu_n = 0;
  title_sm=XtVaCreatePopupShell("title_simplemenu",simpleMenuWidgetClass,title_mb,
                                XtNforeground,textcolor, XtNbackground,textbgcolor,
                                XtNbackingStore,NotUseful, XtNsaveUnder,False, NULL);
  bsb=XtVaCreateManagedWidget("dummyfile",smeLineObjectClass,title_sm,NULL);
  snprintf(local_buf,sizeof(local_buf),"TiMidity++ %s",timidity_version);
  XtVaSetValues(title_mb,XtNlabel,local_buf,NULL);
  if(ctl->trace_playing)
    strcpy(window_title,"[ No Playing File ]");
#ifndef WIDGET_IS_LABEL_WIDGET
  deleteTextCB(w,NULL,NULL);
#endif
}

/*ARGSUSED*/
static void backspaceCB(Widget w,XtPointer data,XtPointer call_data) {
  XawTextPosition begin,end,curr;
  XawTextBlock tb;
  XawTextGetSelectionPos(w, &begin, &end);
  curr = XawTextGetInsertionPoint(w);
  if(begin == end) return;
  tb.firstPos = 0;
  tb.ptr = ".";
  tb.format = FMT8BIT;
  tb.length = strlen(tb.ptr);
  XawTextReplace(w,begin,end,&tb);
  XawTextSetInsertionPoint(w,begin+1);
}

/*ARGSUSED*/
#ifndef WIDGET_IS_LABEL_WIDGET
static void deleteTextCB(Widget w,XtPointer data,XtPointer call_data) {
  XtVaSetValues(lyric_t,XtNstring,(String)"<< TiMidity Messages >>",NULL);
}
#endif

static void createFlist(void) {
  if(!(popup_shell_exist & FLIST_WINDOW)) {
    popup_file= XtVaCreatePopupShell("popup_file",transientShellWidgetClass,
                                  toplevel,NULL);
    popup_fbox= XtVaCreateManagedWidget("popup_fbox",boxWidgetClass,popup_file,
                                  XtNwidth,280,XtNheight,360,
                                  XtNorientation,XtorientVertical,
                                  XtNbackground,bgcolor,NULL);
    file_vport = XtVaCreateManagedWidget("file_vport",viewportWidgetClass,popup_fbox,
                                  XtNallowHoriz,True, XtNallowVert,True,
                                  XtNuseBottom,True,
                                  XtNwidth,FILEVPORT_WIDTH, XtNheight,FILEVPORT_HEIGHT,
                                  XtNbackground,textbgcolor,XtNborderWidth,1,NULL);
    file_list= XtVaCreateManagedWidget("filelist",listWidgetClass,file_vport,
                                  XtNverticalList,True,
                                  XtNforceColumns,True,XtNdefaultColumns,1,
                                  XtNbackground,textbgcolor,NULL);
    flist_cmdbox= XtVaCreateManagedWidget("flist_cmdbox",boxWidgetClass,popup_fbox,
                                  XtNorientation,XtorientHorizontal,
                                  XtNwidth,272,XtNheight,24,
                                  XtNborderWidth,0,
                                  XtNbackground,bgcolor,NULL);
    popup_fplay= XtVaCreateManagedWidget("fplaybutton",
                    commandWidgetClass,flist_cmdbox,
                    XtNfont,volumefont,XtNresize,False,NULL);
    popup_fdelete= XtVaCreateManagedWidget("fdeletebutton",
                    commandWidgetClass,flist_cmdbox,
                    XtNfont,volumefont,XtNresize,False,NULL);
    popup_fdelall= XtVaCreateManagedWidget("fdelallbutton",
                    commandWidgetClass,flist_cmdbox,
                    XtNfont,volumefont,XtNresize,False,NULL);
    popup_fclose= XtVaCreateManagedWidget("closebutton",
                    commandWidgetClass,flist_cmdbox,
                    XtNfont,volumefont,XtNresize,False,NULL);
    XtAddCallback(popup_fclose,XtNcallback,flistcloseCB,NULL);
    XtAddCallback(popup_fplay,XtNcallback,fselectCB,NULL);
    XtAddCallback(popup_fdelete,XtNcallback,fdeleteCB,NULL);
    XtAddCallback(popup_fdelall,XtNcallback,fdelallCB,NULL);
    XtSetKeyboardFocus(popup_file, popup_fbox);
    popup_shell_exist |= FLIST_WINDOW;
    XawListChange(file_list, flist, 0,0,True);
  }
}

void a_start_interface(int pipe_in) {
  static XtActionsRec actions[] ={
    {"do-quit",(XtActionProc)quitCB},
    {"fix-menu", (XtActionProc)filemenuCB},
    {"do-menu", (XtActionProc)filemenuAction},
    {"do-complete", (XtActionProc)completeDir},
    {"do-chgdir", (XtActionProc)setDirAction},
    {"draw-trace",(XtActionProc)redrawAction},
    {"do-exchange",(XtActionProc)exchgWidth},
    {"do-toggletrace",(XtActionProc)toggletrace},
    {"do-revcaption",(XtActionProc)redrawCaption},
    {"do-dialog-button",(XtActionProc)popdownLoad},
    {"do-load",(XtActionProc)popupLoad},
    {"do-play",(XtActionProc)playCB},
    {"do-sndspec",(XtActionProc)sndspecCB},
    {"do-pause",(XtActionProc)pauseAction},
    {"do-stop",(XtActionProc)stopCB},
    {"do-next",(XtActionProc)nextCB},
    {"do-prev",(XtActionProc)prevCB},
    {"do-forward",(XtActionProc)forwardCB},
    {"do-back",(XtActionProc)backCB},
    {"do-key",(XtActionProc)soundkeyAction},
    {"do-speed",(XtActionProc)speedAction},
    {"do-voice",(XtActionProc)voiceAction},
    {"do-volset",(XtActionProc)volsetCB},
    {"do-volupdown",(XtActionProc)volupdownAction},
    {"do-tuneset",(XtActionProc)tunesetAction},
    {"do-tuneslide",(XtActionProc)tuneslideAction},
    {"do-resize",(XtActionProc)resizeAction},
    {"checkRightAndPopupSubmenu",checkRightAndPopupSubmenu},
    {"leaveSubmenu",leaveSubmenu},
    {"popdownSubmenu",popdownSubmenu},
    {"do-options",(XtActionProc)optionspopupCB},
    {"do-filelist",(XtActionProc)flistpopupCB},
    {"do-about",(XtActionProc)aboutCB},
    {"do-closeparent",closeParent},
    {"do-flistmove",flistMove},
    {"do-fselect",(XtActionProc)fselectCB},
    {"do-fdelete",(XtActionProc)fdeleteCB},
    {"do-backspace",(XtActionProc)backspaceCB},
#ifndef WIDGET_IS_LABEL_WIDGET
    {"do-deltext",(XtActionProc)deleteTextCB},
#endif
  };

  static String fallback_resources[]={
#ifdef I18N
    "*Command*international: True",
    "*file_simplemenu*international: True",
    "*Label*fontSet: -adobe-helvetica-bold-o-*-*-14-*-*-*-*-*-*-*",
    "*MenuButton*fontSet: -adobe-helvetica-bold-r-*-*-14-*-*-*-*-*-*-*",
    "*Command*fontSet: -adobe-helvetica-bold-r-*-*-12-*-*-*-*-*-*-*",
    "*List*fontSet: -adobe-helvetica-medium-r-*-*-12-*-*-*-*-*-*-*",
    "*Text*fontSet: -misc-fixed-medium-r-normal--14-*-*-*-*-*-*-*",
    "*Form*fontSet: -adobe-helvetica-medium-r-*-*-12-*-*-*-*-*-*-*",
#else
    "*Label.font: -adobe-helvetica-bold-r-*-*-14-*-*-*-*-*-*-*",
    "*Text*font: -misc-fixed-medium-r-normal--14-*-*-*-*-*-*-*",
#endif
    "*Text*background: " TEXTBG_COLOR "",
    "*Text*scrollbar*background: " TEXTBG_COLOR "",
    "*Scrollbar*background: " TEXTBG_COLOR "",
    "*Label.foreground: black",
    "*Label.background: #CCFF33",
    "*Command.background: " COMMANDBUTTON_COLOR "",
    "*Dialog.Command.background: " COMMANDBUTTON_COLOR "",
    "*Dialog.Text.background: " TEXTBG_COLOR "",
    "*fontSet: -*--14-*",
    "*load_dialog.label.background: " COMMON_BGCOLOR "",
#ifdef I18N
    "*international: True",
#else
    "*international: False",
#endif
    "*Command.font: -adobe-helvetica-bold-r-*-*-12-*-*-*-*-*-*-*",
    "*Toggle.font: -adobe-helvetica-medium-o-*-*-12-*-*-*-*-*-*-*",
    "*MenuButton.translations:<EnterWindow>:    highlight()\\n\
        <LeaveWindow>:  reset()\\n\
        Any<BtnDown>:   reset() fix-menu() PopupMenu()",
    "*menu_box.borderWidth: 0",
    "*button_box.borderWidth: 0",
    "*button_box.horizDistance: 4",
    "*file_menubutton.menuName: file_simplemenu",
    "*file_menubutton.width: 60",
    "*file_menubutton.height: 28",
    "*file_menubutton.horizDistance: 6",
    "*file_menubutton.vertDistance: 4",
#ifdef I18N
    "*file_menubutton.file_simplemenu*fontSet: -*--14-*",
    "*title_menubutton.title_simplemenu*fontSet: -adobe-helvetica-medium-r-*-*-12-*-*-*-*-*-*-*",
#else
    "*file_menubutton.file_simplemenu*font: -adobe-helvetica-medium-r-*-*-12-*-*-*-*-*-*-*",
    "*title_menubutton.title_simplemenu*font: -adobe-helvetica-medium-r-*-*-12-*-*-*-*-*-*-*",
#endif
    "*title_menubutton*SmeBSB.font: -adobe-helvetica-medium-r-*-*-12-*-*-*-*-*-*-*",
    "*title_menubutton.menuName: title_simplemenu",
    "*popup_abox.OK.label: OK",
    "*title_menubutton.width: 210",
    "*title_menubutton.height: 28",
    "*title_menubutton.resize: false",
    "*title_menubutton.horizDistance: 6",
    "*title_menubutton.vertDistance: 4",
    "*title_menubutton.fromHoriz: file_menubutton",
    "*time_label.width: 92",
    "*time_label.height: 26",
    "*time_label.resize: false",
    "*time_label.fromHoriz: title_menubutton",
    "*time_label.horizDistance: 1",
    "*time_label.vertDistance: 4",
    "*time_label.label: time / mode",
    "*button_box.height: 40",
    "*play_button.width: 32",
    "*play_button.height: 32",
    "*play_button.horizDistance: 1",
    "*play_button.vertDistance: 9",
    "*pause_button.width: 32",
    "*pause_button.height: 32",
    "*pause_button.horizDistance: 1",
    "*pause_button.vertDistance: 1",
    "*stop_button.width: 32",
    "*stop_button.height: 32",
    "*stop_button.horizDistance: 1",
    "*stop_button.vertDistance: 1",
    "*prev_button.width: 32",
    "*prev_button.height: 32",
    "*prev_button.horizDistance: 1",
    "*prev_button.vertDistance: 1",
    "*back_button.width: 32",
    "*back_button.height: 32",
    "*back_button.horizDistance: 1",
    "*back_button.vertDistance: 1",
    "*fwd_button.width: 32",
    "*fwd_button.height: 32",
    "*fwd_button.horizDistance: 1",
    "*fwd_button.vertDistance: 1",
    "*next_button.width: 32",
    "*next_button.height: 32",
    "*next_button.horizDistance: 1",
    "*next_button.vertDistance: 1",
    "*quit_button.width: 32",
    "*quit_button.height: 32",
    "*quit_button.horizDistance: 1",
    "*quit_button.vertDistance: 1",
    "*random_button.width: 32",
    "*random_button.height: 32",
    "*random_button.horizDistance: 4",
    "*random_button.vertDistance: 1",
    "*repeat_button.width: 32",
    "*repeat_button.height: 32",
    "*repeat_button.horizDistance: 1",
    "*repeat_button.vertDistance: 1",
    "*lyric_text.fromVert: tune_box",
    "*lyric_text.borderWidth: 1" ,
    "*lyric_text.vertDistance: 4",
    "*lyric_text.horizDistance: 6",
#ifndef WIDGET_IS_LABEL_WIDGET
    "*lyric_text.height: 120",
    "*lyric_text.scrollVertical: WhenNeeded",
    "*lyric_text.translations: #override\\n\
        <Btn2Down>:     do-deltext()",
#else
    "*lyric_text.height: 30",
    "*lyric_text.label: MessageWindow",
    "*lyric_text.resize: true",
#endif
#ifdef I18N
    "*popup_optbox*international: True",
    "*lyric_text.international: True",
    "*volume_box*fontSet: -adobe-helvetica-bold-r-*-*-12-*-*-*-*-*-*-*",
    "*tune_box*fontSet: -adobe-helvetica-bold-r-*-*-12-*-*-*-*-*-*-*",
#else
    "*popup_optbox*international: False",
    "*lyric_text.international: False",
    "*volume_box*font: -adobe-helvetica-bold-r-*-*-12-*-*-*-*-*-*-*",
    "*tune_box*font: -adobe-helvetica-bold-r-*-*-12-*-*-*-*-*-*-*",
#endif
    "*volume_label.vertDistance: 0",
    "*volume_box.vertDistance: 2",
    "*volume_box.borderWidth: 0",
    "*volume_label.borderWidth: 0",
    "*volume_bar.length: 330",
    "*tune_box.borderWidth: 0",
    "*tune_label.label: ----",
    "*tune_label.vertDistance: 0",
    "*tune_label.horizDistance: 0",
    "*tune_label0.horizDistance: 0",
    "*tune_box.vertDistance: 2",
    "*tune_bar.length: 330",
    "*popup_load.title: TiMidity <Load File>",
    "*popup_loadform.height: 400",
    "*popup_option.title: TiMidity <Extend Modes>",
    "*popup_file.title: TiMidity <File List>",
    "*popup_about.title: Information",
    "*load_dialog.label: File Name",
    "*load_dialog.borderWidth: 0",
    "*load_dialog.height: 132",
    "*trace.vertDistance: 2",
    "*trace.borderWidth: 1",
    "*trace_vport.borderWidth: 1",
#ifdef I18N
    "*popup_loadform.load_dialog.label.fontSet: -*--14-*",
    "*popup_abox*fontSet: -adobe-helvetica-bold-o-*-*-14-*-*-*-*-*-*-*",
#else
    "*popup_loadform.load_dialog.label.font: -adobe-helvetica-bold-r-*-*-14-*-*-*-*-*-*-*",
    "*popup_abox*font: -adobe-helvetica-bold-o-*-*-14-*-*-*-*-*-*-*",
#endif
    "*cwd_label.font: -adobe-helvetica-medium-r-*-*-12-*-*-*-*-*-*-*",
    "*time_label*cwd_info.font: -adobe-helvetica-medium-r-*-*-12-*-*-*-*-*-*-*",
    "*time_label.font: -adobe-helvetica-bold-r-*-*-14-*-*-*-*-*-*-*",
    "*BitmapDir: " XAW_BITMAP_DIR "/",
#ifdef XAW3D
    "*volume_bar.translations: #override\\n\
        ~Ctrl Shift<Btn1Down>: do-volupdown(-50)\\n\
        ~Ctrl Shift<Btn3Down>: do-volupdown(50)\\n\
        Ctrl ~Shift<Btn1Down>: do-volupdown(-5)\\n\
        Ctrl ~Shift<Btn3Down>: do-volupdown(5)\\n\
        <Btn1Down>: MoveThumb()\\n\
        <BtnUp>: NotifyScroll(FullLength) EndScroll()",
    "*tune_bar.translations: #override\\n\
        <Btn1Up>: do-tuneset()\\n\
        <Btn3Up>: do-tuneslide()\\n\
        <Btn1Down>: MoveThumb()\\n\
        <BtnUp>: NotifyScroll(FullLength) EndScroll()",
#else
    "*volume_bar.translations: #override\\n\
        ~Ctrl Shift<Btn1Down>: do-volupdown(-50)\\n\
        ~Ctrl Shift<Btn3Down>: do-volupdown(50)\\n\
        Ctrl ~Shift<Btn1Down>: do-volupdown(-5)\\n\
        Ctrl ~Shift<Btn3Down>: do-volupdown(5)\\n\
        <Btn1Down>: StartScroll(Forward) MoveThumb()\\n\
        <Btn3Down>: StartScroll(Backward) MoveThumb()\\n\
        <BtnUp>: NotifyScroll(FullLength) EndScroll()",
    "*tune_bar.translations: #override\\n\
        <Btn1Up>: do-tuneset()\\n\
        <Btn3Up>: do-tuneslide()\\n\
        <Btn1Down>: StartScroll(Forward) MoveThumb()\\n\
        <Btn3Down>: StartScroll(Backward) MoveThumb()\\n\
        <BtnUp>: NotifyScroll(FullLength) EndScroll()",
#endif
    "*file_simplemenu.load.label: Load (Meta-N)",
    "*file_simplemenu.saveconfig.label: Save Config (Meta-S)",
    "*file_simplemenu.hidetext.label: (Un)Hide Messages (Ctrl-M)",
    "*file_simplemenu.hidetrace.label: (Un)Hide Trace (Ctrl-T)",
    "*file_simplemenu.shuffle.label: Shuffle (Ctrl-S)",
    "*file_simplemenu.repeat.label: Repeat (Ctrl-R)",
    "*file_simplemenu.autostart.label: Auto Start",
    "*file_simplemenu.autoquit.label: Auto Exit",
    "*file_simplemenu.filelist.label: File List (Ctrl-F)",
    "*file_simplemenu.modes.label: Extend Modes (Ctrl-O)",
    "*file_simplemenu.about.label: About",
    "*file_simplemenu.quit.label: Quit (Meta-Q, Q)",
    "*load_dialog.OK.label: OK",
    "*load_dialog.add.label: Add ALL",
    "*load_dialog.cancel.label: Cancel",
    "*flist_cmdbox.fplaybutton.label: Play",
    "*flist_cmdbox.fdeletebutton.label: Delete",
    "*flist_cmdbox.fdelallbutton.label: Delete ALL",
    "*closebutton.label: Close",
    "*modul_box.modul_lbl.label: Modulation control",
    "*porta_box.porta_lbl.label: Portamento control",
    "*nrpnv_box.nrpnv_lbl.label: NRPN Vibration",
    "*reverb_box.reverb_lbl.label: Reverb control",
    "*chorus_box.chorus_lbl.label: Chorus control",
    "*chpressure_box.chpressure_lbl.label: Channel Pressure control",
    "*overlapvoice_box.overlapv_lbl.label: Allow Multiple Same Notes",
    "*txtmeta_box.txtmeta_lbl.label: Tracing All Text Meta Events",
    "*base_form.translations: #override\\n\
        ~Ctrl Meta<Key>n:   do-load()\\n\
        ~Ctrl Meta<Key>s:   do-menu(" IDS_SAVECONFIG ")\\n\
        Ctrl <Key>r:        do-menu(" IDS_REPEAT ")\\n\
        Ctrl <Key>s:        do-menu(" IDS_SHUFFLE ")\\n\
        Ctrl<Key>t:     do-menu(" IDS_HIDETRACE ")\\n\
        Ctrl<Key>m:     do-menu(" IDS_HIDETXT ")\\n\
        ~Ctrl<Key>q:        do-quit()\\n\
        ~Ctrl<Key>r:        do-play()\\n\
        <Key>Return:        do-play()\\n\
        <Key>KP_Enter:      do-play()\\n\
        ~Ctrl<Key>g:        do-sndspec()\\n\
        ~Ctrl<Key>space:    do-pause()\\n\
        ~Ctrl<Key>s:        do-stop()\\n\
        <Key>p:         do-prev()\\n\
        <Key>Left:      do-prev()\\n\
        ~Meta<Key>n:        do-next()\\n\
        <Key>Right:     do-next()\\n\
        ~Ctrl<Key>f:        do-forward()\\n\
        ~Ctrl<Key>b:        do-back()\\n\
        ~Ctrl<Key>plus:     do-key()\\n\
        ~Shift<Key>-:       do-key(1)\\n\
        <Key>KP_Add:        do-key()\\n\
        <Key>KP_Subtract:   do-key(1)\\n\
        ~Ctrl<Key>greater:  do-speed()\\n\
        ~Ctrl<Key>less:     do-speed(1)\\n\
        ~Ctrl ~Shift<Key>o: do-voice()\\n\
        ~Ctrl Shift<Key>o:  do-voice(1)\\n\
        Ctrl<Key>o:     do-options()\\n\
        Ctrl<Key>f:     do-filelist()\\n\
        <Key>l:         do-filelist()\\n\
        <Key>a:         do-about()\\n\
        ~Ctrl ~Shift<Key>v: do-volupdown(-10)\\n\
        ~Ctrl Shift<Key>v:  do-volupdown(10)\\n\
        <Key>Down:  do-volupdown(-10)\\n\
        <Key>Up:    do-volupdown(10)\\n\
        ~Ctrl<Key>x:        do-exchange()\\n\
        ~Ctrl<Key>t:        do-toggletrace()\\n\
        <ConfigureNotify>:  do-resize()",

    "*load_dialog.value.translations: #override\\n\
        ~Ctrl<Key>Return:   do-chgdir() end-of-line()\\n\
        ~Ctrl<Key>KP_Enter: do-chgdir() end-of-line()\\n\
        ~Ctrl ~Meta<Key>Tab:    do-complete() end-of-line()\\n\
        Ctrl ~Shift<Key>g:  do-dialog-button(1)\\n\
        <Key>BackSpace:     do-backspace() delete-previous-character()\\n\
        <Key>Delete:        do-backspace() delete-previous-character()\\n\
        Ctrl<Key>H:         do-backspace() delete-previous-character()\\n\
        <Key>Escape:        do-dialog-button(1)",
    "*trace.translations: #override\\n\
        <Btn1Down>:     do-toggletrace()\\n\
        <EnterNotify>:      do-revcaption()\\n\
        <LeaveNotify>:      do-revcaption()\\n\
        <Expose>:       draw-trace()",
    "*time_label.translations: #override\\n\
        <Btn2Down>:     do-menu(" IDS_HIDETRACE ")\\n\
        <Btn3Down>:     do-exchange()",
    "*popup_optbox.translations: #override\\n\
        ~Ctrl<Key>c:        do-closeparent()\\n\
        ~Ctrl<Key>q:        do-quit()",
    "*popup_fbox.translations: #override\\n\
        ~Ctrl<Key>c:        do-closeparent()\\n\
        <Key>Up:        do-flistmove(-1)\\n\
        <Key>p:         do-flistmove(-1)\\n\
        <Key>Prior:     do-flistmove(-5)\\n\
        <Key>Right:     do-flistmove(-5)\\n\
        ~Ctrl<Key>r:        do-fselect()\\n\
        <Key>Return:        do-fselect()\\n\
        <Key>KP_Enter:      do-fselect()\\n\
        Ctrl<Key>m:     do-fselect()\\n\
        <Key>space:     do-pause()\\n\
        <Key>s:         do-stop()\\n\
        <Key>Down:      do-flistmove(1)\\n\
        <Key>n:         do-flistmove(1)\\n\
        <Key>Next:      do-flistmove(5)\\n\
        <Key>Left:      do-flistmove(5)\\n\
        <Key>d:         do-fdelete()\\n\
        ~Shift<Key>v:       do-volupdown(-10)\\n\
        Shift<Key>v:        do-volupdown(10)\\n\
        ~Ctrl<Key>f:        do-forward()\\n\
        ~Ctrl<Key>b:        do-back()\\n\
        ~Ctrl<Key>q:        do-quit()",
    "*popup_abox.translations: #override\\n\
        ~Ctrl<Key>c:        do-closeparent()\\n\
        <Key>KP_Enter:      do-closeparent()\\n\
        <Key>Return:        do-closeparent()",
    NULL,
  };
  XtAppContext app_con;
  char cbuf[PATH_MAX];
  Pixmap bmPixmap;
  int bmwidth, bmheight;
  int i, j, k, tmpi;
  int argc=1;
  float thumb, l_thumb, l_thumbj;
  char *argv=APP_NAME, *filetext;
#ifdef I18N
  #define XtNfontDEF XtNfontSet
  XFontSet textfont;
#else
  #define XtNfontDEF XtNfont
  XFontStruct *textfont;
#endif
  XawListReturnStruct lrs;
  unsigned long gcmask;
  XGCValues gcval;

#ifdef DEBUG_PRINT_RESOURCE
  char str[BUFSIZ], *p,*new;
  for(i=0; fallback_resources[i] != NULL; i++) {
    p = fallback_resources[i]; new= str;
    while(*p != NULL) {
      if(*p == 'n' && *(p-1) == '\\') {
        *new++ = 'n'; *new++ = '\\'; *new++ = '\n';
      } else {
        *new++ = *p;
      }
      p++;
    }
    *new = '\0'; puts(str);
  }
  exit(0);
#endif

  xaw_vendor_setup();

#ifdef I18N
  XtSetLanguageProc(NULL,NULL,NULL);
#endif
  toplevel=XtVaAppInitialize(&app_con,APP_CLASS,NULL,ZERO,&argc,&argv,
                         fallback_resources,NULL);
  XtGetApplicationResources(toplevel,(caddr_t)&app_resources,xaw_resources,
                          XtNumber(xaw_resources),NULL,0);
  bitmapdir = app_resources.bitmap_dir;
  arrangetitle = app_resources.arrange_title;
  savelist = app_resources.save_list;
  text_height = (Dimension)app_resources.text_height;
  trace_width = (Dimension)app_resources.trace_width;
  trace_height = (Dimension)app_resources.trace_height;
  menu_width = (Dimension)app_resources.menu_width;
  labelfont = app_resources.label_font;
  volumefont = app_resources.volume_font;
  textfont = app_resources.text_font;
  tracefont = app_resources.trace_font;
  ttitlefont = app_resources.ttitle_font;
  a_readconfig(&Cfg);
  amplitude=
    (amplification == DEFAULT_AMPLIFICATION)? Cfg.amplitude:amplification;
  disp = XtDisplay(toplevel);
  screen = DefaultScreen(disp);
  root_height = DisplayHeight(disp, screen);
  root_width = DisplayWidth(disp, screen);
  check_mark = XCreateBitmapFromData(XtDisplay(toplevel),
                                     RootWindowOfScreen(XtScreen(toplevel)),
                                     (char *)check_bits,
                                     check_width, check_height);
  arrow_mark = XCreateBitmapFromData(XtDisplay(toplevel),
                                      RootWindowOfScreen(XtScreen(toplevel)),
                                      (char *)arrow_bits,
                                      (unsigned int)arrow_width,arrow_height);
  on_mark = XCreateBitmapFromData(XtDisplay(toplevel),
                                      RootWindowOfScreen(XtScreen(toplevel)),
                                      (char *)on_bits,
                                      (unsigned int)on_width,on_height);
  off_mark = XCreateBitmapFromData(XtDisplay(toplevel),
                                      RootWindowOfScreen(XtScreen(toplevel)),
                                      (char *)off_bits,
                                      (unsigned int)off_width,off_height);
  for(i= 0; i < MAXBITMAP; i++) {
    snprintf(cbuf,sizeof(cbuf),"%s/%s",bitmapdir,bmfname[i]);
    XReadBitmapFile(disp,RootWindow(disp,screen),cbuf,&bm_width[i],&bm_height[i],
                    &bm_Pixmap[i],&x_hot,&y_hot);
  }

  safe_getcwd(basepath, sizeof(basepath));

#ifdef OFFIX
  DndInitialize(toplevel);
  DndRegisterOtherDrop(FileDropedHandler);
  DndRegisterIconDrop(FileDropedHandler);
#endif
  XtAppAddActions(app_con, actions, XtNumber(actions));

  bgcolor = app_resources.common_bgcolor;
  menubcolor = app_resources.menub_bgcolor;
  textcolor = app_resources.common_fgcolor;
  textbgcolor = app_resources.text_bgcolor;
  text2bgcolor = app_resources.text2_bgcolor;
  buttonbgcolor = app_resources.button_bgcolor;
  buttoncolor = app_resources.button_fgcolor;
  togglecolor = app_resources.toggle_fgcolor;
  if(ctl->trace_playing) {
    volcolor = app_resources.volume_color;
    expcolor = app_resources.expr_color;
    pancolor = app_resources.pan_color;
    tracecolor = app_resources.trace_bgcolor;
  }
  if(ctl->trace_playing) {
    gcmask = GCForeground | GCBackground | GCFont;
    gcval.foreground = 1;
    gcval.background = 1;
    gcval.plane_mask = 1;
#ifdef I18N
    XFontsOfFontSet(ttitlefont,&fs_list,&ml);
    ttitlefont0 = fs_list[0];
    if (! ttitlefont0->fid) {
      ttitlefont0 = XLoadQueryFont(disp, ml[0]);
      if (! ttitlefont0) {
	fprintf(stderr, "can't load fonts %s\n", ml[0]);
	exit(1);
      }
    }
    gcval.font = ttitlefont0->fid;
#else
    gcval.font = ttitlefont->fid;
#endif
    gcs = XCreateGC(disp, RootWindow(disp, screen), gcmask, &gcval);
  }
  base_f=XtVaCreateManagedWidget("base_form",boxWidgetClass,toplevel,
            XtNbackground,bgcolor,
            XtNwidth,rotatewidth[currwidth], NULL);
  m_box=XtVaCreateManagedWidget("menu_box",boxWidgetClass,base_f,
            XtNorientation,XtorientHorizontal,
            XtNbackground,bgcolor, NULL);
  filetext = app_resources.file_text;
  file_mb=XtVaCreateManagedWidget("file_menubutton",menuButtonWidgetClass,m_box,
            XtNbackground,menubcolor,
            XtNfont,labelfont, XtNlabel,filetext, NULL);
  file_sm=XtVaCreatePopupShell("file_simplemenu",simpleMenuWidgetClass,file_mb,
            XtNforeground,textcolor, XtNbackground,textbgcolor,XtNresize,False,
            XtNbackingStore,NotUseful, XtNsaveUnder,False, XtNwidth,menu_width+100,
            NULL);
  snprintf(cbuf,sizeof(cbuf),"TiMidity++ %s",timidity_version);
  title_mb=XtVaCreateManagedWidget("title_menubutton",menuButtonWidgetClass,m_box,
            XtNbackground,menubcolor,XtNlabel,cbuf,
            XtNfont,labelfont, NULL);
  title_sm=XtVaCreatePopupShell("title_simplemenu",simpleMenuWidgetClass,title_mb,
            XtNforeground,textcolor, XtNbackground,textbgcolor,
            XtNbackingStore,NotUseful, XtNsaveUnder,False, NULL);
  time_l=XtVaCreateManagedWidget("time_label",commandWidgetClass,m_box,
            XtNfont,labelfont,
            XtNbackground,menubcolor,NULL);
  b_box=XtVaCreateManagedWidget("button_box",boxWidgetClass,base_f,
            XtNorientation,XtorientHorizontal,
            XtNwidth,rotatewidth[currwidth]-10,
            XtNbackground,bgcolor,XtNfromVert,m_box, NULL);
  v_box=XtVaCreateManagedWidget("volume_box",boxWidgetClass,base_f,
            XtNorientation,XtorientHorizontal,
            XtNwidth,TRACE_WIDTH_SHORT, XtNheight,36,
            XtNfromVert,b_box,XtNbackground,bgcolor, NULL);
  t_box=XtVaCreateManagedWidget("tune_box",boxWidgetClass,base_f,
            XtNorientation,XtorientHorizontal,
            XtNwidth,TRACE_WIDTH_SHORT, XtNheight,36,
            XtNfromVert,v_box,XtNbackground,bgcolor, NULL);
  i = XTextWidth(volumefont,"Volume ",7)+8;
  vol_l0=XtVaCreateManagedWidget("volume_label0",labelWidgetClass,v_box,
            XtNwidth,i, XtNresize,False,
            XtNfont,volumefont, XtNlabel, "Volume", XtNborderWidth,0,
            XtNforeground,textcolor, XtNbackground,bgcolor, NULL);
  VOLUME_LABEL_WIDTH = i+30;
  j =   XTextWidth(volumefont,"000",3)+8;
  VOLUME_LABEL_WIDTH += j;
  vol_l=XtVaCreateManagedWidget("volume_label",labelWidgetClass,v_box,
            XtNwidth,j, XtNresize,False,XtNborderWidth,0,
            XtNfont,volumefont, XtNorientation, XtorientHorizontal,
            XtNforeground,textcolor, XtNbackground,bgcolor, NULL);
  vol_bar=XtVaCreateManagedWidget("volume_bar",scrollbarWidgetClass,v_box,
            XtNorientation, XtorientHorizontal,
            XtNwidth, TRACE_WIDTH_SHORT -VOLUME_LABEL_WIDTH,
            XtNbackground,textbgcolor,
            XtNfromVert,vol_l, XtNtopOfThumb,&l_thumb, NULL);
  i = XTextWidth(volumefont," 00:00",6);
  tune_l0=XtVaCreateManagedWidget("tune_label0",labelWidgetClass,t_box,
            XtNwidth,i, XtNresize,False, XtNlabel, " 0:00",
            XtNfont,volumefont, XtNfromVert,vol_l0,XtNborderWidth,0,
            XtNforeground,textcolor, XtNbackground,bgcolor, NULL);
  j = XTextWidth(volumefont,"/ 00:00",7);
  tune_l=XtVaCreateManagedWidget("tune_label",labelWidgetClass,t_box,
            XtNwidth,j, XtNresize,False,
            XtNfont,volumefont, XtNfromVert,vol_l,XtNborderWidth,0,
            XtNforeground,textcolor, XtNbackground,bgcolor, NULL);
  tune_bar=XtVaCreateManagedWidget("tune_bar",scrollbarWidgetClass,t_box,
            XtNwidth, TRACE_WIDTH_SHORT -i -j -30,
            XtNbackground,textbgcolor,XtNorientation, XtorientHorizontal,
            XtNfromVert,tune_l, XtNtopOfThumb,&l_thumbj, NULL);
  l_thumb = thumb = (float)amplitude / (float)MAXVOLUME;
  if (sizeof(thumb) > sizeof(XtArgVal)) {
    XtVaSetValues(vol_bar,XtNtopOfThumb,&thumb,NULL);
  } else {
    XtArgVal * l_thumb = (XtArgVal *) &thumb;
    XtVaSetValues(vol_bar,XtNtopOfThumb,*l_thumb,NULL);
  }
  play_b=XtVaCreateManagedWidget("play_button",toggleWidgetClass,b_box,
            XtNbitmap,bm_Pixmap[BM_PLAY],
            XtNforeground,buttoncolor, XtNbackground,buttonbgcolor,
            NULL);
  pause_b=XtVaCreateManagedWidget("pause_button",toggleWidgetClass,b_box,
            XtNbitmap,bm_Pixmap[BM_PAUSE],
            XtNfromHoriz, play_b,
            XtNforeground,togglecolor, XtNbackground,buttonbgcolor,
            NULL);
  stop_b=XtVaCreateManagedWidget("stop_button",commandWidgetClass,b_box,
            XtNbitmap,bm_Pixmap[BM_STOP],
            XtNforeground,buttoncolor, XtNbackground,buttonbgcolor,
            XtNfromHoriz,pause_b,NULL);
  prev_b=XtVaCreateManagedWidget("prev_button",commandWidgetClass,b_box,
            XtNbitmap,bm_Pixmap[BM_PREV],
            XtNforeground,buttoncolor, XtNbackground,buttonbgcolor,
            XtNfromHoriz,stop_b,NULL);
  back_b=XtVaCreateManagedWidget("back_button",commandWidgetClass,b_box,
            XtNbitmap,bm_Pixmap[BM_BACK],
            XtNforeground,buttoncolor, XtNbackground,buttonbgcolor,
            XtNfromHoriz,prev_b,NULL);
  fwd_b=XtVaCreateManagedWidget("fwd_button",commandWidgetClass,b_box,
            XtNbitmap,bm_Pixmap[BM_FWRD],
            XtNforeground,buttoncolor, XtNbackground,buttonbgcolor,
            XtNfromHoriz,back_b,NULL);
  next_b=XtVaCreateManagedWidget("next_button",commandWidgetClass,b_box,
            XtNbitmap,bm_Pixmap[BM_NEXT],
            XtNforeground,buttoncolor, XtNbackground,buttonbgcolor,
            XtNfromHoriz,fwd_b,NULL);
  quit_b=XtVaCreateManagedWidget("quit_button",commandWidgetClass,b_box,
            XtNbitmap,bm_Pixmap[BM_QUIT],
            XtNforeground,buttoncolor, XtNbackground,buttonbgcolor,
            XtNfromHoriz,next_b,NULL);
  random_b=XtVaCreateManagedWidget("random_button",toggleWidgetClass,b_box,
            XtNbitmap,bm_Pixmap[BM_RANDOM],
            XtNfromHoriz,quit_b,
            XtNforeground,togglecolor, XtNbackground,buttonbgcolor,
            NULL);
  repeat_b=XtVaCreateManagedWidget("repeat_button",toggleWidgetClass,b_box,
            XtNbitmap,bm_Pixmap[BM_REPEAT],
            XtNfromHoriz,random_b,
            XtNforeground,togglecolor, XtNbackground,buttonbgcolor,
            NULL);
  popup_load=XtVaCreatePopupShell("popup_load",transientShellWidgetClass,toplevel,
            NULL);  
  popup_load_f= XtVaCreateManagedWidget("popup_loadform",formWidgetClass,popup_load,
            XtNbackground,bgcolor, NULL);
  load_d=XtVaCreateManagedWidget("load_dialog",dialogWidgetClass,popup_load_f,
            XtNbackground,bgcolor, XtNresizable,True, NULL);
  cwd_l = XtVaCreateManagedWidget("cwd_label",labelWidgetClass,popup_load_f,
            XtNlabel,basepath, XtNborderWidth,0, XtNfromVert,load_d,
            XtNwidth,250, XtNheight,20,
            XtNbackground,text2bgcolor, XtNresizable,False, NULL);
  load_vport = XtVaCreateManagedWidget("vport",viewportWidgetClass, popup_load_f,
            XtNfromVert,cwd_l, XtNallowHoriz,True, XtNallowVert,True,
            XtNbackground,textbgcolor, XtNuseBottom,True, 
            XtNwidth,250, XtNheight,200, NULL);
  load_flist = XtVaCreateManagedWidget("files",listWidgetClass,load_vport,
            XtNverticalList,True, XtNforceColumns,False,
            XtNbackground,textbgcolor, XtNdefaultColumns,3, NULL);
  load_info = XtVaCreateManagedWidget("cwd_info",labelWidgetClass,popup_load_f,
            XtNborderWidth,0, XtNwidth,250, XtNheight,20, XtNresizable,False,
            XtNbackground,text2bgcolor, XtNfromVert,load_vport, NULL);
  XawDialogAddButton(load_d, "OK", popdownLoad,"Y");
  XawDialogAddButton(load_d, "add", popdownLoad,"A");
  XawDialogAddButton(load_d, "cancel", popdownLoad,NULL);
#ifndef WIDGET_IS_LABEL_WIDGET
  lyric_t=XtVaCreateManagedWidget("lyric_text",asciiTextWidgetClass,base_f,
            XtNwrap,XawtextWrapWord, XtNeditType,XawtextAppend,
            XtNwidth, rotatewidth[currwidth]-10,
#else
  lyric_t=XtVaCreateManagedWidget("lyric_text",labelWidgetClass,base_f,
            XtNresize,False,
            XtNforeground,textcolor, XtNbackground,menubcolor,
            XtNwidth,rotatewidth[currwidth]-10,
#endif
            XtNfontDEF,textfont, XtNheight,text_height,
            XtNfromVert,t_box, NULL);
  if(ctl->trace_playing) {
    trace_vport = XtVaCreateManagedWidget("trace_vport",viewportWidgetClass, base_f,
            XtNallowHoriz,True, XtNallowVert,True,
            XtNuseBottom,True, XtNfromVert,lyric_t, XtNbackground,tracecolor,
#ifdef WIDGET_IS_LABEL_WIDGET
            XtNuseRight,True,
#endif
            XtNwidth,trace_width, XtNheight,trace_height+12, NULL);
    trace = XtVaCreateManagedWidget("trace",widgetClass,trace_vport,
            XtNwidth,trace_width, XtNbackground,tracecolor,
            XtNheight,BAR_SPACE*MAX_XAW_MIDI_CHANNELS+TRACEV_OFS, NULL);
  }
  XtAddCallback(quit_b,XtNcallback,quitCB,NULL);
  XtAddCallback(play_b,XtNcallback,playCB,NULL);
  XtAddCallback(pause_b,XtNcallback,pauseCB,NULL);
  XtAddCallback(stop_b,XtNcallback,stopCB,NULL);
  XtAddCallback(prev_b,XtNcallback,prevCB,NULL);
  XtAddCallback(next_b,XtNcallback,nextCB,NULL);
  XtAddCallback(fwd_b,XtNcallback,forwardCB,NULL);
  XtAddCallback(back_b,XtNcallback,backCB,NULL);
  XtAddCallback(random_b,XtNcallback,randomCB,NULL);
  XtAddCallback(repeat_b,XtNcallback,repeatCB,NULL);
  XtAddCallback(vol_bar,XtNjumpProc,volsetCB,NULL);
  XtAddCallback(vol_bar,XtNscrollProc,volupdownCB,NULL);
  XtAppAddInput(app_con,pipe_in,(XtPointer)XtInputReadMask,handle_input,NULL);
  XtAddCallback(load_flist,XtNcallback,(XtCallbackProc)setDirList,cwd_l);
  XtAddCallback(time_l,XtNcallback,(XtCallbackProc)filemenuAction,NULL);

  XtRealizeWidget(toplevel);
  dirlist = NULL;
  lrs.string = "";
  setDirList(load_flist, cwd_l, &lrs);
  XtSetKeyboardFocus(base_f, base_f);
  XtSetKeyboardFocus(lyric_t, base_f);
  if(ctl->trace_playing) 
    XtSetKeyboardFocus(trace, base_f);
  XtSetKeyboardFocus(popup_load_f, load_d);
  load_t = XtNameToWidget(load_d,"value");
  XtOverrideTranslations(toplevel,
            XtParseTranslationTable("<Message>WM_PROTOCOLS: do-quit()"));
  snprintf(cbuf,sizeof(cbuf),"%s/%s",bitmapdir,iconname);
  XReadBitmapFile(disp,RootWindow(disp,screen),cbuf,
                  &bmwidth,&bmheight,&bmPixmap,&x_hot,&y_hot);
  XtVaSetValues(toplevel,XtNiconPixmap,bmPixmap,NULL);
  strcpy(window_title,APP_CLASS);
  w_title = strncat(window_title," : ",3);
  w_title += sizeof(APP_CLASS)+ 2;
  XtVaGetValues(toplevel,XtNheight,&base_height,NULL);
  XtVaGetValues(lyric_t,XtNheight,&lyric_height,NULL);
  a_print_text(lyric_t,strcpy(local_buf,"<< TiMidity Messages >>"));
  a_pipe_write("READY");

  if(ctl->trace_playing) {
    XGCValues gv;
    gv.fill_style = FillTiled;
    gv.fill_rule = WindingRule;
    Panel = (PanelInfo *)safe_malloc(sizeof(PanelInfo));
    gc_xcopy = XCreateGC(disp,RootWindow(disp, screen),GCFillStyle|GCFillRule,&gv);
    gct = XCreateGC(disp, RootWindow(disp, screen), 0, NULL);
    gc = XCreateGC(disp, RootWindow(disp, screen), 0, NULL);
    for(i=0; i<MAX_XAW_MIDI_CHANNELS; i++) {
      if(ISDRUMCHANNEL(i)) {
        Panel->is_drum[i]=1;
        barcol[i]=app_resources.drumvelocity_color;
      }
      else {
        barcol[i]=app_resources.velocity_color;
      }
      inst_name[i] = (char *)safe_malloc(sizeof(char) * INST_NAME_SIZE);
    }
    rimcolor = app_resources.rim_color;
    boxcolor = app_resources.box_color;
    capcolor = app_resources.caption_color;
    black = app_resources.black_key_color;
    white = app_resources.white_key_color;
    suscolor = app_resources.sus_color;
    playcolor = app_resources.play_color;
    revcolor = app_resources.rev_color;
    chocolor = app_resources.cho_color;
    XSetFont(disp, gct, tracefont->fid);

    keyG = (ThreeL *)safe_malloc(sizeof(ThreeL) * KEY_NUM);
    for(i=0, j= BARH_OFS8+1; i<KEY_NUM; i++) {
      tmpi = i%12;
      switch(tmpi) {
      case 0:
      case 5:
      case 10:
        keyG[i].k[0].y = 11; keyG[i].k[0].l = 7;
        keyG[i].k[1].y = 2; keyG[i].k[1].l = 16;
        keyG[i].k[2].y = 11; keyG[i].k[2].l = 7;
        keyG[i].col = white;
        break;
      case 2:
      case 7:
        keyG[i].k[0].y = 11; keyG[i].k[0].l = 7;
        keyG[i].k[1].y = 2; keyG[i].k[1].l = 16;
        keyG[i].k[2].y = 2; keyG[i].k[2].l = 16;        
        keyG[i].col = white; break;
      case 3:
      case 8:
        j += 2;
        keyG[i].k[0].y = 2; keyG[i].k[0].l = 16;
        keyG[i].k[1].y = 2; keyG[i].k[1].l = 16;
        keyG[i].k[2].y = 11; keyG[i].k[2].l = 7;
        keyG[i].col = white; break;
      default:  /* black key */
        keyG[i].k[0].y = 2; keyG[i].k[0].l = 8;
        keyG[i].k[1].y = 2; keyG[i].k[1].l = 8;
        keyG[i].k[2].y = 2; keyG[i].k[2].l = 8;
        keyG[i].col = black; break;
      }
      keyG[i].xofs = j; j += 2;
    }

    /* draw on template pixmaps that includes one channel row */
    for(i=0; i<2; i++) {
      layer[i] = XCreatePixmap(disp,XtWindow(trace),TRACE_WIDTH,BAR_SPACE,
                               DefaultDepth(disp,screen));
      drawKeyboardAll(disp, layer[i]);
      XSetForeground(disp, gc, capcolor);
      XDrawLine(disp,layer[i],gc,0,0,TRACE_WIDTH,0);
      XDrawLine(disp,layer[i],gc,0,0,0,BAR_SPACE);
      XDrawLine(disp,layer[i],gc,TRACE_WIDTH-1,0,TRACE_WIDTH-1,BAR_SPACE);

      for(j=0; j<pl[i].col -1; j++) {
        int w;
        tmpi= TRACEH_OFS; w= pl[i].w[j];
        for(k= 0; k<j; k++) tmpi += pl[i].w[k];
        tmpi = pl[i].ofs[j];
        XSetForeground(disp,gc, capcolor);
        XDrawLine(disp,layer[i],gc, tmpi+w,0,tmpi+w,BAR_SPACE);
        XSetForeground(disp,gc, rimcolor);
        XDrawLine(disp,layer[i],gc,tmpi+w-2,2,tmpi+w-2,BAR_HEIGHT+1);
        XDrawLine(disp,layer[i],gc,tmpi+2,BAR_HEIGHT+2,tmpi+w-2,BAR_HEIGHT+2);
        XSetForeground(disp,gc, ((j)? boxcolor:textbgcolor));
        XFillRectangle(disp,layer[i],gc,tmpi+2,2,w-4,BAR_HEIGHT);
      }
    }
    initStatus();
    XFreeGC(disp,gc);
    voices_num_width = XTextWidth(tracefont,"Voices",6) +VOICES_NUM_OFS;
  }
  while (1) {
    a_pipe_read(local_buf,sizeof(local_buf));
    if (local_buf[0] < 'A') break;
    a_print_text(lyric_t,local_buf+2);
  }
  bsb=XtVaCreateManagedWidget("dummyfile",smeLineObjectClass,title_sm,XtNforeground,textbgcolor,NULL);
  init_options=atoi(local_buf);
  a_pipe_read(local_buf,sizeof(local_buf));
  init_chorus=atoi(local_buf);

  a_pipe_read(local_buf,sizeof(local_buf));
  max_files=atoi(local_buf);
  flist=(String *)safe_malloc((INIT_FLISTNUM+1)*sizeof(char *));
  for (i=0;i<max_files;i++) {
    a_pipe_read(local_buf,sizeof(local_buf));
    addOneFile(max_files,i,local_buf,True);
  }
  for(i=0;i<dot_nfile;i++) {
    snprintf(local_buf,sizeof(local_buf),"X %s\n",dotfile_flist[i]);
    free(dotfile_flist[i]);
    a_pipe_write(local_buf);
  }
  free(dotfile_flist);
  for (i = 0; i < XtNumber(file_menu); i++) {
    bsb=XtVaCreateManagedWidget(file_menu[i].name,
            (file_menu[i].trap ? smeBSBObjectClass:smeLineObjectClass),
            file_sm,XtNleftBitmap,None,XtNleftMargin,24,NULL);
    XtAddCallback(bsb,XtNcallback,filemenuCB,(XtPointer)&file_menu[i].id);
    file_menu[i].widget = bsb;
  }
  if(!ctl->trace_playing || !Cfg.disptrace) {
    Dimension w2,h2,h;
    XtVaSetValues(lyric_t,XtNwidth,rotatewidth[0] -12,NULL);
    XtVaGetValues(toplevel,XtNheight,&h,NULL);
    XtMakeResizeRequest(toplevel,rotatewidth[0],h,&w2,&h2);
  }
  if (!ctl->trace_playing)
      XtVaSetValues(file_menu[ID_HIDETRACE-100].widget,XtNsensitive,False,NULL);
  /* Please sleep here to make widgets arrange by Form Widget,
   * otherwise the widget geometry is broken.
   * Strange!!
   */
  if(Cfg.hidetext || !Cfg.disptrace) {
    XSync(disp, False);
    usleep(10000);
  }
  sprintf(cbuf,"%d",amplitude);
  XtVaSetValues(vol_l,XtNlabel,cbuf,NULL);
  sprintf(local_buf,"V %d\n",amplitude);
  a_pipe_write(local_buf);
  if (init_options == DEFAULT_OPTIONS) init_options=Cfg.extendopt;
  if (init_chorus == DEFAULT_CHORUS) init_chorus=Cfg.chorusopt;
  sprintf(local_buf,"E %03d",init_options);
  a_pipe_write(local_buf);
  sprintf(local_buf,"C %03d",init_chorus);
  a_pipe_write(local_buf);

  if(Cfg.autostart)
    filemenuCB(file_menu[ID_AUTOSTART-100].widget,
               &file_menu[ID_AUTOSTART-100].id,NULL);
  if(Cfg.autoexit) {
    filemenuCB(file_menu[ID_AUTOQUIT-100].widget,
               &file_menu[ID_AUTOQUIT-100].id,NULL);
  }
  if(Cfg.hidetext)
    filemenuCB(file_menu[ID_HIDETXT-100].widget,
        &file_menu[ID_HIDETXT-100].id,NULL);
  if(!Cfg.disptrace)
    filemenuCB(file_menu[ID_HIDETRACE-100].widget,
               &file_menu[ID_HIDETRACE-100].id,NULL);

  if(Cfg.repeat) repeatCB(NULL,&Cfg.repeat,NULL);
  if(Cfg.shuffle) randomCB(NULL,&Cfg.shuffle,NULL);
  if(Cfg.autostart) {
    if(max_files==0)
      prevCB(NULL,NULL,NULL);
    else
      playCB(NULL,NULL,NULL);
  } else
    stopCB(NULL,NULL,NULL);
  if(ctl->trace_playing) initStatus();
  XtAppMainLoop(app_con);
}

static void safe_getcwd(char *cwd, int maxlen)
{
#ifndef STDC_HEADERS
  if(!getwd(cwd))
#else
  if(!getcwd(cwd, maxlen))
#endif
  {
    ctl->cmsg(CMSG_WARNING, VERB_NORMAL,
	      "Warning: Can't get current working directory");
    strcpy(cwd, ".");
  }
}

#include "interface.h"
#if defined(IA_MOTIF)
/*
 * Switch -lXm's vendorShellWidgetClass to -lXaw's vendorShellWidgetClass
 */
#define vendorShellClassRec xaw_vendorShellClassRec
#define vendorShellWidgetClass xaw_vendorShellWidgetClass
#include "xaw_redef.c"
#undef vendorShellClassRec
#undef vendorShellWidgetClass
extern WidgetClass vendorShellWidgetClass;
extern VendorShellClassRec vendorShellClassRec;
static void xaw_vendor_setup(void)
{
    memcpy(&vendorShellClassRec, &xaw_vendorShellClassRec,
	   sizeof(VendorShellClassRec));
    vendorShellWidgetClass = (WidgetClass)&xaw_vendorShellWidgetClass;
}
#else
static void xaw_vendor_setup(void) { }
#endif
