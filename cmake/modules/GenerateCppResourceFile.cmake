
# Convert a binary data file into a C++
# source file for embedding into an application binary
#
# Currently only implemented for Unix.  Requires the 'xxd'
# tool to be installed.
#
# INPUT_FILE : The name of the binary data file to be converted into a C++
#              source file.
#
# CPP_FILE   : The path of the C++ source file to be generated.
#              See the documentation for xxd for information on
#              the structure of the generated source file.
#
# INPUT_FILE_TARGET : The name of the target which generates INPUT_FILE
#
function (generate_cpp_resource_file TARGET_NAME INPUT_DIR INPUT_FILE CPP_FILE)
	add_custom_command(OUTPUT ${CPP_FILE} COMMAND cd ${INPUT_DIR} && xxd -i ${INPUT_FILE} ${CPP_FILE}
	                   DEPENDS ${INPUT_DIR}/${INPUT_FILE})
	add_custom_target(${TARGET_NAME} ALL DEPENDS ${CPP_FILE})
endfunction()
