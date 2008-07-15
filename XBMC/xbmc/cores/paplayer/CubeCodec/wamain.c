/*

  in_cube Gamecube Stream Player for Winamp
  by hcs

  includes work by Destop and bero

*/

// winamp and user interface

#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <mmreg.h>
#include <msacm.h>
#include <math.h>
#include <stdio.h>
#include "in2.h"
#include "wamain.h"
#include "resource.h"

#define BinaryName "in_cube.dll"
#define AppName "GC Stream Player"
#define AppVer "v0.16"
#define AppDate "1/8/06"

#ifdef _DEBUG

void __cdecl DisplayError (char * Message, ...) {
	char Msg[400];
	va_list ap;

	va_start( ap, Message );
	vsprintf( Msg, Message, ap );
	va_end( ap );
	MessageBox(NULL,Msg,"Error",MB_OK|MB_ICONERROR|MB_SETFOREGROUND);
}
#else
void __cdecl DisplayError (char * Message, ...) {}
#endif

// avoid CRT. Evil. Big. Bloated.
BOOL WINAPI _DllMainCRTStartup(HANDLE hInst, ULONG ul_reason_for_call, LPVOID lpReserved)
{
	return TRUE;
}

DWORD WINAPI __stdcall DecodeThread(void *b); // the decode thread procedure

#define WM_WA_MPEG_EOF WM_USER+2 // post this to the main window at end of file (after playback as stopped)
char lastfn[MAX_PATH]; // currently playing file (used for getting info on the current file)

CUBEFILE cubefile;

char sample_buffer[576*2*2]; // sample buffer

double decode_pos_ms; // current decoding position, in milliseconds
int paused; // are we paused?
int seek_needed; // if != -1, it is the point that the decode thread should seek to, in ms.
int killDecodeThread=0; // the kill switch for the decode thread

char *pristr[] = {"Idle","Lowest","Below Normal","Normal","Above Normal","Highest (not recommended)","Time Critical (not recommended)"};
int priarray[] = {THREAD_PRIORITY_IDLE,THREAD_PRIORITY_LOWEST,THREAD_PRIORITY_BELOW_NORMAL,THREAD_PRIORITY_NORMAL,THREAD_PRIORITY_ABOVE_NORMAL,THREAD_PRIORITY_HIGHEST,THREAD_PRIORITY_TIME_CRITICAL};

int CPUPriority = 3;
int looptimes = 2;
int fadelength = 10;
int fadedelay = 0;

In_Module mod; // the input module (declared near the bottom of this file)

//HANDLE input_file=INVALID_HANDLE_VALUE; // input file handle
HANDLE thread_handle=INVALID_HANDLE_VALUE;	// the handle to the decode thread

// uses the configuration file plugin.ini in the same dir as the DLL (a la HE)
void GetINIFileName(char * iniFile) {
	if (GetModuleFileName(GetModuleHandle(BinaryName), iniFile, MAX_PATH)) {
		char * lastSlash = strrchr(iniFile, '\\');
				
		*(lastSlash + 1) = 0;
		strcat(iniFile, "plugin.ini");
	}
}

void ReadSettings(void) {
	char iniFile[MAX_PATH+1];
	int t;

	GetINIFileName(iniFile);

	CPUPriority=GetPrivateProfileInt(AppName,"CPU priority",3,iniFile);
	
	fadelength=GetPrivateProfileInt(AppName,"fadelength",10,iniFile);
	fadedelay=GetPrivateProfileInt(AppName,"fadedelay",0,iniFile);
	looptimes=GetPrivateProfileInt(AppName,"looptimes",2,iniFile);

	t=GetPrivateProfileInt(AppName,"adxonechan",0,iniFile);
	if (t) adxonechan=GetPrivateProfileInt(AppName,"adxchannum",1,iniFile);
	else adxonechan=0;
	
	t=GetPrivateProfileInt(AppName,"adxlowvol",0,iniFile);
	if (t) BASE_VOL=0x11E0;
	else BASE_VOL=0x2000;
}

