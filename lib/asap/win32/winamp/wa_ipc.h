/*
** Copyright (C) 2003 Nullsoft, Inc.
**
** This software is provided 'as-is', without any express or implied warranty. In no event will the authors be held 
** liable for any damages arising from the use of this software. 
**
** Permission is granted to anyone to use this software for any purpose, including commercial applications, and to 
** alter it and redistribute it freely, subject to the following restrictions:
**
**   1. The origin of this software must not be misrepresented; you must not claim that you wrote the original software. 
**      If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.
**
**   2. Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.
**
**   3. This notice may not be removed or altered from any source distribution.
**
*/

#ifndef _WA_IPC_H_
#define _WA_IPC_H_

/*
** This is the modern replacement for the classic 'frontend.h'. Most of these 
** updates are designed for in-process use, i.e. from a plugin.
**
*/

/* message used to sent many messages to winamp's main window. 
** most all of the IPC_* messages involve sending the message in the form of:
**   result = SendMessage(hwnd_winamp,WM_WA_IPC,(parameter),IPC_*);
*/
#define WM_WA_IPC WM_USER
/* but some of them use WM_COPYDATA. be afraid.
*/

#define IPC_GETVERSION 0
/* int version = SendMessage(hwnd_winamp,WM_WA_IPC,0,IPC_GETVERSION);
**
** Version will be 0x20yx for winamp 2.yx. versions previous to Winamp 2.0
** typically (but not always) use 0x1zyx for 1.zx versions. Weird, I know.
*/

#define IPC_GETREGISTEREDVERSION 770


typedef struct {
  char *filename;
  char *title;
  int length;
} enqueueFileWithMetaStruct; // send this to a IPC_PLAYFILE in a non WM_COPYDATA, 
// and you get the nice desired result. if title is NULL, it is treated as a "thing",
// otherwise it's assumed to be a file (for speed)

#define IPC_PLAYFILE 100  // dont be fooled, this is really the same as enqueufile
#define IPC_ENQUEUEFILE 100 
/* sent as a WM_COPYDATA, with IPC_PLAYFILE as the dwData, and the string to play
** as the lpData. Just enqueues, does not clear the playlist or change the playback
** state.
*/


#define IPC_DELETE 101
#define IPC_DELETE_INT 1101 // don't use this, it's used internally by winamp when 
                            // dealing with some lame explorer issues.
/* SendMessage(hwnd_winamp,WM_WA_IPC,0,IPC_DELETE);
** Use IPC_DELETE to clear Winamp's internal playlist.
*/


#define IPC_STARTPLAY 102   // starts playback. almost like hitting play in Winamp.
#define IPC_STARTPLAY_INT 1102 // used internally, don't bother using it (won't be any fun)


#define IPC_CHDIR 103
/* sent as a WM_COPYDATA, with IPC_CHDIR as the dwData, and the directory to change to
** as the lpData. 
*/


#define IPC_ISPLAYING 104
/* int res = SendMessage(hwnd_winamp,WM_WA_IPC,0,IPC_ISPLAYING);
** If it returns 1, it is playing. if it returns 3, it is paused, 
** if it returns 0, it is not playing.
*/


#define IPC_GETOUTPUTTIME 105
/* int res = SendMessage(hwnd_winamp,WM_WA_IPC,mode,IPC_GETOUTPUTTIME);
** returns the position in milliseconds of the current track (mode = 0), 
** or the track length, in seconds (mode = 1). Returns -1 if not playing or error.
*/


#define IPC_JUMPTOTIME 106
/* (requires Winamp 1.60+)
** SendMessage(hwnd_winamp,WM_WA_IPC,ms,IPC_JUMPTOTIME);
** IPC_JUMPTOTIME sets the position in milliseconds of the 
** current song (approximately).
** Returns -1 if not playing, 1 on eof, or 0 if successful
*/

#define IPC_GETMODULENAME 109
#define IPC_EX_ISRIGHTEXE 666
/* usually shouldnt bother using these, but here goes:
** send a WM_COPYDATA with IPC_GETMODULENAME, and an internal
** flag gets set, which if you send a normal WM_WA_IPC message with
** IPC_EX_ISRIGHTEXE, it returns whether or not that filename
** matches. lame, I know.
*/

#define IPC_WRITEPLAYLIST 120
/* (requires Winamp 1.666+)
** SendMessage(hwnd_winamp,WM_WA_IPC,0,IPC_WRITEPLAYLIST);
**
** IPC_WRITEPLAYLIST writes the current playlist to <winampdir>\\Winamp.m3u,
** and returns the current playlist position.
** Kinda obsoleted by some of the 2.x new stuff, but still good for when
** using a front-end (instead of a plug-in)
*/


#define IPC_SETPLAYLISTPOS 121
/* (requires Winamp 2.0+)
** SendMessage(hwnd_winamp,WM_WA_IPC,position,IPC_SETPLAYLISTPOS)
** IPC_SETPLAYLISTPOS sets the playlist position to 'position'. It
** does not change playback or anything, it just sets position, and
** updates the view if necessary
*/


