cmake_minimum_required(VERSION 2.8)
project(cinatra)

find_package (Threads)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread -std=c++17")

find_package(Boost 1.55 REQUIRED COMPONENTS system filesystem)
find_package(OpenSSL REQUIRED)

include_directories(
        ${Boost_INCLUDE_DIR}
        ${OPENSSL_INCLUDE_DIR}
        "${CMAKE_SOURCE_DIR}/libcinatra_http"
        "${CMAKE_SOURCE_DIR}/libcinatra"
        "${CMAKE_SOURCE_DIR}/iguana")

if (WIN32)
    set(EXTRA_LIB mswsock ws2_32)
endif ()

find_package(Boost 1.55 COMPONENTS coroutine context system thread filesystem REQUIRED)


set(CINATRA_FILES
        libcinatra/cinatra/middleware/cookies.hpp
        libcinatra/cinatra/middleware/session.hpp
        libcinatra/cinatra/aop.hpp
        libcinatra/cinatra/cinatra.h
        libcinatra/cinatra/context_container.hpp
        libcinatra/cinatra/function_traits.hpp
        libcinatra/cinatra/router.h
        libcinatra/router.cpp libcinatra/cinatra/http_router.hpp
        )

set(CINATRA_HTTP_FILES
        libcinatra_http/cinatra_http/connection.hpp
        libcinatra_http/cinatra_http/http_parser.h
        libcinatra_http/cinatra_http/http_server.h
        libcinatra_http/cinatra_http/io_service_pool.h
        libcinatra_http/cinatra_http/mime_types.h
        libcinatra_http/cinatra_http/multipart_parser.h
        libcinatra_http/cinatra_http/picohttpparser.h
        libcinatra_http/cinatra_http/request.h
        libcinatra_http/cinatra_http/response.h
        libcinatra_http/cinatra_http/utils.h
        libcinatra_http/cinatra_http/websocket.h
        libcinatra_http/http_parser.c
        libcinatra_http/http_server.cpp
        libcinatra_http/io_service_pool.cpp
        libcinatra_http/mime_types.cpp
        libcinatra_http/multipart_parser.c
        libcinatra_http/picohttpparser.c
        libcinatra_http/request.cpp
        libcinatra_http/response.cpp
        libcinatra_http/utils.cpp
        libcinatra_http/websocket.cpp)

set(CINATRA_TEST_FILES
        cinatra_test/main.cpp)

add_library(cinatra_http STATIC ${CINATRA_HTTP_FILES})

add_library(cinatra STATIC ${CINATRA_FILES})

add_executable(cinatra_test ${CINATRA_TEST_FILES})
target_link_libraries(cinatra_test cinatra cinatra_http ${Boost_LIBRARIES} ${OPENSSL_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT} ${EXTRA_LIB})

install(TARGETS cinatra DESTINATION lib)
install(TARGETS cinatra_http DESTINATION lib)
install(DIRECTORY libcinatra/cinatra DESTINATION include)
install(DIRECTORY libcinatra_http/cinatra_http DESTINATION include)
install(DIRECTORY cmake DESTINATION share/cinatra)
