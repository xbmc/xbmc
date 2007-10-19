/*
XBOX Front LED control
OFF/Green/Red/Orange/Cycle
10.11.2004 GeminiServer
*/
#include "stdafx.h"
#include "LED.h"
#include "SystemInfo.h"
#include "../xbox/XKUtils.h"

#include <conio.h>

ILEDSmartxxRGB g_iledSmartxxrgb;

void ILED::CLEDControl(int ixLED)
{
  if (ixLED == LED_COLOUR_OFF)
  {
    XKUtils::SetXBOXLEDStatus(0);
  }
  else if (ixLED == LED_COLOUR_GREEN)
  {
    XKUtils::SetXBOXLEDStatus(XKUtils::LED_REGISTER_CYCLE0_GREEN | XKUtils::LED_REGISTER_CYCLE2_GREEN | XKUtils::LED_REGISTER_CYCLE1_GREEN | XKUtils::LED_REGISTER_CYCLE3_GREEN);
  }
  else if (ixLED == LED_COLOUR_RED)
  {
    XKUtils::SetXBOXLEDStatus(XKUtils::LED_REGISTER_CYCLE0_RED | XKUtils::LED_REGISTER_CYCLE2_RED | XKUtils::LED_REGISTER_CYCLE1_RED | XKUtils::LED_REGISTER_CYCLE3_RED);
  }
  else if (ixLED == LED_COLOUR_ORANGE)
  {
    XKUtils::SetXBOXLEDStatus(XKUtils::LED_REGISTER_CYCLE0_ORANGE | XKUtils::LED_REGISTER_CYCLE2_ORANGE | XKUtils::LED_REGISTER_CYCLE1_ORANGE | XKUtils::LED_REGISTER_CYCLE3_ORANGE);
  }
  else if (ixLED == LED_COLOUR_CYCLE)
  {
    XKUtils::SetXBOXLEDStatus(XKUtils::LED_REGISTER_CYCLE0_GREEN | XKUtils::LED_REGISTER_CYCLE2_GREEN | XKUtils::LED_REGISTER_CYCLE1_ORANGE | XKUtils::LED_REGISTER_CYCLE3_RED);
  }
  else if (ixLED == LED_COLOUR_NO_CHANGE) //Default Bios Settings
  {
    //No need to Change! Leave the LED with the Default Bios Settings
    //Since we can't get the BIOS LED Color Setting: we will set it to Standart Green!
    //XKUtils::SetXBOXLEDStatus(XKUtils::LED_REGISTER_CYCLE0_GREEN | XKUtils::LED_REGISTER_CYCLE2_GREEN | XKUtils::LED_REGISTER_CYCLE1_GREEN | XKUtils::LED_REGISTER_CYCLE3_GREEN);
    //may be there is a way to read the LED settings from BIOS!
  }
}

ILEDSmartxxRGB::ILEDSmartxxRGB()
{
	strCurrentStatus = "NULL";
	strLastStatus = "NULL";
  
  s_RGBs.strTransition = "NULL";
	s_CurRGB.red = 0;
	s_CurRGB.green = 0;
	s_CurRGB.blue = 0;
  s_CurRGB.white = 0;
	
  dwLastTime = 0;
	iSleepTime = 0;
}

