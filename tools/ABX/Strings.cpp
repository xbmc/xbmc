

//=============================
//		Strings.cpp
//=============================


#include "stdafx.h"
//#include "Main.h"
#include "Helper.h"
#include "Strings.h"
#include "SystemX.h"
#include "FileSpecs.h"

Strings str; // Global instance


string Strings::ToLower(string str){

//#include <cctype> // for toupper
//#include <algorithm>
//std::transform(myString.begin(), myString.end(), myString.begin(), toupper);

string s = str;
for (int i = 0; i < (int)s.length(); ++i) 
	s[i] = tolower(s[i]); 

return s;

}

string Strings::ToUpper(string str){

string s = str;
for (int i = 0; i < (int)s.length(); ++i) 
	s[i] = toupper(s[i]); 

return s;

}


void Strings::RemoveExt(string& name){

int extLen = (int)string(ExtractFileExt(name)).length();
if (extLen)
	name.resize(name.length() - extLen-1);
}


string Strings::GetExt(string name){

// UnCommentMeSoon
//int dotPos = name.find_last_of(".");
//if (dotPos)
//	name.Delete(1, dotPos);
//
//return name;
return "";

}

string Strings::GetFileName(string name){

int slash = (int)name.find_last_of("\\");
if (!slash)
	slash = (int)name.find_last_of("/");

if (slash){
	// UnCommentMeSoon
	//string fn = name.Delete(1, slash);
	//RemoveExt(fn);
	//return fn;
}

return "";

}

bool Strings::MaxPathClip(string& path){

if (path.length() <= 265)
	return false;

path.resize(365);
return true;

}

string Strings::FormatTime(const float& time){

int mins = (int)time / 60;
int secs = (int)time % 60;
char zero1[] = "0";
char zero2[] = "0";

if (mins > 9)
	*zero1 = NULL;
if (secs > 9)
	*zero2 = NULL;

// UnCommentMeSoon
//return zero1 + string(mins) + ":" + zero2 + (string)secs;
return "";

}



bool Strings::IllegalCharRemover(string& txt){

// UnCommentMeSoon
//int x, ogLen = txt.length();
//// Remove all illegal characters /\*<>|":
//while ( (x = txt.find("\\")) || (x = txt.find("*")) || (x = txt.find("<"))
//		|| (x = txt.find(">")) || (x = txt.find("|")) || (x = txt.find("\""))
//		|| (x = txt.find(":")) )
//{
//	txt.c_str()[x-1] = '?';
//}
//while (x = txt.find("?"))
//	txt.Delete (x, 1);
//
//if (ogLen == txt.Length()) // No char were removed
//	return 0;
//
//return 1;
return true;

}

void Strings::StrClean(string& str){

// UnCommentMeSoon
//str.Trim();
//str = str.LowerCase();
//
//// Remove '-' & ' ' & '/' from start
//if (str.find("_") == 1) str.Delete (str.find("_"), 1);
//if (str.find("/") == 1) str.Delete (str.find("/"), 1);
//if (str.find(" ") == 1) str.Delete (str.find(" "), 1);
//
//u_short x;
//
//// Remove '_' after every '('
//while ( x = str.find("(_") )
//{
//	str.c_str()[x] = '?';
//}
//while (x = str.find("?"))
//	str.Delete (x, 1);
//
//
//// Remove '_' before every ')'
//while ( x = str.find("_)") )
//{
//	str.c_str()[x-1] = '?';
//}
//while (x = str.find("?"))
//	str.Delete (x, 1);
//
//
//// Change all '_' to ' '
//while (x = str.find("_"))
//	str.c_str()[x-1] = ' ';
//
//// Change all '/' to ' \\ '
//while (x = str.find("/"))
//{
//	str.Delete (x, 1);
//	str.Insert("\\", x);
//}
//
//// Remove all illegal characters /\*<>|":
//while ( (x = str.find("*")) || (x = str.find("<")) || (x = str.find(">"))
//	|| (x = str.find("|"))  || (x = str.find(":")) )
////while ( (x = str.Pos("\\")) || (x = str.Pos("*")) || (x = str.Pos("<"))
////		|| (x = str.Pos(">")) || (x = str.Pos("|")) || (x = str.Pos("\""))
////		|| (x = str.Pos(":")) )
//{
//	str.c_str()[x-1] = '?';
//}
//while (x = str.find("?"))
//	str.Delete (x, 1);
//
//// Remove all spaces > 1
//while (x = str.find("  "))
//	str.c_str()[x-1] = '?';
//while (x = str.find("?"))
//	str.Delete (x, 1);
//
//// Capitalize letters after every ' '
//while (x = str.find(" "))
//{
//	str.c_str()[x-1] = '?';
//	str.c_str()[x] = toupper(str.c_str()[x]);
//}
//while (x = str.find("?"))
//	str.c_str()[x-1] = ' ';
//
//// Capitalize letters after every ' \\ '
//while (x = str.Pos("\\"))
//{
//	str.c_str()[x-1] = '?';
//	str.c_str()[x] = toupper(str.c_str()[x]);
//}
//while (x = str.find("?"))
//	str.c_str()[x-1] = '\\';
//
//// Capitalize letters after every '('
//while (x = str.find("("))
//{
//	str.c_str()[x-1] = '?';
//	str.c_str()[x] = toupper(str.c_str()[x]);
//}
//while (x = str.find("?"))
//	str.c_str()[x-1] = '(';
//
//// Capitalize letters after every ' ' '
//while (x = str.find("'"))
//{
//	str.c_str()[x-1] = '?';
//	str.c_str()[x] = toupper(str.c_str()[x]);
//}
//while (x = str.find("?"))
//	str.c_str()[x-1] = '\'';
//
//
//
//
//
//
//// Check again to make sure that no space exists at the start
//if (str.find(" ") == 1) str.Delete (str.find(" "), 1);
//
//// InitCaps
//str.c_str()[0] = toupper( str.c_str()[0]);

}// End CleanName



