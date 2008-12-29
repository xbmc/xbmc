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

	Macintosh interface for TiMidity
	by T.Nogami	<t-nogami@happy.email.ne.jp>
	
    mac_dlog.c
    Preference dialog
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */
#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>

#include	"timidity.h"
#include	"instrum.h"
#include	"playmidi.h"
#include	"output.h"
#include	"readmidi.h"
#include	"reverb.h"

#include	"mac_main.h"
#include	"mac_util.h"

extern int effect_lr_mode;


void mac_DefaultOption()
{
	play_mode->rate=DEFAULT_RATE;
	play_mode->encoding &= ~PE_MONO;	/*stereo*/
	/*play_mode->encoding |= PE_MONO;*/	/*mono*/
	
	antialiasing_allowed=0;
	fast_decay=0;
	adjust_panning_immediately=0;
	free_instruments_afterwards=1;	/*option -U*/
	
	voices=DEFAULT_VOICES;
	control_ratio = play_mode->rate/CONTROLS_PER_SECOND;
	gSilentSec=3.0;
	mac_LogWindow.show=mac_ListWindow.show=true;
	mac_WrdWindow.show=mac_DocWindow.show=
		mac_SpecWindow.show=mac_TraceWindow.show=false;
	gShuffle=false;

	opt_modulation_wheel = 1;
	opt_portamento = 1;
	opt_nrpn_vibrato = 1;
#ifdef REVERB_CONTROL_ALLOW
	opt_reverb_control = 1;
#else
	opt_reverb_control = 0;
#endif
#ifdef CHORUS_CONTROL_ALLOW
	opt_chorus_control = 1;
#else
	opt_chorus_control = 0;
#endif
	opt_surround_chorus = 0;
	opt_channel_pressure = 0;
	opt_trace_text_meta_event = 0;
	opt_overlap_voice_allow = 1;
	
	effect_lr_mode=-1; //no effect
	modify_release=0;
	opt_default_mid=0x7e; //GM
	
	readmidi_wrd_mode=1;
	
	evil_level=EVIL_NORMAL;
	do_initial_filling=0;
	reduce_voice_threshold = 0;
	auto_reduce_polyphony = 0;
}

enum{iOK=1, iCancel=2, iDefault=3, iRate=5, iMono=6, iStereo=7,
			iFreeInstr=8, iAntiali=9, iFastDecay=10, iAdjustPanning=11,
			iVoices=13, iControlRaio=15, iSilent=18,

			iModulation_wheel = 21,
			iPortamento = 22,
			iNrpn_vibrato = 23,
			
			iReverb = 24,
			iReverb_setlevel = 38,
			iReverb_level = 39,
			iReverb_level_hint = 42,
						
			iChorus = 25,
			iChorus_setlevel = 40,
			iChorus_level = 41,
			iChorus_level_hint = 43,
			
			iChannel_pressure = 26,
			iText_meta_event = 27,
			iOverlap_voice = 28,
			/*iPReverb = 29,*/
			
			iModify_release=31,
			iModify_release_ms=37,
			iModify_release_ms_hint1=30,			
			iModify_release_ms_hint2=44,
			iModify_release_ms_hint3=45,
			
			iPresence_balance=32,
			iManufacture=33,
			iEvil_level=34,
			/*iDo_initial_filling=35,*/
			iShuffle= 36
			};

// ******************************************

