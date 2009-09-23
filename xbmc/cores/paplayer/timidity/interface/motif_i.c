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

    motif_interface.c: written by Vincent Pagel (pagel@loria.fr) 10/4/95

    Policy : I choose to make a separate process for a TIMIDITY motif
    interface for TIMIDITY (if the interface was in the same process
    X redrawings would interfere with the audio computation. Besides
    XtAppAddWorkProc mechanism is not easily controlable)

    The solution : I create a pipe between Timidity and the forked interface
    and use XtAppAddInput to watch the data arriving on the pipe.

    10/4/95
      - Initial working version with prev, next, and quit button and a
        text display

    17/5/95
      - Add timidity icon with filename displaying to play midi while
        I work without having that big blue window in the corner of
        my screen :)
      - Solve the problem of concurent scale value modification

    21/5/95
      - Add menus, file selection box

    14/6/95
      - Make the visible part of file list follow the selection

   */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdarg.h>

#include <Xm/Xm.h>
#include <Xm/Form.h>
#include <Xm/Text.h>
#include <Xm/PushB.h>
#include <Xm/Label.h>
#include <Xm/Scale.h>
#include <Xm/List.h>
#include <Xm/Frame.h>
#include <Xm/RowColumn.h>
#include <Xm/CascadeB.h>
#include <Xm/FileSB.h>
#include <Xm/FileSB.h>
#include <Xm/ToggleB.h>

#include "timidity.h"
#include "common.h"
#include "instrum.h"
#include "playmidi.h"
#include "output.h"
#include "controls.h"
#include "motif.h"

static XtAppContext context;
static XmStringCharSet char_set=XmSTRING_DEFAULT_CHARSET;

static Widget toplevel;

static Widget text;
static XmTextPosition wpr_position=0;

static Widget mainForm;

static Widget menu_bar, open_option, quit_option, auto_next_option;
static Widget open_dialog , add_all_button;

static Widget btnForm, backBtn,fwdBtn, restartBtn, pauseBtn, quitBtn,
	      nextBtn, prevBtn;

static Pixmap backPixmap, fwdPixmap, pausePixmap , restartPixmap,
	      prevPixmap, nextPixmap, quitPixmap,
	      timidityPixmap;

static Widget countFrame, countForm, counterlbl, totlbl , count_headlbl;
static int last_sec=0;
static int max_sec=0;

static Widget file_namelbl, file_headlbl;

static Widget volume_scale , locator_scale ;
static Boolean locator_scale_button= ButtonRelease;

static Widget file_list;
static int file_number_to_play; /* Number of the file we're playing in the list */

/*
 * CREATE PIXMAPS FOR THE BUTONS
 */
static void CreatePixmaps(Widget parent)
{
  /*
   *
   * BITMAPS
   */
#include "motif_bitmaps/back.xbm"
#include "motif_bitmaps/next.xbm"
#include "motif_bitmaps/prev.xbm"
#include "motif_bitmaps/restart.xbm"
#include "motif_bitmaps/fwd.xbm"
#include "motif_bitmaps/pause.xbm"
#include "motif_bitmaps/quit.xbm"
#include "motif_bitmaps/timidity.xbm"

    Display *disp;
    Drawable d;
    Pixel fg,bg;
    int ac;
    Arg al[20];
    unsigned int depth=DefaultDepthOfScreen(XtScreen(toplevel));

    ac = 0;
    XtSetArg(al[ac], XmNbackground, &bg); ac++;
    XtSetArg(al[ac], XmNforeground, &fg); ac++;
    XtGetValues(parent, al, ac);

    disp=XtDisplay(toplevel);
    d=RootWindowOfScreen(XtScreen(toplevel));

    backPixmap = XCreatePixmapFromBitmapData(disp, d,
					     back_bits, back_width, back_height,
					     fg, bg,depth);
    fwdPixmap = XCreatePixmapFromBitmapData( disp, d,
					     fwd_bits, fwd_width, fwd_height,
					     fg, bg,depth);
    pausePixmap = XCreatePixmapFromBitmapData(disp, d,
					pause_bits, pause_width, pause_height,
					fg, bg,depth);

    restartPixmap = XCreatePixmapFromBitmapData(disp, d,
					  restart_bits, restart_width, restart_height,
					  fg, bg,depth);

    nextPixmap  = XCreatePixmapFromBitmapData(disp, d,
					next_bits, next_width, next_height,
					fg, bg,depth);

    prevPixmap  = XCreatePixmapFromBitmapData(disp, d,
					prev_bits, prev_width, prev_height,
					fg, bg,depth);

    quitPixmap  = XCreatePixmapFromBitmapData(disp, d,
					quit_bits, quit_width, quit_height,
					fg, bg,depth);

    timidityPixmap  = XCreatePixmapFromBitmapData(disp, d,
						  timidity_bits, timidity_width, timidity_height,
						  WhitePixelOfScreen(XtScreen(toplevel)),
						  BlackPixelOfScreen(XtScreen(toplevel)),depth);
}


/************************************
 *                                  *
 * ALL THE INTERFACE'S CALLBACKS    *
 *                                  *
 ************************************/

/*
 * Generic buttons callbacks ( Transport Buttons )
 */
/*ARGSUSED*/
static void  GenericCB(Widget widget, int data, XtPointer call_data)
{

	if(data != MOTIF_QUIT && data != MOTIF_NEXT && data != MOTIF_PREV)
	{
		Arg al[10];
		int ac, nbfile;

		ac=0;
		XtSetArg(al[ac], XmNitemCount, &nbfile); ac++;
		XtGetValues(file_list, al, ac);
		if(file_number_to_play==0||file_number_to_play>nbfile)
			return;
	}
    m_pipe_int_write( data );
}

/*
 *  Generic scales callbacks : VOLUME and LOCATOR
 */
/*ARGSUSED*/
static void Generic_scaleCB(Widget widget, int data, XtPointer call_data)
{
    XmScaleCallbackStruct *cbs = (XmScaleCallbackStruct *) call_data;

	if(data == MOTIF_CHANGE_LOCATOR)
	{
		Arg al[10];
		int ac, nbfile;

		ac=0;
		XtSetArg(al[ac], XmNitemCount, &nbfile); ac++;
		XtGetValues(file_list, al, ac);
		if(file_number_to_play<=0||file_number_to_play>nbfile)
		{
			XmScaleSetValue(locator_scale,0);
			return;
		}
	}
    m_pipe_int_write(  data );
    m_pipe_int_write(cbs->value);
}

