//  RegKey.cpp:  implementation of RegKey class
//
//////////////////////////////////////////////////////////////////////

#ifdef _DEBUG
#define _CRTDBG_MAP_ALLOC
#endif //_DEBUG

#include <DshowUtil/RegKey.h>
#include <sstream>
#include <memory>
#ifdef _DEBUG
#include <iostream>
#endif

#ifdef _DEBUG
// Include CRTDBG.H after all other headers
#include <stdlib.h>
#include <crtdbg.h>
#define NEW_INLINE_WORKAROUND new ( _NORMAL_BLOCK ,\
                                    __FILE__ , __LINE__ )
#define new NEW_INLINE_WORKAROUND
#endif

using namespace std;

RegKey::RegKey(HKEY base,const string& pathname,bool bCreate)throw(SysError)
{
  HKEY hKey;
  int n;
  if(bCreate)
  {
    DWORD dwDummy;
    n=RegCreateKeyEx(base,pathname.c_str(),0,"zello",REG_OPTION_NON_VOLATILE,KEY_ALL_ACCESS,NULL,&hKey,&dwDummy);
  }
  else
  {
    n=RegOpenKeyEx(base,pathname.c_str(),0,KEY_ALL_ACCESS,&hKey);
  }
  if(n!=ERROR_SUCCESS)
    throw SysError(n);
  m_hkey=new HKEYWrapper(hKey);
  size_t lastslash=pathname.rfind('\\');
  if(lastslash!=string::npos)
  {
    m_name=pathname.substr(lastslash+1);
    n=RegOpenKeyEx(base,pathname.substr(0,lastslash).c_str(),0,KEY_SET_VALUE,&hKey);
    if(n!=ERROR_SUCCESS)
      throw SysError(n);
    m_hkeyfather=new HKEYWrapper(hKey);
  }
  else
  {
    m_name=pathname;
    m_hkeyfather=new HKEYWrapper(base,false);
  }

}

std::list<string> RegKey::getSubkeys()const throw(SysError)
{
  int n;
  int c=0;
  list<string> lst;
  FILETIME ft;
  do
  {
    ULONG size=MAX_PATH;
    char buf[MAX_PATH];
    HKEY hk=m_hkey.ptr()->get();
    n=RegEnumKeyEx(m_hkey.ptr()->get(),c++,buf,&size,NULL,NULL,NULL,&ft);
    switch(n)
    {
    case ERROR_SUCCESS:
      {
        lst.push_back(buf);
        break;
      }
    case ERROR_NO_MORE_ITEMS:
      break;
    default:
      throw SysError(n);
    }
  }
  while(n!=ERROR_NO_MORE_ITEMS);
  return lst;
}

void RegKey::erase()throw(SysError)
{
  list<string> subs=getSubkeys();
  for(list<string>::iterator it=subs.begin();it!=subs.end();++it)
  {
    RegKey(m_hkey.ptr()->get(),*it,false).erase();
  }
  int n=RegDeleteKey(m_hkeyfather.ptr()->get(),m_name.c_str());
  if(n!=ERROR_SUCCESS)
    throw SysError(n);
}

size_t RegKey::readRawData(const string& valueName,BYTE* buf,DWORD bufsize)const throw(SysError)
{
  DWORD dwDummy;
  int n=RegQueryValueEx(m_hkey.ptr()->get(),
              (valueName==""?NULL:valueName.c_str()),
              NULL,
              &dwDummy,
              buf,
              &bufsize);
  if(n!=ERROR_SUCCESS)
    throw SysError(n);
  return bufsize;
}
std::string RegKey::getValue(const string& valueName)const throw(SysError)
{
  size_t size=readRawData(valueName,NULL,0);
  if(size)
  {
    std::auto_ptr<char> buf=std::auto_ptr<char>(new char[size+1]);
    readRawData(valueName,(BYTE*)(buf.get()),size);
    return (buf.get());
  }
  else 
    return "";
}

DWORD RegKey::getDwordValue(const string& valueName)const throw(SysError)
{
  DWORD dwRet;
  readRawData(valueName,(BYTE*)&dwRet,sizeof(DWORD));
  return dwRet;
}

