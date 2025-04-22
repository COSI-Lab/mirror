add_executable(stats-bot)

target_sources(stats-bot
PRIVATE
    main.cpp
    stats.cpp
    bot.cpp
)

target_include_directories(stats-bot
PRIVATE
    ../include
)

target_link_libraries(stats-bot
PRIVATE
    curl
    dpp
    spdlog::spdlog
    cppzmq
)

set_target_properties(stats-bot 
PROPERTIES
    CXX_STANDARD 23
    CXX_STANDARD_REQUIRED ON
)
