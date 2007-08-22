/*
  * dlfcn_win32.h	1.0 2003/01/16
  *
  * Emulates the POSIX dlfcn.h
  *
  * By Wu Yongwei
  *
  */
 
 #ifndef _DLFCN_WIN32_H
 #define _DLFCN_WIN32_H
 
 #ifdef _WIN32
 #define WIN32_LEAN_AND_MEAN
 #include <windows.h>
 #include <errno.h>
 #define dlopen(P,G) (void*)LoadLibrary(P)
 #define dlsym(D,F) (void*)GetProcAddress((HMODULE)D,F)
 #define dlclose(D) FreeLibrary((HMODULE)D)
 __inline const char* dlerror()
 {
 	static char szMsgBuf[256];
 	FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
 			NULL,
 			GetLastError(),
 			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
 			szMsgBuf,
 			sizeof szMsgBuf,
 			NULL);
 	return szMsgBuf;
 }
 #endif /* _WIN32 */
 
 #endif /* _DLFCN_WIN32_H */
