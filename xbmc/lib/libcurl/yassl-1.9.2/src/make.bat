REM quick and dirty build file for testing different MSDEVs
setlocal 

set myFLAGS= /I../include /I../taocrypt/mySTL /I../taocrypt/include /W3 /c /ZI

cl %myFLAGS% buffer.cpp
cl %myFLAGS% cert_wrapper.cpp
cl %myFLAGS% crypto_wrapper.cpp
cl %myFLAGS% handshake.cpp

cl %myFLAGS% lock.cpp
cl %myFLAGS% log.cpp
cl %myFLAGS% socket_wrapper.cpp
cl %myFLAGS% ssl.cpp

cl %myFLAGS% template_instnt.cpp
cl %myFLAGS% timer.cpp
cl %myFLAGS% yassl.cpp
cl %myFLAGS% yassl_error.cpp

cl %myFLAGS% yassl_imp.cpp
cl %myFLAGS% yassl_int.cpp

link.exe -lib /out:yassl.lib buffer.obj cert_wrapper.obj crypto_wrapper.obj handshake.obj lock.obj log.obj socket_wrapper.obj ssl.obj template_instnt.obj timer.obj yassl.obj yassl_error.obj yassl_imp.obj yassl_int.obj



