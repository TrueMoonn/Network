cmake_minimum_required(VERSION 3.10)
project(Network)
include(FetchContent)

########## SETUP ##########
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR})
set(NET_SRC_DIR "${PROJECT_SOURCE_DIR}/src")
set(NET_HDR_DIR "${PROJECT_SOURCE_DIR}/include")
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

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

########## PROTOCOL GENERATION ##########
# Chemins des fichiers générés
set(GENERATED_HEADER "${NET_HDR_DIR}/Network/generated_messages.hpp")
set(GENERATED_SOURCE "${NET_SRC_DIR}/generated_messages.cpp")
set(PROTOCOL_JSON "${PROJECT_SOURCE_DIR}/config/protocol.json")
set(GENERATOR_SCRIPT "${PROJECT_SOURCE_DIR}/tools/generate_protocol.py")

find_package(Python3 COMPONENTS Interpreter REQUIRED)

add_custom_command(
    OUTPUT
        ${GENERATED_HEADER}
        ${GENERATED_SOURCE}
    COMMAND ${Python3_EXECUTABLE} ${GENERATOR_SCRIPT} ${PROTOCOL_JSON}
    DEPENDS
        ${PROTOCOL_JSON}
        ${GENERATOR_SCRIPT}
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
    COMMENT "Generating protocol messages from ${PROTOCOL_JSON}"
    VERBATIM
)

add_custom_target(generate_protocol
    DEPENDS
        ${GENERATED_HEADER}
        ${GENERATED_SOURCE}
)

########## NETWORK ##########
add_library( ${PROJECT_NAME} STATIC
    ${NET_SRC_DIR}/NetworkSocket.cpp
    ${NET_SRC_DIR}/Address.cpp
    ${NET_SRC_DIR}/NetworkUtils.cpp
    ${NET_SRC_DIR}/ProtocolManager.cpp
    ${NET_SRC_DIR}/Server.cpp
    ${NET_SRC_DIR}/Client.cpp
    ${GENERATED_SOURCE}  # Fichier généré
)

add_dependencies(${PROJECT_NAME} generate_protocol)

target_include_directories(${PROJECT_NAME}
    PUBLIC
        ${NET_HDR_DIR}
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