/*
 * Detect when a mouse button is pushed or released in a scale area to
 * avoid concurent scale value modification while holding it with the mouse
 */
/*ARGSUSED*/
static void Locator_btn(Widget w,XtPointer client_data,XEvent *event,Boolean *cont)
{
    /* Type = ButtonPress or ButtonRelease */
    locator_scale_button= event->xbutton.type;
}

/*
 * File List selection CALLBACK
 */
/*ARGSUSED*/
static void File_ListCB(Widget widget, int data, XtPointer call_data)
{
    XmListCallbackStruct *cbs= (XmListCallbackStruct *) call_data;
    char *text;
    int nbvisible, first_visible ;
    Arg al[10];
    int ac;

    /* First, check that the selected file is really visible in the list */
    ac=0;
    XtSetArg(al[ac],XmNtopItemPosition,&first_visible); ac++;
    XtSetArg(al[ac],XmNvisibleItemCount,&nbvisible); ac++;
    XtGetValues(widget, al, ac);

    if ( ( first_visible > cbs->item_position) ||
         ((first_visible+nbvisible) <= cbs->item_position))
	XmListSetPos(widget, cbs->item_position);

    /* Tell the application to play the requested file */
    XmStringGetLtoR(cbs->item,char_set,&text);
    m_pipe_int_write(MOTIF_PLAY_FILE);
    m_pipe_string_write(text);
    file_number_to_play=cbs->item_position;
    XtFree(text);
}

/*
 * Generic menu callback
 */
/*ARGSUSED*/
static void menuCB(Widget w,int client_data,XmAnyCallbackStruct *call_data)
{
    switch (client_data)
	{
	case MENU_OPEN:
	{
	    XtManageChild(open_dialog);
	}
	break;

	case MENU_QUIT : {
	    m_pipe_int_write(MOTIF_QUIT);
	}
	    break;
	case MENU_TOGGLE : {
	    /* Toggle modified : for the moment , nothing to do ! */
	    /* if (XmToggleButtonGetState(w)) TRUE else FALSE */
	    }
	    break;
	}
}

/*
 * File selection box callback
 */
/*ARGSUSED*/
static void openCB(Widget w,int client_data,XmFileSelectionBoxCallbackStruct *call_data)
{
    if (client_data==DIALOG_CANCEL)
	{ /* do nothing if cancel is selected. */
	    XtUnmanageChild(open_dialog);
	    return;
	}
    else if (client_data==DIALOG_ALL)
	{ /* Add all the listed files  */
	    Arg al[10];
	    int ac;
	    Widget the_list;
	    int nbfile;
	    XmStringTable files;
		char *text;
	    int i;

	    the_list=XmFileSelectionBoxGetChild(open_dialog,XmDIALOG_LIST);
	    if (!XmIsList(the_list))
		{
		    fprintf(stderr, "PANIC: List are not what they used to be"
			    NLS);
		    exit(1);
		}

	    ac=0;
	    XtSetArg(al[ac], XmNitemCount, &nbfile); ac++;
	    XtSetArg(al[ac], XmNitems, &files); ac++;
	    XtGetValues(the_list, al, ac);

	    m_pipe_int_write(MOTIF_EXPAND);
	    m_pipe_int_write(nbfile);
	    for (i=0;i<nbfile;i++)
		{
			XmStringGetLtoR(files[i],char_set,&text);
			m_pipe_string_write(text);
			XtFree(text);
		}
	    XtUnmanageChild(open_dialog);
	}
    else
	{   /* get filename from file selection box and add it to the list*/
		char *text;

	    m_pipe_int_write(MOTIF_EXPAND);
	    m_pipe_int_write(1);
		XmStringGetLtoR(call_data->value,char_set,&text);
	    m_pipe_string_write(text);
		XtFree(text);
	    XtUnmanageChild(open_dialog);
	}
}


/********************************************************
 *                                                      *
 * Receive DATA sent by the application on the pipe     *
 *                                                      *
 ********************************************************/
