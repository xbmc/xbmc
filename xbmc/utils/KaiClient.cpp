
#include "stdafx.h"
#include "KaiClient.h"
#include "Log.h"
#include "..\settings.h"

CKaiClient* CKaiClient::client = NULL;

struct KAIVECTOR
{
	DWORD dwTitleId;
	CHAR* szVector;
};

KAIVECTOR g_kaiVectors[] = {

0x4D530041,"Arena/XBox/Sports/Amped 2",
0x4D53001E,"Arena/XBox/Third Person Shooter/Brute Force",
0x54540010,"Arena/XBox/Driving/Carve",
0x544D000B,"Arena/XBox/Third Person Shooter/Clone Wars",
0x4C410004,"Arena/XBox/Third Person Shooter/Clone Wars",
0x4D530036,"Arena/XBox/First Person Shooter/CounterStrike",
0x4D530021,"Arena/XBox/Flying/Crimson Skies",
0x49470027,"Arena/XBox/First Person Shooter/Dead Mans Hand",
0x55530004,"Arena/XBox/Sports/Deathrow",
0x55530006,"Arena/XBox/First Person Shooter/Ghost Recon",
0x55530007,"Arena/XBox/First Person Shooter/Ghost Recon Island Thunder",
0x4D530004,"Arena/XBox/First Person Shooter/Halo",
0x4D530034,"Arena/XBox/Sports/Inside Pitch 2003",
0x4C41000B,"Arena/XBox/First Person Shooter/Jedi Academy",
0x4D53005D,"Arena/XBox/Sports/Links 2004",
0x4D530017,"Arena/XBox/Third Person Shooter/Mech Assault",
0x54540008,"Arena/XBox/Driving/Midnight Club 2",
0x4D53002A,"Arena/XBox/Driving/Midtown Madness 3",
0x54510008,"Arena/XBox/Driving/Moto GP",
0x54510016,"Arena/XBox/Driving/Moto GP 2",
0x41560022,"Arena/XBox/Driving/MTX Mototrax",
0x4D530028,"Arena/XBox/Sports/NFL Fever 2003",
0x4D53004D,"Arena/XBox/Sports/NFL Fever 2004",
0x53450017,"Arena/XBox/Sports/NHL 2K3",
0x4D53004E,"Arena/XBox/Sports/NHL Rivals 2004",
0x4D53004B,"Arena/XBox/Driving/Project Gotham Racing 2",
0x55530013,"Arena/XBox/First Person Shooter/Rainbow Six 3",
0x53450021,"Arena/XBox/Driving/Sega GT Online",
0x54540004,"Arena/XBox/First Person Shooter/Serious Sam",
0x4156001B,"Arena/XBox/First Person Shooter/Soldier Of Fortune 2",
0x55530019,"Arena/XBox/First Person Shooter/Splinter Cell Pandora Tomorrow",
0x4553000A,"Arena/XBox/First Person Shooter/Time Splitters 2",
0x4156002D,"Arena/XBox/Sports/Tony Hawk Underground",
0x41560001,"Arena/XBox/Sports/Tony Hawk 2",
0x41560004,"Arena/XBox/Sports/Tony Hawk 3",
0x41560017,"Arena/XBox/Sports/Tony Hawk 4",
0x4D530035,"Arena/XBox/Sports/Top Spin",
0x49470024,"Arena/XBox/First Person Shooter/Unreal",
0x4947003C,"Arena/XBox/First Person Shooter/Unreal II",
0x4D530027,"Arena/XBox/Third Person Shooter/Whacked",
0x41560010,"Arena/XBox/First Person Shooter/Wolfenstein",
0x55530009,"Arena/XBox/First Person Shooter/XIII",
0x186418CD,"Arena/XBox/Homebrew/XLime",

0,0};

void CKaiClient::ResolveVector(DWORD aTitleId)
{
//	CStdString strVectorMessage;
//	strVectorMessage.Format("KAI_CLIENT_VECTOR;%s;",aVector);
//	Send(server_addr, KAI_PORT, strVectorMessage);

	//Spoof response
	int i=0;
	while(g_kaiVectors[i].dwTitleId!=0)
	{
		if ( (g_kaiVectors[i].dwTitleId==aTitleId) && (observer!=NULL) )
		{
			CStdString defaultVector = g_kaiVectors[i].szVector;
			observer->OnSupportedTitle(aTitleId, defaultVector );
		}

		i++;
	}
}

