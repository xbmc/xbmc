/*
 *  AppleRemote.cpp
 *  AppleRemote
 *
 *
 */

#include <stdio.h>
#include <getopt.h>
#include <unistd.h>
#include <stdlib.h>
#include <ctype.h>
#include <sys/errno.h>
#include <sysexits.h>
#include <mach/mach.h>
#include <mach/mach_error.h>
#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/sysctl.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <mach-o/dyld.h>

#include "AppleRemote.h"
#include "../../lib/c++/xbmcclient.h"

#define DEFAULT_MAX_CLICK_DURATION 0.5

#define LOG if (m_bVerbose) printf

#define APPLICATION_NAME "XBMC"

enum {
    IR_Select,
    IR_Right,
    IR_Left,
    IR_Up,
    IR_Down,
    IR_RightHold,
    IR_LeftHold,
    IR_Menu,
    IR_MenuHold
};

// magic HID key cookies for 10.4
static std::string key_cookies10_4[] =
{ 
    "5_8_",
    "5_9_",
    "5_10_",
    "5_12_",
    "5_13_",
    "4_5_",
    "3_5_",
    "5_",
    "5_7_"
};

// magic HID key cookies for 10.5
static std::string key_cookies10_5[] =
{ 
    "21_",
    "22_",
    "23_",
    "29_",
    "30_",
    "4_",
    "3_",
    "18_",
    "20_"
};

AppleRemote::AppleRemote() :m_bVerbose(false), 
							m_remoteMode(REMOTE_NORMAL),
							m_dMaxClickDuration(DEFAULT_MAX_CLICK_DURATION), 
							m_socket(-1),
                            m_timer(NULL) 
{
	m_serverAddress = "localhost";
}

AppleRemote::~AppleRemote()
{
	DeInitialize();
}

void AppleRemote::RegisterCommand(const std::string &strSequence, CPacketBUTTON *pPacket)
{
	LOG("Registering command %s\n", strSequence.c_str());
	m_mapCommands[strSequence] = pPacket;
}

