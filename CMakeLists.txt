# VTK examples now require CMake 2.0 or higher.
CMAKE_MINIMUM_REQUIRED(VERSION 2.6)

PROJECT( SmokeSim )
FIND_PACKAGE(OpenCV REQUIRED)
find_package(OpenGL REQUIRED)
Find_Package(SDL REQUIRED) 
Find_Package(SDL_image REQUIRED)

if (NOT SDL_FOUND) 
message(FATAL_ERROR "SDL not found!") 
endif (NOT SDL_FOUND) 

find_package(GLUT)



SET( CMAKE_VERBOSE_MAKEFILE on )

set(CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE STRING "Configs" FORCE)

SET( EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin CACHE PATH
	"Single Directory for all Executables")
SET( LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR}/lib CACHE PATH
	"Single Directory for all Libraries")
SET( CMAKE_VERBOSE_MAKEFILE on )

# Common Warning = All On
# Use pipes instead of temp files (lots of memory on our machines, don't write scrub files!)
# -g3 means super-high debug info
set(CMAKE_CXX_FLAGS "-Wall -pipe -g3" CACHE STRING "compiler flags always used" FORCE) # temporary force -g3...
set(CMAKE_CXX_FLAGS_DEBUG "-g3" CACHE STRING "extra compiler flags for debug" FORCE )
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG" CACHE STRING "extra compiler flags for release" FORCE)
set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "configuration (default: debug)" )


include_directories(${CMAKE_SOURCE_DIR}/getopt)
link_directories(${CMAKE_BINARY_DIR}/lib)
add_subdirectory(getopt)
add_subdirectory(fluid-core) 
add_subdirectory(demos) 

message(STATUS "    Compiler:                  ${CMAKE_COMPILER}")
message(STATUS "    C++ flags (Release):       ${CMAKE_CXX_FLAGS} ${CMAKE_CXX_FLAGS_RELEASE}")
message(STATUS "    C++ flags (Debug):         ${CMAKE_CXX_FLAGS} ${CMAKE_CXX_FLAGS_DEBUG}")

message(STATUS "    SDL Include Dir:                  ${SDL_INCLUDE_DIR}")