#define IPC_SETVOLUME 122
/* (requires Winamp 2.0+)
** SendMessage(hwnd_winamp,WM_WA_IPC,volume,IPC_SETVOLUME);
** IPC_SETVOLUME sets the volume of Winamp (from 0-255).
*/


#define IPC_SETPANNING 123
/* (requires Winamp 2.0+)
** SendMessage(hwnd_winamp,WM_WA_IPC,panning,IPC_SETPANNING);
** IPC_SETPANNING sets the panning of Winamp (from 0 (left) to 255 (right)).
*/


#define IPC_GETLISTLENGTH 124
/* (requires Winamp 2.0+)
** int length = SendMessage(hwnd_winamp,WM_WA_IPC,0,IPC_GETLISTLENGTH);
** IPC_GETLISTLENGTH returns the length of the current playlist, in
** tracks.
*/


#define IPC_GETLISTPOS 125
/* (requires Winamp 2.05+)
** int pos=SendMessage(hwnd_winamp,WM_WA_IPC,0,IPC_GETLISTPOS);
** IPC_GETLISTPOS returns the playlist position. A lot like IPC_WRITEPLAYLIST
** only faster since it doesn't have to write out the list. Heh, silly me.
*/


#define IPC_GETINFO 126
/* (requires Winamp 2.05+)
** int inf=SendMessage(hwnd_winamp,WM_WA_IPC,mode,IPC_GETINFO);
** IPC_GETINFO returns info about the current playing song. The value
** it returns depends on the value of 'mode'.
** Mode      Meaning
** ------------------
** 0         Samplerate (i.e. 44100)
** 1         Bitrate  (i.e. 128)
** 2         Channels (i.e. 2)
** 3 (5+)    Video LOWORD=w HIWORD=h
** 4 (5+)    > 65536, string (video description)
*/


#define IPC_GETEQDATA 127
/* (requires Winamp 2.05+)
** int data=SendMessage(hwnd_winamp,WM_WA_IPC,pos,IPC_GETEQDATA);
** IPC_GETEQDATA queries the status of the EQ. 
** The value returned depends on what 'pos' is set to:
** Value      Meaning
** ------------------
** 0-9        The 10 bands of EQ data. 0-63 (+20db - -20db)
** 10         The preamp value. 0-63 (+20db - -20db)
** 11         Enabled. zero if disabled, nonzero if enabled.
** 12         Autoload. zero if disabled, nonzero if enabled.
*/


#define IPC_SETEQDATA 128
/* (requires Winamp 2.05+)
** SendMessage(hwnd_winamp,WM_WA_IPC,pos,IPC_GETEQDATA);
** SendMessage(hwnd_winamp,WM_WA_IPC,value,IPC_SETEQDATA);
** IPC_SETEQDATA sets the value of the last position retrieved
** by IPC_GETEQDATA. This is pretty lame, and we should provide
** an extended version that lets you do a MAKELPARAM(pos,value).
** someday...

  new (2.92+): 
    if the high byte is set to 0xDB, then the third byte specifies
    which band, and the bottom word specifies the value.
*/

#define IPC_ADDBOOKMARK 129
/* (requires Winamp 2.4+)
** Sent as a WM_COPYDATA, using IPC_ADDBOOKMARK, adds the specified
** file/url to the Winamp bookmark list.
*/
/*
In winamp 5+, we use this as a normal WM_WA_IPC and the string:

  "filename\0title\0"

  to notify the library/bookmark editor that a bookmark
was added. Note that using this message in this context does not
actually add the bookmark.
do not use :)
*/


#define IPC_INSTALLPLUGIN 130
/* not implemented, but if it was you could do a WM_COPYDATA with 
** a path to a .wpz, and it would install it.
*/


#define IPC_RESTARTWINAMP 135
/* (requires Winamp 2.2+)
** SendMessage(hwnd_winamp,WM_WA_IPC,0,IPC_RESTARTWINAMP);
** IPC_RESTARTWINAMP will restart Winamp (isn't that obvious ? :)
*/


#define IPC_ISFULLSTOP 400
/* (requires winamp 2.7+ I think)
** ret=SendMessage(hwnd_winamp,WM_WA_IPC,0,IPC_ISFULLSTOP);
** useful for when you're an output plugin, and you want to see
** if the stop/close is a full stop, or just between tracks.
** returns nonzero if it's full, zero if it's just a new track.
*/


#define IPC_INETAVAILABLE 242
/* (requires Winamp 2.05+)
** val=SendMessage(hwnd_winamp,WM_WA_IPC,0,IPC_INETAVAILABLE);
** IPC_INETAVAILABLE will return 1 if the Internet connection is available for Winamp.
*/


#define IPC_UPDTITLE 243
/* (requires Winamp 2.2+)
** SendMessage(hwnd_winamp,WM_WA_IPC,0,IPC_UPDTITLE);
** IPC_UPDTITLE will ask Winamp to update the informations about the current title.
*/


#define IPC_REFRESHPLCACHE 247
/* (requires Winamp 2.2+)
** SendMessage(hwnd_winamp,WM_WA_IPC,0,IPC_REFRESHPLCACHE);
** IPC_REFRESHPLCACHE will flush the playlist cache buffer.
** (send this if you want it to go refetch titles for tracks)
*/


