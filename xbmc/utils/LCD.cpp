#include "lcd.h"

void ILCD::StringToLCDCharSet(CStdString& strText)
{
	unsigned char cTest;
	char cLCD;
	int iSize = (int)strText.size();

	for (int i=0; i < iSize; ++i)
	{
		// display is not supporting all asci characters.
		cTest = strText.at(i);
		if (cTest >= 127)
		{
			if (cTest == 228) cLCD = char(225);      // fix for ä
			else if (cTest == 246) cLCD = char(239); // fix for ö
			else if (cTest == 252) cLCD = char(245); // fix for ü

			else if (cTest >= 192 && cTest <= 197) cLCD = char(65);  // display as A
			else if (cTest >= 200 && cTest <= 203) cLCD = char(69);  // display as E
			else if (cTest >= 204 && cTest <= 207) cLCD = char(73);  // display as I
			else if (cTest >= 210 && cTest <= 214) cLCD = char(79);  // display as O
			else if (cTest >= 217 && cTest <= 220) cLCD = char(85);  // display as U

			else if (cTest >= 224 && cTest <= 229) cLCD = char(97);  // display as a
			else if (cTest >= 232 && cTest <= 235) cLCD = char(101); // display as e
			else if (cTest >= 236 && cTest <= 239) cLCD = char(105); // display as i
			else if (cTest >= 242 && cTest <= 246) cLCD = char(111); // display as o
			else if (cTest >= 249 && cTest <= 252) cLCD = char(117); // display as u
			else cLCD = ' ';

			strText.SetAt(i, cLCD);
		}
	}
}

