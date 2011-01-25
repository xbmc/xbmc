#ifndef __W32G_TRACER_H__
#define __W32G_TRACER_H__

#define TRACER_CHANNELS 32
typedef struct w32g_tracer_wnd_t_ {
	HWND hwnd;
	HWND hParentWnd;
	HDC hdc;
	HDC hmdc;
	HGDIOBJ hgdiobj_hmdcprev;
	HBITMAP hbitmap;
	HFONT hFontCommon;
	HFONT hFontHalf;
	RECT rc;
	int font_common_height;
	int font_common_width;
	int height;
	int width;
	int valid;
	int active;
	int updateall;
	RECT rc_current_time;
	RECT rc_tempo;
	RECT rc_master_volume;	// マスターボリューム
	RECT rc_maxvoices;

	RECT rc_head;
	RECT rc_all_channels;		// すべてのチャンネル
	int ch_height;		// チャンネルの表示の高さ
	int ch_space;		// チャンネルの間のスペース
	RECT rc_channel_top;			// チャンネル
	RECT rc_instrument;		// プログラム文字列
	RECT rc_inst_map;
	RECT rc_bank;
	RECT rc_program;		// プログラム番号
	RECT rc_velocity;
	RECT rc_volume;
	RECT rc_expression;
	RECT rc_panning;
	RECT rc_sustain;
	RECT rc_pitch_bend;
	RECT rc_mod_wheel;
	RECT rc_chorus_effect;
	RECT rc_reverb_effect;
	RECT rc_temper_keysig;
	RECT rc_temper_type;
	RECT rc_notes;
	RECT rc_gm;
	RECT rc_gs;
	RECT rc_xg;
	RECT rc_head_rest;

	char current_time[30];
	long current_time_sec;
	long tempo;
	int master_volume;
	int maxvoices;
	char instrument[TRACER_CHANNELS][256];
	short bank[TRACER_CHANNELS];
	short program[TRACER_CHANNELS];
	int velocity[TRACER_CHANNELS];
	short volume[TRACER_CHANNELS];
	short expression[TRACER_CHANNELS];
	short panning[TRACER_CHANNELS];
	short sustain[TRACER_CHANNELS];
	short pitch_bend[TRACER_CHANNELS];
	short mod_wheel[TRACER_CHANNELS];
	short chorus_effect[TRACER_CHANNELS];
	short reverb_effect[TRACER_CHANNELS];
	int8 tt[TRACER_CHANNELS];
	char notes[TRACER_CHANNELS][256];
	char filename[1024];
	char titlename[1024];
	int play_system_mode;
	ChannelBitMask quietchannels;
	ChannelBitMask channel_mute;
	int mapID[TRACER_CHANNELS];

	HBRUSH hNullBrush;
	HPEN hNullPen;
} w32g_tracer_wnd_t;

extern void TracerWndReset(void);
extern void TracerWndClear(int lockflag);
extern void TracerWndPaintAll(int lockflag);
extern void TracerWndPaintDo(int flag);
extern w32g_tracer_wnd_t w32g_tracer_wnd;

// section of ini file
// [TracerWnd]
// PosX =
// PosY =
typedef struct TRACERWNDINFO_ {
	HWND hwnd;
	int PosX;
	int PosY;
	int mode;
} TRACERWNDINFO;
extern TRACERWNDINFO TracerWndInfo;

extern int INISaveTracerWnd(void);
extern int INILoadTracerWnd(void);


#endif /* __W32G_TRACER_H__ */
