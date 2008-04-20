BEGIN {
	print "/* ";
	print "   Unix SMB/CIFS implementation.";
	print "   Build Options for Samba Suite";
	print "   Copyright (C) Vance Lankhaar <vlankhaar@linux.ca> 2003";
	print "   Copyright (C) Andrew Bartlett <abartlet@samba.org> 2001";
	print "   ";
	print "   This program is free software; you can redistribute it and/or modify";
	print "   it under the terms of the GNU General Public License as published by";
	print "   the Free Software Foundation; either version 2 of the License, or";
	print "   (at your option) any later version.";
	print "   ";
	print "   This program is distributed in the hope that it will be useful,";
	print "   but WITHOUT ANY WARRANTY; without even the implied warranty of";
	print "   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the";
	print "   GNU General Public License for more details.";
	print "   ";
	print "   You should have received a copy of the GNU General Public License";
	print "   along with this program; if not, write to the Free Software";
	print "   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.";
	print "*/";
	print "";
	print "#include \"includes.h\"";
	print "#include \"build_env.h\"";
	print "#include \"dynconfig.h\"";
	print "";
	print "static void output(BOOL screen, const char *format, ...) PRINTF_ATTRIBUTE(2,3);";
	print "";
	print "";
	print "/****************************************************************************";
	print "helper function for build_options";
	print "****************************************************************************/";
	print "static void output(BOOL screen, const char *format, ...)";
	print "{";
	print "       char *ptr;";
	print "       va_list ap;";
	print "       ";
	print "       va_start(ap, format);";
	print "       vasprintf(&ptr,format,ap);";
	print "       va_end(ap);";
	print "";
	print "       if (screen) {";
	print "              d_printf(\"%s\", ptr);";
	print "       } else {";
	print "	       DEBUG(4,(\"%s\", ptr));";
	print "       }";
	print "       ";
	print "       SAFE_FREE(ptr);";
	print "}";
	print "";
	print "/****************************************************************************";
	print "options set at build time for the samba suite";
	print "****************************************************************************/";
	print "void build_options(BOOL screen)";
	print "{";
	print "       if ((DEBUGLEVEL < 4) && (!screen)) {";
	print "	       return;";
	print "       }";
	print "";
	print "#ifdef _BUILD_ENV_H";
	print "       /* Output information about the build environment */";
	print "       output(screen,\"Build environment:\\n\");";
	print "       output(screen,\"   Built by:    %s@%s\\n\",BUILD_ENV_USER,BUILD_ENV_HOST);";
	print "       output(screen,\"   Built on:    %s\\n\",BUILD_ENV_DATE);";
	print "";
	print "       output(screen,\"   Built using: %s\\n\",BUILD_ENV_COMPILER);";
	print "       output(screen,\"   Build host:  %s\\n\",BUILD_ENV_UNAME);";
	print "       output(screen,\"   SRCDIR:      %s\\n\",BUILD_ENV_SRCDIR);";
	print "       output(screen,\"   BUILDDIR:    %s\\n\",BUILD_ENV_BUILDDIR);";
	print "";
	print "     ";  
	print "#endif";
	print "";

	print "       /* Output various paths to files and directories */";
	print "       output(screen,\"\\nPaths:\\n\");";

	print "       output(screen,\"   SBINDIR: %s\\n\", dyn_SBINDIR);";
	print "       output(screen,\"   BINDIR: %s\\n\", dyn_BINDIR);";
	print "       output(screen,\"   SWATDIR: %s\\n\", dyn_SWATDIR);";

	print "       output(screen,\"   CONFIGFILE: %s\\n\", dyn_CONFIGFILE);";
	print "       output(screen,\"   LOGFILEBASE: %s\\n\", dyn_LOGFILEBASE);";
	print "       output(screen,\"   LMHOSTSFILE: %s\\n\",dyn_LMHOSTSFILE);";

	print "       output(screen,\"   LIBDIR: %s\\n\",dyn_LIBDIR);";
	print "       output(screen,\"   SHLIBEXT: %s\\n\",dyn_SHLIBEXT);";

	print "       output(screen,\"   LOCKDIR: %s\\n\",dyn_LOCKDIR);";
	print "       output(screen,\"   PIDDIR: %s\\n\", dyn_PIDDIR);";

	print "       output(screen,\"   SMB_PASSWD_FILE: %s\\n\",dyn_SMB_PASSWD_FILE);";
	print "       output(screen,\"   PRIVATE_DIR: %s\\n\",dyn_PRIVATE_DIR);";
	print "";


##################################################
# predefine first element of *_ary
# predefine *_i (num of elements in *_ary)
	with_ary[0]="";
	with_i=0;
	have_ary[0]="";
	have_i=0;
	utmp_ary[0]="";
	utmp_i=0;
	misc_ary[0]="";
	misc_i=0;
	sys_ary[0]="";
	sys_i=0;
	headers_ary[0]="";
	headers_i=0;
	in_comment = 0;
}

# capture single line comments
/^\/\* (.*?)\*\// {
	last_comment = $0;
	next;
}

# end capture multi-line comments
/(.*?)\*\// {
	last_comment = last_comment $0; 
	in_comment = 0;
	next;
}