ILEDSmartxxRGB::~ILEDSmartxxRGB()
{
}
void ILEDSmartxxRGB::OnStartup()
{
  if (g_sysinfo.SmartXXModCHIP().Equals("SmartXX V3") || g_sysinfo.SmartXXModCHIP().Equals("SmartXX OPX"))
  {
	  SetThreadPriority(GetCurrentThread(),THREAD_PRIORITY_LOWEST);
	  CLog::Log(LOGDEBUG,"Starting SmartXX RGB LED thread");
    SetRGBStatus("general");
  }
}
void ILEDSmartxxRGB::Process()
{
  bool bSettingsRGBTrue = true; // for settings!
  bool bRepeat = false;
	while(!m_bStop && bSettingsRGBTrue)
	{
		dwFrameTime = timeGetTime() - dwLastTime;

    if( (s_RGBs.strTransition.IsEmpty() || s_RGBs.strTransition.Equals("none")) && !strLastTransition.Equals("none") )
		{
			strLastTransition = "none";
      s_CurRGB.red = s_RGBs.red1;
			s_CurRGB.green = s_RGBs.green1;
      s_CurRGB.blue = s_RGBs.blue1;
      s_CurRGB.white = s_RGBs.white1;

			SetRGBLed(s_CurRGB.red,s_CurRGB.green,s_CurRGB.blue, s_CurRGB.white);
		}
    else if(s_RGBs.strTransition.Equals("switch") && !strLastTransition.Equals("switch"))
		{
      if(dwFrameTime >= s_RGBs.iTime )
			{
				s_CurRGB.red = s_RGBs.red2;
				s_CurRGB.green = s_RGBs.green2;
        s_CurRGB.blue = s_RGBs.blue2;	
        s_CurRGB.white = s_RGBs.white2;
        strLastTransition = "switch";
        SetRGBLed(s_CurRGB.red,s_CurRGB.green,s_CurRGB.blue,s_CurRGB.white);
			}
			else
      {
        s_CurRGB.red = s_RGBs.red1;
			  s_CurRGB.green = s_RGBs.green1;
        s_CurRGB.blue = s_RGBs.blue1;
        s_CurRGB.white = s_RGBs.white1;
        SetRGBLed(s_CurRGB.red,s_CurRGB.green,s_CurRGB.blue,s_CurRGB.white);
      }
				
		}
    else if(s_RGBs.strTransition.Equals("blink"))
		{
			strLastTransition = "blink";
			if(dwFrameTime >= s_RGBs.iTime )
			{
				s_CurRGB.red = (s_CurRGB.red != s_RGBs.red1) ? s_RGBs.red1 : s_RGBs.red2;
				s_CurRGB.green = (s_CurRGB.green != s_RGBs.green1) ? s_RGBs.green1 : s_RGBs.green2;
        s_CurRGB.blue = (s_CurRGB.blue != s_RGBs.blue1) ? s_RGBs.blue1 : s_RGBs.blue2;	
        s_CurRGB.white= (s_CurRGB.white != s_RGBs.white1) ? s_RGBs.white1 : s_RGBs.white2;	
				dwLastTime = timeGetTime();
        SetRGBLed(s_CurRGB.red,s_CurRGB.green,s_CurRGB.blue,s_CurRGB.white);
			}
			else
				iSleepTime = s_RGBs.iTime - dwFrameTime;

		}
    else if(s_RGBs.strTransition.Equals("fade"))
		{
      if(!strLastTransition.Equals("fade"))
			{
				s_CurRGB.red = s_RGBs.red1;
				s_CurRGB.green = s_RGBs.green1;
				s_CurRGB.blue = s_RGBs.blue1;
        s_CurRGB.white = s_RGBs.white1;
				strLastTransition = "fade";
			}

			if(dwFrameTime >= s_RGBs.iTime )
			{
				if(s_CurRGB.red > s_RGBs.red2 )
					s_CurRGB.red--;
				else if(s_CurRGB.red < s_RGBs.red2)
					s_CurRGB.red++;
        
				if(s_CurRGB.green > s_RGBs.green2)
					s_CurRGB.green--;
				else if(s_CurRGB.green < s_RGBs.green2)
					s_CurRGB.green++;
        
				if(s_CurRGB.blue > s_RGBs.blue2)
					s_CurRGB.blue--;
				else if(s_CurRGB.blue < s_RGBs.blue2)
					s_CurRGB.blue++;

        if(s_CurRGB.white > s_RGBs.white2)
          s_CurRGB.white--;
        else if(s_CurRGB.white < s_RGBs.white2)
          s_CurRGB.white++;
        
				dwLastTime = timeGetTime();
        SetRGBLed(s_CurRGB.red,s_CurRGB.green,s_CurRGB.blue,s_CurRGB.white);
			}
			else
				iSleepTime = s_RGBs.iTime - dwFrameTime;
			
		}
    else if(s_RGBs.strTransition.Equals("fadeloop"))
		{
      if(!strLastTransition.Equals("fadeloop"))
			{
				s_CurRGB.red = s_RGBs.red1;
				s_CurRGB.green = s_RGBs.green1;
				s_CurRGB.blue = s_RGBs.blue1;
        s_CurRGB.white = s_RGBs.white1;
				strLastTransition = "fadeloop";
			}

			if(dwFrameTime >= s_RGBs.iTime )
			{
				if(s_CurRGB.red > s_RGBs.red2 )
					s_CurRGB.red--;
				else if(s_CurRGB.red < s_RGBs.red2)
					s_CurRGB.red++;
        
				if(s_CurRGB.green > s_RGBs.green2)
					s_CurRGB.green--;
				else if(s_CurRGB.green < s_RGBs.green2)
					s_CurRGB.green++;
        
				if(s_CurRGB.blue > s_RGBs.blue2)
					s_CurRGB.blue--;
				else if(s_CurRGB.blue < s_RGBs.blue2)
					s_CurRGB.blue++;

        if(s_CurRGB.white > s_RGBs.white2)
          s_CurRGB.white--;
        else if(s_CurRGB.white < s_RGBs.white2)
          s_CurRGB.white++;

        if (s_CurRGB.red == s_RGBs.red2 && s_CurRGB.green == s_RGBs.green2 && s_CurRGB.blue == s_RGBs.blue2 && s_CurRGB.white == s_RGBs.white2)
        { 
          strLastTransition.clear();
        }
        
				dwLastTime = timeGetTime();
        SetRGBLed(s_CurRGB.red,s_CurRGB.green,s_CurRGB.blue,s_CurRGB.white);
			}
			else
				iSleepTime = s_RGBs.iTime - dwFrameTime;
      
			
		}
    else if(s_RGBs.strTransition.Equals("faderepeat"))
		{
      if(!strLastTransition.Equals("faderepeat"))
			{
				s_CurRGB.red = (bRepeat) ? s_RGBs.red1 : s_RGBs.red2;
				s_CurRGB.green = (bRepeat) ? s_RGBs.green1 : s_RGBs.green2;
        s_CurRGB.blue = (bRepeat) ? s_RGBs.blue1 : s_RGBs.blue2;
        s_CurRGB.white = (bRepeat) ? s_RGBs.white1 : s_RGBs.white2;
				strLastTransition = "faderepeat";
			}

			if(dwFrameTime >= s_RGBs.iTime )
			{
        int i_RGB_R=0,i_RGB_G=0,i_RGB_B=0,i_RGB_W=0;
        i_RGB_R = (!bRepeat) ? s_RGBs.red1 : s_RGBs.red2;
        i_RGB_G = (!bRepeat) ? s_RGBs.green1 : s_RGBs.green2;
        i_RGB_B = (!bRepeat) ? s_RGBs.blue1 : s_RGBs.blue2;
        i_RGB_W = (!bRepeat) ? s_RGBs.white1 : s_RGBs.white2;

				if(s_CurRGB.red > i_RGB_R )
					s_CurRGB.red--;
				else if(s_CurRGB.red < i_RGB_R)
					s_CurRGB.red++;
        
				if(s_CurRGB.green > i_RGB_G)
					s_CurRGB.green--;
				else if(s_CurRGB.green < i_RGB_G)
					s_CurRGB.green++;
        
				if(s_CurRGB.blue > i_RGB_B)
					s_CurRGB.blue--;
				else if(s_CurRGB.blue < i_RGB_B)
					s_CurRGB.blue++;

        if(s_CurRGB.white > i_RGB_W)
          s_CurRGB.white--;
        else if(s_CurRGB.white < i_RGB_W)
          s_CurRGB.white++;

        if (s_CurRGB.red == i_RGB_R && s_CurRGB.green == i_RGB_G && s_CurRGB.blue == i_RGB_B && s_CurRGB.white == i_RGB_W)
        { //reset for loop
          strLastTransition.clear();
          bRepeat = !bRepeat;
        }
        SetRGBLed(s_CurRGB.red,s_CurRGB.green,s_CurRGB.blue,s_CurRGB.white);
        dwLastTime = timeGetTime();
			}
		}
		if(iSleepTime < 0)
			iSleepTime = 0;
		Sleep(iSleepTime);
	}
}

