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



#define DC_UP		0x0001
#define DC_DOWN		0x0002
#define DC_LEFT		0x0004
#define DC_RIGHT	0x0008
#define DC_SKIP		0x1000
#define DC_LEFTTRIGGER 0x10
#define DC_RIGHTTRIGGER 0x20

class CDelayController
{
public:
	CDelayController( DWORD dwMoveDelay, DWORD dwRepeatDelay );
	WORD	DpadInput( WORD wDpad,bool bLeftTrigger,bool bRightTrigger  );
	WORD	IRInput( WORD wIR );
	WORD	StickInput( int x, int y );
	WORD    DirInput( WORD wDir );
	void	SetDelays( DWORD dwMoveDelay, DWORD dwRepeatDelay );

protected:
	WORD				m_wLastDir;
	DWORD				m_dwTimer;
	int					m_iCount;
	DWORD				m_dwMoveDelay;
	DWORD				m_dwRepeatDelay;
};


#endif // __DELAYCONTROLLER_H__