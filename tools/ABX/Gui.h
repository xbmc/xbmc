#ifndef GUI_H
#define GUI_H

//=============================
//		Gui.h
//=============================

#include "stdafx.h" 
#include "FileSpecs.h"

class Gui{

private:
	bool SpeedOn;
	string AppDir;

public:

	Gui();
	~Gui();
	
	void Msg(const string& msg);
	void MsgA(const string& msg, const char* caption);
	void MsgInfo(const string& msg);
	bool MsgWarning(const string& msg);
	void MsgError(const string& msg);

	void FinishDisplay(int& extracted, int& filesTotal,
		const double& time, string statusCaption, string progressCaption);

	bool SpeedHackIsOn() {return SpeedOn;}
	void SpeedHackEnable(const bool& value);

	void NameCaption(const string& name);
	void Caption(const string& txt);
	void Status (const string& caption, const int& sleep);
	void Error (const string& caption, const int& sleep, FILE* stream);
	void Finish();
	bool IsExtracting() {return CItem.Extracting;}
	

};// End Gui

extern Gui gui; // Global instance

#endif
