#ifndef __W32G_WRD_H__
#define __W32G_WRD_H__

#define W32G_WRDWND_ROW 80
#define W32G_WRDWND_COL 25
#define W32G_WRDWND_ATTR_REVERSE	0x01
#define W32G_WRDWND_BLACK	0
#define W32G_WRDWND_RED			1
#define W32G_WRDWND_BLUE		2
#define W32G_WRDWND_PURPLE	3
#define W32G_WRDWND_GREEN	4
#define W32G_WRDWND_LIGHTBLUE	5
#define W32G_WRDWND_YELLOW	6
#define W32G_WRDWND_WHITE	7
#define W32G_WRDWND_GRAPHIC_PLANE_MAX 2
#define W32G_WRDWND_GRAPHIC_PALLETE_MAX 16
#define W32G_WRDWND_GRAPHIC_BITS 8
#define W32G_WRDWND_GRAPHIC_PALLETE_BUF_MAX 20

#define WRD_FLAG_TEXT 1
#define WRD_FLAG_GRAPHIC 2
#define WRD_FLAG_DEFAULT ( WRD_FLAG_TEXT | WRD_FLAG_GRAPHIC )

typedef struct w32g_wrd_wnd_t_ {
	HWND hwnd;
	HWND hParentWnd;
	HDC hdc;
	HDC hmdc;
	HGDIOBJ hgdiobj_hmdcprev;
	HBITMAP hbitmap;

	int flag;	// フラグ
	int draw_skip;

	// ワーク
	HBITMAP hbmp_work;
	// テキストマスク
	HBITMAP hbmp_tmask;
	// グラフィック
	w32g_dib_t *graphic_dib[W32G_WRDWND_GRAPHIC_PLANE_MAX];
	int index_active;		// アクティブ画面
	int index_display;		// ディスプレイ画面
	int gmode;
	// 画像データの形式
	BITMAPINFO *bmi_graphic[W32G_WRDWND_GRAPHIC_PLANE_MAX];
	// パレットベースの画像データ
	char *bits_mag_work;
	// グラフィックパレットバッファ
	RGBQUAD default_gpal[W32G_WRDWND_GRAPHIC_PALLETE_MAX];
	RGBQUAD gpal_buff[W32G_WRDWND_GRAPHIC_PALLETE_BUF_MAX][W32G_WRDWND_GRAPHIC_PALLETE_MAX];
	// 変更情報
	int modified_graphic[W32G_WRDWND_GRAPHIC_PLANE_MAX];
	// フェード
	int fade_from;
	int fade_to;
	//
	HPEN hNullPen;
	HBRUSH hNullBrush;

	HFONT hFont;
	RECT rc;

	int font_height;
	int font_width;
	int height;
	int width;
	int row;
	int col;
	int curposx;
	int curposy;
	char curforecolor;
	char curbackcolor;
	char curattr;
	char textbuf[W32G_WRDWND_COL][W32G_WRDWND_ROW];
	char forecolorbuf[W32G_WRDWND_COL][W32G_WRDWND_ROW];
	char backcolorbuf[W32G_WRDWND_COL][W32G_WRDWND_ROW];
	char attrbuf[W32G_WRDWND_COL][W32G_WRDWND_ROW];
	int valid;
	int active;
	int updateall;
	COLORREF pals[32];
} w32g_wrd_wnd_t;
extern void WrdWndReset(void);
extern void WrdWndCopyLine(int from, int to, int lockflag);
extern void WrdWndClearLineFromTo(int from, int to, int lockflag);
extern void WrdWndMoveLine(int from, int to, int lockflag);
extern void WrdWndScrollDown(int lockflag);
extern void WrdWndScrollUp(int lockflag);
extern void WrdWndClear(int lockflag);
extern void WrdWndPutString(char *str, int lockflag);
extern void WrdWndPutStringN(char *str, int n, int lockflag);
extern void WrdWndLineClearFrom(int left, int lockflag);
extern void WrdWndSetAttr98(int attr);
extern void WrdWndSetAttrReset(void);
extern void WrdWndGoto(int x, int y);
extern void WrdWndPaintAll(int lockflag);
extern void WrdWndPaintDo(int flag);
extern void WrdWndCurStateSaveAndRestore(int saveflag);
extern w32g_wrd_wnd_t w32g_wrd_wnd;

// section of ini file
// [WrdWnd]
// PosX =
// PosY =
typedef struct WRDWNDINFO_ {
	HWND hwnd;
	int PosX;
	int PosY;
	int volatile GraphicStop;
} WRDWNDINFO;
extern WRDWNDINFO WrdWndInfo;

extern int INISaveWrdWnd(void);
extern int INILoadWrdWnd(void);


#endif /* __W32G_WRD_H__ */
