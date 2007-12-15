


//=============================
//		Gui.cpp
//=============================

#include "stdafx.h" 

#include "FileSpecs.h"
#include "SystemX.h"
#include "Gui.h"
#include "Helper.h"
//#include "Main.h"
#include "Strings.h"
//#include "LoadGraphics.cpp"
//#include "MediaPlayer.h"


#define FORM Form1

//------------------------------------------------------------------------------

Gui::Gui(){

//AppDir = ExtractFileDir(Application->ExeName);
AppDir = ExtractFileDir(__argv[0]);

}

//------------------------------------------------------------------------------

Gui::~Gui(){


}

//------------------------------------------------------------------------------

void Gui::Msg (const string& msg){
MessageBox(NULL, msg.c_str(), NULL, 0);
//ShowMessage(msg);

}
void Gui::MsgA (const string& msg, const char* caption){
// UnCommentMeSoon
MessageBoxA(NULL, msg.c_str(), NULL, 0);
//Application->MessageBoxA(msg, caption, NULL);
}
void Gui::MsgInfo (const string& msg){
MessageBox(NULL, msg.c_str(), NULL, MB_ICONINFORMATION);
//MessageDlg (msg, mtInformation, TMsgDlgButtons() << mbOK, 0);
}
bool Gui::MsgWarning (const string& msg){

if (MB_OK == MessageBox(NULL, msg.c_str(), NULL, MB_OKCANCEL | MB_ICONWARNING)) 
//if (MessageDlg (msg, mtWarning, TMsgDlgButtons() << mbOK << mbCancel, 0) == mrOk)
	return true;
return false;
}
void Gui::MsgError (const string& msg){

MessageBox(NULL, msg.c_str(), NULL, MB_OK|MB_ICONERROR);
//MessageDlg (msg, mtError, TMsgDlgButtons() << mbOK, 0);
}


//------------------------------------------------------------------------------

void Gui::FinishDisplay(int& extracted, int& filesTotal,
			const double& time, string statusCaption, string progressCaption){

// UnCommentMeSoon
//FORM->EkImgProgressBar->Text = progressCaption;

if (time > 60)
{
	int min = (int)time / 60;
	int sec = (int)time % 60;
	//Status(statusCaption + " " + string (extracted) + " File/s in "
	//+ string (min) + " Min/s " + string (sec) + " Sec/s", 0, false);
}
else
{
	//Status(statusCaption + " " + string (extracted)
	//+ " File/s in " + string (time) + " Sec/s", 0, false);
}

if (filesTotal != extracted)
{
	//MsgError(string(filesTotal - extracted)
	//+ " file/s not extracted due to error/s");
}

//ProgressMaxOut();

Finish();
extracted = 0,
filesTotal = 0;


}// End FinishDisplay

//------------------------------------------------------------------------------
void Gui::SpeedHackEnable(const bool& value){

if (SpeedOn != value){
	SpeedOn = value;
	//FORM->TimrRefreshRate->Enabled = value;
}

}

//------------------------------------------------------------------------------

void Gui::NameCaption(const string& name){
//name = ExtractFileName(name);
//FORM->LblCaption->Text1 = APPNAME;
//FORM->LblCaption->Text2 = " - " + name;
//name = ExtractFileName(name);
//FORM->LblCaption->Text1 = name;
//FORM->LblCaption->Text2 = " - " + string(APPNAME);
}

//------------------------------------------------------------------------------

void Gui::Caption(const string& txt){

//FORM->LblCaption->Text1 = APPNAME;
//if (txt.Length())
//	FORM->LblCaption->Text2 = " - " +  txt;
//else
//	FORM->LblCaption->Text2 = "";
//
//FORM->LblCaption->Text1 = txt;
//if (!txt.Length())
//	FORM->LblCaption->Text2 = APPNAME;
//else
//	FORM->LblCaption->Text2 = " - " + string(APPNAME);

}


//------------------------------------------------------------------------------

void Gui::Status (const string& caption, const int& sleep){

//if (MediaPlayer.GetIsPlaying()) return;
	//gui.ProgressSetScroll(false);

//FORM->EkImgProgressBar->Text = caption;

//if (refresh)
//FORM->EkImgProgressBar->Update();

//if (MediaPlayer.GetIsPlaying()){
//	Sleep(300);
//	MediaPlayer.SetIsScrolling(false);
//}
cout<<"Status: "<<caption<<endl;
Sleep((u_long)sleep);

}

//------------------------------------------------------------------------------

void Gui::Error (const string& caption, const int& sleep, FILE* stream){
if (stream)
	fclose(stream);
cout<<"Error: "<<caption<<endl;
Sleep((u_long)sleep);
CItem.Error = true;
}

//------------------------------------------------------------------------------

void Gui::Finish(){
CItem.Extracting = false;
//FORM->EkImgProgressBar->Position = 100;
//ExtractDlg->Caption = "100%";
}

//------------------------------------------------------------------------------

Gui gui; // Global instance