#define IPC_GET_SHUFFLE 250
/* (requires Winamp 2.4+)
** val=SendMessage(hwnd_winamp,WM_WA_IPC,0,IPC_GET_SHUFFLE);
**
** IPC_GET_SHUFFLE returns the status of the Shuffle option (1 if set)
*/


#define IPC_GET_REPEAT 251
/* (requires Winamp 2.4+)
** val=SendMessage(hwnd_winamp,WM_WA_IPC,0,IPC_GET_REPEAT);
**
** IPC_GET_REPEAT returns the status of the Repeat option (1 if set)
*/


#define IPC_SET_SHUFFLE 252
/* (requires Winamp 2.4+)
** SendMessage(hwnd_winamp,WM_WA_IPC,value,IPC_SET_SHUFFLE);
**
** IPC_SET_SHUFFLE sets the status of the Shuffle option (1 to turn it on)
*/


#define IPC_SET_REPEAT 253
/* (requires Winamp 2.4+)
** SendMessage(hwnd_winamp,WM_WA_IPC,value,IPC_SET_REPEAT);
**
** IPC_SET_REPEAT sets the status of the Repeat option (1 to turn it on)
*/


#define IPC_ENABLEDISABLE_ALL_WINDOWS 259 // 0xdeadbeef to disable
/* (requires Winamp 2.9+)
** SendMessage(hwnd_winamp,WM_WA_IPC,enable?0:0xdeadbeef,IPC_MBOPENREAL);
** sending with 0xdeadbeef as the param disables all winamp windows, 
** any other values will enable all winamp windows.
*/


#define IPC_GETWND 260
/* (requires Winamp 2.9+)
** HWND h=SendMessage(hwnd_winamp,WM_WA_IPC,IPC_GETWND_xxx,IPC_GETWND);
** returns the HWND of the window specified.
*/
  #define IPC_GETWND_EQ 0 // use one of these for the param
  #define IPC_GETWND_PE 1
  #define IPC_GETWND_MB 2
  #define IPC_GETWND_VIDEO 3
#define IPC_ISWNDVISIBLE 261 // same param as IPC_GETWND




/************************************************************************
***************** in-process only (WE LOVE PLUGINS)
************************************************************************/


#define IPC_SETSKIN 200
/* (requires Winamp 2.04+, only usable from plug-ins (not external apps))
** SendMessage(hwnd_winamp,WM_WA_IPC,(WPARAM)"skinname",IPC_SETSKIN);
** IPC_SETSKIN sets the current skin to "skinname". Note that skinname 
** can be the name of a skin, a skin .zip file, with or without path. 
** If path isn't specified, the default search path is the winamp skins 
** directory.
*/


#define IPC_GETSKIN 201
/* (requires Winamp 2.04+, only usable from plug-ins (not external apps))
** SendMessage(hwnd_winamp,WM_WA_IPC,(WPARAM)skinname_buffer,IPC_GETSKIN);
** IPC_GETSKIN puts the directory where skin bitmaps can be found 
** into  skinname_buffer.
** skinname_buffer must be MAX_PATH characters in length.
** When using a .zip'd skin file, it'll return a temporary directory
** where the ZIP was decompressed.
*/


#define IPC_EXECPLUG 202
/* (requires Winamp 2.04+, only usable from plug-ins (not external apps))
** SendMessage(hwnd_winamp,WM_WA_IPC,(WPARAM)"vis_file.dll",IPC_EXECPLUG);
** IPC_EXECPLUG executes a visualization plug-in pointed to by WPARAM.
** the format of this string can be:
** "vis_whatever.dll"
** "vis_whatever.dll,0" // (first mod, file in winamp plug-in dir)
** "C:\\dir\\vis_whatever.dll,1" 
*/


#define IPC_GETPLAYLISTFILE 211
/* (requires Winamp 2.04+, only usable from plug-ins (not external apps))
** char *name=SendMessage(hwnd_winamp,WM_WA_IPC,index,IPC_GETPLAYLISTFILE);
** IPC_GETPLAYLISTFILE gets the filename of the playlist entry [index].
** returns a pointer to it. returns NULL on error.
*/


#define IPC_GETPLAYLISTTITLE 212
/* (requires Winamp 2.04+, only usable from plug-ins (not external apps))
** char *name=SendMessage(hwnd_winamp,WM_WA_IPC,index,IPC_GETPLAYLISTTITLE);
**
** IPC_GETPLAYLISTTITLE gets the title of the playlist entry [index].
** returns a pointer to it. returns NULL on error.
*/


#define IPC_GETHTTPGETTER 240
/* retrieves a function pointer to a HTTP retrieval function.
** if this is unsupported, returns 1 or 0.
** the function should be:
** int (*httpRetrieveFile)(HWND hwnd, char *url, char *file, char *dlgtitle);
** if you call this function, with a parent window, a URL, an output file, and a dialog title,
** it will return 0 on successful download, 1 on error.
*/


