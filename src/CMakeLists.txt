# This file was automatically generated for projects
# without default 'CMakeLists.txt' file.

FILE(GLOB_RECURSE app_sources ${CMAKE_SOURCE_DIR}/src/*.*)
set(lib_sources
	${CMAKE_SOURCE_DIR}/lib/at24c32/at24c32.cpp
	${CMAKE_SOURCE_DIR}/lib/logger/logger.cpp
)

idf_component_register(
	SRCS ${app_sources} ${lib_sources}
	INCLUDE_DIRS
		${CMAKE_SOURCE_DIR}/lib/at24c32
		${CMAKE_SOURCE_DIR}/lib/logger
)