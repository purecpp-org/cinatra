# - Find cinatra
#
#  cinatra_INCLUDES  - List of cinatra includes
#  cinatra_LIBRARIES - List of libraries when using cinatra.
#  cinatra_FOUND     - True if cinatra found.
#  cinatra_http_INCLUDES  - List of cinatra_http includes
#  cinatra_http_LIBRARIES - List of libraries when using cinatra_http.
#  cinatra_http_FOUND     - True if cinatra_http found.


# Look for the header file.
find_path(cinatra_INCLUDE NAMES cinatra_http/http_server.h cinatra/cinatra.h
        PATHS /opt/local/include /usr/local/include /usr/include $ENV{cinatra_ROOT}/include
        DOC "Path in which the file cinatra/cinatra.h is located." )

# Look for the library.
find_library(cinatra_http_LIBRARY NAMES cinatra_http
        PATHS /opt/local/include /usr/local/include /usr/include $ENV{cinatra_ROOT}/include
        DOC "Path to cinatra_http library." )
find_library(cinatra_LIBRARY NAMES cinatra
        PATHS /opt/local/include /usr/local/include /usr/include $ENV{cinatra_ROOT}/include
        DOC "Path to cinatra library." )

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(cinatra DEFAULT_MSG cinatra_INCLUDE cinatra_LIBRARY cinatra_http_LIBRARY)

if(cinatra_FOUND)
    message(STATUS "Found cinatra libraries (include: ${cinatra_INCLUDE}, library: ${cinatra_LIBRARY} ${cinatra_http_LIBRARY})")
    set(cinatra_INCLUDES ${cinatra_INCLUDE})
    set(cinatra_LIBRARIES ${cinatra_LIBRARY} ${cinatra_http_LIBRARY})
    mark_as_advanced(cinatra_INCLUDE cinatra_LIBRARY)
endif()