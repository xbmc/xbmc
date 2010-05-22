In order to build this codec, you need the Windows98 DDK from Microsoft. It can also work
with the Windows2000/ME/XP/2003 DDK:

http://www.microsoft.com/ddk/

Alternatively, the required headers are also available in CYGWIN+w32api, MINGW32 or Wine :

http://www.cygwin.com/
http://www.mingw.org/
http://www.winehq.com/


If you do not have a ddk, you should be able to use the alternative msacmdrv.h provided
with this ACM codec. It is not used by default because it would probably broke any real
DDK already installed.



---------------

Define ENABLE_DECODING if you want to use the decoding (alpha state, doesn't decode at the
 moment, so use it only if you plan to develop)

---------------

To release this codec you will need :
- lameACM.acm (result of the build process)
- lameACM.inf
- lame_acm.xml (where the initial configuration is stored)
