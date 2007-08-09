FTGL 1.31

NOTES FOR COMPILING ON WINDOWS 

14 Feb 2002

Ellers, ellers@iinet.net.au





SUPPORTED COMPILERS



I have rebuilt the FTGL project files for Visual C++ (version 6). There are

presently no other compilers or environments supported but feel free to

contribute them. 





QUICK GUIDE: COMPILING FTGL



 - Start up MSVC++ with ftgl.dsw. 

 - Tell MSVC++ where FreeType is. You'll need to do something like this:

 

     *  select Project>Settings

     *  select ftgl_static (for a start)

     *  select "All Configurations"

     *  go to the tab C++ > PreProcessor

     *  Set additional include directories appropriately. For me it is:

        D:\cots\freetype-2.0.5\include

     *  repeat for all configurations of ftgl_dll





QUICK GUIDE: COMPILING/RUNNING SUPPLIED DEMO PROGRAM 



 - The program expects the first argument to be the name of a truetype file.

   I copied timesbi.ttf from the windows directory to C:\TEMP and then edit

   the settings of the project:

   

    * select Project>Settings

    * select Demo project

    * select panel Debug>General

    * set Program Arguments to be "C:\TEMP\timesbi.ttf"





QUICK GUIDE: COMPILING YOUR PROGRAM TO USE FTGL



 - Choose dynamic or static library linkage

     *  if you want to link to a static FTGL library ensure that 

        FTGL_LIBRARY_STATIC is defined in the preprocessor section

     



CONFIGURATION / CODE GENERATION / C LIBRARIES



FTGL can be built in various configurations (inspired by Freetype and libpng):



 - static library (.lib)

 - dynamic library (.dll)

 

MSVC++ requires selection of "code generation" option, which seems to be 

mostly to do with which version of the Standard C library is linked with the

library. 



The following modes are supported:



 - static/dynamic

 - single threaded (ST) or multithreaded (MT)

   NOTE: the multithreaded DLL (MD) mode was NOT included, as freetype itself

         doesn't support that mode so I figure there's no point yet.

 - debug/release (debug has _d suffix)

 

So the static multithreaded release library is:



	ftgl_static_MT.lib

	

The same library built in DEBUG mode:



	ftgl_static_MT_d.lib



If you're not sure which one is appropriate (and if you're a novice don't

been too put off...) start with making the decision about debug or release.

This should be easy because if you're building the debug version of your 

app its probably a good idea to link with the debug version of FTGL (but

not compulsory). Once thats done, you may get errors like:



	LIBCMTD.lib(crt0init.obj) : warning LNK4098: defaultlib "libcmt.lib" conflicts with use of other libs; use /NODEFAULTLIB:library

 

This will happen, for example, when you link a glut app with an FTGL library

compiled with different codegen options than the GLUT library. 



MSVC++ "sort of" 

requires that all libs be linked with the same codegen option. GLUT is built

in XXX mode, so if you're linking with GLUT, you can get rid of the warning

by linking with the XXX version of FTGL. The various versions are particularly

useful if you're doing std C stuff, like printf etc. 







FAQ



Q: "But... do I HAVE to use all these DIFFERENT build modes, like multi-

   threaded, debug single threaded, etc?"

   

A: No. Sometimes library makers only generate one style anyway. It depends

   on your needs. Unless you're linking with standard C stuff (e.g. printf)

   then it probably won't make a great deal of difference. If you get 

   warnings about "default lib libcmt.lib conflicts" etc, then you can make

   use of the different libraries.



