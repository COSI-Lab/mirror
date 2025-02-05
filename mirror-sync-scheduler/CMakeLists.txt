cmake_minimum_required(VERSION 3.22)
project(syncScheduler)

add_executable(${PROJECT_NAME}
    src/main.cpp
    src/schedule.cpp
    src/queue.cpp
)

set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 23)

include(FetchContent)

FetchContent_Declare(json
    GIT_REPOSITORY https://github.com/nlohmann/json
    GIT_TAG v3.11.3
)
FetchContent_MakeAvailable(json)

add_subdirectory(lib/mirror-logging)

target_link_libraries(${PROJECT_NAME} 
    PUBLIC mirror-logging
    PRIVATE nlohmann_json::nlohmann_json
)
