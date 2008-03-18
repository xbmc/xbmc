
This contains the VS.NET solution to create a xbmc compatible sqlite3.dll.  The only change is the max page size is set at 16384 in src/pager.h.

This will compile the dll as sqlite-3.2.2-xbmc.dll.  It just needs to be renamed to sqlite3.dll and placed in the same folder as sqlite3explorer or other sqlite dbms tools.  A precompiled win32 version is available in the bin directory.