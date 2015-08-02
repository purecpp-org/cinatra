SET(TEST_SOURCES
	test.cpp
	request_parser/request_parser.cpp
	utils/case_map.cpp
	utils/ncase_multi_map.cpp
	response/response.cpp
	router/router.cpp
	string_utils/string_utils.cpp
)

ADD_EXECUTABLE(cinatra_test ${TEST_SOURCES})
INCLUDE_DIRECTORIES(../include)

IF(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
	TARGET_LINK_LIBRARIES(cinatra_test boost_system-mt pthread boost_coroutine-mt boost_thread-mt)
ELSE(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
	TARGET_LINK_LIBRARIES(cinatra_test boost_system pthread boost_coroutine boost_thread)
ENDIF(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")

ADD_CUSTOM_TARGET(run_test COMMAND ${CMAKE_BINARY_DIR}/test/cinatra_test DEPENDS cinatra_test WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
