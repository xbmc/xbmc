# -*- mode: python -*-
#
# Unix SMB/CIFS implementation.
# Module packaging setup for Samba python extensions
#
# Copyright (C) Tim Potter, 2002-2003
# Copyright (C) Martin Pool, 2002
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#   
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#   
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
#

from distutils.core import setup
from distutils.extension import Extension

import sys, string, os

# The Makefile passes in environment variable $PYTHON_OBJ as being the
# list of Samba objects.  This kind of goes against the distutils.cmd
# method of adding setup commands and will also confuse people who are
# familiar with the python Distutils module.

samba_objs = os.environ.get("PYTHON_OBJS", "")

samba_cflags = os.environ.get("PYTHON_CFLAGS", "")

samba_srcdir = os.environ.get("SRCDIR", "")

compiler = os.environ.get("CC", "")

# These variables are filled in by configure

samba_libs = os.environ.get("LIBS", "")

obj_list = string.split(samba_objs)

# Unfortunately the samba_libs variable contains both shared libraries
# and linker flags.  The python distutils doesn't like this so we have
# to split $samba_libs into a flags component and a library component.

libraries = []
library_dirs = []

next_is_path = 0
next_is_flag = 0

for lib in string.split(samba_libs):
    if next_is_path != 0:
        library_dirs.append(lib);
        next_is_path = 0;
    elif next_is_flag != 0:
        next_is_flag = 0;
    elif lib == "-Wl,-rpath":
        next_is_path = 1;
    elif lib[0:2] == ("-l"):
        libraries.append(lib[2:])
    elif lib[0:8] == ("-pthread"):
        pass # Skip linker flags
    elif lib[0:2] == "-L":
        library_dirs.append(lib[2:])
    elif lib[0:2] in ("-W","-s"):
        pass # Skip linker flags
    elif lib[0:2] == "-z":
        next_is_flag = 1 # Skip linker flags
    else:
        print "Unknown entry '%s' in $LIBS variable passed to setup.py" % lib
        sys.exit(1)

flags_list = string.split(samba_cflags)

# Invoke distutils.setup

setup(

    # Overview information
    
    name = "Samba Python Extensions",
    version = "0.1",
    author = "Tim Potter",
    author_email = "tpot@samba.org",
    license = "GPL",

    # Get the "samba" directory of Python source.  At the moment this
    # just contains the __init__ file that makes it work as a
    # subpackage.  This is needed even though everything else is an
    # extension module.
    package_dir = {"samba": os.path.join(samba_srcdir, "python", "samba")},
    packages = ["samba"],
    
    # Module list
    ext_package = "samba", 
    ext_modules = [

    # SPOOLSS pipe module

    Extension(name = "spoolss",
              sources = [samba_srcdir + "python/py_spoolss.c",
                         samba_srcdir + "python/py_common.c",
                         samba_srcdir + "python/py_conv.c",
                         samba_srcdir + "python/py_ntsec.c",
                         samba_srcdir + "python/py_spoolss_common.c",
                         samba_srcdir + "python/py_spoolss_forms.c",
                         samba_srcdir + "python/py_spoolss_forms_conv.c",
                         samba_srcdir + "python/py_spoolss_drivers.c",
                         samba_srcdir + "python/py_spoolss_drivers_conv.c",
                         samba_srcdir + "python/py_spoolss_printers.c",
                         samba_srcdir + "python/py_spoolss_printers_conv.c",
                         samba_srcdir + "python/py_spoolss_printerdata.c",
                         samba_srcdir + "python/py_spoolss_ports.c",
                         samba_srcdir + "python/py_spoolss_ports_conv.c",
                         samba_srcdir + "python/py_spoolss_jobs.c",
                         samba_srcdir + "python/py_spoolss_jobs_conv.c",
                         ],
              libraries = libraries,
              library_dirs = ["/usr/kerberos/lib"] + library_dirs,
              extra_compile_args = flags_list,
              extra_objects = obj_list),

    # LSA pipe module

    Extension(name = "lsa",
              sources = [samba_srcdir + "python/py_lsa.c",
                         samba_srcdir + "python/py_common.c",
                         samba_srcdir + "python/py_ntsec.c"],
              libraries = libraries,
              library_dirs = ["/usr/kerberos/lib"] + library_dirs,
              extra_compile_args = flags_list,
              extra_objects = obj_list),

    # SAMR pipe module

    Extension(name = "samr",
              sources = [samba_srcdir + "python/py_samr.c",
                         samba_srcdir + "python/py_conv.c",
                         samba_srcdir + "python/py_samr_conv.c",
                         samba_srcdir + "python/py_common.c"],
              libraries = libraries,
              library_dirs = ["/usr/kerberos/lib"] + library_dirs,
              extra_compile_args = flags_list,
              extra_objects = obj_list),

    # winbind client module

    Extension(name = "winbind",
              sources = [samba_srcdir + "python/py_winbind.c",
                         samba_srcdir + "python/py_winbind_conv.c",
                         samba_srcdir + "python/py_conv.c",
                         samba_srcdir + "python/py_common.c"],
              libraries = libraries,
              library_dirs = ["/usr/kerberos/lib"] + library_dirs,
              extra_compile_args = flags_list,
              extra_objects = obj_list),

    # WINREG pipe module

    Extension(name = "winreg",
              sources = [samba_srcdir + "python/py_winreg.c",
                         samba_srcdir + "python/py_common.c"],
              libraries = libraries,
              library_dirs = ["/usr/kerberos/lib"] + library_dirs,
              extra_compile_args = flags_list,
              extra_objects = obj_list),

    # SRVSVC pipe module

    Extension(name = "srvsvc",
              sources = [samba_srcdir + "python/py_srvsvc.c",
                         samba_srcdir + "python/py_conv.c",
                         samba_srcdir + "python/py_srvsvc_conv.c",
                         samba_srcdir + "python/py_common.c"],
              libraries = libraries,
              library_dirs = ["/usr/kerberos/lib"] + library_dirs,
              extra_compile_args = flags_list,
              extra_objects = obj_list),

    # tdb module

    Extension(name = "tdb",
              sources = [samba_srcdir + "python/py_tdb.c"],
              libraries = libraries,
              library_dirs = ["/usr/kerberos/lib"] + library_dirs,
              extra_compile_args = flags_list,
              extra_objects = obj_list),

    # libsmb module

    Extension(name = "smb",
              sources = [samba_srcdir + "python/py_smb.c",
                         samba_srcdir + "python/py_common.c",
                         samba_srcdir + "python/py_ntsec.c"],
              libraries = libraries,
              library_dirs = ["/usr/kerberos/lib"] + library_dirs,
              extra_compile_args = flags_list,
              extra_objects = obj_list),

    # tdbpack/unpack extensions.  Does not actually link to any Samba
    # code, although it implements a compatible data format.
    
    Extension(name = "tdbpack",
              sources = [os.path.join(samba_srcdir, "python", "py_tdbpack.c")],
              extra_compile_args = ["-I."])
    ],
)