#define IPC_MBOPEN 241
/* (requires Winamp 2.05+)
** SendMessage(hwnd_winamp,WM_WA_IPC,0,IPC_MBOPEN);
** SendMessage(hwnd_winamp,WM_WA_IPC,(WPARAM)url,IPC_MBOPEN);
** IPC_MBOPEN will open a new URL in the minibrowser. if url is NULL, it will open the Minibrowser window.
*/



#define IPC_CHANGECURRENTFILE 245
/* (requires Winamp 2.05+)
** SendMessage(hwnd_winamp,WM_WA_IPC,(WPARAM)file,IPC_CHANGECURRENTFILE);
** IPC_CHANGECURRENTFILE will set the current playlist item.
*/


#define IPC_GETMBURL 246
/* (requires Winamp 2.2+)
** char buffer[4096]; // Urls can be VERY long
** SendMessage(hwnd_winamp,WM_WA_IPC,(WPARAM)buffer,IPC_GETMBURL);
** IPC_GETMBURL will retrieve the current Minibrowser URL into buffer.
** buffer must be at least 4096 bytes long.
*/


#define IPC_MBBLOCK 248
/* (requires Winamp 2.4+)
** SendMessage(hwnd_winamp,WM_WA_IPC,value,IPC_MBBLOCK);
**
** IPC_MBBLOCK will block the Minibrowser from updates if value is set to 1
*/

#define IPC_MBOPENREAL 249
/* (requires Winamp 2.4+)
** SendMessage(hwnd_winamp,WM_WA_IPC,(WPARAM)url,IPC_MBOPENREAL);
**
** IPC_MBOPENREAL works the same as IPC_MBOPEN except that it will works even if 
** IPC_MBBLOCK has been set to 1
*/

#define IPC_ADJUST_OPTIONSMENUPOS 280
/* (requires Winamp 2.9+)
** int newpos=SendMessage(hwnd_winamp,WM_WA_IPC,(WPARAM)adjust_offset,IPC_ADJUST_OPTIONSMENUPOS);
** moves where winamp expects the Options menu in the main menu. Useful if you wish to insert a
** menu item above the options/skins/vis menus.
*/

#define IPC_GET_HMENU 281
/* (requires Winamp 2.9+)
** HMENU hMenu=SendMessage(hwnd_winamp,WM_WA_IPC,(WPARAM)0,IPC_GET_HMENU);
** values for data:
** 0 : main popup menu 
** 1 : main menubar file menu
** 2 : main menubar options menu
** 3 : main menubar windows menu
** 4 : main menubar help menu
** other values will return NULL.
*/

#define IPC_GET_EXTENDED_FILE_INFO 290 //pass a pointer to the following struct in wParam
#define IPC_GET_EXTENDED_FILE_INFO_HOOKABLE 296
/* (requires Winamp 2.9+)
** to use, create an extendedFileInfoStruct, point the values filename and metadata to the
** filename and metadata field you wish to query, and ret to a buffer, with retlen to the
** length of that buffer, and then SendMessage(hwnd_winamp,WM_WA_IPC,&struct,IPC_GET_EXTENDED_FILE_INFO);
** the results should be in the buffer pointed to by ret.
** returns 1 if the decoder supports a getExtendedFileInfo method
*/
typedef struct {
  char *filename;
  char *metadata;
  char *ret;
  int retlen;
} extendedFileInfoStruct;

#define IPC_GET_BASIC_FILE_INFO 291 //pass a pointer to the following struct in wParam
typedef struct {
  char *filename;

  int quickCheck; // set to 0 to always get, 1 for quick, 2 for default (if 2, quickCheck will be set to 0 if quick wasnot used)

  // filled in by winamp
  int length;
  char *title;
  int titlelen;
} basicFileInfoStruct;

#define IPC_GET_EXTLIST 292 //returns doublenull delimited. GlobalFree() it when done. if data is 0, returns raw extlist, if 1, returns something suitable for getopenfilename

#define IPC_INFOBOX 293
typedef struct {
  HWND parent;
  char *filename;
} infoBoxParam;

#define IPC_SET_EXTENDED_FILE_INFO 294 //pass a pointer to the a extendedFileInfoStruct in wParam
/* (requires Winamp 2.9+)
** to use, create an extendedFileInfoStruct, point the values filename and metadata to the
** filename and metadata field you wish to write in ret. (retlen is not used). and then 
** SendMessage(hwnd_winamp,WM_WA_IPC,&struct,IPC_SET_EXTENDED_FILE_INFO);
** returns 1 if the metadata is supported
** Call IPC_WRITE_EXTENDED_FILE_INFO once you're done setting all the metadata you want to update
*/

#define IPC_WRITE_EXTENDED_FILE_INFO 295 
/* (requires Winamp 2.9+)
** writes all the metadata set thru IPC_SET_EXTENDED_FILE_INFO to the file
** returns 1 if the file has been successfully updated, 0 if error
*/

#define IPC_FORMAT_TITLE 297
typedef struct 
{
  char *spec; // NULL=default winamp spec
  void *p;

  char *out;
  int out_len;

  char * (*TAGFUNC)(char * tag, void * p); //return 0 if not found
  void (*TAGFREEFUNC)(char * tag,void * p);
} waFormatTitle;

