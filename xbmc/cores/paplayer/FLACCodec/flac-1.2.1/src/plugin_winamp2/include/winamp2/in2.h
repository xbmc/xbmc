/* Standard Winamp input-plugin header
 */

#include "out.h"

/* post this to the main window at end of file (after playback as stopped) */
#define WM_WA_MPEG_EOF          (WM_USER + 2)

// note: exported symbol is now winampGetInModule2.

#define IN_VER 0x100

typedef struct
{
	int version;				// module type (IN_VER)
	char *description;			// description of module, with version string

	HWND hMainWindow;			// winamp's main window (filled in by winamp)
	HINSTANCE hDllInstance;		// DLL instance handle (Also filled in by winamp)

	char *FileExtensions;		// "mp3\0Layer 3 MPEG\0mp2\0Layer 2 MPEG\0mpg\0Layer 1 MPEG\0"
								// May be altered from Config, so the user can select what they want

	int is_seekable;			// is this stream seekable?
	int UsesOutputPlug;			// does this plug-in use the output plug-ins? (musn't ever change, ever :)

	void (*Config)(HWND hwndParent); // configuration dialog
	void (*About)(HWND hwndParent);  // about dialog

	void (*Init)();				// called at program init
	void (*Quit)();				// called at program quit

	void (*GetFileInfo)(char *file, char *title, int *length_in_ms); // if file == NULL, current playing is used
	int (*InfoBox)(char *file, HWND hwndParent);

	int (*IsOurFile)(char *fn);	// called before extension checks, to allow detection of mms://, etc
	// playback stuff
	int (*Play)(char *fn);		// return zero on success, -1 on file-not-found, some other value on other (stopping winamp) error
	void (*Pause)();			// pause stream
	void (*UnPause)();			// unpause stream
	int (*IsPaused)();			// ispaused? return 1 if paused, 0 if not
	void (*Stop)();				// stop (unload) stream

	// time stuff
	int (*GetLength)();			// get length in ms
	int (*GetOutputTime)();		// returns current output time in ms. (usually returns outMod->GetOutputTime()
	void (*SetOutputTime)(int time_in_ms);	// seeks to point in stream (in ms). Usually you signal yoru thread to seek, which seeks and calls outMod->Flush()..

	// volume stuff
	void (*SetVolume)(int volume);	// from 0 to 255.. usually just call outMod->SetVolume
	void (*SetPan)(int pan);	// from -127 to 127.. usually just call outMod->SetPan

	// in-window builtin vis stuff

	void (*SAVSAInit)(int maxlatency_in_ms, int srate);		// call once in Play(). maxlatency_in_ms should be the value returned from outMod->Open()
	// call after opening audio device with max latency in ms and samplerate
	void (*SAVSADeInit)();	// call in Stop()


	// simple vis supplying mode
	void (*SAAddPCMData)(void *PCMData, int nch, int bps, int timestamp);
											// sets the spec data directly from PCM data
											// quick and easy way to get vis working :)
											// needs at least 576 samples :)

	// advanced vis supplying mode, only use if you're cool. Use SAAddPCMData for most stuff.
	int (*SAGetMode)();		// gets csa (the current type (4=ws,2=osc,1=spec))
							// use when calling SAAdd()
	void (*SAAdd)(void *data, int timestamp, int csa); // sets the spec data, filled in by winamp


	// vis stuff (plug-in)
	// simple vis supplying mode
	void (*VSAAddPCMData)(void *PCMData, int nch, int bps, int timestamp); // sets the vis data directly from PCM data
											// quick and easy way to get vis working :)
											// needs at least 576 samples :)

	// advanced vis supplying mode, only use if you're cool. Use VSAAddPCMData for most stuff.
	int (*VSAGetMode)(int *specNch, int *waveNch); // use to figure out what to give to VSAAdd
	void (*VSAAdd)(void *data, int timestamp); // filled in by winamp, called by plug-in


	// call this in Play() to tell the vis plug-ins the current output params.
	void (*VSASetInfo)(int nch, int srate);


	// dsp plug-in processing:
	// (filled in by winamp, called by input plug)

	// returns 1 if active (which means that the number of samples returned by dsp_dosamples
	// could be greater than went in.. Use it to estimate if you'll have enough room in the
	// output buffer
	int (*dsp_isactive)();

	// returns number of samples to output. This can be as much as twice numsamples.
	// be sure to allocate enough buffer for samples, then.
	int (*dsp_dosamples)(short int *samples, int numsamples, int bps, int nch, int srate);


	// eq stuff
	void (*EQSet)(int on, char data[10], int preamp); // 0-64 each, 31 is +0, 0 is +12, 63 is -12. Do nothing to ignore.

	// info setting (filled in by winamp)
	void (*SetInfo)(int bitrate, int srate, int stereo, int synched); // if -1, changes ignored? :)

	Out_Module *outMod; // filled in by winamp, optionally used :)
} In_Module;