/*ARGSUSED*/
static void handle_input(client_data, source, id)
    XtPointer client_data;
    int *source;
    XtInputId *id;
{
    int message;

    m_pipe_int_read(&message);

    switch (message)
	{
	case REFRESH_MESSAGE : {
	    fprintf(stderr, "REFRESH MESSAGE IS OBSOLETE !!!" NLS);
	}
	    break;

	case TOTALTIME_MESSAGE : {
	    int tseconds;
	    int minutes,seconds;
	    char local_string[20];
	    Arg al[10];
	    int ac;

	    m_pipe_int_read(&tseconds);

	    seconds=tseconds;
	    minutes=seconds/60;
	    seconds-=minutes*60;
	    sprintf(local_string,"/ %02d:%02d",minutes,seconds);
	    ac=0;
	    XtSetArg(al[ac], XmNlabelString,
		     XmStringCreate(local_string, char_set)); ac++;
	    XtSetValues(totlbl, al, ac);

	    /* Readjust the time scale */
	    XmScaleSetValue(locator_scale,0);
	    ac=0;
	    XtSetArg(al[ac], XmNmaximum, tseconds); ac++;
	    XtSetValues(locator_scale, al, ac);
	    max_sec=tseconds;
	}
	    break;

	case MASTERVOL_MESSAGE: {
	    int volume;

	    m_pipe_int_read(&volume);
	    XmScaleSetValue(volume_scale,volume);
	}
	    break;

	case FILENAME_MESSAGE : {
	    char filename[255], separator[255];
	    Arg al[10];
	    char *pc;
	    int ac, i;
	    short nbcol;

	    m_pipe_string_read(filename);

	    /* Extract basename of the file */
	    pc=strrchr(filename,'/');
	    if (pc==NULL)
		pc=filename;
	    else
		pc++;

	    ac=0;
	    XtSetArg(al[ac], XmNlabelString,
		     XmStringCreate(pc, char_set)); ac++;
	    XtSetValues(file_namelbl, al, ac);

	    /* Change the icon  */
	    ac=0;
	    XtSetArg(al[ac], XmNiconName,pc); ac++;
	    XtSetValues(toplevel,al,ac);

	    /* Put a separator in the text Window */
	    ac=0;
	    nbcol=10;
	    XtSetArg(al[ac], XmNcolumns,&nbcol); ac++;
	    XtGetValues(text,al,ac);

	    for (i=0;i<nbcol;i++)
		separator[i]='*';
	    separator[i]='\0';

	    XmTextInsert(text,wpr_position, separator);
	    wpr_position+= strlen(separator);
	    XmTextInsert(text,wpr_position++,"\n");
	    XtVaSetValues(text,XmNcursorPosition, wpr_position,NULL);
	    XmTextShowPosition(text,wpr_position);
	}
	    break;

	case FILE_LIST_MESSAGE : {
	    char filename[255];
	    int i, number_of_files;
	    XmString s;

	    /* reset the playing list : play from the start */
	    file_number_to_play=0;

	    m_pipe_int_read(&number_of_files);

	    for (i=0;i<number_of_files;i++)
		{
		    m_pipe_string_read(filename);
		    s=XmStringCreate(filename,char_set);
		    XmListAddItemUnselected(file_list,s,0);
		    XmStringFree(s);
		}
	}
	    break;

	case ERROR_MESSAGE :
	case NEXT_FILE_MESSAGE :
	case PREV_FILE_MESSAGE :
	case TUNE_END_MESSAGE :{
	    Arg al[10];
	    int ac;
	    int nbfile;
	    XmStringTable files;

	    /* When a file ends, launch next if auto_next toggle */
	    if ((message==TUNE_END_MESSAGE) &&
		!XmToggleButtonGetState(auto_next_option))
		return;

	    /* Total number of file to play in the list */
	    ac=0;
	    XtSetArg(al[ac], XmNitemCount, &nbfile); ac++;
	    XtGetValues(file_list, al, ac);
	    XmListDeselectAllItems(file_list);

	    if (message==NEXT_FILE_MESSAGE||message==TUNE_END_MESSAGE)
		file_number_to_play++;
	    else if (message==PREV_FILE_MESSAGE)
		file_number_to_play--;
		else
		{
			ac=0;
			XtSetArg(al[ac], XmNitems, &files); ac++;
			XtGetValues(file_list, al, ac);
			XmListDeleteItem(file_list, files[file_number_to_play-1]);
			XmListSelectPos(file_list,file_number_to_play,TRUE);
			return;
		}

	    /* Do nothing if requested file is before first one */
	    if (file_number_to_play<=0)
		{
			file_number_to_play=0;
			XmScaleSetValue(locator_scale,0);
		    return;
		}

	    /* Stop after playing the last file */
	    if (file_number_to_play>nbfile)
		{
			file_number_to_play=nbfile+1;
			XmScaleSetValue(locator_scale,0);
		    return;
		}

	    XmListSelectPos(file_list,file_number_to_play,TRUE);
	}
	    break;

	case CURTIME_MESSAGE : {
	    int tseconds;
	    int  sec,seconds, minutes;
	    int nbvoice;
	    char local_string[20];
	    Arg al[10];
	    int ac;

	    m_pipe_int_read(&tseconds);
	    m_pipe_int_read(&nbvoice);

	    sec=seconds=tseconds;

				/* To avoid blinking */
	    if (sec!=last_sec)
		{
		    minutes=seconds/60;
		    seconds-=minutes*60;

		    sprintf(local_string,"%02d:%02d",
			    minutes, seconds);

		    ac=0;
		    XtSetArg(al[ac], XmNlabelString,
			     XmStringCreate(local_string, char_set)); ac++;
		    XtSetValues(counterlbl, al, ac);
		}

	    last_sec=sec;

	    /* Readjust the time scale if not dragging the scale */
	    if ( (locator_scale_button==ButtonRelease) &&
		 (tseconds<max_sec))
		XmScaleSetValue(locator_scale, tseconds);
	}
	    break;

	case NOTE_MESSAGE : {
	    int channel;
	    int note;

	    m_pipe_int_read(&channel);
	    m_pipe_int_read(&note);
#ifdef DEBUG
	    printf("NOTE chn%i %i\n",channel,note);
#endif /* DEBUG */
	}
	    break;

	case    PROGRAM_MESSAGE :{
	    int channel;
	    int pgm;

	    m_pipe_int_read(&channel);
	    m_pipe_int_read(&pgm);
#ifdef DEBUG
	    printf("NOTE chn%i %i\n",channel,pgm);
#endif /* DEBUG */
	}
	    break;

	case VOLUME_MESSAGE : {
	    int channel;
	    int volume;

	    m_pipe_int_read(&channel);
	    m_pipe_int_read(&volume);
#ifdef DEBUG
	    printf("VOLUME= chn%i %i \n",channel, volume);
#endif /* DEBUG */
	}
	    break;


	case EXPRESSION_MESSAGE : {
	    int channel;
	    int express;

	    m_pipe_int_read(&channel);
	    m_pipe_int_read(&express);
#ifdef DEBUG
	    printf("EXPRESSION= chn%i %i \n",channel, express);
#endif /* DEBUG */
	}
	    break;

	case PANNING_MESSAGE : {
	    int channel;
	    int pan;

	    m_pipe_int_read(&channel);
	    m_pipe_int_read(&pan);
#ifdef DEBUG
	    printf("PANNING= chn%i %i \n",channel, pan);
#endif /* DEBUG */
	}
	    break;

	case  SUSTAIN_MESSAGE : {
	    int channel;
	    int sust;

	    m_pipe_int_read(&channel);
	    m_pipe_int_read(&sust);
#ifdef DEBUG
	    printf("SUSTAIN= chn%i %i \n",channel, sust);
#endif /* DEBUG */
	}
	    break;

	case  PITCH_MESSAGE : {
	    int channel;
	    int bend;

	    m_pipe_int_read(&channel);
	    m_pipe_int_read(&bend);
#ifdef DEBUG
	    printf("PITCH BEND= chn%i %i \n",channel, bend);
#endif /* DEBUG */
	}
	    break;

	case RESET_MESSAGE : {
#ifdef DEBUG
	    printf("RESET_MESSAGE\n");
#endif /* DEBUG */
	}
	    break;

	case CLOSE_MESSAGE : {
#ifdef DEBUG
	    printf("CLOSE_MESSAGE\n");
#endif /* DEBUG */
	    exit(0);
	}
	    break;

	case CMSG_MESSAGE : {
	    int type;
	    char message[1000];

	    m_pipe_int_read(&type);
	    m_pipe_string_read(message);

	    XmTextInsert(text,wpr_position, message);
	    wpr_position+= strlen(message);
	    XmTextInsert(text,wpr_position++, "\n");
	    XtVaSetValues(text,XmNcursorPosition, wpr_position,NULL);
	    XmTextShowPosition(text,wpr_position);
	}
	    break;
	case LYRIC_MESSAGE : {
	    char message[1000];

	    m_pipe_string_read(message);

	    XmTextInsert(text, wpr_position, message);
	    wpr_position += strlen(message);
	    XtVaSetValues(text,XmNcursorPosition, wpr_position,NULL);
	    XmTextShowPosition(text,wpr_position);
	    XmTextInsert(text,wpr_position, ""); /* Update cursor */
	}
	    break;
	default:
	    fprintf(stderr,"UNKNOW MOTIF MESSAGE %i" NLS, message);
	}

}

