
#include "stdafx.h"
#include "ApplicationMessenger.h"
#include "application.h"
#include "xbox/xkutils.h"
#include "texturemanager.h"
#include "playlistplayer.h"
#include "util.h"

CApplicationMessenger g_applicationMessenger;

void CApplicationMessenger::Cleanup()
{
	vector<ThreadMessage*>::iterator it = m_vecMessages.begin();
	while (it != m_vecMessages.end())
	{
		ThreadMessage* pMsg = *it;
		delete pMsg;
		it = m_vecMessages.erase(it);
	}

	it = m_vecWindowMessages.begin();
	while (it != m_vecWindowMessages.end())
	{
		ThreadMessage* pMsg = *it;
		delete pMsg;
		it = m_vecWindowMessages.erase(it);
	}
}

void CApplicationMessenger::SendMessage(ThreadMessage& message, bool wait)
{
	if (wait)	message.hWaitEvent = CreateEvent(NULL, false, false, "threadWaitEvent");
	else message.hWaitEvent = NULL;

	ThreadMessage* msg = new ThreadMessage();
	msg->dwMessage = message.dwMessage;
	msg->dwParam1 = message.dwParam1;
	msg->dwParam2 = message.dwParam2;
	msg->hWaitEvent = message.hWaitEvent;
	msg->lpVoid = message.lpVoid;
	msg->strParam = message.strParam;

	CSingleLock lock(m_critSection);
	if (msg->dwMessage == TMSG_DIALOG_DOMODAL ||
			msg->dwMessage == TMSG_WRITE_SCRIPT_OUTPUT)
	{
		m_vecWindowMessages.push_back(msg);
	}
	else m_vecMessages.push_back(msg);
	lock.Leave();

	if (message.hWaitEvent)
	{
		WaitForSingleObject(message.hWaitEvent, INFINITE);
	}
}

void CApplicationMessenger::ProcessMessages()
{
	// process threadmessages
	CSingleLock lock(m_critSection);
	if (m_vecMessages.size() > 0)
	{
		vector<ThreadMessage*>::iterator it = m_vecMessages.begin();
		while (it != m_vecMessages.end())
		{
			ThreadMessage* pMsg = *it;
			//first remove the message from the queue, else the message could be processed more then once
			it = m_vecMessages.erase(it);

			switch (pMsg->dwMessage)
			{
				case TMSG_SHUTDOWN:
					g_application.Stop();
          Sleep(200);
          XKUtils::XBOXPowerOff();
				break;

				case TMSG_DASHBOARD:
				break;

				case TMSG_RESTART:
					g_application.Stop();
          Sleep(200);
					XKUtils::XBOXPowerCycle();
				break;

        case TMSG_RESET:
					g_application.Stop();
          Sleep(200);
          XKUtils::XBOXReset();
				break;

        case TMSG_RESTARTAPP:
          {
	          char szXBEFileName[1024];
	          CIoSupport helper;
	          helper.GetXbePath(szXBEFileName);
	          CUtil::RunXBE(szXBEFileName);
          }
				break;

				case TMSG_MEDIA_PLAY:
				{
					// restore to previous window if needed
					if (m_gWindowManager.GetActiveWindow() == WINDOW_SLIDESHOW ||
							m_gWindowManager.GetActiveWindow() == WINDOW_FULLSCREEN_VIDEO ||
							m_gWindowManager.GetActiveWindow() == WINDOW_VISUALISATION)
						m_gWindowManager.PreviousWindow();

					//g_application.StopPlaying();
					// play file
					CFileItem item;
					item.m_strPath = pMsg->strParam;
					g_application.PlayFile(item);
				}
				break;

				case TMSG_PICTURE_SHOW:
				{
					CGUIWindowSlideShow *pSlideShow = (CGUIWindowSlideShow *)m_gWindowManager.GetWindow(WINDOW_SLIDESHOW);
					if (!pSlideShow) return;

					// stop playing file
					if(g_application.IsPlayingVideo()) g_application.StopPlaying();

					if (m_gWindowManager.GetActiveWindow() == WINDOW_FULLSCREEN_VIDEO)
						m_gWindowManager.PreviousWindow();

					g_graphicsContext.Lock();
					pSlideShow->Reset();
					if (m_gWindowManager.GetActiveWindow() != WINDOW_SLIDESHOW)
						m_gWindowManager.ActivateWindow(WINDOW_SLIDESHOW);
					pSlideShow->Add(pMsg->strParam);
					pSlideShow->Select(pMsg->strParam);
					g_graphicsContext.Unlock();
				}
				break;

				case TMSG_MEDIA_STOP:
				{
					// restore to previous window if needed
					if (m_gWindowManager.GetActiveWindow() == WINDOW_SLIDESHOW ||
							m_gWindowManager.GetActiveWindow() == WINDOW_FULLSCREEN_VIDEO ||
							m_gWindowManager.GetActiveWindow() == WINDOW_VISUALISATION)
						m_gWindowManager.PreviousWindow();

					// stop playing file
					if(g_application.IsPlaying()) g_application.StopPlaying();
				}
				break;

				case TMSG_MEDIA_PAUSE:
					if (g_application.m_pPlayer) g_application.m_pPlayer->Pause();
				break;

				case TMSG_EXECUTE_SCRIPT:
					g_pythonParser.evalFile(pMsg->strParam.c_str());
				break;

				case TMSG_PLAYLISTPLAYER_PLAY:
					g_playlistPlayer.Play(pMsg->dwParam1);
				break;

				case TMSG_PLAYLISTPLAYER_NEXT:
					g_playlistPlayer.PlayNext();
				break;
				
				case TMSG_PLAYLISTPLAYER_PREV:
					g_playlistPlayer.PlayPrevious();
				break;
			}
			if (pMsg->hWaitEvent)
			{
				PulseEvent(pMsg->hWaitEvent);
				CloseHandle(pMsg->hWaitEvent);
			}
			delete pMsg;
		}
	}
}