BOOL CALLBACK configDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	char buf[256];
	char iniFile[MAX_PATH];
	static int mypri;
	HANDLE hSlider;

	switch (uMsg) {	
	case WM_CLOSE:
		EndDialog(hDlg,TRUE);
		return 0;
	case WM_INITDIALOG:
		GetINIFileName(iniFile);
		
		// set CPU Priority slider
		hSlider=GetDlgItem(hDlg,IDC_PRISLIDER);
		SendMessage(hSlider, TBM_SETRANGE, (WPARAM) TRUE,        // redraw flag 
			(LPARAM) MAKELONG(1, 7));  // min. & max. positions 
		SendMessage(hSlider, TBM_SETPOS, 
			(WPARAM) TRUE,                   // redraw flag 
			(LPARAM) CPUPriority+1);
		mypri=CPUPriority;
		SetDlgItemText(hDlg,IDC_CPUPRI,pristr[CPUPriority]);

		GetPrivateProfileString(AppName,"fadelength","10",buf,sizeof(buf),iniFile);
		SetDlgItemText(hDlg,IDC_FADELEN,buf);
		GetPrivateProfileString(AppName,"fadedelay","0",buf,sizeof(buf),iniFile);
		SetDlgItemText(hDlg,IDC_FADEDELAY,buf);
		GetPrivateProfileString(AppName,"looptimes","2",buf,sizeof(buf),iniFile);
		SetDlgItemText(hDlg,IDC_LOOPS,buf);
		GetPrivateProfileString(AppName,"adxchannum","1",buf,sizeof(buf),iniFile);
		SetDlgItemText(hDlg,IDC_CHANNUM,buf);
		
		if (adxonechan) CheckDlgButton(hDlg,IDC_ONECHAN,BST_CHECKED);
		if (BASE_VOL==0x11E0) CheckDlgButton(hDlg,IDC_LOWVOL,BST_CHECKED);
				
		break;
	case WM_COMMAND:
		switch (GET_WM_COMMAND_ID(wParam, lParam)) {
		case IDOK:
			GetINIFileName(iniFile);

			CPUPriority=mypri;
			sprintf(buf,"%i",CPUPriority);
			WritePrivateProfileString(AppName,"CPU priority",buf,iniFile);
			
			GetDlgItemText(hDlg,IDC_FADELEN,buf,sizeof(buf)-1);
			sscanf(buf,"%i",&fadelength);
			WritePrivateProfileString(AppName,"fadelength",buf,iniFile);
			GetDlgItemText(hDlg,IDC_FADEDELAY,buf,sizeof(buf)-1);
			sscanf(buf,"%i",&fadedelay);
			WritePrivateProfileString(AppName,"fadedelay",buf,iniFile);
			GetDlgItemText(hDlg,IDC_LOOPS,buf,sizeof(buf)-1);
			sscanf(buf,"%i",&looptimes);
			WritePrivateProfileString(AppName,"looptimes",buf,iniFile);

			GetDlgItemText(hDlg,IDC_CHANNUM,buf,sizeof(buf)-1);
			WritePrivateProfileString(AppName,"adxchannum",buf,iniFile);
			if (IsDlgButtonChecked(hDlg, IDC_ONECHAN) == BST_CHECKED) {
				sscanf(buf,"%i",&adxonechan);
				sprintf(buf,"1");
			} else {
				adxonechan=0;
				sprintf(buf,"0");
			}
			WritePrivateProfileString(AppName,"adxonechan",buf,iniFile);

			BASE_VOL=(IsDlgButtonChecked(hDlg, IDC_LOWVOL) == BST_CHECKED) ? 0x11E0 : 0x2000;
			if (BASE_VOL==0x11E0) sprintf(buf,"1");
			else sprintf(buf,"0");
			WritePrivateProfileString(AppName,"adxlowvol",buf,iniFile);
		case IDCANCEL:
			EndDialog(hDlg,TRUE);
			break;
		}
	case WM_HSCROLL:
		if ((struct HWND__ *)lParam==GetDlgItem(hDlg,IDC_PRISLIDER)) {
			//DisplayError("HScroll=%i",HIWORD(wParam));
			
			if (LOWORD(wParam)==TB_THUMBPOSITION || LOWORD(wParam)==TB_THUMBTRACK) mypri=HIWORD(wParam)-1;
			else mypri=SendMessage(GetDlgItem(hDlg,IDC_PRISLIDER),TBM_GETPOS,0,0)-1;
			SetDlgItemText(hDlg,IDC_CPUPRI,pristr[mypri]);
		}
		break;
	default:
		return 0;
	}

	return 1;
}

void config(HWND hwndParent)
{
	DialogBox(mod.hDllInstance, (const char *)IDD_CONFIG, hwndParent, configDlgProc);
}

void about(HWND hwndParent)
{
	MessageBox(hwndParent,"in_cube " AppVer" \n\nDSP decoder by Destop\nADX decoder by bero\nADP decoder by hcs\n\nAmalgamation and improvements by hcs",AppName,MB_OK);
}

void init() {
	ReadSettings();
}

void quit() {
}

