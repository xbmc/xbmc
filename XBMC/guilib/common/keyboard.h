#ifndef KEYBOARD_H
#define KEYBOARD_H

#define DEBUG_KEYBOARD
#include <xtl.h>
#include <xkbd.h>

class CKeyboard
{
public:
	CKeyboard();
	~CKeyboard();

	void Initialize();
	void Update();
	bool GetShift() {return m_bShift;};
	bool GetCtrl() {return m_bCtrl;};
	bool GetAlt() {return m_bAlt;};
	char GetAscii() {return m_cAscii;};
	BYTE GetKey() {return m_VKey;};

private:
	// variables for mouse state
	XINPUT_STATE	m_KeyboardState[4*2];					// one for each port
	HANDLE				m_hKeyboardDevice[4*2];				// handle to each device
	DWORD					m_dwKeyboardPort;						// mask of ports that currently hold a keyboard	
	XINPUT_DEBUG_KEYSTROKE m_CurrentKeyStroke;

	bool					m_bShift;
	bool					m_bCtrl;
	bool					m_bAlt;
	char					m_cAscii;
	BYTE					m_VKey;

	bool					m_bInitialized;
	bool					m_bKeyDown;
};

extern CKeyboard g_Keyboard;

#endif

