#SET(CMAKE_VERBOSE_MAKEFILE ON)
#SET(CMAKE_CXX_COMPILER clang++)
#ADD_COMPILE_OPTIONS("-std=c++1y")
#ADD_COMPILE_OPTIONS("-Wall")
#ADD_COMPILE_OPTIONS("-Wextra")
SUBDIRS(
	http_server
	cinatra
	hello_world
	echo
)
