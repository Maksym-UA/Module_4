# This file was automatically generated for projects
# without default 'CMakeLists.txt' file.

FILE(GLOB_RECURSE app_sources ${CMAKE_SOURCE_DIR}/src/*.*)
set(lib_sources
	${CMAKE_SOURCE_DIR}/lib/at24c32/at24c32.cpp
	${CMAKE_SOURCE_DIR}/lib/logger/logger.cpp
)

idf_component_register(
    SRCS "main.cpp" "../lib/at24c32/at24c32.cpp" "../lib/logger/logger.cpp"
    INCLUDE_DIRS "." "../lib/at24c32" "../lib/logger"
)