RECT RegKey::getRectValue(const string& valueName)const throw(SysError)
{
  RECT rectRet;
  readRawData(valueName,(BYTE*)&rectRet,sizeof(RECT));
  return rectRet;
}

BYTE* RegKey::getBinaryValue(const string& valueName,BYTE* buf,DWORD dwBufSize)const throw(SysError)
{
  readRawData(valueName,buf,dwBufSize);
  return buf;
}

DWORD RegKey::getBinarySize(const string& valueName)
{
  return readRawData(valueName,NULL,0);
}
void RegKey::setValue(const string& valueName,const string& value) throw(SysError)
{
  size_t s=value.size();
  int n=RegSetValueEx(m_hkey.ptr()->get(),(valueName.c_str()==""?NULL:valueName.c_str()),0,REG_SZ,(UCHAR*)value.c_str(),value.size());
  if(n!=ERROR_SUCCESS)
    throw SysError(n);
}
void RegKey::setValue(const string& valueName,DWORD value)throw(SysError)
{
  int n=RegSetValueEx(m_hkey.ptr()->get(),(valueName.c_str()==""?NULL:valueName.c_str()),0,REG_DWORD,(UCHAR*)&value,sizeof(value));
  if(n!=ERROR_SUCCESS)
    throw SysError(n);
}

void RegKey::setValue(const string& valueName,const BYTE* buf,DWORD dwBufSize)throw(SysError)
{
  int n=RegSetValueEx(m_hkey.ptr()->get(),(valueName.c_str()==""?NULL:valueName.c_str()),0,REG_BINARY,buf,dwBufSize);
  if(n!=ERROR_SUCCESS)
    throw SysError(n);
}
void RegKey::eraseValue(const string& valueName)throw(SysError)
{
  int n=RegDeleteValue(m_hkey.ptr()->get(),valueName.c_str());
  if(n!=ERROR_SUCCESS)
    throw SysError(n);
}
list<pair<string,string> > RegKey::getValues()const throw(SysError)
{
  list<pair<string,string> > retlist;
  char buf[MAX_PATH];
  char buf2[MAX_PATH];
  ULONG size,size2;
  DWORD dwType;
  int n;
  int c=0;
  do
  {
    size=size2=MAX_PATH;
    n=RegEnumValue(m_hkey.ptr()->get(),c++,buf,&size,NULL,&dwType,(UCHAR*)buf2,&size2);
    switch(n)
    {
    case ERROR_SUCCESS:
      {
        switch(dwType)
        {
        case REG_SZ:
          retlist.push_back(make_pair<string,string>(buf,buf2));
          break;
        case REG_DWORD:
          {
            ostringstream oss;
            oss<<*(int*)buf[2];
            retlist.push_back(make_pair<string,string>(buf,oss.str()));
            break;
          }
        case REG_EXPAND_SZ:
          {
            ExpandEnvironmentStrings(buf2,buf2,MAX_PATH);
            retlist.push_back(make_pair<string,string>(buf,buf2));
            break;
          }
        default:
          retlist.push_back(make_pair<string,string>(buf,"[Unrecognized regvalue type]"));
        }
      }
    case ERROR_NO_MORE_ITEMS:
      break;
    default:
      throw SysError(n);
    }
  }
  while(n!=ERROR_NO_MORE_ITEMS);
  return retlist;
}

RegKey RegKey::createSubkey(const string& name)throw (SysError)
{
  return RegKey(m_hkey.ptr()->get(),name);
}

RegKey::operator HKEY()const
{
  return m_hkey.ptr()->get();
}

bool RegKey::hasValue(const string& name)const throw(SysError)
{
  bool b=true;
  try
  {
    readRawData(name,NULL,0);
  }
  catch(const SysError& err)
  {
    switch(err.code())
    {
    case ERROR_FILE_NOT_FOUND:
      b=false;
    case ERROR_MORE_DATA:
      break;
    default:
      throw;

    }
  }
  return b;
}
