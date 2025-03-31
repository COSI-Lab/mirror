file(GLOB HEADER_LIST CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/include/*.hpp")

add_executable(sync_scheduler)

target_sources(sync_scheduler
PRIVATE
    main.cpp
    SyncScheduler.cpp
    Schedule.cpp
    SyncDetails.cpp
    JobManager.cpp
    ${HEADER_LIST}
)

target_include_directories(sync_scheduler
PRIVATE
    ../include
)

target_link_libraries(sync_scheduler
PRIVATE 
    nlohmann_json::nlohmann_json
    spdlog::spdlog
    cppzmq
)

target_compile_features(sync_scheduler
PRIVATE
    cxx_std_23
)

set_target_properties(sync_scheduler 
PROPERTIES
    CXX_STANDARD 23
#    CXX_CLANG_TIDY "${CLANG_TIDY_COMMAND}"
)

