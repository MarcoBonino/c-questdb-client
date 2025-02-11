cmake_minimum_required(VERSION 3.15.0)
project(c-questdb-client VERSION 1.0.0)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)

include(CTest)
enable_testing()

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(LIBECC_SOURCE_DIR "${CMAKE_BINARY_DIR}/libecc-src")
set(LIBECC_BINARY_DIR "${CMAKE_BINARY_DIR}/libecc-build")
if(CMAKE_BUILD_TYPE MATCHES Debug)
    set(LIBECC_DEBUG "debug")
endif()
include(ExternalProject)
ExternalProject_Add(
    libecc
    GIT_REPOSITORY    https://github.com/ANSSI-FR/libecc.git
    GIT_TAG           v0.9.5.1
    SOURCE_DIR        ${LIBECC_SOURCE_DIR}
    BINARY_DIR        ${LIBECC_BINARY_DIR}
    CONFIGURE_COMMAND ""
    BUILD_COMMAND        export BUILD_DIR=${LIBECC_BINARY_DIR}
                      && export WITH_DYNAMIC_LIBS=1
                      && make -j8 -C ${LIBECC_SOURCE_DIR} ${LIBECC_DEBUG} ${LIBECC_BINARY_DIR}/libsign.a ${LIBECC_BINARY_DIR}/libsign.so
    INSTALL_COMMAND      mv ${LIBECC_BINARY_DIR}/libsign.a ${CMAKE_BINARY_DIR}/libsign.a
                      && mv ${LIBECC_BINARY_DIR}/libsign.so ${CMAKE_BINARY_DIR}/libsign.so
    TEST_COMMAND      ""
)
add_definitions(-DWITH_STDLIB=1)
set(LIBECC_EXT_DEPS
    ${LIBECC_SOURCE_DIR}/src/external_deps/print.c
    ${LIBECC_SOURCE_DIR}/src/external_deps/rand.c
)
set_source_files_properties(${LIBECC_EXT_DEPS} PROPERTIES GENERATED TRUE)

set(LIB_SOURCES
    src/aborting_malloc.c
    src/line_sender.c
    src/mem_writer.c
    src/utf8.c
    src/base64.c
    src/authentication.c
)

function(set_compile_flags TARGET_NAME)
    if(MSVC)
        # We disable warning C5105 via `/wd5105`
        # to work around non-C11-compliant
        # code in WinBase.h which is included from
        # #include <winsock2.h>
        # See: https://docs.microsoft.com/en-us/cpp/
        #   error-messages/compiler-warnings/c5105?view=msvc-170
        # And: https://developercommunity2.visualstudio.com/t/
        #   std:c17-generates-warning-compiling-Win/1249671?preview=true
        # The warning is not applicable in new releases of the Windows SDK.
        target_compile_options(
            ${TARGET_NAME} PRIVATE
            /W4 /WX $<$<COMPILE_LANGUAGE:C>:/wd5105>)
    else()
        target_compile_options(
            ${TARGET_NAME} PRIVATE
            -Wall -Wextra -Wpedantic -Werror)
    endif()
endfunction()

# Shared Library
add_library(
    c_questdb_client SHARED
    ${LIB_SOURCES})
target_include_directories(
    c_questdb_client
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${LIBECC_SOURCE_DIR}/src
)
set_compile_flags(c_questdb_client)
target_compile_definitions(
    c_questdb_client
    PUBLIC
    LINESENDER_DYN_LIB)
if(WIN32)
    set_target_properties(
        c_questdb_client
        PROPERTIES
        DEFINE_SYMBOL "LINESENDER_EXPORTS")
    target_link_libraries(c_questdb_client wsock32 ws2_32)
endif(WIN32)
if(UNIX)
    target_compile_options(
        c_questdb_client PRIVATE
        -fvisibility=hidden)
endif(UNIX)

# Static Library
add_library(
    c_questdb_client_static STATIC
    ${LIB_SOURCES})
target_include_directories(
    c_questdb_client_static
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${LIBECC_SOURCE_DIR}/src
)
set_compile_flags(c_questdb_client_static)
if(WIN32)
    target_link_libraries(c_questdb_client_static wsock32 ws2_32)
endif(WIN32)

# Static Library w/ -fPIC
if(UNIX)
    add_library(
        c_questdb_client_static_pic STATIC
        ${LIB_SOURCES})
    set_property(
        TARGET c_questdb_client_static_pic
        PROPERTY POSITION_INDEPENDENT_CODE ON)
    target_include_directories(
        c_questdb_client_static_pic
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${LIBECC_SOURCE_DIR}/src
    )
    set_compile_flags(c_questdb_client_static_pic)
endif(UNIX)

# Examples

add_executable(
    line_sender_c_example
    examples/line_sender_c_example.c
    ${LIBECC_EXT_DEPS}
    )
target_link_libraries(
    line_sender_c_example
    c_questdb_client_static)  # example depending on static lib.
target_link_libraries(
    line_sender_c_example
    ${CMAKE_BINARY_DIR}/libsign.a)

add_executable(
    line_sender_cpp_example
    examples/line_sender_cpp_example.cpp
    ${LIBECC_EXT_DEPS}
    )
target_link_libraries(
    line_sender_cpp_example
    c_questdb_client)  # example depending on dynamic lib.
target_link_libraries(
    line_sender_cpp_example
    ${CMAKE_BINARY_DIR}/libsign.so)

# Unit test binary.
add_executable(
    test_line_sender
    test/mock_server.cpp
    test/test_line_sender.cpp
    ${LIBECC_EXT_DEPS}
)
target_link_libraries(
    test_line_sender
    c_questdb_client_static)
target_link_libraries(
    test_line_sender
    ${CMAKE_BINARY_DIR}/libsign.a)
target_include_directories(
    test_line_sender
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
set_compile_flags(test_line_sender)
add_test(
    NAME test_line_sender
    COMMAND test_line_sender)

# System testing Python3 script.
# This will download the latest QuestDB instance from Github,
# thus will also require a Java 11 installation to run the tests.
option(QUESTDB_SYSTEM_TESTING "Run system tests" OFF)
if(QUESTDB_SYSTEM_TESTING)
    find_package(
        Python3
        REQUIRED
        COMPONENTS Interpreter)
    find_package(
        Java
        11
        REQUIRED)
    add_test(
        NAME system_test
        COMMAND python3 ${CMAKE_CURRENT_SOURCE_DIR}/system_test/test.py)
    set_tests_properties(
        system_test PROPERTIES
        ENVIRONMENT BUILD_DIR_PATH=${CMAKE_BINARY_DIR})
endif(QUESTDB_SYSTEM_TESTING)
