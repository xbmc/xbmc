// RegKey.h  RegKey class header.
//
//////////////////////////////////////////////////////////////////////
#pragma warning (disable:4786)
#pragma warning (disable:4290)
#ifndef REGKEY_H
#define REGKEY_H
#include <winsock2.h>
#include <list>
#include <string>
#include <DshowUtil/Pointers.h>
#include <DshowUtil/SysError.h>

class RegKey
{
public:
  RegKey(HKEY base,const std::string& pathname,bool bCreate=true)throw(SysError);
  virtual ~RegKey(){};
  std::list<std::string> getSubkeys()const throw(SysError);
  void erase()throw(SysError);
  std::string getValue(const std::string& valueName)const throw(SysError);
  DWORD getDwordValue(const std::string& valueName)const throw(SysError);
  RECT getRectValue(const std::string& valueName)const throw(SysError);
  BYTE* getBinaryValue(const std::string& valueName,BYTE* buf,DWORD dwBufSize)const throw(SysError);
  DWORD getBinarySize(const std::string& valueName);
  void setValue(const std::string& valueName,const std::string& value) throw(SysError);
  void setValue(const std::string& valueName,DWORD value)throw(SysError);
  void setValue(const std::string& valueName,const BYTE* buf,DWORD dwBufSize)throw(SysError);
  void eraseValue(const std::string& valueName)throw(SysError);
  std::list<std::pair<std::string,std::string> > getValues()const throw(SysError);
  RegKey createSubkey(const std::string& name)throw (SysError);
  operator HKEY()const;
  bool hasValue(const std::string& name)const throw(SysError);

private:
  class HKEYWrapper
  {
  public:
    HKEYWrapper(HKEY hkey,bool bClose=true):m_hkey(hkey),m_bClose(bClose){}
    virtual ~HKEYWrapper()
    {
      if(m_bClose)
        RegCloseKey(m_hkey);
    }
    HKEY get()const{return m_hkey;}
  private:
    void operator=(const HKEYWrapper&); //not implem
    HKEYWrapper(const HKEYWrapper&);//not impl
    HKEY m_hkey;
    bool m_bClose;
  };
  RefCountedPtr<HKEYWrapper> m_hkey;
  RefCountedPtr<HKEYWrapper> m_hkeyfather;
  std::string m_name;
  size_t readRawData(const std::string& valueName,BYTE* buf,DWORD bufsize)const throw(SysError);
};

#endif//REGKEY_H