static void mac_AdjustDialog( DialogRef dialog )
{
	//reverb
	if( GetDialogItemValue(dialog, iReverb)==2 ){
		SetDialogItemHilite(dialog, iReverb_setlevel, kControlNoPart);
	}else{
		SetDialogItemHilite(dialog, iReverb_setlevel, kControlInactivePart);
		SetDialogItemValue(dialog, iReverb_setlevel, 0);
	}

	if( GetDialogItemValue(dialog, iReverb_setlevel) )
	{
		ShowDialogItem(dialog, iReverb_level);
		ShowDialogItem(dialog, iReverb_level_hint);						
	}else{
		HideDialogItem(dialog, iReverb_level);
		HideDialogItem(dialog, iReverb_level_hint);						
	}
	//chorus
	if( GetDialogItemValue(dialog, iChorus)==2 ){
		// activate
		SetDialogItemHilite(dialog, iChorus_setlevel, kControlNoPart);
		mySetDialogItemText(dialog, iChorus_level_hint, "\p(1..127)");
	}else if( GetDialogItemValue(dialog, iChorus)==3 ){
		// activate
		SetDialogItemHilite(dialog, iChorus_setlevel, kControlNoPart);
		mySetDialogItemText(dialog, iChorus_level_hint, "\p(1..63)");
	}else{	// inactivate
		SetDialogItemHilite(dialog, iChorus_setlevel, kControlInactivePart);
		SetDialogItemValue(dialog, iChorus_setlevel, 0);
	}
	
	if( GetDialogItemValue(dialog, iChorus_setlevel) )
	{
		ShowDialogItem(dialog, iChorus_level);
		ShowDialogItem(dialog, iChorus_level_hint);
	}else{
		HideDialogItem(dialog, iChorus_level);
		HideDialogItem(dialog, iChorus_level_hint);
	}
	
	// modify release
	if( GetDialogItemValue(dialog, iModify_release) )
	{
		ShowDialogItem(dialog, iModify_release_ms);
		ShowDialogItem(dialog, iModify_release_ms_hint1);
		ShowDialogItem(dialog, iModify_release_ms_hint2);
		ShowDialogItem(dialog, iModify_release_ms_hint3);
	}else{
		HideDialogItem(dialog, iModify_release_ms);
		HideDialogItem(dialog, iModify_release_ms_hint1);
		HideDialogItem(dialog, iModify_release_ms_hint2);
		HideDialogItem(dialog, iModify_release_ms_hint3);
	}
}

// ***************************************************

static void SetDialogValue(DialogRef theDialog)
{
#define BUFSIZE 80
	short	value;
	char	buf[BUFSIZE];
	Str255		s;
	
	SetDialogItemValue(theDialog, iStereo, !(play_mode->encoding & PE_MONO));
	SetDialogItemValue(theDialog, iMono, play_mode->encoding & PE_MONO);
	value=(play_mode->rate==11025? 1:(play_mode->rate==44100? 3:2));
	SetDialogItemValue(theDialog, iRate, value);
	SetDialogItemValue(theDialog, iFreeInstr, free_instruments_afterwards);
	SetDialogItemValue(theDialog, iAntiali, antialiasing_allowed);
	SetDialogItemValue(theDialog, iFastDecay, fast_decay);
	SetDialogItemValue(theDialog, iAdjustPanning, adjust_panning_immediately);
	NumToString(voices, s);
	mySetDialogItemText(theDialog, iVoices, s);
	SelectDialogItemText(theDialog, iVoices, 0, 32000);
	NumToString(control_ratio, s);
	mySetDialogItemText(theDialog, iControlRaio, s);
	snprintf(buf, BUFSIZE, "%g",gSilentSec); /*use s as C string*/
	mySetDialogItemText(theDialog, iSilent, c2pstr(buf));

	SetDialogItemValue(theDialog, iFastDecay, fast_decay);

	SetDialogItemValue(theDialog, iModulation_wheel, opt_modulation_wheel);
	SetDialogItemValue(theDialog, iPortamento, opt_portamento);
	SetDialogItemValue(theDialog, iNrpn_vibrato, opt_nrpn_vibrato);
	SetDialogItemValue(theDialog, iChannel_pressure, opt_channel_pressure);
	SetDialogItemValue(theDialog, iText_meta_event, opt_trace_text_meta_event);
	SetDialogItemValue(theDialog, iOverlap_voice, opt_overlap_voice_allow);
	
	/*-----reverb-----*/
	if( opt_reverb_control<0 ){ //Enabel
		SetDialogItemValue(theDialog, iReverb, 2);
		SetDialogItemValue(theDialog, iReverb_setlevel, 1);
		SetDialogItemHilite(theDialog, iReverb_setlevel, kControlNoPart);
		SetDialogTEValue(theDialog, iReverb_level, -opt_reverb_control);
		
	}else if( opt_reverb_control==0 || opt_reverb_control==2 ){ //Non or global
		SetDialogItemValue(theDialog, iReverb, opt_reverb_control+1);
		SetDialogItemValue(theDialog, iReverb_setlevel, 1);
		SetDialogItemHilite(theDialog, iReverb_setlevel, kControlInactivePart);
	}else{	// opt_reverb_control==1, no level
		SetDialogItemValue(theDialog, iReverb, 2);
		SetDialogItemValue(theDialog, iReverb_setlevel, 0);
		SetDialogItemHilite(theDialog, iReverb_setlevel, kControlNoPart);
	}
	
	/*-----chorus-----*/
	if( opt_surround_chorus ){
		SetDialogItemValue(theDialog, iChorus, 3); //surround
	}else if( opt_chorus_control<0 || opt_chorus_control==1 ){
		SetDialogItemValue(theDialog, iChorus, 2); //Enable
		SetDialogItemHilite(theDialog, iChorus_setlevel, kControlNoPart);
	}else{
		SetDialogItemValue(theDialog, iChorus, 1); //Non
		SetDialogItemHilite(theDialog, iChorus_setlevel, kControlInactivePart);
	}
	
	if( opt_chorus_control<0 ){
		SetDialogItemHilite(theDialog, iChorus_setlevel, kControlNoPart);
		SetDialogItemValue(theDialog, iChorus_setlevel, 1);
		SetDialogTEValue(theDialog, iChorus_level, -opt_chorus_control);
	}else{
		SetDialogItemValue(theDialog, iChorus_setlevel, 0);
	}
	
	/*-----modify_release-----*/
	SetDialogItemValue(theDialog, iModify_release,  (modify_release!=0) );
	SetDialogTEValue(theDialog, iModify_release_ms, modify_release);
	
	SetDialogItemValue(theDialog, iPresence_balance, effect_lr_mode+2);
	value= (	opt_default_mid==0x41? 1:           //GS
				opt_default_mid==0x43? 2:3	);		//XG:GM
	SetDialogItemValue(theDialog, iManufacture, value);
	SetDialogItemValue(theDialog, iEvil_level, evil_level);
	//SetDialogItemValue(theDialog, iDo_initial_filling, do_initial_filling);
	SetDialogItemValue(theDialog, iShuffle, gShuffle);
	
	mac_AdjustDialog( theDialog );
}

