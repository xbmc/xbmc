/*
XBOX Front LED control
OFF/Green/Red/Orange/Cycle
10.11.2004 GeminiServer
*/

#include "LED.h"
#include "../xbox/XKUtils.h"
#include "../GUISettings.h"

void ILED::CLEDControl(int ixLED)
{   
	if (ixLED==LED_COLOUR_OFF)
		{
			XKUtils::SetXBOXLEDStatus(0);
		}
	else if (ixLED==LED_COLOUR_GREEN)
		{
			XKUtils::SetXBOXLEDStatus(XKUtils::LED_REGISTER_CYCLE0_GREEN|XKUtils::LED_REGISTER_CYCLE2_GREEN|XKUtils::LED_REGISTER_CYCLE1_GREEN|XKUtils::LED_REGISTER_CYCLE3_GREEN);
		}
	else if (ixLED==LED_COLOUR_RED)
		{
			XKUtils::SetXBOXLEDStatus(XKUtils::LED_REGISTER_CYCLE0_RED|XKUtils::LED_REGISTER_CYCLE2_RED|XKUtils::LED_REGISTER_CYCLE1_RED|XKUtils::LED_REGISTER_CYCLE3_RED);
		}
	else if (ixLED==LED_COLOUR_ORANGE)
		{
			XKUtils::SetXBOXLEDStatus(XKUtils::LED_REGISTER_CYCLE0_ORANGE|XKUtils::LED_REGISTER_CYCLE2_ORANGE|XKUtils::LED_REGISTER_CYCLE1_ORANGE|XKUtils::LED_REGISTER_CYCLE3_ORANGE);
		}
	else if (ixLED==LED_COLOUR_CYCLE)
		{
			XKUtils::SetXBOXLEDStatus(XKUtils::LED_REGISTER_CYCLE0_GREEN|XKUtils::LED_REGISTER_CYCLE2_GREEN|XKUtils::LED_REGISTER_CYCLE1_ORANGE|XKUtils::LED_REGISTER_CYCLE3_RED);
		}
	else if (ixLED==LED_COLOUR_NO_CHANGE) //Default Bios Settings
		{
			//No need to Change! Leave the LED with the Default Bios Settings
		}
}
