# - Collection of String utility macros.
# Defines the following macros:
#   STRING_TRIM(var str [NOUNQUOTE])
#   - Trim a string by removing the leading and trailing spaces,
#     just like string(STRIP ...) in CMake 2.6 and later.
#     This macro is needed as CMake 2.4 does not support string(STRIP ..)
#     This macro also remove quote and double quote marks around the string,
#     unless NOUNQUOTE is defined.
#     * Parameters:
#       + var: A variable that stores the result.
#       + str: A string.
#       + NOUNQUOTE: (Optional) do not remove the double quote mark around the string.
#
#   STRING_UNQUOTE(var str)
#   - Remove double quote marks and quote marks around a string.
#     If the string is not quoted, then it returns an empty string.
#     * Parameters:
#       + var: A variable that stores the result.
#       + str: A string.
#
#   STRING_JOIN(var delimiter str_list [str...])
#   - Concatenate strings, with delimiter inserted between strings.
#     * Parameters:
#       + var: A variable that stores the result.
#       + str_list: A list of string.
#       + str: (Optional) more string to be join.
#
#   STRING_SPLIT(var delimiter str [NOESCAPE_SEMICOLON])
#   - Split a string into a list using a delimiter, which can be in 1 or more
#     characters long.
#     * Parameters:
#       + var: A variable that stores the result.
#       + delimiter: To separate a string.
#       + str: A string.
#       + NOESCAPE_SEMICOLON: (Optional) Do not escape semicolons.
#

