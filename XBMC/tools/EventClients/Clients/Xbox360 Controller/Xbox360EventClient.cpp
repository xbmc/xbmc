// Xbox360EventClient.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "Xbox360Controller.h"
#include "../../lib/c++/xbmcclient.h"
#pragma comment(lib, "wsock32.lib")		  // needed for xmbclient.h?

// global variable :(
// needed for exit event handler
CXBMCClient *client;

BOOL exitHandler( DWORD ctrlType )
{
	// TODO: Send BYE Packet
	delete client;

	WSACleanup();

	return FALSE;
}

void checkTrigger(Xbox360Controller &cont, CXBMCClient *client, int num, const char* name)
{
	if (cont.triggerMoved(num))
	{
		client->SendButton(name, "XG", 0x20, cont.getTrigger(num) * 128);
	}
}

void checkThumb(Xbox360Controller &cont, CXBMCClient *client, int num,
				const char* leftname, const char* rightname)
{
	if (cont.thumbMoved(num))
	{
		if (cont.getThumb(num) < 0)
		{
			client->SendButton(leftname, "XG", 0x20, -cont.getThumb(num));
		}
		else
		{
			client->SendButton(rightname, "XG", 0x20, cont.getThumb(num));
		}
	}
}

void checkButton(Xbox360Controller &cont, CXBMCClient *client, int num, const char* name)
{
	if (cont.buttonPressed(num))
	{
		client->SendButton(name, "XG", 0x02);
	}
	else if (cont.buttonReleased(num))
	{
		client->SendButton(name, "XG", 0x04);
	}
}

int main(int argc, char* argv[])
{
	char *host = "localhost";
	char *port = "9777";

	Xbox360Controller cont(0);

	// Start Winsock stuff
	WSADATA wsaData;
	WSAStartup(MAKEWORD(2, 2), &wsaData);

	if ( argc > 3 )
	{
		printf("USAGE: %s [HOST [PORT]]\n\nThe event client connects to the XBMC EventServer at HOST:PORT.\
			    Default value for HOST is localhost, default value for port	is 9777.\n", argv[0]);
		return -1;
	}

	if ( argc > 1 )
	{
		host = argv[1];
	}

	if ( argc > 2 )
	{
		port = argv[2];
	}

	client = new CXBMCClient(host, atoi(port));

	SetConsoleCtrlHandler( (PHANDLER_ROUTINE) exitHandler, TRUE);

	client->SendHELO("Xbox 360 Controller", 0);

	while(true)
	{
		if (cont.isConnected())
		{
			cont.updateState();
			checkButton(cont, client, 0, "a");
			checkButton(cont, client, 1, "b");
			checkButton(cont, client, 2, "x");
			checkButton(cont, client, 3, "y");
			checkButton(cont, client, 4, "dpadup");
			checkButton(cont, client, 5, "dpaddown");
			checkButton(cont, client, 6, "dpadleft");
			checkButton(cont, client, 7, "dpadright");
			checkButton(cont, client, 8, "start");
			checkButton(cont, client, 9, "back");
			checkButton(cont, client, 10, "leftthumbbutton");
			checkButton(cont, client, 11, "rightthumbbutton");
			checkButton(cont, client, 12, "lefttrigger");
			checkButton(cont, client, 13, "righttrigger");
			checkTrigger(cont, client, 0, "rightanalogtrigger");
			checkTrigger(cont, client, 1, "leftanalogtrigger");
			checkThumb(cont, client, 0, "leftthumbstickleft", "leftthumbstickright");
			checkThumb(cont, client, 1, "leftthumbstickdown", "leftthumbstickup");
			checkThumb(cont, client, 2, "rightthumbstickleft", "rightthumbstickright");
			checkThumb(cont, client, 3, "rightthumbstickdown", "rightthumbstickup");
		}
		Sleep(10);
	}

	return 0;
}