int play(char *fn) 
{ 
	int maxlatency;
	int thread_id;
	
	if (InitADPFILE(fn, &cubefile) && InitADXFILE(fn, &cubefile) && InitDSPFILE(fn, &cubefile)) return 1;

	lstrcpy(lastfn,fn);
	paused=0;
	decode_pos_ms=0;
	seek_needed=-1;

	maxlatency = mod.outMod->Open(cubefile.ch[0].sample_rate,cubefile.NCH,BPS, -1,-1);
	if (maxlatency < 0) // error opening device
	{
		MessageBox(NULL, "error opening device", AppName, MB_OK);
		return 1;
	}
	// dividing by 1000 for the first parameter of setinfo makes it
	// display 'H'... for hundred.. i.e. 14H Kbps.
	mod.SetInfo((cubefile.ch[0].sample_rate*BPS*cubefile.NCH)/1000,cubefile.ch[0].sample_rate/1000,cubefile.NCH,1);

	// initialize vis stuff
	mod.SAVSAInit(maxlatency,cubefile.ch[0].sample_rate);
	mod.VSASetInfo(cubefile.NCH,cubefile.ch[0].sample_rate);

	mod.outMod->SetVolume(-666); // set the output plug-ins default volume

	killDecodeThread=0;
	thread_handle = (HANDLE) CreateThread(NULL,0,(LPTHREAD_START_ROUTINE) DecodeThread,(void *) &killDecodeThread,0,&thread_id);
	SetThreadPriority(thread_handle,priarray[CPUPriority]);
	
	return 0; 
}


void pause() 
{ 
	if (!paused) mod.outMod->Pause(1); 
	paused=1; 
}


void unpause() 
{ 
	if (paused) mod.outMod->Pause(0);
	paused=0;
}


int ispaused() 
{ 
	return paused; 
}


void stop() 
{ 
	if (thread_handle != INVALID_HANDLE_VALUE)
	{
		killDecodeThread=1;
		if (WaitForSingleObject(thread_handle,INFINITE) == WAIT_TIMEOUT)
		{
			MessageBox(mod.hMainWindow,"error asking thread to die!\n","error killing decode thread",0);
			TerminateThread(thread_handle,0);
		}
		CloseHandle(thread_handle);
		thread_handle = INVALID_HANDLE_VALUE;
	}

	CloseCUBEFILE(&cubefile);
	
	mod.outMod->Close();
	mod.SAVSADeInit();
}


int getlength() 
{ 
	return cubefile.nrsamples/cubefile.ch[0].sample_rate*1000;
}


int getoutputtime() 
{ 
	return mod.outMod->GetOutputTime();
}


void setoutputtime(int time_in_ms) 
{ 
	seek_needed=time_in_ms; 
}


void setvolume(int volume) 
{ 
	mod.outMod->SetVolume(volume); 
}


void setpan(int pan) 
{ 
	mod.outMod->SetPan(pan); 
}


int infoDlg(char *fn, HWND hwnd)
{
	CUBEFILE idcube;
	char msgbuf[512],titbuf[32],loopstr[50]="no loop",type[50],cat[50];
	int sasec,easec;
	
	if (!InitADPFILE(fn,&idcube) || !InitADXFILE(fn,&idcube) || !InitDSPFILE(fn,&idcube)) {
		switch(idcube.ch[0].type) {
		case type_adx03:
		case type_adx04:
			sasec=idcube.ch[0].sa*32/idcube.NCH/18/idcube.ch[0].sample_rate;
			easec=idcube.ch[0].ea*32/idcube.NCH/18/idcube.ch[0].sample_rate;
			
			strcpy(cat,"ADX");
			break;
		case type_adp:
			strcpy(cat,"ADP");
			break;
		default:

			if (idcube.ch[0].interleave) {
				sasec=idcube.ch[0].sa*14/(8*8/idcube.ch[0].bps)/idcube.NCH/idcube.ch[0].sample_rate;
				easec=idcube.ch[0].ea*14/(8*8/idcube.ch[0].bps)/idcube.NCH/idcube.ch[0].sample_rate;
			} else {
				sasec=idcube.ch[0].sa*14/(8*8/idcube.ch[0].bps)/idcube.ch[0].sample_rate;
				easec=idcube.ch[0].ea*14/(8*8/idcube.ch[0].bps)/idcube.ch[0].sample_rate;
			}
			
			strcpy(cat,"DSP");
			break;
		}

		if (idcube.ch[0].loop_flag) 
			sprintf(loopstr,"loop start: %d:%02d\nloop end: %d:%02d",sasec/60,sasec%60,easec/60,easec%60);
		
		switch (idcube.ch[0].type) {
		
		case type_std:
			strcpy(type,"standard or unrecognized");
			break;
		case type_sfass:
			strcpy(type,"Cstr");
			break;
		case type_mp2:
			strcpy(type,"RS03");
			break;
		case type_pm2:
			strcpy(type,"STM");
			break;
		case type_halp:
			strcpy(type,"HALPST");
			break;
		case type_idsp:
			strcpy(type,"IDSP");
			break;
		case type_spt:
			strcpy(type,"SPT+SPD");
			break;
		case type_mss:
			strcpy(type,"MSS");
			break;
		case type_gcm:
			strcpy(type,"GCM");
			break;
		case type_mpdsp:
			strcpy(type,"Monopoly Party HACK");
			break;
		case type_ish:
			strcpy(type,"ISH+ISD");
			break;
		case type_ymf:
			strcpy(type,"YMF");
			break;
		case type_adx03:
			strcpy(type,"03");
			break;
		case type_adx04:
			strcpy(type,"04");
			break;
		case type_adp:
			strcpy(type,"headerless");
			break;
		}

		sprintf(titbuf,"%s Info",cat);
		sprintf(msgbuf,"%d Hz %s %s\ntype: %s\n%d samples\n%s",idcube.ch[0].sample_rate,
			(idcube.NCH==2?"stereo":"mono"),cat,type,
			idcube.nrsamples,loopstr
			);

		MessageBox(mod.hMainWindow,msgbuf,titbuf,MB_OK);
		CloseCUBEFILE(&idcube);
	}

	return 0;
}