if(NOT DEFINED _MANAGE_STRING_CMAKE_)
    set(_MANAGE_STRING_CMAKE_ "DEFINED")

    macro(STRING_TRIM var str)
	set(${var} "")
	if(NOT "${ARGN}" STREQUAL "NOUNQUOTE")
	    # Need not trim a quoted string.
	    STRING_UNQUOTE(_var "${str}")
	    if(NOT _var STREQUAL "")
		# String is quoted
		set(${var} "${_var}")
	    endif(NOT _var STREQUAL "")
	endif(NOT "${ARGN}" STREQUAL "NOUNQUOTE")

	if(${var} STREQUAL "")
	    set(_var_1 "${str}")
	    string(REGEX REPLACE  "^[ \t\r\n]+" "" _var_2 "${str}" )
	    string(REGEX REPLACE  "[ \t\r\n]+$" "" _var_3 "${_var_2}" )
	    set(${var} "${_var_3}")
	endif(${var} STREQUAL "")
    endmacro(STRING_TRIM var str)

    # Internal macro
    # Variable cannot be escaped here, as variable is already substituted
    # at the time it passes to this macro.
    macro(STRING_ESCAPE var str)
	# ';' and '\' are tricky, need to be encoded.
	# '#' => '#H'
	# '\' => '#B'
	# ';' => '#S'
	set(_NOESCAPE_SEMICOLON "")
	set(_NOESCAPE_HASH "")

	foreach(_arg ${ARGN})
	    if(${_arg} STREQUAL "NOESCAPE_SEMICOLON")
		set(_NOESCAPE_SEMICOLON "NOESCAPE_SEMICOLON")
	    elseif(${_arg} STREQUAL "NOESCAPE_HASH")
		set(_NOESCAPE_HASH "NOESCAPE_HASH")
	    endif(${_arg} STREQUAL "NOESCAPE_SEMICOLON")
	endforeach(_arg)

	if(_NOESCAPE_HASH STREQUAL "")
	    string(REGEX REPLACE "#" "#H" _ret "${str}")
	else(_NOESCAPE_HASH STREQUAL "")
	    set(_ret "${str}")
	endif(_NOESCAPE_HASH STREQUAL "")

	string(REGEX REPLACE "\\\\" "#B" _ret "${_ret}")
	if(_NOESCAPE_SEMICOLON STREQUAL "")
	    string(REGEX REPLACE ";" "#S" _ret "${_ret}")
	endif(_NOESCAPE_SEMICOLON STREQUAL "")
	set(${var} "${_ret}")
    endmacro(STRING_ESCAPE var str)

    macro(STRING_UNESCAPE var str)
	# '#B' => '\'
	# '#H' => '#'
	# '#D' => '$'
	# '#S' => ';'
	set(_ESCAPE_VARIABLE "")
	set(_NOESCAPE_SEMICOLON "")
	set(_ret "${str}")
	foreach(_arg ${ARGN})
	    if(${_arg} STREQUAL "NOESCAPE_SEMICOLON")
		set(_NOESCAPE_SEMICOLON "NOESCAPE_SEMICOLON")
	    elseif(${_arg} STREQUAL "ESCAPE_VARIABLE")
		set(_ESCAPE_VARIABLE "ESCAPE_VARIABLE")
		string(REGEX REPLACE "#D" "$" _ret "${_ret}")
	    endif(${_arg} STREQUAL "NOESCAPE_SEMICOLON")
	endforeach(_arg)

	string(REGEX REPLACE "#B" "\\\\" _ret "${_ret}")
	if(_NOESCAPE_SEMICOLON STREQUAL "")
	    # ';' => '#S'
	    string(REGEX REPLACE "#S" "\\\\;" _ret "${_ret}")
	else(_NOESCAPE_SEMICOLON STREQUAL "")
	    string(REGEX REPLACE "#S" ";" _ret "${_ret}")
	endif(_NOESCAPE_SEMICOLON STREQUAL "")

	if(NOT _ESCAPE_VARIABLE STREQUAL "")
	    # '#D' => '$'
	    string(REGEX REPLACE "#D" "$" _ret "${_ret}")
	endif(NOT _ESCAPE_VARIABLE STREQUAL "")
	string(REGEX REPLACE "#H" "#" _ret "${_ret}")
	set(${var} "${_ret}")
    endmacro(STRING_UNESCAPE var str)


    macro(STRING_UNQUOTE var str)
	STRING_ESCAPE(_ret "${str}" ${ARGN})
	if(_ret MATCHES "^[ \t\r\n]+")
	    string(REGEX REPLACE "^[ \t\r\n]+" "" _ret "${_ret}")
	endif(_ret MATCHES "^[ \t\r\n]+")
	if(_ret MATCHES "^\"")
	    # Double quote
	    string(REGEX REPLACE "\"\(.*\)\"[ \t\r\n]*$" "\\1" _ret "${_ret}")
	elseif(_ret MATCHES "^'")
	    # Single quote
	    string(REGEX REPLACE "'\(.*\)'[ \t\r\n]*$" "\\1" _ret "${_ret}")
	else(_ret MATCHES "^\"")
	    set(_ret "")
	endif(_ret MATCHES "^\"")

	# Unencoding
	STRING_UNESCAPE(${var} "${_ret}" ${ARGN})
    endmacro(STRING_UNQUOTE var str)

    macro(STRING_JOIN var delimiter str_list)
	set(_ret "")
	foreach(_str ${str_list})
	    if(_ret STREQUAL "")
		set(_ret "${_str}")
	    else(_ret STREQUAL "")
		set(_ret "${_ret}${delimiter}${_str}")
	    endif(_ret STREQUAL "")
	endforeach(_str ${str_list})

	foreach(_str ${ARGN})
	    if(_ret STREQUAL "")
		set(_ret "${_str}")
	    else(_ret STREQUAL "")
		set(_ret "${_ret}${delimiter}${_str}")
	    endif(_ret STREQUAL "")
	endforeach(_str ${str_list})
	set(${var} "${_ret}")
    endmacro(STRING_JOIN var delimiter str_list)

    macro(STRING_SPLIT var delimiter str)
	set(_max_tokens "")
	set(_NOESCAPE_SEMICOLON "")
	set(_ESCAPE_VARIABLE "")
	foreach(_arg ${ARGN})
	    if(${_arg} STREQUAL "NOESCAPE_SEMICOLON")
		set(_NOESCAPE_SEMICOLON "NOESCAPE_SEMICOLON")
	    elseif(${_arg} STREQUAL "ESCAPE_VARIABLE")
		set(_ESCAPE_VARIABLE "ESCAPE_VARIABLE")
	    else(${_arg} STREQUAL "NOESCAPE_SEMICOLON")
		set(_max_tokens ${_arg})
	    endif(${_arg} STREQUAL "NOESCAPE_SEMICOLON")
	endforeach(_arg)

	if(NOT _max_tokens)
	    set(_max_tokens -1)
	endif(NOT _max_tokens)

	STRING_ESCAPE(_str "${str}" ${_NOESCAPE_SEMICOLON} ${_ESCAPE_VARIABLE})
	STRING_ESCAPE(_delimiter "${delimiter}" ${_NOESCAPE_SEMICOLON} ${_ESCAPE_VARIABLE})

	set(_str_list "")
	set(_token_count 0)
	string(LENGTH "${_delimiter}" _de_len)

	while(NOT _token_count EQUAL _max_tokens)
	    math(EXPR _token_count ${_token_count}+1)
	    if(_token_count EQUAL _max_tokens)
		# Last token, no need splitting
		set(_str_list ${_str_list} "${_str}")
	    else(_token_count EQUAL _max_tokens)
		# in case encoded characters are delimiters
		string(LENGTH "${_str}" _str_len)
		set(_index 0)
		set(_token "")
		set(_str_remain "")
		math(EXPR _str_end ${_str_len}-${_de_len}+1)
		set(_bound "k")
		while(_index LESS _str_end)
		    string(SUBSTRING "${_str}" ${_index} ${_de_len} _str_cursor)
		    if(_str_cursor STREQUAL _delimiter)
			# Get the token
			string(SUBSTRING "${_str}" 0 ${_index} _token)
			# Get the rest
			math(EXPR _rest_index ${_index}+${_de_len})
			math(EXPR _rest_len ${_str_len}-${_index}-${_de_len})
			string(SUBSTRING "${_str}" ${_rest_index} ${_rest_len} _str_remain)
			set(_index ${_str_end})
		    else(_str_cursor STREQUAL _delimiter)
			math(EXPR _index ${_index}+1)
		    endif(_str_cursor STREQUAL _delimiter)
		endwhile(_index LESS _str_end)

		if(_str_remain STREQUAL "")
		    # Meaning: end of string
		    list(APPEND _str_list "${_str}")
		    set(_max_tokens ${_token_count})
		else(_str_remain STREQUAL "")
		    list(APPEND _str_list "${_token}")
		    set(_str "${_str_remain}")
		endif(_str_remain STREQUAL "")
	    endif(_token_count EQUAL _max_tokens)
	endwhile(NOT _token_count EQUAL _max_tokens)


	# Unencoding
	STRING_UNESCAPE(${var} "${_str_list}" ${_NOESCAPE_SEMICOLON} ${_ESCAPE_VARIABLE})
    endmacro(STRING_SPLIT var delimiter str)

endif(NOT DEFINED _MANAGE_STRING_CMAKE_)