CKaiClient::CKaiClient(void) : CUdpClient()
{
	CLog::Log("KAICLIENT: Instatiating...");
	observer = NULL;
	Create();
}

CKaiClient::~CKaiClient(void)
{
	client = NULL;
}

CKaiClient* CKaiClient::GetInstance()
{
	if (client==NULL)
	{
		client = new CKaiClient();
	}

	return client;
}

void CKaiClient::SetObserver(IBuddyObserver* aObserver)
{
	if (observer != aObserver)
	{
		observer = aObserver;
		Discover();
	}
}

void CKaiClient::EnterVector(CStdString& aVector)
{	
	if (client_state==State::Authenticated)
	{
		CStdString strVectorMessage;
		strVectorMessage.Format("KAI_CLIENT_VECTOR;%s;",aVector);
		Send(server_addr, strVectorMessage);
	}
}

void CKaiClient::AddContact(CStdString& aContact)
{	
	if (client_state==State::Authenticated)
	{
		CStdString strVectorMessage;
		strVectorMessage.Format("KAI_CLIENT_ADD_CONTACT;%s;",aContact);
		Send(server_addr, strVectorMessage);
	}
}

void CKaiClient::RemoveContact(CStdString& aContact)
{	
	if (client_state==State::Authenticated)
	{
		CStdString strVectorMessage;
		strVectorMessage.Format("KAI_CLIENT_REMOVE_CONTACT;%s;",aContact);
		Send(server_addr, strVectorMessage);
	}
}

void CKaiClient::Invite(CStdString& aPlayer, CStdString& aPersonalMessage, CStdString& aVector)
{
	if (client_state==State::Authenticated)
	{
		CStdString strInvitationMessage;
		strInvitationMessage.Format("KAI_CLIENT_PM;%s;%s;",aPlayer,aPersonalMessage);
		Send(server_addr, strInvitationMessage);
	}
}

void CKaiClient::GetSubVectors(CStdString& aVector)
{
	if (client_state==State::Authenticated)
	{
		CStdString strGetSubArenasMessage;
		strGetSubArenasMessage.Format("KAI_CLIENT_GET_VECTORS;%s;",aVector);
		Send(server_addr, strGetSubArenasMessage);
	}
}

void CKaiClient::ExitVector()
{
	if (client_state==State::Authenticated)
	{
		INT vectorDelimiter = client_vector.ReverseFind('/');
		if (vectorDelimiter>0)
		{
			CStdString previousVector = client_vector.Mid(0,vectorDelimiter);
			EnterVector(previousVector);
		}
	}
}

CStdString CKaiClient::GetCurrentVector()
{
	return client_vector;
}


void CKaiClient::Discover()
{	
	client_state = State::Discovering;
	CStdString strInitiateDiscoveryMessage = "KAI_CLIENT_DISCOVER;";
	Broadcast(KAI_SYSTEM_PORT, strInitiateDiscoveryMessage);
	//Send("192.168.1.2", KAI_SYSTEM_PORT, strInitiateDiscoveryMessage);
}

void CKaiClient::Attach(SOCKADDR_IN& aAddress)
{	
	client_state = State::Attaching;
	CStdString strAttachMessage = "KAI_CLIENT_ATTACH;";
	server_addr = aAddress;
	Send(server_addr, strAttachMessage);
}

void CKaiClient::TakeOver()
{	
	client_state = State::Attaching;
	CStdString strTakeOverMessage = "KAI_CLIENT_TAKEOVER;";
	Send(server_addr, strTakeOverMessage);
}

void CKaiClient::Query()
{	
	client_state = State::Querying;
	CStdString strQueryMessage = "KAI_CLIENT_GETSTATE;";
	Send(server_addr, strQueryMessage);
}

void CKaiClient::Login(LPCSTR aUsername, LPCSTR aPassword)
{	
	client_state = State::LoggingIn;
	CStdString strLoginMessage;
	strLoginMessage.Format("KAI_CLIENT_LOGIN;%s;%s;",aUsername,aPassword);
	Send(server_addr, strLoginMessage);
}