static int mac_limit_params(int var, int min_limit, int max_limit)
{
	if( var < min_limit ) var = min_limit;
	if( var > max_limit ) var = max_limit;
	return var;
}


// *******************************************
//	mac_SetPlayOption
// *******************************************
OSErr mac_SetPlayOption()
{
	Boolean		more=false;
	short		item, value;
	long		i;
	DialogRef	dialog, theDialog;
	EventRecord	event;
	Str255		s;
	
	theDialog=GetNewDialog(201,0,(WindowRef)-1);
	
	if( !theDialog ) return 1;
	else
	{
		SetDialogDefaultItem(theDialog, iOK);
		SetDialogCancelItem(theDialog, iCancel);
		
		SetDialogValue(theDialog);
		ShowWindow(GetDialogWindow(theDialog));
		
		for(;;)
		{
			WaitNextEvent(everyEvent, &event, 1,0);
			/*if( event.what==mouseDown ){
				short		part;
				WindowRef	window;
				part = FindWindow(event.where, &window);
				if( part==inDrag ){
					DragWindow(window, event.where, &qd.screenBits.bounds);
					continue;
				}
			}else if(event.what==)
			*/
			if( IsDialogEvent( &event ) )
			{
				if( StdFilterProc(dialog, &event, &item) ) /**/;
					else DialogSelect(&event, &dialog, &item);
				
				if( theDialog!=dialog ) continue;
				switch(item)
				{
				case iOK:
					myGetDialogItemText(dialog, iVoices, s);
					StringToNum(s,&i);
					if( i<=0 ) i=1;
					max_voices=MAX_SAFE_MALLOC_SIZE / sizeof(Voice);
					if ( i<max_voices ) max_voices=i;
					voices=max_voices;
						
					myGetDialogItemText(dialog, iControlRaio, s);
					StringToNum(s,&i);
					if( i<=0 ) i=22; /*i=1 cause ploblem*/
					if( MAX_CONTROL_RATIO<i ) i=MAX_CONTROL_RATIO;
					control_ratio=i;
					
					myGetDialogItemText(dialog, iSilent, s);
					gSilentSec=atof(p2cstr(s));
					if( gSilentSec<0 ) gSilentSec=0;
					if( gSilentSec>10 ) gSilentSec=10;
					
					if( GetDialogItemValue(dialog, iMono) )
						play_mode->encoding |= PE_MONO;
					else play_mode->encoding &= ~PE_MONO;
					
					if( (value=GetDialogItemValue(dialog, iRate))==1 )	play_mode->rate=11025;
					else if( value==2 ) play_mode->rate=22050;
					else if( value==3 ) play_mode->rate=44100;
					else play_mode->rate=22050;
					
					free_instruments_afterwards=GetDialogItemValue(dialog, iFreeInstr)? 1:0;
					antialiasing_allowed=		GetDialogItemValue(dialog, iAntiali)? 1:0;
					fast_decay=					GetDialogItemValue(dialog, iFastDecay)? 1:0;
					adjust_panning_immediately=	GetDialogItemValue(dialog, iAdjustPanning)? 1:0;

					opt_modulation_wheel=		GetDialogItemValue(dialog, iModulation_wheel)? 1:0;
					opt_portamento=				GetDialogItemValue(dialog, iPortamento)? 1:0;
					opt_nrpn_vibrato=			GetDialogItemValue(dialog, iNrpn_vibrato)? 1:0;
					opt_channel_pressure=		GetDialogItemValue(dialog, iChannel_pressure)? 1:0;
					opt_trace_text_meta_event=	GetDialogItemValue(dialog, iText_meta_event)? 1:0;
					opt_overlap_voice_allow=	GetDialogItemValue(dialog, iOverlap_voice)? 1:0;
					
					/*-----reverb-----*/
					switch(GetDialogItemValue(dialog, iReverb))
					{
					  case 1:
					    opt_reverb_control = 0;
					    break;
					  case 2:
					    if(GetDialogItemValue(dialog, iReverb_setlevel))
						opt_reverb_control =
							- (mac_limit_params( GetDialogTEValue( dialog, iReverb_level ), 1, 127));
					    else
						opt_reverb_control = 1;
					    break;
					  case 3:
					    opt_reverb_control = 2;
					    break;
					  /*default:
					    ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
						      "Invalid -EFreverb parameter.");*/
					}
					
					/*-----chorus-----*/
					opt_surround_chorus = 0;
					switch(GetDialogItemValue(dialog, iChorus))
					{
					  case 1:
					    opt_chorus_control = 0;
					    break;

					  case 2:
					    if( GetDialogItemValue(dialog, iChorus_setlevel) )
						opt_chorus_control =
							- (mac_limit_params( GetDialogTEValue(dialog,iChorus_level), 1, 127 ) );
					    else
						opt_chorus_control = 1;
					    break;
					  case 3:
						opt_surround_chorus = 1;
					    	if( GetDialogItemValue(dialog, iChorus_setlevel) )
							opt_chorus_control =
								- (mac_limit_params( GetDialogTEValue(dialog,iChorus_level), 1,63));
					  	else
							opt_chorus_control = 1;
						break;
					  /*default:
					    ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
						      "Invalid -EFchorus parameter.");
					    return 1;*/
					}
					
					modify_release=0;
					if( GetDialogItemValue(dialog, iModify_release) ){
						modify_release=GetDialogTEValue(dialog, iModify_release_ms);
						modify_release = mac_limit_params(modify_release, 0, 5000);
					}
					effect_lr_mode=	GetDialogItemValue(dialog, iPresence_balance)-2;
					value=GetDialogItemValue(dialog, iManufacture);
					opt_default_mid= 	(value==1? 0x41:		//GS
										 value==2? 0x43:0x7e);	//XG:GM
					evil_level= 		GetDialogItemValue(dialog, iEvil_level);
					//do_initial_filling=	GetDialogItemValue(dialog, iDo_initial_filling)? 1:0;
					gShuffle=			GetDialogItemValue(dialog, iShuffle)? 1:0;
					
					DisposeDialog(theDialog);
					return noErr;
				case iCancel:
					DisposeDialog(theDialog);
					return 1;
				case iDefault:
					mac_DefaultOption();
					SetDialogValue(theDialog);
					break;
/*				case iMore:
					if( !more )
					{
						SizeWindow(GetDialogWindow(theDialog), 400, 300, 1);
						SetDialogControlTitle(theDialog,iMore, "\pFew...");
						more=true;
					}
					else
					{
						SizeWindow(GetDialogWindow(theDialog), 400, 160, 1);
						SetDialogControlTitle(theDialog,iMore, "\pMore...");
						more=false;
					}
*/
					break;
				case iRate:
					break;
				case iMono:
					SetDialogItemValue(dialog, iMono, 1);
					SetDialogItemValue(dialog, iStereo, 0);
					break;
				case iStereo:
					SetDialogItemValue(dialog, iStereo, 1);
					SetDialogItemValue(dialog, iMono, 0);
					break;
				case iFreeInstr:	ToggleDialogItem(dialog, iFreeInstr);	break;
				case iAntiali:		ToggleDialogItem(dialog, iAntiali);		break;
				case iFastDecay:	ToggleDialogItem(dialog, iFastDecay);	break;
				case iAdjustPanning:ToggleDialogItem(dialog, iAdjustPanning); break;

				case iModulation_wheel:	ToggleDialogItem(dialog, iModulation_wheel);	break;
				case iPortamento:		ToggleDialogItem(dialog, iPortamento);	break;
				case iNrpn_vibrato:		ToggleDialogItem(dialog, iNrpn_vibrato);	break;
				case iChannel_pressure:	ToggleDialogItem(dialog, iChannel_pressure);	break;
				case iText_meta_event:	ToggleDialogItem(dialog, iText_meta_event);	break;
				case iOverlap_voice:	ToggleDialogItem(dialog, iOverlap_voice);	break;

				case iReverb:
					mac_AdjustDialog(dialog);
					break;
				case iReverb_setlevel:
					ToggleDialogItem(dialog, iReverb_setlevel);
					mac_AdjustDialog(dialog);
					break;
				case iChorus:
					mac_AdjustDialog(dialog);
					break;

				case iChorus_setlevel:
					ToggleDialogItem(dialog, iChorus_setlevel);
					mac_AdjustDialog(dialog);
					break;
				case iModify_release:
					ToggleDialogItem(dialog, iModify_release);
					if( GetDialogItemValue(dialog, iModify_release) ){ //newly checked on
						SetDialogTEValue(theDialog, iModify_release_ms, DEFAULT_MREL);
					}
					mac_AdjustDialog(dialog);
					break;
				case iShuffle:			ToggleDialogItem(dialog, iShuffle);	break;
				}
			}
		}
	}
}