void CApplicationMessenger::ProcessWindowMessages()
{
	CSingleLock lock(m_critSection);
	//message type is window, process window messages
	if (m_vecWindowMessages.size() > 0)
	{
		vector<ThreadMessage*>::iterator it = m_vecWindowMessages.begin();
		while (it != m_vecWindowMessages.end())
		{
			ThreadMessage* pMsg = *it;
			//first remove the message from the queue, else the message could be processed more then once
			it = m_vecWindowMessages.erase(it);

			switch (pMsg->dwMessage)
			{
				case TMSG_DIALOG_DOMODAL: //doModel of window
				{
					CGUIDialog* pDialog = (CGUIDialog*)m_gWindowManager.GetWindow(pMsg->dwParam1);
          if (!pDialog) return;
					pDialog->DoModal(pMsg->dwParam2);
				}
				break;

				case TMSG_WRITE_SCRIPT_OUTPUT:
				{
					//send message to window 2004 (CGUIWindowScriptsInfo)
					CGUIMessage msg(GUI_MSG_USER, 0, 0);
					msg.SetLabel(pMsg->strParam);
					CGUIWindow* pWindowScripts=m_gWindowManager.GetWindow(WINDOW_SCRIPTS_INFO);
          if (pWindowScripts) pWindowScripts->OnMessage(msg);
				}
				break;
			}
			if (pMsg->hWaitEvent)
			{
				PulseEvent(pMsg->hWaitEvent);
				CloseHandle(pMsg->hWaitEvent);
			}
			delete pMsg;
		}
	}
	lock.Leave();
}

void CApplicationMessenger::MediaPlay(string filename)
{
		ThreadMessage tMsg = {TMSG_MEDIA_PLAY};
		tMsg.strParam = filename;
		SendMessage(tMsg, true);
}

void CApplicationMessenger::MediaStop()
{
		ThreadMessage tMsg = {TMSG_MEDIA_STOP};
		SendMessage(tMsg, true);
}

void CApplicationMessenger::MediaPause()
{
		ThreadMessage tMsg = {TMSG_MEDIA_PAUSE};
		SendMessage(tMsg, true);
}

void CApplicationMessenger::PlayListPlayerPlay(int iSong)
{
		ThreadMessage tMsg = {TMSG_PLAYLISTPLAYER_PLAY, iSong};
		SendMessage(tMsg, true);
}

void CApplicationMessenger::PlayListPlayerNext()
{
		ThreadMessage tMsg = {TMSG_PLAYLISTPLAYER_NEXT};
		SendMessage(tMsg, true);
}

void CApplicationMessenger::PlayListPlayerPrevious()
{
		ThreadMessage tMsg = {TMSG_PLAYLISTPLAYER_PREV};
		SendMessage(tMsg, true);
}

void CApplicationMessenger::PictureShow(string filename)
{
	ThreadMessage tMsg = {TMSG_PICTURE_SHOW};
	tMsg.strParam = filename;
	SendMessage(tMsg);
}

void CApplicationMessenger::Shutdown()
{
		ThreadMessage tMsg = {TMSG_SHUTDOWN};
		SendMessage(tMsg);
}

void CApplicationMessenger::Restart()
{
		ThreadMessage tMsg = {TMSG_RESTART};
		SendMessage(tMsg);
}

void CApplicationMessenger::Reset()
{
		ThreadMessage tMsg = {TMSG_RESET};
		SendMessage(tMsg);
}

void CApplicationMessenger::RestartApp()
{
		ThreadMessage tMsg = {TMSG_RESTARTAPP};
		SendMessage(tMsg);
}

void CApplicationMessenger::RebootToDashBoard()
{
		ThreadMessage tMsg = {TMSG_DASHBOARD};
		SendMessage(tMsg);
}