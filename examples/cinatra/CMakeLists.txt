ADD_EXECUTABLE(cinatra main.cpp)
INCLUDE_DIRECTORIES(../../include)
TARGET_LINK_LIBRARIES(cinatra boost_system pthread boost_coroutine boost_thread)
