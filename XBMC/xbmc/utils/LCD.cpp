#include "lcd.h"
#include "../GUISettings.h"

void ILCD::StringToLCDCharSet(CStdString& strText)
{

	//0 = HD44780, 1=KS0073
  unsigned int iLCDContr = g_guiSettings.GetInt("LCD.Type") == LCD_TYPE_LCD_KS0073 ? 1 : 0;
		//the timeline is using blocks
		//a block is used at address 0xA0, smallBlocks at address 0xAC-0xAF

		unsigned char LCD[2][256] = {
			{//HD44780
			0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
			0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f,
			0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f,
			0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f,
			0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f,
			0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5a, 0x5b, 0xa4, 0x5d, 0x5e, 0x5f,
			0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6a, 0x6b, 0x6c, 0x6d, 0x6e, 0x6f,
			0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7a, 0x7b, 0x7c, 0x7d, 0xb0, 0x20,
			0x20, 0x20, 0x2c, 0x20, 0x22, 0x20, 0x20, 0x20, 0x5e, 0x20, 0x53, 0x3c, 0x20, 0x20, 0x5a, 0x20,
			0x20, 0x27, 0x27, 0x22, 0x22, 0xa5, 0xb0, 0xb0, 0xb0, 0x20, 0x73, 0x3e, 0x20, 0x20, 0x7a, 0x59,
			0xff, 0x21, 0x20, 0x20, 0x20, 0x5c, 0x7c, 0x20, 0x22, 0x20, 0x20, 0x3c, 0x0E, 0x0A, 0x09, 0x08,
			0xdf, 0x20, 0x20, 0x20, 0x27, 0xe4, 0x20, 0xa5, 0x20, 0x20, 0xdf, 0x3e, 0x20, 0x20, 0x20, 0x3f,
			0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x20, 0x43, 0x45, 0x45, 0x45, 0x45, 0x49, 0x49, 0x49, 0x49,
			0x44, 0x4e, 0x4f, 0x4f, 0x4f, 0x4f, 0x4f, 0x78, 0x30, 0x55, 0x55, 0x55, 0x55, 0x59, 0x20, 0xe2,
			0x61, 0x61, 0x61, 0x61, 0xe1, 0x61, 0x20, 0x63, 0x65, 0x65, 0x65, 0x65, 0x69, 0x69, 0x69, 0x69,
			0x6f, 0x6e, 0x6f, 0x6f, 0x6f, 0x6f, 0x6f, 0xfd, 0x6f, 0x75, 0x75, 0xfb, 0xf5, 0x79, 0x20, 0x79
			},

			{//KS0073
			0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
			0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f,
			0x20, 0x21, 0x22, 0x23, 0xa2, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f,
			0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f,
			0xa0, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f,
			0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5a, 0xfa, 0xfb, 0xfc, 0x1d, 0xc4,
			0x27, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6a, 0x6b, 0x6c, 0x6d, 0x6e, 0x6f,
			0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7a, 0xfd, 0xfe, 0xff, 0xce, 0x20,
			0x20, 0x20, 0x2c, 0xd5, 0x22, 0x20, 0x20, 0x20, 0x1d, 0x20, 0xf3, 0x3c, 0x20, 0x20, 0xf4, 0x20,
			0x20, 0x27, 0x27, 0x22, 0x22, 0xdd, 0x2d, 0x2d, 0xce, 0x20, 0xf8, 0x3e, 0x20, 0x20, 0xf9, 0x59,
			0xd0, 0x40, 0xb1, 0xa1, 0x24, 0xa3, 0xfe, 0x5f, 0x20, 0x20, 0x20, 0x14, 0xd1, 0xd2, 0xd3, 0xd4,
			0x80, 0x8c, 0x82, 0x83, 0x27, 0x8f, 0x20, 0xdd, 0x20, 0x81, 0x80, 0x15, 0x8b, 0x8a, 0xbe, 0x60,
			0xae, 0xe2, 0xae, 0xae, 0x5b, 0xae, 0xbc, 0xa9, 0xc5, 0xbf, 0xc6, 0x45, 0xcc, 0xe3, 0xcc, 0x49,
			0x44, 0x5d, 0x4f, 0xe0, 0xec, 0x4f, 0x5c, 0x78, 0xab, 0xee, 0xe5, 0xee, 0x5e, 0xe6, 0x20, 0xbe,
			0x7f, 0xe7, 0xaf, 0xaf, 0x7b, 0xaf, 0xbd, 0xc8, 0xa4, 0xa5, 0xc7, 0x65, 0xa7, 0xe8, 0x69, 0x69,
			0x6f, 0x7d, 0xa8, 0xe9, 0xed, 0x6f, 0x7c, 0x25, 0xac, 0xa6, 0xea, 0xef, 0x7e, 0xeb, 0x70, 0x79
			}
		};

	unsigned char cLCD;
	int iSize = (int)strText.size();
	
	for (int i=0; i < iSize; ++i)
	{
		cLCD = strText.at(i);
		cLCD = LCD[iLCDContr][cLCD];
		strText.SetAt(i, cLCD);
	}
}

    /* TEMPLATE: TRANSLATION-TABLE FOR FUTURE LCD-TYPE-CHARSETS
			{
			0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
			0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f,
			0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f,
			0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f,
			0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f,
			0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5a, 0x5b, 0x5c, 0x5d, 0x5e, 0x5f,
			0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6a, 0x6b, 0x6c, 0x6d, 0x6e, 0x6f,
			0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7a, 0x7b, 0x7c, 0x7d, 0x7e, 0x7f,
			0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8a, 0x8b, 0x8c, 0x8d, 0x8e, 0x8f,
			0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9a, 0x9b, 0x9c, 0x9d, 0x9e, 0x9f,
			0xa0, 0xa1, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7, 0xa8, 0xa9, 0xaa, 0xab, 0xac, 0xad, 0xae, 0xaf,
			0xb0, 0xb1, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6, 0xb7, 0xb8, 0xb9, 0xba, 0xbb, 0xbc, 0xbd, 0xbe, 0xbf,
			0xc0, 0xc1, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xcb, 0xcc, 0xcd, 0xce, 0xcf,
			0xd0, 0xd1, 0xd2, 0xd3, 0xd4, 0xd5, 0xd6, 0xd7, 0xd8, 0xd9, 0xda, 0xdb, 0xdc, 0xdd, 0xde, 0xdf,
			0xe0, 0xe1, 0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9, 0xea, 0xeb, 0xec, 0xed, 0xee, 0xef,
			0xf0, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8, 0xf9, 0xfa, 0xfb, 0xfc, 0xfd, 0xfe, 0xff
			}
	*/

