REM quick and dirty build file for testing different MSDEVs
setlocal 

set myFLAGS= /I../include /I../taocrypt/include /I../taocrypt/mySTL /c /W3 /G6 /O2 /MT /D"WIN32" /D"NO_MAIN_DRIVER"

cl %myFLAGS% testsuite.cpp
cl %myFLAGS% ../examples/client/client.cpp
cl %myFLAGS% ../examples/echoclient/echoclient.cpp
cl %myFLAGS% ../examples/server/server.cpp
cl %myFLAGS% ../examples/echoserver/echoserver.cpp
cl %myFLAGS% ../taocrypt/test/test.cpp

link.exe  /out:testsuite.exe ../src/yassl.lib ../taocrypt/src/taocrypt.lib testsuite.obj client.obj server.obj echoclient.obj echoserver.obj test.obj advapi32.lib Ws2_32.lib

