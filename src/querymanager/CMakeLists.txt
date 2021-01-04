find_package(BISON REQUIRED)
find_package(FLEX REQUIRED)

BISON_TARGET(Parser ${CMAKE_CURRENT_SOURCE_DIR}/parser.y ${CMAKE_CURRENT_BINARY_DIR}/parser.tab.cpp
        VERBOSE ${CMAKE_CURRENT_BINARY_DIR}/parser.output)
FLEX_TARGET(Lexer ${CMAKE_CURRENT_SOURCE_DIR}/lexer.l ${CMAKE_CURRENT_BINARY_DIR}/parser.yy.cpp)

include_directories(${CMAKE_CURRENT_BINARY_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

ADD_FLEX_BISON_DEPENDENCY(Lexer Parser)

add_library(sqlparser
        ${BISON_Parser_OUTPUT_SOURCE}
        ${FLEX_Lexer_OUTPUTS}
        ${BISON_Parser_OUTPUT_HEADER}
        )

set(SOURCE
        ${SOURCE}
        ${CMAKE_CURRENT_SOURCE_DIR}/QueryManager.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/SqlParser.cpp
        PARENT_SCOPE
        )

set(HEADERS
        ${HEADERS}
        ${CMAKE_CURRENT_SOURCE_DIR}/QueryManager.h
        ${CMAKE_CURRENT_SOURCE_DIR}/SqlParser.h
        PARENT_SCOPE
        )
