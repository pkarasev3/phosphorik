link_directories(  ${CMAKE_SOURCE_DIR}/lib )
include_directories(${CMAKE_SOURCE_DIR}/getopt)
include_directories(include ${SDL_INCLUDE_DIR}) 
include_directories(include ${SDL_IMAGE_INCLUDE_DIR}) 

file(GLOB fluid_sources src/*.cpp src/*.c )
file(GLOB fluid_headers include/*.h include/*.hpp)

include_directories(${CMAKE_SOURCE_DIR}/fluid-core/include)
set(CORE3DLIBS ${GLUT_LIBRARY} ${OPENGL_LIBRARY} )
set(SDL_LIBS  ${SDL_LIBRARY} ${SDLIMAGE_LIBRARY} SDLmain )

add_library(fluid-core ${fluid_headers} ${fluid_sources})
target_link_libraries(fluid-core ${OpenCV_LIBS} ${CORE3DLIBS} ${SDL_LIBS} )




