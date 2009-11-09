The MediaInfoActiveX.dll is only a wrapper for the MediaInfo.dll
which must already be installed on your computer.

The MediaInfoActiveX.dll must be registered before it can be used.
If you copied it, for example, to C:\WINDOWS\SYSTEM, you must call
regsvr32 C:\WINDOWS\SYSTEM\MediaInfoActiveX.dll from Start / Run
to be able to use it.

Call regsvr32 /u C:\WINDOWS\SYSTEM\MediaInfoActiveX.dll to unregister.

There are two batch files named MediaInfoActiveX_Install.bat and
MediaInfoActiveX_UnInstall.bat carrying out all required steps for you.
