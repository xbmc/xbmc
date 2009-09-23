REM quick and dirty build file for testing different MSDEVs
setlocal 

set myFLAGS= /I../include /I../mySTL /c /W3 /G6 /O2 

cl %myFLAGS% aes.cpp
cl %myFLAGS% aestables.cpp
cl %myFLAGS% algebra.cpp
cl %myFLAGS% arc4.cpp

cl %myFLAGS% asn.cpp
cl %myFLAGS% bftables.cpp
cl %myFLAGS% blowfish.cpp
cl %myFLAGS% coding.cpp

cl %myFLAGS% des.cpp
cl %myFLAGS% dh.cpp
cl %myFLAGS% dsa.cpp
cl %myFLAGS% file.cpp

cl %myFLAGS% hash.cpp
cl %myFLAGS% integer.cpp
cl %myFLAGS% md2.cpp
cl %myFLAGS% md4.cpp
cl %myFLAGS% md5.cpp

cl %myFLAGS% misc.cpp
cl %myFLAGS% random.cpp
cl %myFLAGS% ripemd.cpp
cl %myFLAGS% rsa.cpp

cl %myFLAGS% sha.cpp
cl %myFLAGS% template_instnt.cpp
cl %myFLAGS% tftables.cpp
cl %myFLAGS% twofish.cpp

link.exe -lib /out:taocrypt.lib aes.obj aestables.obj algebra.obj arc4.obj asn.obj bftables.obj blowfish.obj coding.obj des.obj dh.obj dsa.obj file.obj hash.obj integer.obj md2.obj md4.obj md5.obj misc.obj random.obj ripemd.obj rsa.obj sha.obj template_instnt.obj tftables.obj twofish.obj

