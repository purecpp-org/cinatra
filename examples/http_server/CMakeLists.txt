ADD_EXECUTABLE(http_server main.cpp)
INCLUDE_DIRECTORIES(../../include)
TARGET_LINK_LIBRARIES(http_server boost_system pthread boost_coroutine boost_thread)
