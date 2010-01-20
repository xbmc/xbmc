Compiled using foobar2000 0.7 final SDK.

Changes 1.4 -> 1.5
------------------
* Added options for looping tracks (number of loops and fade length)

Changes 1.3 -> 1.4
------------------
* Finally fixed long-standing issues with memory leaks

Changes 1.2b -> 1.3
-------------------
* Compiled with 0.7 final SDK
* Added options to allow for turning individual YM2612 and PSG channels on and off

Changes 1.2a -> 1.2b
--------------------

* Necessary changes for 0.7b29 SDK made

Changes 1.2 -> 1.2a
-------------------

* Only populates info fields if they aren't empty

Changes 1.1 -> 1.2
------------------

* Fixed (I think) potential crash bug in loading EZPK-compressed GYM files
* Necessary changes made for foobar 0.7(b13)
* Some minor code cleanup, mostly dealing with configuration
* Compressed with UPX

Changes 1.0b -> 1.1
-------------------

* Now supports EZPK-compressed (basically customized libbzip2) GYM files
* No longer relies on zlib.dll (necessary parts statically linked in now instead)
* Some minor optimization

---

Credits:


Uses source code from libbzip2, zlib, Gens, and kpigym.  Uses information from YMAMP 2.0.

  libbzip2 1.0.2  copyright (C) 1996-2002 Julian R Seward.  All rights reserved.
  zlib 1.1.4    (C) 1995-1998 Jean-loup Gailly and Mark Adler
  Gens 2.12a  Copyright (c) 2002 by Stéphane Dallongeville
  kpigym 1.0r6+ (C) 2002 by Mamiya
  YMAMP 2.0  (C) 1999-2000 by Marp

<----- END OF ORIGINAL DOCUMENTATION ------>

I used the sources for foo_gym as basis for adding the plugin to xbmc. thanks a lot! 
currently stripped for zlib and libbzip2 

spiff