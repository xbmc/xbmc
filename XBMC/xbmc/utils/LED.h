#pragma once
#include "LED.h"
#include "../util.h"

#define SMARTXX_PWD_RED    0xf70c   //PWM1: SmartXX V3 port for RGB red output
#define SMARTXX_PWD_GREEN  0xf70d   //PWM2: SmartXX V3 port for RGB green output
#define SMARTXX_PWD_BLUE   0xf70e   //PWM3: SmartXX V3 port for RGB blue output
#define SMARTXX_PWM_STATUS 0xf702   //PWM4: SmartXX V3 Status LED port BLUE!

struct RGBVALUE
{
	unsigned short red;
	unsigned short green;
	unsigned short blue;
};

struct RGBVALUES
{
	CStdString strTransition;
	unsigned int iTime;

	unsigned short red1;
	unsigned short green1;
	unsigned short blue1;

	unsigned short red2;
	unsigned short green2;
	unsigned short blue2;
};

static map<CStdString,RGBVALUES>mapc_rgb;
static RGBVALUES					s_RGBs;
static CStdString					strCurrentStatus;
static CStdString					strLastStatus;
static CRITICAL_SECTION m_criticalSection;

class ILED
{
public:
  static void CLEDControl(int ixLED);
};
class ILEDSmartxxRGB : public CThread
{
protected:
	
	CStdString  strLastTransition;
	RGBVALUE s_CurRGB;
	DWORD	dwLastTime;
	DWORD dwFrameTime;
	int iSleepTime;

	void getRGBValues(CStdString strRGBa,CStdString strRGBb,RGBVALUES* s_rgb);

	bool SetRGBLed(int red, int green, int blue);
  static void outb(unsigned short port, unsigned char data)
  {
    __asm
    {
      nop
      mov dx, port
      nop
      mov al, data
      nop
      out dx,al
      nop
      nop
    }
  }

public:
	ILEDSmartxxRGB();
	~ILEDSmartxxRGB();

	virtual void OnStartup();
	virtual void OnExit();
	virtual void Process();
  virtual bool IsRunning();
  virtual bool Start();
  virtual void Stop();

	static bool SetRGBStatus(CStdString strStatus);
	static bool SetLastRGBStatus();
  bool SetRGBState(CStdString strRGB1, CStdString strRGB2, CStdString strTransition, int iTranTime);

};
extern ILEDSmartxxRGB g_iledSmartxxrgb;