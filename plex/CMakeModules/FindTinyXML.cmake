# - Find TinyXML
# Find the native TinyXML includes and library
#
#   TINYXML_FOUND       - True if TinyXML found.
#   TINYXML_INCLUDE_DIR - where to find tinyxml.h, etc.
#   TINYXML_LIBRARY   - TinyXML library.
#

IF( TINYXML_INCLUDE_DIR )
    # Already in cache, be silent
    SET( TinyXML_FIND_QUIETLY TRUE )
ENDIF( TINYXML_INCLUDE_DIR )

FIND_PATH( TINYXML_INCLUDE_DIR "tinyxml.h"
           PATH_SUFFIXES "tinyxml" )

FIND_LIBRARY( TINYXML_LIBRARY
              NAMES "tinyxml"
              PATH_SUFFIXES "tinyxml" )

SET(TINYXML_FOUND "NO")
IF(TINYXML_LIBRARY AND TINYXML_INCLUDE_DIR)
    SET(TINYXML_FOUND "YES")
ENDIF(TINYXML_LIBRARY AND TINYXML_INCLUDE_DIR)
