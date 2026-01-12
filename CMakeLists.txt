cmake_minimum_required(VERSION 3.10)
project(Network)
include(FetchContent)

########## SETUP ##########
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR})
set(NET_SRC_DIR "${PROJECT_SOURCE_DIR}/src")
set(NET_HDR_DIR "${PROJECT_SOURCE_DIR}/include")
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

########## GENERATED FILES ##########
set(GENERATED_HEADER "${NET_HDR_DIR}/generated_messages.hpp")
set(GENERATED_SOURCE "${NET_SRC_DIR}/generated_messages.cpp")

set_source_files_properties(
    ${GENERATED_HEADER}
    ${GENERATED_SOURCE}
    PROPERTIES GENERATED TRUE
)

########## OPTIONS ##########
option(ENABLE_NET_TESTS "Build tests along with the library" OFF)
option(ENABLE_NET_COVERAGE "Enable coverage reporting" OFF)

########## TESTING ##########
if(ENABLE_NET_COVERAGE)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --coverage -fprofile-arcs -ftest-coverage")
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --coverage")
    endif()
endif()

if (ENABLE_NET_TESTS)
    enable_testing()
    add_subdirectory(tests/unit_tests)
endif ()

########## NETWORK ##########
set(NETWORK_SOURCES
    ${NET_SRC_DIR}/NetworkSocket.cpp
    ${NET_SRC_DIR}/Address.cpp
    ${NET_SRC_DIR}/NetworkUtils.cpp
    ${NET_SRC_DIR}/ProtocolManager.cpp
    ${NET_SRC_DIR}/Server.cpp
    ${NET_SRC_DIR}/Client.cpp
)

if (EXISTS ${GENERATED_SOURCE})
    list(APPEND NETWORK_SOURCES ${GENERATED_SOURCE})
endif()

add_library( ${PROJECT_NAME} STATIC ${NETWORK_SOURCES})

target_include_directories(${PROJECT_NAME}
    PUBLIC
        ${NET_HDR_DIR}
)

set_target_properties(${PROJECT_NAME} PROPERTIES
    OBJECT_DEPENDS "${GENERATED_HEADER};${GENERATED_SOURCE}"
)

########## JSONCPP ##########
set(JSONCPP_WITH_TESTS OFF CACHE BOOL "Disable JsonCpp tests" FORCE)
set(JSONCPP_WITH_POST_BUILD_UNITTEST OFF CACHE BOOL "Disable JsonCpp post build unittest" FORCE)
FetchContent_Declare(
    jsoncpp
    GIT_REPOSITORY https://github.com/open-source-parsers/jsoncpp.git
    GIT_TAG 1.9.6
)
FetchContent_MakeAvailable(jsoncpp)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
        jsoncpp_lib
)