#define IPC_GETUNCOMPRESSINTERFACE 331
/* returns a function pointer to uncompress().
** int (*uncompress)(unsigned char *dest, unsigned long *destLen, const unsigned char *source, unsigned long sourceLen);
** right out of zlib, useful for decompressing zlibbed data.
** if you pass the parm of 0x10100000, it will return a wa_inflate_struct * to an inflate API.
*/

typedef struct {
  int (*inflateReset)(void *strm);
  int (*inflateInit_)(void *strm,const char *version, int stream_size);
  int (*inflate)(void *strm, int flush);
  int (*inflateEnd)(void *strm);
  unsigned long (*crc32)(unsigned long crc, const unsigned  char *buf, unsigned int len);
} wa_inflate_struct;


#define IPC_ADD_PREFS_DLG 332
#define IPC_REMOVE_PREFS_DLG 333
/* (requires Winamp 2.9+)
** to use, allocate a prefsDlgRec structure (either on the heap or some global
** data, but NOT on the stack), initialze the members:
** hInst to the DLL instance where the resource is located
** dlgID to the ID of the dialog,
** proc to the window procedure for the dialog
** name to the name of the prefs page in the prefs.
** where to 0 (eventually we may add more options)
** then, SendMessage(hwnd_winamp,WM_WA_IPC,&prefsRec,IPC_ADD_PREFS_DLG);
**
** you can also IPC_REMOVE_PREFS_DLG with the address of the same prefsRec,
** but you shouldn't really ever have to.
**
*/
#define IPC_OPENPREFSTOPAGE 380 // pass an id of a builtin page, or a &prefsDlgRec of prefs page to open

typedef struct _prefsDlgRec {
  HINSTANCE hInst;
  int dlgID;
  void *proc;

  char *name;
  int where; // 0 for options, 1 for plugins, 2 for skins, 3 for bookmarks, 4 for prefs


  int _id;
  struct _prefsDlgRec *next;
} prefsDlgRec;


#define IPC_GETINIFILE 334 // returns a pointer to winamp.ini
#define IPC_GETINIDIRECTORY 335 // returns a pointer to the directory to put config files in (if you dont want to use winamp.ini)

#define IPC_SPAWNBUTTONPOPUP 361 // param =
// 0 = eject
// 1 = previous
// 2 = next
// 3 = pause
// 4 = play
// 5 = stop

#define IPC_OPENURLBOX 360 // pass a HWND to a parent, returns a HGLOBAL that needs to be freed with GlobalFree(), if successful
#define IPC_OPENFILEBOX 362 // pass a HWND to a parent
#define IPC_OPENDIRBOX 363 // pass a HWND to a parent

// pass an HWND to a parent. call this if you take over the whole UI so that the dialogs are not appearing on the
// bottom right of the screen since the main winamp window is at 3000x3000, call again with NULL to reset
#define IPC_SETDIALOGBOXPARENT 364 



// pass 0 for a copy of the skin HBITMAP
// pass 1 for name of font to use for playlist editor likeness
// pass 2 for font charset
// pass 3 for font size
#define IPC_GET_GENSKINBITMAP 503


#define IPC_GET_EMBEDIF 505 // pass an embedWindowState
// returns an HWND embedWindow(embedWindowState *); if the data is NULL, otherwise returns the HWND directly
typedef struct
{
  HWND me; //hwnd of the window

  int flags;

  RECT r;

  void *user_ptr; // for application use

  int extra_data[64]; // for internal winamp use
} embedWindowState;

#define EMBED_FLAGS_NORESIZE 1 // set this bit in embedWindowState.flags to keep window from being resizable
#define EMBED_FLAGS_NOTRANSPARENCY 2 // set this bit in embedWindowState.flags to make gen_ff turn transparency off for this wnd


#define IPC_EMBED_ENUM 532
typedef struct embedEnumStruct
{
  int (*enumProc)(embedWindowState *ws, struct embedEnumStruct *param); // return 1 to abort
  int user_data; // or more :)
} embedEnumStruct;
  // pass 

#define IPC_EMBED_ISVALID 533

#define IPC_CONVERTFILE 506
/* (requires Winamp 2.92+)
** Converts a given file to a different format (PCM, MP3, etc...)
** To use, pass a pointer to a waFileConvertStruct struct
** This struct can be either on the heap or some global
** data, but NOT on the stack. At least, until the conversion is done.
**
** eg: SendMessage(hwnd_winamp,WM_WA_IPC,&myConvertStruct,IPC_CONVERTFILE);
**
** Return value:
** 0: Can't start the conversion. Look at myConvertStruct->error for details.
** 1: Conversion started. Status messages will be sent to the specified callbackhwnd.
**    Be sure to call IPC_CONVERTFILE_END when your callback window receives the
**    IPC_CB_CONVERT_DONE message.
*/
typedef struct 
{
  char *sourcefile;  // "c:\\source.mp3"
  char *destfile;    // "c:\\dest.pcm"
  int destformat[8]; // like 'PCM ',srate,nch,bps
  HWND callbackhwnd; // window that will receive the IPC_CB_CONVERT notification messages
  
  //filled in by winamp.exe
  char *error;        //if IPC_CONVERTFILE returns 0, the reason will be here

  int bytes_done;     //you can look at both of these values for speed statistics
  int bytes_total;
  int bytes_out;

  int killswitch;     // don't set it manually, use IPC_CONVERTFILE_END
  int extra_data[64]; // for internal winamp use
} convertFileStruct;

