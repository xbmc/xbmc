
#include "stdafx.h" 
#include "Gui.h" 
#include <sstream>

//====================================================================================

string IntToStr(const int& i){

std::ostringstream o;
if (!(o << i))
     return "";
   return o.str();
}

//====================================================================================

string ToLower(const string& str){

string s = str;
for (int i = 0; i < (int)s.length(); ++i) 
	s[i] = tolower(s[i]); 

return s;

}

//====================================================================================

string ToUpper(const string& str) {

string s = str;
for (int i = 0; i < (int)s.length(); ++i) 
	s[i] = toupper(s[i]); 

return s;

}

//====================================================================================

string ReplaceAll(const string& str, const string& orig, const string& repl){

if ((int)str.find(orig) == -1 || !str.length() || !repl.length()) 
	return str;

string s=str;
int pos;

for (int n; n = (int)s.find(orig) != -1;)
{
	string a, b;
	a = b = s;
	a.resize(n);
	b = b.substr(n+1);
	s = a + repl + b;
	gui.Status(s, 0);
}

//while (pos = (int)s.find(orig) != -1) 
//{
//	s.replace(pos, repl.length(), repl);
//	gui.Status(s, 0);
//}

gui.Status(s, 0);
return s;

}

//====================================================================================

WCHAR* CharToWChar(const char* pValue) {

int	length;		
WCHAR*	wfilename;	
length = (int)strlen(pValue)+1;
wfilename = new WCHAR[length];
MultiByteToWideChar(CP_ACP, 0, pValue, -1, wfilename, length);
return wfilename;

}


//====================================================================================

void Msg(std::string msg){

MessageBox(NULL, msg.c_str(), NULL, MB_OK); 

}

//====================================================================================

void Msg(int msg){

string s;
std::ostringstream o;
if (!(o << msg))
	s = o.str();
else
	return;

MessageBox(NULL, s.c_str(), NULL, MB_OK); 

}

//====================================================================================

bool DirectoryExists(const char* dirName){

//u_long attr = GetFileAttributes(dirName.c_str()); // windows specific
//
//if (attr == 0xFFFFFFFF) 
//{ 
//    u_long error = GetLastError(); 
//    if (error == ERROR_FILE_NOT_FOUND) 
//		return false; // file not found     
//    else if (error == ERROR_PATH_NOT_FOUND) 
//		return false; // path not found 
//    else if (error == ERROR_ACCESS_DENIED) 
//		return false; // file or directory exists, but access is denied 
//    else 
//		return false; // some other error has occured 
//} 
//
//
//if (attr & FILE_ATTRIBUTE_DIRECTORY) 
//    return true;	// this is a directory 
//else 
//    return false;	// this is a directory 

return true;

}

//====================================================================================

//bool CreateDirectory(const char* dirName, void* attrib){
//
//return true;
//}

//====================================================================================

string ChangeFileExt(const std::string& pName, const std::string& pExt) {

if (!pName.length() || !pExt.length()) return pName;

string n = pName;
string e = pExt;

int pos = (int)n.find_last_of('.');
if (!pos) 
	return pName;
else
{ 
	int extLen = (int)n.length() - pos;
	n.replace(pos, pos + pExt.length(), pExt);
	return n;
}

};

//====================================================================================

string RenameFile(const std::string& pName, const std::string& pNewName) {

// To be coded
return "";

};

//====================================================================================

string RemoveFileExt(const std::string& pValue) {

if (!pValue.length()) return "";

string str = pValue;

int pos = (int)str.find('.');
if (!pos) 
	return "";
else
{ 
	int extLen = (int)str.length() - pos;
	str.resize(str.length() - extLen);
	return str;
}

}

//====================================================================================
string RemoveDrive(const std::string& pValue){

string s = pValue;
int pos = (int)s.find("\\");
if (pos != -1)
	return s.substr(pos+1);
else
	return pValue;

}

//====================================================================================

string RemoveFileExt(const char* pValue) {

if (pValue == "") return "";

string str = pValue;

int pos = (int)str.find('.');
if (pos==-1) 
	return str;
else
{ 
	int extLen = (int)str.length() - pos;
	str.resize(str.length() - extLen);
	return str;
}

}

//====================================================================================

string ExtractFileExt(const std::string& pValue) {

if (!pValue.length() || pValue.find('.') == -1) return "";

string str = pValue;

int pos = (int)str.find('.');
if (!pos) 
	return "";
else 
	return str.substr(pos+1);

}

//====================================================================================

string ExtractFileExt(const char* pValue) {

if (!pValue) return "";

string str = pValue;

int pos = (int)str.find('.');
if (!pos) 
	return "";
else 
	return str.substr(pos+1);

}

//====================================================================================

void ProcessMessages()
{
  MSG msg;
  while (PeekMessage (&msg, 0, 0, 0, PM_REMOVE))
  {
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }
} 

//====================================================================================

string ExtractFileName(const std::string& pValue) {

if (pValue == "") return "";

std::string str = pValue;

int pos = (int)str.find_last_of("\\");
//Msg(pos);
if (!pos) 
	return str;
else 
	return str.substr(pos+1);

}

//====================================================================================

std::string ExtractFileDir(const std::string& pValue) {

if (pValue == "") return "";

std::string str = pValue;

int pos = (int)str.find_last_of("\\");

if (!pos) 
	return str;
else 
	str.resize(pos);

pos = (int)str.find_last_of("\\");

if (!pos) 
	return str;
else 
	return str.substr(pos+1);


}

//====================================================================================

std::string ExtractDirPath(const std::string& pValue) {

if (pValue == "") return "";

std::string str = pValue;

int pos = (int)str.find_last_of("\\");

if (!pos) 
	return str;
else 
	str.resize(pos);


return str;

}

//====================================================================================




