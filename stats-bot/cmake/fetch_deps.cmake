include(FetchContent)

FetchContent_Declare(json
    GIT_REPOSITORY https://github.com/nlohmann/json
    GIT_TAG v3.11.3
)
FetchContent_MakeAvailable(json)

FetchContent_Declare(dpp
    GIT_REPOSITORY https://github.com/brainboxdotcc/DPP
    GIT_TAG v10.1.0
)
FetchContent_MakeAvailable(dpp)

FetchContent_Declare(spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog
    GIT_TAG v1.15.1
)
FetchContent_MakeAvailable(spdlog)

FetchContent_Declare(cppzmq
    GIT_REPOSITORY https://github.com/zeromq/cppzmq
    GIT_TAG v4.10.0
)
FetchContent_MakeAvailable(cppzmq)