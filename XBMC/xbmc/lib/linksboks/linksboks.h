/*
 * LinksBoks
 * Copyright (c) 2003-2005 ysbox
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef LINKSBOKS_H
#define LINKSBOKS_H

#ifdef _XBOX

#include <xtl.h>

#ifdef XBOX_USE_XFONT
#define XFONT_TRUETYPE
#include "xfont.h"
#endif

#endif

typedef struct _LinksBoksViewport
{
	int width, height;
	int margin_left, margin_right;
	int margin_top, margin_bottom;
} LinksBoksViewPort;

/* Option types */
#define LINKSBOKS_OPTION_GROUP		0
#define LINKSBOKS_OPTION_INT		1
#define LINKSBOKS_OPTION_BOOL		2
#define LINKSBOKS_OPTION_STRING		3



/* Keyboard constants. Renamed but still come from links.h */
#define LINKSBOKS_MOUSE_LEFT		0
#define LINKSBOKS_MOUSE_MIDDLE	1
#define LINKSBOKS_MOUSE_RIGHT		2
#define LINKSBOKS_MOUSE_WHEELUP	8
#define LINKSBOKS_MOUSE_WHEELDOWN	9
#define LINKSBOKS_MOUSE_WHEELUP1	10
#define LINKSBOKS_MOUSE_WHEELDOWN1	11
#define LINKSBOKS_MOUSE_WHEELLEFT	12
#define LINKSBOKS_MOUSE_WHEELRIGHT	13
#define LINKSBOKS_MOUSE_WHEELLEFT1	14
#define LINKSBOKS_MOUSE_WHEELRIGHT1	15

#define LINKSBOKS_BM_ACT		48
#define LINKSBOKS_MOUSE_DOWN		0
#define LINKSBOKS_MOUSE_UP		16
#define LINKSBOKS_MOUSE_DRAG		32
#define LINKSBOKS_MOUSE_MOVE		48
#define LINKSBOKS_MOUSE_CTRL		64

#define LINKSBOKS_KBD_ENTER	-0x100
#define LINKSBOKS_KBD_BS		-0x101
#define LINKSBOKS_KBD_TAB		-0x102
#define LINKSBOKS_KBD_ESC		-0x103
#define LINKSBOKS_KBD_LEFT	-0x104
#define LINKSBOKS_KBD_RIGHT	-0x105
#define LINKSBOKS_KBD_UP		-0x106
#define LINKSBOKS_KBD_DOWN	-0x107
#define LINKSBOKS_KBD_INS		-0x108
#define LINKSBOKS_KBD_DEL		-0x109
#define LINKSBOKS_KBD_HOME	-0x10a
#define LINKSBOKS_KBD_END		-0x10b
#define LINKSBOKS_KBD_PAGE_UP	-0x10c
#define LINKSBOKS_KBD_PAGE_DOWN	-0x10d

#define LINKSBOKS_KBD_F1		-0x120
#define LINKSBOKS_KBD_F2		-0x121
#define LINKSBOKS_KBD_F3		-0x122
#define LINKSBOKS_KBD_F4		-0x123
#define LINKSBOKS_KBD_F5		-0x124
#define LINKSBOKS_KBD_F6		-0x125
#define LINKSBOKS_KBD_F7		-0x126
#define LINKSBOKS_KBD_F8		-0x127
#define LINKSBOKS_KBD_F9		-0x128
#define LINKSBOKS_KBD_F10		-0x129
#define LINKSBOKS_KBD_F11		-0x12a
#define LINKSBOKS_KBD_F12		-0x12b

#define LINKSBOKS_KBD_CTRL_C	-0x200
#define LINKSBOKS_KBD_CLOSE	-0x201
#define LINKSBOKS_KBD_PASTE       -0x202

#define LINKSBOKS_KBD_SHIFT	1
#define LINKSBOKS_KBD_CTRL	2
#define LINKSBOKS_KBD_ALT		4


/* States for pages and others */
#define LINKSBOKS_S_WAIT		0
#define LINKSBOKS_S_DNS		1
#define LINKSBOKS_S_CONN		2
#define LINKSBOKS_S_SSL_NEG	3
#define LINKSBOKS_S_SENT		4
#define LINKSBOKS_S_LOGIN		5
#define LINKSBOKS_S_GETH		6
#define LINKSBOKS_S_PROC		7
#define LINKSBOKS_S_TRANS		8
#define LINKSBOKS_S_QUESTIONS	7

