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

#include "AppleRemote.h"
#include "../../lib/c++/xbmcclient.h"

#define DEFAULT_MAX_CLICK_DURATION 0.5

#define LOG if (m_bVerbose) printf

#define APPLICATION_NAME "XBMC"

enum {
    IR_Select,
    IR_SelectHold,
    IR_Right,
    IR_Left,
    IR_Up,
    IR_Down,
    IR_RightHold,
    IR_LeftHold,
    IR_Menu,
    IR_MenuHold
};
enum {
    IR_Event_Term_ATV1X   = 5,
    IR_Event_Term_ATV20X  = 5,
    IR_Event_Term_ATV21   = 8,
    IR_Event_Term_10_4    = 5,
    IR_Event_Term_10_5    = 18
};
// magic HID key cookies AppleTV running r1.x (same as 10.4)
static std::string key_cookiesATV1X[] =
{ 
    "8_",   //SelectHold = "18_"
    "18_",
    "9_",
    "10_",
    "12_",
    "13_",
    "4_",
    "3_",
    "5_",
    "7_"
};
// magic HID key cookies AppleTV running r2.0x
static std::string key_cookiesATV20X[] =
{ 
    "8_",   //SelectHold = "18_"
    "18_",
    "9_",
    "10_",
    "12_",
    "13_",
    "4_",
    "3_",
    "7_",
    "5_",
};
// magic HID key cookies for AppleTV running r2.1
static std::string key_cookiesATV21[] =
{ 
    "9_",   //SelectHold = "19_"
    "19_",
    "10_",
    "11_",
    "13_",
    "14_",
    "5_",
    "4_",
    "8_",
    "6_"
};
// magic HID key cookies for 10.4
static std::string key_cookies10_4[] =
{ 
    "8_",   //SelectHold = "18_"
    "18_",
    "9_",
    "10_",
    "12_",
    "13_",
    "4_",
    "3_",
    "5_",
    "7_"
};

// magic HID key cookies for 10.5
static std::string key_cookies10_5[] =
{ 
    "21_",  //SelectHold = "35_"
    "35_",
    "22_",
    "23_",
    "29_",
    "30_",
    "4_",
    "3_",
    "20_",
    "18_"
};

