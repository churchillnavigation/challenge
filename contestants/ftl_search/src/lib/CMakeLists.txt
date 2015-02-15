file( GLOB_RECURSE SRC_LIST "*.cpp" "." )

include_directories( . )
add_library( libftl_search SHARED ${SRC_LIST} libftl_search.def )