void ILEDSmartxxRGB::OnExit()
{
	SetRGBLed(0,0,0,0xb); //r=0,g=0,b=0  w=0xb (Status LED ON)
	CLog::Log(LOGDEBUG,"Stopping SmartXX RGB LED thread");
}

bool ILEDSmartxxRGB::Start()
{
  if (g_sysinfo.SmartXXModCHIP().Equals("SmartXX V3") || g_sysinfo.SmartXXModCHIP().Equals("SmartXX OPX"))
  {
    Create();
    return true;
  }
  else 
    return false;
}
void ILEDSmartxxRGB::Stop()
{
  StopThread();
}
bool ILEDSmartxxRGB::IsRunning()
{
  return (m_ThreadHandle != NULL);
}

void ILEDSmartxxRGB::getRGBValues(const CStdString &strRGBa, const CStdString &strRGBb, const CStdString &strWhiteA, const CStdString &strWhiteB, RGBVALUES* s_rgb)
{
	DWORD red=0,green=0,blue=0,white=0;
	
  int ret = sscanf(strRGBa,"#%2X%2X%2X",&red,&green,&blue); 
	if(ret == 3)
	{
		s_rgb->red1 = int(red/2);
		s_rgb->green1 = int(green/2);
		s_rgb->blue1 = int(blue/2);
	}
	else
	{
		s_rgb->red1 = 0;
		s_rgb->green1 = 0;
		s_rgb->blue1 = 0;
	}

	ret = sscanf(strRGBb,"#%2X%2X%2X",&red,&green,&blue);
	if(ret == 3)
	{
		s_rgb->red2 = int(red/2);
		s_rgb->green2 = int(green/2);
		s_rgb->blue2 = int(blue/2);
	}
	else
	{
		s_rgb->red2 = 0;
		s_rgb->green2 = 0;
		s_rgb->blue2 = 0;
	}
  
  ret = sscanf(strWhiteA,"#%2X",&white);
	if(ret == 1)
	{
    s_rgb->white1 = int(white/2);
	}
	else
	{
    s_rgb->white1 = 0;
	}

  ret = sscanf(strWhiteB,"#%2X",&white);
	if(ret == 1)
	{
    s_rgb->white2 = int(white/2);
	}
	else
	{
    s_rgb->white2 = 0;
	}
}