AppleRemote::AppleRemote() :m_bVerbose(false), 
							m_remoteMode(REMOTE_NORMAL),
							m_dMaxClickDuration(DEFAULT_MAX_CLICK_DURATION), 
							m_socket(-1),
							m_timer(NULL)
{
	m_serverAddress = "localhost";
	m_appPath = "";
	m_appHome = "";
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
        // OSX 10.4/AppleTV
        size_t      len = 512;
        char        buffer[512];
        std::string hw_model = "unknown";
        
        if (sysctlbyname("hw.model", &buffer, &len, NULL, 0) == 0)
            hw_model = buffer;

        if (hw_model == "AppleTV1,1")
        {
            FILE *inpipe;
            bool atv_version_found = false;
            char linebuf[1000];

            //Find the build version of the AppleTV OS
            inpipe = popen("sw_vers -buildVersion", "r");
            if (inpipe)
            {
                //get output
                if(fgets(linebuf, sizeof(linebuf) - 1, inpipe))
                {
                    if( strstr(linebuf,"8N5107") || strstr(linebuf,"8N5239"))
                    {
                        // r1.0 or r1.1  
                        atv_version_found = true;
                        fprintf(stderr, "Using key code for AppleTV r1.x\n");
                        key = key_cookiesATV1X;
                        m_button_event_terminator = IR_Event_Term_ATV1X;
                    }
                    else if (strstr(linebuf,"8N5400") || strstr(linebuf,"8N5455") || strstr(linebuf,"8N5461"))
                    {
                        // r2.0, r2.01 or r2.02   
                        atv_version_found = true;
                        fprintf(stderr, "Using key code for AppleTV r2.0x\n");
                        key = key_cookiesATV20X;
                        m_button_event_terminator = IR_Event_Term_ATV20X;							
                    }
                    else if( strstr(linebuf,"8N5519"))
                    {
                        // r2.10   
                        atv_version_found = true;
                        fprintf(stderr, "Using key code for AppleTV r2.1\n");
                        key = key_cookiesATV21;
                        m_button_event_terminator = IR_Event_Term_ATV21;
                        
                    }
                    else if( strstr(linebuf,"8N5622"))
                    {
                        // r2.10   
                        atv_version_found = true;
                        fprintf(stderr, "Using key code for AppleTV r2.2\n");
                        key = key_cookiesATV21;
                        m_button_event_terminator = IR_Event_Term_ATV21;
                    }                    
                }
                pclose(inpipe); 
            }
            
            if(!atv_version_found){
                //handle fallback or just exit
                fprintf(stderr, "AppletTV software version could not be determined.\n");
                fprintf(stderr, "Defaulting to using key code for AppleTV r2.1\n");
                key = key_cookiesATV21;
                m_button_event_terminator = IR_Event_Term_ATV21;
            }
        }
        else
        {
            fprintf(stderr, "Using key code for OSX 10.4\n");
            key = key_cookies10_4;
            m_button_event_terminator = IR_Event_Term_10_4;
        }
    }
    else
    {
        // OSX 10.5
        fprintf(stderr, "Using key code for OSX 10.5\n");
        key = key_cookies10_5;
        m_button_event_terminator = IR_Event_Term_10_5;
    }
    m_launch_xbmc_button = key[IR_MenuHold];
    
    RegisterCommand(key[IR_Select],    new CPacketBUTTON(5, "JS0:AppleRemote", BTN_DOWN | BTN_NO_REPEAT | BTN_QUEUE));
    RegisterCommand(key[IR_SelectHold],new CPacketBUTTON(7, "JS0:AppleRemote", BTN_DOWN | BTN_NO_REPEAT | BTN_QUEUE));
    RegisterCommand(key[IR_Right],     new CPacketBUTTON(4, "JS0:AppleRemote", BTN_DOWN  | BTN_NO_REPEAT | BTN_QUEUE));
    RegisterCommand(key[IR_Left],      new CPacketBUTTON(3, "JS0:AppleRemote", BTN_DOWN  | BTN_NO_REPEAT | BTN_QUEUE));
    RegisterCommand(key[IR_Up],        new CPacketBUTTON(1, "JS0:AppleRemote", BTN_DOWN));
    RegisterCommand(key[IR_Down],      new CPacketBUTTON(2, "JS0:AppleRemote", BTN_DOWN));
    RegisterCommand(key[IR_RightHold], new CPacketBUTTON(4, "JS0:AppleRemote", BTN_DOWN));
    RegisterCommand(key[IR_LeftHold],  new CPacketBUTTON(3, "JS0:AppleRemote", BTN_DOWN));

    RegisterCommand(key[IR_Menu],      new CPacketBUTTON(6, "JS0:AppleRemote", BTN_DOWN | BTN_NO_REPEAT | BTN_QUEUE));

    // Menu Hold will be used both for sending "Back" and for starting universal remote combinations (if universal mode is on)
    RegisterCommand(key[IR_MenuHold],  new CPacketBUTTON(8, "JS0:AppleRemote", BTN_DOWN | BTN_NO_REPEAT | BTN_QUEUE));

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
		LOG("universal mode on. registering prefixes: %s\n", key[IR_MenuHold].c_str());
		m_universalPrefixes.push_back( key[IR_MenuHold] );
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
  // the path to xbmc.app is passed as an arg, 
  // use this to launch from a menu press
  LOG("Trying to start XBMC: [%s]\n", m_appPath.c_str());
  if (!m_appPath.empty())
  {
    std::string strCmd;
    
    // build a finder open command
    strCmd = "XBMC_HOME=";
    strCmd += m_appHome;
    strCmd += " ";
    strCmd += m_appPath;
    strCmd += " &";
    LOG("xbmc open command: [%s]\n", strCmd.c_str());
    system(strCmd.c_str());
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

void AppleRemote::SetAppPath(const std::string &strAddress)
{
	m_appPath = strAddress;
}

void AppleRemote::SetAppHome(const std::string &strAddress)
{
	m_appHome = strAddress;
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

    	//LOG("proc->kp_proc.p_comm: %s\n", proc->kp_proc.p_comm);
    	// Process names are at most sixteen characters long.
		if (strncmp(proc->kp_proc.p_comm, strProgram, 16) == 0)
		{
			if (ignorePid == 0 || ignorePid != proc->kp_proc.p_pid)
      		{
        		//LOG("found: %s\n", proc->kp_proc.p_comm);
				ret = true;
      		}
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

int AppleRemote::GetButtonEventTerminator(void)
{
    return(m_button_event_terminator);
}
