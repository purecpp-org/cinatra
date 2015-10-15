SET(TEST_SOURCES
	test.cpp
	request_parser/request_parser.cpp
	utils/case_map.cpp
	utils/ncase_multi_map.cpp
	utils/timer.cpp
	response/response.cpp
	router/router.cpp
	string_utils/string_utils.cpp
    body_parser/body_parser.cpp
    context_container/context_container.cpp
)

ADD_EXECUTABLE(cinatra_test ${TEST_SOURCES})

TARGET_LINK_LIBRARIES(cinatra_test ${EXTRA_LIBS})

ADD_CUSTOM_TARGET(run_test COMMAND ${CMAKE_BINARY_DIR}/test/cinatra_test DEPENDS cinatra_test WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