/* ***************************************
 *                                       *
 * Convenience function to create menus  *
 *                                       *
 *****************************************/

/* adds an accelerator to a menu option. */
static void add_accelerator(Widget w,char *acc_text,char *key)
{
    int ac;
    Arg al[10];

    ac=0;
    XtSetArg(al[ac],XmNacceleratorText,
	     XmStringCreate(acc_text,char_set)); ac++;
    XtSetArg(al[ac],XmNaccelerator,key); ac++;
    XtSetValues(w,al,ac);
}

/* Adds a toggle option to an existing menu. */
static Widget make_menu_toggle(char *item_name, int client_data, Widget menu)
{
    int ac;
    Arg al[10];
    Widget item;

    ac = 0;
    XtSetArg(al[ac],XmNlabelString, XmStringCreateLtoR(item_name,
						       char_set)); ac++;
    item=XmCreateToggleButton(menu,item_name,al,ac);
    XtManageChild(item);
    XtAddCallback(item, XmNvalueChangedCallback,
		  (XtCallbackProc) menuCB,(XtPointer) client_data);
    XtSetSensitive(item, True);
    return(item);
}

/* Adds an option to an existing menu. */
static Widget make_menu_option(char *option_name, KeySym mnemonic,
			       int client_data, Widget menu)
{
    int ac;
    Arg al[10];
    Widget b;

    ac = 0;
    XtSetArg(al[ac], XmNlabelString,
	     XmStringCreateLtoR(option_name, char_set)); ac++;
    XtSetArg (al[ac], XmNmnemonic, mnemonic); ac++;
    b=XtCreateManagedWidget(option_name,xmPushButtonWidgetClass,
			    menu,al,ac);
    XtAddCallback (b, XmNactivateCallback,
		   (XtCallbackProc) menuCB, (XtPointer) client_data);
    return(b);
}

/* Creates a new menu on the menu bar. */
static Widget make_menu(char *menu_name,KeySym  mnemonic, Widget menu_bar)
{
    int ac;
    Arg al[10];
    Widget menu, cascade;

    ac = 0;
    menu = XmCreatePulldownMenu (menu_bar, menu_name, al, ac);

    ac = 0;
    XtSetArg (al[ac], XmNsubMenuId, menu); ac++;
    XtSetArg (al[ac], XmNmnemonic, mnemonic); ac++;
    XtSetArg(al[ac], XmNlabelString,
        XmStringCreateLtoR(menu_name, char_set)); ac++;
    cascade = XmCreateCascadeButton (menu_bar, menu_name, al, ac);
    XtManageChild (cascade);

    return(menu);
}

/* *******************************************
 *                                           *
 * Interface initialisation before launching *
 *                                           *
 *********************************************/

static void create_menus(Widget menu_bar)
{
    Widget menu;

    menu=make_menu("File",'F',menu_bar);
    open_option = make_menu_option("Open", 'O', MENU_OPEN, menu);
    add_accelerator(open_option, "meta+o", "Meta<Key>o:");

    quit_option = make_menu_option("Exit", 'E', MENU_QUIT, menu);
    add_accelerator(quit_option, "meta+q", "Meta<Key>q:");

    menu=make_menu("Options",'O',menu_bar);
    auto_next_option= make_menu_toggle("Auto Next", MENU_TOGGLE, menu);
    XmToggleButtonSetState( auto_next_option , True , False );
}

static void create_dialog_boxes(void)
{
    Arg al[10];
    int ac;
    XmString add_all = XmStringCreateLocalized("ADD ALL");

    /* create the file selection box used by MENU_OPEN */
    ac=0;
    XtSetArg(al[ac],XmNmustMatch,True); ac++;
    XtSetArg(al[ac],XmNautoUnmanage,False); ac++;
    XtSetArg(al[ac],XmNdialogTitle,
	     XmStringCreateLtoR("TIMIDITY: Open",char_set)); ac++;
    open_dialog=XmCreateFileSelectionDialog(toplevel,"open_dialog",al,ac);
    XtAddCallback(open_dialog, XmNokCallback,
		  (XtCallbackProc) openCB, (XtPointer) DIALOG_OK);
    XtAddCallback(open_dialog, XmNcancelCallback,
		  (XtCallbackProc) openCB, (XtPointer) DIALOG_CANCEL);
    XtUnmanageChild(XmFileSelectionBoxGetChild(open_dialog, XmDIALOG_HELP_BUTTON));

    ac = 0;
    XtSetArg(al[ac], XmNleftOffset, 10); ac++;
    XtSetArg(al[ac], XmNrightOffset, 10); ac++;
    XtSetArg(al[ac], XmNtopOffset, 10); ac++;
    XtSetArg(al[ac], XmNbottomOffset, 10); ac++;
    XtSetArg(al[ac], XmNlabelString, add_all); ac++;
    add_all_button = XmCreatePushButton(open_dialog, "add_all",al, ac);
    XtManageChild(add_all_button);
    XtAddCallback(add_all_button, XmNactivateCallback,
		  (XtCallbackProc) openCB, (XtPointer) DIALOG_ALL);
}