#define LINKSBOKS_S_WAIT_REDIR		-999
#define LINKSBOKS_S_OKAY			-1000
#define LINKSBOKS_S_INTERRUPTED		-1001
#define LINKSBOKS_S_EXCEPT		-1002
#define LINKSBOKS_S_INTERNAL		-1003
#define LINKSBOKS_S_OUT_OF_MEM		-1004
#define LINKSBOKS_S_NO_DNS		-1005
#define LINKSBOKS_S_CANT_WRITE		-1006
#define LINKSBOKS_S_CANT_READ		-1007
#define LINKSBOKS_S_MODIFIED		-1008
#define LINKSBOKS_S_BAD_URL		-1009
#define LINKSBOKS_S_TIMEOUT		-1010
#define LINKSBOKS_S_RESTART		-1011
#define LINKSBOKS_S_STATE			-1012

#define LINKSBOKS_S_HTTP_ERROR		-1100
#define LINKSBOKS_S_HTTP_100		-1101
#define LINKSBOKS_S_HTTP_204		-1102

#define LINKSBOKS_S_FILE_TYPE		-1200
#define LINKSBOKS_S_FILE_ERROR		-1201

#define LINKSBOKS_S_FTP_ERROR		-1300
#define LINKSBOKS_S_FTP_UNAVAIL		-1301
#define LINKSBOKS_S_FTP_LOGIN		-1302
#define LINKSBOKS_S_FTP_PORT		-1303
#define LINKSBOKS_S_FTP_NO_FILE		-1304
#define LINKSBOKS_S_FTP_FILE_ERROR	-1305

#define LINKSBOKS_S_SSL_ERROR		-1400
#define LINKSBOKS_S_NO_SSL		-1401

#define LINKSBOKS_S_INVALID_DEVICE -65532
#define LINKSBOKS_S_INVALID_TERM -65533
#define LINKSBOKS_S_INVALID_WINDOW -65534
#define LINKSBOKS_S_INVALID_FDATA -65530
#define LINKSBOKS_S_NOT_BUSY      -65535



class LinksBoksOption
{
public:
	LinksBoksOption(const char *name, const char *caption, int type, int depth, unsigned char *default_value);
	LinksBoksOption(const char *name, const char *caption, int type, int depth, int default_value = 0);
	VOID Register();
	/* Normally you just have to implement the version of BeforeChange you need,
	   depending on your option type. Return FALSE if you don't accept the change */
	virtual BOOL OnBeforeChange(void *session, unsigned char *oldvalue, unsigned char *newvalue);
	virtual BOOL OnBeforeChange(void *session, int oldvalue, int newvalue);
	virtual VOID OnAfterChange(void *session);

	const char *m_sName;
	const char *m_sCaption;
	int m_iType;
	int m_iDepth;
	unsigned char *m_sDefaultValue;
	int m_iDefaultValue;

protected:
	// Used during OnBeforeChange, use the void *session you get from there
	void MsgBox(void *session, unsigned char *title, unsigned char *msg);
};


/* Don't derive this class! Use LinksBoksExternalProtocol or LinksBoksInternalProtocol */
class LinksBoksProtocol
{
public:
	unsigned char *m_sName;
	int m_iPort;
	BOOL m_bFreeSyntax;
	BOOL m_bNeedSlashes;
	BOOL m_bNeedSlashAfterHost;

	virtual VOID Register() {}

	virtual int OnCall(unsigned char *url, void *) { return 0; }

protected:
	LinksBoksProtocol(unsigned char *name, int port, BOOL free_syntax, BOOL need_slashes, BOOL need_slash_after_host) :
		m_sName(name),
		m_iPort(port),
		m_bFreeSyntax(free_syntax),
		m_bNeedSlashes(need_slashes),
		m_bNeedSlashAfterHost(need_slash_after_host) {}

};

/* External protocol: the function will be triggered as soon as the user clicks the link.
You get the URL, you do whatever you want with it and that's it. You can however display
a little messagebox. */
class LinksBoksExternalProtocol : public LinksBoksProtocol
{
public:
	LinksBoksExternalProtocol(unsigned char *name, int port, BOOL free_syntax, BOOL need_slashes, BOOL need_slash_after_host) :
	  LinksBoksProtocol(name, port, free_syntax, need_slashes, need_slash_after_host) {}
	VOID Register();
	virtual int OnCall(unsigned char *url, void *session) { return 0; }

protected:
	// Used during OnCall, use the void *session you get from there
	void MsgBox(void *session, unsigned char *title, unsigned char *msg);

};

