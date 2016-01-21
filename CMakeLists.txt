project(modern-opengl-samples)
cmake_minimum_required(VERSION 3.0)

find_package(PkgConfig)

pkg_check_modules(SDL2 REQUIRED sdl2)
pkg_check_modules(OPENGL REQUIRED gl glew)
pkg_check_modules(LIBPNG REQUIRED libpng)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
include_directories(${OPENGL_INCLUDE_DIRECTORIES} ${SDL2_INCLUDE_DIRECTORIES})

add_executable(glDrawArrays "glDrawArrays/main")
target_link_libraries(glDrawArrays ${OPENGL_LIBRARIES} ${SDL2_LIBRARIES})

add_executable(glDrawElements "glDrawElements/main")
target_link_libraries(glDrawElements ${OPENGL_LIBRARIES} ${SDL2_LIBRARIES})

add_executable(texture "Texture/main")
target_link_libraries(texture ${OPENGL_LIBRARIES} ${SDL2_LIBRARIES} ${LIBPNG_LIBRARIES})
