cmake_minimum_required(VERSION 3.20...3.30)

#find_program(CLANG_TIDY_EXE NAMES "clang-tidy-20" "clang-tidy" REQUIRED)
#set(CLANG_TIDY_COMMAND ${CLANG_TIDY_EXE} --header-filter=${CMAKE_SOURCE_DIR}/include)


set(CPPZMQ_BUILD_TESTS OFF CACHE INTERNAL "")

project(syncScheduler
    LANGUAGES CXX
)

include(cmake/fetch_deps.cmake)

add_subdirectory(src)

