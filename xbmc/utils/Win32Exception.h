#ifdef _XBOX
#include "xtl.h"
#else
#include <windows.h>
#endif
#include <exception>

class win32_exception: public std::exception
{
public:
    typedef const void* Address; // OK on Win32 platform

    static void install_handler();
    virtual const char* what() const { return mWhat; };
    Address where() const { return mWhere; };
    unsigned code() const { return mCode; };
    virtual void writelog(const char *prefix) const;
protected:
    win32_exception(const EXCEPTION_RECORD& info);
    static void translate(unsigned code, EXCEPTION_POINTERS* info);
private:
    const char* mWhat;
    Address mWhere;
    unsigned mCode;
};

class access_violation: public win32_exception
{
public:
    bool iswrite() const { return mIsWrite; };
    Address address() const { return mBadAddress; };    
    virtual void writelog(const char *prefix) const;
private:
    bool mIsWrite;
    Address mBadAddress;
    access_violation(const EXCEPTION_RECORD& info);
    friend void win32_exception::translate(unsigned code, EXCEPTION_POINTERS* info);
};