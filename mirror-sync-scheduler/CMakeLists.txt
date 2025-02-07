cmake_minimum_required(VERSION 3.20...3.30)

find_program(CLANG_TIDY_EXE NAMES "clang-tidy" REQUIRED)
set(CLANG_TIDY_COMMAND ${CLANG_TIDY_EXE} --config-file=${CMAKE_SOURCE_DIR}/../.clang-tidy --header-filter=${CMAKE_SOURCE_DIR}/include)

project(syncScheduler
    LANGUAGES CXX
)

include(cmake/fetch_deps.cmake)

add_subdirectory(lib/mirror-logging)
add_subdirectory(src)