#define IPC_CONVERTFILE_END 507
/* (requires Winamp 2.92+)
** Stop/ends a convert process started from IPC_CONVERTFILE
** You need to call this when you receive the IPC_CB_CONVERTDONE message or when you
** want to abort a conversion process
**
** eg: SendMessage(hwnd_winamp,WM_WA_IPC,&myConvertStruct,IPC_CONVERTFILE_END);
**
** No return value
*/

typedef struct {
  HWND hwndParent;
  int format;

  //filled in by winamp.exe
  HWND hwndConfig;
  int extra_data[8];
} convertConfigStruct;
#define IPC_CONVERT_CONFIG 508
#define IPC_CONVERT_CONFIG_END 509

typedef struct
{
  void (*enumProc)(int user_data, const char *desc, int fourcc);
  int user_data;
} converterEnumFmtStruct;
#define IPC_CONVERT_CONFIG_ENUMFMTS 510
/* (requires Winamp 2.92+)
*/


typedef struct
{
  char cdletter;
  char *playlist_file;
  HWND callback_hwnd;

  //filled in by winamp.exe
  char *error;
} burnCDStruct;
#define IPC_BURN_CD 511
/* (requires Winamp 5.0+)
*/

typedef struct
{
  convertFileStruct *cfs;
  int priority;
} convertSetPriority;
#define IPC_CONVERT_SET_PRIORITY 512

typedef struct
{
  char *filename;
  char *title; // 2048 bytes
  int length;
  int force_useformatting; // can set this to 1 if you want to force a url to use title formatting shit
} waHookTitleStruct;
// return TRUE if you hook this
#define IPC_HOOK_TITLES 850

#define IPC_GETSADATAFUNC 800 
// 0: returns a char *export_sa_get() that returns 150 bytes of data
// 1: returns a export_sa_setreq(int want);

#define IPC_ISMAINWNDVISIBLE 900


#define IPC_SETPLEDITCOLORS 920
typedef struct
{
  int numElems;
  int *elems;
  HBITMAP bm; // set if you want to override
} waSetPlColorsStruct;


// the following IPC use waSpawnMenuParms as parameter
#define IPC_SPAWNEQPRESETMENU 933
#define IPC_SPAWNFILEMENU 934 //menubar
#define IPC_SPAWNOPTIONSMENU 935 //menubar
#define IPC_SPAWNWINDOWSMENU 936 //menubar
#define IPC_SPAWNHELPMENU 937 //menubar
#define IPC_SPAWNPLAYMENU 938 //menubar
#define IPC_SPAWNPEFILEMENU 939 //menubar
#define IPC_SPAWNPEPLAYLISTMENU 940 //menubar
#define IPC_SPAWNPESORTMENU 941 //menubar
#define IPC_SPAWNPEHELPMENU 942 //menubar
#define IPC_SPAWNMLFILEMENU 943 //menubar
#define IPC_SPAWNMLVIEWMENU 944 //menubar
#define IPC_SPAWNMLHELPMENU 945 //menubar
#define IPC_SPAWNPELISTOFPLAYLISTS 946

typedef struct
{
  HWND wnd;
  int xpos; // in screen coordinates
  int ypos;
} waSpawnMenuParms;

// waSpawnMenuParms2 is used by the menubar submenus
typedef struct
{
  HWND wnd;
  int xpos; // in screen coordinates
  int ypos;
  int width;
  int height;
} waSpawnMenuParms2;


// system tray sends this (you might want to simulate it)
#define WM_WA_SYSTRAY WM_USER+1

// input plugins send this when they are done playing back
#define WM_WA_MPEG_EOF WM_USER+2



//// video stuff

#define IPC_IS_PLAYING_VIDEO 501 // returns >1 if playing, 0 if not, 1 if old version (so who knows):)
#define IPC_GET_IVIDEOOUTPUT 500 // see below for IVideoOutput interface
#define VIDEO_MAKETYPE(A,B,C,D) ((A) | ((B)<<8) | ((C)<<16) | ((D)<<24))
#define VIDUSER_SET_INFOSTRING 0x1000
#define VIDUSER_GET_VIDEOHWND  0x1001
#define VIDUSER_SET_VFLIP      0x1002

#ifndef NO_IVIDEO_DECLARE
#ifdef __cplusplus

class VideoOutput;
class SubsItem;

typedef	struct {
	unsigned char*	baseAddr;
	long			rowBytes;
} YV12_PLANE;

typedef	struct {
	YV12_PLANE	y;
	YV12_PLANE	u;
	YV12_PLANE	v;
} YV12_PLANES;

