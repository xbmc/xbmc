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

#define TMSG_SHUTDOWN             300
#define TMSG_DASHBOARD            301
#define TMSG_RESTART              302

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
	void	Shutdown();
	void	Restart();
	void	RebootToDashBoard();

private:

	vector<ThreadMessage*>	m_vecMessages;
	vector<ThreadMessage*>	m_vecWindowMessages;
	CCriticalSection				m_critSection;

};

extern CApplicationMessenger g_applicationMessenger;
