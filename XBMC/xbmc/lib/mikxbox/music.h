//thread code ripped off flipcode
//mikwin is from some other dude
#include "mikmod.h"
#include "mikwin.h"

MODULE *module;


class cThread
{
protected:
  HANDLE  d_threadHandle;
  DWORD   d_threadID;
  bool    d_bIsRunning;

public:
  cThread();
  virtual ~cThread();

  void Begin();
  void End();
  bool IsRunning();

  virtual DWORD ThreadProc();
};

static DWORD WINAPI cThreadProc( cThread *pThis )
{
  return pThis->ThreadProc();
}

cThread::cThread()
{
  d_threadID = 0;
  d_threadHandle = NULL;
  d_bIsRunning = false;
}


cThread::~cThread()
{
  End();
}


void cThread::Begin()
{
  if( d_threadHandle )
    End();  // just to be safe.

  // Start the thread.
  d_threadHandle = CreateThread( NULL,
                                 0,
                                 (LPTHREAD_START_ROUTINE)cThreadProc,
                                 this,
                                 0,
                                 (LPDWORD)&d_threadID );
  if( d_threadHandle == NULL )
  {
    // Arrooga! Dive, dive!  And deal with the error, too!
  }
  d_bIsRunning = true;
  SetError("started music thread");
}

 
 
void cThread::End()
{
  if( d_threadHandle != NULL )
  {
    d_bIsRunning = false;
    WaitForSingleObject( d_threadHandle, INFINITE );
    CloseHandle( d_threadHandle );
    d_threadHandle = NULL;
  }
}


DWORD cThread::ThreadProc()
{
  return 0;
}

class MusicThread : public cThread
{
public:
	void Init(char* filename);
	DWORD ThreadProc();
};

void MusicThread::Init(char* filename)
{
	char debug[255];
	if(!mikwinInit(44100, 1, 1, 1,32,32,16))
	{
        sprintf(debug,"Could not initialize sound, reason: %s\n",
                MikMod_strerror(mikwinGetErrno()));
		SetError(debug);
    }else{
		SetError("initialized sound device");
	}

	mikwinSetMusicVolume(120);
	mikwinSetSfxVolume(127);

    /* load module */
    module = Player_Load(filename, 32, 0);
    if (module) {
        /* start module */
        Player_Start(module);
		Begin();
    } else {
        sprintf(debug,"Could not load module, reason: %s\n",
                MikMod_strerror(mikwinGetErrno()));
		SetError(debug);
	}
}

DWORD MusicThread::ThreadProc()
{
	//this thing should get pumped at least once a frame at 60 fps
    DWORD dwQuantum = 1000 / 60;
	while(d_bIsRunning)
	{
		MikMod_Update();
		Sleep(dwQuantum);
	}
  return 0;
}
