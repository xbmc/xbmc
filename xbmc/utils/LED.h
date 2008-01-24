#pragma once
#include "LED.h"
#include "../Util.h"

#define SMARTXX_PWD_RED       0xf70c   //PWM1: SmartXX V3 port for RGB red output
#define SMARTXX_PWD_GREEN     0xf70d   //PWM2: SmartXX V3 port for RGB green output
#define SMARTXX_PWD_BLUE      0xf70e   //PWM3: SmartXX V3 port for RGB blue output
#define SMARTXX_PWM_STATUS    0xf702   //PWM4: SmartXX V3 Status LED port BLUE!

#define SMARTXX_OPX_PWD_RED   0xf701    //PWM1: SmartXX OPX port for RGB red
#define SMARTXX_OPX_PWD_GREEN 0xf70c    //PWM2: SmartXX OPX port for RGB green
#define SMARTXX_OPX_PWD_BLUE  0xf70d    //PWM3: SmartXX OPX port for RGB blue

//#define SMARTXX_PWM_LIGHT	    0xF701	 //PWM5: Display Port brightness control
//#define SMARTXX_PWM_CONTRAST  0xF703	 //PWM6: Display Port contrast control

struct RGBVALUE
{
	unsigned short red;
	unsigned short green;
	unsigned short blue;
  unsigned short white;
};

struct RGBVALUES
{
	CStdString strTransition;
	DWORD iTime;

	unsigned short red1;
	unsigned short green1;
	unsigned short blue1;
  unsigned short white1;

	unsigned short red2;
	unsigned short green2;
	unsigned short blue2;
  unsigned short white2;
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

	void getRGBValues(const CStdString &strRGBa, const CStdString &strRGBb, const CStdString &strWhiteA, const CStdString &strWhiteB, RGBVALUES* s_rgb);
  bool SetRGBStatus(const CStdString &strStatus);
  
public:
	ILEDSmartxxRGB();
	~ILEDSmartxxRGB();

	virtual void OnStartup();
	virtual void OnExit();
	virtual void Process();
  virtual bool IsRunning();
  virtual bool Start();
  virtual void Stop();
  bool SetRGBState(const CStdString &strRGB1, const CStdString &strRGB2, const CStdString &strWhiteA, const CStdString &strWhiteB, const CStdString &strTransition, int iTranTime);
  
  //can used outsite to pass the values directly to the RGB port! 
  //Don't forget to check if there is a SmartXX V3/OPX! -> CSysInfo::SmartXXModCHIP()
  bool SetRGBLed(int red, int green, int blue, int white);

};
extern ILEDSmartxxRGB g_iledSmartxxrgb;