bool ILEDSmartxxRGB::SetRGBStatus(const CStdString &strStatus)
{
  strLastStatus = strCurrentStatus;
  strCurrentStatus = strStatus;
  return true;
}

bool ILEDSmartxxRGB::SetRGBLed(int red, int green, int blue, int white)
{
  _outp( g_sysinfo.SmartXXModCHIP().Equals("SmartXX V3") ? SMARTXX_PWD_RED:SMARTXX_OPX_PWD_RED, red);
  _outp( g_sysinfo.SmartXXModCHIP().Equals("SmartXX V3") ? SMARTXX_PWD_GREEN:SMARTXX_OPX_PWD_GREEN, green); 
  _outp( g_sysinfo.SmartXXModCHIP().Equals("SmartXX V3") ? SMARTXX_PWD_BLUE:SMARTXX_OPX_PWD_BLUE, blue);
    
  _outp( SMARTXX_PWM_STATUS, white);
  
  return true;
}

//strRGB1: from rgb state in form: #rrggbb
//strRGB2: to rgb state in form: #rrggbb
//strWhiteA: from state in form: #00 //
//strWhiteB: to state in form: #FF  //I Hope this LED port can handle this ;)
//strTransition: "none", "blink", "fade", "fadeloop", "faderepeat"
//iTranTime: Transition time in ms between transitions e.g. 50
bool ILEDSmartxxRGB::SetRGBState(const CStdString &strRGB1, const CStdString &strRGB2, const CStdString &strWhiteA, const CStdString &strWhiteB, const CStdString &strTransition, int iTranTime)
{
  // we have a new request: start reset
  strCurrentStatus = "NULL";
  strLastStatus = "NULL";
  strLastTransition = "NULL";
  s_RGBs.strTransition = "NULL";
  s_CurRGB.red = 0;
  s_CurRGB.green = 0;
  s_CurRGB.blue = 0;
  s_CurRGB.white = 0;
  iSleepTime = 0;
  dwFrameTime = 0;
  dwLastTime = timeGetTime();
  // end reset

  getRGBValues(strRGB1,strRGB2,strWhiteA,strWhiteB,&s_RGBs);
  if(!strTransition.Equals("none") || !strTransition.IsEmpty())
    s_RGBs.strTransition = strTransition;
  else
    s_RGBs.strTransition = "none";

  if (iTranTime > 0)
    s_RGBs.iTime = iTranTime;
  else
    s_RGBs.iTime = 0;

  return SetRGBStatus(strTransition);
}
