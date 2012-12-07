find_all_sources(. WINSRC)
list(REMOVE_ITEM WINSRC ./pch.cpp)
list(REMOVE_ITEM WINSRC ./WMIInterface.cpp)
add_sources(${WINSRC})
