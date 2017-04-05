include(${ODB_USE_FILE})

set(OdbCMake_ODB_HEADERS
        ODBDate.h
        ODBPath.h
        ODBFile.h
        ODBRating.h
        ODBGenre.h
        ODBPerson.h
        ODBPersonLink.h
        ODBSet.h
        ODBStreamDetails.h
        ODBBookmark.h
        ODBUniqueID.h
        ODBArt.h
        ODBCountry.h
        ODBStudio.h
        ODBTag.h
        ODBMovie.h
        ODBSeason.h
        ODBTVShow.h
        ODBEpisode.h
        ODBSetting.h
        ODBStacktime.h
        ODBAlbum.h
        ODBSong.h
        ODBArtistDetail.h
        ODBArtistDiscography.h
        ODBCue.h
        ODBRole.h
        ODBVersionTagScan.h)

odb_compile(OdbCMake_SOURCES FILES ${OdbCMake_ODB_HEADERS}
    MULTI_DATABASE dynamic
    DB sqlite mysql
    GENERATE_QUERY
    GENERATE_SESSION
    GENERATE_SCHEMA
    GENERATE_PREPARED
    SCHEMA_FORMAT embedded
    CPP11)

set(SOURCES ${OdbCMake_SOURCES})
set(HEADERS ${OdbCMake_ODB_HEADERS})

add_library(odb_binding STATIC ${SOURCES} ${HEADERS})
set_target_properties(odb_binding PROPERTIES LINKER_LANGUAGE CXX)

foreach(header ${OdbCMake_ODB_HEADERS})
    configure_file(${header} ${ODB_COMPILE_OUTPUT_DIR}/${header} COPYONLY)
endforeach()

set(core_DEPENDS odb_binding ${core_DEPENDS} CACHE STRING "" FORCE)
