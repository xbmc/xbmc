#pragma once
#include "UdpClient.h"

#include "KaiVoice.h"
#include "../Xbox/VoiceManager.h"
#include "../Xbox/MediaPacketQueue.h"
#include "xbstopwatch.h"

#define KAI_SYSTEM_PORT		34522
#define KAI_SYSTEM_ROOT		"Arena"


class CKaiClient;
class CKaiRequestList;

class IBuddyObserver
{
	public:
		virtual void OnInitialise(CKaiClient* pClient)=0;
		virtual void OnEngineDetached()=0;
		virtual void OnAuthenticationFailed(CStdString& aUsername)=0;
		virtual void OnNetworkError(CStdString& aError)=0;
		virtual void OnNetworkReachable(CStdString& aOrbitalServerName)=0;
		virtual void OnContactOffline(CStdString& aContact)=0;
		virtual void OnContactOnline(CStdString& aContact)=0;
		virtual void OnContactsOnline(int nCount)=0;
		virtual void OnContactPing(CStdString& aContact, CStdString& aVector, DWORD aPing, int aStatus, CStdString& aBearerCapability)=0;
		virtual void OnContactRemove(CStdString& aContact)=0;
		virtual void OnContactSpeexStatus(CStdString& aContact, bool bSpeexEnabled)=0;
		virtual void OnContactSpeexRing(CStdString& aContact)=0;
		virtual void OnContactSpeex(CStdString& aContact)=0;
		virtual void OnContactInvite(CStdString& aContact, CStdString& aVector, CStdString& aTime, CStdString& aMessage)=0;
		virtual void OnSupportedTitle(DWORD dwTitle, CStdString& aVector)=0;
		virtual void OnEnterArena(CStdString& aVector, BOOL bCanCreate)=0;
		virtual void OnEnterArenaFailed(CStdString& aVector, CStdString& aReason)=0;
		virtual void OnOpponentEnter(CStdString& aContact)=0;
		virtual void OnOpponentPing(CStdString& aOpponent, DWORD aPing, int aStatus, CStdString& aBearerCapability)=0;
		virtual void OnOpponentLeave(CStdString& aContact)=0;
		virtual void OnNewArena(CStdString& aVector, CStdString& aDescription,
								int nPlayers, int nPlayerLimit, int nPassword, bool bPersonal)=0;
		virtual void OnUpdateArena(CStdString& aVector, int nPlayers)=0;
		virtual void OnUpdateOpponent(CStdString& aOpponent, CStdString& aAge, CStdString& aBandwidth,
									 CStdString& aLocation, CStdString& aBio)=0;
		virtual void OnUpdateOpponent(CStdString& aOpponent, CStdString& aAvatarURL)=0;
		virtual void OnUpdateHostingStatus(BOOL bHosting)=0;
		virtual void OnJoinsChat(CStdString& aOpponent)=0;
		virtual void OnChat(CStdString& aVector, CStdString& aOpponent, CStdString& aMessage, bool bPrivate)=0;
		virtual void OnLeavesChat(CStdString& aOpponent)=0;
};


class CKaiClient : public CUdpClient
{
public:
	enum Item{Unknown=0,Player=1,Arena=2};

	static CKaiClient* GetInstance();
	virtual ~CKaiClient(void);

	void SetObserver(IBuddyObserver* aObserver);
	void RemoveObserver();
	void EnterVector(CStdString& aVector, CStdString& aPassword);
	void JoinTextChat();
	void Chat(CStdString& aMessage);
	void ExitVector();
	void GetSubVectors(CStdString& aVector);

	void QueryVector(DWORD aTitleId);
	void QueryVectorPlayerCount(CStdString& aVector);
	
	void QueryAvatar(CStdString& aPlayerName);
	void QueryUserProfile(CStdString& aPlayerName);

	void AddContact(CStdString& aContact);
	void RemoveContact(CStdString& aContact);
	void Invite(CStdString& aPlayer, CStdString& aVector, CStdString& aMessage);
	void EnableContactVoice(CStdString& aContactName, BOOL bEnable=TRUE);
	void Host();
	void Host(CStdString& aPassword, CStdString& aDescription, int aPlayerLimit);
	void Reattach();

	CStdString GetCurrentVector();

	bool IsEngineConnected();
	bool IsNetworkReachable();
	void DoWork();

    // Public so that CVoiceManager can get to them
    static void VoiceDataCallback( DWORD dwPort, DWORD dwSize, VOID* pvData, VOID* pContext );
    static void CommunicatorCallback( DWORD dwPort, VOICE_COMMUNICATOR_EVENT event, VOID* pContext );

protected:
	enum State {Discovering,Attaching,Querying,LoggingIn,Authenticated,Disconnected};

	void Discover();
	void Attach(SOCKADDR_IN& aAddress);
	void Detach();
	void TakeOver();
	void Query();
	void Login(LPCSTR aUsername, LPCSTR aPassword);
	void SetHostingStatus(BOOL bIsHosting);
	void SetBearerCaps(BOOL bIsHeadsetPresent);

	virtual void OnMessage(SOCKADDR_IN& aRemoteAddress, CStdString& aMessage, 
		LPBYTE pMessage, DWORD dwMessageLength);

	void VoiceChatStart();
	void VoiceChatStop();

private:
	CKaiClient(void);

	void QueryClientMetrics();
	void QueueContactVoice(CStdString& aContactName, DWORD aPlayerId, LPBYTE pMessage, DWORD dwMessageLength);
	void OnVoiceData( DWORD dwPort, DWORD dwSize, VOID* pvData );
    void OnCommunicatorEvent( DWORD dwPort, VOICE_COMMUNICATOR_EVENT event );
	void SendVoiceDataToEngine();
	DWORD Crc32FromString(CStdString& aString);

private:
	static CKaiClient* client;
	SOCKADDR_IN server_addr;
	State client_state;
	IBuddyObserver* observer;
	CStdString client_vector;

	BOOL	m_bHeadset;
	BOOL	m_bHosting;
	BOOL	m_bContactsSettling;
	BOOL	m_bReachable;

	INT		m_nFriendsOnline;
	DWORD	m_dwSettlingTimer;
	DWORD	m_dwReachableTimer;

	// KAI Speex support
    LPDIRECTSOUND8		m_pDSound;
	CMediaPacketQueue*	m_pEgress;
    CXBStopWatch		m_VoiceTimer;

	// KAI Requests
	CKaiRequestList*	m_pRequestList;
};
