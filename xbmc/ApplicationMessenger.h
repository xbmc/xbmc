#pragma once

#include "utils/CriticalSection.h"
#include "stdstring.h"
#include <vector>

// defines here
#define TMSG_DIALOG_DOMODAL       100
#define TMSG_WRITE_SCRIPT_OUTPUT  101
#define TMSG_EXECUTE_SCRIPT				102

#define TMSG_MEDIA_PLAY           200
#define TMSG_MEDIA_STOP           201
#define TMSG_MEDIA_PAUSE          202

#define TMSG_PLAYLISTPLAYER_PLAY  210
#define TMSG_PLAYLISTPLAYER_NEXT  211
#define TMSG_PLAYLISTPLAYER_PREV  212

#define TMSG_PICTURE_SHOW					220

#define TMSG_SHUTDOWN             300
#define TMSG_DASHBOARD            301
#define TMSG_RESTART              302
#define TMSG_RESET                303
#define TMSG_RESTARTAPP           304

typedef struct {
	DWORD dwMessage;
	DWORD dwParam1;
  DWORD dwParam2;
	string strParam;
	HANDLE hWaitEvent;
	LPVOID lpVoid;
}ThreadMessage;

using namespace std;

class CApplicationMessenger
{

public:
	void	Cleanup();
	// if a message has to be send to the gui, use MSG_TYPE_WINDOW instead
	void	SendMessage(ThreadMessage& msg, bool wait = false);
	void	ProcessMessages(); // only call from main thread.
	void	ProcessWindowMessages();


	void	MediaPlay(string filename);
	void	MediaStop();
	void	MediaPause();

	void	PlayListPlayerPlay(int iSong);
	void	PlayListPlayerNext();
	void	PlayListPlayerPrevious();
	void	PictureShow(string filename);
	void	Shutdown();
	void	Restart();
	void	RebootToDashBoard();
	void	RestartApp();
  void  Reset();

private:

	vector<ThreadMessage*>	m_vecMessages;
	vector<ThreadMessage*>	m_vecWindowMessages;
	CCriticalSection				m_critSection;

};

extern CApplicationMessenger g_applicationMessenger;
