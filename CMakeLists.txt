cmake_minimum_required(VERSION 3.10)
project(ConcurrencyKit C)

set(SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
set(BUILD_DIR "${CMAKE_BINARY_DIR}")
set(VERSION "0.7.0")
set(VERSION_MAJOR "0")
set(PREFIX "/usr/local" CACHE PATH "Installation prefix")
set(LIBRARY "${PREFIX}/lib")
set(HEADERS "${PREFIX}/include/ck")
set(PKGCONFIG_DATA "${LIBRARY}/pkgconfig")
set(LDNAME "libck.so")
set(LDNAME_VERSION "libck.so.${VERSION}")
set(LDNAME_MAJOR "libck.so.${VERSION_MAJOR}")

set(CFLAGS -Wall -Wextra)
set(LDFLAGS)

if(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64")
    list(APPEND CFLAGS -m64 -D__x86_64__)
    list(APPEND LDFLAGS -m64)
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "i[3-6]86")
    list(APPEND CFLAGS -m32 -D__x86__)
    list(APPEND LDFLAGS -m32)
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "arm")
    list(APPEND CFLAGS -D__arm__)
else()
    message(WARNING "Arch not supported: ${CMAKE_SYSTEM_PROCESSOR}")
endif()

include_directories(${SRC_DIR}/include)

set(SOURCES
    ${SRC_DIR}/src/ck_array.c
    ${SRC_DIR}/src/ck_ec.c
    ${SRC_DIR}/src/ck_epoch.c
    ${SRC_DIR}/src/ck_hs.c
    ${SRC_DIR}/src/ck_rhs.c
    ${SRC_DIR}/src/ck_ht.c
    ${SRC_DIR}/src/ck_hp.c
    ${SRC_DIR}/src/ck_barrier_centralized.c
    ${SRC_DIR}/src/ck_barrier_combining.c
    ${SRC_DIR}/src/ck_barrier_dissemination.c
    ${SRC_DIR}/src/ck_barrier_tournament.c
    ${SRC_DIR}/src/ck_barrier_mcs.c
)


add_library(ck_shared SHARED ${SOURCES})
set_target_properties(ck_shared PROPERTIES 
    OUTPUT_NAME "ck"
    SOVERSION ${VERSION_MAJOR}
    VERSION ${VERSION}
)
target_compile_options(ck_shared PRIVATE ${CFLAGS})
if(LDFLAGS)
    target_link_options(ck_shared PRIVATE ${LDFLAGS})
endif()


add_library(ck_static STATIC ${SOURCES})
set_target_properties(ck_static PROPERTIES OUTPUT_NAME "ck")
target_compile_options(ck_static PRIVATE ${CFLAGS})
if(LDFLAGS)
    target_link_options(ck_static PRIVATE ${LDFLAGS})
endif()


install(DIRECTORY ${SRC_DIR}/include/ 
        DESTINATION ${HEADERS}
        FILES_MATCHING PATTERN "*.h"
)
install(TARGETS ck_shared ck_static
        LIBRARY DESTINATION ${LIBRARY}
        ARCHIVE DESTINATION ${LIBRARY}
)
install(CODE "execute_process(COMMAND ${CMAKE_COMMAND} -E create_symlink ${LDNAME_VERSION} ${LIBRARY}/${LDNAME})")
install(CODE "execute_process(COMMAND ${CMAKE_COMMAND} -E create_symlink ${LDNAME_VERSION} ${LIBRARY}/${LDNAME_MAJOR})")
install(FILES ${BUILD_DIR}/build/ck.pc 
        DESTINATION ${PKGCONFIG_DATA}
)


add_custom_target(doc
    COMMAND ${CMAKE_MAKE_PROGRAM} -C ${SRC_DIR}/doc all
    WORKING_DIRECTORY ${SRC_DIR}/doc
)


add_custom_target(regressions
    COMMAND ${CMAKE_MAKE_PROGRAM} -C ${SRC_DIR}/regressions all
    WORKING_DIRECTORY ${SRC_DIR}/regressions
)


add_custom_target(check
    COMMAND ${CMAKE_MAKE_PROGRAM} -C ${SRC_DIR}/regressions check
    WORKING_DIRECTORY ${SRC_DIR}/regressions
    DEPENDS regressions
)


add_custom_target(dist
    COMMAND git archive --format=tar --prefix=ck-${VERSION}/ HEAD | gzip > ${BUILD_DIR}/ck-${VERSION}.tar.gz
    WORKING_DIRECTORY ${SRC_DIR}
)


add_custom_target(distclean
    COMMAND ${CMAKE_MAKE_PROGRAM} clean
    COMMAND ${CMAKE_COMMAND} -E remove 
        ${BUILD_DIR}/include/ck_md.h
        ${BUILD_DIR}/build/regressions.build
        ${BUILD_DIR}/build/ck.build
        ${BUILD_DIR}/build/ck.pc
        ${BUILD_DIR}/Makefile
        ${BUILD_DIR}/doc/Makefile
        ${BUILD_DIR}/*~
        ${BUILD_DIR}/*.o
        ${BUILD_DIR}/*.tar.gz
)

add_custom_target(uninstall
    COMMAND ${CMAKE_COMMAND} -E remove 
        ${LIBRARY}/${LDNAME_VERSION}
        ${LIBRARY}/${LDNAME}
        ${LIBRARY}/${LDNAME_MAJOR}
        ${LIBRARY}/libck.so*
        ${LIBRARY}/libck.a
        ${HEADERS}/ck_*.h
        ${HEADERS}/spinlock/*.h
        ${HEADERS}/gcc/ck_*.h
        ${HEADERS}/gcc/*/ck_*.h
        ${PKGCONFIG_DATA}/ck.pc
)


add_subdirectory(examples)