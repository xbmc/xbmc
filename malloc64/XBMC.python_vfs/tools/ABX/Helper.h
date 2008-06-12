#ifndef HELPER_H
#define HELPER_H

#include "stdafx.h" 
//#include <string>

#define XORBUFC(x, buf, xorVal) while (x) buf[--x] ^= xorVal
#define BSWAPC(x) x = (x>>24) | ((x<<8) & 0x00FF0000) | ((x>>8) & 0x0000FF00) | (x<<24)

string IntToStr(const int& i);
string ToLower(const string& str);
string ToLower(const string& str);
string ReplaceAll(const string& str, const string& orig, const string& repl);
WCHAR* CharToWChar(const char* pValue);
void Msg(std::string msg);
void Msg(int msg);
bool DirectoryExists(const char* dirName);
string ChangeFileExt(const std::string& pName, const std::string& pExt);
string RenameFile(const std::string& pName, const std::string& pNewName);
string RemoveFileExt(const char* pValue);
string RemoveFileExt(const std::string& pValue);
string RemoveDrive(const std::string& pValue);
string ExtractFileExt(const std::string& pValue);
string ExtractFileName(const std::string& pValue);
string ExtractFileDir(const std::string& pValue);
string ExtractDirPath(const std::string& pValue);
void ProcessMessages();

#endif