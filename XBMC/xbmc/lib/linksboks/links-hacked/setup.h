/* setup.h
 * (c) 2002 Mikulas Patocka
 * This file is a part of the Links program, released under GPL.
 */

#if 0
#define VERSION_STRING			VERSION " ["__DATE__ " " __TIME__"]"
#else
#define VERSION_STRING			VERSION
#endif

/* DEBUG LEVEL:
 *		 0=vsechno vypnuty
 *		 1=leaky
 *		 2=leaky, ruda zona, alloc, realloc a free patterny
 *		-1=tajny level ;-)
 */
#define DEBUGLEVEL                      0

#if DEBUGLEVEL >= 1
#define DEBUG
#define LEAK_DEBUG
#define LEAK_DEBUG_LIST
#endif

#if DEBUGLEVEL < 0
#define OOPS
#define LEAK_DEBUG
#endif

#define LINKS_SOCK_NAME			"socket"
#define LINKS_G_SOCK_NAME		"gsocket"
#define LINKS_PORT			23755
#define MAX_BIND_TRIES			3

#define DNS_TIMEOUT			3600000UL

#define HTTP_KEEPALIVE_TIMEOUT		60000
#define FTP_KEEPALIVE_TIMEOUT		600000
#define MAX_KEEPALIVE_CONNECTIONS	30
#define KEEPALIVE_CHECK_TIME		20000

#define MAX_REDIRECTS			10

#define MEMORY_CACHE_GC_PERCENT		9/10
#define MAX_CACHED_OBJECT		1/4

#define MAX_HISTORY_ITEMS		256
#define MENU_HOTKEY_SPACE		2

#define COL(x)				((x)*0x100)

#define COLOR_MENU			COL(070)
#define COLOR_MENU_FRAME		COL(070)
#define COLOR_MENU_SELECTED		COL(007)
#define COLOR_MENU_HOTKEY		COL(007)

#define COLOR_MAINMENU			COL(070)
#define COLOR_MAINMENU_SELECTED		COL(007)
#define COLOR_MAINMENU_HOTKEY		COL(070)

#define COLOR_DIALOG			COL(070)
#define COLOR_DIALOG_FRAME		COL(070)
#define COLOR_DIALOG_TITLE		COL(007)
#define COLOR_DIALOG_TEXT		COL(070)
#define COLOR_DIALOG_CHECKBOX		COL(070)
#define COLOR_DIALOG_CHECKBOX_TEXT	COL(070)
#define COLOR_DIALOG_BUTTON		COL(070)
#define COLOR_DIALOG_BUTTON_SELECTED	COL(0107)
#define COLOR_DIALOG_FIELD		COL(007)
#define COLOR_DIALOG_FIELD_TEXT		COL(007)
#define COLOR_DIALOG_METER		COL(007)

#define SCROLL_ITEMS			2

#define DIALOG_LEFT_BORDER		3
#define DIALOG_TOP_BORDER		1
#define DIALOG_LEFT_INNER_BORDER	2
#define DIALOG_TOP_INNER_BORDER		0
#define DIALOG_FRAME			2

#define COLOR_TITLE			COL(007)
#define COLOR_TITLE_BG			COL(007)
#define COLOR_STATUS			COL(070)
#define COLOR_STATUS_BG			COL(007)

#define G_SHADOW_WIDTH                  5
#define G_SHADOW_HEIGHT                 5

#define G_MENU_LEFT_BORDER		8
#define G_MENU_LEFT_INNER_BORDER	8
#define G_MENU_TOP_BORDER		16
#define G_MENU_HOTKEY_SPACE		16
#define G_MAINMENU_LEFT_BORDER		16
#define G_MAINMENU_BORDER		16

#define G_DIALOG_TITLE_BORDER		8
#define G_DIALOG_LEFT_BORDER		24
#define G_DIALOG_TOP_BORDER		16
#define G_DIALOG_HLINE_SPACE		3
#define G_DIALOG_VLINE_SPACE		4
#define G_DIALOG_LEFT_INNER_BORDER	16
#define G_DIALOG_TOP_INNER_BORDER       (G_BFU_FONT_SIZE < 24 ? 8 : G_BFU_FONT_SIZE - 16)