void AppleRemote::Initialize()
{
    std::string    *key;
    std::string     prefix;

	LOG("initializing apple remote\n");
	
	DeInitialize();

	m_socket = socket(AF_INET, SOCK_DGRAM, 0);
	if (m_socket < 0)
	{
		fprintf(stderr, "Error opening UDP socket! error: %d\n", errno);
		exit(1);
	}
	
	LOG("udp socket (%d) opened\n", m_socket);
	
    // Runtime Version Check
    SInt32      MacVersion;

    Gestalt(gestaltSystemVersion, &MacVersion);

    if (MacVersion < 0x1050)
    {
        key = key_cookies10_4;
    }
    else
    {
        key = key_cookies10_5;
    }
    m_launch_xbmc_button = key[IR_MenuHold];
        
    RegisterCommand(key[IR_Select],    new CPacketBUTTON("Select", "R1", BTN_DOWN | BTN_NO_REPEAT | BTN_QUEUE));
    RegisterCommand(key[IR_Right],     new CPacketBUTTON("Right", "R1", BTN_DOWN  | BTN_NO_REPEAT | BTN_QUEUE));
    RegisterCommand(key[IR_Left],      new CPacketBUTTON("Left",  "R1", BTN_DOWN  | BTN_NO_REPEAT | BTN_QUEUE));
    RegisterCommand(key[IR_Up],        new CPacketBUTTON("Up", "R1", BTN_DOWN));
    RegisterCommand(key[IR_Down],      new CPacketBUTTON("Down", "R1", BTN_DOWN));
    RegisterCommand(key[IR_RightHold], new CPacketBUTTON("Right", "R1", BTN_DOWN));
    RegisterCommand(key[IR_LeftHold],  new CPacketBUTTON("Left", "R1", BTN_DOWN));
    RegisterCommand(key[IR_Menu],      new CPacketBUTTON("Menu", "R1", BTN_DOWN | BTN_NO_REPEAT | BTN_QUEUE));

    // Menu Hold will be used both for sending "Back" and for starting universal remote combinations (if universal mode is on)
    RegisterCommand(key[IR_MenuHold],  new CPacketBUTTON("Back", "R1", BTN_DOWN | BTN_NO_REPEAT | BTN_QUEUE));

    // Universal commmands:
    RegisterCommand(key[IR_MenuHold] + key[IR_Down], new CPacketBUTTON("Back", "R1", BTN_DOWN | BTN_NO_REPEAT | BTN_QUEUE));
    
    prefix = key[IR_MenuHold] + key[IR_Select];
    RegisterCommand(prefix + key[IR_Right], new CPacketBUTTON("Info", "R1", BTN_DOWN | BTN_NO_REPEAT | BTN_QUEUE));
    RegisterCommand(prefix + key[IR_Left],  new CPacketBUTTON("Title", "R1", BTN_DOWN | BTN_NO_REPEAT | BTN_QUEUE));
    RegisterCommand(prefix + key[IR_Up],    new CPacketBUTTON("PagePlus", "R1", BTN_DOWN | BTN_NO_REPEAT | BTN_QUEUE));
    RegisterCommand(prefix + key[IR_Down],  new CPacketBUTTON("PageMinus", "R1", BTN_DOWN | BTN_NO_REPEAT | BTN_QUEUE));
    RegisterCommand(prefix + key[IR_Select],new CPacketBUTTON("Display", "R1", BTN_DOWN | BTN_NO_REPEAT | BTN_QUEUE));

    prefix = key[IR_MenuHold] + key[IR_Up];

    RegisterCommand(prefix + key[IR_Select],new CPacketBUTTON("Stop", "R1", BTN_DOWN | BTN_NO_REPEAT | BTN_QUEUE));
    RegisterCommand(prefix + key[IR_Left],  new CPacketBUTTON("Power", "R1", BTN_DOWN | BTN_NO_REPEAT | BTN_QUEUE));
    RegisterCommand(prefix + key[IR_Right], new CPacketBUTTON("Zero", "R1", BTN_DOWN | BTN_NO_REPEAT | BTN_QUEUE));
    RegisterCommand(prefix + key[IR_Up],    new CPacketBUTTON("Play", "R1", BTN_DOWN | BTN_NO_REPEAT | BTN_QUEUE));
    RegisterCommand(prefix + key[IR_Down],  new CPacketBUTTON("Pause", "R1", BTN_DOWN | BTN_NO_REPEAT | BTN_QUEUE));
    
	// for universal mode - some keys are part of a key combination.
	// when those keys are pressed, we'll wait for more to come - until a valid combination was entered or a timeout expired.
	// the keys "leading" a combination are added to the prefix vector. 
	// all we need to do to turn universal mode off is clear the prefix vector and then every command will be sent immidiately.
	if (m_remoteMode == REMOTE_UNIVERSAL)
	{
		LOG("universal mode on. registering prefixes: 20_\n");
		m_universalPrefixes.push_back("20_");
	}
	else 
	{
		LOG("universal mode off.\n");
		m_universalPrefixes.clear();
	}
	
	m_strCombination.clear();
	m_bSendUpRequired = false;
}

void AppleRemote::DeInitialize()
{
	LOG("uninitializing apple remote\n");

	ResetTimer();
	
	if (m_socket > 0)
		close(m_socket);

	m_socket = -1;
	
	LOG("deleting commands map\n");
	std::map<std::string, CPacketBUTTON *>::iterator iter = m_mapCommands.begin();
	while (iter != m_mapCommands.end())
	{
		delete iter->second;
		iter++;
	}
	m_mapCommands.clear();
	m_strCombination.clear();
}

void AppleRemote::SetTimer()
{
	if (m_timer)
		ResetTimer();

	LOG("setting timer to expire in %.2f secs. \n", m_dMaxClickDuration);
	CFRunLoopTimerContext context = { 0, this, 0, 0, 0 };
	m_timer = CFRunLoopTimerCreate(kCFAllocatorDefault, CFAbsoluteTimeGetCurrent() + m_dMaxClickDuration, 0, 0, 0, TimerCallBack, &context);
	CFRunLoopAddTimer(CFRunLoopGetCurrent(), m_timer, kCFRunLoopCommonModes);			
}

