/*
XBOX Front LED control
OFF/Green/Red/Orange/Cycle
10.11.2004 GeminiServer
*/
#include "../stdafx.h"
#include "LED.h"
#include "SystemInfo.h"
#include "../xbox/XKUtils.h"

ILEDSmartxxRGB g_iledSmartxxrgb;

void ILED::CLEDControl(int ixLED)
{
  if (ixLED == LED_COLOUR_OFF)
  {
    XKUtils::SetXBOXLEDStatus(0);
	  //g_sysinfo.SmartXXLEDControll(SMARTXX_LED_OFF);
  }
  else if (ixLED == LED_COLOUR_GREEN)
  {
    XKUtils::SetXBOXLEDStatus(XKUtils::LED_REGISTER_CYCLE0_GREEN | XKUtils::LED_REGISTER_CYCLE2_GREEN | XKUtils::LED_REGISTER_CYCLE1_GREEN | XKUtils::LED_REGISTER_CYCLE3_GREEN);
	  //g_sysinfo.SmartXXLEDControll(SMARTXX_LED_BLUE);
  }
  else if (ixLED == LED_COLOUR_RED)
  {
    XKUtils::SetXBOXLEDStatus(XKUtils::LED_REGISTER_CYCLE0_RED | XKUtils::LED_REGISTER_CYCLE2_RED | XKUtils::LED_REGISTER_CYCLE1_RED | XKUtils::LED_REGISTER_CYCLE3_RED);
	  //g_sysinfo.SmartXXLEDControll(SMARTXX_LED_RED);
  }
  else if (ixLED == LED_COLOUR_ORANGE)
  {
    XKUtils::SetXBOXLEDStatus(XKUtils::LED_REGISTER_CYCLE0_ORANGE | XKUtils::LED_REGISTER_CYCLE2_ORANGE | XKUtils::LED_REGISTER_CYCLE1_ORANGE | XKUtils::LED_REGISTER_CYCLE3_ORANGE);
	  //g_sysinfo.SmartXXLEDControll(SMARTXX_LED_BLUE_RED);
  }
  else if (ixLED == LED_COLOUR_CYCLE)
  {
    XKUtils::SetXBOXLEDStatus(XKUtils::LED_REGISTER_CYCLE0_GREEN | XKUtils::LED_REGISTER_CYCLE2_GREEN | XKUtils::LED_REGISTER_CYCLE1_ORANGE | XKUtils::LED_REGISTER_CYCLE3_RED);
	  //g_sysinfo.SmartXXLEDControll(SMARTXX_LED_CYCLE);
  }
  else if (ixLED == LED_COLOUR_NO_CHANGE) //Default Bios Settings
  {
    //No need to Change! Leave the LED with the Default Bios Settings
    //Since we can't get the BIOS LED Color Setting: we will set it to Standart Green!
    XKUtils::SetXBOXLEDStatus(XKUtils::LED_REGISTER_CYCLE0_GREEN | XKUtils::LED_REGISTER_CYCLE2_GREEN | XKUtils::LED_REGISTER_CYCLE1_GREEN | XKUtils::LED_REGISTER_CYCLE3_GREEN);
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
    outb(SMARTXX_PWM_STATUS, 0x0);  // Status LED OFF
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
			SetRGBLed(s_CurRGB.red,s_CurRGB.green,s_CurRGB.blue);
		}
    else if(s_RGBs.strTransition.Equals("switch") && !strLastTransition.Equals("switch"))
		{
      if(dwFrameTime >= s_RGBs.iTime )
			{
				s_CurRGB.red = s_RGBs.red2;
				s_CurRGB.green = s_RGBs.green2;
        s_CurRGB.blue = s_RGBs.blue2;	
        strLastTransition = "switch";
        SetRGBLed(s_CurRGB.red,s_CurRGB.green,s_CurRGB.blue);
			}
			else
      {
        s_CurRGB.red = s_RGBs.red1;
			  s_CurRGB.green = s_RGBs.green1;
        s_CurRGB.blue = s_RGBs.blue1;
        SetRGBLed(s_CurRGB.red,s_CurRGB.green,s_CurRGB.blue);
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
				dwLastTime = timeGetTime();
				SetRGBLed(s_CurRGB.red,s_CurRGB.green,s_CurRGB.blue);
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
        
				dwLastTime = timeGetTime();
				SetRGBLed(s_CurRGB.red,s_CurRGB.green,s_CurRGB.blue);
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

        if (s_CurRGB.red == s_RGBs.red2 && s_CurRGB.green == s_RGBs.green2 && s_CurRGB.blue == s_RGBs.blue2)
        { 
          strLastTransition.clear();
        }
        
				dwLastTime = timeGetTime();
				SetRGBLed(s_CurRGB.red,s_CurRGB.green,s_CurRGB.blue);
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
				strLastTransition = "faderepeat";
			}

			if(dwFrameTime >= s_RGBs.iTime )
			{
        int i_RGB_R=0,i_RGB_G=0,i_RGB_B=0;
        i_RGB_R = (!bRepeat) ? s_RGBs.red1 : s_RGBs.red2;
        i_RGB_G = (!bRepeat) ? s_RGBs.green1 : s_RGBs.green2;
        i_RGB_B = (!bRepeat) ? s_RGBs.blue1 : s_RGBs.blue2;

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

        if (s_CurRGB.red == i_RGB_R && s_CurRGB.green == i_RGB_G && s_CurRGB.blue == i_RGB_B)
        { //reset for loop
          strLastTransition.clear();
          bRepeat = !bRepeat;
        }
				dwLastTime = timeGetTime();
				SetRGBLed(s_CurRGB.red,s_CurRGB.green,s_CurRGB.blue);
			}
		}
		if(iSleepTime < 0)
			iSleepTime = 0;
		Sleep(iSleepTime);
	}
}