class IVideoOutput
{
  public:
    virtual ~IVideoOutput() { }
    virtual int open(int w, int h, int vflip, double aspectratio, unsigned int fmt)=0;
    virtual void setcallback(LRESULT (*msgcallback)(void *token, HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam), void *token) { }
    virtual void close()=0;
    virtual void draw(void *frame)=0;
    virtual void drawSubtitle(SubsItem *item) { }
    virtual void showStatusMsg(const char *text) { }
    virtual int get_latency() { return 0; }
    virtual void notifyBufferState(int bufferstate) { } /* 0-255*/

    virtual int extended(int param1, int param2, int param3) { return 0; } // Dispatchable, eat this!
};
#endif //cplusplus
#endif//NO_IVIDEO_DECLARE

// these messages are callbacks that you can grab by subclassing the winamp window

// wParam = 
#define IPC_CB_WND_EQ 0 // use one of these for the param
#define IPC_CB_WND_PE 1
#define IPC_CB_WND_MB 2
#define IPC_CB_WND_VIDEO 3
#define IPC_CB_WND_MAIN 4

#define IPC_CB_ONSHOWWND 600 
#define IPC_CB_ONHIDEWND 601 

#define IPC_CB_GETTOOLTIP 602

#define IPC_CB_MISC 603
    #define IPC_CB_MISC_TITLE 0
    #define IPC_CB_MISC_VOLUME 1 // volume/pan
    #define IPC_CB_MISC_STATUS 2
    #define IPC_CB_MISC_EQ 3
    #define IPC_CB_MISC_INFO 4
    #define IPC_CB_MISC_VIDEOINFO 5

#define IPC_CB_CONVERT_STATUS 604 // param value goes from 0 to 100 (percent)
#define IPC_CB_CONVERT_DONE   605

#define IPC_ADJUST_FFWINDOWSMENUPOS 606
/* (requires Winamp 2.9+)
** int newpos=SendMessage(hwnd_winamp,WM_WA_IPC,(WPARAM)adjust_offset,IPC_ADJUST_FFWINDOWSMENUPOS);
** moves where winamp expects the freeform windows in the menubar windows main menu. Useful if you wish to insert a
** menu item above extra freeform windows.
*/

#define IPC_ISDOUBLESIZE 608

#define IPC_ADJUST_FFOPTIONSMENUPOS 609
/* (requires Winamp 2.9+)
** int newpos=SendMessage(hwnd_winamp,WM_WA_IPC,(WPARAM)adjust_offset,IPC_ADJUST_FFOPTIONSMENUPOS);
** moves where winamp expects the freeform preferences item in the menubar windows main menu. Useful if you wish to insert a
** menu item above preferences item.
*/

#define IPC_GETTIMEDISPLAYMODE 610 // returns 0 if displaying elapsed time or 1 if displaying remaining time

#define IPC_SETVISWND 611 // param is hwnd, setting this allows you to receive ID_VIS_NEXT/PREVOUS/RANDOM/FS wm_commands
#define ID_VIS_NEXT                     40382
#define ID_VIS_PREV                     40383
#define ID_VIS_RANDOM                   40384
#define ID_VIS_FS                       40389
#define ID_VIS_CFG                      40390
#define ID_VIS_MENU                     40391

#define IPC_GETVISWND 612 // returns the vis cmd handler hwnd
#define IPC_ISVISRUNNING 613
#define IPC_CB_VISRANDOM 628 // param is status of random

#define IPC_SETIDEALVIDEOSIZE 614 // sent by winamp to winamp, trap it if you need it. width=HIWORD(param), height=LOWORD(param)

#define IPC_GETSTOPONVIDEOCLOSE 615
#define IPC_SETSTOPONVIDEOCLOSE 616

typedef struct {
  HWND hwnd;
  int uMsg;
  int wParam;
  int lParam;
} transAccelStruct;

#define IPC_TRANSLATEACCELERATOR 617

typedef struct {
  int cmd;
  int x;
  int y;
  int align;
} windowCommand; // send this as param to an IPC_PLCMD, IPC_MBCMD, IPC_VIDCMD

#define IPC_CB_ONTOGGLEAOT 618 

#define IPC_GETPREFSWND 619

#define IPC_SET_PE_WIDTHHEIGHT 620 // data is a pointer to a POINT structure that holds width & height

#define IPC_GETLANGUAGEPACKINSTANCE 621

#define IPC_CB_PEINFOTEXT 622 // data is a string, ie: "04:21/45:02"

#define IPC_CB_OUTPUTCHANGED 623 // output plugin was changed in config

#define IPC_GETOUTPUTPLUGIN 625

#define IPC_SETDRAWBORDERS 626
#define IPC_DISABLESKINCURSORS 627
#define IPC_CB_RESETFONT 629

#define IPC_IS_FULLSCREEN 630 // returns 1 if video or vis is in fullscreen mode
#define IPC_SET_VIS_FS_FLAG 631 // a vis should send this message with 1/as param to notify winamp that it has gone to or has come back from fullscreen mode

#define IPC_SHOW_NOTIFICATION 632

#define IPC_GETSKININFO 633

// >>>>>>>>>>> Next is 634