#define LINKSBOKS_RESPONSE_OK				-1000
#define LINKSBOKS_RESPONSE_INTERRUPTED		-1001
#define LINKSBOKS_RESPONSE_EXCEPTION		-1002
#define LINKSBOKS_RESPONSE_INTERNAL			-1003
#define LINKSBOKS_RESPONSE_OUT_OF_MEMORY	-1004
#define LINKSBOKS_RESPONSE_NO_DNS			-1005
#define LINKSBOKS_RESPONSE_CANT_WRITE		-1006
#define LINKSBOKS_RESPONSE_CANT_READ		-1007
#define LINKSBOKS_RESPONSE_MODIFIED			-1008
#define LINKSBOKS_RESPONSE_BAD_URL			-1009
#define LINKBOKSE_RESPONSE_TIMEOUT			-1010

/* Internal protocol: this is more like a regular protocol like HTTP; you are expected to
call (once) the SendResponse() function with a content-type, some data (ie. HTML or image
data). Additionally, the return code is important because if it's not LINKSBOKS_RESPONSE_OK,
a generic error message will be displayed instead. */
class LinksBoksInternalProtocol : public LinksBoksProtocol
{
public:
	LinksBoksInternalProtocol(unsigned char *name, int port, BOOL free_syntax, BOOL need_slashes, BOOL need_slash_after_host) :
	  LinksBoksProtocol(name, port, free_syntax, need_slashes, need_slash_after_host) {}
	VOID Register();
	virtual int OnCall(unsigned char *url, void *connection) { return LINKSBOKS_RESPONSE_EXCEPTION; }

protected:
	// Used during OnCall, use the void *session you get from there
	int SendResponse(void *connection, unsigned char *content_type, unsigned char *data, int data_size);
};


#define LINKSBOKS_URLLIST_BOOKMARKS     0
#define LINKSBOKS_URLLIST_HISTORY       1

class ILinksBoksURLList
{
public:
    virtual ~ILinksBoksURLList() { }
    virtual bool GetRoot(void)=0;
    virtual bool GetNext(void)=0;

    virtual unsigned char *GetTitle(void)=0;
    virtual unsigned char *GetURL(void)=0;
    virtual int GetDepth(void)=0;
    virtual int GetType(void)=0;
    virtual long GetLastVisit(void)=0;
};

class LinksBoksBookmarks : public ILinksBoksURLList
{
public:
    virtual bool GetRoot(void);
    virtual bool GetNext(void);

    virtual unsigned char *GetTitle(void);
    virtual unsigned char *GetURL(void);
    virtual int GetDepth(void);
    virtual int GetType(void);
    virtual long GetLastVisit(void);

protected:
    void *l;
    void *root;
};

class LinksBoksHistory : public ILinksBoksURLList
{
public:
    virtual bool GetRoot(void);
    virtual bool GetNext(void);

    virtual unsigned char *GetTitle(void);
    virtual unsigned char *GetURL(void);
    virtual int GetDepth(void);
    virtual int GetType(void);
    virtual long GetLastVisit(void);

protected:
    void *l;
    void *root;
};

class ILinksBoksBookmarksWriter
{
public:
    virtual ~ILinksBoksBookmarksWriter() { };
    virtual bool Begin()=0;
    virtual bool AppendBookmark(unsigned char *title, unsigned char *url, int depth, bool isfolder)=0;
    virtual bool Save()=0;
};

class LinksBoksBookmarksWriter : public ILinksBoksBookmarksWriter
{
public:
    virtual bool Begin();
    virtual bool AppendBookmark(unsigned char *title, unsigned char *url, int depth, bool isfolder);
    virtual bool Save();

protected:
    unsigned char *convert_to_entity_string(unsigned char *str);  // helper function from bookmarks.c
    void *m_pFile;  /* FILE pointer */
    struct conv_table *ct;
    int m_iDepth;
};


#if defined(XBOX_USE_XFONT) || defined(XBOX_USE_FREETYPE)
/* External fonts types */
#define LINKSBOKS_EXTFONT_TYPE_XFONT	0
#define LINKSBOKS_EXTFONT_TYPE_FREETYPE	1

