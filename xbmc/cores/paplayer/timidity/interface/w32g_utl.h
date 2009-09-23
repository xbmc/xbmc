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

#ifndef ___W32G_UTL_H_
#define ___W32G_UTL_H_

#ifdef IA_W32G_SYN
#ifndef MAX_PORT
#define MAX_PORT 4
#endif
#endif

// ini & config
#define IniVersion "2.2"
typedef struct SETTING_PLAYER_ {
// Main Window
	int InitMinimizeFlag;
// SubWindow Starting Create Flag
	int DebugWndStartFlag;
	int ConsoleWndStartFlag;
	int ListWndStartFlag;
	int TracerWndStartFlag;
	int DocWndStartFlag;
	int WrdWndStartFlag;
// SubWindow Starting Valid Flag
	int DebugWndFlag;
	int ConsoleWndFlag;
	int ListWndFlag;
	int TracerWndFlag;
	int DocWndFlag;
	int WrdWndFlag;
	int SoundSpecWndFlag;
// SubWindow Max Numer
	int SubWindowMax;
// Default File
	char ConfigFile[MAXPATH + 32];
	char PlaylistFile[MAXPATH + 32];
	char PlaylistHistoryFile[MAXPATH + 32];
// Default Dir
	char MidiFileOpenDir[MAXPATH + 32];
	char ConfigFileOpenDir[MAXPATH + 32];
	char PlaylistFileOpenDir[MAXPATH + 32];
// Thread Priority
	int PlayerThreadPriority;
	int GUIThreadPriority;
// Font
	char SystemFont[256];
	char PlayerFont[256];
	char WrdFont[256];
	char DocFont[256];
	char ListFont[256];
	char TracerFont[256];
	int SystemFontSize;
	int PlayerFontSize;
	int WrdFontSize;
	int DocFontSize;
	int ListFontSize;
	int TracerFontSize;
// Misc.
	int WrdGraphicFlag;
	int TraceGraphicFlag;
	int DocMaxSize;
	char DocFileExt[256];
// End.
	int PlayerLanguage;
	int DocWndIndependent; 
	int DocWndAutoPopup; 
	int SeachDirRecursive;
	int IniFileAutoSave;
	int SecondMode;
  int AutoloadPlaylist;
  int AutosavePlaylist;
  int PosSizeSave;
  char DefaultPlaylistName[256];
// End.
} SETTING_PLAYER;

typedef struct SETTING_TIMIDITY_ {
    // Parameter from command line options.

    int32 amplification;	// A
    int antialiasing_allowed;	// a
    int buffer_fragments;	// B
    int32 control_ratio;	// C
				// c (ignore)
    ChannelBitMask default_drumchannels, default_drumchannel_mask; // D
				// d (ignore)

				// E...
    int opt_modulation_wheel;	// E w/W
    int opt_portamento;		// E p/P
    int opt_nrpn_vibrato;  	// E v/V
    int opt_channel_pressure;	// E s/S
    int opt_trace_text_meta_event; // E t/T
    int opt_overlap_voice_allow;// E o/O
    int opt_default_mid;	// E mXX
    int default_tonebank;	// E b
    int special_tonebank;	// E B
    int effect_lr_mode;		// E Fdelay
    int effect_lr_delay_msec;	// E Fdelay
    int opt_reverb_control;	// E Freverb
    int opt_chorus_control;	// E Fchorus
    int noise_sharp_type;	// E Fns
	int opt_surround_chorus; // E ?
	int opt_tva_attack;			// E ?
	int opt_tva_decay;			// E ?
	int opt_tva_release;		// E ?
	int opt_delay_control;		// E ?
	int opt_default_module;		// --module
	int opt_lpf_def;			// E ?
	int opt_drum_effect;			// E ?
	int opt_modulation_envelope;			// E ?
	int opt_pan_delay;			// E ?
	int opt_eq_control;			// E ?
	int opt_insertion_effect;	// E ?
    int opt_evil_mode;		// e
    int adjust_panning_immediately; // F
    int fast_decay;		// f
#ifdef SUPPORT_SOUNDSPEC
    int view_soundspec_flag;	// g
    double spectrogram_update_sec; // g
#endif
				// h (ignore)
    int default_program[MAX_CHANNELS]; // I
    char opt_ctl[30];		// i
    int opt_realtime_playing;	// j
    int reduce_voice_threshold; // k
				// L (ignore)
    char opt_playmode[16];	// O
    char OutputName[MAXPATH + 32]; // o : string
    char OutputDirName[MAXPATH + 32]; // o : string
	int auto_output_mode;
				// P (ignore)
    int voices;			// p
    int auto_reduce_polyphony;  // pa
    ChannelBitMask quietchannels; // Q
    int temper_type_mute;	// Q
    char opt_qsize[16];		// q
    int32 modify_release;	// R
    int32 allocate_cache_size;	// S
	int32 opt_drum_power;	// ?
	int32 opt_amp_compensation;	// ?
	int key_adjust;		// K
	int8 opt_force_keysig;	// H
	int opt_pure_intonation;	// Z
	int8 opt_init_keysig;	// Z
    int output_rate;		// s
    char output_text_code[16];	// t
    int free_instruments_afterwards; // U
    char opt_wrd[16];		// W
#if defined(__W32__) && defined(SMFCONV)
    int opt_rcpcv_dll;		// wr, wR
#endif
				// x (ignore)
				// Z (ignore)
    /* for w32g_a.c */
    int data_block_bits;
    int data_block_num;
//??    int waveout_data_block_size;
#ifdef IA_W32G_SYN
		int SynIDPort[MAX_PORT];
		int syn_AutoStart;
		DWORD processPriority;
		DWORD syn_ThreadPriority;
		int SynPortNum;
		int SynShTime;
#endif
} SETTING_TIMIDITY;

