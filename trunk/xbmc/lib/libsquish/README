LICENSE
-------

The squish library is distributed under the terms and conditions of the MIT
license. This license is specified at the top of each source file and must be
preserved in its entirety.

BUILDING AND INSTALLING THE LIBRARY
-----------------------------------

If you are using Visual Studio 2003 or above under Windows then load the Visual
Studio 2003 project in the vs7 folder. By default, the library is built using
SSE2 optimisations. To change this either change or remove the SQUISH_USE_SSE=2
from the preprocessor symbols.

If you are using a Mac then load the Xcode 2.2 project in the distribution. By
default, the library is built using Altivec optimisations. To change this
either change or remove SQUISH_USE_ALTIVEC=1 from the preprocessor symbols. I
guess I'll have to think about changing this for the new Intel Macs that are
rolling out...

If you are using unix then first edit the config file in the base directory of
the distribution, enabling Altivec or SSE with the USE_ALTIVEC or USE_SSE
variables, and editing the optimisation flags passed to the C++ compiler if
necessary. Then make can be used to build the library, and make install (from
the superuser account) can be used to install (into /usr/local by default).

REPORTING BUGS OR FEATURE REQUESTS
----------------------------------

Feedback can be sent to Simon Brown (the developer) at si@sjbrown.co.uk

New releases are announced on the squish library homepage at
http://sjbrown.co.uk/?code=squish

