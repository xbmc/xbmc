# - Collection of String utility macros.
# Defines the following macros:
#   STRING_TRIM(var str [NOUNQUOTE])
#   - Trim a string by removing the leading and trailing spaces,
#     just like STRING(STRIP ...) in CMake 2.6 and later.
#     This macro is needed as CMake 2.4 does not support STRING(STRIP ..)
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

IF(NOT DEFINED _MANAGE_STRING_CMAKE_)
    SET(_MANAGE_STRING_CMAKE_ "DEFINED")

    MACRO(STRING_TRIM var str)
	SET(${var} "")
	IF (NOT "${ARGN}" STREQUAL "NOUNQUOTE")
	    # Need not trim a quoted string.
	    STRING_UNQUOTE(_var "${str}")
	    IF(NOT _var STREQUAL "")
		# String is quoted
		SET(${var} "${_var}")
	    ENDIF(NOT _var STREQUAL "")
	ENDIF(NOT "${ARGN}" STREQUAL "NOUNQUOTE")

	IF(${var} STREQUAL "")
	    SET(_var_1 "${str}")
	    STRING(REGEX REPLACE  "^[ \t\r\n]+" "" _var_2 "${str}" )
	    STRING(REGEX REPLACE  "[ \t\r\n]+$" "" _var_3 "${_var_2}" )
	    SET(${var} "${_var_3}")
	ENDIF(${var} STREQUAL "")
    ENDMACRO(STRING_TRIM var str)

    # Internal macro
    # Variable cannot be escaped here, as variable is already substituted
    # at the time it passes to this macro.
    MACRO(STRING_ESCAPE var str)
	# ';' and '\' are tricky, need to be encoded.
	# '#' => '#H'
	# '\' => '#B'
	# ';' => '#S'
	SET(_NOESCAPE_SEMICOLON "")
	SET(_NOESCAPE_HASH "")

	FOREACH(_arg ${ARGN})
	    IF(${_arg} STREQUAL "NOESCAPE_SEMICOLON")
		SET(_NOESCAPE_SEMICOLON "NOESCAPE_SEMICOLON")
	    ELSEIF(${_arg} STREQUAL "NOESCAPE_HASH")
		SET(_NOESCAPE_HASH "NOESCAPE_HASH")
	    ENDIF(${_arg} STREQUAL "NOESCAPE_SEMICOLON")
	ENDFOREACH(_arg)

	IF(_NOESCAPE_HASH STREQUAL "")
	    STRING(REGEX REPLACE "#" "#H" _ret "${str}")
	ELSE(_NOESCAPE_HASH STREQUAL "")
	    SET(_ret "${str}")
	ENDIF(_NOESCAPE_HASH STREQUAL "")

	STRING(REGEX REPLACE "\\\\" "#B" _ret "${_ret}")
	IF(_NOESCAPE_SEMICOLON STREQUAL "")
	    STRING(REGEX REPLACE ";" "#S" _ret "${_ret}")
	ENDIF(_NOESCAPE_SEMICOLON STREQUAL "")
	SET(${var} "${_ret}")
    ENDMACRO(STRING_ESCAPE var str)

    MACRO(STRING_UNESCAPE var str)
	# '#B' => '\'
	# '#H' => '#'
	# '#D' => '$'
	# '#S' => ';'
	SET(_ESCAPE_VARIABLE "")
	SET(_NOESCAPE_SEMICOLON "")
	SET(_ret "${str}")
	FOREACH(_arg ${ARGN})
	    IF(${_arg} STREQUAL "NOESCAPE_SEMICOLON")
		SET(_NOESCAPE_SEMICOLON "NOESCAPE_SEMICOLON")
	    ELSEIF(${_arg} STREQUAL "ESCAPE_VARIABLE")
		SET(_ESCAPE_VARIABLE "ESCAPE_VARIABLE")
		STRING(REGEX REPLACE "#D" "$" _ret "${_ret}")
	    ENDIF(${_arg} STREQUAL "NOESCAPE_SEMICOLON")
	ENDFOREACH(_arg)

	STRING(REGEX REPLACE "#B" "\\\\" _ret "${_ret}")
	IF(_NOESCAPE_SEMICOLON STREQUAL "")
	    # ';' => '#S'
	    STRING(REGEX REPLACE "#S" "\\\\;" _ret "${_ret}")
	ELSE(_NOESCAPE_SEMICOLON STREQUAL "")
	    STRING(REGEX REPLACE "#S" ";" _ret "${_ret}")
	ENDIF(_NOESCAPE_SEMICOLON STREQUAL "")

	IF(NOT _ESCAPE_VARIABLE STREQUAL "")
	    # '#D' => '$'
	    STRING(REGEX REPLACE "#D" "$" _ret "${_ret}")
	ENDIF(NOT _ESCAPE_VARIABLE STREQUAL "")
	STRING(REGEX REPLACE "#H" "#" _ret "${_ret}")
	SET(${var} "${_ret}")
    ENDMACRO(STRING_UNESCAPE var str)


    MACRO(STRING_UNQUOTE var str)
	STRING_ESCAPE(_ret "${str}" ${ARGN})
	IF(_ret MATCHES "^[ \t\r\n]+")
	    STRING(REGEX REPLACE "^[ \t\r\n]+" "" _ret "${_ret}")
	ENDIF(_ret MATCHES "^[ \t\r\n]+")
	IF(_ret MATCHES "^\"")
	    # Double quote
	    STRING(REGEX REPLACE "\"\(.*\)\"[ \t\r\n]*$" "\\1" _ret "${_ret}")
	ELSEIF(_ret MATCHES "^'")
	    # Single quote
	    STRING(REGEX REPLACE "'\(.*\)'[ \t\r\n]*$" "\\1" _ret "${_ret}")
	ELSE(_ret MATCHES "^\"")
	    SET(_ret "")
	ENDIF(_ret MATCHES "^\"")

	# Unencoding
	STRING_UNESCAPE(${var} "${_ret}" ${ARGN})
    ENDMACRO(STRING_UNQUOTE var str)

    MACRO(STRING_JOIN var delimiter str_list)
	SET(_ret "")
	FOREACH(_str ${str_list})
	    IF(_ret STREQUAL "")
		SET(_ret "${_str}")
	    ELSE(_ret STREQUAL "")
		SET(_ret "${_ret}${delimiter}${_str}")
	    ENDIF(_ret STREQUAL "")
	ENDFOREACH(_str ${str_list})

	FOREACH(_str ${ARGN})
	    IF(_ret STREQUAL "")
		SET(_ret "${_str}")
	    ELSE(_ret STREQUAL "")
		SET(_ret "${_ret}${delimiter}${_str}")
	    ENDIF(_ret STREQUAL "")
	ENDFOREACH(_str ${str_list})
	SET(${var} "${_ret}")
    ENDMACRO(STRING_JOIN var delimiter str_list)

    MACRO(STRING_SPLIT var delimiter str)
	SET(_max_tokens "")
	SET(_NOESCAPE_SEMICOLON "")
	SET(_ESCAPE_VARIABLE "")
	FOREACH(_arg ${ARGN})
	    IF(${_arg} STREQUAL "NOESCAPE_SEMICOLON")
		SET(_NOESCAPE_SEMICOLON "NOESCAPE_SEMICOLON")
	    ELSEIF(${_arg} STREQUAL "ESCAPE_VARIABLE")
		SET(_ESCAPE_VARIABLE "ESCAPE_VARIABLE")
	    ELSE(${_arg} STREQUAL "NOESCAPE_SEMICOLON")
		SET(_max_tokens ${_arg})
	    ENDIF(${_arg} STREQUAL "NOESCAPE_SEMICOLON")
	ENDFOREACH(_arg)

	IF(NOT _max_tokens)
	    SET(_max_tokens -1)
	ENDIF(NOT _max_tokens)

	STRING_ESCAPE(_str "${str}" ${_NOESCAPE_SEMICOLON} ${_ESCAPE_VARIABLE})
	STRING_ESCAPE(_delimiter "${delimiter}" ${_NOESCAPE_SEMICOLON} ${_ESCAPE_VARIABLE})

	SET(_str_list "")
	SET(_token_count 0)
	STRING(LENGTH "${_delimiter}" _de_len)

	WHILE(NOT _token_count EQUAL _max_tokens)
	    MATH(EXPR _token_count ${_token_count}+1)
	    IF(_token_count EQUAL _max_tokens)
		# Last token, no need splitting
		SET(_str_list ${_str_list} "${_str}")
	    ELSE(_token_count EQUAL _max_tokens)
		# in case encoded characters are delimiters
		STRING(LENGTH "${_str}" _str_len)
		SET(_index 0)
		SET(_token "")
		SET(_str_remain "")
		MATH(EXPR _str_end ${_str_len}-${_de_len}+1)
		SET(_bound "k")
		WHILE(_index LESS _str_end)
		    STRING(SUBSTRING "${_str}" ${_index} ${_de_len} _str_cursor)
		    IF(_str_cursor STREQUAL _delimiter)
			# Get the token
			STRING(SUBSTRING "${_str}" 0 ${_index} _token)
			# Get the rest
			MATH(EXPR _rest_index ${_index}+${_de_len})
			MATH(EXPR _rest_len ${_str_len}-${_index}-${_de_len})
			STRING(SUBSTRING "${_str}" ${_rest_index} ${_rest_len} _str_remain)
			SET(_index ${_str_end})
		    ELSE(_str_cursor STREQUAL _delimiter)
			MATH(EXPR _index ${_index}+1)
		    ENDIF(_str_cursor STREQUAL _delimiter)
		ENDWHILE(_index LESS _str_end)

		IF(_str_remain STREQUAL "")
		    # Meaning: end of string
		    LIST(APPEND _str_list "${_str}")
		    SET(_max_tokens ${_token_count})
		ELSE(_str_remain STREQUAL "")
		    LIST(APPEND _str_list "${_token}")
		    SET(_str "${_str_remain}")
		ENDIF(_str_remain STREQUAL "")
	    ENDIF(_token_count EQUAL _max_tokens)
	ENDWHILE(NOT _token_count EQUAL _max_tokens)


	# Unencoding
	STRING_UNESCAPE(${var} "${_str_list}" ${_NOESCAPE_SEMICOLON} ${_ESCAPE_VARIABLE})
    ENDMACRO(STRING_SPLIT var delimiter str)

ENDIF(NOT DEFINED _MANAGE_STRING_CMAKE_)

