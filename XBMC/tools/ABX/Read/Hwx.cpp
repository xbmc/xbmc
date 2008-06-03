


//========================================================================
//	Hwx.cpp - Star Wars Episode III: Revenge Of The Sith - (xadpcm files)
//========================================================================


#include "stdafx.h"
#include "../Supported.h"
#include "../FileSpecs.h"
//#include "../Gui.h"
#include "../SystemX.h"
#include "../Helper.h"

//------------------------------------------------------------------------------

void Supported::HwxRead(FILE* in){

sys.PushFiles(1);
sys.PushAudioFormat("xadpcm");
sys.PushBits(4);
sys.PushChannel(2);
sys.PushSampleRate(44100);
sys.PushOffset(0);
sys.PushChunk(CItem.FileSize);
sys.PushFileName(CItem.DirName);
sys.PushExtension(".wav");

}



