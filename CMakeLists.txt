cmake_minimum_required(VERSION 3.15)
project(hats VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Library
add_library(hats_lib
	src/graph.cpp
	src/knowledge.cpp
	src/solver.cpp
)
target_include_directories(hats_lib PUBLIC include)

include(cmake/Sanitizers.cmake)
add_sanitizers(hats_lib)

# Apple Clang ships without OpenMP; help CMake find Homebrew's libomp.
if(APPLE)
    execute_process(
        COMMAND brew --prefix libomp
        OUTPUT_VARIABLE HOMEBREW_LIBOMP_PREFIX
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET)
    if(HOMEBREW_LIBOMP_PREFIX)
        set(OpenMP_CXX_FLAGS "-Xpreprocessor -fopenmp -I${HOMEBREW_LIBOMP_PREFIX}/include")
        set(OpenMP_CXX_LIB_NAMES "omp")
        set(OpenMP_omp_LIBRARY "${HOMEBREW_LIBOMP_PREFIX}/lib/libomp.dylib")
    endif()
endif()

find_package(OpenMP)
if(OpenMP_CXX_FOUND)
    target_link_libraries(hats_lib PUBLIC OpenMP::OpenMP_CXX)
    message(STATUS "OpenMP ${OpenMP_CXX_VERSION} enabled")
else()
    message(STATUS "OpenMP not found — building single-threaded")
endif()

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

add_executable(test_solver tests/test_solver.cpp src/solver.cpp)
target_link_libraries(test_solver PRIVATE hats_lib)
add_sanitizers(test_solver)

enable_testing()
add_test(NAME GraphTests COMMAND test_graph)
add_test(NAME KnowledgeTests COMMAND test_knowledge)
add_test(NAME SolverTests COMMAND test_solver)

# Python bindings
find_package(pybind11 QUIET)
if(NOT pybind11_FOUND)
    include(FetchContent)
    FetchContent_Declare(
        pybind11
        GIT_REPOSITORY https://github.com/pybind/pybind11.git
        GIT_TAG        v2.13.6
    )
    FetchContent_MakeAvailable(pybind11)
endif()

pybind11_add_module(hats_py src/bindings.cpp)
target_link_libraries(hats_py PRIVATE hats_lib)