void getfileinfo(char *filename, char *title, int *length_in_ms)
{
	CUBEFILE ficube;
	
	if (!filename || !*filename)  // currently playing file
	{
		if (length_in_ms) *length_in_ms=getlength();
		if (title) 
		{
			char *p=lastfn+lstrlen(lastfn);
			while (*p != '\\' && p >= lastfn) p--;
			lstrcpy(title,++p);
		}
	}
	else // some other file
	{
		if (length_in_ms) 
		{
			*length_in_ms=-1000;
			if (!InitADPFILE(filename,&ficube) || !InitADXFILE(filename,&ficube) || !InitDSPFILE(filename,&ficube))
			{
				// these are only second-accurate, but how accurate does this need to be anyway?
				*length_in_ms = ficube.nrsamples/ficube.ch[0].sample_rate*1000;
			}
			CloseCUBEFILE(&ficube);
		}
		if (title) 
		{
			char *p=filename+lstrlen(filename);
			while (*p != '\\' && p >= filename) p--;
			lstrcpy(title,++p);
		}
	}
}


void eq_set(int on, char data[10], int preamp) 
{ 
	// most plug-ins can't even do an EQ anyhow.. I'm working on writing
	// a generic PCM EQ, but it looks like it'll be a little too CPU 
	// consuming to be useful :)
}

int isourfile(char *fn) { return 0; } 
// used for detecting URL streams.. unused here. strncmp(fn,"http://",7) to detect HTTP streams, etc

// render 576 samples into buf. 
// note that if you adjust the size of sample_buffer, for say, 1024
// sample blocks, it will still work, but some of the visualization 
// might not look as good as it could. Stick with 576 sample blocks
// if you can, and have an additional auxiliary (overflow) buffer if 
// necessary.. 

int get_576_samples(short *buf, cubefile& cubefile)
{
	int i;

	for (i=0;i<576;i++) {
		if ((looptimes || !cubefile.ch[0].loop_flag) && decode_pos_ms*cubefile.ch[0].sample_rate/1000+i >= cubefile.nrsamples) return i*cubefile.NCH*(BPS/8);
		if (cubefile.ch[0].readloc == cubefile.ch[0].writeloc) {
			fillbuffers(&cubefile);
			//if (cubefile.ch[0].readloc != cubefile.ch[0].writeloc) return i*cubefile.NCH*(BPS/8);
		}
		buf[i*cubefile.NCH]=cubefile.ch[0].chanbuf[cubefile.ch[0].readloc++];
		if (cubefile.NCH==2) buf[i*cubefile.NCH+1]=cubefile.ch[1].chanbuf[cubefile.ch[1].readloc++];
		if (cubefile.ch[0].readloc>=0x8000/8*14) cubefile.ch[0].readloc=0;
		if (cubefile.ch[1].readloc>=0x8000/8*14) cubefile.ch[1].readloc=0;

		// fade
		if (looptimes && cubefile.ch[0].loop_flag && cubefile.nrsamples*1000.0/cubefile.ch[0].sample_rate-decode_pos_ms < fadelength*1000) {
			buf[i*cubefile.NCH]=(short)((double)buf[i*cubefile.NCH]*(cubefile.nrsamples*1000.0/cubefile.ch[0].sample_rate-decode_pos_ms)/fadelength/1000.0);
			if (cubefile.NCH==2) buf[i*cubefile.NCH+1]=(short)((double)buf[i*cubefile.NCH+1]*(cubefile.nrsamples*1000.0/cubefile.ch[0].sample_rate-decode_pos_ms)/fadelength/1000.0);
		}
	}
	return 576*cubefile.NCH*(BPS/8);
}


