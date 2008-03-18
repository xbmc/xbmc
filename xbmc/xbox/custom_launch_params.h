#ifndef CUSTOM_LAUNCH_PARAMS_H
#define CUSTOM_LAUNCH_PARAMS_H

#include <XBApp.h>

#define CUSTOM_LAUNCH_MAGIC 0xEE456777

#define COUNTRY_TYPE_AUTODETECT 0
#define COUNTRY_TYPE_USA 1
#define COUNTRY_TYPE_JAPAN 2
#define COUNTRY_TYPE_EUROPE 3
#define COUNTRY_TYPE_OTHER 4

typedef struct _CUSTOM_LAUNCH_DATA
{
	DWORD magic ;							//populate this with CUSTOM_LAUNCH_MAGIC so we know we are using this special structure
	char szFilename[300] ;					//this is the path to the game to load upon startup
	char szLaunchXBEOnExit[100] ;			//this is the XBE name that should be launched when exiting the emu  ( "FILE.XBE" )
	char szRemap_D_As[350] ;				//this is what D drive should be mapped to in order to launch the XBE specified in szLaunchXBEOnExit  ( "\\Device\\Harddisk0\\Partition1\\GAMES" )
	BYTE country ;							//country code to use
	BYTE launchInsertedMedia ;				//should we auto-run the inserted CD/DVD ?
	BYTE executionType ;					//generic variable that determines how the emulator is run - for example, if you wish to run FMSXBOX as MSX1 or MSX2 or MSX2+
	char reserved[MAX_LAUNCH_DATA_SIZE-757] ;	//MAX_LAUNCH_DATA_SIZE is 3KB 

} CUSTOM_LAUNCH_DATA, *PCUSTOM_LAUNCH_DATA;

int XGetCustomLaunchData() ;

#endif
