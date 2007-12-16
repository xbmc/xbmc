// ABX.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "Open.h"
#include "Supported.h"
#include "Helper.h"
#include "Gui.h"

//int main(int argc, char *argv[])
int _tmain(int argc, _TCHAR* argv[])
{
	Open op;
	bool rtn = false;
	std::string s;

	if (argc > 1 && argv[1] != "")
		s = argv[1];

	gui.Status("Opening", 0);

	if (s != "")
		rtn = op.DoOpen(s);
	else{
		const string path = "c:\\XboxFormats\\";
		//rtn = op.DoOpen(path + "EMOTION.ADF");
		//rtn = op.DoOpen(path + "adx0.afs");
		//rtn = op.DoOpen(path + "sounds.bif");
		//rtn = op.DoOpen(path + "FEMusic2.big");
		//rtn = op.DoOpen(path + "music.big");
		//rtn = op.DoOpen(path + "css1.dat");
		//rtn = op.DoOpen(path + "streams_music_common_audio.fsb");
		//rtn = op.DoOpen(path + "sounds.hot");
		//rtn = op.DoOpen(path + "mus_ui_mainmenu_lp.hwx");
		//rtn = op.DoOpen(path + "River.lug");
		//rtn = op.DoOpen(path + "music.mpk");
		//rtn = op.DoOpen(path + "shell_music.msx");
		//rtn = op.DoOpen(path + "AudXbox0.pak");
		//rtn = op.DoOpen(path + "Font36.piz");
		//rtn = op.DoOpen(path + "XBXMUSND.POD");
		//rtn = op.DoOpen(path + "COMMON.POD");
		//rtn = op.DoOpen(path + "music.rcf");
		//rtn = op.DoOpen(path + "Resource.rez");
		//rtn = op.DoOpen(path + "Audio.rfa"); !Not Working
		//rtn = op.DoOpen(path + "music (menu).rws");
		//rtn = op.DoOpen(path + "Namco50.samp");
		rtn = op.DoOpen(path + "wma.wad");
		//rtn = op.DoOpen("c:\\Forza\\UIMusic.wmapack");
	}

	if (rtn == true){
		gui.Status("Opened", 0);
		Supported sup; sup.StdExtract(-1);
		//Supported sup; sup.StdExtract(1);
	}

	return 0;
}

