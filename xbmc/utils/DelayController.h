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



#define DC_SKIP							0x0001
#define DC_UP							0x0002
#define DC_DOWN							0x0004
#define DC_LEFT							0x0008
#define DC_RIGHT						0x0010
#define DC_LEFTTRIGGER					0x0020
#define DC_RIGHTTRIGGER 				0x0040



class CDelayController
{
public:
	CDelayController(DWORD dwMoveDelay, DWORD dwRepeatDelay );
	WORD		DpadInput( WORD wDpad,bool bLeftTrigger,bool bRightTrigger  );
	WORD		IRInput( WORD wIR );
	WORD		StickInput( int x, int y );

	WORD        DirInput( WORD wDir );
	void		SetDelays( DWORD dwMoveDelay, DWORD dwRepeatDelay );

protected:
	WORD		m_wLastDir;
	bool		m_bRepeat;
	DWORD		m_dwRepeatDelay;
	DWORD		m_dwRepeatDelayOrig;
	DWORD       m_dwMoveDelay;
	DWORD       m_dwLastTime;
	DWORD		m_dwLastRepeat;
};


#endif // __DELAYCONTROLLER_H__