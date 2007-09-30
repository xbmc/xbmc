#pragma once

// defines here
#define TMSG_DIALOG_DOMODAL       100
#define TMSG_WRITE_SCRIPT_OUTPUT  101
#define TMSG_EXECUTE_SCRIPT       102
#define TMSG_EXECUTE_BUILT_IN     103

#define TMSG_MEDIA_PLAY           200
#define TMSG_MEDIA_STOP           201
#define TMSG_MEDIA_PAUSE          202
#define TMSG_MEDIA_RESTART        203

#define TMSG_PLAYLISTPLAYER_PLAY  210
#define TMSG_PLAYLISTPLAYER_NEXT  211
#define TMSG_PLAYLISTPLAYER_PREV  212

#define TMSG_PICTURE_SHOW         220
#define TMSG_PICTURE_SLIDESHOW    221
#define TMSG_SLIDESHOW_SCREENSAVER  222

#define TMSG_SHUTDOWN             300
#define TMSG_DASHBOARD            301
#define TMSG_RESTART              302
#define TMSG_RESET                303
#define TMSG_RESTARTAPP           304
#define TMSG_SWITCHTOFULLSCREEN   305

#define TMSG_HTTPAPI              400

#define TMSG_NETWORKMESSAGE         500

typedef struct
{
  DWORD dwMessage;
  DWORD dwParam1;
  DWORD dwParam2;
  string strParam;
  HANDLE hWaitEvent;
  LPVOID lpVoid;
}
ThreadMessage;

class CApplicationMessenger
{

public:
  void Cleanup();
  // if a message has to be send to the gui, use MSG_TYPE_WINDOW instead
  void SendMessage(ThreadMessage& msg, bool wait = false);
  void ProcessMessages(); // only call from main thread.
  void ProcessWindowMessages();


  void MediaPlay(string filename);
  void MediaStop();
  void MediaPause();
  void MediaRestart(bool bWait);

  void PlayListPlayerPlay();
  void PlayListPlayerPlay(int iSong);
  void PlayListPlayerNext();
  void PlayListPlayerPrevious();
  void PlayFile(const CFileItem &item, bool bRestart = false); // thread safe version of g_application.PlayFile()
  void PictureShow(string filename);
  void PictureSlideShow(string pathname, bool bScreensaver = false);
  void Shutdown();
  void Restart();
  void RebootToDashBoard();
  void RestartApp();
  void Reset();
  void SwitchToFullscreen(); //

  CStdString GetResponse();
  int SetResponse(CStdString response);
  void HttpApi(string cmd);
  void ExecBuiltIn(const CStdString &command);

  void NetworkMessage(DWORD dwMessage, DWORD dwParam = 0);
private:
  void ProcessMessage(ThreadMessage *pMsg);


  vector<ThreadMessage*> m_vecMessages;
  vector<ThreadMessage*> m_vecWindowMessages;
  CCriticalSection m_critSection;
  CCriticalSection m_critBuffer;
  CStdString bufferResponse;

};