/* ****************************************************** */
struct{
	int		version;
	short	playerX,playerY,
			logX, logY,
			logW, logH,
			listX, listY,
			listW, listH,
			docX,  docY,
			docW,  docH,
			specX, specY,
			traceX, traceY, wrdX,wrdY, skinX,skinY;
	int		rate;
	char	mono, freeinstrument, antialias, fastdecay, adjustpanning;
	int		voice;
	short	amplitude;
	int32	controlratio;
	double	silentsec;
	int32	modify_release;
	int		effect_lr_mode;
	int		opt_default_mid;
	int		showMsg, showList, showWrd, showDoc, showSpec, showTrace, showSkin,
			modulation_wheel, portamento, nrpn_vibrato, reverb_control,
			chorus_control, surround_chorus, channel_pressure,
			xg_bank_select_lsb, trace_text_meta_event, overlap_voice_allow,
			do_reverb_flag;
	int		evil_level,do_initial_filling;
	int		gShuffle;
	char	skin_mainfile[256];
	char	wrdfontname[256];
	char	rsv[256];
}Preference;


#define	PREF_VER	14
						/* ++ 2.6.1        ->prefver=14 */
						/* ++ 2.1.0        ->prefver=13 */
#define	PREF_NUM	(sizeof(Preference))	/*pref data bytes*/

