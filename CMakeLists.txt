cmake_minimum_required(VERSION 3.10)
project(Network)

########## SETUP ##########
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR})
set(NET_SRC_DIR "${PROJECT_SOURCE_DIR}/src")
set(NET_HDR_DIR "${PROJECT_SOURCE_DIR}/include")

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
add_library( ${PROJECT_NAME} STATIC
    ${NET_SRC_DIR}/temp.cpp
    ${NET_SRC_DIR}/NetworkSocket.cpp
    ${NET_SRC_DIR}/Address.cpp
    ${NET_SRC_DIR}/NetworkUtils.cpp   
    ${NET_SRC_DIR}/PacketSerializer.cpp
    ${NET_SRC_DIR}/PacketFactory.cpp
    ${NET_SRC_DIR}/ProtocolManager.cpp
)

target_include_directories(${PROJECT_NAME}
    PUBLIC
        ${NET_HDR_DIR}
)

# Link jsoncpp library
find_package(PkgConfig REQUIRED)
pkg_check_modules(JSONCPP REQUIRED jsoncpp)
target_link_libraries(${PROJECT_NAME} ${JSONCPP_LIBRARIES})
target_include_directories(${PROJECT_NAME} PRIVATE ${JSONCPP_INCLUDE_DIRS})
