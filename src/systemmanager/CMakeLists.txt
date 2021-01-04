set(HEADERS
        ${HEADERS}
        ${CMAKE_CURRENT_SOURCE_DIR}/Table.h
        ${CMAKE_CURRENT_SOURCE_DIR}/Database.h
        ${CMAKE_CURRENT_SOURCE_DIR}/TableHeader.h
        PARENT_SCOPE
        )

set(SOURCE
        ${SOURCE}
        ${CMAKE_CURRENT_SOURCE_DIR}/Database.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/Table.cpp
        PARENT_SCOPE
        )