typedef struct _LinksBoksExtFont
{
	unsigned char *name;
	int type;

	unsigned char *family;
	unsigned char *weight;
	unsigned char *slant;
	unsigned char *adstyl;
	unsigned char *spacing;

	void *fontdata;
} LinksBoksExtFont;
#endif

#ifdef XBMC_LINKS_DLL
// virtual class for exporting from dll
class ILinksBoksWindow
{
public:
    virtual ~ILinksBoksWindow() { }
	virtual int Freeze(void)=0;
	virtual int Unfreeze(void)=0;
	virtual LPDIRECT3DSURFACE8 GetSurface(void)=0;
	virtual int GetViewPortWidth(void)=0;
	virtual int GetViewPortHeight(void)=0;
	virtual void Close(void)=0;
    virtual VOID KeyboardAction(int key, int flags)=0;
	virtual VOID MouseAction(int x, int y, int buttons)=0;
	virtual VOID GoBack()=0;
	virtual VOID GoForward()=0;
	virtual VOID Stop()=0;
	virtual VOID Reload(BOOL nocache)=0;
    virtual VOID GoToURL(unsigned char *url)=0;
	virtual BOOL CanInputText(unsigned char *buffer, int size)=0;
    virtual VOID SendText(unsigned char *text)=0;
/*
    virtual BOOL MoveRight()=0;
    virtual BOOL MoveLeft()=0;
    virtual BOOL MoveUp()=0;
    virtual BOOL MoveDown()=0;
*/
	virtual VOID ResizeWindow(LinksBoksViewPort viewport)=0;
	virtual VOID RedrawWindow()=0;
    virtual VOID SetFocus(BOOL bFocus)=0;
	virtual int NumberOfTabs()=0;
	virtual VOID SwitchToTab(int index)=0;
	virtual VOID CloseCurrentTab()=0;
	virtual BOOL GetCurrentURL(unsigned char *buffer, int size)=0;
	virtual BOOL GetCurrentTitle(unsigned char *buffer, int size)=0;
	virtual BOOL GetCurrentLinkURL(unsigned char *buffer, int size)=0;
	virtual BOOL GetCurrentStatus(unsigned char *buffer, int size)=0;
	virtual int GetCurrentState()=0;

};
#endif


