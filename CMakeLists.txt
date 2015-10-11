CMAKE_MINIMUM_REQUIRED(VERSION 2.8.12)
PROJECT(cinatra)

OPTION(ENABLE_OPENSSL "Enable use of OpenSSL for https" ON)

SET(CMAKE_VERBOSE_MAKEFILE ON)


find_package(Boost 1.57 COMPONENTS coroutine context system  REQUIRED)

include_directories(${Boost_INCLUDE_DIRS})
include_directories(include)
set(EXTRA_LIBS ${Boost_LIBRARIES})

if (ENABLE_OPENSSL)
	find_package(OpenSSL)
	include_directories(${OPENSSL_INCLUDE_DIR})
	set(EXTRA_LIBS ${EXTRA_LIBS} ${OPENSSL_LIBRARIES})
	MESSAGE(STATUS "Using OpenSSL ${OPENSSL_VERSION}")
endif()


ADD_COMPILE_OPTIONS("-std=c++11")
ADD_COMPILE_OPTIONS("-Wall")
ADD_COMPILE_OPTIONS("-Wextra")

if (WIN32)
	set(EXTRA_LIBS ${EXTRA_LIBS} ws2_32 mswsock)
endif()

SUBDIRS(
	examples
	test
)

ADD_CUSTOM_TARGET(debug
	COMMAND ${CMAKE_COMMAND} -DCMAKE_BUILD_TYPE=Debug ${CMAKE_SOURCE_DIR}
	COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target all
	COMMENT "Switch CMAKE_BUILD_TYPE to Debug"

)
ADD_CUSTOM_TARGET(release
	COMMAND ${CMAKE_COMMAND} -DCMAKE_BUILD_TYPE=Release ${CMAKE_SOURCE_DIR}
	COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target all
	COMMENT "Switch CMAKE_BUILD_TYPE to Release"
)