OSErr mac_GetPreference()
{
	OSErr	err;
	short	vRefNum, refNum=0;
	long	count, dirID;
	FSSpec	spec;
	
	memset(&Preference, 0, sizeof(Preference) );
	
	err=FindFolder(kOnSystemDisk, kPreferencesFolderType, kCreateFolder,
						&vRefNum, &dirID);
	if( err ) return err;
	
	err=FSMakeFSSpec(vRefNum, dirID, PREF_FILENAME, &spec);
	if( err )	return err;	//pref file not found
	
	err=FSpOpenDF(&spec, fsRdPerm, &refNum);
	if( err )	return err;
	
	count=PREF_NUM;
	err=FSRead(refNum, &count, (char*)&Preference);
	FSClose(refNum);
	if( Preference.version!=PREF_VER || count<PREF_NUM )
	{
		StopAlertMessage("\pPreference file is invalid! Default Setting is applyed.");
		return 1;
	}
	
	
	mac_PlayerWindow.X=	Preference.playerX;
	mac_PlayerWindow.Y=	Preference.playerY;
	mac_LogWindow.X=	Preference.logX;
	mac_LogWindow.Y=	Preference.logY;
	mac_LogWindow.width=Preference.logW;
	mac_LogWindow.hight=Preference.logH;
	mac_ListWindow.X=		Preference.listX;
	mac_ListWindow.Y=		Preference.listY;
	mac_ListWindow.width=	Preference.listW;
	mac_ListWindow.hight=	Preference.listH;
	
	mac_DocWindow.X=		Preference.docX;
	mac_DocWindow.Y=		Preference.docY;
	mac_DocWindow.width=	Preference.docW;
	mac_DocWindow.hight=	Preference.docH;
	
	mac_SpecWindow.X=	Preference.specX;
	mac_SpecWindow.Y=	Preference.specY;
	mac_TraceWindow.X=	Preference.traceX;
	mac_TraceWindow.Y=	Preference.traceY;
	mac_WrdWindow.X=	Preference.wrdX;
	mac_WrdWindow.Y=	Preference.wrdY;
	mac_WrdWindow.X=	Preference.wrdX;
	mac_WrdWindow.Y=	Preference.wrdY;
	mac_SkinWindow.X=	Preference.skinX;
	mac_SkinWindow.Y=	Preference.skinY;
	
	play_mode->rate=Preference.rate;
	if( Preference.mono ){
		play_mode->encoding |= PE_MONO;		/*mono*/
	}else{
		play_mode->encoding &= ~PE_MONO;	/*stereo*/
	}
	free_instruments_afterwards=	Preference.freeinstrument;
	antialiasing_allowed=			Preference.antialias;
	fast_decay=						Preference.fastdecay;
	adjust_panning_immediately=		Preference.adjustpanning;
	voices=							Preference.voice;
	mac_amplitude=					Preference.amplitude;
	control_ratio=					Preference.controlratio;
	gSilentSec=						Preference.silentsec;
	modify_release=					Preference.modify_release;
	effect_lr_mode=					Preference.effect_lr_mode;
	opt_default_mid=				Preference.opt_default_mid;
	
	mac_LogWindow.show=				Preference.showMsg;
	mac_ListWindow.show=			Preference.showList;
	mac_DocWindow.show=				Preference.showDoc;
	mac_WrdWindow.show=				Preference.showWrd;
	mac_DocWindow.show=				Preference.showDoc;
	mac_SpecWindow.show=			Preference.showSpec;
	mac_TraceWindow.show=			Preference.showTrace;

	opt_modulation_wheel =		Preference.modulation_wheel;
	opt_portamento =			Preference.portamento;
	opt_nrpn_vibrato =			Preference.nrpn_vibrato;
	opt_reverb_control =		Preference.reverb_control;
	opt_chorus_control =		Preference.chorus_control;
	opt_surround_chorus = 		Preference.surround_chorus;
	opt_channel_pressure =		Preference.channel_pressure;
	opt_trace_text_meta_event =	Preference.trace_text_meta_event;
	opt_overlap_voice_allow =	Preference.overlap_voice_allow;
	evil_level=					Preference.evil_level;
	do_initial_filling=			Preference.do_initial_filling;
	gShuffle=					Preference.gShuffle;

	return noErr;
}

