/*
 *  AppleRemote.h
 *  AppleRemote
 *
 *
 */
#ifndef __APPLE__REMOTE__H__
#define __APPLE__REMOTE__H__

#include <ctype.h>
#include <Carbon/Carbon.h>

#include <vector>
#include <map>
#include <string>

typedef enum { REMOTE_NONE, REMOTE_NORMAL, REMOTE_UNIVERSAL } RemoteModes;
typedef struct kinfo_proc kinfo_proc;

class CPacketBUTTON;
class AppleRemote
{	
public:
	AppleRemote();
	virtual ~AppleRemote();

	void Initialize();
	void DeInitialize();

	void ResetTimer();
	void SetTimer();
	
    void OnKeyDown(const std::string &key);
    void OnKeyUp(const std::string &key);
		
    int  GetButtonEventTerminator(void);
    
	void SetMaxClickDuration(double dDuration);

	void SetVerbose(bool bVerbose);
	bool IsVerbose();
	
	void SetMaxClickTimeout(double dTimeout);
	void SetServerAddress(const std::string &strAddress);
	void SetAppPath(const std::string &strAddress);
	void SetAppHome(const std::string &strAddress);
	void SetRemoteMode(RemoteModes mode);
	
	const std::string &GetServerAddress();
	
	static int	GetBSDProcessList(kinfo_proc **procList, size_t *procCount);
	bool		IsProgramRunning(const char* strProgram, int ignorePid=0);
	void		LaunchApp();
	
	void		SendPacket(CPacketBUTTON &packet);
	
protected:	
    static void TimerCallBack (CFRunLoopTimerRef timer, void *info);
	void		RegisterCommand(const std::string &strSequence, CPacketBUTTON *pPacket);
	bool		SendCommand(const std::string &key);
	void		ParseConfig();
	
	bool				m_bVerbose;
	bool				m_bSendUpRequired;
	RemoteModes			m_remoteMode;
	double				m_dMaxClickDuration;
	int					m_socket;
	std::string			m_strCombination;
	std::string			m_serverAddress;
	std::string     m_appPath;
	std::string     m_appHome;

	std::map<std::string, CPacketBUTTON *>	m_mapCommands;
	std::vector<std::string>				m_universalPrefixes;
    std::string         m_launch_xbmc_button;
    int                 m_button_event_terminator;
	CFRunLoopTimerRef	m_timer;
};

#endif