void CKaiClient::OnMessage(SOCKADDR_IN& aRemoteAddress, CStdString& aMessage)
{
//	CLog::Log(aMessage);	

	CHAR* szMessage = strtok( (char*)((LPCSTR)aMessage), ";"); 

	switch (client_state)
	{
		case State::Discovering:
			if (strcmp(szMessage,"KAI_CLIENT_ENGINE_HERE")==0)
			{
				Attach(aRemoteAddress);
			}
			break;

		case State::Attaching:
			if (strcmp(szMessage,"KAI_CLIENT_ATTACH")==0)
			{
				Query();
			}
			else if (strcmp(szMessage,"KAI_CLIENT_ENGINE_IN_USE")==0)
			{
				TakeOver();
			}
			break;

		case State::Querying:
			if (strcmp(szMessage,"KAI_CLIENT_LOGGED_IN")==0)
			{
				client_state = State::Authenticated;
//				Sleep(1000);
//				CStdString aRootVector = KAI_ARENA;
//				EnterVector(aRootVector);
			}
			else if (strcmp(szMessage,"KAI_CLIENT_NOT_LOGGED_IN")==0)
			{
				Login(g_stSettings.szOnlineUsername, g_stSettings.szOnlinePassword);
			}
			break;

		case State::LoggingIn:
			if (strcmp(szMessage,"KAI_CLIENT_USER_DATA")==0)
			{
				client_state = State::Authenticated;
//				Sleep(1000);
//				CStdString aRootVector = KAI_ARENA;
//				EnterVector(aRootVector);
			}
			break;

		case State::Authenticated:

			if ((strcmp(szMessage,"KAI_CLIENT_ADD_CONTACT")==0) || 
				(strcmp(szMessage,"KAI_CLIENT_CONTACT_OFFLINE")==0))

			{
				if (observer!=NULL)
				{
					CStdString strContactName = strtok(NULL, ";");  

					observer->OnContactOffline(	strContactName );
				}
			}
			else if (strcmp(szMessage,"KAI_CLIENT_CONTACT_ONLINE")==0)
			{
				if (observer!=NULL)
				{
					CStdString strContactName = strtok(NULL, ";");  
				  
					observer->OnContactOnline( strContactName );
				}
			}
			else if (strcmp(szMessage,"KAI_CLIENT_CONTACT_PING")==0)
			{
				if (observer!=NULL)
				{
					CHAR* szContact = strtok(NULL, ";");
					CStdString strContactName = szContact;
					CStdString strVector = "Online";

					if ( szContact[strlen(szContact)+1] != ';' )
					{
						strVector = strtok(NULL, ";");  
					}

					CStdString strPing = strtok(NULL, ";");

					observer->OnContactPing(strContactName, strVector, strPing);
				}
			}
			else if (strcmp(szMessage,"KAI_CLIENT_REMOVE_CONTACT")==0)
			{
				if (observer!=NULL)
				{
					CStdString strContactName = strtok(NULL, ";");  
				  
					observer->OnContactRemove( strContactName );
				}
			}
			else if (strcmp(szMessage,"KAI_CLIENT_VECTOR")==0)
			{
				CStdString strVector = strtok(NULL, ";");  
				if (observer!=NULL)
				{
					client_vector = strVector;
					observer->OnEnterArena(strVector);
				}
			}
			else if (strcmp(szMessage,"KAI_CLIENT_SUB_VECTOR")==0)
			{
				CStdString strVector = strtok(NULL, ";");  
				if (observer!=NULL)
				{
					observer->OnNewArena(strVector);
				}
			}
			else if (strcmp(szMessage,"KAI_CLIENT_JOINS_VECTOR")==0)
			{
				if (observer!=NULL)
				{
					CStdString strContactName = strtok(NULL, ";");  
			  
					observer->OnOpponentEnter( strContactName );
				}
			}
			else if (strcmp(szMessage,"KAI_CLIENT_ARENA_PING")==0)
			{
				if (observer!=NULL)
				{
					CStdString strContactName = strtok(NULL, ";");
					CStdString strPing = strtok(NULL, ";");

					observer->OnOpponentPing(strContactName, strPing);
				}
			}
			else if (strcmp(szMessage,"KAI_CLIENT_LEAVES_VECTOR")==0)
			{
				if (observer!=NULL)
				{
					CStdString strContactName = strtok(NULL, ";");  
			  
					observer->OnOpponentLeave( strContactName );
				}
			}
			break;
	}
}
