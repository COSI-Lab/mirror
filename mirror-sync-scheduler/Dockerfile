# IMPORTANT:
# Replace --uid 1000 and --gid 1000 with the uid and gid of the mirror user

# Build
FROM ubuntu:24.04 as builder
RUN apt update && apt install -y g++ cmake libzmq3-dev ninja-build git
WORKDIR /mirror
COPY src src
COPY include include
COPY cmake cmake
COPY CMakeLists.txt CMakeLists.txt
RUN cmake -B build -G Ninja
RUN cmake --build build --target all

# Run
FROM ubuntu:24.04
RUN apt update && apt install -y python3 curl wget libzmq3-dev rsync zsh
WORKDIR /mirror
COPY --from=builder /mirror/build/src/sync_scheduler .
RUN chmod 744 sync_scheduler
ENTRYPOINT ["./sync_scheduler"]