# capture middle lines of multi-line comments
in_comment {
	last_comment = last_comment $0; 
	next;
}

# begin capture multi-line comments
/^\/\* (.*?)/ {
	last_comment = $0;
	in_comment = 1;
	next
}

##################################################
# if we have an #undef and a last_comment, store it
/^\#undef/ {
	split($0,a);
	comments_ary[a[2]] = last_comment;
	last_comment = "";
}

##################################################
# for each line, sort into appropriate section
# then move on

/^\#undef WITH/ {
	with_ary[with_i++] = a[2];
	# we want (I think) to allow --with to show up in more than one place, so no next
}


/^\#undef HAVE_UT_UT_/ || /^\#undef .*UTMP/ {
	utmp_ary[utmp_i++] = a[2];
	next;
}

/^\#undef HAVE_SYS_.*?_H$/ {
	sys_ary[sys_i++] = a[2];
	next;
}

/^\#undef HAVE_.*?_H$/ {
	headers_ary[headers_i++] = a[2];
	next;
}

/^\#undef HAVE_/ {
	have_ary[have_i++] = a[2];
	next;
}

/^\#undef/ {
	misc_ary[misc_i++] = a[2];
	next;
}


##################################################
# simple sort function
function sort(ARRAY, ELEMENTS) {
        for (i = 1; i <= ELEMENTS; ++i) {
                for (j = i; (j-1) in ARRAY && (j) in ARRAY && ARRAY[j-1] > ARRAY[j]; --j) {
                        temp = ARRAY[j];
			ARRAY[j] = ARRAY[j-1];
			ARRAY[j-1] = temp;
		}
        }
	return;
}    


##################################################
# output code from list of defined
# expects: ARRAY     an array of things defined
#          ELEMENTS  number of elements in ARRAY
#          TITLE     title for section
# returns: nothing 
function output(ARRAY, ELEMENTS, TITLE) {
	
	# add section header
	print "\n\t/* Show " TITLE " */";
	print "\toutput(screen, \"\\n " TITLE ":\\n\");\n";
	

	# sort element using bubble sort (slow, but easy)
	sort(ARRAY, ELEMENTS);

	# loop through array of defines, outputting code
	for (i = 0; i < ELEMENTS; i++) {
		print "#ifdef " ARRAY[i];
		
		# I don't know which one to use....
		
		print "\toutput(screen, \"   " ARRAY[i] "\\n\");";
		#printf "\toutput(screen, \"   %s\\n   %s\\n\\n\");\n", comments_ary[ARRAY[i]], ARRAY[i];
		#printf "\toutput(screen, \"   %-35s   %s\\n\");\n", ARRAY[i], comments_ary[ARRAY[i]];

		print "#endif";
	}
	return;
}

END {
	##################################################
	# add code to show various options
	print "/* Output various other options (as gleaned from include/config.h.in) */";
	output(sys_ary,     sys_i,     "System Headers");
	output(headers_ary, headers_i, "Headers");
	output(utmp_ary,    utmp_i,    "UTMP Options");
	output(have_ary,    have_i,    "HAVE_* Defines");
	output(with_ary,    with_i,    "--with Options");
	output(misc_ary,    misc_i,    "Build Options");

	##################################################
	# add code to display the various type sizes
	print "       /* Output the sizes of the various types */";
	print "       output(screen, \"\\nType sizes:\\n\");";
	print "       output(screen, \"   sizeof(char):         %lu\\n\",(unsigned long)sizeof(char));";
	print "       output(screen, \"   sizeof(int):          %lu\\n\",(unsigned long)sizeof(int));";
	print "       output(screen, \"   sizeof(long):         %lu\\n\",(unsigned long)sizeof(long));";
	print "#if HAVE_LONGLONG"
	print "       output(screen, \"   sizeof(long long):    %lu\\n\",(unsigned long)sizeof(long long));";
	print "#endif"
	print "       output(screen, \"   sizeof(uint8):        %lu\\n\",(unsigned long)sizeof(uint8));";
	print "       output(screen, \"   sizeof(uint16):       %lu\\n\",(unsigned long)sizeof(uint16));";
	print "       output(screen, \"   sizeof(uint32):       %lu\\n\",(unsigned long)sizeof(uint32));";
	print "       output(screen, \"   sizeof(short):        %lu\\n\",(unsigned long)sizeof(short));";
	print "       output(screen, \"   sizeof(void*):        %lu\\n\",(unsigned long)sizeof(void*));";
	print "       output(screen, \"   sizeof(size_t):       %lu\\n\",(unsigned long)sizeof(size_t));";
	print "       output(screen, \"   sizeof(off_t):        %lu\\n\",(unsigned long)sizeof(off_t));";
	print "       output(screen, \"   sizeof(ino_t):        %lu\\n\",(unsigned long)sizeof(ino_t));";
	print "       output(screen, \"   sizeof(dev_t):        %lu\\n\",(unsigned long)sizeof(dev_t));";

	##################################################
	# add code to give information about modules
	print "       output(screen, \"\\nBuiltin modules:\\n\");";
	print "       output(screen, \"   %s\\n\", STRING_STATIC_MODULES);";

	print "}";

}