// #### obsoleted
#define PLAYERMODE_AUTOQUIT				0x0001
#define PLAYERMODE_AUTOREFINE				0x0002
#define PLAYERMODE_AUTOUNIQ				0x0004
#define PLAYERMODE_NOT_CONTINUE			0x0008
#define PLAYERMODE_NOT_DRAG_START	0x0010


extern char *OutputName;

extern void LoadIniFile(SETTING_PLAYER *sp,  SETTING_TIMIDITY *st);
extern void SaveIniFile(SETTING_PLAYER *sp,  SETTING_TIMIDITY *st);

extern SETTING_PLAYER *sp_default, *sp_current, *sp_temp;
extern SETTING_TIMIDITY *st_default, *st_current, *st_temp;
extern CHAR *INI_INVALID;
extern CHAR *INI_SEC_PLAYER;
extern CHAR *INI_SEC_TIMIDITY;
extern char *SystemFont;
extern char *PlayerFont;
extern char *WrdFont;
extern char *DocFont;
extern char *ListFont;
extern char *TracerFont;
extern HFONT hSystemFont;
extern HFONT hPlayerFont;
extern HFONT hWrdFont;
extern HFONT hDocFont;
extern HFONT hListFont;
extern HFONT hTracerFont;
extern int SystemFontSize;
extern int PlayerFontSize;
extern int WrdFontSize;
extern int DocFontSize;
extern int ListFontSize;
extern int TracerFontSize;

extern int IniGetKeyInt32(char *section, char *key,int32 *n);
extern int IniGetKeyInt32Array(char *section, char *key, int32 *n, int arraysize);
extern int IniGetKeyInt(char *section, char *key, int *n);
extern int IniGetKeyInt8(char *section, char *key, int8 *n);
extern int IniGetKeyChar(char *section, char *key, char *c);
extern int IniGetKeyIntArray(char *section, char *key, int *n, int arraysize);
extern int IniGetKeyString(char *section, char *key,char *str);
extern int IniGetKeyStringN(char *section, char *key,char *str, int size);
extern int IniGetKeyFloat(char *section, char *key, FLOAT_T *n);
extern int IniPutKeyInt32(char *section, char *key,int32 *n);
extern int IniPutKeyInt32Array(char *section, char *key, int32 *n, int arraysize);
extern int IniPutKeyInt(char *section, char *key, int *n);
extern int IniPutKeyInt8(char *section, char *key, int8 *n);
extern int IniPutKeyChar(char *section, char *key, char *c);
extern int IniPutKeyIntArray(char *section, char *key, int *n, int arraysize);
extern int IniPutKeyString(char *section, char *key, char *str);
extern int IniPutKeyStringN(char *section, char *key, char *str, int size);
extern int IniPutKeyFloat(char *section, char *key, FLOAT_T n);
extern void ApplySettingPlayer(SETTING_PLAYER *sp);
extern void SaveSettingPlayer(SETTING_PLAYER *sp);
extern void ApplySettingTiMidity(SETTING_TIMIDITY *st);
extern void SaveSettingTiMidity(SETTING_TIMIDITY *st);
extern void SettingCtlFlag(SETTING_TIMIDITY *st, int opt_id, int onoff);
extern int IniVersionCheck(void);
extern void BitBltRect(HDC dst, HDC src, RECT *rc);
#if 0
extern TmColors tm_colors[ /* TMCC_SIZE */ ];
#define TmCc(c) (tm_colors[c].color)
extern void TmInitColor(void);
extern void TmFreeColor(void);
extern void TmFillRect(HDC hdc, RECT *rc, int color);
#endif
extern void w32g_initialize(void);
extern int is_directory(char *path);
extern int directory_form(char *path_in_out);

extern char *timidity_window_inifile;
extern char *timidity_output_inifile;

#endif /* ___W32G_UTL_H_ */
