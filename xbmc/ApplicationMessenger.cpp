#include "ApplicationMessenger.h"
#include "application.h"
#include "xbox/xkutils.h"

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
					XKUtils::XBOXPowerOff();
					break;

				case TMSG_DASHBOARD:
					break;

				case TMSG_RESTART:
					g_application.Stop();
					XKUtils::XBOXPowerCycle();
					break;

				case TMSG_MEDIA_PLAY:
					g_application.PlayFile(pMsg->strParam);
					if (g_application.IsPlayingVideo())
					{
						g_graphicsContext.Lock();
						g_graphicsContext.SetFullScreenVideo(true);
						g_graphicsContext.Unlock();
					}
					break;

				case TMSG_MEDIA_STOP:
					if (g_application.m_pPlayer) g_application.m_pPlayer->closefile();
					break;

				case TMSG_MEDIA_PAUSE:
					if (g_application.m_pPlayer) g_application.m_pPlayer->Pause();
					break;

				case TMSG_EXECUTE_SCRIPT:
					m_pythonParser.evalFile(pMsg->strParam.c_str());
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
					pDialog->DoModal(pMsg->dwParam2);
				}
				break;

				case TMSG_WRITE_SCRIPT_OUTPUT:
				{
					//send message to window 2004 (CGUIWindowScriptsInfo)
					CGUIMessage msg(GUI_MSG_USER, 0, 0);
					msg.SetLabel(pMsg->strParam);
					m_gWindowManager.GetWindow(2004)->OnMessage(msg);
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
		SendMessage(tMsg);
}

void CApplicationMessenger::MediaStop()
{
		ThreadMessage tMsg = {TMSG_MEDIA_STOP};
		SendMessage(tMsg);
}

void CApplicationMessenger::MediaPause()
{
		ThreadMessage tMsg = {TMSG_MEDIA_PAUSE};
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

void CApplicationMessenger::RebootToDashBoard()
{
		ThreadMessage tMsg = {TMSG_DASHBOARD};
		SendMessage(tMsg);
}