#define G_DIALOG_BUTTON_SPACE		16
#define G_DIALOG_CHECKBOX_SPACE		8

#define G_DIALOG_GROUP_SPACE		16
#define G_DIALOG_GROUP_TEXT_SPACE	8

#define G_DIALOG_CHECKBOX_L		"["
#define G_DIALOG_CHECKBOX_R		"]"
#define G_DIALOG_CHECKBOX_X		"X"

#define G_DIALOG_RADIO_L		"["
#define G_DIALOG_RADIO_R		"]"
#define G_DIALOG_RADIO_X		"X"

#define G_DIALOG_BUTTON_L		"[ "
#define G_DIALOG_BUTTON_R		" ]"

#define G_SCROLL_BAR_WIDTH		12
#define G_SCROLL_BAR_MIN_SIZE		20
#define G_DEFAULT_SCROLL_BAR_FRAME_COLOR	0xdddddd
#define G_DEFAULT_SCROLL_BAR_AREA_COLOR		0x888888
#define G_DEFAULT_SCROLL_BAR_BAR_COLOR		0xdddddd

#define G_HTML_DEFAULT_FAMILY		"default"
#define G_HTML_DEFAULT_FAMILY_MONO	"default"

#define G_HTML_TABLE_FRAME_COLOR	0xe0

#define G_HTML_MARGIN			8

#define G_IMG_REFRESH                   1

#define ESC_TIMEOUT			200

#define DISPLAY_TIME_MIN		200
#define DISPLAY_TIME_MAX_FIRST		1000
#define DISPLAY_TIME			15
#define IMG_DISPLAY_TIME		4

#define STAT_UPDATE_MIN			100
#define STAT_UPDATE_MAX			1000

#define HTML_LEFT_MARGIN		3
#define HTML_MAX_TABLE_LEVEL		10
#define HTML_MAX_FRAME_DEPTH		7
#define HTML_CHAR_WIDTH			7
#define HTML_CHAR_HEIGHT		12
#define HTML_FRAME_CHAR_WIDTH		10
#define HTML_FRAME_CHAR_HEIGHT		16
#define HTML_TABLE_2ND_PASS
#define HTML_DEFAULT_INPUT_SIZE		20

#define MAX_INPUT_URL_LEN		4096
#define MAX_INPUT_LUA_LEN		4096

#define SPD_DISP_TIME			200
#define CURRENT_SPD_SEC			50
#define CURRENT_SPD_AFTER		100

#define RESOURCE_INFO_REFRESH		100

#define DOWN_DLG_MIN			20

/* width and height of BFU element in list window in graphical mode
 * (draw_bfu_element function in listedit.c)
 * BFU_ELEMENT_WIDTH is a size of one bfu element (doesn't depend on graphical/text mode)
 */
#define BFU_GRX_WIDTH                 (G_BFU_FONT_SIZE>>1)
#define BFU_ELEMENT_WIDTH             (gf_val(5,5*BFU_GRX_WIDTH))
#define BFU_GRX_HEIGHT                        G_BFU_FONT_SIZE

/* higher number=more sensitive scrolling */
/* used in list_event_handler in listedit.c */
#define MOUSE_SCROLL_DIVIDER          1

#define MAGICKA_KONSTANTA_NA_MAXIMALNI_DYLKU_JS_KODU_PRI_ERRORU	256
#define MAX_UID_LEN			20
#define MAX_PASSWD_LEN			20

/* Buttonbar stuff - symbols from system-medium-roman-serif-vari font */

#define BUTTON_BACK       "\x2"
#define BUTTON_HISTORY    "\x3"
#define BUTTON_RELOAD     "\x4"
#define BUTTON_FORWARD    "\x5"
#define BUTTON_BOOKMARKS  "\x6"
#define BUTTON_HOME       "\x7"
#define BUTTON_STOP       "\x8"

/* Some other special symbols */

#define SYMBOL_CHECKBOX_CHECKED   "\x9"
#define SYMBOL_CHECKBOX_UNCHECKED "\xa"

#define SYMBOL_RADIO_DEPRESSED    "\xb"
#define SYMBOL_RADIO_PRESSED      "\xc"
