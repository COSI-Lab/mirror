# IMPORTANT:
# Replace --uid 1000 and --gid 1000 with the uid and gid of the mirror user

# Build
FROM ubuntu:24.04 as builder
RUN apt update && apt install -y g++ cmake libzmq3-dev ninja-build git
WORKDIR /mirror
COPY ./src /mirror/src
COPY ./lib /mirror/lib
COPY ./CMakeLists.txt /mirror/CMakeLists.txt
RUN cmake -S/mirror -B/mirror/build -G Ninja
RUN cmake --build /mirror/build --target clean
RUN cmake --build /mirror/build --target all

# Run
FROM ubuntu:24.04
RUN apt update && apt install -y python3 curl wget
RUN apt install -y libzmq3-dev rsync
WORKDIR /mirror
COPY --from=builder /mirror/build/syncScheduler .
RUN chmod 744 syncScheduler
# Port for listening for manual syncs
EXPOSE 9281
ENTRYPOINT ["./syncScheduler"]
