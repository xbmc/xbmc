//-----------------------------------------------------------------------------
// File: DelayController.h
//
// Desc: CDelayController header file
//
// Hist: 
//
// 
//-----------------------------------------------------------------------------

#ifndef __DELAYCONTROLLER_H__
#define __DELAYCONTROLLER_H__



#define DC_SKIP									0x0001
#define DC_UP										0x0002
#define DC_DOWN									0x0004
#define DC_LEFT									0x0008
#define DC_RIGHT								0x0010
#define DC_LEFTTRIGGER					0x0020
#define DC_RIGHTTRIGGER 				0x0040

#define DC_REMOTE_DISPLAY				100
#define DC_REMOTE_PLAY					101
#define DC_REMOTE_SKIP_MINUS		102
#define DC_REMOTE_STOP					103
#define DC_REMOTE_PAUSE					104
#define DC_REMOTE_SKIP_PLUS			105
#define DC_REMOTE_TITLE					106
#define DC_REMOTE_INFO					107
#define DC_REMOTE_SELECT				108
#define DC_REMOTE_MENU				  109
#define DC_REMOTE_BACK				  110
#define DC_REMOTE_1						  200
#define DC_REMOTE_2						  201
#define DC_REMOTE_3						  202
#define DC_REMOTE_4						  203
#define DC_REMOTE_5						  204
#define DC_REMOTE_6						  205
#define DC_REMOTE_7						  206
#define DC_REMOTE_8						  207
#define DC_REMOTE_9						  208
#define DC_REMOTE_0						  209




class CDelayController
{
public:
	CDelayController( DWORD dwMoveDelay, DWORD dwRepeatDelay );
	WORD		DpadInput( WORD wDpad,bool bLeftTrigger,bool bRightTrigger  );
	WORD		IRInput( WORD wIR );
	WORD    DIRInput( WORD wDir );

	WORD		StickInput( int x, int y );
	WORD    DirInput( WORD wDir );
	void		SetDelays( DWORD dwMoveDelay, DWORD dwRepeatDelay );

protected:
	WORD				m_wLastDir;
	DWORD				m_dwTimer;
	int					m_iCount;
	DWORD				m_dwMoveDelay;
	DWORD				m_dwRepeatDelay;
};


#endif // __DELAYCONTROLLER_H__