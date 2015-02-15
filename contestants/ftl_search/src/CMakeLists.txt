project( ftl_search )
cmake_minimum_required( VERSION 2.8.9 )

set( FTL_SEARCH_VERSION_MAJOR 0 )
set( FTL_SEARCH_VERSION_MINOR 1 )
set( FTL_SEARCH_VERSION_PATCH 0 )

add_definitions( -DFTL_WITH_MAIN=1 )

add_subdirectory( lib )

include_directories( lib )

add_executable( ftl_search ftl_search.cpp )
target_link_libraries( ftl_search libftl_search )
