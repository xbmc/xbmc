find_program(7ZIP_EXECUTABLE NAMES 7z.exe
             HINTS PATHS "c:/Program Files/7-Zip")

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(7Zip DEFAULT_MSG 7ZIP_EXECUTABLE)

mark_as_advanced(7ZIP_EXECUTABLE)
