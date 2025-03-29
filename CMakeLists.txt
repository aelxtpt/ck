cmake_minimum_required(VERSION 3.10)
project(ConcurrencyKit C)

# Definir variáveis básicas
set(SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
set(BUILD_DIR "${CMAKE_BINARY_DIR}")
set(VERSION "0.7.2")
set(VERSION_MAJOR "0")
set(PREFIX "/usr/local" CACHE PATH "Installation prefix")
set(LIBRARY "${PREFIX}/lib" CACHE PATH "Library installation directory")
set(HEADERS "${PREFIX}/include" CACHE PATH "Headers installation directory")
set(MANDIR "${PREFIX}/share/man" CACHE PATH "Manual pages installation directory")
set(PKGCONFIG_DATA "${LIBRARY}/pkgconfig")
set(LDNAME "libck.so")
set(LDNAME_VERSION "libck.so.${VERSION}")
set(LDNAME_MAJOR "libck.so.${VERSION_MAJOR}")

option(ENABLE_POINTER_PACKING "Enable pointer packing" OFF)
option(ENABLE_PPC32_LWSYNC "Enable lwsync for PPC32" OFF)
option(ENABLE_RTM "Enable restricted transactional memory" OFF)
option(ENABLE_LSE "Enable large system extensions" OFF)
option(DISABLE_SSE "Disable SSE instructions" OFF)
option(DISABLE_DOUBLE "Disable double type functions" OFF)
option(DISABLE_SHARED "Disable shared library" OFF)
option(DISABLE_STATIC "Disable static library" OFF)
option(WITH_PIC "Generate position-independent code" ON)
option(USE_CC_BUILTINS "Use compiler atomic builtins" OFF)
set(PROFILE "" CACHE STRING "Custom build profile")
set(VMA_BITS "unknown" CACHE STRING "Number of valid VMA bits")
set(MEMORY_MODEL "unknown" CACHE STRING "Memory model (tso, pso, rmo)")
set(CORES "" CACHE STRING "Number of cores available on target machine")
set(PLATFORM "" CACHE STRING "Force platform type")

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(SYSTEM "linux")
    execute_process(COMMAND egrep "(^CPU[0-9]+|^processor.*:.*)" /proc/cpuinfo COMMAND wc -l
                    OUTPUT_VARIABLE DCORES OUTPUT_STRIP_TRAILING_WHITESPACE)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    set(SYSTEM "darwin")
    execute_process(COMMAND sysctl -n hw.ncpu OUTPUT_VARIABLE DCORES OUTPUT_STRIP_TRAILING_WHITESPACE)
elseif(CMAKE_SYSTEM_NAME STREQUAL "FreeBSD" OR CMAKE_SYSTEM_NAME STREQUAL "GNU/kFreeBSD")
    set(SYSTEM "freebsd")
    execute_process(COMMAND sysctl -n hw.ncpu OUTPUT_VARIABLE DCORES OUTPUT_STRIP_TRAILING_WHITESPACE)
elseif(CMAKE_SYSTEM_NAME STREQUAL "NetBSD")
    set(SYSTEM "netbsd")
    execute_process(COMMAND sysctl -n hw.ncpu OUTPUT_VARIABLE DCORES OUTPUT_STRIP_TRAILING_WHITESPACE)
elseif(CMAKE_SYSTEM_NAME STREQUAL "OpenBSD")
    set(SYSTEM "openbsd")
    execute_process(COMMAND sysctl -n hw.ncpu OUTPUT_VARIABLE DCORES OUTPUT_STRIP_TRAILING_WHITESPACE)
elseif(CMAKE_SYSTEM_NAME STREQUAL "SunOS")
    set(SYSTEM "solaris")
elseif(CMAKE_SYSTEM_NAME MATCHES "MINGW32.*" OR CMAKE_SYSTEM_NAME MATCHES "MSYS_NT.*")
    set(SYSTEM "mingw32")
elseif(CMAKE_SYSTEM_NAME MATCHES "MINGW64.*")
    set(SYSTEM "mingw64")
elseif(CMAKE_SYSTEM_NAME MATCHES "CYGWIN_NT.*")
    set(SYSTEM "cygwin")
else()
    message(FATAL_ERROR "Unsupported operating system: ${CMAKE_SYSTEM_NAME}")
endif()
set(CORES "${CORES}" "${DCORES}" CACHE STRING "Number of cores" FORCE)

if(NOT PLATFORM)
    if(SYSTEM STREQUAL "freebsd")
        execute_process(COMMAND uname -p OUTPUT_VARIABLE PLATFORM OUTPUT_STRIP_TRAILING_WHITESPACE)
    else()
        set(PLATFORM "${CMAKE_SYSTEM_PROCESSOR}")
    endif()
endif()

if(PLATFORM MATCHES "macppc|Power Macintosh|powerpc|powerpcspe")
    set(PLATFORM "ppc")
    set(ENVIRONMENT "32")
    set(RTM_ENABLE OFF)
    set(LSE_ENABLE OFF)
    set(MM "CK_MD_RMO")
    list(APPEND LDFLAGS -m32)
elseif(PLATFORM MATCHES "sun4u|sun4v|sparc64")
    set(PLATFORM "sparcv9")
    set(ENVIRONMENT "64")
    set(RTM_ENABLE OFF)
    set(LSE_ENABLE OFF)
    set(MM "CK_MD_TSO")
    list(APPEND LDFLAGS -m64)
elseif(PLATFORM MATCHES "i[3-6]86|pentium.*|athlon.*|k5|k6|k6_2|k6_3")
    set(PLATFORM "x86")
    set(ENVIRONMENT "32")
    set(LSE_ENABLE OFF)
    set(MM "CK_MD_TSO")
    if(NOT DISABLE_SSE)
        list(APPEND CFLAGS -msse -msse2)
    endif()
    if(SYSTEM STREQUAL "darwin")
        set(PLATFORM "x86_64")
        set(ENVIRONMENT "64")
    endif()
elseif(PLATFORM MATCHES "amd64|x86_64")
    set(PLATFORM "x86_64")
    set(ENVIRONMENT "64")
    set(LSE_ENABLE OFF)
    set(MM "CK_MD_TSO")
    list(APPEND LDFLAGS -m64)
elseif(PLATFORM MATCHES "ppc64|ppc64le|powerpc64")
    set(PLATFORM "ppc64")
    set(ENVIRONMENT "64")
    set(RTM_ENABLE OFF)
    set(LSE_ENABLE OFF)
    set(MM "CK_MD_RMO")
elseif(PLATFORM MATCHES "arm|armv6|armv6l|armv7|armv7l")
    set(PLATFORM "arm")
    set(ENVIRONMENT "32")
    set(RTM_ENABLE OFF)
    set(LSE_ENABLE OFF)
    set(MM "CK_MD_RMO")
    if(PLATFORM MATCHES "armv6.*")
        list(APPEND CFLAGS -march=armv6k)
    elseif(PLATFORM MATCHES "armv7.*")
        list(APPEND CFLAGS -march=armv7-a)
    endif()
elseif(PLATFORM MATCHES "arm64|aarch64")
    set(PLATFORM "aarch64")
    set(ENVIRONMENT "64")
    set(RTM_ENABLE OFF)
    set(MM "CK_MD_RMO")
elseif(PLATFORM MATCHES "riscv32|riscv")
    set(PLATFORM "riscv32")
    set(ENVIRONMENT "32")
    set(RTM_ENABLE OFF)
    set(MM "CK_MD_RMO")
elseif(PLATFORM MATCHES "riscv64")
    set(PLATFORM "riscv64")
    set(ENVIRONMENT "64")
    set(RTM_ENABLE OFF)
    set(MM "CK_MD_RMO")
elseif(PLATFORM MATCHES "s390x")
    set(PLATFORM "s390x")
    set(ENVIRONMENT "64")
    set(RTM_ENABLE OFF)
    set(LSE_ENABLE OFF)
    set(MM "CK_MD_RMO")
else()
    set(PLATFORM "unknown")
    set(RTM_ENABLE OFF)
    set(LSE_ENABLE OFF)
    set(MM "CK_MD_RMO")
    set(USE_CC_BUILTINS ON)
endif()

if(MEMORY_MODEL STREQUAL "tso")
    set(MM "CK_MD_TSO")
elseif(MEMORY_MODEL STREQUAL "pso")
    set(MM "CK_MD_PSO")
elseif(MEMORY_MODEL STREQUAL "rmo")
    set(MM "CK_MD_RMO")
elseif(MEMORY_MODEL STREQUAL "unknown")
else()
    message(FATAL_ERROR "Invalid memory model: ${MEMORY_MODEL}")
endif()

if(VMA_BITS STREQUAL "unknown")
    if(PLATFORM MATCHES "x86|x86_64")
        if(SYSTEM STREQUAL "darwin")
            execute_process(COMMAND sysctl -n machdep.cpu.address_bits.virtual
                            OUTPUT_VARIABLE VMA OUTPUT_STRIP_TRAILING_WHITESPACE)
        elseif(SYSTEM STREQUAL "linux")
            execute_process(COMMAND awk "/address sizes/ {print $7;exit}" /proc/cpuinfo
                            OUTPUT_VARIABLE VMA OUTPUT_STRIP_TRAILING_WHITESPACE)
        elseif(PLATFORM STREQUAL "x86")
            set(VMA "32")
        endif()
    endif()
    if(NOT VMA)
        set(VMA "unknown")
    endif()
    set(VMA_BITS "${VMA}")
endif()
if(VMA_BITS STREQUAL "unknown")
    set(VMA_BITS_R "CK_MD_VMA_BITS_UNKNOWN")
    set(VMA_BITS_VALUE_R "")
    set(POINTER_PACK_ENABLE OFF)
else()
    set(VMA_BITS_R "CK_MD_VMA_BITS")
    set(VMA_BITS_VALUE_R "${VMA_BITS}ULL")
endif()

set(CFLAGS -D_BSD_SOURCE -D_DEFAULT_SOURCE -std=gnu99 -pedantic -Wall -W -Wundef -Wendif-labels -Wshadow -Wpointer-arith -Wcast-align -Wcast-qual -Wwrite-strings -Wstrict-prototypes -Wmissing-prototypes -Wnested-externs -Winline -Wdisabled-optimization -fstrict-aliasing -O2 -pipe -Wno-parentheses)
set(LDFLAGS)
set(PTHREAD_CFLAGS -pthread)
if(WITH_PIC)
    list(APPEND CFLAGS -fPIC)
    list(APPEND LDFLAGS -shared -fPIC)
else()
    list(APPEND CFLAGS -fno-PIC)
    list(APPEND LDFLAGS -fno-PIC)
endif()
if(USE_CC_BUILTINS)
    list(APPEND CFLAGS -DCK_USE_CC_BUILTINS=1)
    set(PC_CFLAGS "-DCK_USE_CC_BUILTINS=1")
endif()
if(SYSTEM MATCHES "mingw.*|cygwin")
    list(APPEND LDFLAGS -mthreads)
endif()
if(SYSTEM STREQUAL "netbsd")
    list(APPEND CFLAGS -D_NETBSD_SOURCE)
endif()
if(NOT SYSTEM STREQUAL "dragonflybsd")
    list(APPEND CFLAGS -D_XOPEN_SOURCE=600)
endif()

find_program(CC NAMES cc gcc clang)
if(NOT CC)
    message(FATAL_ERROR "No suitable compiler found")
endif()
find_program(LD NAMES ld ${CC})
find_program(AR NAMES ar)
if(NOT AR)
    message(FATAL_ERROR "No suitable archiver found")
endif()

if(ENABLE_POINTER_PACKING)
    set(POINTER_PACK_ENABLE "CK_MD_POINTER_PACK_ENABLE")
else()
    set(POINTER_PACK_ENABLE "CK_MD_POINTER_PACK_DISABLE")
endif()
if(DISABLE_DOUBLE)
    set(DISABLE_DOUBLE "CK_PR_DISABLE_DOUBLE")
else()
    set(DISABLE_DOUBLE "CK_PR_ENABLE_DOUBLE")
endif()
if(ENABLE_PPC32_LWSYNC)
    set(PPC32_LWSYNC_ENABLE "CK_MD_PPC32_LWSYNC")
else()
    set(PPC32_LWSYNC_ENABLE "CK_MD_PPC32_LWSYNC_DISABLE")
endif()
if(ENABLE_RTM)
    set(RTM_ENABLE "CK_MD_RTM_ENABLE")
else()
    set(RTM_ENABLE "CK_MD_RTM_DISABLE")
endif()
if(ENABLE_LSE)
    set(LSE_ENABLE "CK_MD_LSE_ENABLE")
else()
    set(LSE_ENABLE "CK_MD_LSE_DISABLE")
endif()
if(DISABLE_SSE)
    set(SSE_DISABLE "CK_MD_SSE_DISABLE")
else()
    set(SSE_DISABLE "CK_MD_SSE_ENABLE")
endif()

if(NOT PROFILE)
    set(PROFILE "${PLATFORM}")
endif()
set(PLATFORM "__${PLATFORM}__")

find_program(GZIP gzip)
if(NOT GZIP)
    set(GZIP "cat")
    set(GZIP_SUFFIX "")
else()
    set(GZIP "${GZIP} -c")
    set(GZIP_SUFFIX ".gz")
endif()

include(CheckIncludeFile)
foreach(header stdbool.h stddef.h stdint.h string.h)
    check_include_file(${header} HAVE_${header})
    if(NOT HAVE_${header})
        message(FATAL_ERROR "${header} not found")
    endif()
endforeach()

file(GLOB SOURCES "${SRC_DIR}/src/*.c")

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

if(NOT DISABLE_SHARED)
    add_library(ck_shared SHARED ${SOURCES})
    set_target_properties(ck_shared PROPERTIES 
        OUTPUT_NAME "ck"
        SOVERSION ${VERSION_MAJOR}
        VERSION ${VERSION}
    )
    target_compile_options(ck_shared PRIVATE ${CFLAGS} ${PTHREAD_CFLAGS})
    target_link_options(ck_shared PRIVATE ${LDFLAGS})
    if(SYSTEM STREQUAL "darwin")
        set_target_properties(ck_shared PROPERTIES 
            OUTPUT_NAME "ck.${VERSION}"
            SUFFIX ".dylib"
            LINK_FLAGS "-Wl,-install_name,${LIBRARY}/libck.${VERSION_MAJOR}.dylib"
        )
    else()
        target_link_options(ck_shared PRIVATE -Wl,-soname,${LDNAME_MAJOR})
    endif()

    target_include_directories(ck_shared PUBLIC
        $<BUILD_INTERFACE:${SRC_DIR}/include>
        $<INSTALL_INTERFACE:include/ck>
    )
endif()

if(NOT DISABLE_STATIC)
    add_library(ck_static STATIC ${SOURCES})
    set_target_properties(ck_static PROPERTIES OUTPUT_NAME "ck")
    target_compile_options(ck_static PRIVATE ${CFLAGS} ${PTHREAD_CFLAGS})

    target_include_directories(ck_static PUBLIC
        $<BUILD_INTERFACE:${SRC_DIR}/include>
        $<INSTALL_INTERFACE:include/ck>
    )
endif()

# Geração de arquivos com configure_file
file(MAKE_DIRECTORY ${BUILD_DIR}/include ${BUILD_DIR}/include/freebsd ${BUILD_DIR}/src ${BUILD_DIR}/doc ${BUILD_DIR}/build)
if(NOT BUILD_DIR STREQUAL SRC_DIR)
    file(MAKE_DIRECTORY ${BUILD_DIR}/regressions)
endif()

configure_file(${SRC_DIR}/include/ck_md.h.in ${SRC_DIR}/include/ck_md.h @ONLY)
configure_file(${SRC_DIR}/include/freebsd/ck_md.h.in ${SRC_DIR}/include/freebsd/ck_md.h @ONLY)
configure_file(${SRC_DIR}/build_flags/ck.build.in ${BUILD_DIR}/build/ck.build @ONLY)
configure_file(${SRC_DIR}/build_flags/regressions.build.in ${BUILD_DIR}/build/regressions.build @ONLY)
configure_file(${SRC_DIR}/build_flags/ck.pc.in ${BUILD_DIR}/build/ck.pc @ONLY)
configure_file(${SRC_DIR}/build_flags/ck.spec.in ${BUILD_DIR}/build/ck.spec @ONLY)

install(DIRECTORY ${SRC_DIR}/include/ 
        DESTINATION ${HEADERS}
        FILES_MATCHING PATTERN "*.h")
if(NOT DISABLE_SHARED)
    install(TARGETS ck_shared LIBRARY DESTINATION ${LIBRARY})
endif()
if(NOT DISABLE_STATIC)
    install(TARGETS ck_static ARCHIVE DESTINATION ${LIBRARY})
endif()
if(NOT DISABLE_SHARED AND NOT SYSTEM STREQUAL "darwin")
    install(CODE "execute_process(COMMAND ${CMAKE_COMMAND} -E create_symlink ${LDNAME_VERSION} ${LIBRARY}/${LDNAME})")
    install(CODE "execute_process(COMMAND ${CMAKE_COMMAND} -E create_symlink ${LDNAME_VERSION} ${LIBRARY}/${LDNAME_MAJOR})")
endif()
install(FILES ${BUILD_DIR}/build/ck.pc DESTINATION ${PKGCONFIG_DATA})

add_custom_target(doc
    COMMAND make -C ${BUILD_DIR}/doc all
    WORKING_DIRECTORY ${BUILD_DIR}/doc
)

add_custom_target(regressions
    COMMAND make -C ${BUILD_DIR}/regressions all
    WORKING_DIRECTORY ${BUILD_DIR}/regressions
)

add_custom_target(check
    COMMAND make -C ${BUILD_DIR}/regressions check
    WORKING_DIRECTORY ${BUILD_DIR}/regressions
    DEPENDS regressions
)

add_custom_target(dist
    COMMAND git archive --format=tar --prefix=ck-${VERSION}/ HEAD | ${GZIP} > ${BUILD_DIR}/ck-${VERSION}.tar${GZIP_SUFFIX}
    WORKING_DIRECTORY ${SRC_DIR}
)

add_custom_target(distclean
    COMMAND make clean
    COMMAND ${CMAKE_COMMAND} -E remove 
        ${BUILD_DIR}/include/ck_md.h
        ${BUILD_DIR}/include/freebsd/ck_md.h
        ${BUILD_DIR}/src/Makefile
        ${BUILD_DIR}/doc/Makefile
        ${BUILD_DIR}/build/ck.build
        ${BUILD_DIR}/build/regressions.build
        ${BUILD_DIR}/build/ck.pc
        ${BUILD_DIR}/build/ck.spec
        ${BUILD_DIR}/Makefile
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

if(PROFILE STREQUAL "unknown")
    message(WARNING "Your target architecture is not a first-class citizen. The test suite may not work as intended.")
endif()