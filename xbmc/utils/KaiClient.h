#pragma once
#include "stdstring.h"
#include "UdpClient.h"

#define KAI_SYSTEM_PORT		34522
#define KAI_SYSTEM_ROOT		"Arena"

using namespace std;
#include <vector>

class IBuddyObserver
{
	public:
		virtual void OnContactOffline(CStdString& aContact)=0;
		virtual void OnContactOnline(CStdString& aContact)=0;
		virtual void OnContactPing(CStdString& aContact, CStdString& aVector, CStdString& aPing)=0;
		virtual void OnContactRemove(CStdString& aContact)=0;
		virtual void OnSupportedTitle(DWORD dwTitle, CStdString& aVector)=0;
		virtual void OnEnterArena(CStdString& aVector)=0;
		virtual void OnOpponentEnter(CStdString& aContact)=0;
		virtual void OnOpponentPing(CStdString& aOpponent, CStdString& aPing)=0;
		virtual void OnOpponentLeave(CStdString& aContact)=0;
		virtual void OnNewArena(CStdString& aVector)=0;
};

class CKaiClient : public CUdpClient
{
public:
	enum Item{Unknown=0,Player=1,Arena=2};

	static CKaiClient* GetInstance();
	virtual ~CKaiClient(void);

	void SetObserver(IBuddyObserver* aObserver);
	void EnterVector(CStdString& aVector);
	void ExitVector();
	void GetSubVectors(CStdString& aVector);
	void ResolveVector(DWORD aTitleId);
	void AddContact(CStdString& aContact);
	void RemoveContact(CStdString& aContact);
	void Invite(CStdString& aPlayer, CStdString& aPersonalMessage, CStdString& aVector);

	CStdString GetCurrentVector();

protected:
	enum State {Discovering,Attaching,Querying,LoggingIn,Authenticated};

	void Discover();
	void Attach(SOCKADDR_IN& aAddress);
	void TakeOver();
	void Query();
	void Login(LPCSTR aUsername, LPCSTR aPassword);

	virtual void OnMessage(SOCKADDR_IN& aRemoteAddress, CStdString& aMessage);

private:
	CKaiClient(void);

private:
	static CKaiClient* client;
	SOCKADDR_IN server_addr;
	State client_state;
	IBuddyObserver* observer;
	CStdString client_vector;
};
