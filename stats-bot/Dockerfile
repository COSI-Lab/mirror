FROM ubuntu:24.04 AS builder
RUN apt-get update && apt-get install -y \
    git \
    clang++-19 \
    cmake \
    libssl-dev \
    ninja-build \
    curl \
    libcurl4 \
    libcurl4-openssl-dev \
    libopus0 \
    libz-dev \
    libzmq3-dev
WORKDIR /stats-bot
COPY ./cmake cmake
COPY ./CMakeLists.txt CMakeLists.txt
COPY ./src src
COPY ./include include
RUN CXX=clang++-19 cmake -B build -G Ninja
RUN cmake --build build

FROM ubuntu:24.04 AS runner
RUN apt-get update && apt-get install -y \
    libcurl4 \
    iputils-ping \
    curl \
    libopus0 \
    libzmq3-dev
COPY --from=builder \
    /stats-bot/build/src/stats-bot \
    /stats-bot/bin/stats-bot
COPY --from=builder \
    /stats-bot/build/_deps/dpp-build/library/libdpp.so \
    /lib/libdpp.so
COPY --from=builder \
    /stats-bot/build/_deps/spdlog-build/libspdlog.so \
    /lib/libspdlog.so
COPY .env /stats-bot/.env
RUN ldconfig
ENTRYPOINT ["/stats-bot/bin/stats-bot"]
