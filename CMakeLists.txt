cmake_minimum_required(VERSION 3.15)
project(hats VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Library
add_library(hats_lib
	src/graph.cpp
	src/knowledge.cpp
)
target_include_directories(hats_lib PUBLIC include)

include(cmake/Sanitizers.cmake)
add_sanitizers(hats_lib)

# Application
add_executable(hats_app apps/main.cpp)
target_link_libraries(hats_app PRIVATE hats_lib)
add_sanitizers(hats_app)

# Tests
add_executable(test_graph tests/test_graph.cpp)
target_link_libraries(test_graph PRIVATE hats_lib)
add_sanitizers(test_graph)

add_executable(test_knowledge tests/test_knowledge.cpp)
target_link_libraries(test_knowledge PRIVATE hats_lib)
add_sanitizers(test_knowledge)

enable_testing()
add_test(NAME GraphTests COMMAND test_graph)
add_test(NAME KnowledgeTests COMMAND test_knowledge)