OSErr mac_SetPreference()
{
	OSErr	err;
	short	vRefNum,refNum=0;
	long	count;
	long	dirID;
	Point p={0,0};
	FSSpec	spec;
	
	err=FindFolder(kOnSystemDisk, kPreferencesFolderType, kCreateFolder,
						&vRefNum, &dirID);
	if( err ) return err;
	
	err=FSMakeFSSpec(vRefNum, dirID, PREF_FILENAME, &spec);
	if( err==fnfErr )
	{
		err=FSpCreate(&spec, 'TIMI', 'pref', smSystemScript);
		if(err) return err;
	}
	else if( err!=noErr ) return err;
	
	err=FSpOpenDF(&spec, fsWrPerm, &refNum);
	if( err )	return err;
	
	Preference.version=	PREF_VER;
	Preference.playerX=	mac_PlayerWindow.X;
	Preference.playerY=	mac_PlayerWindow.Y;
	Preference.logX=	mac_LogWindow.X;
	Preference.logY=	mac_LogWindow.Y;
	Preference.logW=	mac_LogWindow.width;
	Preference.logH=	mac_LogWindow.hight;
	Preference.listX=	mac_ListWindow.X;
	Preference.listY=	mac_ListWindow.Y;
	Preference.listW=	mac_ListWindow.width;
	Preference.listH=	mac_ListWindow.hight;
	Preference.docX=	mac_DocWindow.X;
	Preference.docY=	mac_DocWindow.Y;
	Preference.docW=	mac_DocWindow.width;
	Preference.docH=	mac_DocWindow.hight;
	Preference.specX=	mac_SpecWindow.X;
	Preference.specY=	mac_SpecWindow.Y;
	Preference.traceX=	mac_TraceWindow.X;
	Preference.traceY=	mac_TraceWindow.Y;
	Preference.wrdX=	mac_WrdWindow.X;
	Preference.wrdY=	mac_WrdWindow.Y;
	Preference.skinX=	mac_SkinWindow.X;
	Preference.skinY=	mac_SkinWindow.Y;
	
	Preference.rate=	play_mode->rate;
	Preference.mono=	(play_mode->encoding & PE_MONO);
	Preference.freeinstrument=	free_instruments_afterwards;
	Preference.antialias=		antialiasing_allowed;
	Preference.fastdecay=		fast_decay;
	Preference.adjustpanning=	adjust_panning_immediately;
	Preference.voice=			voices;
	Preference.amplitude=		mac_amplitude;
	Preference.controlratio=	control_ratio;
	Preference.silentsec=		gSilentSec;
	Preference.modify_release=	modify_release;
	Preference.effect_lr_mode=	effect_lr_mode;
	Preference.opt_default_mid=	opt_default_mid;

	Preference.showMsg=			mac_LogWindow.show;
	Preference.showList=		mac_ListWindow.show;
	Preference.showWrd=			mac_WrdWindow.show;
	Preference.showDoc=			mac_DocWindow.show;
	Preference.showSpec=		mac_SpecWindow.show;
	Preference.showTrace=		mac_TraceWindow.show;

	Preference.modulation_wheel=	opt_modulation_wheel;
	Preference.portamento=			opt_portamento;
	Preference.nrpn_vibrato=		opt_nrpn_vibrato;
	Preference.reverb_control=		opt_reverb_control;
	Preference.chorus_control=		opt_chorus_control;
	Preference.channel_pressure=	opt_channel_pressure;
	Preference.trace_text_meta_event=opt_trace_text_meta_event;
	Preference.overlap_voice_allow=	opt_overlap_voice_allow;
	Preference.evil_level=			evil_level;
	Preference.do_initial_filling=	do_initial_filling;
	Preference.gShuffle=			gShuffle;

	count=PREF_NUM;
	err=FSWrite(refNum, &count, &Preference);
	FSClose(refNum);
	
	return noErr;
}

