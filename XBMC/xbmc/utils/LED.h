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

class ILED
{
public:
  static void CLEDControl(int ixLED);
};
class ILEDSmartxxRGB : public CThread
{
protected:
  RGBVALUE  s_CurRGB;
  RGBVALUES s_RGBs;

  CStdString  strCurrentStatus;
  CStdString  strLastStatus;
	CStdString  strLastTransition;
	
	DWORD	dwLastTime;
	DWORD dwFrameTime;
	int iSleepTime;

  void outb(unsigned short port, unsigned char data);
	void getRGBValues(CStdString strRGBa,CStdString strRGBb,RGBVALUES* s_rgb);
  bool SetRGBStatus(CStdString strStatus);
	bool SetLastRGBStatus();
  
public:
	ILEDSmartxxRGB();
	~ILEDSmartxxRGB();

	virtual void OnStartup();
	virtual void OnExit();
	virtual void Process();
  virtual bool IsRunning();
  virtual bool Start();
  virtual void Stop();
  bool SetRGBState(CStdString strRGB1, CStdString strRGB2, CStdString strTransition, int iTranTime);
  
  //can used outsite to pass the values directly to the RGB port! 
  //Don't forget to check if there is a SmartXX V3/OPX! -> g_sysinfo.SmartXXModCHIP()
  bool SetRGBLed(int red, int green, int blue);

};
extern ILEDSmartxxRGB g_iledSmartxxrgb;