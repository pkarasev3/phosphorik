include_directories(${CMAKE_SOURCE_DIR}/fluid-core/include)
include_directories(include ${SDL_INCLUDE_DIR})
include_directories(include ${SDL_IMAGE_INCLUDE_DIR})

set(CORE3DLIBS ${GLUT_LIBRARY} ${OPENGL_LIBRARY} m)
set(SDL_LIBS  ${SDL_LIBRARY} ${SDLIMAGE_LIBRARY} SDLmain )

add_executable(fire32-demo  fire32-demo.cpp )
target_link_libraries(fire32-demo fluid-core  ${Boost_LIBRARIES}
                                 ${OpenCV_LIBS} ${CORE3DLIBS} ${SDL_LIBS} )