DWORD WINAPI __stdcall DecodeThread(void *b)
{
	int done=0;
	while (! *((int *)b) ) 
	{
		if (seek_needed != -1)
		{
			//DisplayError("seek to %d",seek_needed);

			if (decode_pos_ms>seek_needed) {

					if (!InitADPFILE(NULL, &cubefile) || !InitADXFILE(NULL, &cubefile) || !InitDSPFILE(NULL, &cubefile));
	
					decode_pos_ms=-1; // -1 ms, force it to work even if we want to seek to 0
			}
	
			while (decode_pos_ms<seek_needed && ! *((int *)b)) {
				get_576_samples((short*)sample_buffer);
				mod.outMod->Flush((int)decode_pos_ms);
				decode_pos_ms+=(576.0*1000.0)/(double)cubefile.ch[0].sample_rate;
			}
			seek_needed=-1;
		}
		if (done)
		{
			mod.outMod->CanWrite();
			if (!mod.outMod->IsPlaying())
			{
				PostMessage(mod.hMainWindow,WM_WA_MPEG_EOF,0,0);
				return 0;
			}
			Sleep(10);
		}
		else if (mod.outMod->CanWrite() >= ((576*cubefile.NCH*(BPS/8))<<(mod.dsp_isactive()?1:0)))
		{	
			int l=576*cubefile.NCH*(BPS/8);
			l=get_576_samples((short*)sample_buffer);
			if (!l) 
			{
				done=1;
			}
			else
			{
				if (l==576*cubefile.NCH*(BPS/8)) {
					mod.SAAddPCMData((char *)sample_buffer,cubefile.NCH,BPS,mod.outMod->GetWrittenTime());
					mod.VSAAddPCMData((char *)sample_buffer,cubefile.NCH,BPS,mod.outMod->GetWrittenTime());
				}
				decode_pos_ms+=(576.0*1000.0)/(double)cubefile.ch[0].sample_rate;
				if (mod.dsp_isactive()) l=mod.dsp_dosamples((short *)sample_buffer,l/cubefile.NCH/(BPS/8),BPS,cubefile.NCH,cubefile.ch[0].sample_rate)*(cubefile.NCH*(BPS/8));
				mod.outMod->Write(sample_buffer,l);
			}
		}
		else Sleep(20);
	}
	return 0;
}


In_Module mod = 
{
	IN_VER,
	AppName " " AppVer " " AppDate
#ifdef _DEBUG
	" (DEBUG)"
#endif
	,
	0,	// hMainWindow
	0,  // hDllInstance
	//"DSP\0DSP Audio File (*.DSP)\0GCM\0DSP Audio File (*.GCM)\0HPS\0HALPST Audio File (*.HPS)\0IDSP\0IDSP Audio File (*.IDSP)\0SPT\0SPT Audio Header (*.SPT)\0SPD\0SPD Headerless ADPCM (*.SPD)\0"
	"DSP;GCM;HPS;IDSP;SPT;SPD;MSS;MPDSP;ISH;YMF\0DSP Audio File (*.DSP;*.GCM;*.HPS;*.IDSP;*.SPT;*.SPD;*.MSS;*.MPDSP;*.ISH;*.YMF)\0ADX\0ADX Audio File (*.ADX)\0ADP\0ADP Audio File (*.ADP)\0"
	,
	1,	// is_seekable
	1, // uses output
	config,
	about,
	init,
	quit,
	getfileinfo,
	infoDlg,
	isourfile,
	play,
	pause,
	unpause,
	ispaused,
	stop,
	
	getlength,
	getoutputtime,
	setoutputtime,

	setvolume,
	setpan,

	0,0,0,0,0,0,0,0,0, // vis stuff


	0,0, // dsp

	eq_set,

	NULL,		// setinfo

	0 // out_mod

};

__declspec( dllexport ) In_Module * winampGetInModule2()
{
	return &mod;
}

int main (void)
{
	return 0;
}
