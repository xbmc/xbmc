set(SOURCES Database.cpp
            DatabaseQuery.cpp
            dataset.cpp
            qry_dat.cpp
            sqlitedataset.cpp)

set(HEADERS Database.h
            DatabaseQuery.h
            dataset.h
            qry_dat.h
            sqlitedataset.h)

if(TARGET ${APP_NAME_LC}::MySqlClient OR TARGET ${APP_NAME_LC}::MariaDBClient)
  list(APPEND SOURCES mysqldataset.cpp)
  list(APPEND HEADERS mysqldataset.h)
endif()

core_add_library(dbwrappers)