/* The LinksBoksWindow class represents an actual browser "window". Have more than one is not
very tested (at all) at this time, but it should be theorically possible. */
#ifdef XBMC_LINKS_DLL
class LinksBoksWindow : public ILinksBoksWindow
#else
class LinksBoksWindow
#endif
{
public:
	//
	// INIT/DEINIT FUNCTIONS
	//

	/* These functions are automatically called. Use the global function LinksBoks_CreateWindow()
	if you want to create a new window */
	LinksBoksWindow(LinksBoksViewPort viewport);
	int Initialize(void *grdev);

	/* This only queries the current window to be closed,
	if it's the last one, a confirmation dialog is displayed, in this case
	the engine is terminated too. If you call LinksBoks_Terminate(), all
	windows are automatically terminated.
	The effective termination is not immediate, it may be happen during
	a further LinksBoks_FrameMove call. Since we don't know when, you need
	to forget the object (that is, not accessing it anymore) as soon as you
	call this function */
	void Close(void);

	/* !Don't call this!
	It is supposed to be only accessed by the graphics driver.
	It releases all the D3D surfaces and stuff */
	void Terminate(void);

	/* Freeze/unfreeze session: Destroys the buffer surfaces to free memory.
	When unfreezing we recreate the surfaces and request a redraw from Links. */
	int Freeze(void);
	int Unfreeze(void);

	//
	// RENDERING FUNCTIONS
	//

	/* This function copies the current content of the back-buffer surface into the main front-buffer
	surface you can access. It is not advised to flip every single loop iteration. A common way to do
	is to register a timer every nth milliseconds within LinksBoks and to flip in the timer callback
	(you can even do a Present() there if you don't have your own way of doing it). Check out the host
	application and the EmbeddedSample for examples */
	int FlipSurface(void);

	/* Returns a pointer to the LinksBoks front-buffer surface */
	LPDIRECT3DSURFACE8 GetSurface(void);

	/* Retrieve dimensions of the viewport */
	int GetViewPortWidth(void);
	int GetViewPortHeight(void);

	/* Resizes the viewport, using the new provided LinksBoksViewPort structure.
	WARNING: only change the margins for now! The surface size must remain the same (TODO) */
	VOID ResizeWindow(LinksBoksViewPort viewport);

	/* Refreshes the display (actually, resize with the same size so everything gets recalculated */
	VOID RedrawWindow();

	/* These functions are used by the Links graphics driver to manipulate the data
	(ie. D3D surfaces...) stored as class members; You shouldn't have to use them, but it's
	not forbidden (eg. if you want to draw something on top of the browser window) */
	VOID RegisterFlip(int x, int y, int w, int h);
	VOID Blit(LPDIRECT3DSURFACE8 pSurface, int x, int y, int w, int h );
	HRESULT CreatePrimitive(int x, int y, int w, int h, int color);
	VOID RenderPrimitive(LPDIRECT3DSURFACE8 pTargetSurface);
	VOID FillArea(int x1, int y1, int x2, int y2, long color);
	VOID DrawHLine(int x1, int y, int x2, long color);
	VOID DrawVLine(int x, int y1, int y2, long color);
	VOID SetClipArea(int x1, int y1, int x2, int y2);
	VOID ScrollBackBuffer(int x1, int y1, int x2, int y2, int offx, int offy);

	//
	// KEYBOARD/MOUSE FUNCTIONS
	//

	/* Sends an ASCII character or a special key or combination of keys to be handled
	by the Links engine */
	VOID KeyboardAction(int key, int flags);

	/* Sends a new mouse position and/or button status change */
	VOID MouseAction(int x, int y, int buttons);

	/* EXPERIMENTAL!
	- Guess when can input text (focus is on a text field/textarea).
	- Sends a whole string in one call */
	BOOL CanInputText(unsigned char *buffer, int size);
	VOID SendText(unsigned char *text);
/*
	BOOL MoveRight();
	BOOL MoveUp();
	BOOL MoveLeft();
	BOOL MoveDown();
*/
	VOID SetFocus(BOOL bFocus);

	//
	// SESSION ACTIONS
	//

	/* These are to be polished up */
	VOID GoToURL(unsigned char *url);
	VOID GoToURLInNewTab(unsigned char *url);
	int NumberOfTabs();
	VOID SwitchToTab(int index);
	VOID CloseCurrentTab();
	BOOL GetCurrentURL(unsigned char *buffer, int size);
	BOOL GetCurrentTitle(unsigned char *buffer, int size);
	BOOL GetCurrentLinkURL(unsigned char *buffer, int size);
	BOOL GetCurrentStatus(unsigned char *buffer, int size);
	int GetCurrentState();
	VOID GoBack();
	VOID GoForward();
	VOID Stop();
	VOID Reload(BOOL nocache);

	//
	// TERMINATION
	//


protected:
	int CreateBuffers(void);
	int DestroyBuffers(void);

	void *m_grdev;							/* Graphics device */

	LPDIRECT3DDEVICE8       m_pd3dDevice;	/* Provided d3d device object */
	LPDIRECT3DSURFACE8      m_pdSurface;	/* Front-buffer surface */
	LPDIRECT3DSURFACE8      m_pdBkBuffer;	/* Back-buffer surface */

	RECT					m_ClipArea;
	LinksBoksViewPort		m_ViewPort;
	RECT					m_FlipRegion;
	BOOL					m_bWantFlip;
	BOOL					m_bResized;
  int           m_iCurrentLink;
};


#ifdef XBMC_LINKS_DLL
#define __DLLEXPORT__ __declspec(dllexport)
#else
#define __DLLEXPORT__
#endif