#define IPC_PLCMD  1000 

#define PLCMD_ADD  0
#define PLCMD_REM  1
#define PLCMD_SEL  2
#define PLCMD_MISC 3
#define PLCMD_LIST 4

#define IPC_MBCMD  1001 

#define MBCMD_BACK    0
#define MBCMD_FORWARD 1
#define MBCMD_STOP    2
#define MBCMD_RELOAD  3
#define MBCMD_MISC  4

#define IPC_VIDCMD 1002 

#define VIDCMD_FULLSCREEN 0
#define VIDCMD_1X         1
#define VIDCMD_2X         2
#define VIDCMD_LIB        3
#define VIDPOPUP_MISC     4

#define IPC_MBURL       1003 //sets the URL
#define IPC_MBGETCURURL 1004 //copies the current URL into wParam (have a 4096 buffer ready)
#define IPC_MBGETDESC   1005 //copies the current URL description into wParam (have a 4096 buffer ready)
#define IPC_MBCHECKLOCFILE 1006 //checks that the link file is up to date (otherwise updates it). wParam=parent HWND
#define IPC_MBREFRESH   1007 //refreshes the "now playing" view in the library
#define IPC_MBGETDEFURL 1008 //copies the default URL into wParam (have a 4096 buffer ready)

#define IPC_STATS_LIBRARY_ITEMCNT 1300 // updates library count status

// IPC 2000-3000 reserved for freeform messages, see gen_ff/ff_ipc.h
#define IPC_FF_FIRST 2000
#define IPC_FF_LAST  3000

#define IPC_GETDROPTARGET 3001

#define IPC_PLAYLIST_MODIFIED 3002 // sent to main wnd whenever the playlist is modified

#define IPC_PLAYING_FILE 3003 // sent to main wnd with the file as parm whenever a file is played
#define IPC_FILE_TAG_MAY_HAVE_UPDATED 3004 // sent to main wnd with the file as parm whenever a file tag might be updated


#define IPC_ALLOW_PLAYTRACKING 3007
// send nonzero to allow, zero to disallow

#define IPC_HOOK_OKTOQUIT 3010 // return 0 to abort a quit, nonzero if quit is OK

#define IPC_WRITECONFIG 3011 // pass 2 to write all, 1 to write playlist + common, 0 to write common+less common

// pass a string to be the name to register, and returns a value > 65536, which is a unique value you can use
// for custom WM_WA_IPC messages. 
#define IPC_REGISTER_WINAMP_IPCMESSAGE 65536 

/**************************************************************************/

/*
** Finally there are some WM_COMMAND messages that you can use to send 
** Winamp misc commands.
** 
** To send these, use:
**
** SendMessage(hwnd_winamp, WM_COMMAND,command_name,0);
*/

#define WINAMP_OPTIONS_EQ               40036 // toggles the EQ window
#define WINAMP_OPTIONS_PLEDIT           40040 // toggles the playlist window
#define WINAMP_VOLUMEUP                 40058 // turns the volume up a little
#define WINAMP_VOLUMEDOWN               40059 // turns the volume down a little
#define WINAMP_FFWD5S                   40060 // fast forwards 5 seconds
#define WINAMP_REW5S                    40061 // rewinds 5 seconds

// the following are the five main control buttons, with optionally shift 
// or control pressed
// (for the exact functions of each, just try it out)
#define WINAMP_BUTTON1                  40044
#define WINAMP_BUTTON2                  40045
#define WINAMP_BUTTON3                  40046
#define WINAMP_BUTTON4                  40047
#define WINAMP_BUTTON5                  40048
#define WINAMP_BUTTON1_SHIFT            40144
#define WINAMP_BUTTON2_SHIFT            40145
#define WINAMP_BUTTON3_SHIFT            40146
#define WINAMP_BUTTON4_SHIFT            40147
#define WINAMP_BUTTON5_SHIFT            40148
#define WINAMP_BUTTON1_CTRL             40154
#define WINAMP_BUTTON2_CTRL             40155
#define WINAMP_BUTTON3_CTRL             40156
#define WINAMP_BUTTON4_CTRL             40157
#define WINAMP_BUTTON5_CTRL             40158

#define WINAMP_FILE_PLAY                40029 // pops up the load file(s) box
#define WINAMP_FILE_DIR                 40187 // pops up the load directory box
#define WINAMP_OPTIONS_PREFS            40012 // pops up the preferences
#define WINAMP_OPTIONS_AOT              40019 // toggles always on top
#define WINAMP_HELP_ABOUT               40041 // pops up the about box :)

#define ID_MAIN_PLAY_AUDIOCD1           40323 // starts playing the audio CD in the first CD reader
#define ID_MAIN_PLAY_AUDIOCD2           40323 // plays the 2nd
#define ID_MAIN_PLAY_AUDIOCD3           40323 // plays the 3nd
#define ID_MAIN_PLAY_AUDIOCD4           40323 // plays the 4nd

// IDs 42000 to 45000 are reserved for gen_ff
// IDs from 45000 to 57000 are reserved for library 

#endif//_WA_IPC_H_

/*
** EOF.. Enjoy.
*/
