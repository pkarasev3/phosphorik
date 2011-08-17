cmake_minimum_required(VERSION 2.6)
#cmake_policy(SET CMP0015 NEW)
PROJECT( getopt )
SET( CMAKE_VERBOSE_MAKEFILE on )

SET( EXECUTABLE_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/bin CACHE PATH
	"Single Directory for all Executables")
SET( LIBRARY_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/lib CACHE PATH
	"Single Directory for all Libraries")
SET( CMAKE_VERBOSE_MAKEFILE on )


file(GLOB getoptfiles *.h *.cpp)
add_library(getopt ${getoptfiles})