extern "C" {

/* This function loads the LinksBoks section and initializes the main stuff you may be needing before
creating the actual d3d surface. Next you have to call LinksBoks_CreateWindow but this is your chance to
retrieve/set LinksBoks options, register external protocols, file associations timers, various
callbacks and so on, before the graphics engine initialization */
int __DLLEXPORT__ LinksBoks_InitCore(unsigned char *homedir, LinksBoksOption *options[], LinksBoksProtocol *protocols[]);

/* This function initializes the last modules, including the graphics-related stuff. The d3d surface
will be created and an initial rendering will be done */
LinksBoksWindow __DLLEXPORT__ *LinksBoks_CreateWindow(LPDIRECT3DDEVICE8 pd3dDevice, LinksBoksViewPort viewport);

/* Call this function to run the LinksBoks engine's "main loop" once. Be warned that this function
doesn't usually blocks too long but it can last up to several seconds during a page's loading and/or
rendering... Bear with it for now, multithreading is not supported.
Call LinksBoks_InitCore() then LinksBoks_InitLoop() before or it will (of course) crash. */
int __DLLEXPORT__ LinksBoks_FrameMove(void);

/* Empties all caches. Useful to free lots of memory. */
VOID __DLLEXPORT__ LinksBoks_EmptyCaches(void);

/* Shutdowns the LinksBoks engine's subsystems and tries to free the memory resources it uses. */
void __DLLEXPORT__ LinksBoks_Terminate(BOOL bFreeXBESections = 0);

/* Freeze/unfreeze mechanism: Keeps the engine running so you keep your sessions but frees
as much memory as possible. Don't DARE to try doing anything while the engine is sleeping;
the sections are unloaded! */
VOID __DLLEXPORT__ LinksBoks_FreezeCore(void);
VOID __DLLEXPORT__ LinksBoks_UnfreezeCore(void);


/* Registers a new internal timer which will be called at most every 't' ms,
during LinksBoks_FrameMove(). Returns an integer id of the timer or -1 on error */
int __DLLEXPORT__ LinksBoks_RegisterNewTimer(long t, void (*func)(void *), void *data);

/* Use this to provide a callback function for when to launch some external viewer for a mime type.
For compatibility reasons the way of doing things in Links has been left almost untouched.
Here's how it works:
1) The user associates himself a mime type (ie. "application/x-shockwave-flash" with a commandline
(by editing the links.cfg file or interactively in the Associations Manager in the Setup menu).
2) In case the mime type isn't directly provided by the server (for example when trying to open a
file on the local disk), there's also a list of file extensions <=> mime types available.
3) When asked to open a file, when this file of a given mime type, and a commandline is associated
with this mime type, Links asks whether the file has to be opened in the external viewer, and in
that case it downloads the file to some temporary location, runs that commandline, and when the
program returns, deletes the temporary file.

On Xbox, running commandlines makes no sense, but you can provide a function with takes the command-
line as a parameter (and the temp file name), and call what you wish from there. A few reminders:
- the engine will block when in that function as it is called in the same thread
- when the function returns the file may be deleted from disk, so if you create another thread to
  do your background work and prevent blocking, you better read the file into memory first before
  returning */
VOID __DLLEXPORT__ LinksBoks_SetExecFunction(int (*exec_function)(LinksBoksWindow *pLB, unsigned char *cmdline, unsigned char *filepath, int fg));

#if defined(XBOX_USE_XFONT) || defined(XBOX_USE_FREETYPE)
/* Sets the callback function which is expected to return a LinksBoksExtFont* pointer
containing everything needed (incl. a XFONT or freetype FT_Face pointer) */
VOID __DLLEXPORT__ LinksBoks_SetExtFontCallbackFunction(LinksBoksExtFont *(*extfont_function)(unsigned char *fontname, int fonttype));
#endif

/* Callback for using your own window manager to display simple message boxes from Links.
The arrays are NULL terminated
Return TRUE si your application accepts to handle the messagebox, or FALSE if you let Links handle it.
When the user has made a choice from your dialog box call the LinksBoks_ValidateMessageBox function with
the same dlg pointer that was given to you and the index of the chosen button (0=first, 1=second, aso.) */
VOID __DLLEXPORT__ LinksBoks_SetMessageBoxFunction(BOOL (*msgbox_function)(void *dlg, unsigned char *title, int nblabels, unsigned char *labels[], int nbbuttons, unsigned char *buttons[]));
VOID __DLLEXPORT__ LinksBoks_ValidateMessageBox(void *dlg, int choice);


/* URL list factory function */
ILinksBoksURLList __DLLEXPORT__ *LinksBoks_GetURLList(int type);

/* Bookmarks writer factory function */
ILinksBoksBookmarksWriter __DLLEXPORT__ *LinksBoks_GetBookmarksWriter(void);


/* LinksBoks options subsystem bindings */
BOOL __DLLEXPORT__ LinksBoks_GetOptionBool(const char *key);
INT __DLLEXPORT__ LinksBoks_GetOptionInt(const char *key);
unsigned char __DLLEXPORT__ *LinksBoks_GetOptionString(const char *key);
void __DLLEXPORT__ LinksBoks_SetOptionBool(const char *key, BOOL value);
void __DLLEXPORT__ LinksBoks_SetOptionInt(const char *key, INT value);
void __DLLEXPORT__ LinksBoks_SetOptionString(const char *key, unsigned char *value);

}

#endif
