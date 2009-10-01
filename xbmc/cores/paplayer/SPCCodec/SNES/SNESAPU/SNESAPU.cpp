/***************************************************************************************************
* SNES(tm) APU Emulator DLL                                                                        *
*                                                      Copyright (C)2001-2006 Alpha-II Productions *
***************************************************************************************************/

#define	DONT_IMPORT_SNESAPU

#include	"Types.h"
#include	"A2Misc.h"
#include	"SNESAPU.h"

using namespace A2::SNES;


//**************************************************************************************************
// Variables

static u32	ApuOpt;


////////////////////////////////////////////////////////////////////////////////////////////////////
// Functions

//**************************************************************************************************
extern "C" b8 _DllMainCRTStartup(u32 hinst, u32 dwReason, u32 lpReserved)
{
	switch (dwReason)
	{
	case 1:	//DLL_PROCESS_ATTACH
		ApuOpt = InitAPU();
		ResetAPU();
		break;

	case 0:	//DLL_PROCESS_DETACH
		ShutAPU();
		break;
	}

	return 1;
}


//**************************************************************************************************
void* GetAPUData(DataType dt)
{
	switch(dt)
	{
	case(DATA_OPTIONS):	return reinterpret_cast<void*>(ApuOpt);
	case(DATA_RAM):		return static_cast<void*>(pAPURAM);
	case(DATA_DSP):		return static_cast<void*>(&dsp);
	case(DATA_MIX):		return static_cast<void*>(&mix);
	case(DATA_PROFILE):	return static_cast<void*>((ApuOpt & SA_PROFILE) ? &profile : 0);
	default:			return (void*)0;
	}
}