void AppleRemote::ResetTimer()
{
	if (m_timer)
	{
		LOG("removing timer\n");
		
		CFRunLoopRemoveTimer(CFRunLoopGetCurrent(), m_timer, kCFRunLoopCommonModes);
		CFRunLoopTimerInvalidate(m_timer);
		CFRelease(m_timer);
	}
	
	m_timer = NULL;
}

void AppleRemote::SetMaxClickDuration(double dDuration)
{
	m_dMaxClickDuration = dDuration;
	LOG("setting click max duration to %.2f seconds\n", dDuration);
}

void AppleRemote::TimerCallBack (CFRunLoopTimerRef timer, void *info)
{
	if (!info)
	{
		fprintf(stderr, "Error. invalid argument to timer callback\n");
		return;
	}
	
	AppleRemote *pRemote = (AppleRemote *)info;
	pRemote->SendCommand(pRemote->m_strCombination);
	pRemote->m_strCombination.clear();
	pRemote->ResetTimer();
}

bool AppleRemote::SendCommand(const std::string &key)
{
	CPacketBUTTON *pPacket = m_mapCommands[key];
	if (pPacket)
	{
		LOG("Sending command for Key (%s)\n", key.c_str());
		CAddress addr(m_serverAddress.c_str());
		pPacket->Send(m_socket, addr);
		
		if ( 0 == (pPacket->GetFlags() & BTN_NO_REPEAT) )
			m_bSendUpRequired = true;
		else
			m_bSendUpRequired = false;
	}
	
	return pPacket != NULL;
}

void AppleRemote::LaunchApp()
{
	LOG("Trying to start XBMC.\n");

	int      result = -1;
	char     given_path[2*MAXPATHLEN];
	uint32_t path_size = 2*MAXPATHLEN;

	result = _NSGetExecutablePath(given_path, &path_size);
	if (result == 0)
	{
		char real_path[2*MAXPATHLEN];
		if (realpath(given_path, real_path) != NULL)
		{
			// Move backwards out to the application.
			for (int x=0; x<4; x++)
			{
				for (int n=strlen(real_path)-1; real_path[n] != '/'; n--)
					real_path[n] = '\0';
					
				real_path[strlen(real_path)-1] = '\0';
			}
			
			std::string strCmd = "open ";
			strCmd += real_path;
			strCmd += std::string("/") + APPLICATION_NAME;
			strCmd += "&";

			// Start it in the background.
			LOG("Got path: [%s]\n", real_path);
			system(strCmd.c_str());
		}
	}
}

void AppleRemote::SendPacket(CPacketBUTTON &packet)
{
	CAddress addr(m_serverAddress.c_str());
	packet.Send(m_socket, addr);	
}

void AppleRemote::OnKeyDown(const std::string &key)
{
	if (!IsProgramRunning(APPLICATION_NAME) && (m_serverAddress == "localhost" || m_serverAddress == "127.0.0.1"))
	{
		if (key == m_launch_xbmc_button)
			LaunchApp();
			
		return;
	}
	
	LOG("key down: %s\n", key.c_str());
	if (m_remoteMode == REMOTE_NORMAL) {
		SendCommand(key);
    }
	else if (m_remoteMode == REMOTE_UNIVERSAL)
	{
		bool bCombinationStart = false;
		bool bNeedTimer = false;
		if (m_strCombination.empty())
		{
			bNeedTimer = true;
			for (size_t i=0; i<m_universalPrefixes.size(); i++)
			{
				if (m_universalPrefixes[i] == key)
				{
					LOG("start of combination (key=%s)\n", key.c_str());
					bCombinationStart = true;
					break;
				}
			}			
		}
		
		m_strCombination += key;
		
		if (bCombinationStart)
			SetTimer();
		else
		{
			if (SendCommand(m_strCombination))
			{
				m_strCombination.clear();
				ResetTimer();
			}
			else if (bNeedTimer)
				SetTimer();
		}
	}
}

void AppleRemote::OnKeyUp(const std::string &key)
{
	LOG("key up: %s\n", key.c_str());
	if (m_bSendUpRequired)
	{
		LOG("sending key-up event\n");
		CPacketBUTTON btn; 
		CAddress addr(m_serverAddress.c_str());
		btn.Send(m_socket, addr);	
	}
	else
	{
		LOG("no need to send UP event\n");
	}
}

