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
                   && ./bootstrap
                   && ./configure --prefix=${CMAKE_BINARY_DIR}/gnunet
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
    "${CMAKE_SOURCE_DIR}/src" # TODO: temporary
    "${GNUNET_BIN_DIR}/include")

file(GLOB sources
    "${CMAKE_SOURCE_DIR}/example/*.cpp"
    "${CMAKE_SOURCE_DIR}/src/*.cpp")

add_executable(gnunet-channels ${sources})
add_dependencies(gnunet-channels gnunet)

target_link_libraries(gnunet-channels
    ${GNUNET_BIN_DIR}/lib/libgnunethello.so
    ${GNUNET_BIN_DIR}/lib/libgnunettransport.so
    ${GNUNET_BIN_DIR}/lib/libgnunetutil.so
    ${GNUNET_BIN_DIR}/lib/libgnunetcadet.so
    ${Boost_LIBRARIES})

################################################################################