void ILEDSmartxxRGB::OnExit()
{
	SetRGBLed(0,0,0);
  outb(SMARTXX_PWM_STATUS, 0xb);  // Status LED ON
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
void ILEDSmartxxRGB::outb(unsigned short port, unsigned char data)
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
void ILEDSmartxxRGB::getRGBValues(CStdString strRGBa,CStdString strRGBb,RGBVALUES* s_rgb)
{
	DWORD red=0,green=0,blue=0;
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
}

bool ILEDSmartxxRGB::SetRGBStatus(CStdString strStatus)
{
  strLastStatus = strCurrentStatus;
	strCurrentStatus = strStatus;
	return true;
}

bool ILEDSmartxxRGB::SetLastRGBStatus()
{
	return SetRGBStatus(strLastStatus);
}

bool ILEDSmartxxRGB::SetRGBLed(int red, int green, int blue)
{
	outb(SMARTXX_PWD_RED,red);
	outb(SMARTXX_PWD_GREEN,green); 
	outb(SMARTXX_PWD_BLUE,blue);
  return true;
}

//strRGB1: from rgb state in form: #rrggbb
//strRGB2: to rgb state in form: #rrggbb
//strTransition: "none", "blink", "fade", "fadeloop", "faderepeat"
//iTranTime: Transition time in ms between transitions e.g. 50
bool ILEDSmartxxRGB::SetRGBState(CStdString strRGB1, CStdString strRGB2, CStdString strTransition, int iTranTime)
{
  // we have a new request: start reset
  strCurrentStatus = "NULL";
	strLastStatus = "NULL";
  strLastTransition = "NULL";
  s_RGBs.strTransition = "NULL";
	s_CurRGB.red = 0;
	s_CurRGB.green = 0;
	s_CurRGB.blue = 0;
	iSleepTime = 0;
  dwFrameTime = 0;
  dwLastTime = timeGetTime();
  // end reset

  getRGBValues(strRGB1,strRGB2,&s_RGBs);
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