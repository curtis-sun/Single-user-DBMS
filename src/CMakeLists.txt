add_subdirectory(utils)
add_subdirectory(bufmanager)
add_subdirectory(fileio)
add_subdirectory(recordmanager)
add_subdirectory(index)
add_subdirectory(systemmanager)
add_subdirectory(querymanager)

add_definitions(-DDEBUG)
add_definitions("-Wall -g")

add_library(${CMAKE_PROJECT_NAME}_lib ${SOURCE} ${HEADERS})
add_executable(${CMAKE_PROJECT_NAME} main.cpp)
target_link_libraries(${CMAKE_PROJECT_NAME} sqlparser ${CMAKE_PROJECT_NAME}_lib)