CStdString ILCD::GetProgressBar(double tCurrent, double tTotal)
{	
	CStdString strProgressBar;
	double tmpTest, dBlockSize, dBlockSizeRest;
	unsigned char cLCDsmallBlocks = 0xb0;	//this char (0xAC-0xAF) will be translated in LCD.cpp to the smallBlock
	unsigned char cLCDbigBlock = 0xa0;		//this char will be translated in LCD.cpp to the right bigBlock
	int iBigBlock = 5;						// a big block is a combination of 5 small blocks
	int m_iColumns = g_guiSettings.GetInt("LCD.Columns") - 2;

	if (m_iColumns>0)
	{	
		dBlockSize = tTotal*0.99/m_iColumns/iBigBlock;	// mult with 0.99 to show the last bar
		dBlockSizeRest = (tCurrent-((int)(tCurrent/dBlockSize)*dBlockSize));

		strProgressBar = "[";
		for (int i=1;i<=m_iColumns;i++)
		{
			//set full blocks
			if (tCurrent >= i * iBigBlock * dBlockSize)
			{
				strProgressBar += char(cLCDbigBlock);
			}
			//set a part of a block at the end, when needed
			else if (tCurrent > (i-1) * iBigBlock * dBlockSize + dBlockSize)
			{
				tmpTest= tCurrent - ((int)(tCurrent/(iBigBlock * dBlockSize)) * iBigBlock * dBlockSize );
				tmpTest=(tmpTest/dBlockSize);
				if (tmpTest>=iBigBlock) tmpTest=iBigBlock-1;
				strProgressBar += char(cLCDsmallBlocks-(int)tmpTest);
				dBlockSizeRest=0;
			}
			//fill up the rest with blanks
			else 
			{
				strProgressBar += " ";
			}
		}
		strProgressBar += "]";
		return strProgressBar;
	}
	else return "";
}
