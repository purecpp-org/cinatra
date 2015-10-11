ADD_EXECUTABLE(multi_compile_uint main.cpp dummy.cpp)

if (ENABLE_OPENSSL)
	ADD_DEFINITIONS(-DCINATRA_ENABLE_HTTPS)
endif()

TARGET_LINK_LIBRARIES(multi_compile_uint ${EXTRA_LIBS})