;  libFLAC - Free Lossless Audio Codec library
;  Copyright (C) 2001,2002,2003,2004,2005,2006,2007  Josh Coalson
;
;  Redistribution and use in source and binary forms, with or without
;  modification, are permitted provided that the following conditions
;  are met:
;
;  - Redistributions of source code must retain the above copyright
;  notice, this list of conditions and the following disclaimer.
;
;  - Redistributions in binary form must reproduce the above copyright
;  notice, this list of conditions and the following disclaimer in the
;  documentation and/or other materials provided with the distribution.
;
;  - Neither the name of the Xiph.org Foundation nor the names of its
;  contributors may be used to endorse or promote products derived from
;  this software without specific prior written permission.
;
;  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
;  ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
;  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
;  A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR
;  CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
;  EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
;  PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
;  PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
;  LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
;  NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
;  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

	bits 32

%ifdef OBJ_FORMAT_win32
	%define FLAC__PUBLIC_NEEDS_UNDERSCORE
	%idefine code_section section .text align=16 class=CODE use32
	%idefine data_section section .data align=32 class=DATA use32
	%idefine bss_section  section .bss  align=32 class=DATA use32
%elifdef OBJ_FORMAT_aout
	%define FLAC__PUBLIC_NEEDS_UNDERSCORE
	%idefine code_section section .text
	%idefine data_section section .data
	%idefine bss_section  section .bss
%elifdef OBJ_FORMAT_aoutb
	%define FLAC__PUBLIC_NEEDS_UNDERSCORE
	%idefine code_section section .text
	%idefine data_section section .data
	%idefine bss_section  section .bss
%elifdef OBJ_FORMAT_elf
	%idefine code_section section .text align=16
	%idefine data_section section .data align=32
	%idefine bss_section  section .bss  align=32
%else
	%error unsupported object format!
%endif

%imacro cglobal 1
	%ifdef FLAC__PUBLIC_NEEDS_UNDERSCORE
		global _%1
	%else
		global %1
	%endif
%endmacro

%imacro cextern 1
	%ifdef FLAC__PUBLIC_NEEDS_UNDERSCORE
		extern _%1
	%else
		extern %1
	%endif
%endmacro

%imacro cident 1
_%1:
%1:
%endmacro