void Launch_Motif_Process(int pipe_number)
{
    Arg al[20];
    int ac;
    int argc=0;

    XtSetLanguageProc(NULL,NULL,NULL); /* By KINOSHITA, K. <kino@krhm.jvc-victor.co.jp> */

    /* create the toplevel shell */
    toplevel = XtAppInitialize(&context,"timidity",NULL,0,&argc,NULL,
			       NULL,NULL,0);

    /*******************/
    /* Main form       */
    /*******************/
    ac=0;
    XtSetArg(al[ac],XmNtopAttachment,XmATTACH_FORM); ac++;
    XtSetArg(al[ac],XmNbottomAttachment,XmATTACH_FORM); ac++;
    XtSetArg(al[ac],XmNrightAttachment,XmATTACH_FORM); ac++;
    XtSetArg(al[ac],XmNleftAttachment,XmATTACH_FORM); ac++;

    mainForm=XmCreateForm(toplevel,"form",al,ac);
    XtManageChild(mainForm);

    CreatePixmaps(mainForm);


    /* create a menu bar and attach it to the form. */
    ac=0;
    XtSetArg(al[ac], XmNtopAttachment,   XmATTACH_FORM); ac++;
    XtSetArg(al[ac], XmNrightAttachment, XmATTACH_FORM); ac++;
    XtSetArg(al[ac], XmNleftAttachment,  XmATTACH_FORM); ac++;
    menu_bar=XmCreateMenuBar(mainForm,"menu_bar",al,ac);
    XtManageChild(menu_bar);

    create_dialog_boxes();
    create_menus(menu_bar);

    /*******************/
    /* Message window  */
    /*******************/

    ac=0;
    XtSetArg(al[ac], XmNleftOffset, 10); ac++;
    XtSetArg(al[ac], XmNrightOffset, 10); ac++;
    XtSetArg(al[ac], XmNtopOffset, 10); ac++;
    XtSetArg(al[ac], XmNbottomOffset, 10); ac++;
    XtSetArg(al[ac],XmNtopAttachment,XmATTACH_WIDGET); ac++;
    XtSetArg(al[ac],XmNtopWidget, menu_bar); ac++;
    XtSetArg(al[ac],XmNrightAttachment,XmATTACH_FORM); ac++;
    XtSetArg(al[ac],XmNleftAttachment,XmATTACH_FORM); ac++;
    XtSetArg(al[ac],XmNeditMode,XmMULTI_LINE_EDIT); ac++;
    XtSetArg(al[ac],XmNrows,10); ac++;
    XtSetArg(al[ac],XmNcolumns,10); ac++;
    XtSetArg(al[ac],XmNeditable, False); ac++;
    XtSetArg(al[ac],XmNwordWrap, True); ac++;
    XtSetArg(al[ac],XmNvalue, "TIMIDIY RUNNING...\n"); ac++;
    wpr_position+= strlen("TIMIDIY RUNNING...\n");

    text=XmCreateScrolledText(mainForm,"text",al,ac);
    XtManageChild(text);

    /********************/
    /* File_name label  */
    /********************/
    ac = 0;
    XtSetArg(al[ac], XmNleftOffset, 20); ac++;
    XtSetArg(al[ac], XmNrightOffset, 10); ac++;
    XtSetArg(al[ac], XmNtopOffset, 20); ac++;
    XtSetArg(al[ac], XmNbottomOffset, 20); ac++;
    XtSetArg(al[ac], XmNlabelType, XmSTRING); ac++;
    XtSetArg(al[ac], XmNtopAttachment, XmATTACH_WIDGET); ac++;
    XtSetArg(al[ac], XmNtopWidget, text); ac++;
    XtSetArg(al[ac], XmNleftAttachment,XmATTACH_FORM); ac++;
    XtSetArg(al[ac], XmNtraversalOn, False); ac++;
    XtSetArg(al[ac], XmNhighlightThickness,0); ac++;
    XtSetArg(al[ac], XmNalignment,XmALIGNMENT_END); ac++;
    XtSetArg(al[ac], XmNlabelString,
	     XmStringCreate("Playing:",char_set)); ac++;
    file_headlbl = XmCreateLabel(mainForm,"fileheadlbl",al,ac);
    XtManageChild(file_headlbl);


    ac = 0;
    XtSetArg(al[ac], XmNrightOffset, 10); ac++;
    XtSetArg(al[ac], XmNtopOffset, 20); ac++;
    XtSetArg(al[ac], XmNbottomOffset, 20); ac++;
    XtSetArg(al[ac], XmNlabelType, XmSTRING); ac++;
    XtSetArg(al[ac], XmNtopAttachment, XmATTACH_WIDGET); ac++;
    XtSetArg(al[ac], XmNtopWidget, text); ac++;
    XtSetArg(al[ac], XmNleftAttachment,XmATTACH_WIDGET); ac++;
    XtSetArg(al[ac], XmNleftWidget,file_headlbl); ac++;
    XtSetArg(al[ac], XmNtraversalOn, False); ac++;
    XtSetArg(al[ac], XmNhighlightThickness,0); ac++;
    XtSetArg(al[ac], XmNalignment,XmALIGNMENT_BEGINNING); ac++;
    XtSetArg(al[ac], XmNlabelString,
	     XmStringCreate("NONE           ",char_set)); ac++;
    file_namelbl = XmCreateLabel(mainForm,"filenameLbl",al,ac);
    XtManageChild(file_namelbl);

    /*****************************/
    /* TIME LABELS IN A FORM     */
    /*****************************/

    /* Counters frame    */
    ac=0;
    XtSetArg(al[ac], XmNtopOffset, 10); ac++;
    XtSetArg(al[ac], XmNbottomOffset, 10); ac++;
    XtSetArg(al[ac], XmNleftOffset, 10); ac++;
    XtSetArg(al[ac], XmNrightOffset, 10); ac++;
    XtSetArg(al[ac],XmNtopAttachment,XmATTACH_WIDGET); ac++;
    XtSetArg(al[ac],XmNtopWidget,text); ac++;
    XtSetArg(al[ac],XmNrightAttachment,XmATTACH_FORM); ac++;
    /*
      XtSetArg(al[ac],XmNleftAttachment,XmATTACH_WIDGET); ac++;
      XtSetArg(al[ac],XmNleftWidget,file_namelbl); ac++;
      */
    XtSetArg(al[ac],XmNshadowType,XmSHADOW_OUT); ac++;
    countFrame=XmCreateFrame(mainForm,"countframe",al,ac);
    XtManageChild(countFrame);

    /* Counters form       */
    ac=0;
    XtSetArg(al[ac],XmNtopAttachment,XmATTACH_FORM); ac++;
    XtSetArg(al[ac],XmNbottomAttachment,XmATTACH_FORM); ac++;
    XtSetArg(al[ac],XmNrightAttachment,XmATTACH_FORM); ac++;
    XtSetArg(al[ac],XmNleftAttachment,XmATTACH_FORM); ac++;
    XtSetArg(al[ac],XmNleftAttachment,XmATTACH_FORM); ac++;

    countForm=XmCreateForm(countFrame,"countform",al,ac);
    XtManageChild(countForm);

    /* HEADER label       */
    ac = 0;
    XtSetArg(al[ac], XmNtopOffset, 10); ac++;
    XtSetArg(al[ac], XmNlabelType, XmSTRING); ac++;
    XtSetArg(al[ac], XmNtopAttachment, XmATTACH_FORM); ac++;
    XtSetArg(al[ac], XmNleftAttachment,XmATTACH_FORM); ac++;
    XtSetArg(al[ac], XmNrightAttachment,XmATTACH_FORM); ac++;
    XtSetArg(al[ac], XmNtraversalOn, False); ac++;
    XtSetArg(al[ac], XmNhighlightThickness,0); ac++;
    XtSetArg(al[ac], XmNlabelString,
	     XmStringCreate("Time:",char_set)); ac++;
    count_headlbl = XmCreateLabel(countForm,"countheadLbl",al,ac);
    XtManageChild(count_headlbl);

    /* current Time label       */
    ac = 0;
    XtSetArg(al[ac], XmNtopAttachment, XmATTACH_WIDGET); ac++;
    XtSetArg(al[ac], XmNtopWidget, count_headlbl); ac++;
    XtSetArg(al[ac], XmNleftAttachment,XmATTACH_FORM); ac++;
    XtSetArg(al[ac], XmNbottomAttachment,XmATTACH_FORM); ac++;
    XtSetArg(al[ac], XmNtraversalOn, False); ac++;
    XtSetArg(al[ac], XmNhighlightThickness,0); ac++;
    XtSetArg(al[ac], XmNalignment,XmALIGNMENT_END); ac++;
    XtSetArg(al[ac], XmNlabelString,
	     XmStringCreate("00:00",char_set)); ac++;
    counterlbl = XmCreateLabel(countForm,"counterLbl",al,ac);
    XtManageChild(counterlbl);

    /* Total time label */

    ac = 0;
    XtSetArg(al[ac], XmNtopAttachment, XmATTACH_WIDGET); ac++;
    XtSetArg(al[ac], XmNtopWidget, count_headlbl); ac++;
    XtSetArg(al[ac], XmNleftAttachment, XmATTACH_WIDGET); ac++;
    XtSetArg(al[ac], XmNleftWidget, counterlbl); ac++;
    XtSetArg(al[ac], XmNrightAttachment, XmATTACH_FORM); ac++;
    XtSetArg(al[ac], XmNbottomAttachment,XmATTACH_FORM); ac++;
    XtSetArg(al[ac], XmNtraversalOn, False); ac++;
    XtSetArg(al[ac], XmNhighlightThickness,0); ac++;
    XtSetArg(al[ac], XmNalignment,XmALIGNMENT_BEGINNING); ac++;
    XtSetArg(al[ac], XmNlabelString,
	     XmStringCreate("/ 00:00",char_set)); ac++;
    totlbl = XmCreateLabel(countForm,"TotalTimeLbl",al,ac);
    XtManageChild(totlbl);

    /******************/
    /* Locator Scale  */
    /******************/
    {	/*
	 * We need to add an xevent manager for buton pressing since
	 * locator_scale is a critical ressource that can be modified
	 * by shared by the handle input function
	 */

	WidgetList WList;
	Cardinal Card;

	ac = 0;
	XtSetArg(al[ac], XmNleftOffset, 10); ac++;
	XtSetArg(al[ac], XmNrightOffset, 10); ac++;
	XtSetArg(al[ac], XmNtopOffset, 10); ac++;
	XtSetArg(al[ac], XmNbottomOffset, 10); ac++;
	XtSetArg(al[ac], XmNtopAttachment, XmATTACH_WIDGET); ac++;
	XtSetArg(al[ac], XmNtopWidget, countForm); ac++;
	XtSetArg(al[ac], XmNleftAttachment,XmATTACH_FORM); ac++;
	XtSetArg(al[ac], XmNrightAttachment,XmATTACH_FORM); ac++;
	XtSetArg(al[ac], XmNmaximum, 100); ac++;
	XtSetArg(al[ac], XmNminimum, 0); ac++;
	XtSetArg(al[ac], XmNshowValue, True); ac++;
	XtSetArg(al[ac], XmNdecimalPoints, 0); ac++;
	XtSetArg(al[ac], XmNorientation, XmHORIZONTAL); ac++;
	XtSetArg(al[ac], XmNtraversalOn, False); ac++;
	XtSetArg(al[ac], XmNhighlightThickness,0); ac++;
	locator_scale = XmCreateScale(mainForm,"locator",al,ac);
	XtManageChild(locator_scale);
	XtAddCallback(locator_scale,XmNvalueChangedCallback,
		      (XtCallbackProc) Generic_scaleCB,
		      (XtPointer) MOTIF_CHANGE_LOCATOR);

	/* Reach the scrollbar child in the scale  */
	ac = 0;
	XtSetArg(al[ac], XtNchildren, &WList); ac++;
	XtSetArg(al[ac], XtNnumChildren, &Card); ac++;
	XtGetValues(locator_scale,al,ac);
	if ((Card!=2)||
	    strcmp(XtName(WList[1]),"Scrollbar"))
	    fprintf(stderr,"PANIC: Scale has be redefined.. may cause bugs\n");

 	XtAddEventHandler(WList[1],ButtonPressMask|ButtonReleaseMask,
			  FALSE,Locator_btn,NULL);
    }

    /*****************************/
    /* Control buttons in a form */
    /*****************************/

    /* create form for the row of control buttons */
    ac = 0;
    XtSetArg(al[ac], XmNtopOffset, 10); ac++;
    XtSetArg(al[ac], XmNbottomOffset, 10); ac++;
    XtSetArg(al[ac], XmNleftAttachment, XmATTACH_FORM); ac++;
/*    XtSetArg(al[ac], XmNrightAttachment, XmATTACH_FORM); ac++; */
    XtSetArg(al[ac],XmNtopAttachment,XmATTACH_WIDGET); ac++;
    XtSetArg(al[ac], XmNtopWidget, locator_scale); ac++;
    btnForm = XmCreateForm(mainForm,"btnForm", al, ac);
    XtManageChild(btnForm);

    /* Previous Button */
    ac = 0;
    XtSetArg(al[ac], XmNtopAttachment, XmATTACH_FORM); ac++;
    XtSetArg(al[ac], XmNleftAttachment, XmATTACH_FORM); ac++;
    XtSetArg(al[ac], XmNleftOffset, 10); ac++;
    XtSetArg(al[ac], XmNshadowType, XmSHADOW_OUT); ac++;
    XtSetArg(al[ac], XmNshadowThickness, 2); ac++;
    XtSetArg(al[ac], XmNlabelType, XmPIXMAP); ac++;
    XtSetArg(al[ac], XmNlabelPixmap, prevPixmap); ac++;
    XtSetArg(al[ac], XmNhighlightThickness, 2); ac++;
    prevBtn = XmCreatePushButton(btnForm, "previous",al, ac);
    XtManageChild(prevBtn);
    XtAddCallback(prevBtn, XmNactivateCallback,
		  (XtCallbackProc) GenericCB, (XtPointer) MOTIF_PREV);


    /* Backward Button */
    ac = 0;
    XtSetArg(al[ac], XmNtopAttachment, XmATTACH_FORM); ac++;
    XtSetArg(al[ac], XmNleftAttachment, XmATTACH_WIDGET); ac++;
    XtSetArg(al[ac], XmNleftWidget, prevBtn); ac++;
    XtSetArg(al[ac], XmNleftOffset, 2); ac++;
    XtSetArg(al[ac], XmNshadowType, XmSHADOW_OUT); ac++;
    XtSetArg(al[ac], XmNshadowThickness, 2); ac++;
    XtSetArg(al[ac], XmNlabelType, XmPIXMAP); ac++;
    XtSetArg(al[ac], XmNlabelPixmap, backPixmap); ac++;
    XtSetArg(al[ac], XmNhighlightThickness, 2); ac++;
    backBtn = XmCreatePushButton(btnForm, "backward",al, ac);
    XtManageChild(backBtn);
    XtAddCallback(backBtn, XmNactivateCallback,
		  (XtCallbackProc) GenericCB, (XtPointer) MOTIF_RWD);

    /* Restart Button */
    ac = 0;
    XtSetArg(al[ac], XmNtopAttachment, XmATTACH_FORM); ac++;
    XtSetArg(al[ac], XmNleftAttachment, XmATTACH_WIDGET); ac++;
    XtSetArg(al[ac], XmNleftWidget, backBtn); ac++;
    XtSetArg(al[ac], XmNleftOffset, 2); ac++;
    XtSetArg(al[ac], XmNshadowType, XmSHADOW_OUT); ac++;
    XtSetArg(al[ac], XmNshadowThickness, 2); ac++;
    XtSetArg(al[ac], XmNlabelType, XmPIXMAP); ac++;
    XtSetArg(al[ac], XmNlabelPixmap, restartPixmap); ac++;
    XtSetArg(al[ac], XmNhighlightThickness, 2); ac++;
    restartBtn = XmCreatePushButton(btnForm,"restartBtn", al, ac);
    XtManageChild(restartBtn);
    XtAddCallback(restartBtn, XmNactivateCallback,
		  (XtCallbackProc) GenericCB, (XtPointer) MOTIF_RESTART);

    /* Quit Button */
    ac = 0;
    XtSetArg(al[ac], XmNtopAttachment, XmATTACH_FORM); ac++;
    XtSetArg(al[ac], XmNleftAttachment, XmATTACH_WIDGET); ac++;
    XtSetArg(al[ac], XmNleftWidget, restartBtn); ac++;
    XtSetArg(al[ac], XmNleftOffset, 2); ac++;
    XtSetArg(al[ac], XmNshadowType, XmSHADOW_OUT); ac++;
    XtSetArg(al[ac], XmNshadowThickness, 2); ac++;
    XtSetArg(al[ac], XmNlabelType, XmPIXMAP); ac++;
    XtSetArg(al[ac], XmNlabelPixmap, quitPixmap); ac++;
    XtSetArg(al[ac], XmNhighlightThickness, 2); ac++;
    quitBtn = XmCreatePushButton(btnForm,"quitBtn", al, ac);
    XtManageChild(quitBtn);
    XtAddCallback(quitBtn, XmNactivateCallback,
		  (XtCallbackProc) GenericCB, (XtPointer) MOTIF_QUIT);

    /* Pause Button */

    ac = 0;
    XtSetArg(al[ac], XmNtopAttachment, XmATTACH_FORM); ac++;
    XtSetArg(al[ac], XmNleftAttachment, XmATTACH_WIDGET); ac++;
    XtSetArg(al[ac], XmNleftWidget, quitBtn); ac++;
    XtSetArg(al[ac], XmNshadowType, XmSHADOW_OUT); ac++;
    XtSetArg(al[ac], XmNshadowThickness, 2); ac++;
    XtSetArg(al[ac], XmNlabelType, XmPIXMAP); ac++;
    XtSetArg(al[ac], XmNlabelPixmap, pausePixmap); ac++;
    XtSetArg(al[ac], XmNhighlightThickness, 2); ac++;
    pauseBtn =  XmCreatePushButton(btnForm,"pauseBtn", al, ac);
    XtManageChild(pauseBtn);
    XtAddCallback(pauseBtn, XmNactivateCallback,
		  (XtCallbackProc) GenericCB,(XtPointer) MOTIF_PAUSE);

    /* Forward Button */

    ac = 0;
    XtSetArg(al[ac], XmNtopAttachment, XmATTACH_FORM); ac++;
    XtSetArg(al[ac], XmNleftAttachment, XmATTACH_WIDGET); ac++;
    XtSetArg(al[ac], XmNleftWidget,pauseBtn); ac++;
    XtSetArg(al[ac], XmNshadowType, XmSHADOW_OUT); ac++;
    XtSetArg(al[ac], XmNshadowThickness, 2); ac++;
    XtSetArg(al[ac], XmNlabelType, XmPIXMAP); ac++;
    XtSetArg(al[ac], XmNlabelPixmap, fwdPixmap); ac++;
    XtSetArg(al[ac], XmNhighlightThickness, 2); ac++;
    fwdBtn =  XmCreatePushButton(btnForm,"fwdBtn", al, ac);
    XtManageChild(fwdBtn);
    XtAddCallback(fwdBtn, XmNactivateCallback,
		  (XtCallbackProc) GenericCB, (XtPointer) MOTIF_FWD);

    /* Next Button */
    ac = 0;
    XtSetArg(al[ac], XmNtopAttachment, XmATTACH_FORM); ac++;
    XtSetArg(al[ac], XmNrightAttachment, XmATTACH_FORM); ac++;
    XtSetArg(al[ac], XmNleftAttachment, XmATTACH_WIDGET); ac++;
    XtSetArg(al[ac], XmNleftWidget, fwdBtn); ac++;
    XtSetArg(al[ac], XmNleftOffset, 2); ac++;
    XtSetArg(al[ac], XmNshadowType, XmSHADOW_OUT); ac++;
    XtSetArg(al[ac], XmNshadowThickness, 2); ac++;
    XtSetArg(al[ac], XmNlabelType, XmPIXMAP); ac++;
    XtSetArg(al[ac], XmNlabelPixmap, nextPixmap); ac++;
    XtSetArg(al[ac], XmNhighlightThickness, 2); ac++;
    nextBtn = XmCreatePushButton(btnForm,"nextBtn", al, ac);
    XtManageChild(nextBtn);
    XtAddCallback(nextBtn, XmNactivateCallback,
		  (XtCallbackProc) GenericCB, (XtPointer) MOTIF_NEXT);

    /********************/
    /* Volume scale     */
    /********************/
    ac = 0;
    XtSetArg(al[ac], XmNleftOffset, 10); ac++;
    XtSetArg(al[ac], XmNrightOffset, 10); ac++;
    XtSetArg(al[ac], XmNtopOffset, 10); ac++;
    XtSetArg(al[ac], XmNbottomOffset, 10); ac++;
    XtSetArg(al[ac], XmNtopAttachment, XmATTACH_WIDGET); ac++;
    XtSetArg(al[ac], XmNtopWidget, btnForm); ac++;
    XtSetArg(al[ac], XmNleftAttachment,XmATTACH_FORM); ac++;
    XtSetArg(al[ac], XmNbottomAttachment,XmATTACH_FORM); ac++;

    XtSetArg(al[ac], XmNmaximum, MAX_AMPLIFICATION); ac++;
    XtSetArg(al[ac], XmNminimum, 0); ac++;
    XtSetArg(al[ac], XmNshowValue, True); ac++;

    XtSetArg(al[ac], XmNtraversalOn, False); ac++;
    XtSetArg(al[ac], XmNhighlightThickness,0); ac++;
    XtSetArg(al[ac], XmNtitleString,
	     XmStringCreate("VOL",char_set)); ac++;
    volume_scale = XmCreateScale(mainForm,"volscale",al,ac);
    XtManageChild(volume_scale);
    XtAddCallback(volume_scale, XmNvalueChangedCallback,
		  (XtCallbackProc) Generic_scaleCB,
		  (XtPointer) MOTIF_CHANGE_VOLUME);


    /********************/
    /* File list        */
    /********************/

    ac = 0;
    XtSetArg(al[ac], XmNtopOffset, 10); ac++;
    XtSetArg(al[ac], XmNbottomOffset, 10); ac++;
    XtSetArg(al[ac], XmNleftOffset, 10); ac++;
    XtSetArg(al[ac], XmNrightOffset, 10); ac++;
    XtSetArg(al[ac], XmNtopAttachment, XmATTACH_WIDGET); ac++;
    XtSetArg(al[ac], XmNtopWidget, btnForm ); ac++;
    XtSetArg(al[ac], XmNleftAttachment,XmATTACH_WIDGET); ac++;
    XtSetArg(al[ac], XmNleftWidget, volume_scale); ac++;
    XtSetArg(al[ac], XmNrightAttachment, XmATTACH_FORM); ac++;
    XtSetArg(al[ac], XmNbottomAttachment,XmATTACH_FORM); ac++;

    XtSetArg(al[ac], XmNselectionPolicy ,XmSINGLE_SELECT); ac++;
    XtSetArg(al[ac], XmNscrollBarDisplayPolicy ,XmAS_NEEDED); ac++;
    XtSetArg(al[ac], XmNlistSizePolicy ,XmRESIZE_IF_POSSIBLE); ac++;

    XtSetArg(al[ac], XmNtraversalOn, False); ac++;
    XtSetArg(al[ac], XmNhighlightThickness,0); ac++;

    file_list = XmCreateScrolledList(mainForm,"File List",al,ac);
    XtManageChild(file_list);
    XtAddCallback(file_list, XmNsingleSelectionCallback,
		  (XtCallbackProc) File_ListCB,
		  NULL);

    /*
     * Last details on toplevel
     */
    ac=0;
    /*
      XtSetArg(al[ac],XmNwidth,400); ac++;
      XtSetArg(al[ac],XmNheight,800); ac++;
      */
    XtSetArg(al[ac], XmNtitle, "TiMidity"); ac++;
    XtSetArg(al[ac], XmNiconName, "NONE"); ac++;
    XtSetArg(al[ac], XmNiconPixmap, timidityPixmap); ac++;
    XtSetValues(toplevel,al,ac);


  /*******************************************************/
  /* Plug the pipe ..... and heeere we go                */
  /*******************************************************/

    XtAppAddInput(context,pipe_number,
		  (XtPointer) XtInputReadMask,handle_input,NULL);

    XtRealizeWidget(toplevel);
    XtAppMainLoop(context);
}