void AppleRemote::SetVerbose(bool bVerbose)
{
	m_bVerbose = bVerbose;
}

bool AppleRemote::IsVerbose()
{
	return m_bVerbose;
}

void AppleRemote::SetMaxClickTimeout(double dTimeout)
{
	m_dMaxClickDuration = dTimeout;
}

void AppleRemote::SetServerAddress(const std::string &strAddress)
{
	m_serverAddress = strAddress;
}

const std::string &AppleRemote::GetServerAddress()
{
	return m_serverAddress;
}

void AppleRemote::SetRemoteMode(RemoteModes mode)
{
	m_remoteMode = mode;
}

bool AppleRemote::IsProgramRunning(const char* strProgram, int ignorePid)
{
	kinfo_proc* mylist = (kinfo_proc *)malloc(sizeof(kinfo_proc));
	size_t mycount = 0;
	bool ret = false;
	
	GetBSDProcessList(&mylist, &mycount);
	for(size_t k = 0; k < mycount && ret == false; k++) 
	{
		kinfo_proc *proc = NULL;
		proc = &mylist[k];

		if (strcmp(proc->kp_proc.p_comm, strProgram) == 0)
		{
			if (ignorePid == 0 || ignorePid != proc->kp_proc.p_pid)
				ret = true;
		}
	}
	free(mylist);
	return ret;
}

int AppleRemote::GetBSDProcessList(kinfo_proc **procList, size_t *procCount)
    // Returns a list of all BSD processes on the system.  This routine
    // allocates the list and puts it in *procList and a count of the
    // number of entries in *procCount.  You are responsible for freeing
    // this list (use "free" from System framework).
    // On success, the function returns 0.
    // On error, the function returns a BSD errno value.
{
    int                err;
    kinfo_proc *        result;
    bool                done;
    static const int    name[] = { CTL_KERN, KERN_PROC, KERN_PROC_ALL, 0 };
    // Declaring name as const requires us to cast it when passing it to
    // sysctl because the prototype doesn't include the const modifier.
    size_t              length;

    assert( procList != NULL);
    assert(procCount != NULL);

    *procCount = 0;

    // We start by calling sysctl with result == NULL and length == 0.
    // That will succeed, and set length to the appropriate length.
    // We then allocate a buffer of that size and call sysctl again
    // with that buffer.  If that succeeds, we're done.  If that fails
    // with ENOMEM, we have to throw away our buffer and loop.  Note
    // that the loop causes use to call sysctl with NULL again; this
    // is necessary because the ENOMEM failure case sets length to
    // the amount of data returned, not the amount of data that
    // could have been returned.

    result = NULL;
    done = false;
    do {
        assert(result == NULL);

        // Call sysctl with a NULL buffer.

        length = 0;
        err = sysctl( (int *) name, (sizeof(name) / sizeof(*name)) - 1,
                      NULL, &length,
                      NULL, 0);
        if (err == -1) {
            err = errno;
        }

        // Allocate an appropriately sized buffer based on the results
        // from the previous call.

        if (err == 0) {
            result = (kinfo_proc* )malloc(length);
            if (result == NULL) {
                err = ENOMEM;
            }
        }

        // Call sysctl again with the new buffer.  If we get an ENOMEM
        // error, toss away our buffer and start again.

        if (err == 0) {
            err = sysctl( (int *) name, (sizeof(name) / sizeof(*name)) - 1,
                          result, &length,
                          NULL, 0);
            if (err == -1) {
                err = errno;
            }
            if (err == 0) {
                done = true;
            } else if (err == ENOMEM) {
                assert(result != NULL);
                free(result);
                result = NULL;
                err = 0;
            }
        }
    } while (err == 0 && ! done);

    // Clean up and establish post conditions.

    if (err != 0 && result != NULL) {
        free(result);
        result = NULL;
    }
    *procList = result;
    if (err == 0) {
        *procCount = length / sizeof(kinfo_proc);
    }

    assert( (err == 0) == (*procList != NULL) );

    return err;
}
