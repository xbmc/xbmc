# - Try to find Lzo2
# Once done this will define
#
# Lzo2_FOUND - system has Lzo2
# Lzo2_INCLUDE_DIR - the Lzo2 include directory
# Lzo2_LIBRARIES - Link these to use Lzo2
# Lzo2_NEED_PREFIX - this is set if the functions are prefixed with BZ2_

# Copyright (c) 2006, Alexander Neundorf, <neundorf@kde.org>
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.


find_path(LZO2_INCLUDE_DIRS lzo1x.h PATH_SUFFIXES lzo)

find_library(LZO2_LIBRARIES NAMES lzo2 liblzo2)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Lzo2 DEFAULT_MSG LZO2_INCLUDE_DIRS LZO2_LIBRARIES)

mark_as_advanced(LZO2_INCLUDE_DIRS LZO2_LIBRARIES)