string Strings::stringReplacer(string text, const int& x,
											const int& maxNum){
// UnCommentMeSoon
//string fileName =
//FItem.FileNameBak[x];//.SetLength(ExtractFileExt(FItem.FileName[x]).Length());
//
//string NewText =
//stringReplace(text, "%f", fileName, TReplaceFlags () << rfReplaceAll);
//
//NewText =
//stringReplace(NewText, "%a", CItem.DirName, TReplaceFlags () << rfReplaceAll);
//
//return
//stringReplace(NewText, "%n", AutoNumBasic(x, maxNum),
//TReplaceFlags () << rfReplaceAll);
return "";

}



string Strings::AutoNumBasic (const int& num, const int& maxNum){

int prefixLen = 0;

if (maxNum > 9999)
	prefixLen = 4;
else if (maxNum > 999)
	prefixLen = 3;
else if (maxNum > 99)
	prefixLen = 2;
else if (maxNum > 9)
	prefixLen = 1;
else if (maxNum >= 1)
	prefixLen = 0;

string numS = "0000";

// UnCommentMeSoon
if (num+1 < 10){
	numS.resize(prefixLen);
	numS += IntToStr(num+1);
}
else if (num+1 < 100){
	numS.resize(prefixLen-1);
	numS += IntToStr(num+1);
}
else if (num+1 < 1000){
	numS.resize(prefixLen-2);
	numS += IntToStr(num+1);
}
else if (num+1 < 10000){
	numS.resize(prefixLen-3);
	numS += IntToStr(num+1);
}
else if (num+1 < 100000){
	numS.resize(prefixLen-4);
	numS += IntToStr(num+1);
}

return numS;
}


void Strings::AutoNum (string prefix, string& name, const int& num,
									string extension){

// UnCommentMeSoon
//if (num < 10)
//name = prefix + " - 0000" + string(num) + " " + extension;
//else if (num < 100)
//name = prefix + " - 000" + string(num) + " " + extension;
//else if (num < 1000)
//name = prefix + " - 00" + string(num) + " " + extension;
//else if (num < 10000)
//name = prefix + " - 0" + string(num) + " " + extension;
//else
//name = prefix + " - " + string(num) + " " + extension;
}



void Strings::AutoNumList (const bool& tidy, string& name,
									const int& num, const int& maxNum){

string prefix = ExtractFileName(CItem.OpenFileName);
prefix.resize(prefix.length() - sys.GetOpenFileNameExtLen()-1);
//if (Form1->ChkTidyName->Checked && !Form1->dbaseChk->Checked)
if (tidy) StrClean(prefix);

if (num+1 < 10)
name = prefix + " - " + AutoNumBasic(num, maxNum);
else if (num+1 < 100)
name = prefix + " - " + AutoNumBasic(num, maxNum);
else if (num+1 < 1000)
name = prefix + " - " + AutoNumBasic(num, maxNum);
else if (num+1 < 10000)
name = prefix + " - " + AutoNumBasic(num, maxNum);
else
name = prefix + " - " + AutoNumBasic(num, maxNum);

//if (num+1 < 10)
//name = prefix + " - " + AutoNumBasic(num, maxNum) + FItem.Ext[num];
//else if (num+1 < 100)
//name = prefix + " - " + AutoNumBasic(num, maxNum) + FItem.Ext[num];
//else if (num+1 < 1000)
//name = prefix + " - " + AutoNumBasic(num, maxNum) + FItem.Ext[num];
//else if (num+1 < 10000)
//name = prefix + " - " + AutoNumBasic(num, maxNum) + FItem.Ext[num];
//else
//name = prefix + " - " + AutoNumBasic(num, maxNum) + FItem.Ext[num];

}



void Strings::AutoNumScx (const bool& tidy, string& name,
									const int& num, const int& maxNum){

string prefix = ExtractFileName(CItem.OpenFileName);
prefix.resize(prefix.length() - sys.GetOpenFileNameExtLen());

//string text = Form1->EditRenFiles->Text;

if (tidy)
	StrClean(prefix);

//if (tidy && text != ""){
//	StrClean(text);
//	prefix = "";
//}

if (num+1 < 10)
name = prefix + " - " + AutoNumBasic(num, maxNum) + FItem.Ext[num];
else if (num+1 < 100)
name =   prefix + " - " + AutoNumBasic(num, maxNum) + FItem.Ext[num];
else if (num+1 < 1000)
name =  prefix + " - " + AutoNumBasic(num, maxNum) + FItem.Ext[num];
else if (num+1 < 10000)
name =   prefix + " - " + AutoNumBasic(num, maxNum) + FItem.Ext[num];
else
name =  prefix + " - " + AutoNumBasic(num, maxNum) + FItem.Ext[num];
}





