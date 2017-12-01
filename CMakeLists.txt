cmake_minimum_required (VERSION 3.5)
set(GLOB BOOST_VERSION 1.55)
include(ExternalProject)
################################################################################
set(SANITIZE "-fsanitize=address")
################################################################################
set(GNUNET_SRC_DIR "${CMAKE_SOURCE_DIR}/gnunet")
set(GNUNET_BIN_DIR "${CMAKE_BINARY_DIR}/gnunet")

if(NOT BUILD_GNUNET)
    set(BUILD_GNUNET 0)
endif()

externalproject_add(gnunet
    SOURCE_DIR ${IPFS_CACHE_SRC_DIR}
    DOWNLOAD_COMMAND ""
    BUILD_ALWAYS ${BUILD_GNUNET}
    CONFIGURE_COMMAND cd ${GNUNET_SRC_DIR}
                   && sh -c "[ -f ./configure ] || ./bootstrap"
                   && sh -c "[ -f Makefile ] || ./configure --prefix=${CMAKE_BINARY_DIR}/gnunet"
    BUILD_COMMAND $(MAKE) -C ${GNUNET_SRC_DIR}
    INSTALL_COMMAND $(MAKE) install -C ${GNUNET_SRC_DIR}
    PREFIX gnunet-build)

################################################################################
project(gnunet-channels)

find_package(Boost ${BOOST_VERSION} COMPONENTS thread system coroutine REQUIRED)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14 -Wall -ggdb ${SANITIZE}")

include_directories(
    "${Boost_INCLUDE_DIR}"
    "${CMAKE_SOURCE_DIR}/include"
    "${GNUNET_BIN_DIR}/include")

file(GLOB sources
    "${CMAKE_SOURCE_DIR}/src/*.cpp")

add_library(gnunet-channels ${sources})
add_dependencies(gnunet-channels gnunet)

################################################################################
project(chat)

find_package(Boost ${BOOST_VERSION} COMPONENTS thread system coroutine REQUIRED)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14 -Wall -ggdb ${SANITIZE}")

include_directories(
    "${Boost_INCLUDE_DIR}"
    "${CMAKE_SOURCE_DIR}/include"
    "${GNUNET_BIN_DIR}/include")

file(GLOB sources
    "${CMAKE_SOURCE_DIR}/example/chat.cpp")

add_executable(chat ${sources})
add_dependencies(chat gnunet-channels)

target_link_libraries(chat
    ${CMAKE_BINARY_DIR}/libgnunet-channels.a
    ${GNUNET_BIN_DIR}/lib/libgnunethello.so
    ${GNUNET_BIN_DIR}/lib/libgnunettransport.so
    ${GNUNET_BIN_DIR}/lib/libgnunetutil.so
    ${GNUNET_BIN_DIR}/lib/libgnunetcadet.so
    ${Boost_LIBRARIES}
    ${CMAKE_THREAD_LIBS_INIT})

################################################################################
project(tests)

find_package(Boost ${BOOST_VERSION} COMPONENTS system unit_test_framework thread coroutine REQUIRED)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14 -Wall -ggdb ${SANITIZE}")

include_directories(
    "${Boost_INCLUDE_DIR}"
    "${CMAKE_SOURCE_DIR}/include"
    "${GNUNET_BIN_DIR}/include")

file(GLOB sources
    "${CMAKE_SOURCE_DIR}/tests/test.cpp")

# TODO: How to add this define only for this project?
add_definitions(-DBOOST_TEST_DYN_LINK)
add_executable(tests ${sources})
add_dependencies(tests gnunet-channels)

target_link_libraries(tests
    ${CMAKE_BINARY_DIR}/libgnunet-channels.a
    ${GNUNET_BIN_DIR}/lib/libgnunethello.so
    ${GNUNET_BIN_DIR}/lib/libgnunettransport.so
    ${GNUNET_BIN_DIR}/lib/libgnunetutil.so
    ${GNUNET_BIN_DIR}/lib/libgnunetcadet.so
    ${Boost_LIBRARIES}
    ${CMAKE_THREAD_LIBS_INIT})

################################################################################
