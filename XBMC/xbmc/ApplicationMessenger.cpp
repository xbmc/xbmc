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
}

void CApplicationMessenger::SendMessage(ThreadMessage& message, bool wait)
{
	if (wait)	message.hWaitEvent = CreateEvent(NULL, true, false, "threadWaitEvent");
	else message.hWaitEvent = NULL;

	ThreadMessage* msg = new ThreadMessage();
	msg->dwMessage = message.dwMessage;
	msg->dwParam1 = message.dwParam1;
	msg->dwParam2 = message.dwParam2;
	msg->hWaitEvent = message.hWaitEvent;
	msg->lpVoid = message.lpVoid;
	msg->strParam = message.strParam;

	CSingleLock lock(m_critSection);
	m_vecMessages.push_back(msg);
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
				case TMSG_DIALOG_DOMODAL: //doModel of window
				{
					CGUIDialog* pDialog = (CGUIDialog*)m_gWindowManager.GetWindow(pMsg->dwParam1);
					pDialog->DoModal(pMsg->dwParam2);
				}
				break;

				case TMSG_WRITE_SCRIPT_OUTPUT:
				{
					//send message to window 20 (CGUIWindowScripts)
					CGUIMessage msg(GUI_MSG_USER, 20, 0,0,0, NULL);
					msg.SetLabel(pMsg->strParam);
					g_graphicsContext.SendMessage(msg);
				}
				break;

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