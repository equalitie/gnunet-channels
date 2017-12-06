cmake_minimum_required (VERSION 3.5)
set(GLOB BOOST_VERSION 1.55)
include(ExternalProject)
################################################################################
# NOTE: https://stackoverflow.com/questions/37603238/fsanitize-not-using-gold-linker-in-gcc-6-1
set(SANITIZE "-fsanitize=address -fuse-ld=gold")
################################################################################
set(GNUNET_SRC_DIR "${CMAKE_BINARY_DIR}/gnunet-src")
set(GNUNET_BIN_DIR "${CMAKE_BINARY_DIR}/gnunet")

# Useful for debugging GNUnet itself.
if(NOT ALWAYS_BUILD_GNUNET)
    set(ALWAYS_BUILD_GNUNET 0)
endif()

externalproject_add(gnunet
    DOWNLOAD_COMMAND rsync -ru ${CMAKE_SOURCE_DIR}/gnunet/ ${GNUNET_SRC_DIR}/
    BUILD_ALWAYS ${ALWAYS_BUILD_GNUNET}
    CONFIGURE_COMMAND cd ${GNUNET_SRC_DIR}
                   && sh -c "[ -f ./configure ] || ./bootstrap"
                   && sh -c "[ -f Makefile ] || ./configure --prefix=${GNUNET_BIN_DIR}"
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
enable_testing()

# To run these tests, execute: 'make test ARGS="-V"'
add_test(basic_tests
    ${CMAKE_SOURCE_DIR}/tests/run_test.sh "${CMAKE_SOURCE_DIR}"
                                          "${CMAKE_BINARY_DIR}")

################################################################################
