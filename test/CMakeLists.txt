SET(TEST_SOURCES
	test.cpp
	request_parser/request_parser.cpp
	utils/case_map.cpp
	utils/ncase_multi_map.cpp
	response/response.cpp
	router/router.cpp
	string_utils/string_utils.cpp
    body_parser/body_parser.cpp
)

ADD_EXECUTABLE(cinatra_test ${TEST_SOURCES})
INCLUDE_DIRECTORIES(../include)

IF(APPLE)
    TARGET_LINK_LIBRARIES(cinatra_test boost_system-mt pthread boost_coroutine-mt boost_context-mt boost_thread-mt)
ELSEIF(WIN32)
    TARGET_LINK_LIBRARIES(cinatra_test boost_system pthread boost_coroutine boost_context boost_thread ws2_32 mswsock)
ELSE()
    TARGET_LINK_LIBRARIES(cinatra_test boost_system pthread boost_coroutine boost_context boost_thread)
ENDIF()

ADD_CUSTOM_TARGET(run_test COMMAND ${CMAKE_BINARY_DIR}/test/cinatra_test DEPENDS cinatra_test WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
